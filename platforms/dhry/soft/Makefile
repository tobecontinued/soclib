ADD_OBJS=stdio.o dhry_1.o dhry_2.o
ADD_CFLAGS=-DTIME -O3 #  -DUSE_REG
ifeq ($(ARCH),powerpc)
ADD_CFLAGS+=-mcpu=405 -mstrict-align
endif
include ../../soft.mk
