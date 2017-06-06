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
// microsteps: DCBA = 1000, 1100, 0100, 0110, 0010, 0011, 0001, 1001 -- for ULN
// what a shit is this > DCBA = 0001, 0010, 0110, 1010, 1001, 1000, 0100, 0000  - bipolar
// 1000, 1010, 0010, 0110, 0100, 0101, 0001, 1001 - half-step
// 1010, 0110, 0101, 1001 - full step
static const uint8_t usteps[8] = {8, 12, 4, 6, 2, 3, 1, 9}; // ULN - unipolar
static volatile char Ustep = 0; // current microstep count

uint8_t stepper_pulse = 0;

void stepper_setup(){
    TCCR1B |= _BV(WGM12); // configure timer1 for CTC mode, TOP is OCR1A
    OCR1A   = 1000; // set the CTC compare value - 2kHz (means 1kHz)
    TCCR1B |= _BV(CS11); // start the timer at 16MHz/8 = 2MHz
    //TCCR1B |= _BV(CS12) | _BV(CS10); // /1024 == 15625Hz
    //OCR1A   = 15625;
    TIMSK1 |= _BV(OCIE1A); // enable the CTC interrupt
}

/**
 * Change TIM1 speed
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
            usart_send("Speed changed to ");
            printUint((uint8_t*)&O, 2);
            usart_send("\n");
        }else usart_send("Bad speed value\n");
    }
    return 0;
}

/**
 * Check endswitches
 * @return 0 if none pressed, 1 if "-", 2 if "+"
 */
static uint8_t check_endsw(){
    return 0;
}

/**
 * move stepper number Nmotor by Nsteps steps
 * @return  0 if all OK
 *          1 if Nmotor or Nsteps are bad values
 *          2 if motor already on endswitch in given direction
 */
uint8_t stepper_move(uint8_t Nmotor, int16_t Nsteps){
    if(!Nmotor || Nmotor > 6 || !Nsteps) return 1;
    // turn all OFF
    PORTD |= 0xfc;
    PORTC |= 0x0f;
    // turn on the motor we need
    PORTD &= 2 << Nmotor;
    uint8_t c = check_endsw();
    if(c){
        if(c == 1){if(Nsteps > 0) c = 0;}
        else if(Nsteps < 0) c = 0;
    }
    if(c){
        PORTD |= 0xfc;
        return 2; // already at end-switch in given direction
    }
    return 0;
}


static void stop_motor(uint8_t Nmotor){
    // turn off all pulses to place motor in free state & prevent undesirable behaviour
    PORTD |= 0xfc;
    PORTC |= 0x0f;
}

/**
 * process stepper pulses generation @ timer event
 */
void stepper_process(){
    stepper_pulse = 0;
    // change steps
    /*if(TIM2_SR1 & TIM_SR1_UIF){
        TIM2_SR1 &= ~TIM_SR1_UIF; // take off flag
        tmp = PORT(STP_PORT, ODR) & 0xf0;
        PORT(STP_PORT, ODR) = tmp | usteps[Ustep];
        if(Dir){
            if(++Ustep > 7){
                Ustep = 0;
                --Nsteps;
            }
        }else{
            if(--Ustep < 0){
                Ustep = 7;
                --Nsteps;
            }
        }
        if(Nsteps == 0){
            stop_motor();
        }
    }*/
}

/**
 * Timer 1 used to generate stepper pulses
 */
ISR(TIMER1_COMPA_vect){
    stepper_pulse = 1; // say that we can generate next microstep
}
