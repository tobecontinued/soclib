
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

__id__ = "$Id$"
__version__ = "$Revision$"

from soclib_cc import exceptions

class SpuriousDeclarationWarning(Warning):
    def __str__(self):
        return 'Spurious "%s" in %s declaration'%(self.args[0], self.args[1])

class BadInterfacePath(Warning):
    def __str__(self):
        return 'Interface file path "%s" %s'%(self.args[0], self.args[1])

class BadNameWarning(Warning):
    def __str__(self):
        return 'Bad component name: `%s\', %s'%(self.args[0], self.args[1])

class InvalidComponentWarning(Warning):
    def __str__(self):
        return 'Invalid component %s, it will be unavailable. Error: "%s"'%(self.args[0], self.args[1])

class ModuleDeprecationWarning(DeprecationWarning):
    def __str__(self):
        return 'Module %s deprecated: "%s"'%(self.args[0], self.args[1])



class InvalidComponent(exceptions.ExpectedException):
    pass

class ModuleExplicitFailure(exceptions.ExpectedException):
    pass

class ModuleSpecializationError(exceptions.ExpectedException):
    def __init__(self, module, context = None, prev_error = None):
        self.__module = module
        self.__context = context
        self.__prev_error = prev_error
    def __str__(self):
        return '\n'+self.format()
    def format(self, pfx = ' '):
        if isinstance(self.__prev_error, ModuleSpecializationError):
            c = '\n'+pfx+self.__prev_error.format(pfx+' ')
        else:
            c = '\n'+pfx+' '+str(self.__prev_error)
        at = ""
        if self.__context:
            at = (' at '+str(self.__context))
        return 'Error specializing %s%s, error: %s'%(self.__module, at, c)
