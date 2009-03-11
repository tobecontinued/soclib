
import sys
import os
from datetime import datetime
import atexit
import traceback

__all__ = ['bootstrap']

class Outter:
	def __init__(self, fd, term, prefix):
		self.__fd = fd
		self.__term = term
		self.__prefix = prefix
		self.__class__.__last = self
	def write(self, s):
		if self.__class__.__last != self:
			self.__class__.__last = self
			self.__fd.write("\n==== %s ====\n"%self.__prefix)
		self.__fd.write(s)
		self.__term.write(s)
	def flush(self):
		self.__fd.flush()
		self.__term.flush()
	def isatty(self):
		return False

class ExceptHandler:
	def __init__(self):
		self.had = False
	def __call__(self, typ, value, traceback_):
		traceback.print_exception(typ, value, traceback_)
		print 
		print "SoCLib-cc failed unexpectedly."
		self.had = True

class AdvertizeForBugreporter:
	def __init__(self, prev):
		self.__prev = prev
	def __call__(self, typ, value, traceback_):
		self.__prev(typ, value, traceback_)
		print
		print "SoCLib-cc failed unexpectedly. To submit a bug report,"
		print " please re-run soclib-cc with --bug-report"
		print " (i.e run: 'soclib-cc %s --bug-report')"%' '.join(sys.argv[1:])

def end_handler(logname, exc, fd, report_action):
	from soclib_desc.module import Module
	from soclib_utils.repos_file import revision
	import os
	import os.path
	print >> fd, "Used modules:"
	for m in Module.getUsedModules():
		print >> fd, '  ', m
		for f in m.files():
			print >> fd, '    ', os.path.basename(f), revision(f)
	print >> fd
	url = 'https://www.soclib.fr/soclib-cc-bugreport'
	print "Now, to report the bug:"
	print " * open %s"%url,
	if report_action == "openbrowser":
		print '(should open automagically)'
		try:
			import webbrowser
			webbrowser.open(url)
		except:
			pass
	else:
		print
	print " * submit a bug report attaching '%s'"%logname

def bootstrap(create_log, report_action):
	if create_log:
#		filename = datetime.now().strftime("soclib-cc-%Y%m%d-%H%M%S.log")
		filename = "soclib-cc-debug.log"
		fd = open(filename, "w")
		sys.stdout = Outter(fd, sys.stdout, "stdout")
		sys.stderr = Outter(fd, sys.stderr, "stderr")
		print "Soclib-cc is in bug-report mode. Every output will go to '%s'"%filename

		sys.excepthook = ExceptHandler()
		atexit.register(end_handler, filename, sys.excepthook, fd, report_action)

		print >> fd, "Command: %s"%sys.argv
		print >> fd, "Environment:"
		for k in sorted(os.environ.iterkeys()):
			print >> fd, "%s: %r"%(k, os.environ[k])
		print >> fd
	else:
		sys.excepthook = AdvertizeForBugreporter(sys.excepthook)

