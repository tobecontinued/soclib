#!/usr/bin/env python

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

import os, os.path
from pprint import pprint
from utils import *
import types

__all__ = ['config', 'Joined']

import re

# Yes, its ugly and lisp-ish, but who cares ?
class ConfigSpool:
	_filter = re.compile("[a-z][a-z_]+")
	_fmt = '%s:\n%s'

	_desc_paths = ["soclib/communication", 'soclib/lib', 'soclib/module']

	def addDescPath(self, np):
		if not np in self._desc_paths:
			self._desc_paths.append(np)
	addDescPath = classmethod(addDescPath)

	def _pprint(self, obj, pf='  '):
		def _repr(o):
			try:
				if issubclass(o, Configurator):
					return '\n'+self._pprint(o, pf+'  ')
			except TypeError:
				pass
			return repr(o)
		return '\n'.join(map(lambda k:pf+'%s: %s'%(k, _repr(getattr(obj, k))),
							 filter(self._filter.match, dir(obj))))

	def __str__(self):
		return '\n'.join(map(lambda k:self._fmt%(k, self._pprint(getattr(self, k), '  ')),
							 filter(self._filter.match, dir(self))))

	def __iter__(self):
		for i in filter(self._filter.match, dir(self)):
			obj = getattr(self, i)
			if not isinstance(obj, (types.FunctionType, types.MethodType)):
				yield obj

	def __getitem__(self, name):
		return getattr(self, name)

_cur_soclib = os.path.abspath(
	os.path.join(
	os.path.dirname(__file__),
	'../../../../..'))
assert(_cur_soclib)

def include(filename, glbl):
	name = os.path.join(_cur_soclib, filename)
	if os.path.isfile(name):
		execfile(name, glbl, {})

def parseall():
	config = ConfigSpool()
	glbl = {'Config':Config,
			'config':config,
			'include':include}
	for name in (os.path.join(os.path.dirname(__file__), 'templates.py'),
				 os.path.join(_cur_soclib, 'utils', 'conf', 'soclib.conf'),
				 os.path.expanduser("~/.soclib/global.conf"),
				 "soclib.conf"):
		if os.path.isfile(name):
			execfile(name, glbl, {})
	for c in config:
		c._do_expands()
	return config

_configs = parseall()
config = _configs.default
config.doConfigure(_cur_soclib, _configs._desc_paths)
config.type = 'default'

def change_config(name):
	newconf = getattr(_configs, name)
	from soclib_cc import config
	newconf.doConfigure(_cur_soclib, _configs._desc_paths)
	config.config.__dict__ = newconf.__dict__
	for n in dir(newconf):
		setattr(config.config, n, getattr(newconf, n))
	config.type = name

def Joined(i):
	return ' '.join(map(lambda x:'"%s"'%x, i))

if __name__ == "__main__":
	print config

