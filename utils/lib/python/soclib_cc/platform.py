
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
from component_builder import ComponentBuilder

__all__ = ['TlmtPlatform', 'Platform', 'Uses', 'Source']

from soclib_desc.component import Uses

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
		objs = []
		for c in components:
			c.do(self.mode)
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
	def do(self, mode = None, **args):
		for u in self.uses:
			u.do(mode, **args)
			self.deps += u.todo()
			u.builder.getIncl(self.incs)
		obj = os.path.splitext(self.source_file)[0]+'.'+config.toolchain.obj_ext
		obj = os.path.abspath(obj)
		self.builder = CxxCompile(obj, self.source_file, defines = self.defines, inc_paths = self.incs)
	def todo(self):
		return self.deps + [self.builder]
