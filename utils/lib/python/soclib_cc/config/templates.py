
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

__id__ = "$Id$"
__version__ = "$Revision$"

config.systemc = Config(
	vendor = 'OSCI',
	libdir = '%(dir)s/lib-%(os)s',
	includedir = '%(dir)s/include',
	addinc = [],
	libs = ['-L%(libdir)s', '-lsystemc'],
	cflags = ['-I%(dir)s/include'],
	)

import sys
if sys.platform in ['cygwin']:
	libs = ['-lbfd', '-liberty', '-lintl']
elif sys.platform in ['darwin']:
	libs = ['-lbfd', '-liberty', '-lintl']
else:
	libs = ['-lbfd']

config.toolchain = Config(
	tool_map = {
		'CC':'gcc',
		'CXX':'g++',
		'CC_LINKER':'gcc',
		'CXX_LINKER':'g++',
		'LD':'ld'
		},
	obj_ext = 'o',
	lib_ext = 'a',
	
	prefix = '',
	cflags = ['-Wall', '-Wno-pmf-conversions', '-DSOCLIB'],
	libs = libs,
	always_include = ['sys/types.h', 'inttypes.h'],	
	release_cflags = ['-O2', '-DSOCLIB_MODE_RELEASE'],
	release_libs = [],
	prof_cflags = ['-pg', '-DSOCLIB_MODE_PROFILE'],
	prof_libs = ['-pg'],
	debug_cflags = ['-ggdb', '-DSOCLIB_MODE_DEBUG'],
	debug_libs = [],
	max_processes = 1,
	)


def addDescPath(self, path):
	import os
	np = os.path.abspath(os.path.join(self.path, path))
	if not np in self.desc_paths:
		self.desc_paths.append(np)
def getTool(self, name):
	if name in self.toolchain.tool_map:
		name = self.toolchain.tool_map[name]
		if isinstance(name, str):
			tool = name.split(' ')
			return tool
		else:
			return [tool]
	return [self.toolchain.prefix+name]
def getCflags(self, mode=None):
	if mode is None:
		mode = self.mode
	toolchain_cflags = getattr(self.toolchain, mode+'_cflags')
	toolchain_cflags = toolchain_cflags + self.toolchain.cflags
	return toolchain_cflags+self.cflags+self.systemc.cflags
def getLibs(self, mode=None):
	if mode is None:
		mode = self.mode
	toolchain_libs = getattr(self.toolchain, mode+'_libs')
	toolchain_libs = toolchain_libs + self.toolchain.libs
	return self.systemc.libs+toolchain_libs
def reposFile(self, name, mode=None):
	if mode is None:
		mode = self.mode
	import os, sys
	stupid_platform = sys.platform in ['cygwin']
	if stupid_platform:
		if len(name)>128:
			name, ext = os.path.splitext(name)
			name = 'long_name_'+hex(hash(name))+ext
	r = os.path.join(self.repos, mode, name)
	if stupid_platform:
		r = r.replace(':','_')
	return r

def doConfigure(self, soclib_path, desc_paths):
	import os
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
	self.desc_paths.append(".")

	import soclib_utils.repos_file
	self.svn_revision = soclib_utils.repos_file.revision(soclib_utils.repos_file.__file__)
## 	print self.desc_paths
## 	import sys
## 	sys.exit(0)
	
config.build_env = Config(
	# toolchain = 
	# systemc =
	verbose = False,
	quiet = False,
	debug = False,
	progress_bar = False,
	mode = 'release',
	repos = 'repos',
	common_include_paths = [],
	include_paths = ["soclib/lib/include"],
	sd_ignore_regexp = None,
	cache_file = "%(repos)s/soclib_cc.cache",
	addDescPath = addDescPath,
	getTool = getTool,
	getCflags = getCflags,
	getLibs = getLibs,
	reposFile = reposFile,
	doConfigure = doConfigure,
	)
