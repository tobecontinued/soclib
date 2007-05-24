
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

from soclib_cc.builder.action import *
from soclib_cc.builder.cxx import *
from soclib_cc.builder.textfile import *
import os, os.path

__all__ = ['Component', 'VciCabaComponent', 'CabaComponent', 'CommonComponent']

class UndefinedParam(Exception):
	def __init__(self, where, comp, param):
		Exception.__init__(self)
		self.where = where
		self.comp = comp
		self.param = param
	def __str__(self):
		return '\n%s:error: parameter %s not defined for component %s'%(
			self.where, self.param, self.comp)

class Component:
	'''
	A platform component definition, this can be anything, from
	actual hardware component to simple utility. This is the
	base for template-based separated compilation.

	You sould have to define the attributes according to your
	component. See desc/soclib/*.sd for examples.

	You will more likely use derived classes from this one, like
	CabaComponent, VciCabaComponent or CommonComponent.
	'''
	relative_path_files = ['header_files', 'implementation_files']
	mode = 'systemc'
	namespace = ""
	classname = ""
	tmpl_parameters = []
	tmpl_instanciation = ""
	header_files = []
	implementation_files = []
	uses = []
	default_parameters = {}
	
	def __init__(self, where, **args):
		self.where = where
		self.args = args
	def mk_abs_paths(cls, basename):
		for attr in cls.relative_path_files:
			val = getattr(cls, attr)
			def mkabs(name):
				if os.path.isabs(name):
					return name
				else:
					r = os.path.abspath(os.path.join(basename, name))
					if config.debug:
						print basename, name, r
					return r
			val = map(mkabs, val)
			setattr(cls, attr, val)
	mk_abs_paths = classmethod(mk_abs_paths)
	def getBuilder(self, filename):
		bn = self.baseName()
		bn += '_'+os.path.splitext(os.path.basename(filename))[0]
		if self.implementation_files:
			tx = CxxSource(
				config.reposFile(bn+".cc"),
				self.cxxSource(filename))
			return CxxCompile(
				config.reposFile(bn+"."+config.toolchain.obj_ext),
				tx.dests[0])
		else:
			return Noop()
	def baseName(self):
		basename = self.namespace + self.classname
		if self.tmpl_parameters:
			args = self.getParams()
			params = ",".join(
				map(lambda x:'%s=%s'%(x,args[x]),
					self.tmpl_parameters))
			basename += "_" + params.replace(' ', '_')
		return basename
	def results(self):
		builders = map(self.getBuilder, self.implementation_files)
		return reduce(lambda x,y:x+y, map(lambda x:x.dests, builders), [])
	def getParams(self):
		params = {}
		for k in self.tmpl_parameters:
			v = None
			if k in self.args:
				v = self.args[k]
			elif k in self.default_parameters:
				v = self.default_parameters[k]
				if config.verbose:
					print "Using default param %s=%s for component %s used at %s"%(
						k, v, self.classname, self.where)
			if v is None:
				raise UndefinedParam(self.where, self.classname, k)
			params[k] = v
		return params
	def cxxSource(self, s):
		source = ""
		source += '#include "%s"\n'%s
		inst = 'class ' + self.namespace + self.classname
		if self.tmpl_parameters:
			if self.tmpl_instanciation:
				fmt = self.tmpl_instanciation
			else:
				fmt = ','.join(
				map(lambda x:'%%(%s)s'%x,
					self.tmpl_parameters))
			params = fmt % self.getParams()
			inst = 'template '+inst+'<'+params+' >'
		source += inst+';\n'
		return source
	
class CabaComponent(Component):
	namespace = 'soclib::caba::'

class CommonComponent(Component):
	namespace = 'soclib::common::'

class VciCabaComponent(CabaComponent):
	tmpl_parameters = [
		'cell_size', 'plen_size', 'addr_size',
		'rerror_size', 'clen_size', 'rflag_size',
		'srcid_size', 'pktid_size', 'trdid_size',
		'wrplen_size']
	tmpl_instanciation = (
		'soclib::caba::VciParams<%(cell_size)s,%(plen_size)s,%(addr_size)s,'+
		'%(rerror_size)s,%(clen_size)s,%(rflag_size)s,%(srcid_size)s,'+
		'%(pktid_size)s,%(trdid_size)s,%(wrplen_size)s>')
