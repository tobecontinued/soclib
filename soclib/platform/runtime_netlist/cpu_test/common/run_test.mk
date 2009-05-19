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

TESTDIR=..
include $(TESTDIR)/test.mk

MAX_CYCLES?=100000
RESULT=test.res
COMMON=$(TESTDIR)/../common
BASE=$(TESTDIR)/..

mipsel_CPU=mipsel
mipseb_CPU=mipseb
mips32el_CPU=mips32el
mips32eb_CPU=mips32eb
powerpc_CPU=ppc405
microblaze_CPU=microblaze

VPATH=$(TESTDIR) $(COMMON)

SOFT_IMAGE=bin.soft

include $(SOCLIB)/utils/conf/soft_flags.mk

HW_HEADERS=$(SOCLIB)/utils/include

CC_PREFIX=$($(ARCH)_CC_PREFIX)
CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)as
LD = $(CC_PREFIX)ld
OBJDUMP = $(CC_PREFIX)objdump

CFLAGS=-Wall -Os -I. -I$(COMMON) -I$(TESTER_DIR) -I$(HW_HEADERS) $($(ARCH)_CFLAGS) $(ADD_CFLAGS)
LIBGCC:=$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

all: test

test: $(RESULT)

$(SOFT_IMAGE): $(OBJS) $(COMMON)/ldscript
	$(LD) -T $(COMMON)/ldscript $($(ARCH)_LDFLAGS) -o $@ $(OBJS) $(LIBGCC)

%.o: %.s
	$(AS) $< -o $@

%.o : %.c
	$(CC) -o $@ $(CFLAGS) -c $< \
		|| (echo "Compilation failed in $$PWD, ARCH=$(ARCH)" ; exit 1)

deps.mk: $(OBJS:.o=.deps)
	cat $^ /dev/null > $@

%.deps: %.c
	$(CC) $(CFLAGS) -M -MT $*.o -MF $@ $<

%.deps: %.s
	touch $@

include deps.mk

clean :
	-rm -f $(SOFT_IMAGE) $(OBJS)

$(RESULT): $(SOFT_IMAGE) $(TESTER) $(COMMON)/Makefile
	SOCLIB_TTY=TERM $(TESTER) cache_arch=xcache_$($(ARCH)_CPU) arch=$($(ARCH)_CPU) binary=$(SOFT_IMAGE) sim_max_cycles=$(MAX_CYCLES) sim_default_retval=1 &> test.out \
		|| (r=$$? ; tail test.out ; mv test.out test.fail ; echo "Test failed in $$PWD, errorlevel: $$r" | tee -a test.fail ; exit 1)
	cat test.out
	mv test.out $@

.PHONY: $(TESTER)
