
import parameter
import module
import warnings
from component import Uses

class ModuleDeprecationWarning(DeprecationWarning):
	def __str__(self):
		return 'Module %s deprecated: "%s"'%(self.args[0], self.args[1])

class Specialization:
	def __init__(self, mod, **args):
		if not isinstance(mod, module.Module):
			mod = module.Module.getRegistered(mod)
		deprecated = mod['deprecated']
		use = None
		if '__use' in args:
			use = args['__use']
			del args['__use']
		if deprecated:
			if use:
				warnings.warn_explicit(ModuleDeprecationWarning(mod.getModuleName(), deprecated), ModuleDeprecationWarning, use[0], use[1])
			else:
				warnings.warn(ModuleDeprecationWarning(mod.getModuleName(), deprecated), stacklevel = 2)
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

	def getModuleName(self):
		return self.__cdef.getModuleName()
		
	def getType(self):
		tp = self.getTmplParams()
		if tp:
			tp = '<'+tp+'> '
		return self.__cdef['classname']+tp

#	def getConstant(self, name, p=''):
	def getConstant(self, name):
		from specialization import Specialization
		c = self.descAttr('constants')
#		print p, "Looking for constant", name, "in", self.getModuleName()
		if name in c:
#			print p, "found directly"
			return c[name]
		for u in self.__cdef['tmpl_parameters']:
			try:
#				print p, "Looking in tmpl ", u
				tn = u.argval(self.__args)
				n = self.__cdef.fullyQualifiedModuleName(tn)
				return Specialization(n, **self.__args).getConstant(name)
			except parameter.ParameterError:
				raise
			except Exception, e:
#				print p, "Error", e
				pass
		for u in self.__cdef.getUses(self.__args):
			try:
#				print p, "Looking in use ", u
				return u.specialization().getConstant(name)
			except Exception, e:
				pass
		raise ValueError('Constant %s not found in %s'%(name, self.getModuleName()))

	def fullyQualifiedModuleName(self, name):
		return self.__cdef.fullyQualifiedModuleName(name)

	def descAttr(self, name):
		return self.__cdef[name]

	def getParam(self, name):
		try:
			return self.__args[name]
		except:
			return self.__cdef[name]

	def getAllBuilders(self):
		r = set()
		for i in self.__cdef.getUses(self.__args):
			s = i.builder(self)
			r.add(s)
		for i in self.__cdef['tmpl_parameters']:
			r |= i.getAllBuilders(self.__args)
		return r

	def getUse(self):
		return self.__cdef.getUse(**self.__args)

	def getUses(self):
		return self.__cdef.getUses(self.__args)

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
