
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

import os
import os.path
import re
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
            v = config.get_library('systemc').vendor
            if v in ['sccom', 'modelsim']:
                self.stdout_reformat = self.__vcom_stdout_reformat
                assert not dest, ValueError("Must not specify output path with Modelsim")
                dests = []
            else:
                raise NotImplementedError("Vendor not supported: %s"%v)
        self.__usage_deps = usage_deps
        action.Action.__init__(self, dests, srcs, typename = typename,
                               incs = incs)
#        print self.dests, self.sources, usage_deps

    def __users(self):
        us = set()
        for d in self.__usage_deps:
            for u in d.users:
                us.add(u)
        return us

    __err_re = re.compile(r'^\*\* Warning: (?P<filename>[^\(]+)\((?P<lineno>\d+)\): \((?P<tool>\w+)-(?P<error>\d+)\) (?P<warning_type>[\w\s\d]*)Warning: (?P<message>.*)$', re.M)

    def __vcom_stdout_reformat(self, err):
        repl = '\g<filename>:\g<lineno>: warning: \g<warning_type>(\g<tool>-\g<error>): \g<message>'
        return self.__err_re.sub(repl, err)

    def prepare(self):
        r = set()
        for u in self.__users():
            r |= set(u.dests)
        self.add_depends(*(r - set(self.dests)))

        args = config.getTool(self.tool)
        if config.get_library('systemc').vendor in ['sccom', 'modelsim']:
            args += ['-work', config.workpath]
            args += config.toolchain.vflags
        args += self.hdl_add_args()
        args += map(str, self.sources)
        self.run_command(args)

        action.Action.prepare(self)

    def hdl_add_args(self):
        return []

class VhdlCompile(HdlCompile):
    tool = 'VHDL'

class VerilogCompile(HdlCompile):
    tool = 'VERILOG'

    def hdl_add_args(self):
        return map('+incdir+'.__add__, self.options['incs'])
        
