
import types

class ParameterError(Exception): pass

class Base:
	valid_types = ()
	def __init__(self, name, default = None):
		self.name = name
		self.default = default

	def setModule(self, module):
		self.owner = module

	def getTmplType(self, args):
		raise ParameterError("Base parameter has not type")
		
	def get(self, env, values):
		if self.name in values:
			value = values[self.name]
		else:
			assert self.default is not None
			value = self.default
		self.assertValid(value)
		return value
	def assertValid(self, value):
		ok = isinstance(value, self.valid_types)
		if not ok:
			raise ParameterError("Invalid type `%s' for parameter `%s'"%(value, self.name))
	def getTmplUse(self, module):
		pass

	def getAllBuilders(self, args):
		return set()

	def getParamBuilders(self, args):
		return set()
	
	def argval(self, args):
		try:
			return args[self.name]
		except KeyError:
			if self.default is None:
				raise ParameterError("No parameter `%s' specified for %r"%(self.name, self.owner))
			return self.default

class Bool(Base):
	valid_types = (bool)
	def __init__(self, name, default = None):
		Base.__init__(self, name, bool(default))
	
	def getTmplType(self, args):
		value = self.argval(args)
		return str(value).lower()

class Int(Base):
	valid_types = (int)
	def __init__(self, name, default = None, min_ = None, max_ = None):
		Base.__init__(self, name, default)
		self.min = min_
		self.max = max_

	
	def getTmplType(self, args):
		return self.argval(args)

	def assertValid(self, value):
		Base.assertValid(self, value)
		if self.max is not None and value >= self.max:
			raise ParameterError("Invalid value `%s' for parameter `%s': above %d"%(value, self.name, self.max))
		if self.min is not None and value < self.min:
			raise ParameterError("Invalid value `%s' for parameter `%s': below %d"%(value, self.name, self.min))

class IntTab(Base):
	valid_types = (types.IntTab)

class MappingTable(Base):
	valid_types = (types.MappingTable)
	def get(self, env, values):
		return env['mapping_table']

class Type(Base):
	valid_types = (str, unicode)

	def getTmplType(self, args):
		return self.argval(args)

class Module(Base):
	def __init__(self, name, typename = None):
		Base.__init__(self, name, typename)

	def __getSpec(self, args):
		value = self.argval(args)
		from specialization import Specialization
		name = self.owner.fullyQualifiedModuleName(value)
		return Specialization(name, **args)

	def get(self, env, values):
		assert not "implemented"

	def setModule(self, module):
		Base.setModule(self, module)

	def getTmplType(self, args):
		return self.__getSpec(args).getType()

	def getAllBuilders(self, args):
		spec = self.__getSpec(args)
		builders = spec.getAllBuilders()
		builders.add(spec.builder())
		return builders

	def getParamBuilders(self, args):
		spec = self.__getSpec(args)
		builders = spec.getParamBuilders()
		builders.add(spec.builder())
		return builders

