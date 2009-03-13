ARCH=mpc7447a
SOCLIB?=$(shell soclib-cc --getpath)
ADD_OBJS=ldscript firmware.o stdlib.o
ADD_CFLAGS=
export SOCLIB
export ADD_OBJS
export ADD_CFLAGS
include $(SOCLIB)/soclib/platform/topcells/soft.mk
ldscript: soft.lds.S
	$(CC) -E soft.lds.S | sed -e 's/#.*//' > ldscript