SIMULATION_ARGS=mutekh/kernel-soclib-mips.out
SOCLIB?=$(shell soclib-cc --getpath)
NO_SOFT=1
SIMULATOR_BINARY=h264_platform.x
export SOCLIB
include $(SOCLIB)/soclib/platform/topcells/all.mk

# Parameters required to select the appropriate configuration file
NB_PROC?=1
TYPE_PROC?=mips

# Parameters required for compilations
APP_DIR=$(shell pwd)/decoder_h264_clone/decoder_soclib
MUTEKH_DIR?=/home/t0043357/sources/soclib/tools/mutekh

all:
	true

$(SIMULATOR_BINARY): mutekh/kernel-soclib-mips.out

platform_desc: mutekh/.config.py

mutekh:
	mkdir -p $@

mutekh/kernel-soclib-mips.out: mutekh
	$(MAKE) -C mutekh CONF=$(APP_DIR)/config_$(TYPE_PROC)_$(NB_PROC)proc -f $(MUTEKH_DIR)/Makefile SRC_DIR=$(MUTEKH_DIR) BUILD_DIR=$$(pwd)/mutekh

mutekh/.config.py: mutekh
	$(MAKE) -C mutekh CONF=$(APP_DIR)/config_$(TYPE_PROC)_$(NB_PROC)proc -f $(MUTEKH_DIR)/Makefile SRC_DIR=$(MUTEKH_DIR) BUILD_DIR=$$(pwd)/mutekh config

clean: mutekh/.config.py

soft_clean:
	rm -rf mutekh


