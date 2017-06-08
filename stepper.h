/*
 *                                                                                                  geany_encoding=koi8-r
 * stepper.h
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
#ifndef __STEPPER_H__
#define __STEPPER_H__

#include <stdint.h>

extern volatile uint8_t stepper_pulse;
extern uint16_t Steps_left;

// setup timer
void stepper_setup();
void stepper_process();

uint8_t stepper_ch_speed(char *spd);
uint8_t stepper_move(uint8_t Nmotor, int16_t Nsteps);

void stepper_get_esw(uint8_t Nmotor);

#endif // __STEPPER_H__
