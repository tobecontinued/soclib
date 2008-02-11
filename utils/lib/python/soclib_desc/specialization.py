
import module
from component import Uses

class Specialization:
	def __init__(self, mod, **args):
		if not isinstance(mod, module.Module):
			mod = module.Module.getRegistered(mod)
		self.__cdef =  mod
		self.__args = args
	def putArgs(self, d):
		d.update(self.__args)
	def getTmplParams(self):
		tp = self.__cdef['tmpl_parameters']
		if tp:
			def func(x):
				return str(x.getTmplType(self.__args))
			return ','.join(map(func, tp))
		return ''
		
	def getType(self):
		tp = self.getTmplParams()
		if tp:
			tp = '<'+tp+'> '
		return self.__cdef['classname']+tp

	def fullyQualifiedModuleName(self, name):
		return self.__cdef.fullyQualifiedModuleName(name)

	def descAttr(self, name):
		return self.__cdef[name]

	def getAllBuilders(self):
		r = set()
		for i in self.__cdef.getUses(self.__args):
			s = i.builder(self)
			r.add(s)
		for i in self.__cdef['tmpl_parameters']:
			r |= i.getAllBuilders(self.__args)
		return r

	def getParamBuilders(self):
		r = set()
		for i in self.__cdef['tmpl_parameters']:
			r |= i.getParamBuilders(self.__args)
		return r

	def builder(self):
		from soclib_cc import component_builder
		return component_builder.ComponentBuilder(self, None)

	def __hash__(self):
		return hash(self.__cdef)^hash(str(self.__args))

	def __str__(self):
		return '<Specialization of %s: %r>'%(
			self.__cdef,
			self.__args)
