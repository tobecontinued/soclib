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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *         Alain Greiner <alain.greiner@lip6.fr>, 2008
 *
 * Maintainers: alain
 */

/////////////////////////////////////////////////////////////////////////////
// History
// - 01/06/2008
//   The VCI_XCACHE and the ISS_WRAPPER components have been merged, in order
//   to increase the simulation speed: this VCI_XCACHE_WRAPPER component
//   is directly wrapping the processsor ISS, allowing a direct communication
//   between the processor and the cache.
//   The number of associativity levels is now a parameter for both the data
//   and the instruction cache.
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include "arithmetics.h"
#include "alloc_elems.h"
#include "../include/vci_xcache_wrapper.h"

namespace soclib {
namespace caba {

#ifndef XCACHE_WRAPPER_DEBUG
#ifdef SOCLIB_MODULE_DEBUG
#define XCACHE_WRAPPER_DEBUG 1
#else
#define XCACHE_WRAPPER_DEBUG 0
#endif
#endif

#define LINE_VALID 0x80000000

#if XCACHE_WRAPPER_DEBUG
namespace {
const char *dcache_fsm_state_str[] = {
        "DCACHE_INIT",
        "DCACHE_IDLE",
        "DCACHE_WRITE_UPDT",
        "DCACHE_WRITE_REQ",
        "DCACHE_MISS_REQ",
        "DCACHE_MISS_WAIT",
        "DCACHE_MISS_UPDT",
        "DCACHE_UNC_REQ",
        "DCACHE_UNC_WAIT",
        "DCACHE_INVAL",
        "DCACHE_ERROR",
    };

const char *icache_fsm_state_str[] = {
        "ICACHE_INIT",
        "ICACHE_IDLE",
        "ICACHE_WAIT",
        "ICACHE_UPDT",
        "ICACHE_ERROR",
    };

const char *cmd_fsm_state_str[] = {
        "CMD_IDLE",
        "CMD_DATA_MISS",
        "CMD_DATA_UNC",
        "CMD_DATA_WRITE",
        "CMD_INS_MISS",
    };

const char *rsp_fsm_state_str[] = {
        "RSP_IDLE",
        "RSP_INS_MISS",
        "RSP_INS_ERROR_WAIT",
        "RSP_INS_ERROR",
        "RSP_INS_OK",
        "RSP_DATA_MISS",
        "RSP_DATA_UNC",
        "RSP_DATA_WRITE",
        "RSP_DATA_READ_ERROR_WAIT",
        "RSP_DATA_READ_ERROR",
        "RSP_DATA_MISS_OK",
        "RSP_DATA_UNC_OK",
        "RSP_DATA_WRITE_ERROR",
        "RSP_DATA_WRITE_ERROR_WAIT",
    };
}
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciXcacheWrapper<vci_param, iss_t>

tmpl(inline typename VciXcacheWrapper<vci_param, iss_t>::data_t)::be_to_mask( typename iss_t::be_t be )
{
    size_t i;
    data_t ret = 0;
    const typename iss_t::be_t be_up = (1<<(sizeof(data_t)-1));

    for (i=0; i<sizeof(data_t); ++i) {
        ret <<= 8;
        if ( be_up & be )
            ret |= 0xff;
        be <<= 1;
    }
    return ret;
}

///////////////////////////////////////
tmpl(inline bool)::is_write(data_op_t cmd)
{
    return cmd == iss_t::DATA_WRITE;
}

///////////////////////////////////////////////////////////////
tmpl(inline bool)::can_burst(data_op_t old_type, addr_t old_addr,
                             data_op_t new_type, addr_t new_addr )
{
    bool res = is_write(old_type) && is_write(new_type) &&
        ((addr_t)(new_addr&~(vci_param::B-1)) == old_addr&~(vci_param::B-1)+4);
//        !((next.addr^old.addr)&~4095);
    return res;
}

tmpl(typename VciXcacheWrapper<vci_param, iss_t>::data_t &)::icache_data( size_t way, size_t set, size_t word )
{
    return r_icache_data[(way*m_icache_sets*m_icache_words)+(set*m_icache_words)+word];
}

tmpl(typename VciXcacheWrapper<vci_param, iss_t>::tag_t &)::icache_tag( size_t way, size_t set )
{
    return r_icache_tag[(way*m_icache_sets)+set];
}

tmpl(typename VciXcacheWrapper<vci_param, iss_t>::data_t &)::dcache_data( size_t way, size_t set, size_t word )
{
    return r_dcache_data[(way*m_dcache_sets*m_dcache_words)+(set*m_dcache_words)+word];
}

tmpl(typename VciXcacheWrapper<vci_param, iss_t>::tag_t &)::dcache_tag( size_t way, size_t set )
{
    return r_dcache_tag[(way*m_dcache_sets)+set];
}

tmpl(std::string)::dump_dcache_line( size_t way, size_t set )
{
    std::ostringstream o;
    o << std::hex << "<D wst=" << way << "/" << set << "/" << dcache_tag(way,set)
      << ":";
    for ( size_t x=0; x<m_dcache_words; ++x )
        o << " " << dcache_data(way, set, x);
    o << ">";
    return o.str();
}

using soclib::common::uint32_log2;

/////////////////////////////
tmpl(/**/)::VciXcacheWrapper(
    sc_module_name name,
    int proc_id,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &index,
    size_t icache_sets,
    size_t icache_words,
    size_t icache_ways,
    size_t dcache_sets,
    size_t dcache_words,
    size_t dcache_ways )
    : soclib::caba::BaseModule(name),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci"),
      m_cacheability_table(mt.getCacheabilityTable()),
      m_srcid(mt.indexForId(index)),
      m_iss(this->name(), proc_id),

