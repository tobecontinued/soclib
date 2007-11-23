/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include <stdint.h>
#include <iostream>
#include "common/linked_access_buffer.h"

namespace soclib { namespace common {

#define tmpl(...) template<typename addr_t, typename id_t> __VA_ARGS__ LinkedAccessBuffer<addr_t, id_t>

tmpl(typename LinkedAccessBuffer<addr_t, id_t>::entry_t &)::lookup_create( addr_t address )
{
	entry_t *e = lookup(address);
	if ( e != NULL )
		return *e;
	return m_access[next()];
}

tmpl(typename LinkedAccessBuffer<addr_t, id_t>::entry_t *)::lookup( addr_t address )
{
	for ( size_t i=0; i<m_n_entry; ++i )
		if ( address == m_access[i].address )
			return &m_access[i];
	return NULL;
}

tmpl(const typename LinkedAccessBuffer<addr_t, id_t>::entry_t *)::lookup( addr_t address ) const
{
	for ( size_t i=0; i<m_n_entry; ++i )
		if ( address == m_access[i].address )
			return &m_access[i];
	return NULL;
}

tmpl(/**/)::LinkedAccessBuffer( size_t n_entry = 0 )
		   : m_access(NULL), m_n_entry(0), m_ptr(0)
{
	resize( n_entry );
}

tmpl(void)::resize( size_t n_entry )
{
	if ( n_entry == 0 )
		n_entry = 1;
	if ( m_access != NULL )
		delete [] m_access;
	m_access = new entry_t[n_entry];
	m_n_entry = n_entry;
	m_ptr = 0;

}

tmpl(/**/)::~LinkedAccessBuffer()
{
	delete [] m_access;
}

tmpl(void)::clearAll()
{
	for ( size_t i=0; i<m_n_entry; ++i )
		m_access[i].invalidate();
}

tmpl(void)::clearNext()
{
	m_access[next()].invalidate();
}

tmpl(void)::accessDone( addr_t address )
{
	entry_t *e = lookup(address);
	if ( e != NULL )
		e->invalidate();
}

tmpl(void)::doLoadLinked( addr_t address, id_t id )
{
	entry_t &e = lookup_create(address);
	if ( e.address == address ) {
		if ( e.id == id )
			e.atomic = true;
		else
			return;
	} else
		e.set(address, id);
}

tmpl(void)::doStoreConditional( addr_t address, id_t id )
{
	entry_t *e = lookup(address);
	if ( e != NULL )
		e->do_atomic(address, id);
}

tmpl(bool)::isAtomic( addr_t address, id_t id ) const
{
	const entry_t *e = lookup(address);
	if ( e == NULL )
		return false;
	return e->is_atomic( address, id );
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

