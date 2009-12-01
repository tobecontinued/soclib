/*
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
 * Copyright (c) UPMC, Lip6
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 *
 * Maintainers: nipo becoulet
 */

#include <cstring>

#include <cassert>

#include <stdint.h>
#include <signal.h>

#include "iss_memchecker_registers.h"
#include "iss_memchecker.h"
#include "exception.h"

#include "soclib_endian.h"
#include "loader.h"

namespace soclib { namespace common {

namespace __iss_memchecker {

typedef uint32_t error_level_t;

enum {
    ERROR_NONE = 0,
    ERROR_UNINITIALIZED_WORD = 1,
    ERROR_INVALID_REGION = 2,
    ERROR_SP_OUTOFBOUNDS = 4,
    ERROR_FP_OUTOFBOUNDS = 8,
    ERROR_DATA_ACCESS_BELOW_SP = 16,
    ERROR_CREATING_STACK_NOT_ALLOC = 32,
    ERROR_BAD_REGION_REALLOCATION = 64,
    ERROR_INVALID_MAGIC_DISABLE = 128,
    ERROR_CONTEXT_ON_TWO_CPUS = 256,
};

class ContextState
{
    const uint32_t m_id;
    uint32_t m_running_on;
    static const uint32_t s_not_running = (uint32_t)-1;
    uint32_t m_refcount;

public:
    const uint32_t m_stack_lower;
    const uint32_t m_stack_upper;

    ContextState( uint32_t id, uint32_t stack_low, uint32_t stack_up )
        : m_id(id),
          m_running_on(s_not_running),
          m_refcount(0),
          m_stack_lower(stack_low),
          m_stack_upper(stack_up)
    {
        assert(m_stack_lower <= m_stack_upper && "Stack upside down");
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << "Creating new context " << *this << std::endl;
#endif
    }

    ~ContextState()
    {
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << "Deleting context " << *this << std::endl;
#endif
    }

    inline bool stack_contains( uint32_t sp ) const
    {
        return (m_stack_lower <= sp && sp < m_stack_upper);
    }

    inline bool is( uint32_t id ) const
    {
        return m_id == id;
    }

    inline uint32_t id() const
    {
        return m_id;
    }

    inline error_level_t schedule( uint32_t cpu )
    {
        error_level_t r = 0;
        if ( m_running_on != s_not_running )
            r |= ERROR_CONTEXT_ON_TWO_CPUS;
        m_running_on = cpu;
        return r;
    }

    inline void unschedule()
    {
        m_running_on = s_not_running;
    }

    bool overlaps( ContextState &other ) const
    {
        return overlaps(
            other.m_stack_lower,
            other.m_stack_upper );
    }

    bool overlaps( uint32_t base, uint32_t end ) const
    {
        if ( end <= m_stack_lower )
            return false;
        if ( m_stack_upper <= base )
            return false;
        return true;
    }

    void print( std::ostream &o ) const
    {
        o << "<Context "
          << std::hex << std::showbase << m_id
          << " " << m_stack_lower
          << "->" << m_stack_upper
          << ">";
    }

    friend std::ostream &operator << (std::ostream &o, const ContextState &cs)
    {
        cs.print(o);
        return o;
    }

    void ref()
    {
        ++m_refcount;
    }
    void unref()
    {
        if ( --m_refcount == 0 ) {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << *this << " has not more refs, del" << std::endl;
#endif
            delete this;
        }
    }
};

class RegionInfo
{
public:
    enum State {
        REGION_INVALID = 1,
        REGION_STATE_GLOBAL = 2,
        REGION_STATE_GLOBAL_READ_ONLY = 4,
        REGION_STATE_ALLOCATED = 8,
        REGION_STATE_FREE = 16,
        REGION_STATE_STACK = 32,
        REGION_STATE_WAS_STACK = 64,
        REGION_STATE_PERIPHERAL = 128,
        REGION_STATE_RAW = 256,
    };

private:
    enum State m_state;
    uint32_t m_at;
    uint32_t m_refcount;
    uint32_t m_base_addr;
    uint32_t m_end_addr;
    RegionInfo *m_previous_state;

public:
    RegionInfo *get_updated_region( enum State state, uint32_t at, uint32_t base_addr, uint32_t end_addr )
    {
        RegionInfo *n = new RegionInfo( state, at, base_addr, end_addr, this );
        return n;
    }

