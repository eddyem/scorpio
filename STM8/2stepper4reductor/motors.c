/*
 *                                                                                                  geany_encoding=koi8-r
 * motors.c
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
#include "motors.h"
#include "uart.h"
#include "hardware.h"

//U8 irq_flag = 0; // |1 - 1st motor, |2 - second

// motor states
typedef enum{
    MOTOR_RELAX,        // do nothing
    MOTOR_INFMOVE,      // infinite moving
    MOTOR_STOP,         // stop
    MOTOR_ZEROSTOP,     // stop and set current position to zero
    MOTOR_MOVENSTEPS,   // move for N steps
    MOTOR_OFFSWITCH,    // try to pull off the positive switch (for turrets and to return from switch back)
} motor_state;

// direction of rotation
typedef enum{
    DIR_CW,     // clockwise
    DIR_CCW,    // counter-clockwise
    DIR_STOP    // stopped
} motor_direction;

static U16 Stepper_speed[2] = {DEFAULT_USTEP_PERIOD, DEFAULT_USTEP_PERIOD};     // length of one MICROstep in us
static volatile motor_direction Dir[2] = {DIR_STOP, DIR_STOP};     // direction of moving
static volatile motor_state state[2] = {MOTOR_RELAX, MOTOR_RELAX};
static long Steps_left[2] = {0,0}; // steppers left (when moving for given steps amount)
static long Current_pos[2] = {0,0}; // current position
static long Steps_left_at_esw[2] = {0,0}; // amount of `Steps_left` when MOTOR_OFFSWITCH activated
static U16 Acceleration[2] = {0, 0}; // current acceleration (to reach target speed by ACCEL_STEPS microsteps)

// microsteps profile
// microsteps: DCBA = 1000, 1010, 0010, 0110, 0100, 0101, 0001, 1001 - half-step
// 1010, 0110, 0101, 1001 - full step
static const U8 usteps[8] = {0b1000, 0b1010, 0b0010, 0b0110, 0b0100, 0b0101, 0b0001, 0b1001};
static U8 Ustep[2] = {0, 0}; // microstep counter

// init timers & GPIO for motors
void motors_init(){
    // Configure timer 2 to generate signals for CLK of motor 0
    TIM2_PSCR = 4; // 1MHz
    TIM2_ARRH = DEFAULT_USTEP_PERIOD >> 8; // set speed
    TIM2_ARRL = DEFAULT_USTEP_PERIOD & 0xff;
    TIM2_IER = TIM_IER_UIE; // update interrupt enable
    TIM2_CR1 |= TIM_CR1_APRE | TIM_CR1_URS; // auto reload + interrupt on overflow
    // timer 3 for motor 1
    TIM3_PSCR = 4;
    TIM3_ARRH = DEFAULT_USTEP_PERIOD >> 8;
    TIM3_ARRL = DEFAULT_USTEP_PERIOD & 0xff;
    TIM3_IER = TIM_IER_UIE;
    TIM3_CR1 |= TIM_CR1_APRE | TIM_CR1_URS;
}

void show_motors_help(){
    //         "start                        end"
    uart_write("\tE - get end-switches\n");
    uart_write("\tL - move CCW\n");
    uart_write("\tM - get motor state\n");
    uart_write("\tN - go for N st./get rest\n");
    uart_write("\tO - pull off the switch\n");
    uart_write("\tP - get current position\n");
    uart_write("\tR - move CW\n");
    uart_write("\tS - get/set speed\n");
    uart_write("\tX - stop motor\n");
    uart_write("\tZ - stop and zero position\n");
}

/**
 * Check endswitches
 * @return 0 if none pressed, 1 if "-", 2 if "+", 3 if both!
 */
static U8 check_endsw(U8 motor){
    U8 ret = 0;
    switch(motor){
        case 0:
            if(CHK_M0E1()) ++ret;
            if(CHK_M0E2()) ret += 2;
        break;
        case 1:
            if(CHK_M1E1()) ++ret;
            if(CHK_M1E2()) ret += 2;
        break;
        default:
            return 0;
    }
    return ret;
}

