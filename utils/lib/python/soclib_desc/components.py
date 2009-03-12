
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
import re

__id__ = "$Id$"
__version__ = "$Revision$"

def get_descs_in(base):
	sdfile = re.compile('^[^.][a-zA-Z0-9_-]+\\.sd$')
	from soclib_cc.config import config
	if config.sd_ignore_regexp:
		ignore_regexp_compiled = re.compile(config.sd_ignore_regexp)
	else:
		ignore_regexp_compiled = None
	import os
	from os.path import join, getsize
	d = []
	for root, dirs, files in os.walk(os.path.abspath(base)):
		if ".svn" in dirs:
			dirs.remove(".svn")
		files = filter(sdfile.match, files)
		if ignore_regexp_compiled:
			files = filter(lambda x:not ignore_regexp_compiled.match(x), files)
		for f in files:
			d.append(os.path.join(root,f))
	import component
	from soclib_desc.component import Uses
	all_c = {}
	for f in d:
		name = os.path.join(base, f)
		if os.path.isfile(name):
			dirname = os.path.dirname(name)
			glbl = {}
			for n in component.__all__:
				glbl[n] = getattr(component, n)
			glbl['Uses'] = Uses
			glbl['__name__'] = name
			locs = {}
			execfile(name, glbl, locs)

def getDescs(desc_paths):
	for base in desc_paths:
		get_descs_in(base)