    RegionInfo( enum State state, uint32_t at, uint32_t base_addr, uint32_t end_addr, RegionInfo *previous_state = 0 )
        : m_state(state),
          m_at(at),
          m_refcount(0),
          m_base_addr(base_addr),
          m_end_addr(end_addr),
          m_previous_state(previous_state)
    {
        if ( m_previous_state )
            m_previous_state->ref();
    }

    ~RegionInfo()
    {
        if ( m_previous_state )
            m_previous_state->unref();
    }

    bool new_state_valid( enum State new_state )
    {
        int valid_new_states = 0;
        switch(m_state) {
        case REGION_INVALID:
            valid_new_states = 0;
            break;
        case REGION_STATE_GLOBAL:
            valid_new_states =
                REGION_STATE_FREE |
                REGION_STATE_STACK;
            break;
        case REGION_STATE_GLOBAL_READ_ONLY:
            valid_new_states = 0;
            break;
        case REGION_STATE_ALLOCATED:
            valid_new_states =
                REGION_STATE_FREE |
                REGION_STATE_STACK;
            break;
        case REGION_STATE_FREE:
            valid_new_states =
                REGION_STATE_FREE |
                REGION_STATE_ALLOCATED;
            break;
        case REGION_STATE_PERIPHERAL:
            valid_new_states = 0;
            break;
        case REGION_STATE_STACK:
            valid_new_states =
                REGION_STATE_WAS_STACK;
            break;
        case REGION_STATE_WAS_STACK:
            valid_new_states =
                REGION_STATE_FREE |
                REGION_STATE_STACK;
            break;
        case REGION_STATE_RAW:
            valid_new_states =
                REGION_STATE_PERIPHERAL |
                REGION_STATE_FREE;
            break;
        }
        return valid_new_states & new_state;
    }

    bool contains( uint32_t addr ) const
    {
        return m_base_addr <= addr && addr < m_end_addr;
    }

    State state() const
    {
        return m_state;
    }

    void ref()
    {
        ++m_refcount;
    }
    void unref()
    {
        if ( --m_refcount == 0 )
            delete this;
    }

    error_level_t do_write() const
    {
        if ( m_state == REGION_STATE_GLOBAL_READ_ONLY )
            return ERROR_INVALID_REGION;
        return do_read();
    }

    error_level_t do_read() const
    {
        if ( m_state & 
             ( REGION_STATE_FREE
               | REGION_INVALID
               | REGION_STATE_WAS_STACK )
            )
            return ERROR_INVALID_REGION;
        return ERROR_NONE;
    }

    static const char *state_str(State state)
    {
        switch (state) {
        case REGION_INVALID: return "inval";
        case REGION_STATE_GLOBAL: return "globl";
        case REGION_STATE_GLOBAL_READ_ONLY: return "gblRO";
        case REGION_STATE_ALLOCATED: return "alloc";
        case REGION_STATE_FREE: return "free ";
        case REGION_STATE_PERIPHERAL: return "peri.";
        case REGION_STATE_STACK: return "stack";
        case REGION_STATE_WAS_STACK: return "!stak";
        case REGION_STATE_RAW: return "raw  ";
        default: return "uknwn";
        }
    }

    void print( std::ostream &o ) const
    {
        o << "[" << std::hex
//          << "at "  << m_at
          << "@"  << m_base_addr
          << "-"  << m_end_addr
          << ", "  << state_str(m_state);
        if ( m_previous_state )
            o << " was: " << *m_previous_state;
        o << "]";
    }

    friend std::ostream &operator << (std::ostream &o, const RegionInfo &ri)
    {
        ri.print(o);
        return o;
    }
};

class AddressInfo
{
    RegionInfo *m_info;
    static const uintptr_t s_initialized_bit = 1;
    static const uintptr_t s_reserved_bit = 2;
    static const uintptr_t s_addr_mask = ~(uintptr_t)3;

    bool is_initialized() const
    {
        return (uintptr_t)m_info & s_initialized_bit;
    }

public:

    void set_initialized( bool initialized )
    {
        if ( initialized )
            m_info = (RegionInfo*)((uintptr_t)m_info | s_initialized_bit);
        else
            m_info = (RegionInfo*)((uintptr_t)m_info & ~s_initialized_bit);
    }