      m_dcache_sets(dcache_sets),
      m_dcache_words(dcache_words),
      m_dcache_ways(dcache_ways),
      m_icache_sets(icache_sets),
      m_icache_words(icache_words),
      m_icache_ways(icache_ways),

      m_i_x( uint32_log2(m_icache_words), uint32_log2(vci_param::B) ),
      m_i_y( uint32_log2(m_icache_sets), uint32_log2(m_icache_words) + uint32_log2(vci_param::B) ),
      m_i_z( vci_param::N-uint32_log2(m_icache_sets) - uint32_log2(m_icache_words) - uint32_log2(vci_param::B),
             uint32_log2(m_icache_sets) + uint32_log2(m_icache_words) + uint32_log2(vci_param::B) ),
      m_icache_yzmask((~0)<<(uint32_log2(m_icache_words) + uint32_log2(vci_param::B))),

      m_d_x( uint32_log2(m_dcache_words), uint32_log2(vci_param::B) ),
      m_d_y( uint32_log2(m_dcache_sets), uint32_log2(m_dcache_words) + uint32_log2(vci_param::B) ),
      m_d_z( vci_param::N-uint32_log2(m_dcache_sets) - uint32_log2(m_dcache_words) - uint32_log2(vci_param::B),
             uint32_log2(m_dcache_sets) + uint32_log2(m_dcache_words) + uint32_log2(vci_param::B) ),
      m_dcache_yzmask((~0)<<(uint32_log2(m_dcache_words) + uint32_log2(vci_param::B))),

      r_dcache_data(new data_t[m_dcache_ways*m_dcache_sets*m_dcache_words]),
      r_dcache_tag(new tag_t[m_dcache_ways*m_dcache_sets]),
      r_icache_data(new data_t[m_icache_ways*m_icache_sets*m_icache_words]),
      r_icache_tag(new tag_t[m_icache_ways*m_icache_sets]),

      r_dcache_fsm("r_dcache_fsm"),
      r_dcache_be_save("r_dcache_be_save"),
      r_dcache_addr_save("r_dcache_addr_save"),
      r_dcache_data_save("r_dcache_data_save"),
      r_dcache_prev_save("r_dcache_prev_save"),
      r_dcache_type_save("r_dcache_type_save"),
      r_dcache_cached_save("r_dcache_cached_save"),

      m_dreq_addr_fifo("m_dreq_addr_fifo", 8),
      m_dreq_be_fifo("m_dreq_be_fifo", 8),
      m_dreq_data_fifo("m_dreq_data_fifo", 8),
      m_dreq_type_fifo("m_dreq_type_fifo", 8),
      m_dreq_cached_fifo("m_dreq_cached_fifo", 8),

      r_icache_fsm("r_icache_fsm"),
      r_icache_miss_addr("r_icache_miss_addr"),
      r_icache_req("r_icache_req"),

      r_vci_cmd_fsm("r_vci_cmd_fsm"),
      r_dcache_be_cmd("r_dcache_be_cmd"),
      r_dcache_addr_cmd("r_dcache_addr_cmd"),
      r_dcache_data_cmd("r_dcache_data_cmd"),
      r_dcache_type_cmd("r_dcache_type_cmd"),
      r_dcache_cached_cmd("r_dcache_cached_cmd"),
      r_dcache_miss_addr("r_dcache_miss_addr"),
      r_cmd_cpt("r_cmd_cpt"),

      r_vci_rsp_fsm("r_vci_rsp_fsm"),
      r_dcache_unc_valid("r_dcache_unc_valid"),
      r_rsp_cpt("r_rsp_cpt"),

