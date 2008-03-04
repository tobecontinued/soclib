/* -*- c++ -*-
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
 */

#include <cassert>
#include <sstream>
#include "mapping_table.h"

namespace soclib { namespace common {

MappingTable::MappingTable(
    size_t addr_width,
    const IntTab &level_addr_bits,
    const IntTab &level_id_bits,
    const addr_t cacheability_mask )
        : m_segment_list(), m_addr_width(addr_width),
          m_level_addr_bits(level_addr_bits),
          m_level_id_bits(level_id_bits),
          m_cacheability_mask(cacheability_mask)
{
}

MappingTable::MappingTable( const MappingTable &ref )
        : m_segment_list(ref.m_segment_list),
          m_addr_width(ref.m_addr_width),
          m_level_addr_bits(ref.m_level_addr_bits),
          m_level_id_bits(ref.m_level_id_bits),
          m_cacheability_mask(ref.m_cacheability_mask)
{
}

const MappingTable &MappingTable::operator=( const MappingTable &ref )
{
    m_segment_list = ref.m_segment_list;
    m_addr_width = ref.m_addr_width;
    m_level_addr_bits = ref.m_level_addr_bits;
    m_level_id_bits = ref.m_level_id_bits;
    m_cacheability_mask = ref.m_cacheability_mask;
    return *this;
}

void MappingTable::add( const Segment &seg )
{
    std::list<Segment>::iterator i;

    if ( seg.index().level() != m_level_addr_bits.level() ) {
        std::ostringstream o;
        o << seg << " is not the same level as the mapping table.";
        throw soclib::exception::ValueError(o.str());
    }

    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
        Segment &s = *i;
        if ( s.isOverlapping(seg) ) {
            std::ostringstream o;
            o << seg << " bumps in " << s;
            throw soclib::exception::Collision(o.str());
        }
        if ( m_cacheability_mask & s.baseAddress() == m_cacheability_mask & seg.baseAddress() &&
             s.cacheable() != seg.cacheable() ) {
            std::ostringstream oss;
            oss << "Segment " << s
                << " has a different cacheability attribute with same MSBs than " << seg << std::endl;
			throw soclib::exception::RunTimeError(oss.str());
        }
    }
    m_segment_list.push_back(seg);
}

std::list<Segment>
MappingTable::getSegmentList( const IntTab &index ) const
{
    std::list<Segment> ret;
    std::list<Segment>::const_iterator i;
    
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
        if ( i->index() == index )
            ret.push_back(*i);
    }
    return ret;
}

Segment
MappingTable::getSegment( const IntTab &index ) const
{
    std::list<Segment> list = getSegmentList(index);
    assert(list.size() == 1);
    return list.front();
}

AddressDecodingTable<MappingTable::addr_t, bool>
MappingTable::getCacheabilityTable() const
{
    AddressDecodingTable<MappingTable::addr_t, bool> adt(m_cacheability_mask);
	adt.reset(false);

    std::list<Segment>::const_iterator i;
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ )
		adt.set( i->baseAddress(), i->cacheable() );
    return adt;
}

AddressDecodingTable<MappingTable::addr_t, bool>
MappingTable::getLocalityTable( const IntTab &index ) const
{
	size_t nbits = m_level_addr_bits.sum(index.level());
    AddressDecodingTable<MappingTable::addr_t, bool> adt(nbits, m_addr_width-nbits);
	adt.reset(true);

    AddressDecodingTable<MappingTable::addr_t, bool> done(nbits, m_addr_width-nbits);
	done.reset(false);

    std::list<Segment>::const_iterator i;
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
		MappingTable::addr_t addr = i->baseAddress();
		bool val = (i->index().idMatches(index) );

		if ( done[addr] && adt[addr] != val ) {
            std::ostringstream oss;
            oss << "Incoherent Mapping Table:" << std::endl
                << "Segment " << *i << " targets different component than other segments with same MSBs" << std::endl
                << "Mapping table:" << std::endl
                << *this;
			throw soclib::exception::RunTimeError(oss.str());
        }
		adt.set( addr, val );
		done.set( addr, true );
	}
    return adt;
}

AddressDecodingTable<MappingTable::addr_t, int>
MappingTable::getRoutingTable( const IntTab &index, int default_index ) const
{
    std::cout << "Get routing table for " << index << " defaut target " << default_index << std::endl;
    std::cout << "Level for " << index << " " << index.level() << std::endl;

	size_t before = m_level_addr_bits.sum(index.level());
	size_t at = m_level_addr_bits[index.level()];
    AddressDecodingTable<MappingTable::addr_t, int> adt(at, m_addr_width-at-before);
	adt.reset(default_index);

    AddressDecodingTable<MappingTable::addr_t, bool> done(at, m_addr_width-at-before);
	done.reset(false);

    std::list<Segment>::const_iterator i;
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
        if ( ! i->index().idMatches(index) ) {
			continue;
		}

		MappingTable::addr_t addr = i->baseAddress();
		int val = i->index()[index.level()];
        
        std::cout << " -> " << *i << " dest: " << val << std::endl;

		if ( done[addr] && adt[addr] != val ) {
            std::ostringstream oss;
            oss << "Incoherent Mapping Table: for " << index << std::endl
                << "Segment " << *i << " targets different target (or cluster) than other segments with same routing bits" << std::endl
                << "Mapping table:" << std::endl
                << *this;
			throw soclib::exception::RunTimeError(oss.str());
        }
		adt.set( addr, val );
		done.set( addr, true );

        std::cout << adt << std::endl;

	}
    std::cout << index << adt << std::endl;
    return adt;
}

void MappingTable::print( std::ostream &o ) const
{
    std::list<Segment>::const_iterator i;

    o << "Mapping table: ad:" << m_level_addr_bits
      << " id:" << m_level_id_bits
      << std::endl;
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
        o << " " << (*i) << std::endl;
    }
}

AddressMaskingTable<uint32_t>
MappingTable::getIdMaskingTable( const int level ) const
{
    int use = m_level_id_bits[level];
    int drop = 0;
    for ( size_t i=level+1; i<m_level_id_bits.level(); ++i )
        drop += m_level_id_bits[i];
    return AddressMaskingTable<uint32_t>( use, drop );
}

AddressDecodingTable<uint32_t, bool>
MappingTable::getIdLocalityTable( const IntTab &index ) const
{
	size_t nbits = m_level_id_bits.sum(index.level());
    size_t id_width = m_level_id_bits.sum();

    AddressDecodingTable<uint32_t, bool> adt(nbits, id_width-nbits);
	adt.reset(true);

    AddressDecodingTable<uint32_t, bool> done(nbits, id_width-nbits);
	done.reset(false);

    std::list<Segment>::const_iterator i;
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
		uint32_t id = indexForId(i->index());
		bool val = i->index().idMatches(index);

		if ( done[id] && adt[id] != val ) {
            std::ostringstream oss;
            oss << "Incoherent Mapping Table:" << std::endl
                << "Segment " << *i << " targets different component than other segments with same ID MSBs" << std::endl
                << "Mapping table:" << std::endl
                << *this;
			throw soclib::exception::RunTimeError(oss.str());
        }
		adt.set( id, val );
		done.set( id, true );
	}
    return adt;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

