
import os, os.path
from soclib_cc.builder.action import *
from soclib_cc.builder.cxx import *
from soclib_cc.builder.bblock import *
from soclib_cc.builder.textfile import *

try:
	set
except:
	from sets import Set as set

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

class TooSimple(Exception):
	pass

class ComponentBuilder:
	def __init__(self, spec, where, local = False):
		self.specialization = spec
#		print self.specialization
		self.where = where
		self.deps = []
		self.local = self.specialization.descAttr('local')
		from soclib_desc import component, module
		self.deepdeps = set()#(self,))
		try:
#			print self
			self.deps = self.specialization.getAllBuilders()
#			self.deps.remove(self)
			for b in self.deps:
				self.deepdeps |= b.withDeps()
#		except module.NoSuchComponent:
#			raise ComponentInstanciationError(where, u.name, 'not found')
		except RuntimeError, e:
			raise ComponentInstanciationError(where, spec, e)
		except ComponentInstanciationError, e:
			raise ComponentInstanciationError(where, spec, str(e))
		self.headers = set()
		for d in self.withDeps():
			d.getHeaders(self.headers)
	def getHeaders(self, incls):
		for i in self.specialization.descAttr('abs_header_files'):
			incls.add(i)
	def withDeps(self):
		return self.deepdeps | set([self])
	def getParamHeaders(self):
		h = set()
		pb = self.specialization.getParamBuilders()
		for builder in pb:
			for b in builder.withDeps():
				b.getHeaders(h)
#		print 'ParamHeaders for', self, pb, h
		return h
	def getBuilder(self, filename):
		bn = self.baseName()
		bn += '_'+os.path.splitext(os.path.basename(filename))[0]
		from config import config
		source = self.cxxSource(filename)
		if source:
			tx = CxxSource(
				config.reposFile(bn+".cpp"),
				source)
			src = tx.dests[0]
		else:
			src = filename
		incls = set(map(os.path.dirname, self.headers))
		if self.local:
			out = os.path.join(
				os.path.dirname(filename),
				config.type+bn+"."+config.toolchain.obj_ext)
		else:
			out = config.reposFile(bn+"."+config.toolchain.obj_ext)
		return CxxCompile(
			dest = out,
			src = src,
			defines = self.specialization.descAttr('defines'),
			inc_paths = incls)
	def baseName(self):
		basename = self.specialization.descAttr('classname')
		tp = self.specialization.getTmplParams()
		if tp:
			basename += "_" + tp.replace(' ', '_')
		params = ",".join(
			map(lambda x:'%s=%s'%x,
				self.specialization.descAttr('defines').iteritems()))
		if params:
			basename += "_" + params.replace(' ', '_')
		return basename
	def results(self):
		builders = map(self.getBuilder, self.specialization.descAttr('abs_implementation_files'))
		return reduce(lambda x,y:x+y, map(lambda x:x.dests, builders), [])
	def cxxSource(self, s):
		source = ""
		for h in map(os.path.basename, self.getParamHeaders()):
			source += '#include "%s"\n'%h
		from config import config
		for h in config.toolchain.always_include:
			source += '#include <%s>\n'%h
		source += '#include "%s"\n'%s
		inst = 'class '+self.specialization.getType()
		if not '<' in inst:
			return ''
		else:
			source += 'template '+inst+';\n'
		return source

	def __hash__(self):
		return hash(self.specialization)

	def __str__(self):
		return '<builder %s>'%self.specialization

	def __repr__(self):
		return str(self)
