
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
import soclib_cc.component.component as component
from soclib_cc.config import config
import re

def get_descs():
	sdfile = re.compile('^[^.][a-zA-Z_-]+\\.sd$')
	base = os.path.join(config.path, 'desc/soclib')
	d = filter(sdfile.match, os.listdir(base))
	glbl = {}
	for n in component.__all__:
		glbl[n] = getattr(component, n)
	from soclib_cc.platform.platform import Uses
	glbl['Uses'] = Uses
	all_c = {}
	for f in d:
		name = os.path.join(base, f)
		if os.path.isfile(name):
			locs = {}
			execfile(name, glbl, locs)
			all_c[f[:-3]] = locs
	return all_c

component_defs = get_descs()