    RegionInfo *region() const
    {
        return (RegionInfo*)((uintptr_t)m_info & s_addr_mask);
    }

    error_level_t region_set( RegionInfo *ptr )
    {
        error_level_t r = 0;
        assert(ptr);
        ptr->ref();

        if ( region() ) {
            if ( ! region()->new_state_valid(ptr->state()) )
                r |= ERROR_BAD_REGION_REALLOCATION;
            region()->unref();
        }
        m_info = (RegionInfo*)(
            (uintptr_t)ptr & s_addr_mask |
            (uintptr_t)m_info & ~s_addr_mask);
        return r;
    }

    AddressInfo & operator=( const AddressInfo &ref )
    {
        if ( &ref == this )
            return *this;
        region_set(ref.region());
        set_initialized(false);
    }
    
    AddressInfo( RegionInfo *ri, bool initialized = false )
        : m_info(0)
    {
        region_set(ri);
        set_initialized(initialized);
    }

    AddressInfo()
        : m_info(0)
    {}
    
    ~AddressInfo()
    {
        if ( region() )
            region()->unref();
    }

    AddressInfo( const AddressInfo &ref )
        : m_info(0)
    {
        if ( ref.region() )
            region_set(ref.region());
    }

    error_level_t do_write()
    {
        set_initialized(true);
        return ERROR_NONE;
    }

    error_level_t do_read()
    {
        if ( ! is_initialized() )
            return ERROR_UNINITIALIZED_WORD;
        set_initialized(true);
        return ERROR_NONE;
    }

    void print( std::ostream &o ) const
    {
        o << "<AddressInfo " << std::dec
          << (is_initialized() ? " initialized" : " uninitialized");
        if ( region() )
            o << ", " << *region();
        else
            o << ", no region";
        o << ">";
    }

    friend std::ostream &operator << (std::ostream &o, const AddressInfo &ai)
    {
        ai.print(o);
        return o;
    }
};

class MemoryState
{
    Loader m_binary;

    typedef std::map<uint32_t, ContextState *> context_map_t;
    typedef std::map<uint32_t, std::vector<AddressInfo> *> region_map_t;
    context_map_t m_contexts;
    region_map_t m_regions;
    AddressInfo m_default_address;
    uintptr_t m_comm_address;

public:
    ContextState * const unknown_context;

    MemoryState( const soclib::common::MappingTable &mt,
                 const soclib::common::Loader &loader,
                 const std::string &exclusions )
        : m_binary(loader),
          m_contexts(),
          m_regions(),
          m_default_address(new RegionInfo(RegionInfo::REGION_INVALID, 0, 0, 0), true),
          m_comm_address(0x4200),
          unknown_context(new ContextState((uint32_t)-1, 0, 0 )) //(uint32_t)-1 ))
    {
        unknown_context->ref();

        const std::list<Segment> &segments = mt.getAllSegmentList();
        Loader::section_list_t sections = loader.sections();

        std::string exclusion_list = ",";
        exclusion_list += exclusions + ",";

        for ( std::list<Segment>::const_iterator i = segments.begin();
              i != segments.end();
              ++i ) {

            RegionInfo *ri = new RegionInfo(
                RegionInfo::REGION_STATE_RAW, 0,
                i->baseAddress(), i->baseAddress() + i->size() );
            std::vector<AddressInfo> *rm = new std::vector<AddressInfo>( i->size() / 4 );

            for ( size_t j=0; j<i->size()/4; ++j )
                (*rm)[j].region_set(ri);

            m_regions[i->baseAddress()] = rm;
        }

        for ( Loader::section_list_t::const_iterator i = sections.begin();
              i != sections.end();
              ++i ) {

#ifdef SOCLIB_MODULE_DEBUG
//              std::cout << "Creating a region info for"
//                        << " " << i->name()
//                        << " @" << std::hex << i->lma()
//                        << ", " << std::dec << i->size() << " bytes long"
//                        << " flags: " << (i->flag_read_only() ? "RO" : "")
//                        << std::endl;
#endif

            RegionInfo::State state = RegionInfo::REGION_STATE_GLOBAL;
            if ( i->flag_read_only() )
                state = RegionInfo::REGION_STATE_GLOBAL_READ_ONLY;

            region_new_state( state, 0, i->lma(), i->size() );
//            if ( i->has_data() )
            for ( size_t j=0; j<i->size(); j+=4 )
                    info_for_address( i->lma()+j )->do_write();
        }

        for ( std::list<Segment>::const_iterator i = segments.begin();
              i != segments.end();
              ++i ) {
            std::string vname = std::string(",")+i->name()+",";

            if ( exclusion_list.find(vname) == std::string::npos )
                continue;

            region_update_state( RegionInfo::REGION_STATE_PERIPHERAL, 0, i->baseAddress(), i->size() );
            for ( size_t j=0; j<i->size(); j+=4 )
                info_for_address( i->baseAddress()+j )->do_write();
        }

        const BinaryFileSymbol *sym = loader.get_symbol_by_name( "soclib_iss_memchecker_addr" );
        if ( sym ) {
            m_comm_address = sym->address();
            std::cout << "Binary file defined IssMemchecker communication address to "
                      << m_comm_address << std::endl;
        }
    }

