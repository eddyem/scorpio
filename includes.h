/*
 *                                                                                                  geany_encoding=koi8-r
 * includes.h
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
#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include <avr/io.h> // IO ports
#include <avr/wdt.h> // WDT
#include <avr/interrupt.h>
#include <avr/io.h> // IO ports
#include <stdint.h> // int types
#include <util/setbaud.h> // baudrate calculation helper

#include "proto.h"
#include "uart.h"
#include "stepper.h"


#endif // __INCLUDES_H__
