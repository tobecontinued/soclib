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

#include <stdint.h>
#include <signal.h>

#include "iss_memchecker_registers.h"
#include "iss_memchecker.h"
#include "exception.h"

#include "soclib_endian.h"
#include "elf_loader.h"

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
};

class ContextState
{
    uint32_t m_id;

public:
    const uint32_t m_stack_lower;
    const uint32_t m_stack_upper;

    ContextState( uint32_t id, uint32_t stack_low, uint32_t stack_up )
        : m_id(id),
          m_stack_lower(stack_low),
          m_stack_upper(stack_up)
    {
        assert(m_stack_lower <= m_stack_upper && "Stack upside down");
//        std::cout << "Creating new context " << *this << std::endl;
    }

    ~ContextState()
    {
//        std::cout << "Deleting context " << *this << std::endl;
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

    bool overlaps( ContextState &other ) const
    {
        if ( other.m_stack_upper <= m_stack_lower )
            return false;
        if ( m_stack_upper <= other.m_stack_lower )
            return false;
        return true;
    }

    void print( std::ostream &o ) const
    {
        o << "<Context "
          << std::dec << m_id << std::hex << std::showbase
          << " " << m_stack_lower
          << "->" << m_stack_upper
          << ">";
    }

    friend std::ostream &operator << (std::ostream &o, const ContextState &cs)
    {
        cs.print(o);
        return o;
    }
};

class RegionInfo
{
public:
    enum State {
        REGION_STATE_GLOBAL,
        REGION_STATE_GLOBAL_READ_ONLY,
        REGION_STATE_ALLOCATED,
        REGION_STATE_FREE,
        REGION_STATE_PERIPHERAL,
    };

private:
    enum State m_state;
    uint32_t m_created_at;
    uint32_t m_updated_at;
    uint32_t m_refcount;
    uint32_t m_base_addr;
    uint32_t m_end_addr;

public:
    RegionInfo *get_updated_region( enum State state, uint32_t at, uint32_t base_addr, uint32_t end_addr )
    {
        RegionInfo *n = new RegionInfo( state, m_created_at, base_addr, end_addr );
        n->m_updated_at = at;
        return n;
    }

    RegionInfo( enum State state, uint32_t at, uint32_t base_addr, uint32_t end_addr )
        : m_state(state),
          m_created_at(at),
          m_updated_at(0),
          m_refcount(0),
          m_base_addr(base_addr),
          m_end_addr(end_addr)
    {
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
        if ( m_state == REGION_STATE_FREE || m_state == REGION_STATE_GLOBAL_READ_ONLY )
            return ERROR_INVALID_REGION;
        return ERROR_NONE;
    }

    error_level_t do_read() const
    {
        if ( m_state == REGION_STATE_FREE )
            return ERROR_INVALID_REGION;
        return ERROR_NONE;
    }

    static const char *state_str(State state)
    {
        switch (state) {
        case REGION_STATE_GLOBAL: return "global";
        case REGION_STATE_GLOBAL_READ_ONLY: return "global read only";
        case REGION_STATE_ALLOCATED: return "allocated";
        case REGION_STATE_FREE: return "free";
        case REGION_STATE_PERIPHERAL: return "peripheral";
        default: return "unknown";
        }
    }

