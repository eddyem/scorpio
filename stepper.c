/*
 *                                                                                                  geany_encoding=koi8-r
 * stepper.c
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */

#include "includes.h"

/*
 * Half-step mode:
 * D |----|----|    |    |    |    |    |----|
 * D |    |    |----|----|----|----|----|    |
 * C |    |----|----|----|    |    |    |    |
 * C |----|    |    |    |----|----|----|----|
 * B |    |    |    |----|----|----|    |    |
 * B |----|----|----|    |    |    |----|----|
 * A |    |    |    |    |    |----|----|----|
 * A |----|----|----|----|----|    |    |    |
 *
 * In full-step mode pulse rises sequentally: D->C->B->A
 * 0 0000
 * 1 0001
 * 2 0010
 * 3 0011
 * 4 0100
 * 5 0101
 * 6 0110
 * 7 0111
 * 8 1000
 * 9 1001
 *10 1010
 *11 1011
 *12 1100
 *13 1101
 *14 1110
 *15 1111
 */
// winding1: [13], winding2: [24]
// microsteps: [1234] = 1000, 1100, 0100, 0110, 0010, 0011, 0001, 1001 -- for ULN
// [1324] = 1000, 1010, 0010, 0110, 0100, 0101, 0001, 1001 - bipolar
// 1000, 1010, 0010, 0110, 0100, 0101, 0001, 1001 - half-step
// 1010, 0110, 0101, 1001 - full step
static const uint8_t usteps[8] = {8, 12, 4, 6, 2, 3, 1, 9}; // ULN - unipolar, active 1
//static const uint8_t usteps[8] = {7, 3, 11, 9, 13, 12, 14, 6}; // unipolar, active is 0

static int8_t Ustep = 0; // current microstep count
uint16_t Steps_left; // steps left to proceed (absolute value)
static uint8_t direction = 0; // ==1 if rotate CCW
static uint8_t cur_motor = 0; // current motor number

volatile uint8_t stepper_pulse = 0; // interrupt flag, used in main.c

static void stop_motors();

void stepper_setup(){
    TCCR1B |= _BV(WGM12); // configure timer1 for CTC mode, TOP is OCR1A
    OCR1A   = 2000; // set the CTC compare value - 1kHz
    TCCR1B |= _BV(CS11); // start the timer at 16MHz/8 = 2MHz
    //TCCR1B |= _BV(CS12) | _BV(CS10); // /1024 == 15625Hz
    //OCR1A   = 15625;
    TIMSK1 |= _BV(OCIE1A); // enable the CTC interrupt
    PORTC |= _BV(4) | _BV(5); // enable pullup
}

/**
 * Change TIM1 speed
 * Period = 4 * 65535/(spd + 10) microseconds
 */
uint8_t stepper_ch_speed(char *spd){
    int16_t newval;
    if(readInt(spd, &newval)){
        if(newval > -9 && newval < 0x7fff){
            uint16_t O = 0xffff / (newval + 10);
            TIMSK1 &= ~_BV(OCIE1A); // disable timer interrupt
            OCR1A = O;
            TCNT1 = 0; // reset counter
            TIMSK1 |= _BV(OCIE1A);
            #ifdef EBUG
            usart_send("Speed changed to ");
            printUint((uint8_t*)&O, 2);
            usart_send("\n");
            #endif
        }else DBG("Bad speed value\n");
    }
    return 0;
}

/**
 * Check endswitches
 * @return 0 if none pressed, 1 if "-", 2 if "+"
 */
static uint8_t check_endsw(){
    // PC4 - "-", PC5 - "+"
    uint8_t pc = PINC;
    if(0 == (pc & _BV(4))) return 1;
    if(0 == (pc & _BV(5))) return 2;
    return 0;
}

/**
 * move stepper number Nmotor by Nsteps steps
 * @return  1 if all OK, 0 if error occured
 */
uint8_t stepper_move(uint8_t Nmotor, int16_t Nsteps){
    if(!Nmotor || Nmotor > 6 || !Nsteps || Steps_left) return 0;
    TIMSK1 &= ~_BV(OCIE1A); // disable timer interrupt
    // turn all OFF
    STPRS_OFF();
    // turn on the motor we need
    PORTD &= ~(2 << Nmotor);
    uint8_t c = check_endsw();
    cur_motor = Nmotor;
    if(c){
        if(c == 1){if(Nsteps > 0) c = 0;}
        else if(Nsteps < 0) c = 0;
    }
    if(c){
        stop_motors();
        return 0; // already at end-switch in given direction
    }
    if(Nsteps < 0){ // CCW
        Nsteps = -Nsteps;
        direction = 1;
    }else direction = 0; // CW
    Steps_left = Nsteps;
    TCNT1 = 0; // reset counter
    TIMSK1 |= _BV(OCIE1A);
    return 1;
}

static void stop_motors(){
    stepper_get_esw(cur_motor);
    // turn off all pulses to place motor in free state & prevent undesirable behaviour
    STPRS_OFF();
    TIMSK1 &= ~_BV(OCIE1A); // disable timer interrupt
    stepper_pulse = 0;
    Steps_left = 0;
    Ustep = 0;
    cur_motor = 0;
}

/**
 * process stepper pulses generation @ timer event
 */
void stepper_process(){
    stepper_pulse = 0;
    uint8_t port = PORTC & 0xf0; // save old port state & clear clocking
    PORTC = port | usteps[Ustep];
    uint8_t sw = check_endsw(); // 1 - "-", 2 - "+", 0 - none
    if(direction){ // CCW
        if(--Ustep < 0){
            Ustep = 7;
            --Steps_left;
        }
        if(sw == 1){
            stop_motors();
            return;
        }
    }else{ // CW
        if(++Ustep > 7){
            Ustep = 0;
            --Steps_left;
        }
        if(sw == 2){
            stop_motors();
            return;
        }
    }
    if(Steps_left == 0) stop_motors();
}

/**
 * get end-switches state for all motors or only Nth
 * @param Nmotor - number of given motor
 */
void stepper_get_esw(uint8_t Nmotor){
    if(Nmotor == 0 || Nmotor > 7) return; // no running motor
    PORTD |= 0xfc;
    PORTD &= ~(2 << Nmotor); // [2 1 St=2]
    char str[] = "[2 0 St=0]\n"; // 3 - motor number, 5 - endswitch (3 if none)
    str[3] = Nmotor + '0';
    uint8_t sw = check_endsw();
    if(sw == 0) sw = 3;
    str[8] = sw + '0';
    usart_send(str);
}

/**
 * Timer 1 used to generate stepper pulses
 */
ISR(TIMER1_COMPA_vect){
    stepper_pulse = 1; // say that we can generate next microstep
}
