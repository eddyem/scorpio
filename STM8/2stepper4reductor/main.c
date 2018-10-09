/*
 * main.c
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru>
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
#include "hardware.h"
#include "interrupts.h"
#include "uart.h"
#include "proto.h"
#include "motors.h"

int main() {
        char A[3] = {'x', '\n', 0};
    unsigned long T = 0L;

    if(RST_SR) RST_SR = 0x1f; // clear reset flags writing 1
    hw_init();
    motors_init();
    uart_init();
    // enable all interrupts
    enableInterrupts();
    // remove this code if nesessary
        uart_write("\n\nHello! My address is ");
        A[0] = MCU_no + '0';
        uart_write(A);
        show_help(); // show protocol help @start
    // Loop
    do{
        /*if(Global_time - T > paused_val){
        }*/
        IWDG_KR = KEY_REFRESH; // refresh watchdog
        if(uart_rdy){
            process_string();
        }
        process_stepper(0);
        process_stepper(1);
    }while(1);
}


