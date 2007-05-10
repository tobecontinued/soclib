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

#include "common/address_decoding_table.h"

#include <cassert>

namespace soclib { namespace common {

#define tmpl(x) template<typename input_t, typename output_t> x AddressDecodingTable<input_t, output_t>

tmpl(void)::init( int use_bits, int drop_bits )
{
	m_table = new output_t[1<<use_bits];
	m_use_bits = use_bits;
	m_drop_bits = drop_bits;
	m_low_mask = (1<<use_bits)-1;
}

tmpl(/**/)::AddressDecodingTable()
{
	init(0,0);
}

tmpl(/**/)::AddressDecodingTable( int use_bits, int drop_bits )
{
	init(use_bits, drop_bits);
}

tmpl(/**/)::AddressDecodingTable( input_t mask )
{
	int use_bits = 0, drop_bits = 0;
	input_t m = mask;
	
	while ( !(m & 1) ) {
		++drop_bits;
		m >>= 1;
	}
	while ( (m & 1) && (use_bits+drop_bits <= (int)(8*sizeof(input_t))) ) {
		++use_bits;
		m >>= 1;
	}

	init(use_bits, drop_bits);
	assert(this->mask() == mask);
}
    
tmpl(void)::reset( output_t value )
{
	for ( int i=0; i<(1<<m_use_bits); ++i )
		m_table[i] = value;
}

tmpl(void)::set( input_t where, output_t value )
{
	m_table[id(where)] = value;
}

tmpl(/**/)::AddressDecodingTable( const AddressDecodingTable &ref )
{
	init(ref.m_use_bits, ref.m_drop_bits);
	for ( int i=0; i<(1<<m_use_bits); ++i )
		m_table[i] = ref.m_table[i];
}

template <typename input_t, typename output_t>
const AddressDecodingTable<input_t, output_t> &AddressDecodingTable<input_t, output_t>::
operator=( const AddressDecodingTable &ref )
{
	if ( this == &ref )
		return *this;
	
	delete [] m_table;

	init(ref.m_use_bits, ref.m_drop_bits);
	for ( int i=0; i<(1<<m_use_bits); ++i )
		m_table[i] = ref.m_table[i];
    return *this;
}

tmpl(/**/)::~AddressDecodingTable()
{
	delete [] m_table;
}

tmpl(void)::print( std::ostream &o ) const
{
    o << "<ADT: use=" << std::dec << m_use_bits << ", drop=" << m_drop_bits
      << ", mask=" << std::hex << mask() << std::endl;
    for ( int i=0; i<(1<<m_use_bits); ++i )
        o << " " << i << ": " << this->m_table[i] << std::endl;
    o << '>' << std::dec;
}

tmpl(bool)::isAllBelow( output_t val ) const
{
    for ( size_t i=0; i<(size_t)(1<<m_use_bits); ++i )
        if ( m_table[i] >= val )
            return false;
    return true;
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

