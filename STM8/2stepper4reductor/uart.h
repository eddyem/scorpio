/*
 *                                                                                                  geany_encoding=koi8-r
 * uart.h
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
#pragma once
#ifndef __UART_H__
#define __UART_H__

#include "hardware.h"

#define UART_BUF_LEN 32

extern U8 UART_rx[];
extern U8 UART_tx[];
extern U8 uart_rdy, rx_idx, tx_idx, tx_len;


void uart_init();

void uart_write(const char *str);

char *omit_whitespace(char *str);

void long2buf(long Number, char **buf);
void printUint(const U8 *val, U8 len);
U8 readLong(const char *buff, long *val);


#endif // __UART_H__


