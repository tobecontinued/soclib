include config.mk

SIMULATION_ARGS=$(KERNEL)
SOFT=$(KERNEL)
NO_SOFT=1
SOCLIB?=$(shell soclib-cc --getpath)
SIMULATOR_BINARY=system.x
# No need to change this, the soclib part does not build software.
# Change config.mk if you want.
ARCH=mips32el
export SOCLIB

all: $(KERNEL) $(SIMULATOR_BINARY) block0.iso

help:
	@echo "You may either build:"
	@echo "  $(SIMULATOR_BINARY): only the simulator"
	@echo "  $(KERNEL): only the mutekh kernel"
	@echo "  block0.iso: only the iso file from iso_contents"

$(SIMULATOR_BINARY): config.h

config.h:
	grep -q $(CPU) $@ || \
	( ( echo "#define CPU_$(CPU)"; echo "#define KERNEL_FILE \"$(KERNEL)\"" ) > $@ )

$(KERNEL): mutekh
	$(MAKE) \
	   -f $(MUTEKH_DIR)/Makefile \
	   CONF=$(APP)/$(CONFIG) \
	   CONF_DIR=$(shell pwd)/mutekh/$(CPU) \
	   BUILD_DIR=$(shell pwd)/mutekh \
	   SRC_DIR=$(MUTEKH_DIR) -j 3 \
	   BUILD=$(BUILD)

doc: mutekh
	$(MAKE) \
	   -f $(MUTEKH_DIR)/Makefile \
	   CONF=$(APP)/$(CONFIG) \
	   BUILD_DIR=$(shell pwd)/mutekh \
	   SRC_DIR=$(MUTEKH_DIR) -j 3 doc

mutekh:
	mkdir -p $@

.PHONY: $(KERNEL) config.h FORCE

FORCE:

include $(SOCLIB)/soclib/platform/topcells/all.mk

block0.iso: iso_contents FORCE
	mkisofs -f -R -V volname -o $@ $<

iso_contents: FORCE
#	$(MAKE) -C embedded
#	cp embedded/mutekh/kernel-soclib-$(CPU).out $@/kernel
