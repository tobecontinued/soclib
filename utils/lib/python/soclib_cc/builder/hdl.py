
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
#         Nicolas Pouillon <nipo@ssji.net>, 2010

import os, os.path
import action
import sys
import fileops
from soclib_cc.config import config

__id__ = "$Id$"
__version__ = "$Revision$"

__all__ = ['VhdlCompile', 'VerilogCompile']

class HdlCompile(action.Action):
    priority = 150
    def __init__(self, dest, srcs, incs, usage_deps, typename):
        if dest:
            dests = [dest]
        else:
            if config.get_library('systemc').vendor in ['sccom', 'modelsim']:
                assert not dest, ValueError("Must not specify output path with Modelsim")
#                dest = os.path.abspath(os.path.join(
#                    config.workpath, typename, 'rtl.dat'
#                    ))
                dests = []
            else:
                raise NotImplementedError("Not supported")
        self.__usage_deps = usage_deps
        self.__prepared = False
        action.Action.__init__(self, dests, srcs, typename = typename,
                               incs = incs)

    def __users(self):
        us = set()
        for d in self.__usage_deps:
            for u in d.users:
                us.add(u)
        return us

    def prepare(self):
        if self.__prepared:
            return
        
        r = set()
        for u in self.__users():
            r |= set(u.dests)
        r -= set(self.dests)
        self.__deps = list(r)

        action.Action.prepare(self)

        self.__prepared = True

    def getBblocks(self):
        return self.__deps + action.Action.getBblocks(self)

    def processDeps(self):
        return self.__deps

    def add_args(self):
        return []

    def process(self):
        fileops.CreateDir(os.path.dirname(str(self.dests[0]))).process()
        args = config.getTool(self.tool)
        if config.get_library('systemc').vendor in ['sccom', 'modelsim']:
            args += ['-work', config.workpath]
            args += config.toolchain.vflags
        args += self.add_args()
        args += map(str, self.sources)
        r = self.launch(args)
        if r:
            print
            print r
        action.Action.process(self)

    def mustBeProcessed(self):
        try:
            return not self.done
        except:
            return True

    def results(self):
        return self.dests

    def __cmp__(self, other):
        if not isinstance(other, HdlCompile):
            return cmp(self.priority, other.priority)
        import bblock
        self_users = self.__users()
        other_users = other.__users()
        if self in other_users:
#            print '***', bblock.filenames(other.sources), '->', bblock.filenames(self.sources)
            return -1
        if other in self_users:
#            print '***', bblock.filenames(self.sources), '->', bblock.filenames(other.sources)
            return 1
        return 0

    def __str__(self):
        import bblock
        r = action.Action.__str__(self)
        if self.__prepared:
            return r[:-1] + '\n Deps: %s>'%(map(str, self.processDeps()))
        else:
            return r[:-1] + '\n Deps: [unprepared]>'

class VhdlCompile(HdlCompile):
    tool = 'VHDL'

class VerilogCompile(HdlCompile):
    tool = 'VERILOG'

    def add_args(self):
        return map('+incdir+'.__add__, self.options['incs'])
        
