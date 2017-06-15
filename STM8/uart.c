/*
 *                                                                                                  geany_encoding=koi8-r
 * uart.c
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

#include "uart.h"
#include "ports_definition.h"

U8 uart_rdy = 0;
U8 rx_idx = 0, tx_idx = 0, tx_len = 0;

U8 UART_rx[UART_BUF_LEN+1]; // cycle buffer for received data
U8 UART_tx[UART_BUF_LEN+1];

/**
 * Send one byte through UART
 * @param byte - data to send
 *
void UART_send_byte(U8 byte){
    while(!(UART2_SR & UART_SR_TXE)); // wait until previous byte transmitted
    UART2_DR = byte;
}*/

void uart_write(char *str){
    while(tx_len) {IWDG_KR = KEY_REFRESH;}
    UART2_CR2 &= ~UART_CR2_TIEN;
    tx_idx = 0;
    do{
        UART_tx[tx_len++] = *str++;
    }while(*str && tx_len < UART_BUF_LEN);
    UART2_CR2 |= UART_CR2_TIEN; // enable TXE interrupt
}

char *omit_whitespace(char *str){
    char c;
    for(c = *str; c == ' ' || c == '\t' || c == '\r' || c == '\n'; c = *(++str));
    return str;
}

void printUint(U8 *val, U8 len){
    unsigned long Number = 0;
    U8 i = len;
    char ch;
    U8 decimal_buff[12]; // max len of U32 == 10 + \n + \0
    if(len > 4 || len == 3 || len == 0) return;
    for(i = 0; i < 12; i++)
        decimal_buff[i] = 0;
    decimal_buff[10] = '\n';
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
 * print signed long onto terminal
 * max len = 10 symbols + 1 for "-" + 1 for '\n' + 1 for 0 = 13
 */
void print_long(long Number){
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
    uart_write(&decimal_buff[ch]);
}


/**
 * read 16 bit integer value from buffer until first non-number
 * @param buff (i) - input buffer
 * @param (o) - output value
 * @return 1 if all OK or 0 if there's none numbers in buffer
 */
U8 readInt(char *buff, int *val){
    U8 sign = 0, rb, bad = 1;
    long R = 0;
    IWDG_KR = KEY_REFRESH; // refresh watchdog
    //usart_send("readInt, buff=");
    //usart_send(buff);
    if(*buff == '-'){
        sign = 1;
        ++buff;
    }
    do{
        rb = *buff++;
        if(rb < '0' || rb > '9') break;
        bad = 0;
        R = R * 10L + rb - '0';
        if(R > 0x7ffe){ // bad value
            bad = 1;
            break;
        }
    }while(1);
    //print_long(R);
    if(bad) return 0;
    if(sign) R = -R;
    if(val) *val = (int)R;
    return 1;
}
