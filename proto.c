/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.c - base protocol definitions
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

/**
 * Move motor for given amount of steps, cmd should be 'N nnnn any other symbols',
 * N - motor number,
 * nnnn - steps (-32768...32768)
 * @return 1 if all OK
 */
uint8_t move_motor(char *cmd){
    uint8_t N = (uint8_t)*cmd - '0';
    if(N < 1 || N > 6) return 0;
    cmd = omit_whitespace(cmd+1);
    int16_t steps;
    if(!readInt(cmd, &steps)) return 0;
    usart_send("Move motor ");
    printUint((uint8_t*)&N, 1);
    usart_send(" for ");
    print_long((uint32_t)steps);
    usart_send("steps\n");
    return stepper_move(N, steps);
}

extern void print_time();

/**
 * process commands from user buffer
 * @return 1 if all OK
 */
uint8_t process_commands(){
    char *cmd = omit_whitespace(&rx_buffer[1]);
    switch(*cmd){
        case 't':
            print_time();
            return 1;
        break;
        case '2':
            cmd = omit_whitespace(cmd + 1);
        break;
        default:
            return 0;
    }
    if(*cmd > '0' && *cmd < '7')
        return move_motor(cmd);
    switch(*cmd){
        case '0':
            usart_send("restart");
        break;
        case '7':
            usart_send("Shutter");
        break;
        case '8':
            usart_send("Neon");
        break;
        case '9':
            usart_send("Flat");
        break;
        case 'a':
            cmd = omit_whitespace(cmd + 1);
            return stepper_ch_speed(cmd);
        break;
        case 'b':
            usart_send("LED1");
        break;
        case 'c':
            usart_send("LED2");
        break;
        case 'd':
            usart_send("LED3");
        break;
        default:
            return 0;
    }
    usart_send("\n");
    return 1;
}

void process_string(){
    if((usart_flags & U_RX_COMPLETE) == 0) return;
    uint8_t noerr = 1, oldflags = usart_flags;
    usart_flags &= ~(U_RX_COMPLETE | U_RX_OVERFL | U_RX_ERROR);
    if(oldflags & U_RX_OVERFL){
        usart_send("Input buffer overflow\n");
        noerr = 0;
    }
    if(oldflags & U_RX_ERROR){
        usart_send("Rx error\n");
        noerr = 0;
    }
    if(rx_bufsize < 3 || rx_buffer[0] != '[' || rx_buffer[rx_bufsize - 2] != ']'){
        rx_bufsize = 0;
        usart_send("Enter \"[cmd]\"\n");
        noerr = 0;
    }
    if(noerr){ // echo back given string
        rx_buffer[rx_bufsize] = 0;
        uint8_t rbs = rx_bufsize;
        rx_bufsize = 0;
        usart_send(rx_buffer);
        rx_buffer[rbs - 2] = 0;
        process_commands();
    }
}
