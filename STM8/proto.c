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

#include "ports_definition.h"
#include "uart.h"
#include "stepper.h"

/**
 * Move motor for given amount of steps, cmd should be 'N nnnn any other symbols',
 * N - motor number,
 * nnnn - steps (-32768...32768)
 * @return 1 if all OK
 */
U8 move_motor(char *cmd){
    U8 N = (U8)*cmd - '0';
    int steps;
    if(N < 1 || N > 6) return 0;
    cmd = omit_whitespace(cmd+1);
    if(!readInt(cmd, &steps)) return 0;
    #ifdef EBUG
    uart_write("Move motor ");
    printUint((U8*)&N, 1);
    uart_write(" for ");
    print_long((long)steps);
    uart_write("steps\n");
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
 * @param N - second symbol of command ([2 N ...])
 */
U8 relay(char *cmd, char N){
    U8 on = 0;
    if(*cmd == '-'){ // just check
        char ans[] = "[2 N St=1]\n";
        ans[3] = N;
        switch (N){
            case '7':
                on = RELAY_1();
            break;
            case '8':
                on = RELAY_2();
            break;
            case '9':
                on = RELAY_3();
            break;
            default:
            return 0;
        }
        if(!on) ans[8] = '0'; // off
        uart_write(ans);
        return 1;
    }
    if(*cmd == '0'){ // turn OFF
        switch (N){
            case '7':
                RELAY1_OFF();
            break;
            case '8':
                RELAY2_OFF();
            break;
            case '9':
                RELAY3_OFF();
            break;
            default:
            return 0;
        }
        return 1;
    }
    if(*cmd == '1'){ // turn ON
        switch (N){
            case '7':
                RELAY1_ON();
            break;
            case '8':
                RELAY2_ON();
            break;
            case '9':
                RELAY3_ON();
            break;
            default:
            return 0;
        }
        return 1;
    }
    return 0;
}

//extern void print_time();

void LEDshine(char *cmd, U8 N){
    int s;
    if(!readInt(cmd, &s)) return;
    if(s < 0 || s > 255) return;
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
        default:
        return;
    }
}

/**
 * process commands from user buffer
 * @return 1 if all OK
 */
U8 process_commands(char *cmd){
    char s;
    cmd = omit_whitespace(cmd + 1);
    if(*cmd > '0' && *cmd < '7')
        return move_motor(cmd);
    s = *cmd;
    cmd = omit_whitespace(cmd + 1);
    switch(s){
        case '?':
            uart_write("Steps_left=");
            print_long((long) Steps_left);
            uart_write("\n");
        break;
        case '0': // stop motors
            DBG("restart");
            stop_motor();
            RELAYS_OFF();
        break;
        case '7':
            DBG("Shutter");
            relay(cmd, '7');
        break;
        case '8':
            DBG("Neon");
            relay(cmd, '8');
        break;
        case '9':
            DBG("Flat");
            relay(cmd, '9');
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
    U8 rbs, noerr=1;
    char *cmd;
    if(uart_rdy == 0) return;
    uart_rdy = 0;
    if(rx_idx < 3 || UART_rx[0] != '[' || UART_rx[rx_idx - 2] != ']'){
        if(!chk_stpr_cmd(UART_rx[0])){
            DBG("Enter \"[cmd]\"\n");
        }
        //if(rx_buffer[0] == 't'){ print_time(); return; }
        rx_idx = 0;
        noerr = 0;
    }
    if(noerr){ // echo back given string
        UART_rx[rx_idx] = 0;
        cmd = omit_whitespace(&UART_rx[1]);
        if(*cmd != '2') return;
        rbs = rx_idx;
        rx_idx = 0;
        uart_write(UART_rx);
        UART_rx[rbs - 2] = 0;
        process_commands(cmd);
    }
}