static void stop_motor(U8 motorNum){
    // stop timers & turn off power
    switch(motorNum){
        case 0:
            PORT(STP0_PORT, ODR) &= ~STP_PINS;
            TIM2_CR1 &= ~TIM_CR1_CEN;
        break;
        case 1:
            PORT(STP1_PORT, ODR) &= ~STP_PINS;
            TIM3_CR1 &= ~TIM_CR1_CEN;
        break;
        default: return;
    }
    Steps_left[motorNum] = 0;
    Dir[motorNum] = DIR_STOP;
}

static void strtobuf(const char *str, char **buff){
    while(*str) *((*buff)++) = *str++;
}

static void get_motor_state(U8 nmotor, char **buff){
    char sig = '+';
    if(nmotor > 1) return;
    if(Dir[nmotor] == DIR_CCW) sig = '-';
    switch(state[nmotor]){
        case MOTOR_RELAX:
            strtobuf("RELAX", buff);
        break;
        case MOTOR_INFMOVE:
            strtobuf("INFMV", buff); // INVMV+ or INFMV-
            *((*buff)++) = sig;
        break;
        break;
        case MOTOR_STOP:
            strtobuf("STOP", buff);
        break;
        case MOTOR_MOVENSTEPS:
            strtobuf("MVSTP", buff); // MVSTP+ or MVSTP-
            *((*buff)++) = sig;
        break;
        case MOTOR_OFFSWITCH:
            strtobuf("OFFSW", buff); // OFFSW+ or OFFSW-
            *((*buff)++) = sig;
        break;
        default:
            strtobuf("UNDEF", buff);
    }
}

// turn on motor's timer starting from the lowest speed
static void turnontimer(U8 motorNum){
    U8 tmp;
    switch(motorNum){
        case 0:
            // turn on power
            tmp = PORT(STP0_PORT, ODR) & ~STP_PINS;
            PORT(STP0_PORT, ODR) = tmp | usteps[Ustep[0]];
            // start from the slowest speed
            TIM2_ARRH = MAX_USTEP_PERIOD >> 8;
            TIM2_ARRL = MAX_USTEP_PERIOD & 0xff;
            // run timer
            TIM2_CR1 |= TIM_CR1_CEN;
        break;
        case 1:
            tmp = PORT(STP1_PORT, ODR) & ~STP_PINS;
            PORT(STP1_PORT, ODR) = tmp | usteps[Ustep[1]];
            TIM3_ARRH = MAX_USTEP_PERIOD >> 8;
            TIM3_ARRL = MAX_USTEP_PERIOD & 0xff;
            TIM3_CR1 |= TIM_CR1_CEN;
        break;
        default: return;
    }
    Acceleration[motorNum] = 1 + (MAX_USTEP_PERIOD - Stepper_speed[motorNum]) / ACCEL_USTEPS;
}

/**
 * try to move motor motorNum for nsteps
 * @return 1 if all OK; 0 if still moving or on end-switch
 */
static int moveNsteps(U8 motorNum, long nsteps){
    U8 sw;
    if(Dir[motorNum] != DIR_STOP) return 0;
    sw = check_endsw(motorNum);
    if(nsteps < 0){
        if(sw) return 0; // on zero end-switch: no moving backward, on positive - no moving at all!
        Dir[motorNum] = DIR_CCW;
        nsteps = -nsteps;
        state[motorNum] = MOTOR_MOVENSTEPS;
    }else{
        if(sw & 2) return 0; // for positive direction no moving to any side when on end-switch 2!
        else if(sw & 1) state[motorNum] = MOTOR_OFFSWITCH;
        else state[motorNum] = MOTOR_MOVENSTEPS;
        Dir[motorNum] = DIR_CW;
    }
    Steps_left[motorNum] = nsteps;
    // turn On timer
    turnontimer(motorNum);
    return 1;
}

/**
 * try to move motor motorNum off the end-switch for nsteps
 * @return 1 if all OK; 0 if still moving, motor is @ zero endswitch
 */
