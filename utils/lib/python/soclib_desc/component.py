
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

import traceback
import copy

from module import *
import parameter, types

__all__ = ['Module', 'PortDecl',
		   'parameter', 'types',
		   'Signal','Port']

class Port:
	def __init__(self, type, name, count = None, auto = None, **args):
		self.__type = type
		self.__name = name
		self.__count = count
		self.__owner = None
		self.__auto = auto
		self.__args = args
		self.where = traceback.extract_stack()[-2][0:2]
	def setModule(self, module):
		self.__type = module.fullyQualifiedModuleName(self.__type, self)
		self.__module = module
	def getUse(self, module):
		from specialization import Specialization
		ptype = Specialization(self.__type, **self.__args)
		module.addUse(Uses(self.__type, **self.__args))
	def getInfo(self):
		from specialization import Specialization
		ptype = Specialization(self.__type, **self.__args)
		return self.__name, ptype, self.__count, self.__auto
	def __str__(self):
		from specialization import Specialization
		ptype = Specialization(self.__type, **self.__args)
		return '<port: %s %s>'%(self.__name, str(ptype['header_files']))

class Signal(Module):
	tb_delta = -3
	def __init__(self, name, **kwargs):
		accepts = kwargs['accepts']
		del kwargs['accepts']
		Module.__init__(self, name, **kwargs)
		self.setAttr('accepts', accepts)

	def resolveRefsFor(self):
		Module.resolveRefsFor(self)
		naccepts = {}
		for type, count in self['accepts'].iteritems():
			rtype = self.getRegistered(type)
			naccepts[rtype] = count
		self.setAttr('accepts', naccepts)

class PortDecl(Module):
	tb_delta = -3
	def __init__(self, name, **kwargs):
		Module.__init__(self, name, **kwargs)
	def resolveRefsFor(self):
		Module.resolveRefsFor(self)
		if self['signal'] is not None:
			self.setAttr('signal', self.getRegistered(self['signal']))

class Uses:
	"""
	A statement declaring the platform uses a specific component (in a
	global meaning, ie hardware component or utility).

	name is the name of the component, it the filename as in
	desc/soclib/[component].sd

	args is the list of arguments useful for compile-time definition
	(ie template parameters)
	"""
	def __init__(self, name, **args):
		self.name = name
		self.args = args
		# This is for error feedback purposes
		self.where = traceback.extract_stack()[-2][0:2]
	def clone(self, **args):
		a = args
		a.update(self.args)
		r = self.__class__(self.name, **a)
		r.where = self.where
		return r
	def __str__(self):
		return '<Use %s>'%(self.name)
	def specialization(self):
		from specialization import Specialization
		return Specialization(self.name, __use = self.where, **self.args)
	def builder(self, parent):
		self.name = parent.fullyQualifiedModuleName(self.name)
		from soclib_desc import specialization
		args = {}
		args.update(self.args)
		parent.putArgs(args)
		def resolve(newv):
			if not '%' in str(newv):
				return newv
			v = None
			while newv != v:
				v = newv
				newv = newv%args
			v = v.replace('"', '\\"')
			return v
		for k in args.keys():
			v = args[k]
			if isinstance(v, list):
				v = map(resolve, v)
			elif isinstance(v, str):
				v = resolve(v)
			else:
				v = resolve(str(v))
			args[k] = v
		spec = specialization.Specialization(self.name, __use = self.where, **args)
		from soclib_cc import component_builder
		return component_builder.ComponentBuilder(spec, self.where)
	def __repr__(self):
		return str(self)
	def __cmp__(self, other):
		return (
			cmp(self.name, other.name) or
			cmp(self.args, other.args)
			)
	def __hash__(self):
		return (
			hash(self.name)^
			hash(`self.args`))
