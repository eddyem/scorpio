NAME=sco_platform
SDCC=sdcc

CCFLAGS=-DSTM8S105 -I../ -I/usr/share/sdcc/include -mstm8 --out-fmt-ihx
LDFLAGS= -mstm8 --out-fmt-ihx -lstm8
FLASHFLAGS=-cstlinkv2 -pstm8s105?4

SRC=$(wildcard *.c)
#DEFS = -DEBUG

OBJ=$(SRC:%.c=%.rel)
TRASH=$(OBJ) $(SRC:%.c=%.rst) $(SRC:%.c=%.asm) $(SRC:%.c=%.lst)
TRASH+=$(SRC:%.c=%.sym) $(NAME).lk $(NAME).map $(NAME).cdb
INDEPENDENT_HEADERS=stm8l.h ports_definition.h Makefile

all: $(NAME).ihx

clean:
	rm -f $(TRASH)

load: $(NAME).ihx
	stm8flash $(FLASHFLAGS) -w $(NAME).ihx

%.rel: %.c
	$(SDCC) $(CCFLAGS) $(DEFS) -c $<

$(NAME).ihx: $(OBJ)
	$(SDCC) $(LDFLAGS) $(OBJ) -o $(NAME).ihx

.PHONY: all
