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

U8 usteps[8] = {0b1000, 0b1010, 0b0010, 0b0110, 0b0100, 0b0101, 0b0001, 0b1001};

// numbers of motors in inner system:
// MOTOR6=5, MOTOR5=4, MOTOR4=0, MOTOR3=1, MOTOR2=2, MOTOR1=3
static motors_numbers[7] = {0,3,2,1,0,4,5};
// array of end-switches for appropriate motor number
// M1=e1, M2=e4, M3=e3, M4=e0, M5=e2, M6=e5
static U8 esw_arr[7] = {0, 1, 4, 3, 0, 2, 5};
#define ESW_SELECT(NUM) do{register U8 nsw = esw_arr[NUM]; register U8 C=PC_ODR & ~ESW_SEL_PINS; C |= ((nsw)<<5); PC_ODR = C;}while(0)

volatile int Steps_left = 0;  // Number of steps
volatile char Dir = 0;     // direction of moving: 0/1
U16 Stepper_speed = 0;     // length of one MICROstep in us
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
    // A1 - "+", A2 - "-"
    U8 i;
    U8 pc = PORT(ESW_PORT, IDR);
    for(i = 0; i < 255; ++i){ // wait a while for multiplexer
        if(pc != PORT(ESW_PORT, IDR)){
            pc = PORT(ESW_PORT, IDR);
            i = 0;
        }
    }
    if(0 == (pc & ESW_MINUS)) return 1;
    if(0 == (pc & ESW_PLUS)) return 2;
    return 0;
}

/**
 * move stepper number Nmotor by Nsteps steps
 * @return  1 if all OK, 0 if error occured
 */
U8 stepper_move(U8 Nmotor, int Nsteps){
    U8 c, nm;
    if(!Nmotor || Nmotor > 6 || !Nsteps || Steps_left) return 0;
    nm = motors_numbers[Nmotor];
    IWDG_KR = KEY_REFRESH; // refresh watchdog

    if(Nsteps < 0){
        Dir = 1;
        Nsteps *= -1;
    }else
        Dir = 0;
    Steps_left = Nsteps;
    // select endswitch
    ESW_SELECT(Nmotor);
    // turn all motors OFF
    STPRS_OFF();
    // turn on the motor we need
    PORT(STP_SEL_PORT, ODR) |= (1 << (nm/2));
    if(nm & 1) PORT(STP_SEL_PORT, ODR) &= ~GPIO_PIN4;
    else PORT(STP_SEL_PORT, ODR) &= ~GPIO_PIN3;
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
    // turn all motors OFF
    STPRS_OFF();
    Ustep = 0;
    Steps_left = 0;
    chk_esw = 1;
    DBG("stop\n");
}

/**
 * get end-switches state for all motors or only Nth
 * @param Nmotor - number of given motor
 */
void stepper_get_esw(U8 Nmotor){
    U8 sw;
    char str[] = "[2 0 St=0]\n"; // 3 - motor number, 5 - endswitch (3 if none)
    if(Nmotor == 0 || Nmotor > 6) return; // no running motor
    IWDG_KR = KEY_REFRESH; // refresh watchdog
    ESW_SELECT(Nmotor);
    if(chk_esw > 1){
        --chk_esw;
        return;
    }
    chk_esw = 0;
    str[3] = Nmotor + '0';
    sw = check_endsw();
    if(sw == 0) sw = 3;
    str[8] = sw + '0';
    uart_write(str);
    cur_motor = 0;
}

/**
 * prepare for end-switches selection:
 * switch multiplexer and set flag
 */
void prep2chkesw(U8 N){
    if(N == 0 || N > 6) return;
    ESW_SELECT(N);
    cur_motor = N;
    chk_esw = 255;
}
