#!/usr/bin/env python

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
	parser.add_option('-q', '--quiet', dest = 'quiet',
					  action='store_true',
					  help="Print nothing but errors")
	parser.add_option('-c', '--compile', dest = 'compile',
					  action='store_true',
					  help="Do a simple compilation, not a linkage")
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

	config.output = "system.x"
	if opts.getpath:
		print config.path
		return 0
	if opts.compile:
		config.output = "mod.o"
	if opts.output:
		config.output = opts.output
	if opts.platform:
		if not config.quiet:
			print "soclib-cc: Entering directory `%s'"%(
				os.path.abspath(os.getcwd()))
		import soclib_cc.platform.platform as pf
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
	Source('top.cc'),
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
	return 0

if __name__ == '__main__':
	sys.exit(main())
