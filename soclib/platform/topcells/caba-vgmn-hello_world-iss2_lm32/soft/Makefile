ADD_OBJS = stdio.o stdlib.o 
OBJS = boot.o crt0.o main.o exception.o system.o $(ADD_OBJS)
SOCLIB?=$(shell soclib-cc --getpath)
LGCC_FILE = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
lm32_LDFLAGS += -L$(dir $(LGCC_FILE))

export SOCLIB
include $(SOCLIB)/soclib/platform/topcells/soft.mk
