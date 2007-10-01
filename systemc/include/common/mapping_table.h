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
#ifndef SOCLIB_MAPPING_TABLE_H
#define SOCLIB_MAPPING_TABLE_H

#include <inttypes.h>
#include <list>
#include <sstream>

#include "common/segment.h"
#include "common/address_decoding_table.h"
#include "common/address_masking_table.h"
#include "common/int_tab.h"

namespace soclib { namespace common {

class MappingTable
{
public:
    typedef uint32_t addr_t;

private:
    std::list<soclib::common::Segment> m_segment_list;
    size_t m_addr_width;
    const IntTab m_level_addr_bits;
    const IntTab m_level_id_bits;
    const addr_t m_cacheability_mask;

    MappingTable( const MappingTable& );
    const MappingTable &operator=( const MappingTable & );

public:
    MappingTable( size_t addr_width,
                  const IntTab &level_addr_bits,
                  const IntTab &level_id_bits,
                  const addr_t cacheability_mask );
    
    void add( const soclib::common::Segment &seg );

    std::list<Segment> getSegmentList( const IntTab &index ) const;

    soclib::common::Segment getSegment( const IntTab &index ) const;

    AddressDecodingTable<addr_t, bool> getCacheabilityTable() const;

    AddressDecodingTable<addr_t, bool> getLocalityTable( const IntTab &index ) const;

    AddressDecodingTable<addr_t, int> getRoutingTable( const IntTab &index, int default_index = 0 ) const;

    AddressMaskingTable<uint32_t> getIdMaskingTable( const int level ) const;
    
    void print( std::ostream &o ) const;

    friend std::ostream &operator << (std::ostream &o, const MappingTable &mt)
    {
        mt.print(o);
        return o;
    }

    inline unsigned int indexForId( const IntTab &index ) const
    {
        return index*m_level_addr_bits;
    }
};

}}

#endif /* SOCLIB_MAPPING_TABLE_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

