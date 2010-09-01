ARCH=mips32el
SIMULATION_ARGS=100000
SOCLIB_CC_ADD_ARGS=-t systemcass
SOCLIB?=$(shell soclib-cc --getpath)
export SOCLIB
include $(SOCLIB)/soclib/platform/topcells/all.mk
