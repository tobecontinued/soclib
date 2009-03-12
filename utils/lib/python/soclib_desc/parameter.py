
import operator

__id__ = "$Id$"
__version__ = "$Revision$"

class ParameterError(Exception): pass

class Base:
	@staticmethod
	def get_tmpl_value(value, **args):
		return value

	@staticmethod
	def get_inst_value(value, **args):
		return value

class Parameter(Base):
	valid_types = ()
	def __init__(self, name, default = None, auto = None):
		self.name = name
		self.default = default
		self.auto = auto

	def resolve(self, args):
		value = None
		if self.name in args:
			value = args[self.name]
 		if value is None and self.auto and self.auto in args:
			value = args[self.auto]
		if value is None:
			value = self.default
		if value is None:
			raise ValueError("Pleasy give a value for parameter `%s'"%self.name)
		self.assertValid(value)
		return value
		
	def assertValid(self, value):
		try:
			value = self.valid_types[0](value)
		except:
			pass
#		print type(value), self.valid_types, self
		ok = isinstance(value, self.valid_types)
		if not ok:
			raise ParameterError("Invalid type `%s' for parameter `%s'"%(value, self.name))

	def __str__(self):
		return '<Parameter %s: %s>'%(self.__class__.__name__, self.name)

	def __repr__(self):
		return '%s(%r, %r, %r)'%(self.__class__.__name__,
								 self.name,
								 self.default,
								 self.auto)

	l = locals()
	for op in 'mul', 'add', 'mod':
		name = '__%s__'%op
		l[op] = lambda self, right: BinaryOp(getattr(operator, op), self, right)

class Bool(Parameter):
	valid_types = (bool,)
	def __init__(self, name, default = None, auto = None):
		Parameter.__init__(self, name, bool(default), auto)

	@staticmethod
	def get_tmpl_value(value, **args):
		return str(value).lower()

	@staticmethod
	def get_inst_value(value, **args):
		return str(value).lower()

class Int(Parameter):
	valid_types = (int, long)
	def __init__(self, name, default = None, min = None, max = None, auto = None):
		Parameter.__init__(self, name, default, auto)
		self.min = min
		self.max = max


	def __repr__(self):
		return '%s(%r, %r, min = %r, max = %r, auto = %r)'%(self.__class__.__name__,
								 self.name,
								 self.default,
															self.min, self.max,
								 self.auto)

	@staticmethod
	def getTmplType(value):
		return value

	def assertValid(self, value):
		Parameter.assertValid(self, value)
		if self.max is not None and value >= self.max:
			raise ParameterError("Invalid value `%s' for parameter `%s': above %d"%(value, self.name, self.max))
		if self.min is not None and value < self.min:
			raise ParameterError("Invalid value `%s' for parameter `%s': below %d"%(value, self.name, self.min))

	@staticmethod
	def get_inst_value(v, **args):
		if v < 4096:
			return '%d'%v
		else:
			return '0x%08x'%v

class String(Parameter):
	valid_types = (str,)
	@staticmethod
	def get_inst_value(value, **args):
		return '"%s"'%value

class StringArray(Parameter):
	valid_types = (list,)
	@staticmethod
	def get_inst_value(value, **args):
		elems = ', '.join(map(lambda x:'"%s"'%x.replace('"', '\\"'), value)
						  +["NULL"])
		return 'stringArray(%s)'%(elems)

class IntArray(Parameter):
	valid_types = (list,)
	@staticmethod
	def get_inst_value(value, **args):
		v = value
		elems = ', '.join(map(str, v))
		return 'intArray(%d, %s)'%(len(v), elems)

class IntTab(Parameter):
	valid_types = (tuple, list)
	@staticmethod
	def get_inst_value(value, **args):
		v = value
		return 'soclib::common::IntTab(%s)'%(', '.join(map(str, v)))

class Type(Parameter):
	valid_types = (str, unicode)

	@staticmethod
	def get_tmpl_value(v, **args):
		return v

class Module(Type):
	valid_types = (str, unicode)

	def __init__(self, name, typename = None, default = None, auto = None):
		Parameter.__init__(self, name, default, auto)
		self.typename = typename

	def __repr__(self):
		return '%s(%r, typename = %r, default = %r, auto = %r)'%(
			self.__class__.__name__,
			self.typename,
			self.name,
			self.default,
			self.auto)

	@staticmethod
	def get_inst_value(value, **args):
		return value.ref()

	@staticmethod
	def get_tmpl_value(value, **args):
		import specialization
		return specialization.Specialization(value, **args)

class Reference(Base):
	def __init__(self, name, mode = 'val'):
		self.__name = name
		self.__mode = mode

	@property
	def name(self):
		return self.__name

	def __repr__(self):
		return '%s(%r, %r)'%(
			self.__class__.__name__,
			self.__name,
			self.__mode)
		
#	def setValue(self, args, value):
#		args[self.__name] = value

	def resolve(self, args):
		if self.__mode == 'val':
			r = args[self.__name]
		elif self.__mode == 'len':
			r = len(args[self.__name])
		else:
			raise ValueError("Unsupported mode %s for parameter.Reference"%self.__mode)
		return r

class Constant:
	def __init__(self, name):
		self.__name = name
	def name(self):
		return self.__name

class BinaryOp(Base):
	def __init__(self, op, left, right):
		self.__op = op
		self.__left = left
		self.__right = right

	def __repr__(self):
		return '%s(%r, %r)'%(
			self.__op,
			self.__left,
			self.__right)

	def resolve(self, args):
		self.__left = value(self.__left, args, 'inst')
		self.__right = value(self.__right, args, 'inst')
#		print self.__op, self.__left, self.__right
		return self.__op(self.__left, self.__right)

class StringExt(BinaryOp):
	def __init__(self, st, *a):
		BinaryOp.__init__(self, operator.mod, st, a)

def resolve(v, args):
	if isinstance(v, (list, tuple)):
		v = type(v)(map(lambda x:resolve(x, args), v))
#		print "pwet", v
	elif isinstance(v, Base):
		return v.resolve(args)
	return v

def value(thing, args, value_type):
	if isinstance(thing, Base):
		v = resolve(thing, args)
		f = getattr(thing, 'get_%s_value'%value_type)
#		print "***", thing, f, v
		a = {}
		for k in args:
			a[k] = args[k]
		return f(v, **a)
	if isinstance(thing, (list, tuple)):
		return type(thing)(map(lambda x:value(x, args, value_type), thing))
	return thing

