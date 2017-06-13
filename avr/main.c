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
/*
volatile uint16_t Milliseconds = 0, Seconds = 0, days = 0;

void print_time(){
    printUint((uint8_t*)&days, 2);
    usart_send("d");
    printUint((uint8_t*)&Seconds, 2);
    usart_send("s");
    printUint((uint8_t*)&Milliseconds, 2);
    usart_send("ms\n");
}*/

int main() {
    /** setup all other pins **/
    STPRS_OFF(); // turn off steppers before configuring to output
    DDRD = 0xfc; // steppers
    DDRC = 0x0f; // steppers diagram
    // 328p have no port A
    PORTAB |= FLAT_PIN | NEON_PIN | SHTR_PIN; // turn all off
    DDRAB = PORTAB_PINS;

    /** USART config **/
    // set baudrate (using macros from util/setbaud.h)
    #if !defined (__AVR_ATmega8535__)
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    #if USE_2X
    UCSR0A |= _BV(U2X0);
    #else
    UCSR0A &= ~(_BV(U2X0));
    #endif
    #else // __AVR_ATmega8535__
    UCSRA &= ~(_BV(U2X0));
    UBRRH = 0;
    UBRRL = 51;
    #endif
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);   // Enable RX and TX, enable RX interrupt

    /** setup timer 0 - system timer **/
    // set prescaler to 8 and start the timer (2MHz*256 = 0.128ms period)
    #if defined (__AVR_ATmega8535__)
    //TCCR0 |= _BV(CS01) | _BV(CS00); // /64
    TCCR0 |= _BV(CS01);
    #else
    //TCCR0B |= _BV(CS01) | _BV(CS00);
    TCCR0B |= _BV(CS01);
    #endif
    TIMSK0 |= _BV(TOIE0);

    stepper_setup();

    sei(); // enable interrupts

    wdt_enable(WDTO_2S); // start watchdog
    usart_send("Scorpio platform ready\n");

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
    static uint8_t shi_counter = 0;/* tick_ctr = 0;
    TCNT0 += 6; // 0.125ms period
    if(++tick_ctr == 8){
        tick_ctr = 0;
        if(++Milliseconds == 1000){
            Milliseconds = 0;
            if(++Seconds == 86400){
                Seconds = 0;
                ++days;
            }
        }
    }*/
    if(shi_counter == 0){ // turn all LEDs on
        PORTAB |= LED1_PIN | LED2_PIN | LED3_PIN;
    }
    // now check which LEDs we need to turn off
    if(shi_counter == LEDs[0]) PORTAB &= ~LED1_PIN;
    if(shi_counter == LEDs[1]) PORTAB &= ~LED2_PIN;
    if(shi_counter == LEDs[2]) PORTAB &= ~LED3_PIN;
    ++shi_counter;
//    #if defined (__AVR_ATmega8535__)
//    TCNT0 += 128;
//    #endif
}
