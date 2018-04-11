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

#include "includes.h"

char rx_buffer[RX_BUFFER_SIZE + 1];
char rx_copy[RX_BUFFER_SIZE + 1];
volatile uint8_t rx_bufsize = 0;
static char tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_bufsize = 0, tx_idx = 0;

volatile uint8_t usart_flags = U_TX_COMPLETE;

/**
 * Send zero-terminated string using USART
 * if length of string (excluding 0) > TX_BUFFER_SIZE return 1
 * if all OK return 0
 */
int usart_send(char *Str){
    while((usart_flags & U_TX_COMPLETE) == 0);
    usart_flags &= ~(U_TX_COMPLETE | U_TX_ERROR);
    tx_idx = 0;
    for(tx_bufsize = 0; tx_bufsize < TX_BUFFER_SIZE; ++tx_bufsize){
        if(*Str == 0) break;
        tx_buffer[tx_bufsize] = *Str++;
    }
    if(tx_bufsize == TX_BUFFER_SIZE){ // error: buffer overflow
        tx_bufsize = 0;
        usart_flags |= U_TX_COMPLETE;
        return 1;
    }
    UCSRB |= _BV(UDRIE); // allow TX data buffer empty interrupt
    return 0;
}

/**
 * print signed long onto terminal
 * max len = 10 symbols + 1 for "-" + 1 for '\n' + 1 for 0 = 13
 */
void print_long(int32_t Number){
    uint8_t i, L = 0;
    uint8_t ch;
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
    usart_send(&decimal_buff[ch]);
}

void printUint(uint8_t *val, uint8_t len){
    uint32_t Number = 0;
    uint8_t i = len;
    int8_t ch;
    uint8_t decimal_buff[11]; // max len of U32 == 10 + \0
    if(len > 4 || len == 3 || len == 0) return;
    for(i = 0; i < 11; i++)
        decimal_buff[i] = 0;
    ch = 9;
    switch(len){
        case 1:
            Number = *((uint8_t*)val);
            break;
        case 2:
            Number = *((uint16_t*)val);
        break;
        case 4:
            Number = *((uint32_t*)val);
        break;
    }
    do{
        i = Number % 10L;
        decimal_buff[ch--] = i + '0';
        Number /= 10L;
    }while(Number && ch > -1);
    usart_send((char*)&decimal_buff[ch+1]);
}

char *omit_whitespace(char *str){
    char c;
    for(c = *str; c == ' ' || c == '\t' || c == '\r' || c == '\n'; c = *(++str));
    return str;
}

/**
 * read 16 bit integer value from buffer until first non-number
 * @param buff (i) - input buffer
 * @param (o) - output value
 * @return 1 if all OK or 0 if there's none numbers in buffer
 */
uint8_t readInt(char *buff, int16_t *val){
    uint8_t sign = 0, rb, bad = 1;
    int32_t R = 0;
    if(*buff == '-'){
        sign = 1;
        ++buff;
    }
    do{
        rb = *buff++;
        if(rb < '0' || rb > '9') break;
        bad = 0;
        R = R * 10L + rb - '0';
        if(R > 0x7fff){ // bad value
            bad = 1;
            break;
        }
    }while(1);
    //print_long(R);
    if(bad) return 0;
    if(sign) R = -R;
    if(val) *val = (int16_t)R;
    return 1;
}


ISR(USART_RX_vect){
    char c = UDR, r = UCSRA;
    if(0 == (r & (_BV(FE) | _BV(PE) | _BV(DOR)))){ // no errors
        if(c == '\t' || c == '\r' || c == '\n') return; // omit spaces
        rx_buffer[rx_bufsize++] = c;
        if(c == ']'){
            usart_flags |= U_RX_COMPLETE;
            rx_buffer[rx_bufsize++] = '\n';
            rx_buffer[rx_bufsize] = 0;
        }else if(rx_bufsize == RX_BUFFER_SIZE)
            usart_flags |= U_RX_COMPLETE | U_RX_OVERFL;
    }else usart_flags |= U_RX_COMPLETE | U_RX_ERROR;
}

ISR(USART_UDRE_vect){
    if(tx_bufsize == 0){
        UCSRB &= ~_BV(UDRIE);
        usart_flags |= U_TX_COMPLETE;
        return;
    }
    UDR = tx_buffer[tx_idx++];
    if(tx_idx == tx_bufsize){
        tx_idx = 0;
        tx_bufsize = 0;
        UCSRB &= ~_BV(UDRIE);
        usart_flags |= U_TX_COMPLETE;
    }
}

