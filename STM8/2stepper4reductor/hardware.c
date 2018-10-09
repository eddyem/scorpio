/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.c
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
 *
 */
#include "hardware.h"

volatile unsigned long Global_time = 0L; // global time in ms
U8 MCU_no = 0; // unit number read from onboard jumpers

void hw_init(){
    CFG_GCR |= 1; // disable SWIM
    // Configure clocking
    CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
// Timer 4 (8 bit) used as system tick timer
    // prescaler == 128 (2^7), Tfreq = 125kHz
    // period = 1ms, so ARR = 125
    TIM4_PSCR = 7;
    TIM4_ARR = 125;
    // interrupts: update
    TIM4_IER = TIM_IER_UIE;
    // auto-reload + interrupt on overflow + enable
    TIM4_CR1 = TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN;

    // GPIO; ODR-write, IDR-read, DDR-direction, CR1-pullup/pushpull, CR2-exti/speed
    PB_DDR = 0x0f; // motor0
    PB_CR1 = 0x0f;
    PC_DDR = LED_PIN | PWM_PINS; // LED, PWM
    PC_CR1 = LED_PIN | PWM_PINS;
    PC_CR2 = PWM_PINS;
    PD_DDR = 0x0f; // motor 1
    PD_CR1 = 0x0f;
    // end-switches
    PORT(M0E1_PORT, CR1) |= M0E1_PIN;
    PORT(M0E2_PORT, CR1) |= M0E2_PIN;
    PORT(M1E1_PORT, CR1) |= M1E1_PIN;
    PORT(M1E2_PORT, CR1) |= M1E2_PIN;

    // default state setters
    LED_OFF();

    // Setup watchdog
    IWDG_KR = KEY_ENABLE; // start watchdog
    IWDG_KR = KEY_ACCESS; // enable access to protected registers
    IWDG_PR = 6; // /256
    IWDG_RLR = 0xff; // max time for watchdog (1.02s)
    IWDG_KR = KEY_REFRESH;

    // get board address
    MCU_no = GET_ADDR();

    // Configure timer 1 - PWM outputs
    // prescaler = f_{in}/f_{tim1} - 1
    // set Timer1 to 0.2MHz: 16/.2 - 1 = 79 (p-channel mosfet is too slow!)
    TIM1_PSCRH = 0;
    TIM1_PSCRL = 79; // LSB should be written last as it updates prescaler
    // auto-reload each 256ticks:
    TIM1_ARRH = 0x0;
    TIM1_ARRL = 0xFF;
    // P-channel opendrain mosfets are closed
    TIM1_CCR1H = 0;
    TIM1_CCR1L = 0;
    TIM1_CCR2H = 0;
    TIM1_CCR2L = 0;
    TIM1_CCR3H = 0;
    TIM1_CCR3L = 0;
    // interrupts: none
    // PWM mode 2 - OC1M = 111
    TIM1_CCMR1 = 0x70; TIM1_CCMR2 = 0x70; TIM1_CCMR3 = 0x70;
    TIM1_CCER1 = 0x11; // CC1E, CC2E
    TIM1_CCER2 = 0x01; // CC3E
    // auto-reload
    TIM1_CR1 = TIM_CR1_APRE | TIM_CR1_CEN;
    TIM1_BKR |= 1<<7; // MOE - enable main output
}
