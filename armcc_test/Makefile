CC = armcc
AS = armasm
LD = armlink

CFLAGS = --cpu=ARM7TDMI --arm --apcs=interwork -O3
CPPFLAGS = -Iinclude
ASFLAGS = --cpu=ARM7TDMI --apcs=interwork

OBJS = start.o main.o interrupt.o

all : test.gba

clean :
	$(RM) $(OBJS) test.elf test.gba

test.elf : $(OBJS)
	$(LD) --scatter gba_rom.scat  --entry $< $^ --output $@ --map

%.gba : %.elf
	fromelf --bin $< --output $@

%.o : %.asm
	$(AS) $(ASFLAGS) $<

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) main.c
