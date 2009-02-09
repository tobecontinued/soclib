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

SOCLIB:=$(shell soclib-cc --getpath)
export SOCLIB

RESULT=test.res
COMMON=$(TESTDIR)/../common
BASE=$(TESTDIR)/..

ARCH_ORIG:=$(ARCH)
ARCH:=

define decl_arch

$(1)_DISABLE_1=
$(1)_DISABLE_=$(1)
ARCH:=$$(ARCH) $$($(1)_DISABLE_$$(DISABLE_$(1)))

endef
$(foreach bla,$(ARCH_ORIG),$(eval $(call decl_arch,$(bla))))

ARCH:=$(strip $(ARCH))

TESTDIR=../

all: test

test: $(RESULT)

clean:
	rm -rf $(ARCH) $(RESULT)

define onetest

$(1)/%: $(1) FORCE
	$(MAKE) -C $(1) -f $(COMMON)/run_test.mk "$$*" ARCH=$(1)

$(1):
	mkdir -p $(1)

endef

FORCE:

ifeq ($(ARCH),)

$(RESULT):
	@true

else

$(RESULT): $(patsubst %,%/$(RESULT),$(ARCH)) FORCE
	cat $(filter %/$(RESULT),$^) > $@

$(foreach bla,$(ARCH),$(eval $(call onetest,$(bla))))
endif