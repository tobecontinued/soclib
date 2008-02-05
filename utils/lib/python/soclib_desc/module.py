
import os, os.path
import sys
import traceback

__all__ = ['Module', 'getDesc', 'getAllDescs']

class NoSuchComponent(Exception):
	pass

class AllRegistry:
	def __init__(self):
		self.__reg = {}
	def all(self):
		return self.__reg
	def register(self, cl, name, obj):
		m = self.getClass(cl)
		m[name] = obj
	def get(self, cl, name):
		m = self.getClass(cl)
		return cl, m[name]
	def getClass(self, cl):
		if cl in self.__reg:
			return self.__reg[cl]
		r = {}
		self.__reg[cl] = r
		return r

global all_registry
all_registry = AllRegistry()

class Module:
	module_attrs = {
		'namespace' : '',
		'classname' : '',
		'tmpl_parameters' : [],
		'tmpl_instanciation' : "",
		'header_files' : [],
		'force_header_files' : [],
		'implementation_files' : [],
		'uses' : [],
		'default_parameters' : {},
		'defines' : {},
		'ports' : [],
		'mode' : None,
		'instance_parameters' : [],
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
		filename = traceback.extract_stack()[self.tb_delta][0]
		self.mk_abs_paths(os.path.dirname(filename))
		self.registerMe()
		for p in self['ports']:
			p.getUse(self)
	def addPort(self, name, port):
		self.ports[name] = port
		
	def registerMe(self):
		global all_registry
		override = all_registry.register(self.klass, self.__typename, self)
	def __getitem__(self, name):
		return self.__attrs[name]

	def mk_abs_paths(self, basename):
		relative_path_files = ['header_files', 'implementation_files', 'force_header_files']
		def mkabs(name):
			return os.path.isabs(name) \
				   and name \
				   or os.path.abspath(os.path.join(basename, name))
		for attr in relative_path_files:
			self.__attrs['abs_'+attr] = map(mkabs, self.__attrs[attr])

	def getInfo(self):
		r = '<%s\n'%self.__class__.__name__
		for l in 'abs_header_files', 'abs_force_header_files', 'abs_implementation_files':
			for s in self.__attrs[l]:
				r += os.path.isfile(s) and " + " or " - "
				r += s+'\n'
			return r+' >'

	def __str__(self):
		return '<%s %s>'%(self.klass, self.__typename)

def getDesc(klass, name):
	try:
		return all_registry.get(klass, name)
	except KeyError:
		raise NoSuchComponent("%s: %s"%(klass, name))

def getAllDescs():
	return all_registry.all()

