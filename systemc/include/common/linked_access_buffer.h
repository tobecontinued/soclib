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

namespace soclib { namespace common {

template<typename addr_t, typename id_t>
struct LinkedAccessEntry
{
	addr_t address;
	id_t id;
	bool atomic;

	inline void invalidate()
	{
		address = 0;
		id = 0;
		atomic = false;
	}
    inline void set( addr_t addr, id_t i )
    {
        address = addr;
        id = i;
        atomic = true;
    }
    inline bool is_atomic( addr_t addr, id_t i ) const
    {
        return addr == addr && id == i && atomic;
    }
    inline void do_atomic( addr_t addr, id_t i )
    {
        if ( is_atomic( addr, i ) )
            invalidate();
    }
};

template<typename addr_t, typename id_t>
class LinkedAccessBuffer
{
	typedef LinkedAccessEntry<addr_t, id_t> entry_t;
	entry_t * m_access;
	size_t m_n_entry;
	size_t m_ptr;

    inline size_t next() {
        size_t r = m_ptr;
        m_ptr = (m_ptr+1)%m_n_entry;
        return r;
    }

	entry_t &lookup_create( addr_t address );

	entry_t *lookup( addr_t address );
	const entry_t *lookup( addr_t address ) const;
public:
	LinkedAccessBuffer( size_t n_entry );
	void resize( size_t n_entry );
	~LinkedAccessBuffer();

	void clearAll();
	void clearNext();
	void accessDone( addr_t address );
	void doLoadLinked( addr_t address, id_t id );
	void doStoreConditional( addr_t address, id_t id );
	bool isAtomic( addr_t address, id_t id ) const;
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

