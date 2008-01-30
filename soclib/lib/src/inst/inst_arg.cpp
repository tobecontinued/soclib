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

#include "exception.h"
#include "inst/inst_arg.h"

namespace soclib {
namespace common {
namespace inst {

InstArg::InstArg()
{
	
}

bool InstArg::has( const std::string &name ) const
{
	for ( std::vector<InstArgBaseItem*>::const_iterator i = m_items.begin();
		  i != m_items.end();
		  ++i ) {
		if ( (*i)->name() == name )
			return true;
	}
	return false;
}

InstArgBaseItem &InstArg::get( const std::string &name )
{
	for ( std::vector<InstArgBaseItem*>::iterator i = m_items.begin();
		  i != m_items.end();
		  ++i ) {
		if ( (*i)->name() == name )
			return **i;
	}
	throw soclib::exception::RunTimeError(
		std::string("Cant find item `")+name+
		"' in InstArg");
}

void InstArg::add( InstArgBaseItem*item )
{
	m_items.push_back( item );
}

}}}
