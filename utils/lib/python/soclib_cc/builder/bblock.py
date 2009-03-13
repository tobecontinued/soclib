
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

import os, os.path
import time
from soclib_cc.config import config

__id__ = "$Id$"
__version__ = "$Revision$"

_global_bblock_registry = {}

class BBlock:
	def __init__(self, filename, generator = None):
		self.is_blob = False
		if generator is None:
			from action import Noop
			generator = Noop()
		self.filename = filename
		assert( filename not in _global_bblock_registry )
		_global_bblock_registry[filename] = self
		self.generator = generator
		self.rehash()
	def touch(self):
		self.rehash()
	def setIsBlob(self, val = True):
		self.is_blob = val
	def rehash(self):
		if self.filename is '':
			self.last_mod = os.stat('.').st_mtime
		else:
			if os.path.exists(self.filename):
				self.last_mod = os.stat(self.filename).st_mtime
			else:
				self.last_mod = -1
		self.__exists = self.filename is '' or os.path.exists(self.filename)
	def delete(self):
		try:
			os.unlink(self.filename)
		except OSError:
			pass
		self.rehash()
	def generate(self):
		self.generator.process()
	def __str__(self):
		return self.filename
	def exists(self):
		return self.__exists
	def clean(self):
		if os.path.isfile(self.filename):
			if not config.quiet:
				print 'Deleting', self.filename
			self.delete()
	def __hash__(self):
		return hash(self.filename)

def bblockize1(f, gen = None):
	if config.debug:
		print 'BBlockizing', f,
	if isinstance(f, BBlock):
		if config.debug:
			print 'already is'
		return f
	if f in _global_bblock_registry:
		if config.debug:
			print 'in global reg'
		return _global_bblock_registry[f]
	if config.debug:
		print 'needs BBlock'
	return BBlock(f, gen)

def bblockize(files, gen = None):
	return map(lambda x:bblockize1(x, gen), files)
