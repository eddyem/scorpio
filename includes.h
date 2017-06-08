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

// #define ()  do{}while(0)
#if defined (__AVR_ATmega8535__)
    // original MCU
    #define STPRS_OFF() do{PORTD |= 0xfc; PORTC |= 0x0f;}while(0)
    #define DDRAB       DDRA
    #define PORTAB      PORTA
    #define LED1_PIN    (_BV(0))
    #define LED2_PIN    (_BV(1))
    #define LED3_PIN    (_BV(2))
    #define FLAT_PIN    (_BV(5))
    #define NEON_PIN    (_BV(6))
    #define SHTR_PIN    (_BV(7))
#else
    // arduino devboard with stepper
    #define STPRS_OFF() do{PORTD |= 0xfc; PORTC &= 0xf0;}while(0)
    #define DDRAB       DDRB
    #define PORTAB      PORTB
    #define FLAT_PIN    (_BV(0))
    #define NEON_PIN    (_BV(1))
    #define SHTR_PIN    (_BV(2))
    #define LED1_PIN    (_BV(3))
    #define LED2_PIN    (_BV(4))
    #define LED3_PIN    (_BV(5))
#endif

#define PORTAB_PINS (FLAT_PIN | NEON_PIN | SHTR_PIN | LED1_PIN | LED2_PIN | LED3_PIN)

#endif // __INCLUDES_H__
