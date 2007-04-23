
# This file is part of SoCLIB.
# 
# SoCLIB is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# SoCLIB is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with SoCLIB; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
# 
# Copyright (c) UPMC, Lip6, SoC
#         Nicolas Pouillon <nipo@ssji.net>, 2007
# 
# Maintainers: nipo

from soclib_cc.config import config
import depends
import os, os.path, time

class NotFound(Exception):
	pass

class ActionFailed(Exception):
	def __init__(self, rval, action):
		self.rval = rval
		self.action = action

def get_times(files, default, cmp_func, ignore_absent):
	most_time = default
	for f in files:
		if f.exists:
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
	return reduce(lambda x,y:x and y, map(lambda x:x.exists, files), True)

class Action:
	def __init__(self, dests, sources, **options):
		from bblock import bblockize
		self.dests = bblockize(dests, self)
		self.sources = bblockize(sources)
		self.options = options
	def processDeps(self):
		return []
	def process(self):
		pass
	def getDepends(self):
		try:
			return self.__depends
		except:
			pass
		depname = self.__class__.__name__+'_%08u.deps'%hash(self.dests[0].filename)
		try:
			self.__depends = depends.load(depname)
		except Exception, e:
			self.__depends = self._depList()
		depends.dump(depname, self.__depends)
		return self.__depends
	def mustBeProcessed(self):
		deps = self.getDepends()
		if config.debug:
			print 'mustBeProcessed(%s):'%self, map(str,deps)
		newest_src = get_newest(deps, ignore_absent = False)
		oldest_dest = get_oldest(self.dests, ignore_absent = True)
		return (newest_src > oldest_dest) or not check_exist(self.dests)
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

class Noop(Action):
	def __init__(self):
		Action.__init__(self, [], [])
