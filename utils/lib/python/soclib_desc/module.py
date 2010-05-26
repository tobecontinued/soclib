
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

__author__ = 'Nicolas Pouillon, <nipo@ssji.net>'
__copyright__ = 'UPMC, Lip6, SoC, 2007-2010'
__license__ = 'GPL-v2'
__id__ = "$Id$"
__version__ = "$Revision$"

__all__ = ['ModuleInterface', 'PortInterface', 'SignalInterface']

class ModuleCommon:

    def __init__(self, name, source_filename, source_lineno = 0):
        '''
        Creates the Module.
         * name is the fully qualified module name, like caba:vci_foo_bar
         * source_filename is the file where this module got declared
         * source_lineno is the line number where module got declared
        '''
        self.__name = name
        self.__filename = source_filename
        self.__lineno = source_lineno
        self.__debug_mode = False

    def cleanup(self):
        self.set_debug_mode(False)

    def set_debug_mode(self, debug = True):
        '''
        Sets module to debug mode, what happens in specialized modules
        when they are in debug mode is implementation-dependant
        '''
        self.__debug_mode = debug

    @property
    def debug_mode(self):
        '''
        Current debug mode
        '''
        return self.__debug_mode

    def __hash__(self):
        '''
        Unique ID for the module
        '''
        return hash((self.__filename, self.__lineno, self.__name))

    def __eq__(self, other):
        '''
        Returns whether modules are equivalent
        '''
        return (self.__filename, self.__lineno, self.__name) == \
               (other.__filename, other.__lineno, other.__name)

    def path_is(self, path):
        '''
        Tells whether the given path is the path where module got
        declared.
        '''
        return path == self.__filename

    @property
    def name(self):
        '''
        fully qualified module name
        '''
        return self.__name

    # API to implement

    def related_files(self):
        '''
        Gets a collection of files related to this module
        '''
        raise NotImplementedError()

class ModuleInterface:

    # API to implement

    def cleanup(self):
        self.set_debug_mode(False)

    def set_debug_mode(self, debug = True):
        '''
        Sets module to debug mode, what happens in specialized modules
        when they are in debug mode is implementation-dependant
        '''
        self.__debug_mode = debug

    @property
    def debug_mode(self):
        '''
        Current debug mode
        '''
        return self.__debug_mode

    def is_used(self):
        '''
        Returns whether the module got specialized at least once.
        '''
        raise NotImplementedError()

    def get_template_parameters(self):
        '''
        Returns a list of parameters to define in order to be able to
        specialize this module.
        '''
        raise NotImplementedError()

    def specialize(self, **params):
        '''
        Creates a Specialization for the module, with the given
        parameters.
        '''
        raise NotImplementedError()

class PortInterface:
    pass

class SignalInterface:
    pass
