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

extern void print_time();

/**
 * Move motor for given amount of steps, cmd should be 'N nnnn any other symbols',
 * N - motor number,
 * nnnn - steps (-32768...32768)
 * @return 1 if all OK
 */
uint8_t move_motor(uint8_t N, char *cmd){
    N -= '0';
    int16_t steps;
    if(!readInt(cmd, &steps)) return 1;
    #ifdef EBUG
    usart_send("Move motor ");
    printUint((uint8_t*)&N, 1);
    usart_send(" for ");
    print_long((uint32_t)steps);
    usart_send("steps\n");
    #endif
    return stepper_move(N, steps);
}

/**
 * process commands from user buffer
 * @return 0 if there's need to echo input cmdline
 */
uint8_t process_commands(char *cmd){
    cmd = omit_whitespace(cmd + 1);
    char s = *cmd;
    cmd = omit_whitespace(cmd + 1);
    int16_t port;
    switch(s){
        case '0': // stop motors
            DBG("restart");
            stop_motors();
            usart_send("Scorpio_0\n");
            return 1;
        break;
        case '1': // move focus motor
            DBG("focus");
            return move_motor(s, cmd);
        break;
        case '2':
            DBG("polar");
            return move_motor(s, cmd);
        break;
        case '3':
            DBG("turr2");
            return move_motor(s, cmd);
        break;
        case '4':
            DBG("turr1");
            return move_motor(s, cmd);
        break;
        case '5':
            DBG("temper");
            return 1;
        break;
        case '6':
            DBG("pol. magnet");
            if(*cmd == '1') PORTB |= _BV(0);
            else PORTB &= ~_BV(0);
        break;
        case '7':
            DBG("dispenser");
            if(*cmd == '1'){ // IFP
                PORTB |= _BV(1);
            }else{ // Grizm
                PORTB &= ~_BV(1);
            }
        break;
        case '8':
            DBG("an/disp");
            if(*cmd == '1'){ // analisator
                PORTB |= _BV(2);
            }else{ // disperser
                PORTB &= ~_BV(2);
            }
        break;
        case '9':
            DBG("set spd");
            return stepper_ch_speed(cmd);
        break;
        case 'b': // set PB value
            if(!readInt(cmd, &port) || port < 0 || port > 7) return 1;
            usart_send("set portb to");
            printUint((uint8_t*)&port, 2);
            usart_send("\n");
            PORTB = port;
        break;
        case 'l': // return Steps_left
            usart_send("[1 left=");
            printUint((uint8_t*) &Steps_left, 2);
            usart_send("]\n");
            return 1;
        break;
        case 't': // show approx. time from start
            print_time();
        break;
        case 'w':
            return chk_stpr_cmd(*cmd);
        break;
        default:
            return 1;
    }
    return 0;
}

void process_string(){
    if((usart_flags & U_RX_COMPLETE) == 0) return;
    uint8_t noerr = 1, oldflags = usart_flags;
    usart_flags &= ~(U_RX_COMPLETE | U_RX_OVERFL | U_RX_ERROR);
    if(oldflags & U_RX_OVERFL){
        DBG("Input buffer overflow\n");
        noerr = 0;
    }
    if(oldflags & U_RX_ERROR){
        DBG("Rx error\n");
        noerr = 0;
    }
    if(rx_bufsize < 3 || rx_buffer[0] != '[' || rx_buffer[rx_bufsize - 2] != ']'){
        rx_bufsize = 0;
        rx_buffer[0] = 0;
        noerr = 0;
    }
    if(noerr){ // echo back given string & process command
        rx_buffer[rx_bufsize] = 0;
        char *cmd = omit_whitespace(&rx_buffer[1]);
        uint8_t rbs = rx_bufsize;
        rx_bufsize = 0;
        if(*cmd == MODULEID){
            char *c = rx_copy, *o = rx_buffer;
            do{ *c++ = *o; }while(*o++);
            rx_buffer[rbs - 2] = 0;
            if(!process_commands(cmd)) usart_send(rx_copy);;
        }else{
            rx_buffer[0] = 0;
        }
    }
}
