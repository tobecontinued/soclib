
import os
import re
import time

class MetadataFile:
    _known_parsers = set()
    
    def init_parsers(cls, parsers):
        for mn in parsers:
            tokens = mn.split('.')
            mod = '.'.join(tokens[:-1])
            tmp = __import__(mod, globals(), {}, [tokens[-1]])
            p = getattr(tmp, tokens[-1])
            cls._known_parsers.add(p)
    init_parsers = classmethod(init_parsers)

    def handle(cls, filename):
        assert cls._known_parsers, RuntimeError("No known parser")
        for p in cls._known_parsers:
            for ext in p.extensions:
                if filename.endswith(ext):
                    return p(filename)
        raise ValueError('Unhandled file "%s"' % filename)
    handle = classmethod(handle)

    def filename_regexp(cls):
        assert cls._known_parsers, RuntimeError("No known parser")
        gre = []
        for p in cls._known_parsers:
            for ext in p.extensions:
                gre.append(re.escape(ext))
        gre = '|'.join(gre)
        return re.compile('^[^.][a-zA-Z0-9_-]+('+gre+')$')
    filename_regexp = classmethod(filename_regexp)



    def __file_time(self):
        try:
            return os.path.getmtime(self.__path)
        except OSError:
            # If file does not exists, make its time somewhere in the future
            return time.time() + 1024




    def __init__(self, path):
        self.__path = path
        self.__date_loaded = 0
        self.__modules = []

    def path(self):
        return self.__path
    path = property(path)

    def doForModules(self, callback):
        """
        Calls the given callback for each module
        """
        for m in self.__modules:
            callback(m)

    def cleanup(self):
        for m in self.__modules:
            m.cleanup()

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

    def reloaded(self):
        self.__date_loaded = self.__file_time()

    def rehashIfNecessary(self):
        """
        This may rehash the cached file, if necessary (modification).
        """
        if self.isOutdated():
            self.rehash()

    def rehash(self):
        """
        Unconditionnaly rehashes a description.
        """
        try:
            self.__modules = self.get_modules()
        except NotImplementedError, e:
            raise
#        except Exception, e:
#            import description_files
#            raise description_files.FileParsingError(
#                'in %s: %r'%(self.path, e))
        if not isinstance(self.__modules, (list, tuple)):
            raise ValueError("Parsing of %s by %s did not return a valid result"%
                             (self.path, self))
        self.reloaded()

    def get_modules(self):
        """
        Parses the metadata file and returns the modules found in it.
        """
        raise NotImplementedError()        
