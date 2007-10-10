include test.mk

VPATH=. ../common

SOCLIB:=$(shell soclib-cc --getpath)
CC_PREFIX=$(ARCH)-unknown-elf-

SOFT_IMAGE=bin.soft

CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

CFLAGS=-Wall -O2 -I. -I../common -I$(TESTER_DIR) $(shell soclib-cc --getflags=cflags) $(ADD_CFLAGS)

default: clean $(SOFT_IMAGE)

$(SOFT_IMAGE): $(OBJS)
	$(LD) -T ../common/ldscript -o $@ $^

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean :
	-rm -f $(SOFT_IMAGE) $(OBJS)

test.res: $(SOFT_IMAGE)
	$(TESTER) $(SOFT_IMAGE) > test.out
	cp test.out $@
