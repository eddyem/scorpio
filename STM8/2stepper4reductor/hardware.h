/*
 * hardware.h - definition of ports pins & so on
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "stm8s.h"

extern volatile unsigned long Global_time; // global time in ms
extern U8 MCU_no;

// macro for using in port constructions like PORT(LED_PORT, ODR) = xx
#define FORMPORT(a, b)      a ## _ ## b
#define PORT(a, b)          FORMPORT(a , b)
#define CONCAT(a, b)        a ## b

/**
 * HW:
 * PB0-3 motor0 push-pull output
 * PB4   M0E1   pullup input
 * PB5   M0E2   pullup input
 * PC1-3 PWM    push-pull output
 * PC4 - LED    push-pull output
 * PC5-7 Addr   floating input (externall pull-down)
 * PD0-3 motor1 push-pull output
 * PD4   M1E1   pullup input
 * PD5 - Tx     open-drain output \ UART
 * PD6 - Rx     floating input    /
 * PD7   M1E2   pullup input
 */
// PWM
#define PWM_PINS        (7<<1)
// LED
#define LED_PORT        PC
#define LED_PIN         GPIO_PIN4
// Address
#define ADDR_MASK       (7<<5)
#define GET_ADDR()      ((PC_IDR & ADDR_MASK)>>5)

// UART2_TX
#define UART_PORT       PD
#define UART_TX_PIN     GPIO_PIN5

// steppers
#define STP0_PORT       PB
#define STP1_PORT       PD
#define STP_PINS        (0x0f)

// end-switches
#define M0E1_PORT       PB
#define M0E2_PORT       PB
#define M1E1_PORT       PD
#define M1E2_PORT       PD
#define M0E1_PIN        (1<<4)
#define M0E2_PIN        (1<<5)
#define M1E1_PIN        (1<<7)
#define M1E2_PIN        (1<<4)

// getters: 1 active, 0 inactive
// inverse state of LED
#define LED_NSTATE()     (PORT(LED_PORT, ODR) & LED_PIN)

// end-switches (0 - shuttered)
#define CHK_M0E1()      (0 == (PORT(M0E1_PORT, IDR) & M0E1_PIN))
#define CHK_M0E2()      (0 == (PORT(M0E2_PORT, IDR) & M0E2_PIN))
#define CHK_M1E1()      (0 == (PORT(M1E1_PORT, IDR) & M1E1_PIN))
#define CHK_M1E2()      (0 == (PORT(M1E2_PORT, IDR) & M1E2_PIN))

// setters
#define LED_OFF()       (PORT(LED_PORT, ODR) |= LED_PIN)
#define LED_ON()        (PORT(LED_PORT, ODR) &= ~LED_PIN)


// getters


void hw_init();

#endif // __HARDWARE_H__
