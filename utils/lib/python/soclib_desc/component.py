
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

__all__ = ['VciCabaModule', 'CabaModule',
		   'CommonModule',
		   'TlmtModule', 'VciTlmtModule',
		   'Port',
		   'Parameter',
		   'CabaSignal','TlmtSignal',
		   'CabaPort','TlmtPort']

class CommonModule(Module):
	klass = 'common'
	namespace = 'soclib::common::'

class CabaModule(CommonModule):
	klass = 'caba'
	namespace = 'soclib::caba::'
	
class TlmtModule(CommonModule):
	klass = 'tlmt'
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
	tmpl_parameters = ['addr_t', 'data_t']
	tmpl_instanciation = 'soclib::tlmt::VciParams<%(addr_t)s,%(data_t)s>'

class Port:
	def __init__(self, type, name, count = 0):
		self.__type = type
		self.__name = name
		self.__count = count
		self.__owner = None
	def getUse(self, module):
		self.__mode = module.klass+'_port'
		mode, ptype = getDesc(self.__mode, self.__type)
		module['uses'].append(Uses(self.__type, self.__mode))
	def registerIn(self, module):
		self.__owner = module
		if self.__count == 0:
			n = self.__name
			module.addPort(n, self)
		else:
			for i in range(self.__count):
				n = '%s[%d]'%(self.__name, i)
				module.addPort(n, self)
	def __str__(self):
		mode, ptype = getDesc(self.__mode, self.__type)
		return '<port: %s %s>'%(self.__name, str(ptype['header_files']))

class Parameter:
	def __init__(self, type, name):
		self.__type = type
		self.__name = name

class Signal(Module):
	tb_delta = -3
	def __init__(self, name, classname, header_files, uses = [], accepts = {}):
		Module.__init__(self, name, mode = self.klass,
						header_files = header_files,
						uses = uses)
		self.accepts = accepts

class PortDecl(Module):
	tb_delta = -3
	def __init__(self, name, classname, header_files, uses = []):
		Module.__init__(self, name, mode = self.klass,
						header_files = header_files,
						uses = uses)

class CabaSignal(Signal):
	klass = 'caba_signal'

class CabaPort(PortDecl):
	klass = 'caba_port'

class TlmtSignal(Signal):
	klass = 'tlmt_signal'

class TlmtPort(PortDecl):
	klass = 'tlmt_port'

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
	def do(self, mode = None, **inherited_args):
		if self.mode is not None:
			mode = self.mode
		from soclib_desc import component
		mode, cdef = component.getDesc(mode, self.name)
		args = copy.copy(inherited_args)
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
		from soclib_cc import component_builder
		self.builder = component_builder.ComponentBuilder(mode, cdef, self.where, **args)
	def todo(self):
		return self.builder.withDeps()
