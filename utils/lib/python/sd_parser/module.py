

import os.path
import traceback
import warnings
from soclib_desc.error import *
from soclib_desc.abstraction_levels import checker

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
