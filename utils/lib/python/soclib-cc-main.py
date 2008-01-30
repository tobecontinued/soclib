#!/usr/bin/env python

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

import os, os.path
import sys
import soclib_cc
from soclib_cc.config import config, change_config
from soclib_cc.builder.cxx import CxxCompile

from optparse import OptionParser

def main():
	parser = OptionParser(usage="%prog [ -m mode ] [ -t config ] [ -vqd ] [ -c -o output input | -p pf_desc ]")
	parser.add_option('-v', '--verbose', dest = 'verbose',
					  action='store_true',
					  help="Chat a lot")
	parser.add_option('-d', '--debug', dest = 'debug',
					  action='store_true',
					  help="Print too much things")
	parser.add_option('-P', '--progress_bar', dest = 'progress_bar',
					  action='store_true',
					  help="Print evolution with a progress bar")
	parser.add_option('-q', '--quiet', dest = 'quiet',
					  action='store_true',
					  help="Print nothing but errors")
	parser.add_option('-c', '--compile', dest = 'compile',
					  action='store_true',
					  help="Do a simple compilation, not a linkage")
	parser.add_option('-l', '--list-descs', dest = 'list_descs',
					  action='store_true',
					  help="List known descriptions")
	parser.add_option('-x', '--clean', dest = 'clean',
					  action='store_true',
					  help="Clean all outputs, only compatible with -p")
	parser.add_option('-o', '--output', dest = 'output',
					  action='store', type = 'string',
					  help="Select output file")
	parser.add_option('-m', '--mode', dest = 'mode',
					  action='store', type = 'string',
					  default = 'release',
					  help="Select mode: *release|debug|prof")
	parser.add_option('-p', '--platform', dest = 'platform',
					  action='store', type = 'string',
					  help="Use a platform description")
	parser.add_option('--getpath', dest = 'getpath',
					  action='store_true',
					  help="Print soclib path")
	parser.add_option('--getflags', dest = 'getflags',
					  action='store', type = 'string',
					  help="Print soclib path")
	parser.add_option('-t', '--type', dest = 'type',
					  action='store', type = 'string',
					  help="Use a different configuration: *default")
	opts, args = parser.parse_args()
	if opts.type:
		change_config(opts.type)
	config.mode = opts.mode
	config.verbose = opts.verbose
	config.debug = opts.debug
	config.quiet = opts.quiet
	if opts.progress_bar:
		config.progress_bar = True
		config.quiet = True

	config.output = "system.x"
	if opts.getpath:
		print config.path
		return 0
	if opts.list_descs:
		from soclib_cc.components import component_defs
		def l(name, cd):
			for impl, desc in cd.iteritems():
				print name+"("+impl+"): ", '<'
				for l in desc.header_files, desc.force_header_files, desc.implementation_files:
					for s in l:
						if os.path.isfile(s):
							print " +",
						else:
							print " -",
						print s
				print ' >'
		for k, v in component_defs.iteritems():
			l(k, v)
		return 0
	if opts.getflags:
		if opts.getflags == 'cflags':
			print ' '.join(config.common_cflags)
			return 0
	if opts.compile:
		config.output = "mod.o"
	if opts.output:
		config.output = opts.output
	if opts.platform:
		if not config.quiet:
			print "soclib-cc: Entering directory `%s'"%(
				os.path.abspath(os.getcwd()))
		import soclib_cc.platform as pf
		glbls = {}
		for n in pf.__all__:
			glbls[n] = getattr(pf, n)
		locs = {}
		execfile(opts.platform, glbls, locs)
		try:
			todo = locs['todo']
		except:
			print """
Can't find variable `todo' in `%(script)s'. You must define your
platform with something like:

var1 = 'param1'
var2 = 42

todo = Platform(
	Uses('xcache', param_name = var1, param_name2 = 0x2a, ...),
	Uses('multi_ram', ...),
	...
	Source('top.cpp'),
)
"""%{'script':opts.platform}
			return 1
		if opts.clean:
			todo.clean()
		else:
			todo.process()
	elif opts.compile:
		CxxCompile(config.output, args[0]).process()
	else:
		print 'Not supported'
		return 1
	if config.progress_bar:
		print
	return 0

if __name__ == '__main__':
	sys.exit(main())