      r_dcache_cpt_init("r_dcache_cpt_init"),
      r_icache_cpt_init("r_icache_cpt_init")
{
    assert(IS_POW_OF_2(icache_sets));
    assert(IS_POW_OF_2(dcache_sets));
    assert(IS_POW_OF_2(icache_words));
    assert(IS_POW_OF_2(dcache_words));
    assert(IS_POW_OF_2(icache_ways));
    assert(IS_POW_OF_2(dcache_ways));
    assert(icache_words);
    assert(dcache_words);
    assert(icache_sets);
    assert(dcache_sets);
    assert(icache_ways);
    assert(dcache_ways);
    assert(icache_words <= 32);
    assert(dcache_words <= 32);
    assert(icache_sets <= 1024);
    assert(dcache_sets <= 1024);
    assert(icache_ways <= 16);
    assert(dcache_ways <= 16);

    memset(r_dcache_data, 0, sizeof(data_t)*(m_dcache_ways*m_dcache_sets*m_dcache_words));
    memset(r_dcache_tag, 0, sizeof(tag_t)*(m_dcache_ways*m_dcache_sets));
    memset(r_icache_data, 0, sizeof(data_t)*(m_icache_ways*m_icache_sets*m_icache_words));
    memset(r_icache_tag, 0, sizeof(tag_t)*(m_icache_ways*m_icache_sets));

    r_icache_miss_buf = soclib::common::alloc_elems<sc_signal<data_t> >("icache_miss_buff", icache_words);
    r_dcache_miss_buf = soclib::common::alloc_elems<sc_signal<data_t> >("dcache_miss_buff", dcache_words);

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();

    m_iss.setICacheInfo( icache_words*sizeof(data_t), icache_ways, icache_sets );
    m_iss.setDCacheInfo( dcache_words*sizeof(data_t), dcache_ways, dcache_sets );
}


tmpl(/**/)::~VciXcacheWrapper()
{
    delete [] r_dcache_data;
    delete [] r_dcache_tag;
    delete [] r_icache_data;
    delete [] r_icache_tag;
    soclib::common::dealloc_elems(r_icache_miss_buf, m_icache_words);
    soclib::common::dealloc_elems(r_dcache_miss_buf, m_dcache_words);
}

tmpl(void)::transition()
{
    if ( ! p_resetn.read() ) {
        memset(r_dcache_data, 0, sizeof(data_t)*(m_dcache_ways*m_dcache_sets*m_dcache_words));
        memset(r_dcache_tag, 0, sizeof(tag_t)*(m_dcache_ways*m_dcache_sets));
        memset(r_icache_data, 0, sizeof(data_t)*(m_icache_ways*m_icache_sets*m_icache_words));
        memset(r_icache_tag, 0, sizeof(tag_t)*(m_icache_ways*m_icache_sets));

        m_iss.reset();

        r_dcache_fsm = DCACHE_INIT;
        r_icache_fsm = ICACHE_INIT;
        r_vci_cmd_fsm = CMD_IDLE;
        r_vci_rsp_fsm = RSP_IDLE;

        r_dcache_cpt_init = m_dcache_sets - 1;
        r_icache_cpt_init = m_icache_sets - 1;

        m_dreq_addr_fifo.init();
        m_dreq_be_fifo.init();
        m_dreq_data_fifo.init();
        m_dreq_type_fifo.init();
        m_dreq_cached_fifo.init();

        r_icache_req = false;

        r_dcache_unc_valid = false;

        m_cpt_dcache_data_read  = 0;
        m_cpt_dcache_data_write = 0;
        m_cpt_dcache_dir_read  = 0;
        m_cpt_dcache_dir_write = 0;
        m_cpt_icache_data_read  = 0;
        m_cpt_icache_data_write = 0;
        m_cpt_icache_dir_read  = 0;
        m_cpt_icache_dir_write = 0;
        m_cpt_fifo_read  = 0;
        m_cpt_fifo_write = 0;

        return;
    }

    /////////// processor instruction request //////////////////////////////
    // The processor requests are taken into account only
    // in the ICACHE_IDLE state, and the processor is frozen in case of MISS.
    //
    // Error handling : Instruction Bus Errors are synchronous events.
    // If a bus error is detected by the vci_rsp_fsm, the ICACHE FSM
    // goes to the ICACHE_ERROR state, and the error is signaled.
    // The processor is never frozen when there is no instruction request.
    ////////////////////////////////////////////////////////////////////////

    bool    icache_hit = false;
    size_t  icache_way = 0;
    data_t  icache_ins = 0;

    typename iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    m_iss.getInstructionRequest( ireq );

    const int   icache_x = m_i_x[ireq.addr];
    const int   icache_y = m_i_y[ireq.addr];
    const tag_t icache_z = m_i_z[ireq.addr] | LINE_VALID;

    for(size_t way = 0 ; way < m_icache_ways ; way++) {
        if(icache_z == icache_tag(way, icache_y)) {
            icache_way = way;
            icache_hit = true;
            icache_ins = icache_data(icache_way, icache_y, icache_x);
        }
    } // end for

#if XCACHE_WRAPPER_DEBUG
        std::cout << name() << " icache request: " << ireq << std::endl;
#endif

    {
        typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

        switch ((icache_fsm_state_e)r_icache_fsm.read()) {
        case ICACHE_IDLE:
            if (ireq.valid) {
                irsp.valid = icache_hit;
                irsp.instruction = icache_ins;
            } else {
                irsp.valid = false;
            }
            break;
        case ICACHE_ERROR:
            irsp.valid = true;
            irsp.error = true;
            break;
        default:
            irsp.valid = false;
            break;
        }
        m_iss.setInstruction( irsp );
    }

    //////////// processor data request  //////////////////////////////////
    // The processor requests are taken into account only
    // in the DCACHE_IDLE and DCACHE_WRITE_REQ states :
    // - In the IDLE state, the processor is frozen when
    //   there is a cached read miss, or an uncached read miss.
    // - In the WRITE_REQ state, the processor is frozen when
    //   there is a cached read miss, or an uncached read miss,
    //   or when the fifo (write buffer) is full.
    // - In all other states, the processor is frozen for any
    //   data request.
    //
    // Error handling :  Read Bus Errors are synchronous events, but
    // Write Bus Errors are asynchronous events (processor is not frozen).
    // - If a Read Bus Error is detected, the DCACHE FSM goes to
    // the DCACHE_ERROR state, the synchronous error is signaled.
    // - If a Write Bus Error is detected, the VCI_RSP FSM  signals
    // the asynchronous error using the setWriteBerr() method.
    ////////////////////////////////////////////////////////////////////////

    bool      dcache_hit = false;
    size_t    dcache_way = 0;
    data_t    dcache_rdata = 0;

    typename iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    m_iss.getDataRequest( dreq );

    addr_t dcache_addr = dreq.addr;
    typename iss_t::ExternalAccessType xtn_opcod = (typename iss_t::ExternalAccessType)(dreq.addr/4);

    bool   dcache_cached = false;
    if ( dreq.valid ) {
        switch(dreq.type) {
        case iss_t::DATA_LL:
        case iss_t::DATA_SC:
            dcache_cached = false;
            break;
        case iss_t::XTN_READ:
        case iss_t::XTN_WRITE:
            // As a special case, extended operations pass addresses
            // on data field (as the address is in this case the
            // parameter, thus data)
            dcache_addr = dreq.wdata;
        default:
            dcache_cached = m_cacheability_table[dcache_addr];
        }

        const int    dcache_x = m_d_x[dcache_addr];
        const int    dcache_y = m_d_y[dcache_addr];
        const tag_t  dcache_z = m_d_z[dcache_addr] | LINE_VALID;

        if ( dcache_cached ) {
            for (size_t way = 0 ; way < m_dcache_ways ; way++) {
                if ( dcache_z == dcache_tag(way, dcache_y) ) {
                    dcache_way = way;
                    dcache_hit = true;
                    dcache_rdata = dcache_data(dcache_way, dcache_y, dcache_x);
                }
            } // end for
        } else {
            dcache_hit = (r_dcache_unc_valid.read() && (dcache_addr == r_dcache_miss_addr));
            dcache_rdata = r_dcache_miss_buf[0];
        }

#if XCACHE_WRAPPER_DEBUG
        std::cout << name()
                  << " dcache request: " << dreq
                  << " cached: " << dcache_cached
                  << " hit: " << dcache_hit
                  << std::endl;
#endif
    }

    typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;
    switch ((dcache_fsm_state_e)r_dcache_fsm.read()) {
    case DCACHE_ERROR:
        drsp.valid = true;
        drsp.error = true;
        break;
    case DCACHE_WRITE_REQ:
        if ( !m_dreq_addr_fifo.wok() && dreq.valid ) {
            drsp.valid = false;
            break;
        }
    case DCACHE_IDLE:
        if ( !dreq.valid ) {
            drsp.valid = true;
            break;
        }
        switch (dreq.type) {
        case iss_t::DATA_READ:
        case iss_t::DATA_LL:
        case iss_t::DATA_SC:
            drsp.valid = dcache_hit;
            drsp.rdata = dcache_rdata;
            break;
        case iss_t::XTN_WRITE:
        case iss_t::XTN_READ:
            // We only support dcache invalidation, the rest is error.
            drsp.error = xtn_opcod != iss_t::XTN_DCACHE_INVAL;
        case iss_t::DATA_WRITE:
            drsp.valid = true;
            drsp.rdata = 0;
            break;
        } // end switch dreq.type
        break;
    default:
        drsp.valid = false;
    }
    m_iss.setData( drsp );

#if XCACHE_WRAPPER_DEBUG
    if ( drsp.valid )
        std::cout << name() << " Data rsp: " << drsp << std::endl;
#endif

    /////////// execute one iss cycle //////////////////
    {
        uint32_t it = 0;
        for ( size_t i=0; i<(size_t)iss_t::n_irq; i++ )
            if (p_irq[i].read()) it |= (1<<i);
        m_iss.executeNCycles(1, it);
    }

#if XCACHE_WRAPPER_DEBUG
    std::cout
        << name()
        << " dcache: " << dcache_fsm_state_str[r_dcache_fsm]
        << " icache: " << icache_fsm_state_str[r_icache_fsm]
        << " cmd: " << cmd_fsm_state_str[r_vci_cmd_fsm]
        << " rsp: " << rsp_fsm_state_str[r_vci_rsp_fsm]
        << std::endl;
#endif

    ///////// ICACHE_FSM ////////////////////////////////////////////////
    // The ICACHE FSM controls the following ressources:
    // - r_icache_fsm
    // - r_icache_data[m_icache_ways*m_icache_sets*m_icache_words]
    // - r_icache_tag[m_icache_ways*m_icache_sets]
    // - r_icache_miss_addr
    // - r_icache_req set
    // - r_icache_cpt_init
    // The VALID bit for a cache line is the MSB bit in the TAG.
    // Only cached read requests are supported.
    // In case of MISS, the controller writes a request in the
    // r_icache_miss_addr register and ways the r_ireq.req flip-flop.
    // The r_ireq.req flip-flop is reset by the VCI_RSP controller,
    // when the cache line is ready in the ICACHE buffer.
    ///////////////////////////////////////////////////////////////////////

    switch((icache_fsm_state_e)r_icache_fsm.read()) {
    case ICACHE_INIT:
        for(size_t way = 0 ; way < m_icache_ways ; way++) {
            icache_tag(way, r_icache_cpt_init) = 0;
        }
        r_icache_cpt_init = r_icache_cpt_init - 1;
        if (r_icache_cpt_init == 0) r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
    break;
    case ICACHE_IDLE:
        if ( ireq.valid ) {
            if ( ! icache_hit ) {
                r_icache_fsm = ICACHE_WAIT;
                r_icache_miss_addr = ireq.addr & m_icache_yzmask;
                r_icache_req = true;
            }
            m_cpt_icache_dir_read++;
            m_cpt_icache_data_read++;
        }
    break;
    case ICACHE_WAIT:
        if (r_vci_rsp_fsm.read() == RSP_INS_OK)
            r_icache_fsm = ICACHE_UPDT;
        if (r_vci_rsp_fsm.read() == RSP_INS_ERROR)
            r_icache_fsm = ICACHE_ERROR;
    break;
    case ICACHE_ERROR:
        r_icache_fsm = ICACHE_IDLE;
    break;
    case ICACHE_UPDT:
        {
        size_t set = (size_t)m_i_y[r_icache_miss_addr];
        tag_t  tag  = (tag_t)m_i_z[r_icache_miss_addr] | LINE_VALID;
        // selecting a victim
        size_t victim = 0x1000;
        for(size_t way = 0 ; way < m_icache_ways ; way++) {
            if((icache_tag(way, set) & 0x80000000) == 0) victim = way;
        }
        if (victim > 0xFFF) victim = m_cpt_fifo_write % m_icache_ways;
        // cache update
        icache_tag(victim, set) = tag;
        for (size_t i=0 ; i<m_icache_words ; i++) {
            icache_data(victim, set, i) = r_icache_miss_buf[i];
        }
        r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
        m_cpt_icache_data_write++;
        }
    break;
    } // end switch r_icache_fsm

    //////// DCACHE_FSM  /////////////////////////////////////////////
    // The DCACHE FSM controls the following ressources:
    // - r_dcache_fsm
    // - r_dcache_data[m_dcache_ways*m_dcache_sets*m_dcache_words]
    // - r_dcache_tag[m_dcache_ways*m_dcache_sets]
    // - r_dcache_save
    // - r_dcache_cpt_init
    // - r_dcache_unc_valid reset
    // - fifo_put, dreq
    // The VALID bit for a cache line is the MSB bit in the TAG.
    // In the IDLE & WRITE_REQ states, the processor request is saved in the
    // r_dcache_*_save registers.
    // There is five mutually exclusive conditions to exit the IDLE state:
    // - CACHED READ MISS => to the MISS_REQ state (to post the request in the FIFO),
    // then to the MISS_WAIT state (waiting the cache line), then to the MISS_UPDT
    // (to update the cache), and finally to the IDLE state.
    // - UNCACHED READ  => to the UNC_REQ state (to post the request in the FIFO),
    // then to the UNC_WAIT state, and finally to the IDLE state.
    // - CACHE INVALIDATE HIT => to the INVAL state for one cycle, then IDLE.
    // - WRITE MISS => directly to the WRITE_REQ state (to post the request in the FIFO)
    // Then it depends on the processor request: In order to support VCI write burst,
    // the processor requests are taken into account in the WRITE_REQ state
    // as well as in the IDLE state.
    // - WRITE HIT => to the WRITE_UPDT state (to update the cache), then to
    // the WRITE_REQ state.
    ///////////////////////////////////////////////////////////////////////////////////

    bool    fifo_put = false;
    bool    fifo_get = false;

    switch ((dcache_fsm_state_e)r_dcache_fsm.read()) {

    case DCACHE_INIT:
        for(size_t way = 0 ; way < m_dcache_ways ; way++) {
            dcache_tag(way, r_dcache_cpt_init) = 0;
        }
        r_dcache_cpt_init = r_dcache_cpt_init - 1;
        if (r_dcache_cpt_init == 0) r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
    break;

    case DCACHE_WRITE_REQ:
        fifo_put = true;
        if (! m_dreq_addr_fifo.wok())
            break;
        if (! dreq.valid) {
                r_dcache_fsm = DCACHE_IDLE;
                break;
        }
        m_cpt_fifo_write++;

    case DCACHE_IDLE:
        {
        if (! dreq.valid)
                break;
        m_cpt_dcache_data_read++;
        m_cpt_dcache_dir_read++;

        switch(dreq.type) {
        case iss_t::DATA_READ:
            if ( dcache_hit ) {
                r_dcache_fsm = DCACHE_IDLE;
                if (!dcache_cached)
                    r_dcache_unc_valid = false;
            } else {
                if (dcache_cached)
                    r_dcache_fsm = DCACHE_MISS_REQ;
                else
                    r_dcache_fsm = DCACHE_UNC_REQ;
            }
            break;
        case iss_t::DATA_LL:
        case iss_t::DATA_SC:
            if ( dcache_hit ) {
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_unc_valid = false;
            } else {
                r_dcache_fsm = DCACHE_UNC_REQ;
            }
            break;
        case iss_t::XTN_READ:
        case iss_t::XTN_WRITE:
            if ( xtn_opcod == iss_t::XTN_DCACHE_INVAL && dcache_hit )
                r_dcache_fsm = DCACHE_INVAL;
            else
                r_dcache_fsm = DCACHE_IDLE;
            break;
        case iss_t::DATA_WRITE:
            if (dcache_hit && dcache_cached)
                r_dcache_fsm = DCACHE_WRITE_UPDT;
            else
                r_dcache_fsm = DCACHE_WRITE_REQ;
        break;
        } // end switch dreq.type

        r_dcache_addr_save      = dcache_addr;
        r_dcache_be_save      = dreq.be;
        r_dcache_data_save      = dreq.wdata;
        r_dcache_type_save      = dreq.type;
        r_dcache_cached_save    = dcache_cached;
        r_dcache_prev_save      = dcache_rdata;
        r_dcache_way_save       = dcache_way;
        }
    break;

    case DCACHE_WRITE_UPDT:
        {
        data_t  new_data = r_dcache_data_save;
        data_t  prev_data = r_dcache_prev_save;
        size_t  way  = r_dcache_way_save;
        size_t  word = m_d_x[r_dcache_addr_save];
        size_t  set  = m_d_y[r_dcache_addr_save];
        assert((m_d_z[r_dcache_addr_save] | LINE_VALID) == dcache_tag(way, set));
        switch((data_op_t)r_dcache_type_save.read()) {
        case iss_t::DATA_WRITE: {
            data_t mask = be_to_mask(r_dcache_be_save.read());
#if XCACHE_WRAPPER_DEBUG
            std::cout << name() << " upd @" << std::hex << r_dcache_addr_save.read()
                      << " old: " << prev_data
                      << " new: " << new_data
                      << " mask: " << mask
                      << std::endl;
#endif
            dcache_data(way, set, word) = (prev_data & ~mask) | (new_data &  mask) ;
            break;
        }
        default:
            assert(!"There should be nothing but write requests in DCACHE_WRITE_UPDT");
        } // end switch
        m_cpt_dcache_data_write++;
        r_dcache_fsm = DCACHE_WRITE_REQ;
        }
    break;

    case DCACHE_MISS_REQ:
        fifo_put = true;
        if (m_dreq_addr_fifo.wok()) {
            r_dcache_fsm = DCACHE_MISS_WAIT;
            m_cpt_fifo_write++;
        }
    break;

    case DCACHE_MISS_WAIT:
        if (r_vci_rsp_fsm == RSP_DATA_READ_ERROR)
            r_dcache_fsm = DCACHE_ERROR;
        if (r_vci_rsp_fsm == RSP_DATA_MISS_OK)
            r_dcache_fsm = DCACHE_MISS_UPDT;
    break;

    case DCACHE_MISS_UPDT:
        {
        size_t set = (size_t)m_d_y[r_dcache_miss_addr];
        tag_t tag = (tag_t)m_d_z[r_dcache_miss_addr] | LINE_VALID;
        assert(m_cacheability_table[r_dcache_miss_addr]);

        // selecting a victim
        size_t way;
        for(way = 0 ; way < m_dcache_ways ; way++)
            if((dcache_tag(way, set) & 0x80000000) == 0)
                break;
        if (way >= m_dcache_ways)
            way = m_cpt_fifo_write % m_dcache_ways;
        // cache update
        dcache_tag(way, set) = tag;
        for (size_t i=0 ; i<m_icache_words ; i++) {
            dcache_data(way, set, i) = r_dcache_miss_buf[i];
        }
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        }
    break;

    case DCACHE_UNC_REQ:
        fifo_put = true;
        if (m_dreq_addr_fifo.wok()) {
            r_dcache_fsm = DCACHE_UNC_WAIT;
            m_cpt_fifo_write++;
        }
    break;

    case DCACHE_UNC_WAIT:
        if (r_vci_rsp_fsm == RSP_DATA_READ_ERROR)
            r_dcache_fsm = DCACHE_ERROR;
        if (r_vci_rsp_fsm == RSP_DATA_UNC_OK)
            r_dcache_fsm = DCACHE_IDLE;
    break;

    case DCACHE_ERROR:
        r_dcache_fsm = DCACHE_IDLE;
    break;

    case DCACHE_INVAL:
        {
        size_t set = (size_t)m_d_y[r_dcache_addr_save];
        tag_t tag = (tag_t)m_d_z[r_dcache_addr_save] | LINE_VALID;
        bool did_once = false;
#if XCACHE_WRAPPER_DEBUG
        std::cout << name() << " Invalidating st: " << std::hex << set << '/' << tag << std::endl;
#endif
        for (size_t way = 0 ; way < m_dcache_ways ; way++) {
            if (dcache_tag(way, set) == tag) {
                dcache_tag(way, set) = 0;
                did_once = true;
            }
        }
        assert(did_once);
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
        }
    break;

    } // end switch r_dcache_fsm

    ////////////////////////////////////////////////////////////////////////////
    // The VCI_CMD FSM controls the following ressources:
    // - r_vci_cmd_fsm
    // - r_dcache_cmd
    // - r_dcache_miss_addr
    // - r_cmd_cpt
    // - fifo_get
    // This FSM handles requests from both the DCACHE controler
    // (fifo non empty) and the ICACHE controler (r_icache_req).
    // There is  4 VCI transaction types :
    // - INS_MISS
    // - DATA_MISS
    // - DATA_UNC
    // - DATA_WRITE
    // The ICACHE requests have the highest priority.
    // There is at most one (CMD/RSP) VCI transaction, as both CMD_FSM and RSP_FSM
    // exit simultaneously the IDLE state.
    // In case of successive write at consecutive addressses, this FSM builds
    // write burst of variable lengths:
    // A request is consumed from the fifo each time the CMD and RSP FSMs
    // are in IDLE state, or the CMD FSM is in CMD_DATA_WRITE state, and there is
    // another write request at address + 4.
    //////////////////////////////////////////////////////////////////////////////

    switch ((cmd_fsm_state_e)r_vci_cmd_fsm.read()) {

    case CMD_IDLE:
        if (r_vci_rsp_fsm != RSP_IDLE)
            break;

        r_cmd_cpt = 0;
        if (r_icache_req) {  // instruction MISS first
            r_vci_cmd_fsm = CMD_INS_MISS;
        } else if (m_dreq_addr_fifo.rok()) {
            addr_t  req_addr    = m_dreq_addr_fifo.read();
            data_t  req_data    = m_dreq_data_fifo.read();
            typename iss_t::be_t  req_be    = m_dreq_be_fifo.read();
            bool    req_cached  = m_dreq_cached_fifo.read();
            data_op_t  req_type    = (data_op_t)m_dreq_type_fifo.read();
            r_dcache_addr_cmd = req_addr;
            r_dcache_be_cmd = req_be;
            r_dcache_data_cmd = req_data;
            r_dcache_type_cmd = req_type;
            r_dcache_cached_cmd = req_cached;
            m_cpt_fifo_read++;
            fifo_get = true;

            switch(req_type) {
            case iss_t::DATA_READ:
                if ( req_cached )
                    r_vci_cmd_fsm = CMD_DATA_MISS;
                else
                    r_vci_cmd_fsm = CMD_DATA_UNC;
                break;
            case iss_t::DATA_SC:
            case iss_t::DATA_LL:
                r_vci_cmd_fsm = CMD_DATA_UNC;
                break;
            case iss_t::DATA_WRITE:
                r_vci_cmd_fsm = CMD_DATA_WRITE;
                break;
            default:
                assert(0&&"This should not happen");
            }
        }
    break;

    case CMD_INS_MISS:
        if ( p_vci.cmdack.read() ) {
            r_cmd_cpt = r_cmd_cpt + 1;
            if (r_cmd_cpt == m_icache_words - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
    break;

    case CMD_DATA_UNC:
        r_dcache_miss_addr = r_dcache_addr_cmd;
        if ( p_vci.cmdack.read() )
            r_vci_cmd_fsm = CMD_IDLE;
    break;

    case CMD_DATA_MISS:
        r_dcache_miss_addr = r_dcache_addr_cmd;
        if ( p_vci.cmdack.read() ) {
            r_cmd_cpt = r_cmd_cpt + 1;
            if (r_cmd_cpt == m_dcache_words - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
    break;

    case CMD_DATA_WRITE:
        if ( ! p_vci.cmdack.read() )
            break;

        if ( m_dreq_addr_fifo.rok() &&
             can_burst( (data_op_t)m_dreq_type_fifo.read(),
                        m_dreq_addr_fifo.read(),
                        (data_op_t)r_dcache_type_cmd.read(),
                        r_dcache_addr_cmd.read() ) ) {
            fifo_get = true;
            r_dcache_addr_cmd = m_dreq_addr_fifo.read();
            r_dcache_be_cmd = m_dreq_be_fifo.read();
            r_dcache_data_cmd = m_dreq_data_fifo.read();
            r_dcache_type_cmd = m_dreq_type_fifo.read();
            r_dcache_cached_cmd = m_dreq_cached_fifo.read();
        } else {
            r_vci_cmd_fsm = CMD_IDLE;
        }
        break;
    } // end  switch r_vci_cmd_fsm

    //////////////////////////////////////////////////////////////////////////
    //  m_dreq_*_fifo
    //  This FIFO is used as a write buffer and contains the requests from
    //  the DCACHE controler to the VCI controler.
    //  It is controlled by the fifo_put signal (defined by r_dcache_fsm)
    //  and the fifo_get signal (defined by r_vci_cmd_fsm)
    //////////////////////////////////////////////////////////////////////////

    if ( fifo_put ) {
        if ( fifo_get ) {
            m_dreq_addr_fifo.put_and_get(r_dcache_addr_save);
            m_dreq_be_fifo.put_and_get(r_dcache_be_save);
            m_dreq_data_fifo.put_and_get(r_dcache_data_save);
            m_dreq_type_fifo.put_and_get(r_dcache_type_save);
            m_dreq_cached_fifo.put_and_get(r_dcache_cached_save);
        } else {
            m_dreq_addr_fifo.simple_put(r_dcache_addr_save);
            m_dreq_be_fifo.simple_put(r_dcache_be_save);
            m_dreq_data_fifo.simple_put(r_dcache_data_save);
            m_dreq_type_fifo.simple_put(r_dcache_type_save);
            m_dreq_cached_fifo.simple_put(r_dcache_cached_save);
        }
    } else {
        if ( fifo_get ) {
            m_dreq_addr_fifo.simple_get();
            m_dreq_be_fifo.simple_get();
            m_dreq_data_fifo.simple_get();
            m_dreq_type_fifo.simple_get();
            m_dreq_cached_fifo.simple_get();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // The VCI_RSP FSM controls the following ressources:
    // - r_vci_rsp_fsm:
    // - r_icache_miss_buf[m_icache_words]
    // - r_dcache_miss_buf[m_dcache_words]
    // - r_dcache_unc_valid set
    // - r_icache_req reset
    //
    // This FSM is synchronized with the VCI_CMD FSM, as both FSMs exit the
    // IDLE state simultaneously.
    //
    // Error handling:
    // This FSM analyzes the VCI error code.
    // In case of Write Bus Error the VCI_RSP FSM goes to the RSP_DATA_WRITE_ERROR
    // state, and the asynchronous error is directly signaled to the ISS.
    // In case of Read Bus Error, the VCI_RSP FSM goes to the RSP_INS_ERROR
    // or RSP_DATA_READ_ERROR state, and the error is signaled by the
    // ICACHE FSM or by the DCACHE FSM.
    //////////////////////////////////////////////////////////////////////////

    switch ((rsp_fsm_state_e)r_vci_rsp_fsm.read()) {

    case RSP_IDLE:
        assert( ! p_vci.rspval.read() && "Unexpected response" );

        if (r_vci_cmd_fsm != CMD_IDLE)
            break;

        r_rsp_cpt = 0;
        if (r_icache_req.read()) {
            r_vci_rsp_fsm = RSP_INS_MISS;
        } else if (m_dreq_addr_fifo.rok()) {
            data_op_t req_type = (data_op_t)m_dreq_type_fifo.read();
            bool req_cached = m_dreq_cached_fifo.read();

            switch (req_type) {
            case iss_t::DATA_READ:
                if ( req_cached ) {
                    r_vci_rsp_fsm = RSP_DATA_MISS;
                } else {
                    r_vci_rsp_fsm = RSP_DATA_UNC;
                }
                break;
            case iss_t::DATA_LL:
            case iss_t::DATA_SC:
                r_vci_rsp_fsm = RSP_DATA_UNC;
                break;
            case iss_t::DATA_WRITE:
                r_vci_rsp_fsm = RSP_DATA_WRITE;
                break;
            default:
                assert(0&&"This should not happen");
                break;
            }
        }
    break;

    case RSP_INS_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        assert(r_rsp_cpt != m_icache_words &&
               "illegal VCI response packet for instruction miss");
        r_rsp_cpt = r_rsp_cpt + 1;
        r_icache_miss_buf[r_rsp_cpt] = (data_t)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == vci_param::ERR_NORMAL ) {
                r_vci_rsp_fsm = RSP_INS_OK;
                assert(r_rsp_cpt == m_icache_words - 1 &&
                       "illegal VCI response packet for instruction miss");
            } else {
                r_vci_rsp_fsm = RSP_INS_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_INS_ERROR_WAIT;
        }
    break;

    case RSP_INS_OK:
        r_vci_rsp_fsm = RSP_IDLE;
        r_icache_req = false;
    break;

    case RSP_INS_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;
        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_INS_ERROR;
    break;

    case RSP_INS_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
        r_icache_req = false;
    break;

    case RSP_DATA_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        assert(r_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_rsp_cpt = r_rsp_cpt + 1;
        r_dcache_miss_buf[r_rsp_cpt] = (data_t)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == vci_param::ERR_NORMAL ) {
                r_vci_rsp_fsm = RSP_DATA_MISS_OK;
                assert(r_rsp_cpt == m_dcache_words - 1 &&
                       "illegal VCI response packet for data read miss");
            } else {
                r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_DATA_READ_ERROR_WAIT;
        }
    break;

    case RSP_DATA_WRITE:
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_IDLE;
            else
                r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
        } else {
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR_WAIT;
        }
    break;

    case RSP_DATA_UNC:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci.rerror.read() == vci_param::ERR_NORMAL ) {
            r_dcache_miss_buf[0] = (data_t)p_vci.rdata.read();
            r_dcache_unc_valid = true;
            r_vci_rsp_fsm = RSP_DATA_UNC_OK;
        } else {
            r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
        }
    break;

    case RSP_DATA_WRITE_ERROR:
        // signaling an asynchronous bus error
        m_iss.setWriteBerr();
        r_vci_rsp_fsm = RSP_IDLE;
    break;

    case RSP_DATA_UNC_OK:
    case RSP_DATA_MISS_OK:
    case RSP_DATA_READ_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
    break;

    case RSP_DATA_READ_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
    break;

    case RSP_DATA_WRITE_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
    break;
    } // end switch r_vci_rsp_fsm

} // end transition()

//////////////////////
tmpl(void)::genMoore()
///////////////////////
{
    // VCI RSP

    p_vci.rspack = true;

    // VCI CMD

    typename iss_t::be_t req_be = r_dcache_be_cmd.read();
    addr_t req_addr = r_dcache_addr_cmd.read();
    data_t req_data = r_dcache_data_cmd.read();
    data_op_t req_type = (data_op_t)r_dcache_type_cmd.read();

    switch ((cmd_fsm_state_e)r_vci_cmd_fsm.read()) {
    case CMD_IDLE:
        p_vci.cmdval  = false;
        break;

    case CMD_DATA_UNC:
        p_vci.cmdval = true;
        p_vci.address = req_addr;
        switch( (data_op_t)r_dcache_type_cmd.read() ) {
        case iss_t::DATA_READ:
            p_vci.wdata = 0;
            p_vci.be  = req_be;
            p_vci.cmd = vci_param::CMD_READ;
            break;
        case iss_t::DATA_LL:
            p_vci.wdata = 0;
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_LOCKED_READ;
            break;
        case iss_t::DATA_SC:
            p_vci.wdata = req_data;
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_STORE_COND;
            break;
        default:
            assert(0);
        }
        p_vci.plen = 0;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = false;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop    = true;
        break;

    case CMD_DATA_WRITE:
        p_vci.cmdval = true;
        p_vci.address = req_addr;
        p_vci.wdata   = req_data;
        p_vci.be = req_be;
        p_vci.plen   = 0;
        p_vci.cmd    = vci_param::CMD_WRITE;
        p_vci.trdid  = 0;
        p_vci.pktid  = WRITE_PKTID;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = false;
        p_vci.clen   = 0;
        p_vci.cfixed = false;

        p_vci.eop = ! (
            m_dreq_addr_fifo.rok() &&
            can_burst( (data_op_t)m_dreq_type_fifo.read(), m_dreq_addr_fifo.read(),
                       (data_op_t)req_type, req_addr ) );
        break;

    case CMD_DATA_MISS:
        p_vci.cmdval = true;
        p_vci.address = (req_addr & m_dcache_yzmask) + (r_cmd_cpt << 2);
        p_vci.be     = 0xF;
        p_vci.plen   = 0;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = (r_cmd_cpt == m_dcache_words - 1);
        break;

    case CMD_INS_MISS:
        p_vci.cmdval = true;
        p_vci.address = (r_icache_miss_addr & m_icache_yzmask) + (r_cmd_cpt << 2);
        p_vci.be     = 0xF;
        p_vci.plen   = 0;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = (r_cmd_cpt == m_icache_words - 1);
        break;

    } // end switch r_vci_cmd_fsm
} // end genMoore

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

