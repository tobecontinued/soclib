
class ParameterError(Exception): pass

class Base:
	valid_types = ()
	def __init__(self, name, default = None, auto = None):
		self.name = name
		self.default = default
		self.auto = auto

	def setModule(self, module):
		self.owner = module

	def getTmplType(self, args):
		raise ParameterError("Base parameter has not type")
		
	def get(self, env, values):
		value = None
		if self.name in values:
			value = values[self.name]
		if value is None and self.auto:
			method, key = self.auto.split(':', 1)
			if method == 'env':
				if key in env:
					value = env[key]
			else:
				raise ValueError('Auto method %s not implemented in %s'%(method, self))
		if value is None:
			value = self.default
		if value is None:
			raise ValueError("Pleasy give a value for parameter `%s'"%self.name)
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

	def instValue(self, env, param):
		return self.get(env, param)
	
	def argval(self, args):
		try:
			return args[self.name]
		except KeyError:
			if self.default is None:
				raise ParameterError("No parameter `%s' specified for %r"%(self.name, self.owner))
			return self.default

	def __str__(self):
		return '<Parameter %s: %s>'%(self.__class__.__name__, self.name)

class Bool(Base):
	valid_types = (bool)
	def __init__(self, name, default = None, auto = None):
		Base.__init__(self, name, bool(default), auto)
	
	def getTmplType(self, args):
		value = self.argval(args)
		return str(value).lower()

class Int(Base):
	valid_types = (int, long)
	def __init__(self, name, default = None, min = None, max = None, auto = None):
		Base.__init__(self, name, default, auto)
		self.min = min
		self.max = max
	
	def getTmplType(self, args):
		return self.argval(args)

	def assertValid(self, value):
		Base.assertValid(self, value)
		if self.max is not None and value >= self.max:
			raise ParameterError("Invalid value `%s' for parameter `%s': above %d"%(value, self.name, self.max))
		if self.min is not None and value < self.min:
			raise ParameterError("Invalid value `%s' for parameter `%s': below %d"%(value, self.name, self.min))

	def instValue(self, env, param):
		v = Base.instValue(self, env, param)
		if v < 4096:
			return '%d'%v
		else:
			return '0x%08x'%v

class String(Base):
	valid_types = (str)
	def instValue(self, env, param):
		return '"%s"'%Base.instValue(self, env, param)

class StringArray(Base):
	valid_types = (list)
	def instValue(self, env, param):
		v = Base.instValue(self, env, param)
		return 'stringArray(%s)'%(
			', '.join(map(lambda x:'"%s"'%(x%param).replace('"', '\\"'), v)))

class IntTab(Base):
	valid_types = (tuple, list)
	def instValue(self, env, param):
		v = Base.instValue(self, env, param)
		return 'soclib::common::IntTab(%s)'%(', '.join(map(str, v)))

class Type(Base):
	valid_types = (str, unicode)

	def getTmplType(self, args):
		return self.argval(args)

class Module(Base):
	def __init__(self, name, typename = None, default = None, auto = None):
		Base.__init__(self, name, default, auto)
		self.typename = typename
		
	def __getSpec(self, args):
		value = self.argval(args)
		from specialization import Specialization
		name = self.owner.fullyQualifiedModuleName(value)
		return Specialization(name, **args)

	def setModule(self, module):
		Base.setModule(self, module)

	def assertValid(self, value):
		if value.typename() != self.typename:
			raise ValueError('Incorrect value type for %s: %s != %s'%(self, value.typename(), self.typename))

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

	def inst(self, env, param):
		return Base.instValue(self, env, param)

	def instValue(self, env, param):
		r = Base.instValue(self, env, param)
		return r.ref()

class Reference:
	def __init__(self, name, mode = 'val'):
		self.__name = name
		self.__mode = mode
	def setValue(self, args, value):
		args[self.__name] = value
	def getValue(self, args):
		if not self.__name in args:
			return None
		if self.__mode == 'val':
			return args[self.__name]
		elif self.__mode == 'len':
			return len(args[self.__name])
		else:
			raise ValueError("Unsupported mode %s for parameter.Reference"%self.__mode)

class Constant:
	def __init__(self, name):
		self.__name = name
	def name(self):
		return self.__name
