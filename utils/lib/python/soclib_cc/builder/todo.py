
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
import os, os.path
from soclib_cc.config import config
from bblock import bblockize
from action import Noop, ActionFailed, Action

def cr(x):
	return not isinstance(x, Noop)

class ToDo:
	def __init__(self, *dests):
		self.dests = []
		self.add(*dests)
		self.prepared = False
		d = config.reposFile('')
		if not os.path.isdir(d):
			os.makedirs(d)
		self.actions = 0
		self.max_actions = config.toolchain.max_processes
	def add(self, *dests):
		self.dests += bblockize(dests)
	def _getall(self, dests):
		bbs = set(dests)
		todo = set()
		while bbs:
			bb = bbs.pop()
			gen = bb.generator
			bbs |= set(gen.getDepends())
			todo.add(gen)
		todo = filter(cr, todo)
		todo.sort()
		return todo
	def prepare(self):
		if self.prepared:
			return
		self.todo = []
		self.todo = self._getall(self.dests)
		self.prepared = True
	def clean(self):
		self.prepare()
		for i in xrange(len(self.todo)-1,-1,-1):
			todo = self.todo[i]
			todo.clean()
	def wait(self):
		Action.wait()
		self.actions -= 1
		self.progressBar()
	def process(self):
		import sys
		self.prepare()
		l = len(self.todo)
		for pi in self.todo:
			pi.todoRehash()
		left = self.todo[:]
		left.reverse()
		while left:
			todo = left.pop()
			while not todo.canBeProcessed():
				self.wait()
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
			self.progressBar()
			if todo.isBackground():
				self.actions += 1
				if self.actions >= self.max_actions:
					self.wait()
			todo.todoRehash(True)
		while self.actions:
			self.wait()
		self.progressBar()
	def progressBar(self):
		if not config.progress_bar:
			return
		pb = ""
		for pi in self.todo:
			pb += pi.todoInfo()
		sys.stdout.write('\r['+pb+']')
		sys.stdout.write(' %d left '%(pb.count(' ')))
		sys.stdout.flush()
