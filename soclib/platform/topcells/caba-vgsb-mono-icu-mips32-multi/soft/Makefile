LD=mipsel-unknown-elf-ld
CC=mipsel-unknown-elf-gcc
AS=mipsel-unknown-elf-as
DU=mipsel-unknown-elf-objdump

OBJS=   reset.o \
	giet.o \
	syscalls.o \
	isr.o \
	userio.o \
	stdio.o \
	main.o \

CFLAGS= -Wall -O2 -mno-gpopt -G0 -fno-builtin -fomit-frame-pointer -mips32 -ggdb

bin.soft: $(OBJS) ldscript
	$(LD) -q -o $@ -T ldscript $(OBJS)
	$(DU) -D $@ > $@.txt

reset.o: reset.s
	$(AS) -mips32 -o $@ $<
	$(DU) -D $@ > $@.txt

giet.o: giet.s
	$(AS) -mips32 -o $@ $<
	$(DU) -D $@ > $@.txt

isr.o: isr.s
	$(AS) -mips32 -o $@ $<
	$(DU) -D $@ > $@.txt

stdio.o: stdio.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(DU) -D $@ > $@.txt

userio.o: userio.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(DU) -D $@ > $@.txt

syscalls.o: syscalls.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(DU) -D $@ > $@.txt

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(DU) -D $@ > $@.txt

clean:
	rm -f *.o bin.soft *.txt core *~
