
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

__all__ = ['VhdlCompile']

class HdlCompile(action.Action):
    priority = 100
    def __init__(self, dest, srcs, typename):
        if dest:
            dests = [dest]
        else:
            if config.systemc.vendor in ['sccom', 'modelsim']:
                assert dest is None, ValueError("Must not specify output path with Modelsim")
#                dest = os.path.abspath(os.path.join(
#                    config.workpath, typename, 'rtl.dat'
#                    ))
                dests = []
            else:
                raise NotImplementedError("Not supported")
        action.Action.__init__(self, dests, srcs, typename = typename)

    def processDeps(self):
        return []

    def process(self):
        fileops.CreateDir(os.path.dirname(str(self.dests[0]))).process()
        args = config.getTool(self.tool)
        if config.systemc.vendor in ['sccom', 'modelsim']:
            args += ['-work', config.workpath]
            args += config.toolchain.vflags
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

class VhdlCompile(HdlCompile):
    tool = 'VHDL'

class VerilogCompile(HdlCompile):
    tool = 'VERILOG'

