SOCLIB:=$(shell soclib-cc --getpath)
CC_PREFIX=$(ARCH)-unknown-elf-

SOFT_IMAGE=bin.soft
OBJS=main.o exception.o system.o $(ADD_OBJS)

CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

CFLAGS=-Wall -O2 -I. $(shell soclib-cc --getflags=cflags) $(ADD_CFLAGS) $(DEBUG_CFLAGS)
ifeq ($(ARCH),powerpc)
CFLAGS+=-mcpu=405 -mstrict-align
endif

default: clean $(SOFT_IMAGE)

$(SOFT_IMAGE): $(OBJS)
	$(LD) -q -T ldscript -o $@ $^

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean :
	-rm -f $(SOFT_IMAGE) $(OBJS)
