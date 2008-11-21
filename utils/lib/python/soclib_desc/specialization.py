
import parameter
import module
from component import Uses

class Specialization:
	def __init__(self, mod, **args):
		if not isinstance(mod, module.Module):
			mod = module.Module.getRegistered(mod)
		deprecated = mod['deprecated']
		if deprecated:
			print 'Warning: %s is deprecated: "%s"'%(
				mod.getModuleName(), deprecated)
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
