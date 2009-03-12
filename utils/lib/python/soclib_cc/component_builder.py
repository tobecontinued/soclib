
import os, os.path
from soclib_cc.builder.action import *
from soclib_cc.builder.cxx import *
from soclib_cc.builder.bblock import *
from soclib_cc.builder.textfile import *

__id__ = "$Id$"
__version__ = "$Revision$"

try:
	set
except:
	from sets import Set as set

class UndefinedParam(Exception):
	def __init__(self, where, comp, param):
		Exception.__init__(self)
		self.where = ':'.join(where)
		self.comp = comp
		self.param = param
	def __str__(self):
		return '\n%s:error: parameter %s not defined for component %s'%(
			self.where, self.param, self.comp)

class ComponentInstanciationError(Exception):
	def __init__(self, where, comp, err):
		Exception.__init__(self)
		self.where = ':'.join(where)
		self.comp = comp
		self.err = err
	def __str__(self):
		return '\n%s:error: component %s: %s'%(
			self.where, self.comp, self.err)

class TooSimple(Exception):
	pass

class ComponentBuilder:
	def __init__(self, spec, where, local = False):
		self.force_debug = spec.isDebug()
		self.force_mode = self.force_debug and "debug" or None
		self.specialization = spec
#		print self.specialization
		self.where = where
		self.deps = []
		self.local = self.specialization.isLocal()
		from soclib_desc import component, module
		self.deepdeps = set()#(self,))
		try:
			self.deps = self.specialization.getSubTree()
		except RuntimeError, e:
			raise ComponentInstanciationError(where, spec, e)
		except ComponentInstanciationError, e:
			raise ComponentInstanciationError(where, spec, str(e))

		self.headers = set()
		for d in self.specialization.getSubTree():
			headers = d.getHeaderFiles()
			self.headers |= set(headers)

		self.tmpl_headers = set()
		for d in self.specialization.getTmplSubTree():
			headers = d.getHeaderFiles()
			self.tmpl_headers |= set(headers)

## 		from pprint import pprint
## 		if self.specialization.getModuleName() == 'caba:vci_locks':
## 			print self
## 			pprint(sorted(map(str, self.specialization.getSubTree())))
## 			pprint(sorted(list(self.headers)))
## 			for m in self.specialization.getSubTree():
## 				print str(m)
## 				pprint(m.__dict__)
## 				pprint(m.getHeaderFiles())
	def getBuilder(self, filename, *add_filenames):
		bn = self.baseName()
		if add_filenames:
			bn += '_all'
		else:
			bn += '_'+os.path.splitext(os.path.basename(filename))[0]
		from config import config
		source = self.cxxSource(filename, *add_filenames)
		if source:
			tx = CxxSource(
				config.reposFile(bn+".cpp", self.force_mode),
				source)
			src = tx.dests[0]
		else:
			src = filename
		incls = set(map(os.path.dirname, self.headers))
		if self.local:
			try:
				t = config.type
			except:
				t = 'unknown'
			out = os.path.join(
				os.path.dirname(filename),
				t+'_'+bn+"."+config.toolchain.obj_ext)
		else:
			out = config.reposFile(bn+"."+config.toolchain.obj_ext, self.force_mode)
		return CxxCompile(
			dest = out,
			src = src,
			defines = self.specialization.getDefines(),
			inc_paths = incls,
			force_debug = self.force_debug)
	def baseName(self):
		basename = self.specialization.getModuleName()
		tp = self.specialization.getType()
		basename += "_" + tp.replace(' ', '_')
		params = ",".join(
			map(lambda x:'%s=%s'%x,
				self.specialization.getDefines().iteritems()))
		if params:
			basename += "_" + params
		return basename.replace(' ', '_')
	def results(self):
		is_template = ('<' in self.specialization.getType())
		impl = self.specialization.getImplementationFiles()
		if is_template and len(impl) > 1:
			builders = [self.getBuilder(*impl)]
		else:
			builders = map(self.getBuilder, impl)
		objects = bblockize(self.specialization.getObjectFiles())
		map(lambda x:x.setIsBlob(True), objects)
		return reduce(lambda x,y:x+y, map(lambda x:x.dests, builders), objects)
	def cxxSource(self, *sources):
		if not '<' in self.specialization.getType():
			if len(sources) > 1:
				source = ''
				for s in sources:
					source += '#include "%s"\n'%s
				return source
			else:
				return ''

		inst = 'class '+self.specialization.getType()

		source = ""
		for h in map(os.path.basename, self.tmpl_headers):
			source += '#include "%s"\n'%h
		from config import config
		for h in config.toolchain.always_include:
			source += '#include <%s>\n'%h
		for s in sources:
			source += '#include "%s"\n'%s
		source += 'template '+inst+';\n'
		return source

	@classmethod
	def fromSpecialization(cls, spec):
		return cls(spec, None, spec.isLocal())

	def __hash__(self):
		return hash(self.specialization)

	def __str__(self):
		return '<builder %s>'%self.specialization

	def __repr__(self):
		return str(self)
