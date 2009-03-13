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
    m_rt_size = 1<<(addr_width-m_level_addr_bits.sum());
    addr_t cm_rt_size = 1 << AddressMaskingTable<addr_t>(m_cacheability_mask).getDrop();
    m_rt_size = std::min<addr_t>(cm_rt_size, m_rt_size);
}

MappingTable::MappingTable( const MappingTable &ref )
        : m_segment_list(ref.m_segment_list),
          m_addr_width(ref.m_addr_width),
          m_level_addr_bits(ref.m_level_addr_bits),
          m_level_id_bits(ref.m_level_id_bits),
          m_cacheability_mask(ref.m_cacheability_mask),
          m_rt_size(ref.m_rt_size)
{
}

const MappingTable &MappingTable::operator=( const MappingTable &ref )
{
    m_segment_list = ref.m_segment_list;
    m_addr_width = ref.m_addr_width;
    m_level_addr_bits = ref.m_level_addr_bits;
    m_level_id_bits = ref.m_level_id_bits;
    m_cacheability_mask = ref.m_cacheability_mask;
    m_rt_size = ref.m_rt_size;
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
        for ( addr_t address = s.baseAddress() & ~(m_rt_size-1);
              (address < s.baseAddress()+s.size()) &&
                  (address >= (s.baseAddress() & ~(m_rt_size-1)));
              address += m_rt_size ) {
            for ( addr_t segaddress = seg.baseAddress() & ~(m_rt_size-1);
                  (segaddress < seg.baseAddress()+seg.size()) &&
                      (segaddress >= (seg.baseAddress() & ~(m_rt_size-1)));
                  segaddress += m_rt_size ) {
                if ( (m_cacheability_mask & address) == (m_cacheability_mask & segaddress) &&
                     s.cacheable() != seg.cacheable() ) {
                    std::ostringstream oss;
                    oss << "Segment " << s
                        << " has a different cacheability attribute with same MSBs than " << seg << std::endl;
                    throw soclib::exception::RunTimeError(oss.str());
                }
            }
        }
    }
    m_segment_list.push_back(seg);
}

const std::list<Segment> &
MappingTable::getAllSegmentList() const
{
    return m_segment_list;
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
    AddressDecodingTable<MappingTable::addr_t, bool> done(m_cacheability_mask);
	done.reset(false);

    std::list<Segment>::const_iterator i;
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
        for ( addr_t addr = i->baseAddress() & ~(m_rt_size-1);
              (addr < i->baseAddress()+i->size()) &&
                  (addr >= (i->baseAddress() & ~(m_rt_size-1)));
              addr += m_rt_size ) {
            if ( done[addr] && adt[addr] != i->cacheable() ) {
                std::ostringstream oss;
                oss << "Incoherent Mapping Table:" << std::endl
                    << "Segment " << *i << " has different cacheability than other segment with same masked address" << std::endl
                    << "Mapping table:" << std::endl
                    << *this;
                throw soclib::exception::RunTimeError(oss.str());
            }
            adt.set( addr, i->cacheable() );
            done.set( addr, true );
        }
    }
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
        for ( addr_t addr = i->baseAddress() & ~(m_rt_size-1);
              (addr < i->baseAddress()+i->size()) &&
                  (addr >= (i->baseAddress() & ~(m_rt_size-1)));
              addr += m_rt_size ) {
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
	}
    return adt;
}

AddressDecodingTable<MappingTable::addr_t, int>
MappingTable::getRoutingTable( const IntTab &index, int default_index ) const
{
#ifdef SOCLIB_MODULE_DEBUG
    std::cout << __FUNCTION__ << std::endl;
#endif
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
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << *i
                  << ", m_rt_size=" << m_rt_size
                  << ", m_rt_mask=" << ~(m_rt_size-1)
                  << std::endl;
#endif
        if ( ! i->index().idMatches(index) ) {
#ifdef SOCLIB_MODULE_DEBUG
			std::cout << i->index() << " does not match " << index << std::endl;
#endif
			continue;
		}

#ifdef SOCLIB_MODULE_DEBUG
        std::cout
            << ' ' << (i->baseAddress() & ~(m_rt_size-1))
            << ' ' << (i->baseAddress() + i->size())
            << ' ' << (((i->baseAddress() & ~(m_rt_size-1)) < i->baseAddress()+i->size()))
            << ' ' << (((i->baseAddress() & ~(m_rt_size-1)) >= (i->baseAddress() & ~(m_rt_size-1))))
            << std::endl;
#endif

        for ( addr_t addr = i->baseAddress() & ~(m_rt_size-1);
              (addr < i->baseAddress()+i->size()) &&
                  (addr >= (i->baseAddress() & ~(m_rt_size-1)));
              addr += m_rt_size ) {
            int val = i->index()[index.level()];
#ifdef SOCLIB_MODULE_DEBUG
			std::cout << addr << " -> " << val << std::endl;
#endif

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
        }
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << std::endl;
#endif
	}
    return adt;
}

MappingTable::addr_t *MappingTable::getCoherenceTable() const
{
    addr_t *ret = new addr_t[1<<m_level_id_bits.sum()];
    std::list<Segment>::const_iterator i;
    for ( i = m_segment_list.begin();
          i != m_segment_list.end();
          i++ ) {
        if( i->initiator() ) {
            int val = i->initiator_index()*m_level_id_bits;
            assert( val < (1<<m_level_id_bits.sum()) );
            ret[val] = i->baseAddress();
        }
    }
    return ret;
}

void MappingTable::print( std::ostream &o ) const
{
    std::list<Segment>::const_iterator i;

    o << "Mapping table: ad:" << m_level_addr_bits
      << " id:" << m_level_id_bits
      << " cacheability mask: " << std::hex << std::showbase << m_cacheability_mask
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

