
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
#         Nicolas Pouillon <nipo@ssji.net>, 2009
# 
# Maintainers: group:toolmakers

import os, os.path
import sys
import traceback
import warnings
import soclib_utils.repos_file
from abstraction_levels import checker

__id__ = "$Id$"
__version__ = "$Revision$"

class SpuriousDeclarationWarning(Warning):
	def __str__(self):
		return 'Spurious "%s" in %s declaration'%(self.args[0], self.args[1])

class BadInterfacePath(Warning):
	def __str__(self):
		return 'Interface file path "%s" %s'%(self.args[0], self.args[1])

class BadNameWarning(Warning):
	def __str__(self):
		return 'Bad component name: `%s\', %s'%(self.args[0], self.args[1])

class InvalidComponentWarning(Warning):
	def __str__(self):
		return 'Invalid component %s, it will be unavailable. Error: "%s"'%(self.args[0], self.args[1])

__all__ = ['Module']

class InvalidComponent(Exception):
	pass

class Module:

	def getUsedModules(cls):
		"""
		Returns a list of modules used at least one since
		initialization.
		"""
		warnings.warn("Please use soclib_desc.description_files.get_all_used_modules()",
					  DeprecationWarning,
					  2)
		return filter(lambda x:x.isUsed(),
					  cls.allRegistered())
	getUsedModules = classmethod(getUsedModules)

	def getRegistered(cls, name):
		"""
		Returns a module from its fqmn, if not available, a
		NoSuchComponent exception is raised.
		"""
		warnings.warn("Please use soclib_desc.description_files.get_module(...)",
					  DeprecationWarning,
					  2)
		from description_files import get_module
		return get_module(name)
	getRegistered = classmethod(getRegistered)

	def allRegistered(cls):
		"""
		Returns a dict of all modules registered, indexed by fqmn
		"""
		warnings.warn("Please use soclib_desc.description_files.get_all_modules()",
					  DeprecationWarning,
					  2)
		from description_files import get_all_modules
		return get_all_modules()
	allRegistered = classmethod(allRegistered)

	# instance part
	module_attrs = {
		'classname' : '',
		'tmpl_parameters' : [],
		'header_files' : [],
		'global_header_files' : [],
		'implementation_files' : [],
		'object_files' : [],
		'interface_files' : [],
		'uses' : [],
		'defines' : {},
		'ports' : [],
		'sub_signals' : [],
		'signal' : None,
		'instance_parameters' : [],
		'local' : False,
		'extensions' : [],
		'constants' : {},
		"debug" : False,
		"debug_saved" : False,
		"deprecated":'',
		}
	tb_delta = -3

	def __init__(self, typename, **attrs):
		"""
		Creation of a module, with any overrides to defaults
		parameters defined in module_attrs class member.
		"""
		self.__attrs = {}
		self.__typename = typename

# 		for c in self.__typename:
# 			if c.isupper():
# 				warnings.warn(BadNameWarning(
# 					self.__typename, "Please use lowercase letters"),
# 							  stacklevel = -self.tb_delta)

		base_attrs = self.__class__.__dict__
		for name, value in self.module_attrs.iteritems():
			if hasattr(self, name):
				value = getattr(self, name)
			self.__attrs[name] = value
		self.__attrs['abstraction_level'] = self.__typename.split(':', 1)[0]
		filename, lineno = traceback.extract_stack()[self.tb_delta][:2]
		for name, value in attrs.iteritems():
			if not name in self.module_attrs:
				warnings.warn(SpuriousDeclarationWarning(name, typename),
							  stacklevel = -self.tb_delta)
			self.__attrs[name] = value
		self.__attrs['uses'] = set(self.__attrs['uses'])
		for p in self['ports']:
			self.__attrs['uses'].add(p.Use())

		self._mk_abs_paths(os.path.dirname(filename))
		self.__filename = filename
		self.lineno = lineno
		self.__use_count = 0

		if self.__attrs['classname']:
			c = checker[self.__attrs["abstraction_level"]]
			if not c.validClassName(self.__attrs['classname']):
				raise InvalidComponent("Invalid class name '%s' level %s: '%s'"%(
					self.__typename, c, self.__attrs['classname']))

		for f in self.__attrs['interface_files']:
			b = os.path.basename(f)
			d = os.path.dirname(f)
			soclib = os.path.basename(d)
			if soclib != 'soclib':
				warnings.warn(BadInterfacePath(f, 'path should end with "soclib/%s"'%b))

		self.__attrs['debug_saved'] = self.__attrs['debug']

	def instanciated(self):
		"""
		Method to advertize for module usage.
		"""
		self.__use_count += 1

	def isUsed(self):
		return bool(self.__use_count)

	def getModuleName(self):
		"""
		Gets the fqmn
		"""
		return self.__typename

	def forceDebug(self):
		self.__attrs['debug'] = True

	def cleanup(self):
		try:
			self.__attrs['debug'] = self.__attrs['debug_saved']
		except:
			pass

	def __getitem__(self, name):
		"""
		Gets the attribute 'name'
		"""
		import copy
		return copy.copy(self.__attrs[name])

	def _mk_abs_paths(self, basename):
		relative_path_files = ['header_files', 'implementation_files', 'object_files', 'interface_files']
		def mkabs(name):
			return os.path.isabs(name) \
				   and name \
				   or os.path.abspath(os.path.join(basename, name))
		for attr in relative_path_files:
			self.__attrs['abs_'+attr] = map(mkabs, self.__attrs[attr])
		self.__attrs['abs_header_files'] += self.__attrs['global_header_files']
		self.__attrs['abs_header_files'] += self.__attrs['abs_interface_files']

	def getInfo(self):
		"""
		Returns module information (implementation files and paths)
		"""
		r = '<%s\n'%self.__class__.__name__
		for l in 'abs_header_files', 'abs_implementation_files', 'abs_object_files', 'global_header_files':
			for s in self.__attrs[l]:
				r += os.path.isfile(s) and " + " or " - "
				r += s+'\n'
		return r+' >'

	def files(self):
		"""
		Returns module's implementation files
		"""
		r = [self.__filename]
		for l in ('abs_header_files', 'abs_implementation_files',
				  'abs_object_files', 'global_header_files'):
			r += self.__attrs[l]
		return r

	def __str__(self):
		return '<Module %s in %s>'%(self.__typename, self.__filename)

	def __repr__(self):
		return 'soclib_desc.registry.getRegistered(%r)'%(self.__typename)

	def pathIs(self, path):
		return path == self.__filename

	def __hash__(self):
		return hash((self.__filename, self.lineno))
