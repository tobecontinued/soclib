
import os, os.path
from soclib_cc.builder.action import *
from soclib_cc.builder.cxx import *
from soclib_cc.builder.textfile import *

class UndefinedParam(Exception):
	def __init__(self, where, comp, param):
		Exception.__init__(self)
		self.where = where
		self.comp = comp
		self.param = param
	def __str__(self):
		return '\n%s:error: parameter %s not defined for component %s'%(
			self.where, self.param, self.comp)

class ComponentInstanciationError(Exception):
	def __init__(self, where, comp, err):
		Exception.__init__(self)
		self.where = where
		self.comp = comp
		self.err = err
	def __str__(self):
		return '\n%s:error: component %s: %s'%(
			self.where, self.comp, self.err)

class ComponentBuilder:
	def __init__(self, mode, cdef, where, **args):
		self.module = cdef
		self.where = where
		self.args = args
		self.mode = mode
		self.deps = []
		from soclib_desc import component
		for u in self.module['uses']:
			try:
				u.do(mode, **args)
			except component.NoSuchComponent:
				raise ComponentInstanciationError(where, u.name, 'not found')
			except ComponentInstanciationError, e:
				raise ComponentInstanciationError(where, u.name, str(e))
			self.deps.append(u.builder)

	def withDeps(self):
		r = (self,)
		for d in self.deps:
			r += d.withDeps()
		return r
	def getIncl(self, incls):
		for d in self.deps:
			d.getIncl(incls)
		for i in self.module['abs_header_files']:
			p = os.path.dirname(i)
			if not p in incls:
				incls.append(p)
	def getBuilder(self, filename):
		bn = self.baseName()
		bn += '_'+os.path.splitext(os.path.basename(filename))[0]
		if self.module['abs_implementation_files']:
			from config import config
			tx = CxxSource(
					config.reposFile(bn+".cpp"),
					self.cxxSource(filename))
			incls = []
			self.getIncl(incls)
			return CxxCompile(
					config.reposFile(bn+"."+config.toolchain.obj_ext),
					tx.dests[0],
					defines = self.module['defines'],
					inc_paths = incls)
		else:
			return Noop()
	def baseName(self):
		basename = self.mode+'_%(namespace)s%(classname)s'%self.module 
		if self.module['tmpl_parameters']:
			args = self.getParams()
			params = ",".join(
					map(lambda x:'%s=%s'%(x,args[x]),
							self.module['tmpl_parameters']))
			basename += "_" + params.replace(' ', '_')
		if self.module['defines']:
			params = ",".join(
					map(lambda x:'%s=%s'%x,
							self.module['defines'].iteritems()))
			basename += "_" + params.replace(' ', '_')
		return basename
	def results(self):
		builders = map(self.getBuilder, self.module['abs_implementation_files'])
		return reduce(lambda x,y:x+y, map(lambda x:x.dests, builders), [])
	def getParams(self):
		params = {}
		for k in self.module['tmpl_parameters']:
			v = None
			if k in self.args:
				v = self.args[k]
			elif k in self.module['default_parameters']:
				v = self.module['default_parameters'][k]
				from config import config
				if config.verbose:
					print "Using default param %s=%s for component %s used at %s"%(
							k, v, self.module['classname'], self.where)
			if v is None:
				raise UndefinedParam(self.where, self.module['classname'], k)
			params[k] = v
		return params
	def cxxSource(self, s):
		source = ""
		source += '#include "%s"\n'%s
		for h in self.module['force_header_files']:
			source += '#include "%s"\n'%h
		inst = 'class %(namespace)s%(classname)s'%self.module
		if self.module['tmpl_parameters']:
			if self.module['tmpl_instanciation']:
				fmt = self.module['tmpl_instanciation']
			else:
				fmt = ','.join(
				map(lambda x:'%%(%s)s'%x,
						self.module['tmpl_parameters']))
			params = fmt % self.getParams()
			inst = inst+'<'+params+' >'
		if '<' in inst:
			source += 'template '+inst+';\n'
		return source
