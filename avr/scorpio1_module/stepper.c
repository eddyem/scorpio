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
//static const uint8_t usteps[8] = {8, 12, 4, 6, 2, 3, 1, 9}; // ULN - unipolar, active 1
//static const uint8_t usteps[8] = {7, 3, 11, 9, 13, 12, 14, 6}; // unipolar, active is 0
static const uint8_t usteps_matrix[8][8] = {
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

uint8_t const *usteps = usteps_matrix[1];

static int8_t Ustep = 0; // current microstep count
uint16_t Steps_left; // steps left to proceed (absolute value)
static uint8_t direction = 0; // ==1 if rotate CCW
static uint8_t cur_motor = 0; // current motor number

volatile uint8_t stepper_pulse = 0; // interrupt flag, used in main.c

void stepper_setup(){
    TCCR1B |= _BV(WGM12); // configure timer1 for CTC mode, TOP is OCR1A
    OCR1A   = 1000; // set the CTC compare value - 1kHz (125steps per second)
    TCCR1B |= _BV(CS11); // start the timer at 8MHz/8 = 1MHz
    TIMSK |= _BV(OCIE1A); // enable the CTC interrupt
}

/**
 * Change TIM1 speed
 * Period = 8 * 65535/(spd + 10) microseconds
 */
uint8_t stepper_ch_speed(char *spd){
    int16_t newval;
    if(readInt(spd, &newval)){
        if(newval > -9 && newval < 0x7fff){
            uint16_t O = 0xffff / (newval + 10);
            //TIMSK &= ~_BV(OCIE1A); // disable timer interrupt
            OCR1A = O;
            //TCNT1 = 0; // reset counter
            //TIMSK |= _BV(OCIE1A);
        }else{
            DBG("Bad speed value\n");
            return 1;
        }
    }
    return 0;
}

/**
 * Check endswitches
 * @return 0 if none pressed, 1 if "-", 2 if "+"
 */
static uint8_t check_endsw(uint8_t Nmotor){
    // PA1 - "-" - 1, PA0 - "+" - 2
    uint8_t pc = PINA, sw = 0;
//    if(0 == (pc & _BV(0))) sw = 1;
//    if(0 == (pc & _BV(1))) sw = 2;
    if(0 == (pc & _BV(0))) sw = 2;
    if(0 == (pc & _BV(1))) sw = 1;
    return sw;
}

/*
Steppers:
7 - turret1
6 - turret2
5 - analisator
4 - collimator
*/
/**
 * get end-switches state for all motors or only Nth
 * @param Nmotor - number of given motor
 */
void stepper_get_esw(uint8_t Nmotor){
    if(Nmotor < 1 || Nmotor > 4) return; // no running motor
    // [1 x St=y]
    char str[] = "[1 0 St=0]\n"; // 3 - motor number, 8 - endswitch (3 if none)
    str[3] = Nmotor + '0';
    uint8_t sw = check_endsw(Nmotor);
    if(sw == 0) sw = 3;
    str[8] = sw + '0';
    usart_send(str);
}

/**
 * move stepper number Nmotor by Nsteps steps
 * @return 0 if all OK, 1 if error occured
 */
uint8_t stepper_move(uint8_t Nmotor, int16_t Nsteps){
    if(Nmotor < 1 || Nmotor > 4 || Steps_left) return 1;
    TIMSK &= ~_BV(OCIE1A); // disable timer interrupt
    // turn all OFF
    STPRS_OFF();
    // turn on the motor we need
    PORTC |= (1 << (Nmotor+3));
    cur_motor = Nmotor;
    uint8_t c = check_endsw(Nmotor);
    if(!Nsteps){ // check endswitches
        stepper_get_esw(Nmotor);
        STPRS_OFF();
        cur_motor = 0;
        return 1;
    }
    if(c){
        if(c == 1){if(Nsteps > 0) c = 0;}
        else if(Nsteps < 0) c = 0;
    }
    if(c){
        stop_motors();
        return 1; // already at end-switch in given direction
    }
    if(Nsteps < 0){ // CCW
        Nsteps = -Nsteps;
        direction = 1;
    }else direction = 0; // CW
    Steps_left = Nsteps;
    TCNT1 = 0; // reset counter
    TIMSK |= _BV(OCIE1A);
    return 0;
}

void stop_motors(){
    stepper_get_esw(cur_motor);
    // turn off all pulses to place motor in free state & prevent undesirable behaviour
    STPRS_OFF();
    TIMSK &= ~_BV(OCIE1A); // disable timer interrupt
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
    uint8_t sw = check_endsw(cur_motor); // 1 - "-", 2 - "+", 0 - none
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
 * User can change current stepper phases table
 * N - position in table from 'a' (0) to 'h' (7)
 * return 0 if all OK
 */
uint8_t chk_stpr_cmd(char N){
    if(N < '0' || N > '7') return 1;
    usteps = usteps_matrix[N-'0'];
    return 0;
}

/**
 * Timer 1 used to generate stepper pulses
 */
ISR(TIMER1_COMPA_vect){
    stepper_pulse = 1; // say that we can generate next microstep
}

