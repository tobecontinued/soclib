KERNEL=mutekh/kernel-soclib-mips.out
SIMULATION_ARGS=$(KERNEL)
NO_SOFT=1
SOCLIB?=$(shell soclib-cc --getpath)
SIMULATOR_BINARY=system.x
# never mind about the ARCH below, we dont use the software-building
# feature of this makefile anyway
ARCH=mips32el
export SOCLIB

help:
	@echo available targets are:
	@echo "  all: build mutek kernel and simulator"
	@echo "  $(SIMULATOR_BINARY): build simulator only"

all: $(SIMULATOR_BINARY) $(KERNEL)

CONFIG=config_soclib_mipsel
APP=$(MUTEKH_DIR)/examples/hello

$(KERNEL): mutekh
	$(MAKE) \
	   -f $(MUTEKH_DIR)/Makefile \
	   CONF=$(APP)/$(CONFIG) \
	   BUILD_DIR=$(shell pwd)/mutekh \
	   SRC_DIR=$(MUTEKH_DIR) -j 3

mutekh:
	mkdir -p $@

.PHONY: $(KERNEL)

include $(SOCLIB)/soclib/platform/topcells/all.mk
