/*
 *                                                                                                  geany_encoding=koi8-r
 * motors.h
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
#ifndef __MOTORS_H__
#define __MOTORS_H__

// default speed @ start - 200 steps per second
#define DEFAULT_USTEP_PERIOD    (2500)
// max speed == 1/(800us*2) = 625 steps per second
#define MIN_USTEP_PERIOD        (800)
// min speed as 16-bit timer can - 65535 - near 7.6 steps per second
// default min speed - 25 steps per second
#define MAX_USTEP_PERIOD        (20000)
// amount of steps to pull off the switch
#define PULLOFFTHESW_STEPS      (100)
// amount of microsteps for acceleration calculation (50 full steps)
#define ACCEL_USTEPS            (100)

//extern unsigned char irq_flag;

void motors_init();
void show_motors_help();
void motor_command(const char *cmd, char **bufptr);

void stepper_interrupt(unsigned char motor_num);

//void process_stepper(unsigned char stepno);

#endif // __MOTORS_H__
