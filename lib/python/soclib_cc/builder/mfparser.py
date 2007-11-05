
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

__all__ = ["MfRule"]

def bsfilter(spacer, l):
	r = []
	next_follows = False
	for i in l:
		nf = next_follows
		if i.endswith('\\'):
			next_follows = True
			i = i[:-1]+spacer
		else:
			next_follows = False
		if nf:
			r[-1] += i
		else:
			r.append(i)
	return r

class MfRule:
	def __init__(self, text):
		lines = filter(lambda x:not x.startswith('#'),
			bsfilter("", text.split('\n')))
		dest, prereq = lines[0].split(":",1)
		self.rules = lines[1:]
		self.dest = bsfilter(" ", dest.strip().split())
		self.prerequisites = bsfilter(" ", filter(None, map(
			lambda x:x.strip(), prereq.split())))
