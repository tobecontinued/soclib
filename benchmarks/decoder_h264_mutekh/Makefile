include config.mk

SIMULATION_ARGS=$(KERNEL)
SOFT=$(KERNEL)
NO_SOFT=1
SOCLIB?=$(shell soclib-cc --getpath)
SIMULATOR_BINARY=platform_h264.x
# No need to change this, the soclib part does not build software.
# Change config.mk if you want.
ARCH=mips32el
export SOCLIB

all: $(KERNEL) $(SIMULATOR_BINARY) block0.iso

help:
	@echo "You may either build:"
	@echo "  $(SIMULATOR_BINARY): only the simulator"
	@echo "  $(KERNEL): only the mutekh kernel"

$(SIMULATOR_BINARY): cpu_type.h

cpu_type.h:
	echo "#define CPU_$(CPU)" > $@
	echo "#define VIDEO_TYPE_$(FORMAT)" >> $@
	echo "const size_t ncpu = $(NB_PROC);" >> $@

mutekh:
	mkdir -p $@

$(KERNEL): mutekh
	rm $(APP_DIR)/src/conf.h 
	ln -s $(APP_DIR)/soft_config/conf_$(NB_PROC)proc_$(FORMAT).h $(APP_DIR)/src/conf.h
	$(MAKE) \
	   -f $(MUTEKH_DIR)/Makefile \
	   CONF=$(APP_DIR)/$(CONFIG) \
	   BUILD_DIR=$(shell pwd)/mutekh \
	   SRC_DIR=$(MUTEKH_DIR) -j 3 \
	   BUILD=$(BUILD)

doc: mutekh
	$(MAKE) \
	   -f $(MUTEKH_DIR)/Makefile \
	   CONF=$(APP_DIR)/$(CONFIG) \
	   BUILD_DIR=$(shell pwd)/mutekh \
	   SRC_DIR=$(MUTEKH_DIR) -j 3 doc


block0.iso: disk
	mkisofs -f -R -V volname -o $@ $<


.PHONY: $(KERNEL)

include $(SOCLIB)/soclib/platform/topcells/all.mk
