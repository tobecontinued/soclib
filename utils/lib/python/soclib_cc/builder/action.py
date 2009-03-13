
# SOCLIB_GPL_HEADER_BEGIN
# 
# This file is part of SoCLib, GNU GPLv2.
# 
# SoCLib is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# SoCLib is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with SoCLib; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
# 
# SOCLIB_GPL_HEADER_END
# 
# Copyright (c) UPMC, Lip6, SoC
#         Nicolas Pouillon <nipo@ssji.net>, 2007
# 
# Maintainers: group:toolmakers

from soclib_cc.config import config
import depends
import os, os.path, time
import sys

__id__ = "$Id$"
__version__ = "$Revision$"

class NotFound(Exception):
	pass

class ActionFailed(Exception):
	def __init__(self, rval, action):
		Exception.__init__(self, "%s failed: %s"%(action, rval))
		self.rval = rval
		self.action = action

def get_times(files, default, cmp_func, ignore_absent):
	most_time = default
	for f in files:
		if f.exists():
			most_time = cmp_func((f.last_mod, most_time))
		else:
			if ignore_absent:
				continue
			else:
				raise NotFound, f
	return most_time

get_newest = lambda files, ignore_absent: get_times(files, 0, max, ignore_absent)
get_oldest = lambda files, ignore_absent: get_times(files, time.time(), min, ignore_absent)

def check_exist(files):
	return reduce(lambda x,y:x and y, map(lambda x:x.exists(), files), True)

class Action:
	priority = 0
	__handles = {}
	def __init__(self, dests, sources, **options):
		from bblock import bblockize
		self.dests = bblockize(dests, self)
		self.sources = bblockize(sources)
		self.options = options
		self.done = False
		self.hash = reduce(lambda x,y:(x ^ (y>>1)), map(hash, self.dests+self.sources), 0)
		self.has_deps = False
		self.__pid = 0

	def launch(self, cmd):
		import popen2
		self.__command = cmd
		self.__handle = popen2.Popen3(cmd, True, 8192)
		self.__handle.tochild.close()
		self.__child_out = self.__handle.fromchild
		self.__child_err = self.__handle.childerr
		self.__pid = self.__handle.pid
		self.__class__.__handles[self.__handle.pid] = self
	@classmethod
	def wait(cls):
#		try:
		pid, rval = os.wait()
		return int(cls.__done(pid, rval))
#		except OSError:
#			ks = cls.__handles.keys()
#			c = 0
#			for pid in ks:
#				if cls.__done(pid):
#					c += 1
#			return c
	@classmethod
	def __done(cls, pid, rval):
		try:
			self = cls.__handles[pid]
		except KeyError:
			return False
		del self.__handles[pid]
		out = self.__child_out.read()
		err = self.__child_err.read()
		self.__child_out.close()
		self.__child_err.close()
		if out:
			sys.stdout.write('\n')
			sys.stdout.write(out)
		if err:
			sys.stderr.write('\n')
			sys.stderr.write(err)
		if rval:
			raise ActionFailed(rval, self.__command)
		self.__pid = 0
		del self.__handle
		del self.__child_out
		del self.__child_err
		del self.__command
		for d in self.dests:
			d.touch()
		return True
	def isBackground(self):
		return self.__pid

	def todoRehash(self, done = False):
		if done:
			self.done = True
		self.must_be_processed = self.mustBeProcessed()
	def todoInfo(self):
		r = self.__pid and 'W' or (
			self.done and '=' or (
			self.must_be_processed and " " or "+"))
		return r


	def processDeps(self):
		return []
	def process(self):
		self.todoRehash(True)
	def getDepends(self):
		if not self.has_deps:
			depname = self.__class__.__name__+'_%08u.deps'%hash(self.dests[0].filename)
			try:
				self.__depends = depends.load(depname)
			except:
				self.__depends = self._depList()
				depends.dump(depname, self.__depends)
			self.has_deps = True
		return self.__depends
	def mustBeProcessed(self):
		deps = self.getDepends()
		if not check_exist(deps):
			return True
		newest_src = get_newest(deps, ignore_absent = False)
		oldest_dest = get_oldest(self.dests, ignore_absent = True)
		r = (newest_src > oldest_dest) or not check_exist(self.dests)
		if r:
			map(lambda x:x.delete(), self.dests)
		return r
	def _depList(self):
		return self.sources+self.processDeps()
	def canBeProcessed(self):
		return check_exist(self.sources)
	def runningCommand(self, what, outs, cmd):
		if not config.quiet:
			print self.__class__.__name__, what, ', '.join(map(str,outs))
		if config.verbose:
			print cmd
	def clean(self):
		for i in self.dests:
			i.clean()
	def __hash__(self):
		return self.hash
	def __cmp__(self, other):
		return cmp(self.priority, other.priority)

	def genMakefileCleanCommands(self):
		return '\t$(CMDPFX)-rm -f %s\n'%(' '.join(map(lambda x:'"%s"'%x, self.dests)))

	def genMakefileTarget(self):
		def ex(s):
			return str(s).replace(' ', '\\ ').replace(':', '$(ECOLON)')
		def ex2(s):
			return str(s).replace(' ', '\\ ').replace(':', '$$(COLON)')
		r = (' '.join(map(ex, self.dests)))+\
			   ' : '+\
			   (' '.join(map(ex2, self.sources)))+\
			   '\n'
		r += '\t@echo "%s $@"\n'%self.__class__.__name__
		for i in self.commands_to_run():
			r += '\t$(CMDPFX)%s\n'%i
		return r+'\n'

	def commands_to_run(self):
		return ('# Undefined command in '+self.__class__.__name__),

	def __str__(self):
		l = lambda a:' '.join(map(lambda x:os.path.basename(x.filename), a))
		return "<%s:\n sources: %s\n dests: %s>"%(self.__class__.__name__,
												  l(self.sources),
												  l(self.dests))

class Noop(Action):
	def __init__(self):
		Action.__init__(self, [], [])
	def getDepends(self):
		return []
