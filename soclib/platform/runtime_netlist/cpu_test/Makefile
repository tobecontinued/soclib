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

SOCLIB:=$(shell soclib-cc --getpath)

TESTER_DIR=$(shell pwd)
TESTER=$(TESTER_DIR)/tester.netlist

TESTS=$(shell echo */test.mk)
TESTRES=$(TESTS:.mk=.res)

SUBDIRS=$(patsubst %/test.mk,%,$(shell echo */test.mk))

export TESTER_DIR TESTER

all: test

define subdir

$(1)/%: $(1)
	$$(MAKE) -C "$(1)" -f ../common/Makefile $$*

endef

$(eval $(foreach s,$(SUBDIRS),$(call subdir,$(s))))


test: $(patsubst %,%/test,$(SUBDIRS))

clean: $(patsubst %,%/clean,$(SUBDIRS))

.NOTPARALLEL :
