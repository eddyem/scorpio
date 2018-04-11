/*
 *                                                                                                  geany_encoding=koi8-r
 * main.c
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

volatile uint16_t Milliseconds = 0, Seconds = 0, days = 0;
volatile uint8_t tick_ctr = 0, hours = 0;


void print_time(){
    printUint((uint8_t*)&days, 2);
    usart_send("d");
    printUint((uint8_t*)&hours, 1);
    usart_send("h");
    printUint((uint8_t*)&Seconds, 2);
    usart_send("s");
    printUint((uint8_t*)&Milliseconds, 2);
    usart_send("ms\n");
}

int main() {
    DDRC = 0xff; // steppers diagram & stepper select
    DDRB = 0x07; // PB0..PB2 - pol/caret management etc
    STPRS_OFF(); // turn off steppers before configuring to output
    PORTB |= _BV(1);

    /** USART config **/
    // set baudrate
    UCSRA = 0;
    UCSRC = _BV(UCSZ1) | _BV(UCSZ0); // 8-bit data
    UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);   // Enable RX and TX, enable RX interrupt
    UBRRH = 0; // 9600
    UBRRL = 51;

    /** setup timer 0 - system timer **/
    // set prescaler to 8 and start the timer (1MHz*256 = 0.256ms period)
    TCCR0 |= _BV(CS01);
    TIMSK |= _BV(TOIE0);

    stepper_setup();

    sei(); // enable interrupts

    wdt_enable(WDTO_2S); // start watchdog
#ifdef EBUG
    usart_send("Scorpio module1 ready\n");
#endif
    while(1){
        wdt_reset();
        if(stepper_pulse) stepper_process();
        if(usart_flags & U_RX_COMPLETE)
            process_string();
    }
    return 0;
}

uint8_t LEDs[3] = {20,20,20}; // LEDs shining time

ISR(TIMER0_OVF_vect){
    //static uint8_t shi_counter = 0;
    TCNT0 += 6; // -> 0.250ms period
    if(++tick_ctr == 4){
        tick_ctr = 0;
        if(++Milliseconds == 1000){
            Milliseconds = 0;
            if(++Seconds == 3600){
                Seconds = 0;
                if(++hours == 24){
                    hours = 0;
                    ++days;
                }
            }
        }
    }
}
