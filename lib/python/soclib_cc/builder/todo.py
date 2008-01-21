
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
# Maintainers: nipo

import sys
from soclib_cc.config import config
from bblock import bblockize
from action import Noop, ActionFailed

class ToDo:
	def __init__(self, *dests):
		self.dests = []
		self.add(*dests)
		self.prepared = False
	def add(self, *dests):
		self.dests += bblockize(dests)
	def _prepare_one(self, d):
		d = d.generator
		if isinstance(d, Noop) or d in self.todo:
			return
		insind = 0
		for prereq in d.getDepends():
			try:
				ni = self.todo.index(prereq)
				insind = min((ni, insind))
			except ValueError:
				pass
			self._prepare_one(prereq)
		self.todo.insert(insind, d)
	def prepare(self):
		if self.prepared:
			return
		self.todo = []
		for d in self.dests:
			self._prepare_one(d)
		if config.debug:
			print "ToDo:", self.todo
		self.prepared = True
	def clean(self):
		self.prepare()
		for i in xrange(len(self.todo)-1,-1,-1):
			todo = self.todo[i]
			todo.clean()
	def process(self):
		import sys
		self.prepare()
		l = len(self.todo)
		for i in xrange(l-1,-1,-1):
			todo = self.todo[i]
			if config.progress_bar:
				sys.stdout.write('\r'+'+'*(l-i)+'-'*i)
				sys.stdout.flush()
			if todo.mustBeProcessed():
				try:
					todo.process()
				except ActionFailed, e:
					print "soclib-cc: *** Action failed with return value `%s'. Stop."%e.rval
					if config.verbose:
						print "soclib-cc: Failed action: `%s'"%e.action
					sys.exit(1)
			elif config.verbose:
				print 'No need to redo', todo
