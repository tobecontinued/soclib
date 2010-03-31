
import os
import os.path
import time
import re
import warnings
import cPickle as pickle

class CreatorProxy:
	"""
	A proxy class which will get the calls to Module, PortDecl,
	... when parsing a .sd file. This proxy will notify the
	callback the declaration happened.

	This is mostly a decorator, it could be beautifuled to handle
	docstrings et al correctly, but as it should not be inspected
	interactively, this should not be an issue.
	"""
	def __init__(self, callback, klas):
		"""
		callback will be notified of __call__s, calls will be
		proxied to klas.
		"""
		self.__callback = callback
		self.__klas = klas
	def __call__(self, *args, **kwargs):
		"""
		Do the hard work.
		"""
		r = self.__klas(*args, **kwargs)
		self.__callback(r)
		return r

class CachedDescFile:
	"""
	A cache for a .sd. This may then add/remove declarations from the
	cache.
	"""
	def __init__(self, path):
		self.__path = path
		self.__date_loaded = 0
		self.__modules = []

	def __file_time(self):
		try:
			return os.path.getmtime(self.__path)
		except OSError:
			# If file does not exists, make its time somewhere in the future
			return time.time() + 1024
	
	def isOutdated(self):
		"""
		Tells whether a file is newer than its cache.
		"""
		return self.__date_loaded < self.__file_time()

	def exists(self):
		"""
		Tells whether a file still exists in FS.
		"""
		return os.path.isfile(self.__path)

	def pathIs(self, path):
		return path == self.__path

	def cleanup(self):
		for m in self.__modules:
			m.cleanup()

	def rehashIfNecessary(self):
		"""
		This may rehash the cached file, if necessary (modification).
		"""
		if self.isOutdated():
			self.rehash()

	def __mkParserGlobals(self):
		"""
		Creates the globals dictionary for execfile()ing a .sd. This
		creates all the necessary CreatorProxies.
		"""
		from soclib_desc import component
		glbl = {}

		for n in component.__all__:
			widget = getattr(component, n)
			try:
				wrap = issubclass(widget, component.Module)
			except:
				wrap = False
			if wrap:
				widget = CreatorProxy(self.__modules.append, widget)
			glbl[n] = widget
		glbl['Uses'] = component.Uses
		glbl['__name__'] = self.__path
		return glbl

	def doForModules(self, callback):
		"""
		Calls the given callback for each module
		"""
		for m in self.__modules:
			callback(m)

	def rehash(self):
		"""
		Unconditionnaly rehashes a description. If file disappeared,
		this call unregisters contained descriptions.
		"""
		self.__modules = []
		glbl = self.__mkParserGlobals()
		locl = {}
		try:
			execfile(self.__path, glbl, locl)
		except Exception, e:
			import description_files
			raise description_files.FileParsingError(
				'in %s: %r'%(self.__path, e))
		self.__date_loaded = self.__file_time()

class GlobalDescCache:
	_sdfile = re.compile('^[^.][a-zA-Z0-9_-]+\\.sd$')
	"""
	A cache for all definitions. They are indexed by .sd path.
	"""
	def __init__(self, ignore_regexp = None):
		self.__ignore_regexp = None
		if ignore_regexp:
			self.__ignore_regexp = re.compile(ignore_regexp)
		self.__registry = {}
		self.__once_seen = set()

	def visitSubtree(self, path):
		"""
		Recurse in a subtree looking for .sd files, or assume it is cached
		"""
		if path not in self.__once_seen:
		    self.parseSubtree(path)
		self.__once_seen.add(path)

	def parseSubtree(self, path):
		"""
		Inconditionally recurse in a subtree looking for .sd files.
		"""
		path = os.path.abspath(path)
		self.__once_seen.add(path)
		for root, dirs, files in os.walk(path):
			if ".svn" in dirs:
				dirs.remove(".svn")
			files = filter(self._sdfile.match, files)
			if self.__ignore_regexp:
				files = filter(lambda x:not self.__ignore_regexp.match(x), files)
			for f in files:
				self.checkFile(os.path.join(root,f))

	def checkFile(self, path):
		"""
		Rehashes a file, add it to cache if non-existent
		"""
		if not path in self.__registry:
			self.__registry[path] = CachedDescFile(path)
		self.__registry[path].rehashIfNecessary()

	def checkFiles(self):
		"""
		Rehashes all the known files if necessary.
		Remove deleted files.
		"""
		todel = []
		for key, c in self.__registry.iteritems():
			if c.exists():
				c.rehashIfNecessary()
			else:
				todel.append(key)
		for d in todel:
			del self.__registry[d]

	def getCachedDescFilesIn(self, path):
		"""
		Returns cached descriptions which files are under path
		"""
		path = os.path.abspath(path)
		if not path.endswith('/'):
			path += '/'
		r = []
		for p, d in self.__registry.iteritems():
			if p.startswith(path):
				r.append(d)
		return r

	def save(self, path):
		"""
		Saves the cache to path.
		"""
		fd = open(path, 'w')
		pickle.dump(self, fd, pickle.HIGHEST_PROTOCOL)

	def cleanup(self):
		self.__once_seen = set(self.__once_seen)
		for p, d in self.__registry.iteritems():
			d.cleanup()

	def load(cls, path):
		"""
		Loads the cache from path. If unable to load a valid cache,
		this call returns None.
		"""
		try:
			fd = open(path, 'r')
			r = pickle.load(fd)
		except:
			return None
		
		if isinstance(r, cls):
			r.cleanup()
			return r
		return None
	load = classmethod(load)
