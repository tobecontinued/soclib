
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

from soclib_cc.config import config
import depends
import os, os.path, time
import sys
import select
import tempfile
import functools
import operator

__id__ = "$Id$"
__version__ = "$Revision$"

class NotFound(Exception):
    pass

class ActionFailed(Exception):
    def __init__(self, rval, action):
        Exception.__init__(self, "%s failed: %s"%(action, rval))
        self.rval = rval
        self.action = action

def get_times(files, default, cmp_func, ignore_absent):
    most_time = default
    for f in files:
        if f.exists():
            most_time = cmp_func((f.last_mod, most_time))
        else:
            if ignore_absent:
                continue
            else:
                raise NotFound, f
    return most_time

get_newest = lambda files, ignore_absent: get_times(files, 0, max, ignore_absent)
get_oldest = lambda files, ignore_absent: get_times(files, time.time(), min, ignore_absent)

def check_exist(files):
    return functools.reduce(operator.and_, map(lambda x:x.exists(), files), True)

class Action:
    priority = 0
    comp_mode = None
    __jobs = {}
    def __init__(self, dests, sources, **options):
        from bblock import bblockize, BBlock, AnonymousBBlock
        if not dests:
            self.dests = [AnonymousBBlock(self)]
        else:
            self.dests = bblockize(dests, self)
        self.sources = bblockize(sources)
        self.options = options
        self.done = False
        self.__done = False
        self.__hash = hash(
            functools.reduce(
            lambda x,y:(x + (y<<1)),
            map(hash, self.dests+self.sources),
            0))
        self.has_deps = False
        map(lambda x:x.addUser(self), self.sources)

    def prepare(self):
        pass

    def getBblocks(self):
        return self.dests + self.sources

    def launch(self, cmd, cwd = None):
        import subprocess
        #print "---- run", cmd

        if isinstance(cmd, (str, unicode)):
            vcmd = cmd
            shell = True
        else:
#            print cmd
            vcmd = ' '.join(cmd)
            shell = False

        self.__command = vcmd
        if config.verbose:
            self.runningCommand('lauch', self.dests, vcmd)

        self.__out = tempfile.TemporaryFile("w+b", bufsize=128)
        self.__err = tempfile.TemporaryFile("w+b", bufsize=128)

        self.__handle = subprocess.Popen(
            cmd,
            shell = shell,
            cwd = cwd,
            bufsize = 128*1024,
            close_fds = True,
            stdin = None,
            stdout = self.__out,
            stderr = self.__err,
            )
        self.__class__.__jobs[self.__handle.pid] = self

    @classmethod
    def pending_action_count(cls):
        return len(cls.__jobs)

    @classmethod
    def wait(cls):
        try:
            (pid, st) = os.wait()
            killed = st & 0x80
            sig = st & 0x7f
            ret = st >> 8
            cls.__jobs[pid].__handle.returncode = killed and -sig or ret
        except Exception, e:
            pass
        for job in cls.__jobs.values()[:]:
            p = job.__handle.poll()
#            print "---- poll", job, p
            if job.__handle.returncode is not None:
                job.__set_done()

    def __set_done(self):
        try:
            self.__handle.communicate()
        except:
            pass
        self.__out.seek(0)
        out = self.__out.read()
        self.__err.seek(0)
        err = self.__err.read()
        del self.__out
        del self.__err

        #print '--'
        del self.__class__.__jobs[self.__handle.pid]
        if out:
            sys.stdout.write('\n')
            sys.stdout.write(out)
        if err:
            sys.stderr.write('\n')
            sys.stderr.write(err)
        if self.__handle.returncode:
            if self.__handle.returncode == -2: # sigint
                raise KeyboardInterrupt()
#            print self.__handle.returncode
            raise ActionFailed(self.__handle.returncode, self.__command)
        #print '--'
        del self.__handle
        del self.__command
        for d in self.dests:
            d.rehash(True)
        self.__done = True
        #print "---- done"

    def isBackground(self):
        try:
            return self.__handle.poll() is None
        except:
            return False

    def todoRehash(self):
        self.must_be_processed = self.mustBeProcessed()

    def todoInfo(self):
        if self.__done:
            return '='
        if self.isBackground():
            return 'W'
        if self.must_be_processed:
            return ' '
        return '-'

    def processDeps(self):
        return []
    def process(self):
        self.todoRehash()
    def getDepends(self):
        if not self.has_deps:
            depname = self.__class__.__name__+'_%08u.deps'%hash(self.dests[0].filename)
            try:
                self.__depends = depends.load(depname)
            except:
                self.__depends = self._depList()
                depends.dump(depname, self.__depends)
            self.has_deps = True
        return self.__depends
    def mustBeProcessed(self):
        deps = self.getDepends()
        #print "mustBeProcessed", self,
        if not check_exist(deps):
            #print 'ne'
            return True
        newest_src = get_newest(deps, ignore_absent = False)
        oldest_dest = get_oldest(self.dests, ignore_absent = True)
        #print 'new', newest_src, 'old', oldest_dest, 'ex', check_exist(self.dests)
        r = (newest_src > oldest_dest) or not check_exist(self.dests)
        if r:
            map(lambda x:x.delete(), self.dests)
        return r
    def _depList(self):
        return self.sources+self.processDeps()
    def canBeProcessed(self):
        return check_exist(self.sources)
    def dumpAbsentPrerequisites(self):
        for s in self.sources:
            if not s.exists():
                print s
    def runningCommand(self, what, outs, cmd):
        if not config.quiet:
            print self.__class__.__name__, what, ', '.join(map(str,outs))
        if config.verbose:
            print cmd
    def clean(self):
        for i in self.dests:
            i.clean()
    def __hash__(self):
        return self.__hash
    def __cmp__(self, other):
        if other.__class__.__cmp__ != self.__class__.__cmp__:
            r = cmp(other, self)
            return -r
        return cmp(self.priority, other.priority)

    def genMakefileCleanCommands(self):
        return '\t$(CMDPFX)-rm -f %s\n'%(' '.join(map(lambda x:'"%s"'%x, self.dests)))

    def genMakefileTarget(self):
        def ex(s):
            return str(s).replace(' ', '\\ ').replace(':', '$(ECOLON)')
        def ex2(s):
            return str(s).replace(' ', '\\ ').replace(':', '$$(COLON)')
        r = (' '.join(map(ex, self.dests)))+\
               ' : '+\
               (' '.join(map(ex2, self.sources)))+\
               '\n'
        r += '\t@echo "%s $@"\n'%self.__class__.__name__
        for i in self.commands_to_run():
            r += '\t$(CMDPFX)%s\n'%i
        return r+'\n'

    def commands_to_run(self):
        return ('# Undefined command in '+self.__class__.__name__),

    def __str__(self):
        l = lambda a:' '.join(map(lambda x:os.path.basename(x.filename), a))
        return "<%s:\n sources: %s\n dests: %s>"%(self.__class__.__name__,
                                                  l(self.sources),
                                                  l(self.dests))

class Noop(Action):
    def __init__(self):
        Action.__init__(self, [], [])
    def getDepends(self):
        return []
