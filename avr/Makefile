### https://habrahabr.ru/post/247663/

NAME = scorpio

OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
CC      = avr-gcc
OPTIMIZE= -Os
DEFS    = -DBAUD=9600
DEFS   += -DEBUG
LIBS    =

# controller
DEVICE  = atmega8535
#Тактовая частота 8 МГц
CLOCK   = 8000000
# partno (for avrdude)
PARTNO  = m8535

CFLAGS  = -g -Wall $(OPTIMIZE) $(DEFS)
LDFLAGS = -Wl,-Map,$(NAME).map

# programmer (for avrdude)
PROGRAMMER = avrisp

# serial port device (for avrdude)
SERPORT = /dev/ttyUSB0

SRC=$(wildcard *.c)
HEX     = $(NAME).hex
ELF     = $(NAME).elf
OBJECTS = $(SRC:%.c=%.o)

# avrdude command from arduino IDE
AVRDUDE = avrdude -C/usr/share/arduino/hardware/tools/avrdude.conf -v -p$(PARTNO) -c$(PROGRAMMER) -P$(SERPORT) -b19200 -D

COMPILE = $(CC) $(CFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(CLOCK)

all:	$(HEX) lst

$(ELF): $(OBJECTS)
	@echo "ELF"
	@$(COMPILE) $(LDFLAGS) -o $(ELF) $(OBJECTS) $(LIBS)

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
	$(AVRDUDE) -U flash:w:$(HEX):i

clean:
	@echo "Clean"
	@rm -f $(HEX) $(ELF) $(OBJECTS) *.lst *.map

gentags:
	CFLAGS="$(CFLAGS) -I/usr/avr/include" geany -g $(NAME).c.tags *.[hc] 2>/dev/null

.PHONY: gentags clean