    void print( std::ostream &o ) const
    {
        o << "<RegionInfo" << std::hex
          << " created at "  << m_created_at
          << " updated at "  << m_updated_at
          << " @"  << m_base_addr
          << "->"  << m_end_addr
          << ", "  << state_str(m_state)
          << ", ref=" << m_refcount
          << ">";
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

    void region_set( RegionInfo *ptr )
    {
        assert(ptr);
        ptr->ref();

//        std::cout << "Replacing " << *this << " with " << *ptr << std::endl;
        if ( region() )
            region()->unref();
        m_info = (RegionInfo*)(
            (uintptr_t)ptr & s_addr_mask |
            (uintptr_t)m_info & ~s_addr_mask);
    }

    AddressInfo & operator=( const AddressInfo &ref )
    {
        if ( &ref == this )
            return *this;
        region_set(ref.region());
        set_initialized(false);
    }
    
    AddressInfo( RegionInfo *ri, bool initialized = false )
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
    ElfLoader m_binary;

    typedef std::map<uint32_t, ContextState *> context_map_t;
    typedef std::map<uint32_t, std::vector<AddressInfo> *> region_map_t;
    context_map_t m_contexts;
    region_map_t m_regions;
    AddressInfo m_default_address;

public:
    ContextState * const unknown_context;

    MemoryState( const soclib::common::MappingTable &mt,
                 const soclib::common::ElfLoader &loader,
                 const std::string &exclusions )
        : m_binary(loader),
          m_contexts(),
          m_regions(),
          m_default_address(),
          unknown_context(new ContextState((uint32_t)-1, 0, 0 )) //(uint32_t)-1 ))
    {
        const std::list<Segment> &segments = mt.getAllSegmentList();
        ElfLoader::section_list_t sections = loader.sections();

        std::string exclusion_list = ",";
        exclusion_list += exclusions + ",";

        for ( std::list<Segment>::const_iterator i = segments.begin();
              i != segments.end();
              ++i ) {

            RegionInfo *ri = new RegionInfo(
                RegionInfo::REGION_STATE_FREE, 0,
                i->baseAddress(), i->baseAddress() + i->size() );
            std::vector<AddressInfo> *rm = new std::vector<AddressInfo>( i->size() / 4 );

            for ( size_t j=0; j<i->size()/4; ++j )
                (*rm)[j].region_set(ri);

            m_regions[i->baseAddress()] = rm;
        }

        for ( ElfLoader::section_list_t::const_iterator i = sections.begin();
              i != sections.end();
              ++i ) {

//             std::cout << "Creating a region info for"
//                       << " " << i->name()
//                       << " @" << std::hex << i->lma()
//                       << ", " << std::dec << i->size()/4 << " words long"
//                       << std::endl;

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
        context_map_t::const_iterator i = m_contexts.find(id);
        assert(i == m_contexts.end()
               && "Creating two contexts with the same id...");
        for ( context_map_t::const_iterator i = m_contexts.begin();
              i != m_contexts.end();
              ++i ) {
            assert( !context->overlaps( *i->second ) );
        }
        m_contexts[id] = context;
    }

    void context_delete( uint32_t id )
    {
        context_map_t::iterator i = m_contexts.find(id);
        assert(i != m_contexts.end()
               && "Deleting non-existant context...");
        m_contexts.erase(i);
    }

    AddressInfo *info_for_address(uint32_t address)
    {
        region_map_t::iterator i = m_regions.upper_bound(address);
        --i;
//         if ( i != m_regions.end()
//              && i != m_regions.begin()
//              && i->first > address )
//             --i;

        if ( i == m_regions.end() ) {
            std::cout
                << "Address " << std::hex << address << " in no region." << std::endl
                << "Regions: " << std::endl;
            for ( region_map_t::iterator i = m_regions.begin();
                  i != m_regions.end();
                  ++i )
                std::cout << " " << i->first << " size: " << i->second->size() << " words" << std::endl;
            
            abort();
        }
        uint32_t region_base = i->first;
        uint32_t word_no = (address-region_base)/4;
        std::vector<AddressInfo> &r = *(i->second);
        assert( region_base <= address && word_no < r.size() );
        return &r[word_no];
    }

    void region_update_state( RegionInfo::State new_state, uint32_t at, uint32_t addr, uint32_t size )
    {
//         std::cout
//             << " Updating region state " << std::hex << std::showbase
//             << addr << "->" << addr+size
//             << " to " << RegionInfo::state_str(new_state) << std::endl;
        RegionInfo *lri = info_for_address(addr)->region();
        RegionInfo *nri = lri->get_updated_region( new_state, at, addr, addr+size );
        for ( uint32_t a = addr; a < addr+size; a+=4 ) {
            info_for_address(a)->region_set(nri);
        }
    }

    void region_new_state( RegionInfo::State new_state, uint32_t at, uint32_t addr, uint32_t size )
    {
        RegionInfo *nri = new RegionInfo( new_state, at, addr, addr+size );
        for ( uint32_t a = addr; a < addr+size; a+=4 )
            info_for_address(a)->region_set(nri);
    }
 
    std::string get_symbol( uintptr_t addr ) const
    {
        return m_binary.get_symbol(addr);
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
                                 const soclib::common::ElfLoader &loader,
                                 const std::string &exclusions )
{
    s_comm_address = 0x4200;
    std::cout << "iss_memchecker initialized with comm addr " << std::hex << s_comm_address << std::endl;
    s_memory_state = new MemoryState( mt, loader, exclusions );
}

template<typename iss_t>
IssMemchecker<iss_t>::IssMemchecker(const std::string &name, uint32_t ident)
    : iss_t(name, ident),
      m_enabled_checks(0),
      m_last_sp(0)
{
    if ( !s_memory_state ) {
        std::cerr
            << std::endl
            << "You must call the static initialized with:" << std::endl
            << "soclib::common::IssMemchecker<...>::init( mapping_table, elf_loader );" << std::endl
            << "Prior to any IssMemchecker constructor." << std::endl
            << std::endl;
        abort();
    }
    m_current_context = s_memory_state->unknown_context;
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
                assert(!"Wrong magic");
            }
            break;
        default:
            assert(value == 0 && "When in magic mode, only valid value is 0");
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
            err |= ( ai->region()->state() != __iss_memchecker::RegionInfo::REGION_STATE_ALLOCATED );
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
        if ( m_current_context->is( m_r1 ) )
            m_current_context = n;
        break;
    }
	case ISS_MEMCHECKER_CONTEXT_ID_DELETE:
        if ( m_current_context->is( value ) )
            m_current_context = s_memory_state->unknown_context;
        s_memory_state->context_delete(value);
        break;
	case ISS_MEMCHECKER_CONTEXT_CURRENT:
        m_current_context = s_memory_state->context_get( value );
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
        default:
            assert(!"Invalid region state");
        }
        s_memory_state->region_update_state(
            state,
            get_cpu_pc(),
            m_r1, m_r2 );
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
void IssMemchecker<iss_t>::handle_comm( const struct iss_t::DataRequest &dreq )
{
    uint32_t reg_no = (dreq.addr-s_comm_address)/4;
    assert( dreq.be == 0xf && "Only read/write word are allowed in memchecker area" );

    switch ( dreq.type ) {
    case iss_t::DATA_READ:
        m_data_answer_value = register_get(reg_no);
        break;
    case iss_t::DATA_WRITE:
        m_data_answer_value = 0;
        register_set(reg_no, dreq.wdata);
        break;
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

    if ( m_enabled_checks & ISS_MEMCHECKER_CHECK_INIT ) {
//         std::cout
//             << iss_t::m_name
//             << " " << dreq
//             << " " << *ai
//             << std::endl;
        switch ( dreq.type ) {
        case iss_t::DATA_READ:
        case iss_t::DATA_LL:
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
    }
    if ( m_current_context->stack_contains(dreq.addr) ) {
        if ( ( m_enabled_checks & ISS_MEMCHECKER_CHECK_SP )
//             && m_current_context != s_memory_state->unknown_context
             && dreq.addr < get_cpu_sp() ) {
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
void IssMemchecker<iss_t>::report_error(uint32_t errors)
{
    if ( !errors )
        return;

    // Signal to GDB
    iss_t::debugExceptionBypassed( (uint32_t)-1 );

    std::cout << iss_t::m_name << " error:" << std::endl;

    AddressInfo *ai = s_memory_state->info_for_address(m_last_data_access.addr);

    if ( errors & ERROR_UNINITIALIZED_WORD )
        std::cout << " access to uninitialized word" << std::endl;
    if ( errors & ERROR_INVALID_REGION )
        std::cout << " access to invalid region: " << *(ai->region()) << std::endl;
    if ( errors & ERROR_CREATING_STACK_NOT_ALLOC )
        std::cout << " creating stack in non-allocated memory" << std::endl;
    if ( errors & ERROR_SP_OUTOFBOUNDS ) {
        std::cout << " stack pointer out of bounds: ";
        uint32_t sp = get_cpu_sp();
        if ( sp < m_current_context->m_stack_lower )
            std::cout << (m_current_context->m_stack_lower - sp) << " bytes below" << std::endl;
        else
            std::cout << (sp - m_current_context->m_stack_upper) << " bytes above" << std::endl;
    }
    if ( errors & ERROR_FP_OUTOFBOUNDS ) {
        std::cout << " stack pointer out of bounds: ";
        uint32_t fp = get_cpu_fp();
        if ( fp < m_current_context->m_stack_lower )
            std::cout << (m_current_context->m_stack_lower - fp) << " bytes below" << std::endl;
        else
            std::cout << (fp - m_current_context->m_stack_upper) << " bytes above" << std::endl;
    }
    if ( errors & ERROR_DATA_ACCESS_BELOW_SP )
        std::cout << " data access below SP" << std::endl;
    if ( errors & (ERROR_FP_OUTOFBOUNDS | ERROR_SP_OUTOFBOUNDS | ERROR_DATA_ACCESS_BELOW_SP) ) {
        std::cout << " stack range: " << m_current_context->m_stack_lower << "->" << m_current_context->m_stack_upper << std::endl;
    }

    std::cout
        << " at PC=" << s_memory_state->get_symbol(get_cpu_pc()) << std::endl

        << "    SP=" << s_memory_state->get_symbol(get_cpu_sp()) << std::endl

        << "    last Dreq: " << m_last_data_access << ' '
        << s_memory_state->get_symbol(m_last_data_access.addr) << std::endl

        

        << std::endl;
}

template<typename iss_t>
uint32_t IssMemchecker<iss_t>::executeNCycles(
    uint32_t ncycle, struct iss_t::InstructionResponse irsp,
    struct iss_t::DataResponse drsp, uint32_t irq_bit_field )
{
    if ( m_has_data_answer ) {
        assert( !drsp.valid && "Cache speaking while i'm answering ISS" );
        drsp.valid = true;
        drsp.error = false;
        drsp.rdata = m_data_answer_value;
        m_has_data_answer = false;
    } else {
        if ( drsp.valid ) {
            struct iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
            struct iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
            iss_t::getRequests(ireq, dreq);
            check_data_access( dreq );
        }
    }

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
        uint32_t err = ERROR_NONE;
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
template<typename iss_t> uint32_t IssMemchecker<iss_t>::s_comm_address;

}}


// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
