
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
from utils import *

__all__ = ['systemc', 'toolchain', 'build_env']

class systemc(Configurator):
	vendor = 'OSCI'
	libdir = '%(dir)s/lib-%(os)s'
	includedir = '%(dir)s/include'
	addinc = []
	libs = ['-L%(libdir)s', '-lsystemc']
	cflags = ['-I%(dir)s/include']

class toolchain(Configurator):
	tool_map = {
		'CC':'gcc',
		'CXX':'g++',
		'CC_LINKER':'gcc',
		'CXX_LINKER':'g++',
		'LD':'ld'
		}
	obj_ext = 'o'
	lib_ext = 'a'
	
	prefix = ''
	cflags = ['-Wall', '-Wno-pmf-conversions']
	libs = ['-lbfd']
	release_cflags = ['-O2']
	release_libs = []
	prof_cflags = ['-pg']
	prof_libs = ['-pg']
	debug_cflags = ['-ggdb']
	debug_libs = []

class build_env(Configurator):
	# toolchain = 
	# systemc =
	verbose = False
	quiet = False
	debug = False
	mode = 'release'
	repos = 'repos'

	def __init__(self, soclib_path):
		self.path = soclib_path
		self.cflags = [
			'-I'+os.path.join(soclib_path, 'include'),
			'-I'+os.path.join(soclib_path, 'systemc/include')
			]
	def getTool(self, name):
		if name in self.toolchain.tool_map:
			name = self.toolchain.tool_map[name]
		return self.toolchain.prefix+name
	def getCflags(self):
		toolchain_cflags = getattr(self.toolchain, self.mode+'_cflags')
		toolchain_cflags = toolchain_cflags + self.toolchain.cflags
		return toolchain_cflags+self.cflags+self.systemc.cflags
	def getLibs(self):
		toolchain_libs = getattr(self.toolchain, self.mode+'_libs')
		toolchain_libs = toolchain_libs + self.toolchain.libs
		return self.systemc.libs+toolchain_libs
	def reposFile(self, name):
		return os.path.join(self.repos, self.mode, name)

