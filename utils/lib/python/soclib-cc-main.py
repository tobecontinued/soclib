#!/usr/bin/env python

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
import sys
import soclib_desc
import soclib_desc.description_files
import soclib_cc

__id__ = "$Id$"
__version__ = "$Revision$"

from optparse import OptionParser

def main():
    todef = []
    def define_callback(option, opt, value, parser):
        todef.append(value)
    todb = []
    def buggy_callback(option, opt, value, parser):
        todb.append(value)
    one_args = {}
    def one_arg_callback(option, opt, value, parser):
        k, v = value.split('=', 1)
        try:
            v = int(v)
        except:
            pass
        one_args[k] = v
    parser = OptionParser(usage="%prog [ -m mode ] [ -t config ] [ -vqd ] [ -c -o output input | -p pf_desc ]")
    parser.add_option('-v', '--verbose', dest = 'verbose',
                      action='store_true',
                      help="Chat a lot")
    parser.add_option('-d', '--debug', dest = 'debug',
                      action='store_true',
                      help="Print too much things")
    parser.add_option('-P', '--progress_bar', dest = 'progress_bar',
                      action='store_true',
                      help="Print evolution with a progress bar")
    parser.add_option('-D', '--define', nargs = 1, type = "string",
                      action='callback', callback = define_callback,
                      help="mode:name:DEFINE=VALUE")
    parser.add_option('-b', '--buggy', nargs = 1, type = "string",
                      action='callback', callback = buggy_callback,
                      help="Put <mode:name> module in debug mode (disable opt, ...)")
    parser.add_option('-1', '--one-module', nargs = 1, type = "string",
                      action='store', dest = 'one_module',
                      help="Only try to compile one module")
    parser.add_option('-a', '--arg', nargs = 1, type = "string",
                      action='callback', callback = one_arg_callback,
                      help="Specify arguments for one-module build")
    parser.add_option('-q', '--quiet', dest = 'quiet',
                      action='store_true',
                      help="Print nothing but errors")
    parser.add_option('-c', '--compile', dest = 'compile',
                      action='store_true',
                      help="Do a simple compilation, not a linkage")
    parser.add_option('-M', '--makefile', dest = 'gen_makefile',
                      type = 'string', nargs = 1,
                      help="Generate Makefile")
    parser.add_option('-l', dest = 'list_descs',
                      action='store_const', const = "long",
                      help="List known descriptions == --list-descs=long")
    parser.add_option('-I', dest = 'includes',
                      action='append', nargs = 1,
                      help="Append directory to .sd search path")
    parser.add_option('--list-descs', dest = 'list_descs',
                      action='store', nargs = 1, type = 'string',
                      help="List known descriptions. arg may be 'long' or 'names'")
    parser.add_option('--list-files', dest = 'list_files',
                      action='store', nargs = 1, type = 'string',
                      help="List files belonging to a given module")
    parser.add_option('--sd-convert', dest = 'sd_convert',
                      action='store_true',
                      help="Convert sd files to xml")
    parser.add_option('--complete-name', dest = 'complete_name',
                      action='store', nargs = 1, type = 'string',
                      help="Complete module name starting with ...")
    parser.add_option('--complete-separator', dest = 'complete_separator',
                      action='store', nargs = 1, type = 'string',
                      default = '',
                      help="Complete words splitted by this arg")
    parser.add_option('-x', '--clean', dest = 'clean',
                      action='store_true',
                      help="Clean all outputs, only compatible with -p")
    parser.add_option('-X', '--clean-cache', dest = 'clean_cache',
                      action='store_true',
                      help="Clean .desc file cache")
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
    parser.add_option('--getflags', dest = 'getflags',
                      action='store', type = 'string',
                      help="Get a configuration value")
    parser.add_option('-t', '--type', dest = 'type',
                      action='store', type = 'string',
                      help="Use a different configuration: *default")
    parser.add_option('-j', '--jobs', dest = 'jobs',
                      action='store', type = 'int',
                      default = 0,
                      help="Allow n parallel jobs")
    parser.add_option('--bug-report', dest = 'bug_report',
                      action='store_true',
                      help="Create a bug-reporting log")
    parser.add_option('--auto-bug-report', dest = 'auto_bug_report',
                      action='store', nargs = 1,
                      help="Auto report bug. Methods allowed: openbrowser, *none",
                      choices = ("openbrowser", "none"))
    parser.add_option('--embedded-cflags', dest = 'embedded_cflags',
                      action='store_true',
                      help="Print software include directories C flags")
    parser.add_option('--work', dest = 'workpath',
                      action='store',
                      help="When using Modelsim, use this work path")
    parser.set_defaults(auto_bug_report = "none",
                        includes = [],
                        workpath = 'work',
                        embedded_cflags = False)
    opts, args = parser.parse_args()

    from soclib_cc import bugreport
    bugreport.bootstrap(opts.bug_report, opts.auto_bug_report)

    from soclib_cc.config import config

    if opts.getpath:
        print config.path
        return 0

    config = setup_config(opts)

    for value in todef:
        ms = value.split(":")
        define = ms[-1]
        cell = ':'.join(ms[:-1])
        try:
            name, val = define.split('=', 1)
        except:
            name = define
            val = ''
        soclib_desc.description_files.get_module(cell).addDefine(name, val)

    for value in todb:
        soclib_desc.description_files.get_module(value).forceDebug()

    ## Done with setup

    if opts.clean_cache:
        soclib_desc.description_files.cleanup()
        return 0

    if opts.sd_convert:
        from soclib_xml import run
        run.convert_all()
        return 0
    
    if opts.one_module:
        return compile_one_module(opts.output, opts.one_module)
        
    if opts.list_files:
        m = soclib_desc.description_files.get_module(opts.list_files)
        for h in m['abs_header_files']+m['abs_implementation_files']:
            print h
        return 0
    
    if opts.list_descs is not None:
        return list_descs(opts.list_descs)
    
    if opts.complete_name is not None:
        return complete_name(opts.complete_name, opts.complete_separator)
    
    if opts.getflags:
        if opts.getflags == 'cflags':
            print ' '.join(config.common_cflags)
        return 0

    if opts.platform:
        compile_platform(opts.platform,
                         embedded_cflags = opts.embedded_cflags,
                         gen_makefile = opts.gen_makefile,
                         clean = opts.clean)
        if config.progress_bar:
            print
        return 0
    
    if opts.compile:
        from soclib_cc.builder.cxx import CxxCompile
        CxxCompile(config.output, args[0]).process()
        if config.progress_bar:
            print
        return 0
    
    parser.print_help()
    return 1

