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
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Based on previous works by Francois Pecheux & Alain Greiner
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_MAPPING_TABLE_H
#define SOCLIB_MAPPING_TABLE_H

#include <inttypes.h>
#include <list>
#include <sstream>

#include "segment.h"
#include "address_decoding_table.h"
#include "address_masking_table.h"
#include "int_tab.h"

namespace soclib { namespace common {

class MappingTable
{
public:
    typedef uint32_t addr_t;

private:
    std::list<soclib::common::Segment> m_segment_list;
    size_t m_addr_width;
    IntTab m_level_addr_bits;
    IntTab m_level_id_bits;
    addr_t m_cacheability_mask;
    addr_t m_rt_size;

public:
    MappingTable( const MappingTable& );
    const MappingTable &operator=( const MappingTable & );

    MappingTable( size_t addr_width,
                  const IntTab &level_addr_bits,
                  const IntTab &level_id_bits,
                  const addr_t cacheability_mask );
    
    void add( const soclib::common::Segment &seg );

    std::list<Segment> getSegmentList( const IntTab &index ) const;

    const std::list<Segment> &getAllSegmentList() const;

    soclib::common::Segment getSegment( const IntTab &index ) const;

    AddressDecodingTable<addr_t, bool> getCacheabilityTable() const;

    AddressDecodingTable<addr_t, bool> getLocalityTable( const IntTab &index ) const;

    AddressDecodingTable<addr_t, int> getRoutingTable( const IntTab &index, int default_index = 0 ) const;

    AddressDecodingTable<uint32_t, bool> getIdLocalityTable( const IntTab &index ) const;

    AddressMaskingTable<uint32_t> getIdMaskingTable( const int level ) const;
    
    addr_t *getCoherenceTable() const;

    void print( std::ostream &o ) const;

    friend std::ostream &operator << (std::ostream &o, const MappingTable &mt)
    {
        mt.print(o);
        return o;
    }

    inline unsigned int indexForId( const IntTab &index ) const
    {
        return index*m_level_id_bits;
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

