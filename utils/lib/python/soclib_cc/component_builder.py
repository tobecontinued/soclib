
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
#         Nicolas Pouillon <nipo@ssji.net>, 2009
# 
# Maintainers: group:toolmakers

import os, os.path
import subprocess
import operator
from soclib_cc.builder.action import *
from soclib_cc.builder.cxx import *
from soclib_cc.builder.hdl import *
from soclib_cc.builder.bblock import *
from soclib_cc.builder.textfile import *
try:
    from functools import reduce
except:
    pass

__id__ = "$Id$"
__version__ = "$Revision$"

class UndefinedParam(Exception):
    def __init__(self, where, comp, param):
        Exception.__init__(self)
        self.where = ':'.join(where)
        self.comp = comp
        self.param = param
    def __str__(self):
        return '\n%s:error: parameter %s not defined for component %s'%(
            self.where, self.param, self.comp)

class ComponentInstanciationError(Exception):
    def __init__(self, where, comp, err):
        Exception.__init__(self)
        self.where = ':'.join(where)
        self.comp = comp
        self.err = err
    def __str__(self):
        return '\n%s:error: component %s: %s'%(
            self.where, self.comp, self.err)

class TooSimple(Exception):
    pass

class ComponentBuilder:
    
    def results(self):
        return []

class CxxComponentBuilder(ComponentBuilder):
    def __init__(self,
                 module_name,
                 classname,
                 implementation_files,
                 header_files,
                 tmpl_header_files = [],
                 interface_files = [],
                 defines = {},
                 local = False,
                 force_debug = False,
                 ):
        self.__module_name = module_name
        self.__classname = classname
        self.__implementation_files = list(implementation_files)
        self.__header_files = list(header_files)
        self.__tmpl_header_files = list(tmpl_header_files)
        self.__interface_files = list(interface_files)
        self.__defines = defines
        self.force_debug = force_debug
        self.force_mode = self.force_debug and "debug" or None
        self.local = local

    def getCxxBuilder(self, filename, *add_filenames):
        bn = self.baseName()
        if add_filenames:
            bn += '_all'
        else:
            bn += '_'+os.path.splitext(os.path.basename(filename))[0]
        from config import config
        source = self.cxxSource(filename, *add_filenames)
        if source:
            tx = CxxSource(
                config.reposFile(bn+".cpp", self.force_mode),
                source)
            src = tx.dests[0]
        else:
            src = filename
        incls = set(map(os.path.dirname,
                        self.__header_files
                        + self.__interface_files
                        + self.__tmpl_header_files
                        ))

        # Take list of headers where basenames collides
        bincludes = {}
        includes = set()
        for h in list(self.__header_files):
            name = os.path.basename(h)
            if name in bincludes:
                includes.add(bincludes[name])
                includes.add(h)
            else:
                bincludes[name] = h
        includes = list(includes)
        
        if self.local:
            try:
                t = config.type
            except:
                t = 'unknown'
            out = os.path.join(
                os.path.dirname(filename),
                t+'_'+bn+"."+config.toolchain.obj_ext)
        else:
            out = config.reposFile(bn+"."+config.toolchain.obj_ext, self.force_mode)

        add = {}
        if config.get_library('systemc').vendor in ['sccom', 'modelsim']:
            add['comp_mode'] = 'sccom'
            out = os.path.abspath(os.path.join(
                config.get_library('systemc').sc_workpath,
                os.path.splitext(os.path.basename(str(src)))[0]
                +'.'+config.toolchain.obj_ext
                ))
        return CxxCompile(
            dest = out,
            src = src,
            defines = self.__defines,
            inc_paths = incls,
            includes  = includes,
            force_debug = self.force_debug,
            **add)

    def __hash__(self):
        return hash(self.__module_name + self.__classname)

    def __cmp__(self):
        return (self.__module_name + self.__classname) == \
               (other.__module_name + other.__classname)

    def baseName(self):
        basename = self.__module_name
        tp = self.__classname
        basename += "_%08x"%hash(self)
        basename += "_" + tp.replace(' ', '_')
        params = ",".join(
            map(lambda x:'%s=%s'%x,
                self.__defines.iteritems()))
        if params:
            basename += "_" + params
        return basename.replace(' ', '_')

    def results(self):
        is_template = '<' in self.__classname
        impl = self.__implementation_files
        if is_template and len(impl) > 1:
            builders = [self.getCxxBuilder(*impl)]
        else:
            builders = map(self.getCxxBuilder, impl)
        return reduce(operator.add, map(lambda x:x.dests, builders), [])


    def cxxSource(self, *sources):
        if not '<' in self.__classname:
            if len(sources) > 1:
                source = ''
                for s in sources:
                    source += '#include "%s"\n'%s
                return source
            else:
                return ''

        inst = 'class '+self.__classname

        source = ""
        for h in set(map(os.path.basename, self.__tmpl_header_files)):
            source += '#include "%s"\n'%h
        from config import config
        for h in config.toolchain.always_include:
            source += '#include <%s>\n'%h
        for s in sources:
            source += '#include "%s"\n'%s
        source += "#ifndef ENABLE_SCPARSE\n"
        source += 'template '+inst+';\n'
        source += "#endif\n"
        return source

class HdlComponentBuilder(ComponentBuilder):
    def __init__(self,
                 module_name,
                 classname,
                 language,
                 header_files,
                 implementation_files,
                 used_implementation_files,
                 ):
        self.__module_name = module_name
        self.__classname = classname
        self.__language = language
        self.results = getattr(self, self.__language+'_results')
        self.__header_files = list(header_files)
        self.__implementation_files = list(implementation_files)
        self.__used_implementation_files = list(used_implementation_files)

    # Builders for verilog, vhdl, systemc and mpy_vhdl.
    def verilog_results(self):
        return self.hdl_results(VerilogCompile)

    def vhdl_results(self):
        return self.hdl_results(VhdlCompile)

    def hdl_results(self, func):
        vendor = config.get_library('systemc').vendor
        if vendor not in ['sccom', 'modelsim']:
            raise NotImplementedError("Unsuported vendor %s"%vendor)

        deps = list(set(self.__used_implementation_files)
                    - set(self.__implementation_files))
        deps = bblockize(deps)

        incs = set()
        for i in self.__header_files:
            incs.add(os.path.dirname(i))
        builders = []
        for f in self.__implementation_files:
            b = func(dest = [], srcs = [f],
                     needed_compiled_sources = deps,
                     incs = incs,
                     typename = self.__classname)
            builders.append(b)
#            print b
            deps = b.sources
        return reduce(operator.add, map(lambda x:x.dests, builders), [])

    def mpy_vhdl_results(self):
        import pprint
        basedir = config.reposFile('mpy')
        input_file = os.path.basename(self.specialization.getImplementationFiles()[0])
        template = 'mpy_'+hex(hash(input_file))+'.mpy'
        params = self.get_recursive_dict(self.specialization.getTmplParamDict())
        fd = open(os.path.join(basedir, template), 'w')
        fd.write('''{
params = %(dict)s

mpygen('%(name)s', params)
}
'''%dict(name = input_file,
         dict = pprint.pformat(params)))
        fd.close()
        vhd_files = self.call_mpy(template)
        return self.hdl_results(self.getVhdlBuilder, vhd_files)
    
    @staticmethod
    def get_recursive_dict(d):
        params = {}
        for k, v in d.iteritems():
            p = params
            keys = k.split('__')
            for k in keys[:-1]:
                if k not in p:
                    p[k] = {}
                p = p[k]
            p[keys[-1]] = v
        return params

    def call_mpy(self, source_file):
        inst_name = 'mpy_'+hex(hash(source_file))
        basedir = config.reposFile('mpy')
        try:
            os.makedirs(basedir)
        except:
            pass

        implementations = set()
        for d in self.specialization.get_used_modules():
            implementations |= set(d.getImplementationFiles())
        for d in self.specialization.getTmplSubTree():
            implementations |= set(d.getImplementationFiles())
        directories = set(map(os.path.dirname, implementations))
            
        cmd = config.getTool('MPY')
        cmd += ['-o', inst_name+'.results']
        cmd += ['-n', inst_name]
        cmd += ['-w', basedir, source_file]
        cmd += ['-i', ':'.join(directories)]
        proc = subprocess.Popen(cmd,
                                shell = False,
                                stdin = None, stdout = subprocess.PIPE,
                                cwd = basedir)
        (out, err) = proc.communicate()
        if proc.returncode:
            raise RuntimeError("mpy failed:\n"+out)
        files = map(str.strip, open(os.path.join(basedir, inst_name+'.list')).readlines())
        files = filter(lambda x:x.endswith('.vhd'), files)
        ffiles = []
        for f in files:
            if f not in ffiles:
                ffiles.append(f)
#        files.add(os.path.join(basedir, inst_name+'.vhd'))
        ffiles.reverse()
        return ffiles
