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

SUBDIRS=xtty fb_screen soclib-simulate-any headers pipe2fb
DEST=$(shell soclib-cc --getpath)/utils/bin

export DEST

rules=all install clean

define mkrule

$(1): $$(foreach d,$$(SUBDIRS),$$d/$(1))

%/$(1): foo
	$$(MAKE) -C $$* $(1)

endef

$(eval $(foreach rule,$(rules),$(call mkrules,$(rule))))

.PHONY: foo

foo:

