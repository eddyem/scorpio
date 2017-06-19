/*
 * blinky.c
 *
 * Copyright 2014 Edward V. Emelianoff <eddy@sao.ru>
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

#include "ports_definition.h"
#include "interrupts.h"
#include "stepper.h"
#include "uart.h"
#include "proto.h"

//U32 Global_time = 0L; // global time in ms

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
//const U8 ustepsUNI[8] = {8, 12, 4, 6, 2, 3, 1, 9}; // ULN - unipolar
//const U8 ustepsBIP[8] = {8, 10, 2, 6, 4, 5, 1, 9}; // bipolar
// current usteps
//U8 const *usteps = ustepsUNI;

int main() {
    //unsigned long T = 0L;
    // int Ival;
    // U8 rb;
    CLK_ICKR |= 8; // enable LSI for watchdog
    CFG_GCR |= 1; // disable SWIM
    // Configure clocking
    CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz


    PORT(LEDS_PORT, DDR) |= LEDS_PINS;
    PORT(LEDS_PORT, CR1) |= LEDS_PINS;
    PORT(LEDS_PORT, CR2) |= LEDS_PINS;

    // Configure timer 1 - LEDs
    // prescaler = f_{in}/f_{tim1} - 1
    // set Timer1 to 1MHz: 1/1 - 1 = 15
    TIM1_PSCRH = 0;
    TIM1_PSCRL = 15; // LSB should be written last as it updates prescaler
    // auto-reload each 256ticks:
    TIM1_ARRH = 0x0;
    TIM1_ARRL = 0xFF;
    TIM1_CCR1H = 0;
    TIM1_CCR1L = 1; // Minimal brightness
    TIM1_CCR2H = 0;
    TIM1_CCR2L = 1;
    TIM1_CCR3H = 0;
    TIM1_CCR3L = 1;

    // interrupts: none
    // PWM mode 1 - OC1M = 110
    TIM1_CCMR1 = 0x60; TIM1_CCMR2 = 0x60; TIM1_CCMR3 = 0x60;
    TIM1_CCER1 = 0x11; // CC1E, CC2E
    TIM1_CCER2 = 0x01; // CC3E
    // auto-reload + enable
    TIM1_CR1 = TIM_CR1_APRE | TIM_CR1_CEN;
    TIM1_BKR |= 1<<7; // MOE - enable main output

    // Configure pins
    // PD5 - UART2_TX
    PORT(UART_PORT, DDR) |= UART_TX_PIN;
    PORT(UART_PORT, CR1) |= UART_TX_PIN;
    PORT(UART_PORT, CR2) |= UART_TX_PIN;

    // Configure UART
    // 8 bit, no parity, 1 stop (UART_CR1/3 = 0 - reset value)
    // 9600 on 16MHz: BRR1=0x41, BRR2=0x02
    UART2_BRR2 = 0x03; UART2_BRR1 = 0x68;
    UART2_CR2 = UART_CR2_TEN | UART_CR2_REN | UART_CR2_RIEN; // Allow RX/TX, generate ints on rx//tx

    Stepper_speed = 1000;
    // Configure timer 2 to generate signals for CLK
    TIM2_PSCR = 4; // 1MHz
    TIM2_ARRH = 1000 >> 8; // set speed
    TIM2_ARRL = 1000 & 0xff;
    TIM2_IER = TIM_IER_UIE; // update interrupt enable
    TIM2_CR1 |= TIM_CR1_APRE | TIM_CR1_URS; // auto reload + interrupt on overflow

    setup_stepper_pins();
    RELAY_SETUP();
    // setup endswitch selection pins
    PORT(ESW_SEL_PORT, DDR) |= ESW_SEL_PINS;
    PORT(ESW_SEL_PORT, CR1) |= ESW_SEL_PINS;
    // Pullup to esw inputs
    PORT(ESW_PORT, CR1) |= ESW_PINS;

    // enable all interrupts
    enableInterrupts();


    // Setup watchdog
    IWDG_KR = KEY_ENABLE; // start watchdog
    IWDG_KR = KEY_ACCESS; // enable access to protected registers
    IWDG_PR = 6; // /256
    IWDG_RLR = 0xff; // max time for watchdog (1.02s)

    // Loop
    uart_write("Scorpio platform ready\n");
    if(RST_SR) RST_SR = 0x1f; // clear reset flags writing 1
    do{
        IWDG_KR = KEY_REFRESH; // refresh watchdog
        if(uart_rdy){
            process_string();
        }
        if(chk_esw){
            chk_esw = 0;
            stepper_get_esw(cur_motor);
        }
    }while(1);
}


