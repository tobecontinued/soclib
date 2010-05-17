
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
import sys
import traceback
import warnings
import soclib_utils.repos_file
from error import *
import parameter
from abstraction_levels import checker

__id__ = "$Id$"
__version__ = "$Revision$"

__all__ = ['Module', 'PortDecl',
           'MetaSignal', 'MetaPortDecl', 'SubSignal',
           'Signal','Port']

def dict_filtered_str(d):
    vals = []
    for k in sorted(d):
        v = d[k]
        if not isinstance(v, (str, unicode, int)):
            continue
        if ':' in k:
            continue
        vals.append('%s = %r'%(k, v))
    return ', '.join(vals)

class Module:

    # instance part
    module_attrs = {
        'classname' : '',
        'tmpl_parameters' : [],
        'header_files' : [],
        'global_header_files' : [],
        'implementation_files' : [],
        'implementation_type' : 'systemc',
        'object_files' : [],
        'interface_files' : [],
        'uses' : [],
        'accepts' : {},
        'defines' : {},
        'ports' : [],
        'sub_signals' : {},
        'signal' : None,
        'instance_parameters' : [],
        'local' : False,
        'extensions' : [],
        'constants' : {},
        "debug" : False,
        "debug_saved" : False,
        "deprecated":'',
        'can_metaconnect': False,
        }
    tb_delta = -3

    def __set_origin(self):
        filename, lineno = traceback.extract_stack()[self.tb_delta-1][:2]
        self.__filename = filename
        self.__lineno = lineno

    def __init__(self, typename, **attrs):
        """
        Creation of a module, with any overrides to defaults
        parameters defined in module_attrs class member.
        """
        self.__use_count = 0
        self.__typename = typename
        self.__set_origin()

        # Populate attributes
        self.__attrs = {}
        self.__attrs.update(self.module_attrs)
        for name, value in attrs.iteritems():
            if not name in self.module_attrs:
                warnings.warn(SpuriousDeclarationWarning(name, typename),
                              stacklevel = -self.tb_delta)
                continue
            self.__attrs[name] = value

        self.__attrs['abstraction_level'] = self.__typename.split(':', 1)[0]
        self.__attrs['uses'] = set(self.__attrs['uses']) | set(map(lambda p:p.Use(), self.__attrs['ports']))

        # Absolution :)
        self._mk_abs_paths(os.path.dirname(self.__filename))

        # Sanity checks for classname (entity name)
        self.__check_classname()
        self.__check_interface_files()

        self.__attrs['debug_saved'] = self.__attrs['debug']

    def __check_classname(self):
        if self.__attrs['classname']:
            c = checker[self.__attrs["abstraction_level"]]
            if not c.validClassName(self.__attrs['classname']):
                raise InvalidComponent("Invalid class name '%s' level %s: '%s'"%(
                    self.__typename, c, self.__attrs['classname']))

    def __check_interface_files(self):
        for f in self.__attrs['interface_files']:
            b = os.path.basename(f)
            d = os.path.dirname(f)
            soclib = os.path.basename(d)
            if soclib != 'soclib':
                warnings.warn(BadInterfacePath(f, 'path should end with "soclib/%s"'%b))

    def _mk_abs_paths(self, basename):
        relative_path_files = ['header_files', 'implementation_files', 'object_files', 'interface_files']
        def mkabs(name):
            return os.path.isabs(name) \
                   and name \
                   or os.path.abspath(os.path.join(basename, name))
        for attr in relative_path_files:
            self.__attrs['abs_'+attr] = map(mkabs, self.__attrs[attr])
        self.__attrs['abs_header_files'] += self.__attrs['global_header_files']
        self.__attrs['abs_header_files'] += self.__attrs['abs_interface_files']

    def instanciated(self):
        """
        Method to advertize for module usage.
        """
        self.__use_count += 1

    def isUsed(self):
        return bool(self.__use_count)

    def getModuleName(self):
        """
        Gets the fqmn
        """
        return self.__typename

    def forceDebug(self):
        self.__attrs['debug'] = True

    def cleanup(self):
        try:
            self.__attrs['debug'] = self.__attrs['debug_saved']
        except:
            pass

    def __getitem__(self, name):
        """
        Gets the attribute 'name'
        """
        import copy
        return copy.copy(self.__attrs[name])

    def fileName(self):
        return self.__filename

    def files(self):
        """
        Returns module's implementation files
        """
        r = [self.__filename]
        for l in ('abs_header_files', 'abs_implementation_files',
                  'abs_object_files', 'global_header_files'):
            r += self.__attrs[l]
        return r

    def __str__(self):
        return '<Module %s in %s>'%(self.__typename, self.__filename)

    def __repr__(self):
        kv = []
        for k, v in self.__attrs.iteritems():
            if k not in self.module_attrs:
                continue
            if v == self.module_attrs[k]:
                continue
            if k in ['ports', 'uses']:
                v = sorted(v)
            if isinstance(v, list):
                kv.append("%s = [%s]"%(k, ',\n\t\t'.join(map(repr, v))))
            elif isinstance(v, dict):
                kvkv = ',\n\t\t'.join(map(lambda x:'"%s": %s'%(x,repr(v[x])), sorted(v.iterkeys())))
                kv.append("%s = {\n\t\t%s}"%(k, kvkv))
            elif isinstance(v, Module):
                pass
            else:
                kv.append("%s = %s"%(k, repr(v)))
        return '''%(type)s("%(type_name)s",
\t%(kv)s
)
'''%dict(type_name = self.__typename,
         type = self.__class__.__name__,
         kv = ',\n\t'.join(kv))

    def pathIs(self, path):
        return path == self.__filename

    def __hash__(self):
        return hash((self.__filename, self.__lineno))

