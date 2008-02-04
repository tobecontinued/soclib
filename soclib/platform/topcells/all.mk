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

PLATFORM_DESC?=platform_desc
SOCLIB_CC_ARGS?=-p $(PLATFORM_DESC)
SOCLIB_CC=soclib-cc
SOCLIB:=$(shell soclib-cc --getpath)
TEST_OUTPUT=test.out

export ARCH

include $(SOCLIB)/utils/conf/soft_flags.mk

CC=$($(ARCH)_CC_PREFIX)gcc
HAS_CC:=$(shell which $(CC) 2>&1 > /dev/null && echo ok)
ifneq ($(HAS_CC),ok)
NO_SOFT=1
NO_TEST=No compiler for $(ARCH)
endif

ifeq ($(NO_SOFT),)
SOFT?=soft/bin.soft
endif

ifndef SIMULATION_ARGS
NO_TEST=No simulation args
endif

all: test_soclib simulation.x $(SOFT)

ifeq ($(NO_SOFT),)

.PHONY: soft/bin.soft

soft/bin.soft:
	$(MAKE) -C soft bin.soft

endif

test_soclib:
	@test -z "$(SOCLIB)" && (\
	echo "You must have soclib-cc in your $$PATH" ; exit 1 ) || exit 0
	@test ! -z "$(SOCLIB)"

simulation.x: $(PLATFORM_DESC)
	$(SOCLIB_CC) -P $(SOCLIB_CC_ARGS) -o $@

ifdef NO_TEST

test:
	@echo "Test disabled: $(NO_TEST)"

else

test: $(TEST_OUTPUT) post_test

post_test:

$(TEST_OUTPUT): simulation.x $(SOFT)
	SOCLIB_TTY=TERM ./simulation.x $(SIMULATION_ARGS) < /dev/null 2>&1 | tee $@

.PHONY: $(TEST_OUTPUT)

endif

clean: soft_clean
	$(SOCLIB_CC) -P $(SOCLIB_CC_ARGS) -x -o $@
	rm -rf repos

soft_clean:
ifeq ($(NO_SOFT),)
	$(MAKE) -C soft clean
endif

a.out:
	$(MAKE) -C soft

.PHONY: a.out simulation.x
