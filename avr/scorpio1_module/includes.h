/*
 *                                                                                                  geany_encoding=koi8-r
 * includes.h
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

#pragma once
#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include <avr/io.h> // IO ports
#include <avr/wdt.h> // WDT
#include <avr/interrupt.h>
#include <avr/io.h> // IO ports
#include <stdint.h> // int types
#include <util/setbaud.h> // baudrate calculation helper

#include "proto.h"
#include "uart.h"
#include "stepper.h"

#ifdef EBUG
#define DBG(x) usart_send(x)
#else
#define DBG(x)
#endif

// module 1
#define MODULEID '1'

#define STPRS_OFF() do{PORTC = 0;}while(0)

// #define ()  do{}while(0)
/*
    // original MCU
    #define STPRS_OFF() do{PORTD |= 0xfc; PORTC |= 0x0f;}while(0)
    #define DDRAB       DDRA
    #define PORTAB      PORTA
    // 8535 have common irq register for all timers
    #define TIMSK0      TIMSK
    #define TIMSK1      TIMSK
    #define UCSR0A      UCSRA
    #define UCSR0B      UCSRB
    #define UCSR0C      UCSRC
    #define UDRIE0      UDRIE
    #define FE0         FE
    #define UPE0        PE
    #define DOR0        DOR
    #define UDRIE0      UDRIE
    #define UDR0        UDR
    #define UBRR0H      UBRRH
    #define UBRR0L      UBRRL
    #define U2X0        U2X
    #define UCSZ01      UCSZ1
    #define UCSZ00      UCSZ0
    #define RXEN0       RXEN
    #define TXEN0       TXEN
    #define RXCIE0      RXCIE

    #define LED1_PIN    (_BV(0))
    #define LED2_PIN    (_BV(1))
    #define LED3_PIN    (_BV(2))
    #define FLAT_PIN    (_BV(5))
    #define NEON_PIN    (_BV(6))
    #define SHTR_PIN    (_BV(7))
    */

//#define PORTA_PINS (FLAT_PIN | NEON_PIN | SHTR_PIN | LED1_PIN | LED2_PIN | LED3_PIN)

#endif // __INCLUDES_H__
