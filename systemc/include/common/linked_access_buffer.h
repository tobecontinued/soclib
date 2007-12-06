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
#ifndef SOCLIB_COMMON_LINKED_ACCESS_BUFFER_H_
#define SOCLIB_COMMON_LINKED_ACCESS_BUFFER_H_

#include <stdint.h>
#include <assert.h>

namespace soclib { namespace common {

template<typename addr_t, typename id_t>
struct LinkedAccessEntry
{
	addr_t address;
	bool atomic;

	inline void invalidate()
	{
		address = 0;
		atomic = false;
	}
	inline void invalidate(addr_t addr)
	{
        if ( address == addr )
            atomic = false;
	}
    inline void set( addr_t addr )
    {
        address = addr;
        atomic = true;
    }
    inline bool is_atomic( addr_t addr ) const
    {
        return addr == addr && atomic;
    }
};

template<typename addr_t, typename id_t>
class LinkedAccessBuffer
{
	typedef LinkedAccessEntry<addr_t, id_t> entry_t;
	entry_t * const m_access;
	const size_t m_n_entry;
public:
	LinkedAccessBuffer( size_t n_entry );
	~LinkedAccessBuffer();

	void clearAll()
    {
        for ( size_t i=0; i<m_n_entry; ++i )
            m_access[i].invalidate();
    }

	void accessDone( addr_t address )
    {
        for ( size_t i=0; i<m_n_entry; ++i )
            m_access[i].invalidate(address);
    }

	void doLoadLinked( addr_t address, id_t id )
    {
        assert(id < m_n_entry && "Access out of bounds");
        m_access[id].set(address);
    }

	bool isAtomic( addr_t address, id_t id ) const
    {
        assert(id < m_n_entry && "Access out of bounds");
        return m_access[id].is_atomic(address);
    }
};

}}

#endif /* SOCLIB_COMMON_LINKED_ACCESS_BUFFER_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

