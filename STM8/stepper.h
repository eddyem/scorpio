/*
 * stepper.h
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */

#pragma once
#ifndef __STEPPER_H__
#define __STEPPER_H__

#include "ports_definition.h"

#define MIN_STEP_LENGTH 125     // max speed == 1/(125us*16) = 500 steps per second

extern volatile U8 chk_esw;
extern U8 cur_motor;

extern volatile int Steps_left;
extern U16 Stepper_speed;
extern volatile char Dir;
extern U8 *usteps;

void setup_stepper_pins();
U8 stepper_ch_speed(char *spd);
U8 stepper_move(U8 Nmotor, int Nsteps);

void stop_motor();
U8 check_endsw();
void stepper_get_esw(U8 Nmotor);

U8 chk_stpr_cmd(char N);

#endif // __STEPPER_H__
