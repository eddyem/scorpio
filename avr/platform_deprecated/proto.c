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
    #ifdef EBUG
    usart_send("Move motor ");
    printUint((uint8_t*)&N, 1);
    usart_send(" for ");
    print_long((uint32_t)steps);
    usart_send("steps\n");
    #endif
    if(steps) return stepper_move(N, steps);
    else{ // steps == 0 - just check endswitches
        stepper_get_esw(N);
        return 0;
    }
}

/**
 * Switch relay on/off depending on cmd value
 * 1 - on, 0 - off
 * @param pin - pin to change
 * @param N - second symbol of command ([2 N ...])
 */
uint8_t relay(char *cmd, uint8_t pin, char N){
    if(*cmd == '-'){ // just check
        char ans[] = "[2 N St=1]\n";
        ans[3] = N;
        if(PORTAB & pin) ans[8] = '0'; // off
        usart_send(ans);
        return 1;
    }
    if(*cmd == '0'){ // turn OFF
        PORTAB |= pin;
        return 1;
    }
    if(*cmd == '1'){ // turn ON
        PORTAB &= ~pin;
        return 1;
    }
    return 0;
}

//extern void print_time();
extern uint8_t LEDs[3]; // LEDs shining time

void LEDshine(char *cmd, uint8_t N){
    int16_t s;
    if(!readInt(cmd, &s)) return;
    if(s < 0 || s > 255) return;
    LEDs[N] = (uint8_t)s;
}

/**
 * process commands from user buffer
 * @return 1 if all OK
 */
uint8_t process_commands(char *cmd){
    cmd = omit_whitespace(cmd + 1);
    if(*cmd > '0' && *cmd < '7')
        return move_motor(cmd);
    char s = *cmd;
    cmd = omit_whitespace(cmd + 1);
    switch(s){
        case '0': // stop motors
            DBG("restart");
            stop_motors();
        break;
        case '7':
            DBG("Shutter");
            relay(cmd, SHTR_PIN, '7');
        break;
        case '8':
            DBG("Neon");
            relay(cmd, NEON_PIN, '8');
        break;
        case '9':
            DBG("Flat");
            relay(cmd, FLAT_PIN, '9');
        break;
        case 'a':
            return stepper_ch_speed(cmd);
        break;
        case 'b':
            DBG("LED1");
            LEDshine(cmd, 0);
        break;
        case 'c':
            DBG("LED2");
            LEDshine(cmd, 1);
        break;
        case 'd':
            DBG("LED3");
            LEDshine(cmd, 2);
        break;
        default:
            return 0;
    }
    DBG("\n");
    return 1;
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
        #ifdef EBUG
        usart_send(rx_buffer);
        #endif
        if(!chk_stpr_cmd(rx_buffer[0])){
            DBG("Enter \"[cmd]\"\n");
        }
        //if(rx_buffer[0] == 't'){ print_time(); return; }
        rx_bufsize = 0;
        noerr = 0;
    }
    if(noerr){ // echo back given string
        rx_buffer[rx_bufsize] = 0;
        char *cmd = omit_whitespace(&rx_buffer[1]);
        if(*cmd != '2') return;
        uint8_t rbs = rx_bufsize;
        rx_bufsize = 0;
        usart_send(rx_buffer);
        rx_buffer[rbs - 2] = 0;
        process_commands(cmd);
    }
}
