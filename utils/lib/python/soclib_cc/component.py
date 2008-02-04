
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

from soclib_cc.builder.action import *
from soclib_cc.builder.cxx import *
from soclib_cc.builder.textfile import *
import os, os.path

__all__ = ['VciCabaModule', 'CabaModule',
		   'CommonModule',
		   'TlmtModule', 'VciTlmtModule',
		   'Port',
		   'Parameter',
		   'CabaSignals','TlmtSignals']

class UndefinedParam(Exception):
	def __init__(self, where, comp, param):
		Exception.__init__(self)
		self.where = where
		self.comp = comp
		self.param = param
	def __str__(self):
		return '\n%s:error: parameter %s not defined for component %s'%(
			self.where, self.param, self.comp)

class NoSuchComponent(Exception):
	pass

global all_registry
all_registry = {}

class Module:
	module_attrs = ['namespace',
					'classname',
					'tmpl_parameters',
					'tmpl_instanciation',
					'header_files',
					'force_header_files',
					'implementation_files',
					'uses',
					'default_parameters',
					'defines',]
	namespace = ""
	classname = ""
	tmpl_parameters = []
	tmpl_instanciation = ""
	header_files = []
	force_header_files = []
	implementation_files = []
	uses = []
	default_parameters = {}
	defines = {}

	class __metaclass__(type):
		def __new__(cls, name, bases, dct):
			self = type.__new__(cls, name, bases, dct)
			if hasattr(self, "mode"):
				self.registry = {}
				global all_registry
				all_registry[self.mode] = self
			return self

	def __init__(self, name, **attrs):
		self.__attrs = {}
		base_attrs = self.__class__.__dict__
		for i in self.module_attrs:
			self.__attrs[i] = getattr(self, i)
			if i in attrs:
				self.__attrs[i] = attrs[i]
		import traceback
		filename = traceback.extract_stack()[-2][0]
		self.mk_abs_paths(os.path.dirname(filename))
		global all_registry
		r = all_registry[self.mode]
		all_registry[self.mode].register(name, self)

	def get(cls, name):
		if not name in cls.registry:
			return cls.__bases__[0].get(name)
		return cls.mode, cls.registry[name]
	get = classmethod(get)
	def register(cls, name, v):
		cls.registry[name] = v
	register = classmethod(register)
	def getAll(cls):
		return cls.registry
	getAll = classmethod(getAll)
		
	def __call__(self, where, mode, **args):
		return _Component(where, mode, self.__attrs, **args)
	def mk_abs_paths(self, basename):
		relative_path_files = ['header_files', 'implementation_files', 'force_header_files']
		def mkabs(name):
			if os.path.isabs(name):
				return name
			else:
				r = os.path.abspath(os.path.join(basename, name))
				if config.debug:
					print basename, name, r
				return r
		for attr in relative_path_files:
			self.__attrs['abs_'+attr] = map(
				mkabs, self.__attrs[attr])
## 	def __getattr__(self, name):
## 		if name.startswith('_'):
## 			raise AttributeError
## 		return self.__attrs[name]
	def __repr__(self):
		r = '<%s\n'%self.__class__.__name__
		for l in 'abs_header_files', 'abs_force_header_files', 'abs_implementation_files':
			for s in self.__attrs[l]:
				r += os.path.isfile(s) and " + " or " - "
				r += s+'\n'
			return r+' >'

class _Component:
	def __init__(self, where, mode, attrs, **args):
		self.__dict__.update(attrs)
		self.where = where
		self.args = args
		self.mode = mode
		self.deps = []
		for u in self.uses:
			u.do(mode, **args)
			self.deps.append(u.builder)

	def withDeps(self):
		r = (self,)
		for d in self.deps:
			r += d.withDeps()
		return r
	def getIncl(self, incls):
		for d in self.deps:
			d.getIncl(incls)
		for i in self.abs_header_files:
			p = os.path.dirname(i)
			if not p in incls:
				incls.append(p)
	def getBuilder(self, filename):
		bn = self.baseName()
		bn += '_'+os.path.splitext(os.path.basename(filename))[0]
		if self.abs_implementation_files:
			tx = CxxSource(
				config.reposFile(bn+".cpp"),
				self.cxxSource(filename))
			incls = []
			self.getIncl(incls)
			return CxxCompile(
				config.reposFile(bn+"."+config.toolchain.obj_ext),
				tx.dests[0],
				defines = self.defines,
				inc_paths = incls)
		else:
			return Noop()
	def baseName(self):
		basename = str(self.mode) + '_' + self.namespace + self.classname
		if self.tmpl_parameters:
			args = self.getParams()
			params = ",".join(
				map(lambda x:'%s=%s'%(x,args[x]),
					self.tmpl_parameters))
			basename += "_" + params.replace(' ', '_')
		if self.defines:
			params = ",".join(
				map(lambda x:'%s=%s'%x,
					self.defines.iteritems()))
			basename += "_" + params.replace(' ', '_')
		return basename
	def results(self):
		builders = map(self.getBuilder, self.abs_implementation_files)
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
		for h in self.force_header_files:
			source += '#include "%s"\n'%h
		inst = 'class ' + self.namespace + self.classname
		if self.tmpl_parameters:
			if self.tmpl_instanciation:
				fmt = self.tmpl_instanciation
			else:
				fmt = ','.join(
				map(lambda x:'%%(%s)s'%x,
					self.tmpl_parameters))
			params = fmt % self.getParams()
			inst = inst+'<'+params+' >'
		if '<' in inst:
			source += 'template '+inst+';\n'
		return source

class CommonModule(Module):
	mode = 'common'
	namespace = 'soclib::common::'

class CabaModule(CommonModule):
	mode = 'caba'
	namespace = 'soclib::caba::'
	
class TlmtModule(CommonModule):
	mode = 'tlmt'
	namespace = 'soclib::tlmt::'

class VciCabaModule(CabaModule):
	tmpl_parameters = [
		'cell_size', 'plen_size', 'addr_size',
		'rerror_size', 'clen_size', 'rflag_size',
		'srcid_size', 'pktid_size', 'trdid_size',
		'wrplen_size']
	tmpl_instanciation = (
		'soclib::caba::VciParams<%(cell_size)s,%(plen_size)s,%(addr_size)s,'+
		'%(rerror_size)s,%(clen_size)s,%(rflag_size)s,%(srcid_size)s,'+
		'%(pktid_size)s,%(trdid_size)s,%(wrplen_size)s>')

class VciTlmtModule(TlmtModule):
	tmpl_parameters = [
		'addr_t', 'data_t']
	tmpl_instanciation = [
		'soclib::tlmt::VciParams<%(addr_t)s,%(data_t)s>']

def Port(name, count):
	return None

class Parameter:
	def __init__(self, type, name):
		self.__type = type
		self.__name = name

class CabaSignals(CabaModule):
	pass

class TlmtSignals(TlmtModule):
	pass

def getDesc(mode, name):
	r = all_registry[mode]
	try:
		return r.get(name)
	except NoSuchComponent, e:
		raise NoSuchComponent("%s: %s"%(mode, name))

def getAllDescs():
	r = ()
	for name, cl in all_registry.iteritems():
		r += (name, cl.getAll()),
	return r
