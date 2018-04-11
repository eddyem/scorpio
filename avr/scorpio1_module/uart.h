/*
 *                                                                                                  geany_encoding=koi8-r
 * uart.h
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

#pragma once
#ifndef __UART_H__
#define __UART_H__
#include <avr/io.h> // IO ports
#include <stdint.h> // int types

#define U_TX_COMPLETE       (_BV(0))
#define U_RX_COMPLETE       (_BV(1))
#define U_TX_ERROR          (_BV(2))
#define U_RX_ERROR          (_BV(3))
#define U_RX_OVERFL         (_BV(4))

#define RX_BUFFER_SIZE      (32)
#define TX_BUFFER_SIZE      (32)

extern volatile uint8_t usart_flags;
extern char rx_buffer[];
extern char rx_copy[];
extern volatile uint8_t rx_bufsize;

int usart_send(char *Str);

void print_long(int32_t Number);
void printUint(uint8_t *val, uint8_t len);

uint8_t readInt(char *buff, int16_t *val);

char *omit_whitespace(char *str);

#endif // __UART_H__
