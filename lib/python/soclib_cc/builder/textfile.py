
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

import os, os.path
from fileops import CreateDir
from action import Action

class Textfile(Action):
	def __init__(self, output, contents):
		Action.__init__(self, [output], [], contents = contents)
		self.pargen = CreateDir(os.path.dirname(str(self.dests[0])))
	def processDeps(self):
		return [self.pargen.dests[0]]
	def process(self):
		if self.dests[0].exists:
			fd = open(str(self.dests[0]), "r")
			buf = fd.read()
			fd.close()
			if buf == self.options['contents']:
				return
		self.runningCommand('gen', self.dests, '')
		self.pargen.process()
		fd = open(str(self.dests[0]), "w")
		fd.write(self.options['contents'])
		fd.close()
		self.dests[0].touch()

class CxxSource(Textfile):
	pass
