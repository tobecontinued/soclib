#CC_PREFIX=$(CXTOOLS)/gcc_mips/obj/bin/mipsel-soclib-elf-
CC_PREFIX=mipsel-unknown-elf-
SOCLIB:=$(shell soclib-cc --getpath)

CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

CFLAGS=-Wall -O2 -I$(SOCLIB)/include

default: ../a.out

../a.out : main.o uputs.o reset.o
	$(LD) -T./ldscript -o $@ $^
	$(OBJDUMP) -D $@ > aout.txt

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean :
	-rm -f *.o
	-rm -f ../a.out
	-rm -f *.txt
