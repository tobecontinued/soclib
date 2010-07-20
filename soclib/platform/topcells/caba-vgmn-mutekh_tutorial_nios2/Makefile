include config.mk

SIMULATION_ARGS=$(KERNEL)
SOFT=$(KERNEL)
SOCLIB?=$(shell soclib-cc --getpath)
SIMULATOR_BINARY=system.x
# No need to change this, the soclib part does not build software.
# Change config.mk if you want.
ARCH=nios2
export SOCLIB

all: $(KERNEL) block0.iso 

mutekh:
	mkdir -p $@

$(KERNEL): mutekh
	$(MAKE) \
	   -f $(MUTEKH_DIR)/Makefile \
	   CONF=$(APP)/$(CONFIG) \
	   BUILD_DIR=$(shell pwd)/mutekh \
	   SRC_DIR=$(MUTEKH_DIR) -j 3  \
	   BUILD=$(BUILD)


block0.iso: iso_contents 
	mkisofs -f -R -V volname -o $@ $<



.PHONY: $(KERNEL)

#include $(SOCLIB)/soclib/platform/topcells/all.mk