    uintptr_t comm_address() const
    {
        return m_comm_address;
    }

    ContextState *context_get( uint32_t id ) const
    {
        context_map_t::const_iterator i = m_contexts.find(id);
        if ( i != m_contexts.end() )
            return i->second;
        else
            return unknown_context;
    }

    void context_create( uint32_t id, ContextState *context )
    {
#ifdef SOCLIB_MODULE_DEBUG
        std::cout << "Creating " << *context << std::endl;
#endif

        context_map_t::const_iterator i = m_contexts.find(id);
        assert(i == m_contexts.end()
               && "Creating two contexts with the same id...");
        for ( context_map_t::const_iterator i = m_contexts.begin();
              i != m_contexts.end();
              ++i ) {
#ifdef SOCLIB_MODULE_DEBUG
            std::cout << "Checking " << *context << " and " << *i->second << std::endl;
#endif
            if ( context->overlaps( *i->second ) ) {
                std::cout << "Context " << *context << " overlaps " << *i->second << std::endl;
                abort();
            }
        }
        m_contexts[id] = context;
        context->ref();
    }

    void context_delete( uint32_t id )
    {
        context_map_t::iterator i = m_contexts.find(id);

#ifdef SOCLIB_MODULE_DEBUG
         if (i == m_contexts.end()) {
             std::cout << "Trying to delete context " << id << std::endl;
             return;
         }
#endif
        assert(i != m_contexts.end()
               && "Deleting non-existant context...");
        i->second->unref();
        m_contexts.erase(i);
    }

    AddressInfo *info_for_address(uint32_t address)
    {
        region_map_t::iterator i = m_regions.upper_bound(address);
        while ( i != m_regions.begin()
             && i->first > address )
            --i;

        if ( i == m_regions.end() ) {
#ifdef SOCLIB_MODULE_DEBUG
             std::cout
                 << "Address " << std::hex << address << " in no region." << std::endl
                 << "Regions: " << std::endl;
             for ( region_map_t::iterator i = m_regions.begin();
                   i != m_regions.end();
                   ++i )
                 std::cout << " " << i->first << " size: " << i->second->size() << " words" << std::endl;
#endif
            
            //abort();
            return &m_default_address;
        }
        uint32_t region_base = i->first;
        uint32_t word_no = (address-region_base)/4;
        std::vector<AddressInfo> &r = *(i->second);
        if ( region_base <= address && word_no < r.size() )
            return &r[word_no];
#ifdef SOCLIB_MODULE_DEBUG
         std::cout << "Warning: address " << std::hex << address
                   << " " << std::dec << (r.size()-word_no) << " words beyond "
                   << r[r.size()-1] << std::endl;
#endif

        return &m_default_address;
    }

    error_level_t region_update_state( RegionInfo::State new_state, uint32_t at, uint32_t addr, uint32_t size )
    {
        error_level_t r = 0;
        RegionInfo *lri = info_for_address(addr)->region();
        RegionInfo *nri = lri->get_updated_region( new_state, at, addr, addr+size );

#ifdef SOCLIB_MODULE_DEBUG
        std::cout << "Updating " << *nri << std::endl;
#endif

        for ( context_map_t::const_iterator i = m_contexts.begin();
              i != m_contexts.end();
              ++i ) {
            if ( i->second->overlaps( addr, addr+size ) ) {
                std::cout
                    << "Region " << *nri
                    << " overlaps " << *(i->second)
                    << std::endl;
                r |= ERROR_INVALID_REGION;
            }
        }

        for ( uint32_t a = addr; a < addr+size; a+=4 )
            r |= info_for_address(a)->region_set(nri);
        return r;
    }

