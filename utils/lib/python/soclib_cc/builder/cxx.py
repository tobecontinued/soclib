
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

import os, os.path
import subprocess
import action
import sys
import fileops
import mfparser
import bblock
from soclib_cc.config import config
try:
    from functools import reduce
except:
    pass

__id__ = "$Id$"
__version__ = "$Revision$"

class CCompile(action.Action):
    priority = 100
    tool = 'CC'
    def __init__(self, dest, src, defines = {}, inc_paths = [], includes = [], force_debug = False, comp_mode = 'normal'):
        action.Action.__init__(self, [dest], [src], defines = defines, inc_paths = inc_paths, includes = includes)
        self.mode = force_debug and "debug" or None
        self.comp_mode = comp_mode
        if force_debug:
            defines["SOCLIB_MODULE_DEBUG"] = '1'
    def argSort(self, args):
        libdirs = filter(lambda x:str(x).startswith('-L'), args)
        libs = filter(lambda x:str(x).startswith('-l'), args)
        args2 = filter(lambda x:x not in libs and x not in libdirs, args)
        return args2+libdirs+libs
    def call(self, what, args, disp_normal = True):
        args = self.argSort(args)
        self.launch(args)
    def command_line(self, mode = ''):
        args = config.getTool(self.tool, mode)
        args += map(lambda x:'-D%s=%s'%x, self.options['defines'].iteritems())
        args += map(lambda x:'-I%s'%x, self.options['inc_paths'])
        for i in self.options["includes"]:
            args.append('-include')
            args.append(i)
        args += config.getCflags(self.mode)
        return args
    def _processDeps(self, filename):
        filename.generator.process()
        cmd = self.command_line()+['-MM', '-MT', 'foo.o']
        cmd.append(str(filename))
        cmd = self.argSort(cmd)
        process = subprocess.Popen(cmd,
                                   stdout = subprocess.PIPE,
                                   stderr = subprocess.PIPE)
        blob, err = process.communicate()
        from bblock import bblockize
        try:
            deps = mfparser.MfRule(blob)
        except ValueError:
            sys.stderr.write(err)
            sys.stderr.write('\n\n')
            raise action.ActionFailed("Unable to compute dependencies", cmd)
        return bblockize(deps.prerequisites)
    def processDeps(self):
        return reduce(lambda x, y:x+y, map(self._processDeps, self.sources), [])
    def process(self):
        fileops.CreateDir(os.path.dirname(bblock.filenames(self.dests)[0])).process()
        if self.comp_mode == 'sccom':
            args = self.command_line('SCCOM')
            args += ['-work', config.workpath]
        else:
            args = self.command_line()
            args += ['-c', '-o', bblock.filenames(self.dests)[0]]
        args += map(str, self.sources)
        r = self.call('compile', args)
        if r:
            print
            print r
        self.dests[0].touch()
        action.Action.process(self)
    def results(self):
        return self.dests

    def commands_to_run(self):
        args = self.command_line() + [
                '-c', '-o', bblock.filenames(self.dests)[0]]
        args += self.sources
        return ' '.join(map(lambda x:'"%s"'%str(x), args)),

class CxxCompile(CCompile):
    tool = 'CXX'

class CLink(CCompile):
    priority = 200
    tool = 'CC_LINKER'
    def __init__(self, dest, objs):
        action.Action.__init__(self, [dest], objs)
    def processDeps(self):
        return []
    def process(self):
        args = config.getTool(self.tool)
        if config.get_library('systemc').vendor in ['sccom', 'modelsim']:
            args += ['-link']
            args += ['-lpthread']
            args += ['-work', config.workpath]
            args += config.getLibs()
            objs = filter(lambda x:x.generator.comp_mode not in ['sccom', 'modelsim'], self.sources)
            args += bblock.filenames(objs)
        else:
            args += ['-o', bblock.filenames(self.dests)[0]]
            args += config.getLibs()
            objs = self.sources
            args += bblock.filenames(objs)

        r = self.call('link', args)
        if r:
            print
            print r
        action.Action.process(self)
    def mustBeProcessed(self):
        return True

    def commands_to_run(self):
        args = config.getTool(self.tool) + [
                '-o', self.dests[0]]
        args += config.getLibs()
        args += self.sources
        return ' '.join(map(lambda x:'"%s"'%str(x), args)),

class CxxLink(CLink):
    tool = 'CXX_LINKER'

class CMkobj(CLink):
    priority = 200
    tool = 'LD'
    def process(self):
        if config.get_library('systemc').vendor in ['sccom', 'modelsim']:
            self.tool = "CXX_LINKER"
            return CLink.process(self)
        else:
            args = config.getTool(self.tool)
            args += ['-r', '-o', bblock.filenames(self.dests)[0]]
            args += bblock.filenames(self.sources)
        r = self.call('mkobj', args)
        if r:
            print
            print r
        action.Action.process(self)

class CxxMkobj(CMkobj):
    pass

if __name__ == '__main__':
    import sys
    cc = CxxCompile(sys.argv[1], sys.argv[2])
    #print cc.processDeps()
    can = cc.canBeProcessed()
    must = cc.mustBeProcessed()
    print "Can be processed:", can
    print "Must be processed:", must
    if can and must:
        print "Processing...",
        sys.stdout.flush()
        cc.process()
        print "done"
