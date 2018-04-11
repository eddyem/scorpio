SCORPIO main controller
============================

### Based on atmega8535

## PINS description

* A0 - endswitch "+"
* A1 - endswitch "-"
* B0 - polarisator magnet
* B1 - caret relay
* B2 - analisator/disperser relay + wheel end-switch 2 turn off

* C0..C3 - stepper phases
* C4 - select collimator
* C5 - select analisator
* C6 - select turret No2
* C7 - select turret No1

* D0 - USART Rx
* D1 - USART Tx

## Commands protocol
All commands should be in **square brackets**. Spaces between parts inside command ignored.

As heritage, The device have number "1", so it would parse only commands with format **"[1 Xxx]\n"**.
If all OK the answer echoes input command or send some other data, in case of error there's nothing
in answer.

All commands of motors' moving have an option -- end-switches check. To do this just ask to move for
0 steps. Answer have format like **[1 x ST=y]** where x is motor number and y is end-switch position
(1 for "-", 2 for "+" and 3 for intermediate position).

## List of commands:

* **[1 0]** -- *(new behaviour, old was "restart")* -- stop all motors, due to working watchdog there's no need to make full system restart.
    This command have an answer: "Scorpio_0".

* **[1 1 xxx]** -- move focuser (approx. 100 steps per millimeter).

* **[1 2 xxx]** -- move polariser motor.

* **[1 3 xxx]** -- rotate turret 2.

* **[1 4 xxx]** -- rotate turret 1.

* **[1 6 x]** -- turn on (x==1) or off (x==0) relay of analysator magnet and turrets end-switch1 off/on.

* **[1 7 x]** -- move disperser motor to position x (1 - IFP, 0 - Grizm).

* **[1 8 x]** -- change end-switches check between analiser (x==1) and disperser (x==0).

* **[1 9 xxx]** -- change motor's speed. Motor period = 8 * 65535/(xxx + 10) microseconds.

* **[1 b x]** -- change value of PB0..PB3 to x.

* **[1 l]** -- tell amount of steps left.

* **[1 t]** -- show approx. time from start.

* **[1 w x]** -- change stepper diagram to x (x=0..7).

## How to change turret's position

First you should move turrets to zero-position end-switch: **[1 x 5000]** (here and later: x==3 for
turrer No2 and x==4 for turret No1). The "zero" position lays between first and last filters.
To move into first position just do **[1 x -300]**. Turret will move to end-switch of stable position.
To move further you should turn off intermediate end-switches: **[1 6 1]**. After that move from
end-switch: **[1 x -20]**, turn on end-switches: **[1 6 0]** and move to next position: **[1 x 300]**.