    void region_new_state( RegionInfo::State new_state, uint32_t at, uint32_t addr, uint32_t size )
    {
        RegionInfo *nri = new RegionInfo( new_state, at, addr, addr+size );
        for ( uint32_t a = addr; a < addr+size; a+=4 )
            info_for_address(a)->region_set(nri);
    }
 
    BinaryFileSymbolOffset get_symbol( uintptr_t addr ) const
    {
        return m_binary.get_symbol_by_addr(addr);
    }
};

} // namespace __iss_memchecker

using namespace __iss_memchecker;


template<typename iss_t>
uint32_t IssMemchecker<iss_t>::get_cpu_sp() const
{
    return iss_t::debugGetRegisterValue(iss_t::s_sp_register_no);
}

template<typename iss_t>
uint32_t IssMemchecker<iss_t>::get_cpu_fp() const
{
    return iss_t::debugGetRegisterValue(iss_t::s_fp_register_no);
}

template<typename iss_t>
uint32_t IssMemchecker<iss_t>::get_cpu_pc() const
{
    return iss_t::debugGetRegisterValue(iss_t::s_pc_register_no);
}


template<typename iss_t>
void IssMemchecker<iss_t>::init( const soclib::common::MappingTable &mt,
                                 const soclib::common::Loader &loader,
                                 const std::string &exclusions )
{
    if ( s_memory_state == NULL )
        s_memory_state = new MemoryState( mt, loader, exclusions );
}

template<typename iss_t>
IssMemchecker<iss_t>::IssMemchecker(const std::string &name, uint32_t ident)
    : iss_t(name, ident),
      m_last_region_touched(0),
      m_has_data_answer(false),
      m_cpuid(ident),
      m_enabled_checks(0),
      m_r1(0),
      m_r2(0),
      m_last_sp(0),
      m_magic_state(MAGIC_NONE)
{
    struct iss_t::DataRequest init = ISS_DREQ_INITIALIZER;
    m_last_data_access = init;

    if ( !s_memory_state ) {
        std::cerr
            << std::endl
            << "You must call the static initialized with:" << std::endl
            << "soclib::common::IssMemchecker<...>::init( mapping_table, loader );" << std::endl
            << "Prior to any IssMemchecker constructor." << std::endl
            << std::endl;
        abort();
    }

    m_comm_address = s_memory_state->comm_address();

    m_current_context = s_memory_state->unknown_context;
    m_last_context = s_memory_state->unknown_context;
    m_current_context->ref();
    m_last_context->ref();
}

template<typename iss_t>
uint32_t IssMemchecker<iss_t>::register_get(uint32_t reg_no) const
{
    assert( reg_no < ISS_MEMCHECKER_REGISTER_MAX && "Undefined regsiter" );

    switch ((enum SoclibIssMemcheckerRegisters)reg_no) {
    case ISS_MEMCHECKER_CONTEXT_CURRENT:
        return m_current_context->id();
    case ISS_MEMCHECKER_R1:
        return m_current_context->m_stack_lower;
    case ISS_MEMCHECKER_R2:
        return m_current_context->m_stack_upper;
    default:
        assert(!"This register is write only");
        return 0;
    }
}

#define ISS_MEMCHECKER_MAGIC_VAL_SWAPPED                               \
    (((ISS_MEMCHECKER_MAGIC_VAL << 24) & 0xff000000) |                 \
     ((ISS_MEMCHECKER_MAGIC_VAL <<  8) & 0x00ff0000) |                 \
     ((ISS_MEMCHECKER_MAGIC_VAL >>  8) & 0x0000ff00) |                 \
     ((ISS_MEMCHECKER_MAGIC_VAL >> 24) & 0x000000ff))

template<typename iss_t>
void IssMemchecker<iss_t>::register_set(uint32_t reg_no, uint32_t value)
{
    assert( reg_no < ISS_MEMCHECKER_REGISTER_MAX && "Undefined regsiter" );

#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << "memchecker register set " << std::dec << reg_no
        << " val: " << std::hex << value
        << std::endl;
#endif

    switch ((enum SoclibIssMemcheckerRegisters)reg_no) {
    case ISS_MEMCHECKER_MAGIC:
        switch (m_magic_state) {
        case MAGIC_NONE:
            switch (value) {
            case ISS_MEMCHECKER_MAGIC_VAL:
                m_magic_state = MAGIC_LE;
                break;
            case ISS_MEMCHECKER_MAGIC_VAL_SWAPPED:
                m_magic_state = MAGIC_BE;
                break;
            default:
                std::cout << "Received magic " << std::hex << value << std::endl;
                assert(!"Wrong magic");
            }
            break;
        default:
            if ( value != 0 )
                report_error(ERROR_INVALID_MAGIC_DISABLE);
            m_magic_state = MAGIC_NONE;
            break;
        }
        break;
	case ISS_MEMCHECKER_R1:
        m_r1 = value;
        break;
	case ISS_MEMCHECKER_R2:
        m_r2 = value;
        break;
	case ISS_MEMCHECKER_CONTEXT_ID_CREATE:
    {
        s_memory_state->context_create(
            value, new ContextState(
                value, m_r1, m_r1+m_r2 ) );
        bool err = false;
        for ( uint32_t addr = m_r1;
              addr < m_r1+m_r2;
              addr+= 4 ) {
            AddressInfo *ai = s_memory_state->info_for_address(addr);
            err |= ! ( ai->region()->state() & (
                         __iss_memchecker::RegionInfo::REGION_STATE_ALLOCATED
                         | __iss_memchecker::RegionInfo::REGION_STATE_GLOBAL
                         | __iss_memchecker::RegionInfo::REGION_STATE_STACK
                         ) );
            ai->set_initialized(false);
        }
        if ( (m_enabled_checks & ISS_MEMCHECKER_CHECK_REGION) && err)
            report_error(ERROR_CREATING_STACK_NOT_ALLOC);
        break;
    }
	case ISS_MEMCHECKER_CONTEXT_ID_CHANGE:
    {
        ContextState *ref = s_memory_state->context_get( m_r1 );
        ContextState *n = new ContextState(
            value, ref->m_stack_lower, ref->m_stack_upper );
        s_memory_state->context_delete(m_r1);
        s_memory_state->context_create( value, n );
        if ( m_current_context->is( m_r1 ) ) {
            update_context(n);
        }
        break;
    }
	case ISS_MEMCHECKER_CONTEXT_ID_DELETE:
        if ( m_current_context->is( value ) ) {
            update_context(s_memory_state->unknown_context);
        }
        s_memory_state->context_delete(value);
        break;
	case ISS_MEMCHECKER_CONTEXT_CURRENT:
        update_context(s_memory_state->context_get( value ));
        break;
	case ISS_MEMCHECKER_MEMORY_REGION_UPDATE:
    {
        __iss_memchecker::RegionInfo::State state;
        switch (value) {
        case ISS_MEMCHECKER_REGION_FREE:
            state = __iss_memchecker::RegionInfo::REGION_STATE_FREE;
            break;
        case ISS_MEMCHECKER_REGION_ALLOC:
            state = __iss_memchecker::RegionInfo::REGION_STATE_ALLOCATED;
            break;
        case ISS_MEMCHECKER_REGION_NONALLOC_STACK:
            state = __iss_memchecker::RegionInfo::REGION_STATE_STACK;
            break;
        default:
            assert(!"Invalid region state");
        }
        int error = 0;
        error |= s_memory_state->region_update_state(
            state,
            get_cpu_pc(),
            m_r1, m_r2 );
        m_last_region_touched = s_memory_state->info_for_address(m_r1)->region();
        report_error(error);
        break;
    }
	case ISS_MEMCHECKER_ENABLE_CHECKS:
        m_enabled_checks |= value;
        break;
	case ISS_MEMCHECKER_DISABLE_CHECKS:
        m_enabled_checks &= ~value;
        break;
    default:
        assert(!"Unknown register");
        break;
    }
}

template<typename iss_t>
void IssMemchecker<iss_t>::update_context( ContextState *state )
{
#ifdef SOCLIB_MODULE_DEBUG
     std::cout << iss_t::m_name
               << " switching from " << *m_current_context
               << " to " << *state << std::endl;
#endif

#if 1
    m_last_context->unref();
    m_last_context = m_current_context;
#else
    m_current_context->unref();
#endif

    m_current_context->unschedule();
    m_current_context = state;
    m_current_context->schedule(m_cpuid);
    m_current_context->ref();
}

template<typename iss_t>
void IssMemchecker<iss_t>::handle_comm( const struct iss_t::DataRequest &dreq )
{
    uint32_t reg_no = (dreq.addr-m_comm_address)/4;
    assert( dreq.be == 0xf && "Only read/write word are allowed in memchecker area" );

    switch ( dreq.type ) {
    case iss_t::DATA_READ:
        m_data_answer_value = register_get(reg_no);
        if ( m_magic_state == MAGIC_BE )
            m_data_answer_value = soclib::endian::uint32_swap(m_data_answer_value);
        break;
    case iss_t::DATA_WRITE: {
        uint32_t data = dreq.wdata;
        m_data_answer_value = 0;
        if ( m_magic_state == MAGIC_BE )
            data = soclib::endian::uint32_swap(data);
        register_set(reg_no, data);
        break;
    }
    case iss_t::XTN_WRITE:
    case iss_t::XTN_READ:
    case iss_t::DATA_LL:
    case iss_t::DATA_SC:
        assert(!"Only read & write allowed in memchecker area");
        return;
    }
    m_has_data_answer = true;
}

template<typename iss_t>
void IssMemchecker<iss_t>::check_data_access( const struct iss_t::DataRequest &dreq )
{
    error_level_t err = ERROR_NONE;
    AddressInfo *ai = s_memory_state->info_for_address(dreq.addr);

    m_last_data_access = dreq;

    switch ( dreq.type ) {
    case iss_t::DATA_READ:
    case iss_t::DATA_LL:
        if ( m_enabled_checks & ISS_MEMCHECKER_CHECK_INIT )
            err |= ai->do_read();
        break;
    case iss_t::DATA_SC:
    case iss_t::DATA_WRITE:
        err |= ai->do_write();
        break;
    case iss_t::XTN_WRITE:
    case iss_t::XTN_READ:
        return;
    }

    if ( m_current_context->stack_contains(dreq.addr) ) {
        if ( ( m_enabled_checks & ISS_MEMCHECKER_CHECK_SP )
//             && m_current_context != s_memory_state->unknown_context
             && dreq.addr < (get_cpu_sp() - 64) ) {
            err |= ERROR_DATA_ACCESS_BELOW_SP;
        }
    } else {
        if ( m_enabled_checks & ISS_MEMCHECKER_CHECK_REGION ) {
            RegionInfo *ri = ai->region();
            switch ( dreq.type ) {
            case iss_t::DATA_READ:
            case iss_t::DATA_LL:
                err |= ri->do_read();
                break;
            case iss_t::DATA_SC:
            case iss_t::DATA_WRITE:
                err |= ri->do_write();
                break;
            case iss_t::XTN_WRITE:
            case iss_t::XTN_READ:
                return;
            }
        }
    }

    report_error(err);
}

template<typename iss_t>
void IssMemchecker<iss_t>::report_error(error_level_t errors)
{
    if ( !errors )
        return;

    uint32_t pc = get_cpu_pc();
    uint32_t sp = get_cpu_sp();

    // Signal to GDB
    iss_t::debugExceptionBypassed( iss_t::EXCL_TRAP );



        
    std::cout
        << std::endl
        << std::endl
        << iss_t::m_name << " error:" << std::endl
        << " " << *m_current_context << std::endl;

    AddressInfo *ai = s_memory_state->info_for_address(m_last_data_access.addr);

    if ( errors & ERROR_UNINITIALIZED_WORD )
        std::cout << " access to uninitialized word" << std::endl;
    if ( errors & ERROR_INVALID_REGION )
        std::cout << " access to invalid region: " << *(ai->region()) << std::endl;
    if ( errors & ERROR_CREATING_STACK_NOT_ALLOC )
        std::cout << " creating stack in non-allocated memory" << std::endl;
    if ( errors & ERROR_BAD_REGION_REALLOCATION )
        std::cout << " bad reallocation of region" << std::endl;
    if ( errors & ERROR_CONTEXT_ON_TWO_CPUS )
        std::cout << " context running on two cpus" << std::endl;
    if ( errors & ERROR_INVALID_MAGIC_DISABLE )
        std::cout << " bad disabling of magic" << std::endl;
    if ( errors & ERROR_SP_OUTOFBOUNDS ) {
        std::cout << " stack pointer out of bounds: " << sp << ", ";
        if ( sp < m_current_context->m_stack_lower )
            std::cout << (m_current_context->m_stack_lower - sp) << " bytes below" << std::endl;
        else if ( sp > m_current_context->m_stack_upper )
            std::cout << (sp - m_current_context->m_stack_upper) << " bytes above" << std::endl;
    }
    if ( errors & ERROR_FP_OUTOFBOUNDS ) {
        uint32_t fp = get_cpu_fp();
        std::cout << " frame pointer out of bounds: " << fp << ", ";
        if ( fp < m_current_context->m_stack_lower )
            std::cout << (m_current_context->m_stack_lower - fp) << " bytes below" << std::endl;
        else if ( fp > m_current_context->m_stack_upper )
            std::cout << (fp - m_current_context->m_stack_upper) << " bytes above" << std::endl;
    }
    if ( errors & ERROR_DATA_ACCESS_BELOW_SP )
        std::cout << " data access below SP" << std::endl;
    if ( errors & (ERROR_FP_OUTOFBOUNDS | ERROR_SP_OUTOFBOUNDS | ERROR_DATA_ACCESS_BELOW_SP) ) {
        std::cout
            << " stack range: " << m_current_context->m_stack_lower
            << "->" << m_current_context->m_stack_upper
            << " (" << *(s_memory_state->info_for_address(m_current_context->m_stack_lower)->region()) << ')'
            << std::endl;
    }

    if ( (errors & (ERROR_BAD_REGION_REALLOCATION | ERROR_INVALID_REGION)) && m_last_region_touched )
        std::cout << " region: " << *m_last_region_touched << std::endl;


    std::cout
        << " at PC=" << s_memory_state->get_symbol(pc) << std::endl

        << "    SP=" << s_memory_state->get_symbol(sp) << std::endl

        << "    last Dreq: " << m_last_data_access << std::endl
        << "          sym: " << s_memory_state->get_symbol(m_last_data_access.addr) << std::endl
        << "           in: " << *(s_memory_state->info_for_address(m_last_data_access.addr)->region()) << std::endl;

    iss_t::dump();
}

template<typename iss_t>
uint32_t IssMemchecker<iss_t>::executeNCycles(
    uint32_t ncycle, struct iss_t::InstructionResponse irsp,
    struct iss_t::DataResponse drsp, uint32_t irq_bit_field )
{
    struct iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    struct iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    iss_t::getRequests(ireq, dreq);