def compile_one_module(output, one_module):
    from soclib_cc.builder.todo import ToDo
    from soclib_cc.builder.cxx import CxxMkobj
    from soclib_desc.specialization import Specialization
    from soclib_cc.component_builder import ComponentBuilder
    todo = ToDo()
    class foo:
        def fullyQualifiedModuleName(self, d):
            return d
        def putArgs(self, d):
            pass
    out = []
    f = foo()
    for module in one_module.split(','):
        mod = Specialization(module, **one_args)
#           mod.printAllUses()
        for b in mod.getSubTree():
            for o in ComponentBuilder.fromSpecialization(b).results():
                todo.add(o)
                out.append(o)
        if output:
            todo.add(CxxMkobj(output, out).results()[0])
    todo.process()
    return 0

def complete_name(name, separator):
    completions = set()
    suffix = name
    for sep in separator:
        suffix = suffix.split(sep)[-1]
    prefix_len = len(name)-len(suffix)
    for mod in soclib_desc.description_files.get_all_modules():
        name = mod.getModuleName()
        if name.startswith(name):
            client = name[prefix_len:]
            completions.add(client)
    print '\n'.join(completions)
    return 0

def list_descs(mode):
    if mode == 'long':
        for desc in soclib_desc.description_files.get_all_modules():
            print desc.getModuleName()
            for f in desc.files():
                print ' -', f
    elif mode == 'names':
        for module in soclib_desc.description_files.get_all_modules():
            print desc.getModuleName()
    else:
        print "Please give arg 'long' or 'names'"
        return 1
    return 0

def compile_platform(platform,
                     embedded_cflags = False,
                     gen_makefile = False,
                     clean = False):
    from soclib_cc.config import config
    if not config.quiet and not embedded_cflags:
        print "soclib-cc: Entering directory `%s'"%(
            os.path.abspath(os.getcwd()))
    import soclib_cc.platform as pf
    glbls = {}
    for n in pf.__all__:
        glbls[n] = getattr(pf, n)
    glbls['config'] = config
    locs = {}
    exec file(platform) in glbls, locs
    try:
        todo = locs['todo']
    except:
        raise ValueError("Can't find variable `todo' in `%s'."%platform)
    if gen_makefile:
        fd = open(gen_makefile, 'w')
        fd.write(todo.genMakefile())
        fd.close()
    elif clean:
        todo.clean()
        soclib_desc.description_files.cleanup()
    elif embedded_cflags:
        print todo.embeddedCodeCflags()
    else:
        todo.process()

def setup_config(opts):
    from soclib_cc.config import config, change_config
    if opts.type:
        change_config(opts.type)

    for path in opts.includes:
        soclib_desc.description_files.add_path(path, False)

    config.set("mode", opts.mode)
    config.set("verbose", opts.verbose)
    config.set("debug", opts.debug)
    config.set("quiet", opts.quiet)
    config.set("workpath", os.path.abspath(opts.workpath))
    if opts.jobs:
        config.toolchain.set("max_processes", opts.jobs)

    if opts.progress_bar:
        # Dont put progress bar in emacs, it is ugly !
        if os.getenv('EMACS') == 't':
            print 'Progress-bar disabled, you look like in emacs'
        else:
            config.set("progress_bar", True)
            config.set("quiet", True)

    config.set("output", "system.x")

    if opts.compile:
        config.set("output", "mod.o")

    if opts.output:
        config.set("output", opts.output)

    return config

if __name__ == '__main__':
    sys.exit(main())
