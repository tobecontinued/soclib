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
 */

#include "common/address_masking_table.h"

#include <cassert>

namespace soclib { namespace common {

#define tmpl(x) template<typename data_t> x AddressMaskingTable<data_t>

tmpl(void)::init( int use_bits, int drop_bits )
{
	m_use_bits = use_bits;
	m_drop_bits = drop_bits;
	m_low_mask = (1<<use_bits)-1;
}

tmpl(/**/)::AddressMaskingTable()
{
	init(0,0);
}

tmpl(/**/)::AddressMaskingTable( int use_bits, int drop_bits )
{
	init(use_bits, drop_bits);
}

tmpl(/**/)::AddressMaskingTable( data_t mask )
{
	size_t use_bits = 0, drop_bits = 0;
	data_t m = mask;
	
	while ( !(m & 1) ) {
		++drop_bits;
		m >>= 1;
	}
	while ( (m & 1) && (use_bits+drop_bits <= 8*sizeof(data_t)) ) {
		++use_bits;
		m >>= 1;
	}

	init(use_bits, drop_bits);
	assert(this->mask() == mask);
}

tmpl(/**/)::AddressMaskingTable( const AddressMaskingTable &ref )
{
	init(ref.m_use_bits, ref.m_drop_bits);
}

tmpl(const AddressMaskingTable<data_t> &)::operator=( const AddressMaskingTable &ref )
{
	if ( this == &ref )
		return *this;
	
	init(ref.m_use_bits, ref.m_drop_bits);
    return *this;
}

tmpl(void)::print( std::ostream &o ) const
{
    o << "<AMT: use=" << std::dec << m_use_bits << ", drop=" << m_drop_bits
      << ", mask=" << std::hex << mask() << ">" << std::endl;
}

#undef tmpl

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

