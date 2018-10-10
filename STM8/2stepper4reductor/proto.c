/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.c - base protocol definitions
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
#include "proto.h"
#include "hardware.h"
#include "uart.h"
#include "motors.h"

// input command buffer
static char ibuf[UART_BUF_LEN];
// buffer for output data
static char obuf[UART_BUF_LEN] = "[ X "; // constant initializer

void show_help(){
    // uart_write transfer not more than UART_BUF_LEN bytes!
    //         "start                        end"
    uart_write("\n\n");
    uart_write("Command protocol: [ addr command");
    uart_write(" data ]\n\t broadcast addr: " BROADCAST_CHAR "\n");
    uart_write("commands:\n");
    uart_write("0/1 - command for given motor:\n");
        show_motors_help();
    uart_write("r - reset MCU\n");
    uart_write("G - get board address\n");
    uart_write("L 0/1 - LED on/off\n");
    uart_write("P ch val - PWM on channel ch\n");
    uart_write("T - time counter value\n\n");
}

static void set_PWM(char *cmd, char **buff){
    U8 s = 0;
    long l;
    U8 N = *cmd++ - '0';
    if(N > 2){
        long2buf(-1, buff); // error: answer with Nch = -1
        return;
    }
    *((*buff)++) = '0' + N;
    *((*buff)++) = ' ';
    if(*cmd == 0){ // check PWM value
        switch (N){
            case 0:
                s = TIM1_CCR1L;
            break;
            case 1:
                s = TIM1_CCR2L;
            break;
            case 2:
                s = TIM1_CCR3L;
            break;
        }
    }else{
        if(!readLong(cmd, &l) || l < 0 || l > 255){
            long2buf(-1, buff); // error: answer with PWM = -1
            return;
        }
        s = (U8) l;
        switch (N){
            case 0:
                TIM1_CCR1L = s;
            break;
            case 1:
                TIM1_CCR2L = s;
            break;
            case 2:
                TIM1_CCR3L = s;
            break;
        }
    }
    long2buf(s, buff);
}

// @return 0 in case of error
// @param cmd - string with command sequence
static U8 process_commands(char *cmd){
    char s, *bufptr = &obuf[4];
    static const char* const endline = " ]\n";
    IWDG_KR = KEY_REFRESH; // refresh watchdog
   // uart_write("got command: ");
   // uart_write(cmd);
    s = *cmd;
    obuf[2] = MCU_no + '0';
    if(s == '0' || s == '1'){
        motor_command(cmd, &bufptr);
        goto eof;
    }
    *bufptr++ = s;
    *bufptr++ = ' ';
    ++cmd;
    switch(s){
        case 'r':
            IWDG_KR = KEY_ACCESS;
            IWDG_PR = 0;
            IWDG_RLR = 0x1;
            IWDG_KR = KEY_REFRESH;
            while(1);
        break;
        case 'G':
            *bufptr++ = '0' + MCU_no;
        break;
        case 'L': // LED on/off
            if(*cmd){ // if there's no number after LED command - just check its state
                if(*cmd == '0'){
                    LED_OFF();
                }else{
                    LED_ON();
                }
            }
            *bufptr++ = LED_NSTATE() ? '0' : '1';
        break;
        case 'P':
            set_PWM(cmd, &bufptr);
        break;
        case 'T':
            *bufptr = 0;
            uart_write(obuf);
            printUint((U8*)&Global_time, 4);
            uart_write(endline);
            return 1;
        break;
        default:
            return 0;
    }
    eof:
    *bufptr = 0;
    uart_write(obuf);
    uart_write(endline);
    return 1;
}

void process_string(){
    U8 ctr;
    char *iptr = ibuf, *optr = &UART_rx[1];
    U8 mcuno = (U8)UART_rx[0] - '0';
    if(uart_rdy == 0) return;
    if(mcuno != MCU_no && mcuno != BROADCAST_ADDR){ // alien message
        uart_rdy = 0;
        rx_idx = 0;
        return;
    }
    // rx_idx is length of incoming message; next char is '\0', copy it too
    for(ctr = 0; ctr < rx_idx; ++ctr) *iptr++ = *optr++;
    rx_idx = 0; uart_rdy = 0; // command read, buffer ready to get more data
    if(!process_commands(ibuf)) show_help();
}

