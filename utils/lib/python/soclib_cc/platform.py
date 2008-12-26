
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

try:
	set
except:
	from sets import Set as set

__all__ = ['Platform', 'Uses']

from soclib_desc.component import Uses

class NotFound(Exception):
	def __init__(self, name, mode):
		Exception.__init__(self, "Implementation %s not found for %s"%(mode, name))

class Platform:
	"""
	Platform definition, should be passed an arbitrary number of
	Uses() and Source() statements that constitutes the platform.
	"""
	def fullyQualifiedModuleName(self, name):
		if not ':' in name:
			return 'caba:'+name
		return name
	def putArgs(self, d):
		pass
	def addObj(self, o):
		if not o in self.objs:
			self.objs.add(o)
			self.todo.add(o)
	def __init__(self, mode, source_file, uses = [], defines = {}, output = None, **params):
		component = Source(mode, source_file, uses, defines, **params)
		self.todo = ToDo()
		self.objs = set()
		builder = component.builder(self)
		all = builder.withDeps()
		for b in all:
			for o in b.results():
				self.addObj( o )
		if output is None:
			output = config.output
		self.todo.add( *CxxLink(output, self.objs ).dests )
	def process(self):
		self.todo.process()
	def clean(self):
		self.todo.clean()

	def genMakefile(self):
		return self.todo.genMakefile()

def Source(mode, source_file, uses = [], defines = {}, **params):
	name = mode+':'+hex(hash(source_file))
	from soclib_desc.module import Module
	m = Module(name,
			   uses = uses,
			   defines = defines,
			   implementation_files = [source_file],
			   local = True,
			   )
	filename = traceback.extract_stack()[-3][0]
	d = os.path.abspath(os.path.dirname(filename))
	m.mk_abs_paths(d)
	return Uses(name, **params)
