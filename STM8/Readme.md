SCORPIO platform controller
============================

### Based on STM8

PINS description

* D6 -- USART Rx
* D5 -- USART Tx
* D0..D2 -- select motors pair
* D3, D4 -- select motor from pair
* C1..C3 -- LED1..LED3
* B0..B3 -- stepper motors' outputs
* B4, B5, F4 -- relays (shutter, neon, flat)
* A1, A2 -- end-switches ("-" and "+")
* C5..C7 -- end-switch address (C5 is LSB)

### Commands protocol
All commands should be in **square brackets with '\n'** after closing bracket. Spaces between parts
inside command ignored.

As heritage, The device have number "2", so it would parse only commands with format **"[2 Xxx]\n"**.

List of commands:

* **[2 ?]** -- *(debug)* tell amount of steps leave.

* **[2 0]** -- *(new behaviour, old was "restart")* -- stop all motors and turn off all relays, due
to working watchdog there's no need to make full system restart.

* **[2 N xxx]**, where **N** is number from 1 to 6, **xxx** is number from -32767 to 32767 -- run
stepper motor number **N** for given amount of steps; if stepper is at end-switch and can't go further,
the answer would be **[2 N St=x]**, where **x** is 1 or 2 (depending on end-switch number);
when all steps wuold be over (or motor will come to end-switch), controller will answer a status:
**[2 N St=x]**, if x==3 the stepper isn't at any end-switch.

* **[2 N x]**, where **N** is number from 7 to 9 -- turn on/off or get status of relays (7 -- Shutter,
8 -- Neon, 9 -- Flat); if x==1 the relay would be turned on, if x==0 -- off, if x=='-' the status would
be answered (format: **[2 N St=x]**, x==0 if power down, 1 if power up).

* **[2 a xxx]**, where **xxx** if a number from -8 to 32767 -- change motor speed;
new speed is 0xffff / (xxx + 10) * 0.125 steps per second.

* **[2 N xxx], where **N** from 'b' to 'd', **xxx** from 0 to 255 -- change LEDx brightness, ('b' -- LED1,
'c' -- LED2, 'd' -- LED3); brightness 0 is minimum and 255 is maximum.
