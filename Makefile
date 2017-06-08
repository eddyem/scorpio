### https://habrahabr.ru/post/247663/

NAME = scorpio

OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
CC      = avr-gcc
OPTIMIZE= -Os
DEFS    = -DBAUD=9600
DEFS   += -DEBUG
LIBS    =

SRC=$(wildcard *.c)
HEX     = $(NAME).hex
ELF     = $(NAME).elf
OBJECTS = $(SRC:%.c=%.o)

# controller
DEVICE  = atmega328p
#atmega8535

CFLAGS  = -g -Wall $(OPTIMIZE) $(DEFS)
LDFLAGS = -Wl,-Map,$(NAME).map

# programmer (for avrdude)
PROGRAMMER = arduino
# partno (for avrdude)
PARTNO  = m328p
# serial port device (for avrdude)
SERPORT = /dev/ttyUSB0
#Тактовая частота 16 МГц
CLOCK   = 16000000

# avrdude command from arduino IDE
AVRDUDE = avrdude -C/usr/share/arduino/hardware/tools/avrdude.conf -v -p$(PARTNO) -c$(PROGRAMMER) -P$(SERPORT) -b115200 -D

COMPILE = $(CC) $(CFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(CLOCK)

all:	$(HEX) lst

$(ELF): $(OBJECTS)
	@echo "ELF"
	@$(COMPILE) -o $(ELF) $(OBJECTS) $(LIBS)

$(HEX): $(ELF)
	@echo "HEX"
	@rm -f $(HEX)
	@$(OBJCOPY) -j .text -j .data -O ihex $(ELF) $(HEX)
	@avr-size $(ELF)

.c.o:
	@$(COMPILE) -c $< -o $@

.S.o:
	@$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	@$(COMPILE) -S $< -o $@

lst:  $(NAME).lst

%.lst: %.elf
	@echo "Make listing"
	@$(OBJDUMP) -h -S $< > $@

flash:	all
	@echo "Flash"
	@$(AVRDUDE) -U flash:w:$(HEX):i

clean:
	@echo "Clean"
	@rm -f $(HEX) $(ELF) $(OBJECTS) *.lst *.map

gentags:
	CFLAGS="$(CFLAGS) -I/usr/avr/include" geany -g $(NAME).c.tags *.[hc] 2>/dev/null

.PHONY: gentags clean