static int pullofftheswitch(U8 motorNum, long nsteps){
    U8 sw;
    if(Dir[motorNum] != DIR_STOP) return 0;
    sw = check_endsw(motorNum);
    if(sw == 0) return moveNsteps(motorNum, nsteps);
    if(nsteps < 0){
        if(sw & 1) return 0; // on zero end-switch: no moving backward
        Dir[motorNum] = DIR_CCW;
        nsteps = -nsteps;
    }else{
        Dir[motorNum] = DIR_CW;
    }
    Steps_left[motorNum] = nsteps;
    Steps_left_at_esw[motorNum] = nsteps;
    state[motorNum] = MOTOR_OFFSWITCH;
    turnontimer(motorNum);
    return 1;
}

void motor_command(const char *cmd, char **buff){
    U8 motorNum = *cmd++ - '0';
    U16 spd;
    long l;
    char c;
    if(motorNum > 1) goto someerr;
    *((*buff)++) = '0' + motorNum;
    *((*buff)++) = ' ';
    c = *cmd++;
    *((*buff)++) = c;
    switch(c){
        case 'E': // check endswitches state
             *((*buff)++) = ' ';
            *((*buff)++) = '0' + check_endsw(motorNum);
        break;
        case 'L': // infinite move left
            if(check_endsw(motorNum) & 1){
                strtobuf(" E 1", buff);
            }else{
                state[motorNum] = MOTOR_INFMOVE;
                Dir[motorNum] = DIR_CCW;
                turnontimer(motorNum);
            }
        break;
        case 'M': // get motor state
             *((*buff)++) = ' ';
            get_motor_state(motorNum, buff);
        break;
        case 'N': // go for N steps or get steps left
            *((*buff)++) = ' ';
            if(!readLong(cmd, &l)){ // get
                long2buf(Steps_left[motorNum], buff);
            }else{
                if(!moveNsteps(motorNum, l)) goto someerr;
                else long2buf(l, buff);
            }
        break;
        case 'O': // pull off the switch (if no steps given, go for PULLOFFTHESW_STEPS)
             *((*buff)++) = ' ';
            if(!readLong(cmd, &l)){ // get
                l = PULLOFFTHESW_STEPS;
            }
            if(!pullofftheswitch(motorNum, l)) goto someerr;
            else long2buf(l, buff);
        break;
        case 'P': // get current position
            *((*buff)++) = ' ';
            long2buf(Current_pos[motorNum], buff);
        break;
        case 'R': // infinite move right
            if(check_endsw(motorNum) & 2){
                strtobuf(" E 2", buff);
            }else{
                state[motorNum] = MOTOR_INFMOVE;
                Dir[motorNum] = DIR_CW;
                turnontimer(motorNum);
            }
        break;
        case 'S': // change speed
             *((*buff)++) = ' ';
            if(!readLong(cmd, &l) || l < MIN_USTEP_PERIOD || l > MAX_USTEP_PERIOD){ // get speed
                if(motorNum == 0) spd = TIM2_ARRH << 8 | TIM2_ARRL;
                else spd = TIM3_ARRH << 8 | TIM3_ARRL;
                long2buf(spd, buff);
            }else{
                long2buf(l, buff);
                spd = (U16)l;
                Stepper_speed[motorNum] = spd;
                Acceleration[motorNum] = 0;
                if(motorNum == 0){
                    TIM2_ARRH = spd >> 8;
                    TIM2_ARRL = spd & 0xff;
                }else{
                    TIM3_ARRH = spd >> 8;
                    TIM3_ARRL = spd & 0xff;
                }
            }
        break;
        case 'X': // stop
            state[motorNum] = MOTOR_STOP;
        break;
        case 'Z':
            state[motorNum] = MOTOR_ZEROSTOP;
        break;
        default: // return err
            goto someerr;
    }
    return;
    someerr:
        *((*buff)++) = 'e';
        *((*buff)++) = 'r';
        *((*buff)++) = 'r';
}

/**
 * this function calls from timer interrupt (TIM2 or TIM3 - motors 0/1)
 */
