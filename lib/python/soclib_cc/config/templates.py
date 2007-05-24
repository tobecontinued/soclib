
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
import sys
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
	if sys.platform in ['cygwin']:
		libs = ['-lbfd', '-liberty', '-lintl']
	elif sys.platform in ['darwin']:
		libs = ['-lbfd', '-liberty', '-lintl']
	else:
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
	include_paths = ['include']
	common_include_paths = ['systemc/include']

	def __init__(self, soclib_path, desc_paths):
		self.path = soclib_path
		self.common_cflags = map(
			lambda x:'-I'+os.path.join(soclib_path, x),
			self.common_include_paths)
		self.cflags = map(
			lambda x:'-I'+os.path.join(soclib_path, x),
			self.include_paths)+self.common_cflags
		self.desc_paths = []
		for dp in desc_paths:
			self.addDescPath(dp)
	def addDescPath(self, path):
		np = os.path.abspath(os.path.join(self.path, path))
		if not np in self.desc_paths:
			self.desc_paths.append(np)
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
		stupid_platform = sys.platform in ['cygwin']
		if stupid_platform:
			if len(name)>128:
				name, ext = os.path.splitext(name)
				name = 'long_name_'+hex(hash(name))+ext
		r = os.path.join(self.repos, self.mode, name)
		if stupid_platform:
			r = r.replace(':','_')
		return r

