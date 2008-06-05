/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#include "module_holder.h"
#include "port_wrapper"

namespace soclib { namespace common {

std::vector<ModuleHolder*> ModuleHolder::s_existing_modules;

ModuleHolder::ModuleHolder( BaseModule *mod )
	: m_module(mod)
{
	s_existing_modules.push_back(this);
}

ModuleHolder::ModuleHolder( BaseModule &mod )
	: m_module(&mod)
{
	s_existing_modules.push_back(this);
}

ModuleHolder::~ModuleHolder()
{
	// TODO !
	//    s_existing_modules.erase(this);
    for ( std::vector<inst::BasePort*>::iterator i = m_ports.begin();
		  i != m_ports.end();
		  ++i )
		delete *i;
}

void ModuleHolder::autoConn()
{
    for ( std::vector<inst::BasePort*>::iterator i = m_ports.begin();
		  i != m_ports.end();
		  ++i )
		(*i)->autoConn();
}

void ModuleHolder::autoConnAll()
{
    for ( std::vector<ModuleHolder*>::iterator i = s_existing_modules.begin();
		  i != s_existing_modules.end();
		  ++i )
		(*i)->autoConn();
}

void ModuleHolder::portRegister( inst::BasePort *port )
{
    m_ports.push_back(port);
    m_ports.back()->ownerNameSet(m_module->name());
}

inst::BasePort& ModuleHolder::portGet( const std::string &req_name )
{
    for ( std::vector<inst::BasePort*>::iterator i = m_ports.begin();
		  i != m_ports.end();
		  ++i ) {
		if ( (*i)->isNamed(req_name) )
			return **i;
    }
    throw soclib::exception::RunTimeError(
		std::string("Cant find port `")+req_name+
		"' in module `"+m_module->name()+"'");
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

