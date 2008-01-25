ARCH=nios2
SOCLIB:=$(shell soclib-cc --getpath)
CC_PREFIX=$(ARCH)-unknown-elf-

SOFT_IMAGE=bin.soft
OBJS= reset.o exceptionProcessing.o softwareException.o system.o main.o  

CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

#CFLAGS=-Wall -O2 -I. $(shell soclib-cc --getflags=cflags) $(ADD_CFLAGS) $(DEBUG_CFLAGS) $($(ARCH)_CFLAGS)
CFLAGS=-Wall -O2 -mhw-mul -mhw-div -I. $(shell soclib-cc --getflags=cflags)

MAY_CLEAN=$(shell test "$(ARCH)" = "$$(cat /dev/null arch_stamp)" || echo clean)

default: clean $(SOFT_IMAGE)

$(SOFT_IMAGE): $(MAY_CLEAN) arch_stamp $(OBJS)
	$(LD) -T./nios_build.ld -o $@ $(filter %.o,$^)
	$(OBJDUMP) -D $(SOFT_IMAGE) > aout.txt

arch_stamp:
	echo $(ARCH) > $@

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
#	$(CC) -save-temps -o $@ $(CFLAGS) -c $<
	$(CC) -o $@ $(CFLAGS) -c $<

clean :
	-rm -f $(SOFT_IMAGE) $(OBJS)  arch_stamp
