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

#define LED_PIN (_BV(5))


volatile uint16_t Milliseconds = 0, Seconds = 0, days = 0;

void print_time(){
    printUint((uint8_t*)&days, 2);
    usart_send("d");
    printUint((uint8_t*)&Seconds, 2);
    usart_send("s");
    printUint((uint8_t*)&Milliseconds, 2);
    usart_send("ms\n");
}

int main() {
    // LED for debug
    DDRB |= LED_PIN;
    /** setup all other pins **/
    PORTD |= 0xfc; // turn off steppers before configuring to output
    DDRD = 0xfc; // steppers
    PORTD |= 0x0f;
    DDRC = 0x0f; // steppers diagram
    // 328p have no port A
    #if defined (__AVR_ATmega8535__)
    DDRA = 0xe0; // flat, neon, shutter
    #endif


    /** USART config **/
    // set baudrate (using macros from util/setbaud.h)
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    #if USE_2X
    UCSR0A |= _BV(U2X0);
    #else
    UCSR0A &= ~(_BV(U2X0));
    #endif
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);   // Enable RX and TX, enable RX interrupt

    /** setup timer 0 - system timer **/
    // set prescaler to 64 and start the timer
    #if defined (__AVR_ATmega8535__)
    TCCR0 |= _BV(CS01) | _BV(CS00);
    #else
    TCCR0B |= _BV(CS01) | _BV(CS00);
    #endif
    TIMSK0 |= _BV(TOIE0);

    stepper_setup();

    sei(); // enable interrupts
    wdt_enable(WDTO_2S); // start watchdog

    while(1){
        wdt_reset();
        if(stepper_pulse) stepper_process();
    // testing blinking - remove later
    if(Milliseconds == 500) PORTB |= LED_PIN;
    else if(Milliseconds == 0) PORTB &= ~LED_PIN;
        if(usart_flags & U_RX_COMPLETE)
            process_string();
    }
    return 0;
}

ISR(TIMER0_OVF_vect){
    TCNT0 += 6;
    if(++Milliseconds == 1000){
        Milliseconds = 0;
        if(++Seconds == 86400){
            Seconds = 0;
            ++days;
        }
    }
}
