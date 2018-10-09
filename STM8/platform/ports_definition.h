/*
 * ports_definition.h - definition of ports pins & so on
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

#pragma once
#ifndef __PORTS_DEFINITION_H__
#define __PORTS_DEFINITION_H__

#include "stm8l.h"

#ifdef EBUG
    #define DBG(x) uart_write(x)
#else
    #define DBG(x)
#endif

// macro for using in port constructions like PORT(LED_PORT, ODR) = xx
#define CONCAT(a, b)    a ## _ ## b
#define PORT(a, b)      CONCAT(a , b)


// UART2_TX
#define UART_PORT       PD
#define UART_TX_PIN     GPIO_PIN5

/****** Relays: B4, B5, F4 ******/
#define RELAY_SETUP() do{PB_DDR |= 0x30; PB_CR1 |= 0x30; PF_DDR |= 0x10; PF_CR1 |= 0x10;}while(0)
#define RELAYS_OFF()  do{PB_ODR &= ~0x30; PF_ODR |= ~0x10;}while(0)
#define RELAY1_ON()   do{PB_ODR |= 0x10;}while(0)
#define RELAY1_OFF()  do{PB_ODR &= ~0x10;}while(0)
#define RELAY2_ON()   do{PB_ODR |= 0x20;}while(0)
#define RELAY2_OFF()  do{PB_ODR &= ~0x20;}while(0)
#define RELAY3_ON()   do{PF_ODR |= 0x10;}while(0)
#define RELAY3_OFF()  do{PF_ODR &= ~0x10;}while(0)
#define RELAY_1()     (PB_ODR & 0x10)
#define RELAY_2()     (PB_ODR & 0x20)
#define RELAY_3()     (PF_ODR & 0x10)


/***** LEDs (C1..C3) *****/
#define LEDS_PORT       PC
#define LEDS_PINS       0x0E

/***** Stepper motor *****/
// Clocking
// PB0..3 -- pins A..D of stepper
#define STP_PORT        PB
#define STP_PINS        0x0f

// PC5-PC7 - endswitch address (through multiplexer)
#define ESW_SEL_PORT    PC
#define ESW_SEL_PINS    0xe0

// PD0..PD4 - select pair 0..2 & stepper
#define STP_SEL_PORT  PD
#define STP_SEL_PINS  0x1f

#define STPRS_OFF()  do{PORT(STP_PORT, ODR) &= ~STP_PINS; PORT(STP_SEL_PORT, ODR) &= ~0x07; PORT(STP_SEL_PORT, ODR) |= 0x18; }while(0)

#define ESW_PORT    PA
#define ESW_PINS    0x06
// PA1 - "+", PA2 - "-"
#define ESW_PLUS    0x04
#define ESW_MINUS   0x02

#endif // __PORTS_DEFINITION_H__
