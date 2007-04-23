
# This file is part of SoCLIB.
# 
# SoCLIB is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# SoCLIB is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with SoCLIB; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
# 
# Copyright (c) UPMC, Lip6, SoC
#         Nicolas Pouillon <nipo@ssji.net>, 2007
# 
# Maintainers: nipo

import re
import os, os.path

__all__ = ['Configurator']

class MagicGetter:
	def __init__(self, obj):
		self.obj = obj
	def __getitem__(self, name):
		return getattr(self.obj, name)

class Configurator:
	_filter = re.compile("[a-z][a-z_]+")
	def _do_expands(cls):
		for name in filter(cls._filter.match, dir(cls)):
			at = getattr(cls, name)
			if isinstance(at, (list, tuple)):
				at = map(cls._do_expand, at)
			elif isinstance(at, str):
				at = cls._do_expand(at)
			setattr(cls, name, at)
	_do_expands = classmethod(_do_expands)
	
	def _do_expand(cls, new):
		mg = MagicGetter(cls)
		old = ''
		while old != new:
			old = new
			new = new%mg
			new = os.path.expandvars(new)
		return new
	_do_expand = classmethod(_do_expand)

	def __init__(self, **kwargs):
		for name in filter(self._filter.match, dir(self)):
			at = getattr(self, name)
			if isinstance(at, (list, tuple)):
				at = ' '.join(map(lambda x:'"%s"'%x, at))
			setattr(self, name, at)
		for k, v in kwargs.iteritems():
			setattr(self, k, v)
	def __getitem__(self, name):
		return getattr(self, name)
