
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
# Maintainers: group:toolmakers

import re
import os, os.path

__id__ = "$Id$"
__version__ = "$Revision$"

__all__ = ['Config']

class Config:
	_filter = re.compile("[a-z][a-z_]+")
	def _do_expands(self):
		for name in filter(self._filter.match, dir(self)):
			at = getattr(self, name)
			if isinstance(at, (list, tuple)):
				at = map(self._do_expand, at)
			elif isinstance(at, str):
				at = self._do_expand(at)
			setattr(self, name, at)
	
	def _do_expand(self, new):
		old = ''
		while old != new:
			old = new
			new = new%self
			new = os.path.expandvars(new)
		return new

	def onInit(self):
		pass

	def __init__(self, base = None, **kwargs):
		import operator, new, inspect
		if base is not None:
			for k, v in base.__dict__.iteritems():
				if operator.isCallable(v) and inspect.ismethod(v):
					v = v.im_func
					v = new.instancemethod(v, self, self.__class__)
				setattr(self, k, v)
		for k, v in kwargs.iteritems():
			if operator.isCallable(v):
				v = new.instancemethod(v, self, self.__class__)
			setattr(self, k, v)
		for name in filter(self._filter.match, dir(self)):
			at = getattr(self, name)
			#if isinstance(at, (list, tuple)):
			#	at = ' '.join(map(lambda x:'"%s"'%x, at))
			setattr(self, name, at)
		self.onInit()

	def __getitem__(self, name):
		return getattr(self, name)

	def __str__(self):
		import pprint
		return pprint.pformat(self.__dict__)
