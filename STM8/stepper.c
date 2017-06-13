/*
 * stepper.c
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "ports_definition.h"
#include "uart.h"
#include "stepper.h"

volatile U8 chk_esw = 0; // need 2 check end-switches

static const U8 usteps_matrix[8][8] = {
    {0b1000, 0b1100, 0b0100, 0b0110, 0b0010, 0b0011, 0b0001, 0b1001}, // [1234]
    {0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001, 0b0001, 0b0011}, // [3214]
    {0b1000, 0b1001, 0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100}, // [1432]
    {0b1000, 0b1010, 0b0010, 0b0110, 0b0100, 0b0101, 0b0001, 0b1001}, // [1324]
    // inversion: cat | sed -e 's/0b/x/g' -e 's/0/y/g' -e 's/1/0/g' -e 's/y/1/g' -e s'/x/0b/g'
    {0b0111, 0b0011, 0b1011, 0b1001, 0b1101, 0b1100, 0b1110, 0b0110}, // [1234]
    {0b1101, 0b1001, 0b1011, 0b0011, 0b0111, 0b0110, 0b1110, 0b1100}, // [3214]
    {0b0111, 0b0110, 0b1110, 0b1100, 0b1101, 0b1001, 0b1011, 0b0011}, // [1432]
    {0b0111, 0b0101, 0b1101, 0b1001, 0b1011, 0b1010, 0b1110, 0b0110}, // [1324]
};

volatile int Steps_left = 0;  // Number of steps
volatile char Dir = 0;     // direction of moving: 0/1
U16 Stepper_speed = 0;     // length of one MICROstep in us
U8 *usteps = &usteps_matrix[0][0];
U8 cur_motor = 7;
/**
 * Setup pins of stepper motor (all - PP out)
 */
void setup_stepper_pins(){
    // Push-pull for outputs
    // PB0..3
    PORT(STP_PORT, DDR) |= STP_PINS;
    PORT(STP_PORT, CR1) |= STP_PINS;
    PORT(STP_PORT, CR2) |= STP_PINS;
    // PD0..4
    PORT(STP_SEL_PORT, DDR) |= STP_SEL_PINS;
    PORT(STP_SEL_PORT, CR1) |= STP_SEL_PINS;
    PORT(STP_SEL_PORT, CR2) |= STP_SEL_PINS;

    STPRS_OFF();
}

U8 stepper_ch_speed(char *spd){
    int newval;
    if(readInt(spd, &newval)){
        if(newval > -9 && newval < 0x7fff){
            U16 O = 0xffff / (newval + 10);
            if(O < MIN_STEP_LENGTH) return 0;
            Stepper_speed = O;
            // Configure timer 2 to generate signals for CLK
            TIM2_PSCR = 4; // 1MHz
            TIM2_ARRH = O >> 8; // set speed
            TIM2_ARRL = O & 0xff;
            TIM2_IER = TIM_IER_UIE; // update interrupt enable
            TIM2_CR1 |= TIM_CR1_APRE | TIM_CR1_URS; // auto reload + interrupt on overflow & RUN
            #ifdef EBUG
            uart_write("Speed changed to ");
            printUint((U8*)&O, 2);
            uart_write("\n");
            #endif
            return 1;
        }else DBG("Bad speed value\n");
    }
    return 0;
}

/**
 * Check endswitches
 * @return 0 if none pressed, 1 if "-", 2 if "+"
 */
U8 check_endsw(){
    // A1 - "-", A2 - "+"
    U8 pc = PORT(ESW_PORT, IDR);
    if(0 == (pc & ESW_MINUS)) return 1;
    if(0 == (pc & ESW_PLUS)) return 2;
    return 0;
}

/**
 * move stepper number Nmotor by Nsteps steps
 * @return  1 if all OK, 0 if error occured
 */
U8 stepper_move(U8 Nmotor, int Nsteps){
    U8 c;
    if(!Nmotor || Nmotor > 6 || !Nsteps || Steps_left) return 0;

    if(Nsteps < 0){
        Dir = 1;
        Nsteps *= -1;
    }else
        Dir = 0;
    Steps_left = Nsteps;
    STPRS_OFF();

    // turn all OFF
    // turn on the motor we need
    PORT(STP_SEL_PORT, ODR) |= (1 << (Nmotor/2));
    if(Nmotor & 1) PORT(STP_SEL_PORT, ODR) &= ~GPIO_PIN3;
    else PORT(STP_SEL_PORT, ODR) &= ~GPIO_PIN4;
    c = check_endsw();
    cur_motor = Nmotor;
    if(c){
        if(c == 1){if(!Dir) c = 0;}
        else if(Dir) c = 0;
    }
    if(c){
        stop_motor();
        return 0; // already at end-switch in given direction
    }
    DBG("stepper_move\n");
    TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
    return 1;
}

void stop_motor(){
    TIM2_CR1 &= ~TIM_CR1_CEN; // Turn off timer
    Steps_left = 0;
    chk_esw = 1;
    DBG("stop\n");
}

/**
 * User can change current stepper phases table
 * N - position in table from 'a' (0) to 'h' (7)
 * return 1 if all OK
 */
U8 chk_stpr_cmd(char N){
    if(N < 'a' || N > 'h') return 0;
    usteps = &usteps_matrix[N-'a'][0];
    return 1;
}

/**
 * get end-switches state for all motors or only Nth
 * @param Nmotor - number of given motor
 */
void stepper_get_esw(U8 Nmotor){
    U8 sw;
    char str[] = "[2 0 St=0]\n"; // 3 - motor number, 5 - endswitch (3 if none)
    if(Nmotor == 0 || Nmotor > 7) return; // no running motor
    STPRS_OFF();
    PORT(STP_SEL_PORT, ODR) |= (1 << (Nmotor/2));
    if(Nmotor & 1) PORT(STP_SEL_PORT, ODR) |= 1<<4;
    else PORT(STP_SEL_PORT, ODR) |= 1<<5;
    str[3] = Nmotor + '0';
    sw = check_endsw();
    if(sw == 0) sw = 3;
    str[8] = sw + '0';
    uart_write(str);
    STPRS_OFF();
    cur_motor = 7;
}