void stepper_interrupt(U8 motor_num){
    U8 tmp;
    U16 spd;
    //irq_flag ^= 1 << motor_num;
    switch(motor_num){
        case 0:
            if(Acceleration[0]){
                spd = (TIM2_ARRH << 8 | TIM2_ARRL) - Acceleration[0];
                if(spd < Stepper_speed[0]){
                    spd = Stepper_speed[0];
                    Acceleration[0] = 0;
                }
                //printUint((U8*)&spd, 2);
                //uart_write(" - speed0\n");
                TIM2_ARRH = spd >> 8;
                TIM2_ARRL = spd & 0xff;
            }
            tmp = PORT(STP0_PORT, ODR) & ~STP_PINS;
            PORT(STP0_PORT, ODR) = tmp | usteps[Ustep[0]];
        break;
        case 1:
            if(Acceleration[1]){
                spd = (TIM3_ARRH << 8 | TIM3_ARRL) - Acceleration[1];
                if(spd < Stepper_speed[1]){
                    spd = Stepper_speed[1];
                    Acceleration[1] = 0;
                }
                //printUint((U8*)&spd, 2);
                //uart_write(" - speed1\n");
                TIM3_ARRH = spd >> 8;
                TIM3_ARRL = spd & 0xff;
            }
            tmp = PORT(STP1_PORT, ODR) & ~STP_PINS;
            PORT(STP1_PORT, ODR) = tmp | usteps[Ustep[1]];
        break;
        default: return;
    }
    if(Ustep[motor_num] % 2 == 0){ // full amount of half-steps - increment step counters & check for stop
        --Steps_left[motor_num];
        if(Dir[motor_num] == DIR_CCW) --Current_pos[motor_num];
        else ++Current_pos[motor_num];
        if(state[motor_num] == MOTOR_STOP || state[motor_num] == MOTOR_ZEROSTOP){
            stop_motor(motor_num);
            return;
        }
    }
    if(Dir[motor_num] == DIR_CCW){ // counter-clockwise
        if(Ustep[motor_num] == 0) Ustep[motor_num] = 7;
        else --Ustep[motor_num];
    }else{ // clockwise
        if(++Ustep[motor_num] > 7) Ustep[motor_num] = 0;
    }
}

/**
 * Main state-machine process
 */
void process_stepper(U8 motor_num){
    U8 sw = check_endsw(motor_num);
    U8 ccw = (Dir[motor_num] == DIR_CCW) ? 1 : 0;
    switch(state[motor_num]){
        case MOTOR_OFFSWITCH: // don't care about endswitch for first PULLOFFTHESW_STEPS steps
        if(Steps_left[motor_num] < 2) state[motor_num] = MOTOR_STOP;
        else if(Steps_left_at_esw[motor_num] - Steps_left[motor_num] >= PULLOFFTHESW_STEPS)
                state[motor_num] = MOTOR_MOVENSTEPS;
        break;
        case MOTOR_MOVENSTEPS:
        if(Steps_left[motor_num] < 2) state[motor_num] = MOTOR_STOP; // stop @given position
        case MOTOR_INFMOVE: // set curpos to zero only in this state (after reaching ESW1)
            if(sw){
                if(ccw){
                    if(state[motor_num] == MOTOR_INFMOVE){
                        if(sw & 1) state[motor_num] = MOTOR_ZEROSTOP; // esw1 - stop @ zero when inf. left move
                    }else state[motor_num] = MOTOR_STOP; // just stop at any esw in steps move
                }else{ // +switch when move CW
                    if(sw & 2) state[motor_num] = MOTOR_STOP; // stop in CW only on esw2 !!!
                }
            }
        break;
        case MOTOR_STOP:
            if(Dir[motor_num] == DIR_STOP){
                state[motor_num] = MOTOR_RELAX;
            }
        break;
        case MOTOR_ZEROSTOP:
            if(Dir[motor_num] == DIR_STOP){
                Current_pos[motor_num] = 0;
                state[motor_num] = MOTOR_RELAX;
            }
        break;
        default: return; // MOTOR_RELAX
    }
    //if(irq_flag & (1<<motor_num)) stepper_interrupt(motor_num);
}
