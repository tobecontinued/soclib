
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
import soclib_cc.component as component
from soclib_cc.config import config
import re

def get_descs_in(base):
	sdfile = re.compile('^[^.][a-zA-Z0-9_-]+\\.sd$')
	import os
	from os.path import join, getsize
	d = []
	for root, dirs, files in os.walk(os.path.abspath(base)):
		if ".svn" in dirs:
			dirs.remove(".svn")
		files = filter(sdfile.match, files)
		for f in files:
			d.append(os.path.join(root,f))
	glbl = {}
	for n in component.__all__:
		glbl[n] = getattr(component, n)
	from soclib_cc.platform import Uses
	glbl['Uses'] = Uses
	all_c = {}
	for f in d:
		name = os.path.join(base, f)
		if os.path.isfile(name):
			dirname = os.path.dirname(name)
			locs = {}
			execfile(name, glbl, locs)
			for i in locs.itervalues():
				i.mk_abs_paths(dirname)
			all_c[f[:-3]] = locs
	return all_c

def get_descs():
	all_c = {}
	for base in config.desc_paths:
		new_c = get_descs_in(base)
		for k, v in new_c.iteritems():
			if k in all_c:
				all_c[k].update(v)
			else:
				all_c[k] = v
	tmpc = {}
	for name, desc in all_c.iteritems():
		bn = os.path.basename(name)
		if bn in tmpc:
			tmpc[bn].update(desc)
		else:
			tmpc[bn] = desc
	return tmpc

component_defs = get_descs()
