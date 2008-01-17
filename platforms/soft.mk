SOCLIB:=$(shell soclib-cc --getpath)

SOFT_IMAGE=bin.soft
OBJS?=main.o exception.o system.o $(ADD_OBJS)

include $(SOCLIB)/etc/soft_flags.mk

CC_PREFIX=$($(ARCH)_CC_PREFIX)
CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

CFLAGS=-Wall -O2 -I. $(shell soclib-cc --getflags=cflags) $(ADD_CFLAGS) $(DEBUG_CFLAGS) $($(ARCH)_CFLAGS)

MAY_CLEAN=$(shell test "$(ARCH)" = "$$(cat /dev/null arch_stamp)" || echo clean)

default: clean $(SOFT_IMAGE)

$(SOFT_IMAGE): $(MAY_CLEAN) arch_stamp $(OBJS)
	$(LD) -q $($(ARCH)_LDFLAGS) -T ldscript -o $@ $(filter %.o,$^)

arch_stamp:
	echo $(ARCH) > $@

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean :
	-rm -f $(SOFT_IMAGE) $(OBJS) arch_stamp
