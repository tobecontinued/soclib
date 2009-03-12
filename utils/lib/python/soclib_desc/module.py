
import os, os.path
import sys
import traceback
import warnings
import soclib_utils.repos_file

__id__ = "$Id$"
__version__ = "$Revision$"

class DoubleRegistrationWarning(Warning):
	def __str__(self):
		return 'Module %s registered twice, previous registration in "%s"'%(self.args[0], self.args[1])

class SpuriousDeclarationWarning(Warning):
	def __str__(self):
		return 'Spurious "%s" in %s declaration'%(self.args[0], self.args[1])

class InvalidComponentWarning(Warning):
	def __str__(self):
		return 'Invalid component %s, it will be unavailable. Error: "%s"'%(self.args[0], self.args[1])

__all__ = ['Module']

class NoSuchComponent(Exception):
	pass

class Module:
	"""
	The module class acts as a registry for all its instances. Therefore,
	all modules, signals, ports, ... registered in .sd files can be
	looked up for.
	"""
	
	# class part
	__reg = {}
	__module2file = {}

	def getUsedModules(cls):
		"""
		Returns a list of modules used at least one since
		initialization.
		"""
		return filter(lambda x:x.__use_count, cls.__reg.itervalues())
	getUsedModules = classmethod(getUsedModules)
	
	def __register(cls, name, obj, filename):
#		print 'Registering', name, obj
		if name in cls.__reg and cls.__module2file[name] != filename:
			warnings.warn(DoubleRegistrationWarning(name, cls.__module2file[name]), stacklevel = 3)
		cls.__reg[name] = obj
		cls.__module2file[name] = filename
	__register = classmethod(__register)

	def getRegistered(cls, name):
		"""
		Returns a module from its fqmn, if not available, a
		NoSuchComponent exception is raised.
		"""
		try:
			return cls.__reg[name]
		except KeyError:
			raise NoSuchComponent("`%s`"%(name))
	getRegistered = classmethod(getRegistered)

	def allRegistered(cls):
		"""
		Returns a dict of all modules registered, indexed by fqmn
		"""
		return cls.__reg
	allRegistered = classmethod(allRegistered)

	# instance part
	module_attrs = {
		'classname' : '',
		'tmpl_parameters' : [],
		'header_files' : [],
		'global_header_files' : [],
		'implementation_files' : [],
		'object_files' : [],
		'uses' : [],
		'defines' : {},
		'ports' : [],
		'signal' : None,
		'instance_parameters' : [],
		'local' : False,
		'extensions' : [],
		'constants' : {},
		"debug" : False,
		"deprecated":'',
		}
	tb_delta = -2

	def __init__(self, typename, **attrs):
		"""
		Creation of a module, with any overrides to defaults
		parameters defined in module_attrs class member.
		"""
		self.__attrs = {}
		self.__typename = typename
		base_attrs = self.__class__.__dict__
		for name, value in self.module_attrs.iteritems():
			if hasattr(self, name):
				value = getattr(self, name)
			self.__attrs[name] = value
		filename, lineno = traceback.extract_stack()[self.tb_delta][:2]
		for name, value in attrs.iteritems():
			if not name in self.module_attrs:
				warnings.warn(SpuriousDeclarationWarning(name, typename), stacklevel = 2)
			self.__attrs[name] = value
		self.__attrs['uses'] = set(self.__attrs['uses'])
		for p in self['ports']:
			self.__attrs['uses'].add(p.Use())

		self._mk_abs_paths(os.path.dirname(filename))
		self.__register(self.__typename, self, filename)
		self.__filename = filename
		self.lineno = lineno
		self.__use_count = 0

	def instanciated(self):
		"""
		Method to advertize for module usage.
		"""
		self.__use_count += 1

	def getModuleName(self):
		"""
		Gets the fqmn
		"""
		return self.__typename

	def __getitem__(self, name):
		"""
		Gets the attribute 'name'
		"""
		import copy
		return copy.copy(self.__attrs[name])

	def _mk_abs_paths(self, basename):
		relative_path_files = ['header_files', 'implementation_files', 'object_files']
		def mkabs(name):
			return os.path.isabs(name) \
				   and name \
				   or os.path.abspath(os.path.join(basename, name))
		for attr in relative_path_files:
			self.__attrs['abs_'+attr] = map(mkabs, self.__attrs[attr])
		self.__attrs['abs_header_files'] += self.__attrs['global_header_files']

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
		return '<Module %s>'%(self.__typename)

	def __repr__(self):
		return 'soclib_desc.module.Module.getRegistered(%r)'%(self.__typename)

	def __hash__(self):
		return hash((self.__filename, self.lineno))
