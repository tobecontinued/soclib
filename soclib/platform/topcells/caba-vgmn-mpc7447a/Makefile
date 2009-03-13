AM_CPPFLAGS+=-D__STDC_CONSTANT_MACROS
ARCH=mpc7447a
SIMULATION_ARGS=1320000
SOCLIB?=$(shell soclib-cc --getpath)
ifndef UNISIM
NO_TEST=External unisim library needed
endif
export SOCLIB
include $(SOCLIB)/soclib/platform/topcells/all.mk
