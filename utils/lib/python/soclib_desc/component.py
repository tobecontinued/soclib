
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

import traceback
import copy

from module import *
import parameter, types

__id__ = "$Id$"
__version__ = "$Revision$"

__all__ = ['Module', 'PortDecl',
		   'MetaSignal', 'MetaPortDecl', 'SubSignal',
		   'parameter', 'types',
		   'Signal','Port']

class SubConn:
	def __init__(self, type, name, count = None, auto = None, **args):
		self.__type = type
		self.__name = name
		self.__count = count
		self.__auto = auto
		self.__args = args
		self.where = traceback.extract_stack()[-2][0:2]
	def Use(self):
		return Uses(self.__type, **self.__args)
	def getInfo(self, **args):
		from specialization import Specialization
		a = {}
		a.update(args)
		a.update(self.__args)
		ptype = Specialization(self.__type, **a)
		return self.__name, ptype, self.__count, self.__auto
	def __str__(self):
		from specialization import Specialization
		ptype = Specialization(self.__type, **self.__args)
		return '<%s: %s %s>'%(self.__class__.__name__,
							  self.__name,
							  str(ptype['header_files']))

class Port(SubConn):
	pass

class SubSignal(SubConn):
	pass

class Signal(Module):
	tb_delta = Module.tb_delta-1
	def __init__(self, name, **kwargs):
		self.__accepts = kwargs['accepts']
		del kwargs['accepts']
		Module.__init__(self, name, **kwargs)

	def __getitem__(self, key):
		from soclib_desc import description_files
		if key == 'accepts':
			naccepts = {}
			for type, count in self.__accepts.iteritems():
				rtype = description_files.get_module(type)
				naccepts[rtype] = count
			return naccepts
		else:
			return Module.__getitem__(self, key)

class PortDecl(Module):
	def __getitem__(self, key):
		from soclib_desc import description_files
		if key == 'signal':
			sig = Module.__getitem__(self, 'signal')
			if sig:
				return description_files.get_module(sig)
		return Module.__getitem__(self, key)

class MetaPortDecl(PortDecl):
	pass

class MetaSignal(Signal):
	pass

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
		for k in filter(lambda x:':' in x, self.args.keys()):
			del self.args[k]

		self.__hash = (
			hash(self.name)^
			hash(str([(k, self.args[k]) for k in sorted(self.args)])))

		# This is for error feedback purposes
		self.where = traceback.extract_stack()[-2][0:2]

	def specialization(self, **args):	
		from specialization import Specialization
		a = {}
		a.update(args)
		a.update(self.args)
#		print 'use', `self`
#		from pprint import pprint
#		pprint(a)
		return Specialization(self.name, __use = self.where, **a)
		
	def __str__(self):
		return '<Use %s from %s>'%(self.name, self.where)

	def __repr__(self):
		args = ', '.join(['%s = %r'%(k, self.args[k]) for k in sorted(filter(lambda x:':' not in x, self.args.keys()))])
		return 'Uses(%r, %s)'%(self.name, args)

	def __cmp__(self, other):
		return (
			cmp(self.name, other.name) or
			cmp(self.args, other.args)
			)

	def __hash__(self):
		return self.__hash
