ARCH=arm966
SOCLIB?=$(shell soclib-cc --getpath)
export SOCLIB
ADD_OBJS=firm.o code.o
export ADD_OBJS
include $(SOCLIB)/soclib/platform/topcells/soft.mk