class Port:
    def __init__(self, type, name, count = None, auto = None, **args):
        self.__type = type
        self.__name = name
        self.__count = count
        self.__auto = auto
        self.__args = args
        self.where = traceback.extract_stack()[-2][0:2]
    def Use(self):
        return Uses(self.__type, **self.__args)
    def getSpec(self, **args):
        from specialization import Specialization
        a = {}
        a.update(args)
        a.update(self.__args)
        return Specialization(self.__type, **a)
    def getMetaInfo(self):
        return self.__type, self.__name, self.__count
    def getInfo(self, **args):
        ptype = self.getSpec(**args)
        return self.__name, ptype, self.__count, self.__auto
    def __str__(self):
        from specialization import Specialization
        ptype = Specialization(self.__type, **self.__args)
        return '<%s: %s %s>'%(self.__class__.__name__,
                              self.__name,
                              str(ptype['header_files']))
    def __repr__(self):
        args = dict_filtered_str(self.__args)
        return 'Port("%s", "%s", %s)'%(self.__type,
                                       self.__name,
                                       args)

class Signal(Module):
    tb_delta = Module.tb_delta-1
    def __init__(self, name, **kwargs):
        self.__accepts = kwargs['accepts']
        del kwargs['accepts']
        Module.__init__(self, name, **kwargs)

    def __getitem__(self, key):
        from soclib_desc import description_files
        if key == 'accepts':
            naccepts = {}
            for type, count in self.__accepts.iteritems():
                rtype = description_files.get_module(type)
                naccepts[rtype] = count
            return naccepts
        else:
            return Module.__getitem__(self, key)

class PortDecl(Module):
    def __getitem__(self, key):
        from soclib_desc import description_files
        if key == 'signal':
            sig = Module.__getitem__(self, 'signal')
            if sig:
                return description_files.get_module(sig)
        return Module.__getitem__(self, key)

class MetaPortDecl(PortDecl):
    pass

class MetaSignal(Signal):
    pass

class Uses:
    """
    A statement declaring the module uses a specific component (in a
    global meaning, ie hardware component or utility).

    name is the name of the component, it the
    abstraction_level:module_name as in *.sd

    args is the list of arguments useful for compile-time definition
    (ie template parameters)
    """
    def __init__(self, name, **args):
        self.name = name
        self.args = args

        for k in self.args.keys():
            if isinstance(self.args[k], parameter.Foreign) or ':' in k:
                del self.args[k]

        self.__hash = (
            hash(self.name)^
            hash(str([(k, self.args[k]) for k in sorted(self.args)])))

        # This is for error feedback purposes
        self.where = traceback.extract_stack()[-2][0:2]

    def specialization(self, **args):   
        from specialization import Specialization
        a = {}
        a.update(args)
        a.update(self.args)

        for k in a.keys():
            if isinstance(a[k], parameter.Foreign) or ':' in k:
                del a[k]

#       print 'use', `self`
#       from pprint import pprint
#       pprint(a)
        return Specialization(self.name, __use = self.where, **a)
        
    def __str__(self):
        return '<Use %s from %s>'%(self.name, self.where)

    def __repr__(self):
        args = dict_filtered_str(self.args)
        return 'Uses(%r, %s)'%(self.name, args)

    def __cmp__(self, other):
        return (
            cmp(self.name, other.name) or
            cmp(self.args, other.args)
            )

    def __hash__(self):
        return self.__hash

class SubSignal(Uses):
    pass
