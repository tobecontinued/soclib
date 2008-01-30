#
# SOCLIB_GPL_HEADER_BEGIN
# 
# This file is part of SoCLib, GNU GPLv2.
# 
# SoCLib is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# SoCLib is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with SoCLib; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
# 
# SOCLIB_GPL_HEADER_END
#
# Copyright (c) UPMC, Lip6, SoC
#         Nicolas Pouillon <nipo@ssji.net>, 2007
#
# Maintainers: nipo

TESTDIR?=.
include $(TESTDIR)/test.mk

RESULT=test.res
COMMON=$(TESTDIR)/../common
BASE=$(TESTDIR)/..

mipsel_CPU=mipsel
powerpc_CPU=ppc405
microblaze_CPU=microblaze

ifeq ($(words $(ARCH)),1)

VPATH=$(TESTDIR) $(COMMON)

SOCLIB:=$(shell soclib-cc --getpath)

SOFT_IMAGE=bin.soft

include $(SOCLIB)/utils/conf/soft_flags.mk

HW_HEADERS=$(SOCLIB)/utils/include

CC_PREFIX=$($(ARCH)_CC_PREFIX)
CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

CFLAGS=-Wall -O2 -I. -I$(COMMON) -I$(TESTER_DIR) -I$(HW_HEADERS) $(ADD_CFLAGS) $($(ARCH)_CFLAGS)

all: $(RESULT)

$(SOFT_IMAGE): $(OBJS) $(COMMON)/ldscript
	$(LD) -T $(COMMON)/ldscript $($(ARCH)_LDFLAGS) -o $@ $(OBJS)

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean :
	-rm -f $(SOFT_IMAGE) $(OBJS)

$(RESULT): $(SOFT_IMAGE) $(TESTER) $(COMMON)/Makefile
	SOCLIB_TTY=TERM $(TESTER) arch=$($(ARCH)_CPU) binary=$(SOFT_IMAGE) sim_max_cycles=1000000 sim_default_retval=1 > test.out \
		|| (r=$$? ; cat test.out ; mv test.out test.fail ; echo "Test failed in $$PWD, errorlevel: $$r" ; exit 1)
	cat test.out
	mv test.out $@

.PHONY: $(TESTER)

else

TESTDIR=../

all: $(RESULT)

$(RESULT): $(patsubst %,%/$(RESULT),$(ARCH))
	cat $^ > $@

.PHONY: $(RESULT)

clean:
	rm -rf $(ARCH) $(RESULT)

define onetest

$(1)/$(RESULT): $(1)
	make -C $(1) -f $(COMMON)/Makefile TESTDIR=.. ARCH=$(1)

$(1):
	mkdir -p $(1)

.PHONY: $(1)/$(RESULT)

endef

$(foreach bla,$(ARCH),$(eval $(call onetest,$(bla))))

endif
