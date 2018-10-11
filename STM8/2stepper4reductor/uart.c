/*
 *                                                                                                  geany_encoding=koi8-r
 * uart.c
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

#include "uart.h"
#include "hardware.h"

U8 rx_idx = 0, uart_rdy = 0, broadcast = 0;

U8 UART_rx[UART_BUF_LEN]; // buffer for received data

void uart_init(){
    // UART TX will automatically switch into PuPd mode when UART_CR2_TEN
    // 8 bit, no parity, 1 stop (UART_CR1/3 = 0 - reset value)
    // 9600 on 16MHz: DIV=0x0693 -> BRR1=0x68, BRR2=0x03
    UART2_BRR1 = 0x68; UART2_BRR2 = 0x03;
    UART2_CR2 =  UART_CR2_REN | UART_CR2_RIEN; // Allow TX/RX, generate ints on rx
}

void uart_write(const char *str){
    if(broadcast) return; // don't write anything on broadcast commands
    UART2_CR2 |= UART_CR2_TEN; // turn Tx on
    while(*str){
        UART2_DR = *str++;
        while(!(UART2_SR & UART_SR_TC));
    }
    UART2_CR2 &= ~UART_CR2_TEN; // turn Tx off
    /*
    while(tx_len) {IWDG_KR = KEY_REFRESH;}
    UART2_CR2 |= UART_CR2_TEN;
    do{
        UART_tx[tx_len++] = *str++;
    }while(*str && tx_len < UART_BUF_LEN);
    UART2_CR2 |= UART_CR2_TIEN; // enable TXE interrupt
    */
}

void printUint(const U8 *val, U8 len){
    unsigned long Number = 0;
    U8 i = len;
    char ch;
    U8 decimal_buff[11]; // max len of U32 == 10 + \0
    if(len > 4 || len == 3 || len == 0) return;
    decimal_buff[10] = 0;
    ch = 9;
    switch(len){
        case 1:
            Number = *((U8*)val);
            break;
        case 2:
            Number = *((U16*)val);
        break;
        case 4:
            Number = *((unsigned long*)val);
        break;
    }
    do{
        i = Number % 10L;
        decimal_buff[ch--] = i + '0';
        Number /= 10L;
    }while(Number && ch > -1);
    uart_write((char*)&decimal_buff[ch+1]);
}

/**
 * fill buffer with symbols of signed long
 * max len = 10 symbols + 1 for "-" + 1 for 0 = 12
 */
void long2buf(long Number, char **buf){
    U8 i, L = 0;
    U8 ch;
    char decimal_buff[12];
    decimal_buff[11] = 0;
    ch = 11;
    if(Number < 0){
        Number = -Number;
        L = 1;
    }
    do{
        i = Number % 10L;
        decimal_buff[--ch] = i + '0';
        Number /= 10L;
    }while(Number && ch > 0);
    if(ch > 0 && L) decimal_buff[--ch] = '-';
    for(i = ch; i < 12; ++i) *((*buf)++) = decimal_buff[i];
}


/**
 * read 16 bit integer value from buffer until first non-number
 * @param buff (i) - input buffer
 * @param val  (o) - output value
 * @return 1 if all OK or 0 if there's none numbers in buffer
 */
U8 readLong(const char *buff, long *val){
    U8 sign = 0, rb, bad = 1;
    long R = 0, oR = 0;
    if(*buff == '-'){
        sign = 1;
        ++buff;
    }
    do{
        rb = *buff++;
        if(rb == '+') continue;
        if(rb < '0' || rb > '9') break;
        bad = 0;
        oR = R * 10L + rb - '0';
        if(oR < R){ // bad value
            bad = 1;
            break;
        }
        R = oR;
    }while(1);
    if(bad) return 0;
    if(sign) R = -R;
    *val = R;
    return 1;
}
