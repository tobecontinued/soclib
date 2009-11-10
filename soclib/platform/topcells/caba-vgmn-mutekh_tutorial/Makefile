KERNEL=mutekh/kernel-soclib-mips.out
SIMULATION_ARGS=$(KERNEL)
SOFT=$(KERNEL)
NO_SOFT=1
SOCLIB?=$(shell soclib-cc --getpath)
SIMULATOR_BINARY=system.x
export SOCLIB

all:

CONFIG=config_soclib_mipsel
APP=$(MUTEKH_DIR)/examples/hello

$(SIMULATOR_BINARY): $(KERNEL)

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
