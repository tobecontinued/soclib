
import os, os.path
import sys
import traceback
import warnings

__all__ = ['Module']

class NoSuchComponent(Exception):
	pass

class Module:
	# class part
	__reg = {}
	__not_done_registering = ()
	
	def __register(cls, name, obj):
#		print 'Registering', name, obj
		if name in cls.__reg:
			print "Warning ! module %s registered twice..."%name
		cls.__reg[name] = obj
		Module.__not_done_registering += obj,
	__register = classmethod(__register)

	def getRegistered(cls, name):
		cls.resolveRefs()
#		print 'Getting', name
		try:
			return cls.__reg[name]
		except KeyError:
			raise NoSuchComponent("`%s`"%(name))
	getRegistered = classmethod(getRegistered)

	def allRegistered(cls):
		cls.resolveRefs()
		return cls.__reg
	allRegistered = classmethod(allRegistered)

	def resolveRefs(cls):
		r = Module.__not_done_registering
		Module.__not_done_registering = ()
		for module in r:
			module.resolveRefsFor()
	resolveRefs = classmethod(resolveRefs)

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
		}
	tb_delta = -2

	def __init__(self, typename, **attrs):
		self.__attrs = {}
		self.__typename = typename
		base_attrs = self.__class__.__dict__
		for name, value in self.module_attrs.iteritems():
			if hasattr(self, name):
				value = getattr(self, name)
			self.__attrs[name] = value
		for name, value in attrs.iteritems():
			if not name in self.module_attrs:
				sys.stderr.write("Spurious %s in %s declration\n"%(name, typename))
			self.__attrs[name] = value
		self.__attrs['uses'] = set(self.__attrs['uses'])
		filename = traceback.extract_stack()[self.tb_delta][0]
		self.mk_abs_paths(os.path.dirname(filename))
		self.__register(self.__typename, self)

	def getModuleName(self):
		return self.__typename

	def fullyQualifiedModuleName(self, name):
		if not ':' in name:
			mode = self.__typename.split(':',1)[0]
			return mode + ':' + name
		return name

	def __getitem__(self, name):
		import copy
		return copy.copy(self.__attrs[name])

	def mk_abs_paths(self, basename):
		relative_path_files = ['header_files', 'implementation_files', 'object_files']
		def mkabs(name):
			return os.path.isabs(name) \
				   and name \
				   or os.path.abspath(os.path.join(basename, name))
		for attr in relative_path_files:
			self.__attrs['abs_'+attr] = map(mkabs, self.__attrs[attr])
		self.__attrs['abs_header_files'] += self.__attrs['global_header_files']

	def setAttr(self, name, value):
		self.__attrs[name] = value

	def resolveRefsFor(self):
#		print 'Resolving refs for', self
		for p in self['ports']:
			p.setModule(self)
			p.getUse(self)
		for p in self['tmpl_parameters']:
			p.setModule(self)

	def getUse(self, **args):
		from component import Uses
		return Uses(self.__typename, **args)

	def addUse(self, u):
#		print 'Adding', u, 'to', self
#		assert not (
#			self.__typename == u.name)
		from component import Uses
		self.__attrs['uses'].add(Uses(u.name, **u.args))

	def addDefine(self, name, val):
		self.__attrs['defines'][name] = val

	def getUses(self, args):
		from component import Uses
		r = set()
#		r.add(Uses(self.__typename, **args))
		for i in self.__attrs['uses']:
			r.add(i.clone(**args))
		return r

	def getInfo(self):
		r = '<%s\n'%self.__class__.__name__
		for l in 'abs_header_files', 'abs_force_header_files', 'abs_implementation_files', 'abs_object_files', 'global_header_files':
			for s in self.__attrs[l]:
				r += os.path.isfile(s) and " + " or " - "
				r += s+'\n'
		return r+' >'

	def __str__(self):
		return '<%s>'%(self.__typename)

	def __repr__(self):
		return 'soclib_desc.module.Module.getRegistered(%r)'%(self.__typename)
