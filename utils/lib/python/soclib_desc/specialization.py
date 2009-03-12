
import parameter
import module
import warnings

__id__ = "$Id$"
__version__ = "$Revision$"

class ModuleDeprecationWarning(DeprecationWarning):
	def __str__(self):
		return 'Module %s deprecated: "%s"'%(self.args[0], self.args[1])

class ModuleSpecializationError(Exception):
	def __init__(self, module, context = None, prev_error = None):
		self.__module = module
		self.__context = context
		self.__prev_error = prev_error
	def __str__(self):
		return '\n'+self.format()
	def format(self, pfx = ' '):
		if isinstance(self.__prev_error, ModuleSpecializationError):
			c = '\n'+pfx+self.__prev_error.format(pfx+' ')
		else:
			c = '\n'+pfx+' '+repr(self.__prev_error)
		at = ""
		if self.__context:
			at = (' at %s'%self.__context)
		return 'Error specializing %s%s, error: %s'%(self.__module, at, c)

class Specialization:
	def __init__(self, mod, **args):
		if not isinstance(mod, module.Module):
			mod = module.Module.getRegistered(mod)
		self.__cdef =  mod
		self.__cdef.instanciated()
		
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

		try:
			self.__init_args(args)
			self.__init_tmpl_parameters()
			self.__init_tmpl_dependencies()
			self.__init_uses()
#		except ModuleSpecializationError, e:
#			raise ModuleSpecializationError(
#				self.getModuleName(), use, e)
		except Exception, e:
#			print "Error while instanciating component %s from %s: %r"%(
#				self.getModuleName(), use, e)
			raise ModuleSpecializationError(
				self.getModuleName(), use, e)


		self.__dependencies = self.__uses | self.__tmpl_dependencies

		self.__uid = hash(str(self))

	def getTmplParamStr(self):
		return ','.join(map(lambda x:str(x), self.__tmpl_parameters))

	def getModuleName(self):
		return self.__cdef.getModuleName()
		
	def getType(self):
		if not self.__cdef['classname']:
			return ''
		tp = self.getTmplParamStr()
		if tp:
			tp = '<'+tp+' > '
		return self.__cdef['classname']+tp

	def getConstant(self, name):
		c = self.__cdef['constants']
		if name in c:
			return c[name]
		for s in self.__dependencies:
			try:
				return s.getConstant(name)
			except Exception, e:
				pass
		raise ValueError('Constant %s not found in %s'%(name, self.getModuleName()))

	def getParam(self, name):
		try:
			return self.__args[name]
		except:
			return self.__cdef[name]

	def getSubTree(self, pfx = ''):
#		if (self.__dependencies) and self.getModuleName() == 'caba:base_module':
#			print pfx, 'getSubTree', self
#			for d in self.__dependencies:
#				print pfx, ' d: ', `d`
		r = set((self,))
		for m in set(self.__dependencies):
			r |= m.getSubTree(pfx + '  ')
		return r

	def getTmplSubTree(self, pfx = ''):
#		if (self.__dependencies) and self.getModuleName() == 'caba:base_module':
#			print pfx, 'getSubTree', self
#			for d in self.__dependencies:
#				print pfx, ' d: ', `d`
		r = set()
		for m in set(self.__tmpl_dependencies):
			r |= m.getSubTree(pfx + '  ')
		return r

	def printAllUses(self, pfx = ''):
		print pfx, str(self)
		for i in set(self.__dependencies):
			i.printAllUses(pfx+'  ')

	def getParamBuilders(self):
		r = set()
		for tp in self.__dependencies:
			for i in tp.getSubTree():
				r |= i.getParamBuilders()
		return r

	def __hash__(self):
		return self.__uid

	def __eq__(self, other):
		return type(self) == type(other) and \
			   self.__uid == other.__uid

	def __str__(self):
		return self.getType() or '<Spec of %s>'%self.getModuleName()

	def __repr__(self):
		kv = []
		for k in sorted(self.__args.iterkeys()):
			kv.append("%s = %r"%(k, self.__args[k]))
		return 'soclib_desc.specialization.Specialization(%r, %s)'%(
			self.getModuleName(),
			', '.join(kv))

	def getHeaderFiles(self):
		return self.__cdef['abs_header_files']

	def getImplementationFiles(self):
		return self.__cdef['abs_implementation_files']

	def getObjectFiles(self):
		return self.__cdef['abs_object_files']

	def getDefines(self):
		defs = self.__cdef['defines']
		if self.__cdef['debug']:
			defs['SOCLIB_MODULE_DEBUG'] = '1'
		return defs

	def isLocal(self):
		return self.__cdef['local']
	
	def isDebug(self):
		return self.__cdef['debug']

	def getInstanceParameters(self):
		return self.__cdef['instance_parameters']

	def getAccepts(self):
		return self.__cdef['accepts']

	def getExtensions(self):
		return self.__cdef['extensions']

	def getSignal(self):
		return self.__cdef['signal']

	def getPorts(self):
		return self.__cdef['ports']


	def __init_args(self, args):
		self.__args = {}

		args_to_resolve = list(args.iteritems())
		last_args_to_resolve = []
		while args_to_resolve:
			if args_to_resolve == last_args_to_resolve:
				raise RuntimeError("Unresolved parameters: %r"%args_to_resolve)
			unresolved = []
			for k, v in args_to_resolve:
				try:
					v = parameter.resolve(v, self.__args)
					if isinstance(v, parameter.Base):
						v = v.value
					self.__args[k] = v
#				except Exception, e:
#					raise
				except:
					unresolved.append((k, v))
			last_args_to_resolve = args_to_resolve
			args_to_resolve = unresolved

	def __init_tmpl_parameters(self):
		self.__tmpl_parameters = map(
			lambda x:parameter.value(x, self.__args, 'tmpl'),
			self.__cdef['tmpl_parameters'])

	def __init_uses(self):
		self.__uses = set()
		for u in self.__cdef['uses']:
			s = u.specialization(**self.__args)
			self.__uses |= s.getSubTree()

	def __init_tmpl_dependencies(self):
		self.__tmpl_dependencies = set()
		for i in self.__cdef['tmpl_parameters']:
			if isinstance(i, parameter.Module):
				self.__tmpl_dependencies |= parameter.value(i, self.__args, 'tmpl').getSubTree()


	def getUse(self):
		import component
		return component.Uses(self.__cdef.getModuleName(), **self.__args)
