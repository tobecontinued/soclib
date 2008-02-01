
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
import traceback
from copy import copy
from soclib_cc.config import config
from soclib_cc.builder.todo import ToDo
from soclib_cc.builder.cxx import CxxCompile, CxxLink

__all__ = ['TlmtPlatform', 'Platform', 'Uses', 'Source']

class NotFound(Exception):
	def __init__(self, name, mode):
		Exception.__init__(self, "Implementation %s not found for %s"%(mode, name))

class Platform:
	"""
	Platform definition, should be passed an arbitrary number of
	Uses() and Source() statements that constitutes the platform.
	"""
	mode = 'caba'
	def __init__(self, *components):
		self.components = components
		self.todo = ToDo()
		from soclib_cc.components import component_defs
		objs = []
		for c in components:
			c.do(component_defs, self.mode)
			for todo in c.todo():
				for o in todo.results():
					if not o in objs:
						objs.append( o )
						self.todo.add( o )
		self.todo.add( *CxxLink(config.output, objs ).dests )
	def process(self):
		self.todo.process()
	def clean(self):
		self.todo.clean()

class TlmtPlatform(Platform):
	mode = 'tlmt'

class Uses:
	"""
	A statement declaring the platform uses a specific component (in a
	global meaning, ie hardware component or utility).

	name is the name of the component, it the filename as in
	desc/soclib/[component].sd

	mode should be left alone unless specifically targetting a mode

	args is the list of arguments useful for compile-time definition
	(ie template parameters)
	"""
	def __init__(self, name, mode = None, **args):
		self.name = name
		self.mode = mode
		self.args = args
		# This is for error feedback purposes
		self.where = '%s:%d'%(traceback.extract_stack()[-2][0:2])
	def __str__(self):
		return '<Use %s %s>'%(self.name, self.mode)
	def do(self, cdefs, mode = None, **inherited_args):
		cdef = cdefs[self.name]
		if self.mode is not None:
			mode = self.mode
		args = copy(inherited_args)
		args.update(self.args)
		for k in args.keys():
			newv = args[k]
			if not '%' in str(newv):
				continue
			v = None
			while newv != v:
				v = newv
				newv = newv%inherited_args
			args[k] = v
		if mode not in cdef and 'common' in cdef:
			mode = 'common'
		if mode in cdef:
			d = cdef[mode]
		else:
			raise NotFound(self.name, mode)
		self.builder = d(self.where, mode, **args)
	def todo(self):
		return self.builder.withDeps()

class Source:
	'''
	A statement declaring use for a user-defined c++ file to compile
	and link with the platform. There is no template instanciation
	feature with this file, only defines (set of -Dx=y)

	source_file is the relative name to the source file

	all other named arguments will be set as defines when compiling:

	Source('top.cpp', defines = {'test_arg' = '42'}) will add -Dtest_arg=42 on
	compilation command line for top.cpp
	'''
	def __init__(self, source_file, uses = [], defines = {}):
		self.source_file = os.path.abspath(source_file)
		self.uses = uses
		self.defines = defines
		self.deps = []
		self.incs = []
	def do(self, cdefs, mode = None, **args):
		for u in self.uses:
			u.do(cdefs, mode, **args)
			self.deps += u.todo()
			u.builder.getIncl(self.incs)
		obj = os.path.splitext(self.source_file)[0]+'.'+config.toolchain.obj_ext
		self.builder = CxxCompile(obj, self.source_file, defines = self.defines, inc_paths = self.incs)
	def todo(self):
		return self.deps + [self.builder]