    if ( dreq.valid &&
         ( (dreq.addr & ~(uint32_t)0xff) == m_comm_address ) ) {
        if ( ! ireq.valid || irsp.valid )
            handle_comm( dreq );
        dreq.valid = false;
    }

    if ( m_has_data_answer ) {
        assert( !drsp.valid && "Cache speaking while i'm answering ISS" );
        drsp.valid = true;
        drsp.error = false;
        drsp.rdata = m_data_answer_value;
        m_has_data_answer = false;
    } else {
        if ( drsp.valid ) {
            check_data_access( dreq );
        }
    }

//     std::cout << iss_t::m_name << ", " << ireq << irsp
//               << std::endl
//               << "        " << dreq << drsp << ' ' << ncycle << std::endl;

    uint32_t nc = iss_t::executeNCycles( ncycle, irsp, drsp, irq_bit_field );

    {
        uint32_t sp = get_cpu_sp();
        if ( m_last_sp && m_last_sp < sp ) {
            for ( uint32_t i = m_last_sp; i < sp; ++i )
                s_memory_state->info_for_address(i)->set_initialized(false);
            m_last_sp = sp;
        }
    }
    
    if ( m_magic_state == MAGIC_NONE ) {
        error_level_t err = ERROR_NONE;
        if ( (m_enabled_checks & ISS_MEMCHECKER_CHECK_SP) &&
             ! m_current_context->stack_contains(get_cpu_sp()) )
            err |= ERROR_SP_OUTOFBOUNDS;
        if ( (m_enabled_checks & ISS_MEMCHECKER_CHECK_FP) &&
             ! m_current_context->stack_contains(get_cpu_fp()) )
            err |= ERROR_FP_OUTOFBOUNDS;
        report_error( err );
    }
    return nc;
}

template<typename iss_t> __iss_memchecker::MemoryState* IssMemchecker<iss_t>::s_memory_state = NULL;

}}


// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
