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
 *         Alain Greiner <alain.greiner@lip6.fr>
 *
 * Maintainers: alain eric.guthmuller@polytechnique.edu nipo
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
// - 15/07/2008
//   The data & instruction caches implementation have been modified, in order
//   to use the GenericCache object. Similarly, the write buffer implementation
//   has been modified to use the WriteBuffer object.
//   A VCI write burst is constructed when two conditions are satisfied :
//   The processor make strictly successive write requests, and they are
//   in the same cache line. The write buffer performs re-ordering, to
//   respect the contiguous addresses VCI constraint. In case of several
//   WRITE_WORD requests in the same word, only the last request is conserved.
//   In case of several WRITE_HALF or WRITE_BYTE requests in the same word,
//   the requests are merged in the same word. In case of uncached write
//   requests, each request is transmited as a single VCI transaction.
//   Both the data & instruction caches can be flushed in one single cycle.
//   Finally, new activity counters have been introduced for instrumentation.
// - 19/08/2008
//   The VCI CMD FSM has been modified to send single cell packet in case of MISS.
//   The uncached mode (using the mapping table) has been introduced  in the
//   ICACHE FSM.
// - 26/12/2009
//   The VCI_XCACHE_WRAPPER_MULTI has been derived from the VCI_XCACHE_WRAPPER
//   to support several concurrent VCI transactions, in order to improve the CPI.
//   A new write buffer supporting simultaneous write transactions has been 
//   introduced. The VCI command & response FSMs are not synchronized anymore:
//   The read requests can be transmitted before previous write requests
//   (if the missing address does not match apending write in the write buffer).
//   The transactions can complete in any order, depending on the network.
//   Two new flip-flops have been introduced to signal completion of the
//   read transactions from the RSP FSM to the DCACHE & ICACHE FSMs: 
//   r_rsp_data_ok, r_rsp_ins_ok
//   As simultaneous VCI transactions are supported, the PKTID & RPKTID fields
//   are used to transport the transaction index.
//   The transaction index has an odd value for a write transaction, depending
//   on the line index in the write buffer: PKTID = 2*wbuf_index + 1
//   The transaction index has an even value for a read transaction, with only
//   only four possible values, depending on cached/uncached & data/instruction.
///////////////////////////////////////////////////////////////////////////////

//#define SOCLIB_MODULE_DEBUG 1

#include <cassert>
#include <limits>
#include "arithmetics.h"
#include "../include/vci_xcache_wrapper_multi.h"

namespace soclib {
namespace caba {

#ifdef SOCLIB_MODULE_DEBUG
namespace {
const char *dcache_fsm_state_str[] = {
        "DCACHE_IDLE      ",
        "DCACHE_WRITE_UPDT",
        "DCACHE_WRITE_REQ ",
        "DCACHE_MISS_WAIT ",
        "DCACHE_MISS_UPDT ",
        "DCACHE_UNC_WAIT  ",
        "DCACHE_INVAL     ",
        "DCACHE_SYNC      ",
        "DCACHE_ERROR     ",
    };
const char *icache_fsm_state_str[] = {
        "ICACHE_IDLE     ",
        "ICACHE_MISS_WAIT",
        "ICACHE_MISS_UPDT",
        "ICACHE_UNC_WAIT ",
        "ICACHE_ERROR    ",
    };
const char *cmd_fsm_state_str[] = {
        "CMD_IDLE      ",
        "CMD_INS_MISS  ",
        "CMD_INS_UNC   ",
        "CMD_DATA_MISS ",
        "CMD_DATA_UNC  ",
        "CMD_DATA_WRITE",
    };
const char *rsp_fsm_state_str[] = {
        "RSP_IDLE      ",
        "RSP_INS_MISS  ",
        "RSP_INS_UNC   ",
        "RSP_DATA_MISS ",
        "RSP_DATA_UNC  ",
        "RSP_DATA_WRITE",
    };
}
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciXcacheWrapperMulti<vci_param, iss_t>

using soclib::common::uint32_log2;

////////////////////////////////////
tmpl(/**/)::VciXcacheWrapperMulti(
////////////////////////////////////
    sc_module_name name,
    int proc_id,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &index,
    size_t icache_ways,
    size_t icache_sets,
    size_t icache_words,
    size_t dcache_ways,
    size_t dcache_sets,
    size_t dcache_words, 
    size_t wbuf_nwords,
    size_t wbuf_nlines)
    :
      soclib::caba::BaseModule(name),

      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci"),

      m_cacheability_table(mt.getCacheabilityTable()),
      m_iss(this->name(), proc_id),
      m_srcid(mt.indexForId(index)),

      m_dcache_ways(dcache_ways),
      m_dcache_words(dcache_words),
      m_dcache_yzmask((~0)<<(uint32_log2(dcache_words) + 2)),
      m_icache_ways(icache_ways),
      m_icache_words(icache_words),
      m_icache_yzmask((~0)<<(uint32_log2(icache_words) + 2)),

      r_dcache_fsm("r_dcache_fsm"),
      r_dcache_addr_save("r_dcache_addr_save"),
      r_dcache_wdata_save("r_dcache_wdata_save"),
      r_dcache_rdata_save("r_dcache_rdata_save"),
      r_dcache_type_save("r_dcache_type_save"),
      r_dcache_be_save("r_dcache_be_save"),
      r_dcache_cached_save("r_dcache_cached_save"),
      r_dcache_miss_req("r_dcache_miss_req"),
      r_dcache_unc_req("r_dcache_unc_req"),

      r_icache_fsm("r_icache_fsm"),
      r_icache_addr_save("r_icache_addr_save"),
      r_icache_miss_req("r_icache_miss_req"),
      r_icache_unc_req("r_icache_unc_req"),

      r_cmd_fsm("r_cmd_fsm"),
      r_cmd_min("r_cmd_min"),
      r_cmd_max("r_cmd_max"),
      r_cmd_cpt("r_cmd_cpt"),

      r_rsp_fsm("r_rsp_fsm"),
      r_rsp_ins_error("r_rsp_ins_error"),
      r_rsp_data_error("r_rsp_data_error"),
      r_rsp_cpt("r_rsp_cpt"),
      r_rsp_ins_ok("r_rsp_ins_ok"),
      r_rsp_data_ok("r_rsp_data_ok"),

      r_wbuf("wbuf", wbuf_nwords, wbuf_nlines),
      r_icache("icache", icache_ways, icache_sets, icache_words),
      r_dcache("dcache", dcache_ways, dcache_sets, dcache_words)
{
    r_icache_miss_buf = new data_t[icache_words];
    r_dcache_miss_buf = new data_t[dcache_words];

    assert((icache_words*vci_param::B) < (1<<vci_param::K) && "I need more PLEN bits");

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();

    typename iss_t::CacheInfo cache_info;
    cache_info.has_mmu = false;
    cache_info.icache_line_size = icache_words*sizeof(data_t);
    cache_info.icache_assoc = icache_ways;
    cache_info.icache_n_lines = icache_sets;
    cache_info.dcache_line_size = dcache_words*sizeof(data_t);
    cache_info.dcache_assoc = dcache_ways;
    cache_info.dcache_n_lines = dcache_sets;
    m_iss.setCacheInfo(cache_info);
}

////////////////////////////////////
tmpl(/**/)::~VciXcacheWrapperMulti()
////////////////////////////////////
{
    delete [] r_icache_miss_buf;
    delete [] r_dcache_miss_buf;
}

////////////////////////
tmpl(void)::print_cpi()
////////////////////////
{
    std::cout << name() << " CPU " << m_srcid << " : CPI = "
              << (float)m_cpt_total_cycles/(m_cpt_total_cycles - m_cpt_frz_cycles) << std::endl;
}
////////////////////////
tmpl(void)::print_stats()
////////////////////////
{
    float run_cycles = (float)(m_cpt_total_cycles - m_cpt_frz_cycles);
        std::cout << "------------------------------------" << std:: dec << std::endl
        << "CPU " << m_srcid << " / Time = " << m_cpt_total_cycles << std::endl
        << "- CPI               = " << (float)m_cpt_total_cycles/run_cycles << std::endl
        << "- READ RATE         = " << (float)m_cpt_read/run_cycles << std::endl
        << "- UNC READ RATE     = " << (float)m_cpt_unc_read/m_cpt_read << std::endl
        << "- WRITE RATE        = " << (float)m_cpt_write/run_cycles << std::endl
        << "- CACHED WRITE RATE = " << (float)m_cpt_write_cached/m_cpt_write << std::endl
        << "- IMISS_RATE        = " << (float)m_cpt_ins_miss/run_cycles << std::endl
        << "- IMISS COST        = " << (float)m_cost_ins_miss_frz/m_cpt_ins_miss << std::endl
        << "- DMISS RATE        = " << (float)m_cpt_data_miss/(m_cpt_read-m_cpt_unc_read) << std::endl
        << "- DMISS COST        = " << (float)m_cost_data_miss_frz/m_cpt_data_miss << std::endl
        << "- UNC COST          = " << (float)m_cost_unc_read_frz/m_cpt_unc_read << std::endl
        << "- WRITE COST        = " << (float)m_cost_write_frz/m_cpt_write << std::endl
        << "- WRITE LENGTH      = " << (float)m_length_write_transaction/m_cpt_write_transaction
        << std::endl;
}

//////////////////////////
tmpl(void)::transition()
//////////////////////////
{
    if ( ! p_resetn.read() ) {
        m_iss.reset();

        // FSM states
        r_dcache_fsm = DCACHE_IDLE;
        r_icache_fsm = ICACHE_IDLE;
        r_cmd_fsm = CMD_IDLE;
        r_rsp_fsm = RSP_IDLE;

        // write buffer & caches
        r_wbuf.reset();
        r_icache.reset();
        r_dcache.reset();

        // synchronisation flip-flops from ICACHE & DCACHE FSMs to CMD  FSM
        r_icache_miss_req    = false;
        r_dcache_miss_req    = false;
        r_icache_unc_req     = false;
        r_dcache_unc_req     = false;

        // synchronisation flip-flops from the RSP FSM to the ICACHE or DCACHE FSMs
        r_rsp_data_ok        = false;
        r_rsp_ins_ok         = false;
        r_rsp_data_error     = false;
        r_rsp_ins_error      = false;

        // activity counters
        m_cpt_dcache_data_read  = 0;
        m_cpt_dcache_data_write = 0;
        m_cpt_dcache_dir_read  = 0;
        m_cpt_dcache_dir_write = 0;
        m_cpt_icache_data_read  = 0;
        m_cpt_icache_data_write = 0;
        m_cpt_icache_dir_read  = 0;
        m_cpt_icache_dir_write = 0;

        m_cpt_frz_cycles = 0;
        m_cpt_total_cycles = 0;

        m_cpt_read = 0;
        m_cpt_write = 0;
        m_cpt_data_miss = 0;
        m_cpt_ins_miss = 0;
        m_cpt_unc_read = 0;
        m_cpt_write_cached = 0;

        m_cost_write_frz = 0;
        m_cost_data_miss_frz = 0;
        m_cost_unc_read_frz = 0;
        m_cost_ins_miss_frz = 0;

        m_cpt_imiss_transaction = 0;
        m_cpt_dmiss_transaction = 0;
        m_cpt_unc_transaction = 0;
        m_cpt_write_transaction = 0;

        m_length_write_transaction = 0;

        return;
    }

#ifdef SOCLIB_MODULE_DEBUG
std::cout << "--------------------------------------------" << std::endl
          << std::dec << "CACHE " << m_srcid_d << " / Time = " << m_cpt_total_cycles << std::endl
          << "  " << dcache_fsm_state_str[r_dcache_fsm]
          << "  " << icache_fsm_state_str[r_icache_fsm]
          << "  " << cmd_fsm_state_str[r_cmd_fsm]
          << "  " << rsp_fsm_state_str[r_rsp_fsm]
r_wbuf.print();
#endif

    m_cpt_total_cycles++;

    /////////////////////////////////////////////////////////////////////
    // The ICACHE FSM controls the following ressources:
    // - r_icache_fsm
    // - r_icache (instruction cache access)
    // - r_icache_addr_save
    // - r_icache_miss_req set
    // - r_icache_unc_req set
    // - r_rsp_ins_ok reset
    // - r_rsp_ins_error reset
    // - ireq & irsp structures for communication with the processor
    //
    // Processor requests are taken into account only in the IDLE state.
    // In case of MISS, or in case of uncached instruction, the FSM 
    // writes the missing address line in the  r_icache_addr_save register 
    // and sets the r_icache_miss_req (or the r_icache_unc_req) flip-flop.
    // The request flip-flop is reset by the CMD FSM when the VCI command
    // has been send. 
    // The RSP FSM sets the r_rsp_ins_ok flip-flop
    // to signal the availability of data in the r_icache_buf buffer.
    // These two flip-flops are reset by the ICACHE_FSM. 
    // In case of bus error, the VCI_RSP FSM sets the r_rsp_ins_error
    // flip-flop. It is reset by the ICACHE FSM.
    ///////////////////////////////////////////////////////////////////////

    typename iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

    typename iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;

    m_iss.getRequests( ireq, dreq );

    switch(r_icache_fsm) {

    case ICACHE_IDLE:
        if ( ireq.valid ) 
        {
            data_t  icache_ins = 0;
            bool    icache_hit = false;
            bool    icache_cached = m_cacheability_table[ireq.addr];
            // icache_hit & icache_ins evaluation
            if ( icache_cached ) 
            {
                icache_hit = r_icache.read(ireq.addr, &icache_ins);
            } 
            else 
            {
                icache_hit = ( r_rsp_ins_ok && (ireq.addr == r_icache_addr_save) );
                icache_ins = r_icache_miss_buf[0];
            }
            if ( ! icache_hit )   // miss
            {
                m_cpt_ins_miss++;
                m_cost_ins_miss_frz++;
                r_icache_addr_save = ireq.addr;
                if ( icache_cached ) 
                {
                    r_icache_fsm 	= ICACHE_MISS_WAIT;
                    r_icache_miss_req 	= true;
                    r_rsp_ins_ok 	= false;
                } 
                else 
                {
                    r_icache_fsm 	= ICACHE_UNC_WAIT;
                    r_icache_unc_req 	= true;
                    r_rsp_ins_ok 	= false;
                } 
            }
            else   // hit : the uncached data should not be re-used
	    {
                r_rsp_ins_ok = false;
            } 
            m_cpt_icache_dir_read += m_icache_ways;
            m_cpt_icache_data_read += m_icache_ways;
            irsp.valid          = icache_hit;
            irsp.instruction    = icache_ins;
        }
        break;

    case ICACHE_MISS_WAIT:
        m_cost_ins_miss_frz++;
        if ( r_rsp_ins_ok ) 
        {
            if ( r_rsp_ins_error ) 	r_icache_fsm = ICACHE_ERROR;
            else 			r_icache_fsm = ICACHE_MISS_UPDT;
        }
        break;

    case ICACHE_UNC_WAIT:
        m_cost_ins_miss_frz++;
        if ( r_rsp_ins_ok ) 
        {
            if ( r_rsp_ins_error )  	r_icache_fsm = ICACHE_ERROR;  
            else  			r_icache_fsm = ICACHE_IDLE; 
        }
        break;

    case ICACHE_ERROR:
        r_icache_fsm = ICACHE_IDLE;
        r_rsp_ins_error     = false;
        irsp.valid          = true;
        irsp.error          = true;
        break;

    case ICACHE_MISS_UPDT:
    {
        addr_t  ad  = r_icache_addr_save;
        data_t* buf = r_icache_miss_buf;
        data_t  victim_index = 0;
        m_cpt_icache_dir_write++;
        m_cpt_icache_data_write++;
        m_cost_ins_miss_frz++;
        r_icache.update(ad, buf, &victim_index);
        r_icache_fsm = ICACHE_IDLE;
        break;
    }

    } // end switch r_icache_fsm

#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Instruction Response: " << irsp << std::endl;
#endif

    ///////////////////////////////////////////////////////////////////////////////////
    // The DCACHE FSM controls the following ressources:
    // - r_dcache_fsm
    // - r_dcache (data cache access)
    // - r_dcache_addr_save
    // - r_dcache_wdata_save
    // - r_dcache_rdata_save
    // - r_dcache_type_save
    // - r_dcache_be_save
    // - r_dcache_cached_save
    // - r_dcache_miss_req set
    // - r_dcache_unc_req set
    // - r_rsp_data_ok reset
    // - r_rsp_data_error reset
    // - r_wbuf write()
    // - dreq & drsp structures for communication with the processor
    //
    // In order to support VCI write burst, the processor requests are taken into account
    // in the WRITE_REQ state as well as in the IDLE state.
    // - In the IDLE state, the processor request cannot be satisfied if
    //   there is a cached read miss, or an uncached read.
    // - In the WRITE_REQ state, the request cannot be satisfied if
    //   there is a cached read miss, or an uncached read,
    //   or when the write buffer is full.
    // - In all other states, the processor request is not satisfied.
    //
    // The cache access takes into account the cacheability_table.
    // In case of processor request, there is five conditions to exit the IDLE state:
    //   - CACHED READ MISS => to the MISS_WAIT state (waiting r_rsp_data_ok),
    //     then to the MISS_UPDT state, and finally to the IDLE state.
    //   - UNCACHED READ  => to the UNC_WAIT state (waiting r_rsp_data_ok),
    //     and to the IDLE state.
    //   - CACHE INVALIDATE HIT => to the INVAL state for one cycle, then to IDLE state.
    //   - WRITE MISS => directly to the WRITE_REQ state to access the write buffer.
    //   - WRITE HIT => to the WRITE_UPDT state, then to the WRITE_REQ state.
    //
    // Error handling :  Read Bus Errors are synchronous events, but
    // Write Bus Errors are asynchronous events (processor is not frozen).
    // - If a Read Bus Error is detected, the VCI_RSP FSM sets the
    //   r_rsp_data_error flip-flop, and the synchronous error is signaled
    //   by the DCACHE FSM.
    // - If a Write Bus Error is detected, the VCI_RSP FSM  signals
    //   the asynchronous error using the setWriteBerr() method.
    ////////////////////////////////////////////////////////////////////////

    switch ( r_dcache_fsm ) {

    case DCACHE_WRITE_REQ:
    {
        if( !r_wbuf.wok(r_dcache_addr_save) ) 
        {
            //  stay in DCACHE_WRITEREQ state if the write request
            //  is not accepted by the write buffer
            m_cost_write_frz++;
            drsp.valid = false;
            drsp.rdata = 0;
            break;     
        }
        // If the write request is accepted by the write buffer, 
        // the next state and the processor request parameters are computed
        // as in the DCACHE_IDLE state  below ...
    }
    case DCACHE_IDLE:
    {
        if ( dreq.valid ) 
        {
            bool        dcache_hit 	= false;;
            data_t      dcache_rdata	= 0;
            bool        dcache_cached;
            m_cpt_dcache_data_read += m_dcache_ways;
            m_cpt_dcache_dir_read += m_dcache_ways;

            // dcache_cached evaluation
            switch (dreq.type) {
            case iss_t::DATA_LL:
            case iss_t::DATA_SC:
            case iss_t::XTN_READ:
            case iss_t::XTN_WRITE:
                dcache_cached = false;
                break;
            default:
                dcache_cached = m_cacheability_table[dreq.addr];
            }

            // dcache_hit & dcache_rdata evaluation
            if ( dcache_cached ) 
            {
                dcache_hit = r_dcache.read(dreq.addr, &dcache_rdata);
            } 
            else 
            {
                dcache_hit = ( (dreq.addr == r_dcache_addr_save) && r_rsp_data_ok );
                dcache_rdata = r_dcache_miss_buf[0];
            }

            // next state and response computation
            switch( dreq.type ) {
                case iss_t::DATA_READ:
                case iss_t::DATA_LL:
                case iss_t::DATA_SC:
                    m_cpt_read++;
                    if ( dcache_hit ) 
                    {
                        r_dcache_fsm = DCACHE_IDLE;
                        drsp.valid = true;
                        drsp.rdata = dcache_rdata;
                        r_rsp_data_ok  = false;
                    } 
                    else 
                    {
                        if ( dcache_cached ) 
                        {
                            m_cpt_data_miss++;
                            m_cost_data_miss_frz++;
                            r_dcache_miss_req  	= true;
                            r_rsp_data_ok 	= false;
                            r_dcache_fsm 	= DCACHE_MISS_WAIT;
                            drsp.valid		= false;
                            drsp.rdata		= 0;
                        } 
                        else 
                        {
                            m_cpt_unc_read++;
                            m_cost_unc_read_frz++;
                            r_dcache_unc_req   	= true;
                            r_rsp_data_ok  	= false;
                            r_dcache_fsm 	= DCACHE_UNC_WAIT;
                            drsp.valid		= false;
                            drsp.rdata		= 0;
                        }
                    }
                    break;
                case iss_t::XTN_READ:
                case iss_t::XTN_WRITE:
                    // only INVAL & SYNC requests are supported
                    if ( dreq.addr/4 == iss_t::XTN_DCACHE_INVAL )
                    {
                        r_dcache_fsm = DCACHE_INVAL;
                    }
                    else if ( dreq.addr/4 == iss_t::XTN_SYNC )
                    {
                        r_dcache_fsm = DCACHE_SYNC;
                    }
                    else
                    {
                        std::cout << "error in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "unsupported  external access" << dreq.addr/4 << std::endl;
                        std::cout << "only XTN_DCACHE_INVAL & XTN_SYNC are supported" << std::endl;
                        exit(0);
                    }
                    drsp.valid = true;
                    drsp.rdata = 0;
                    break;
                case iss_t::DATA_WRITE:
                    m_cpt_write++;
                    if ( dcache_hit && dcache_cached ) {
                        r_dcache_fsm = DCACHE_WRITE_UPDT;
                        m_cpt_write_cached++;
                    } else {
                        r_dcache_fsm = DCACHE_WRITE_REQ;
                    }
                    drsp.valid = true;
                    drsp.rdata = 0;
                    break;
            } // end switch dreq.type

            r_dcache_addr_save      = dreq.addr;
            r_dcache_type_save      = dreq.type;
            r_dcache_wdata_save     = dreq.wdata;
            r_dcache_be_save        = dreq.be;
            r_dcache_rdata_save     = dcache_rdata;
            r_dcache_cached_save    = dcache_cached;

        } 
        else // no dreq.valid
        {
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true;
            drsp.rdata = 0;
        }
        break;
    }
    case DCACHE_WRITE_UPDT:
    {
        m_cpt_dcache_data_write++;
        data_t mask = vci_param::be2mask(r_dcache_be_save);
        data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
        r_dcache.write(r_dcache_addr_save, wdata);
        r_dcache_fsm = DCACHE_WRITE_REQ;
        break;
    }
    case DCACHE_MISS_WAIT:
        m_cost_data_miss_frz++;
        if ( r_rsp_data_ok ) 
        {
            if ( r_rsp_data_error ) r_dcache_fsm = DCACHE_ERROR;
            else                    r_dcache_fsm = DCACHE_MISS_UPDT;
        }
        break;

    case DCACHE_MISS_UPDT:
    {
        addr_t  ad  = r_dcache_addr_save;
        data_t* buf = r_dcache_miss_buf;
        data_t  victim_index = 0;
        m_cost_data_miss_frz++;
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        r_dcache.update(ad, buf, &victim_index);
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }

    case DCACHE_UNC_WAIT:
    {
        m_cost_unc_read_frz++;
        if ( r_rsp_data_ok ) 
        {
            if ( r_rsp_data_error )   r_dcache_fsm = DCACHE_ERROR;
            else 
            {
                // If request is a DATA_SC we need to invalidate the corresponding cache line, 
                // so that subsequent access to this line are read from RAM
                if (dreq.type == iss_t::DATA_SC)  
                {
                    r_dcache_fsm = DCACHE_INVAL;
                    r_dcache_wdata_save = r_dcache_addr_save;
                }
                else
                {
                    r_dcache_fsm = DCACHE_IDLE;
                }
            }
        }
        break;
    }
    case DCACHE_ERROR:
    {
        r_dcache_fsm = DCACHE_IDLE;
        r_rsp_data_error = false;
        drsp.error = true;
        drsp.valid = true;
        break;
    }
    case DCACHE_INVAL:
    {
        m_cpt_dcache_dir_read += m_dcache_ways;
        r_dcache.inval(r_dcache_wdata_save);
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }
    case DCACHE_SYNC:
    {
        if ( r_wbuf.empty() ) r_dcache_fsm = DCACHE_IDLE;
    }

    } // end DCACHE_FSM switch

    ////////// write buffer handling ///////////////////////////////
    if( r_dcache_fsm == DCACHE_WRITE_REQ ) 
    {
        r_wbuf.write(true, r_dcache_addr_save, r_dcache_be_save, r_dcache_wdata_save); 
    } else {
        r_wbuf.write(false, 0, 0, 0);
    }

#ifdef SOCLIB_MODULE_DEBUG
std::cout << ireq << std::endl << irsp << std::endl << dreq << std::endl << drsp << std::endl;
#endif

    /////////// execute one iss cycle /////////////////////////////////
    {
        uint32_t it = 0;
        for (size_t i=0; i<(size_t)iss_t::n_irq; i++)
            if(p_irq[i].read())
                it |= (1<<i);

        m_iss.executeNCycles(1, irsp, drsp, it);
    }

    if ( (ireq.valid && !irsp.valid) || (dreq.valid && !drsp.valid) )
        m_cpt_frz_cycles++;


    ////////////////////////////////////////////////////////////////////////////
    // The CMD FSM controls the following ressources:
    // - r_cmd_fsm
    // - r_cmd_min
    // - r_cmd_max
    // - wbuf sent()
    //
    // This FSM handles requests from both the DCACHE FSM & the ICACHE FSM.
    // There is 5 request types, with the following priorities :
    // 1 - Data Write           : r_wbuf.rok()
    // 2 - Data Read Miss       : r_dcache_miss_req
    // 3 - Data Read Uncached   : r_dcache_unc_req
    // 4 - Instruction Miss     : r_icache_miss_req
    // 5 - Instruction Uncached : r_icache_unc_req
    //
    // VCI formats:
    // According to the VCI advanced specification, all read command packets
    // (uncached read or cached miss) are one word packets.
    // For write burst packets, all words must be in the same write buffer line,
    // and addresses must be contiguous (the BE field is 0 in case of "holes").
    // The PLEN VCI field is always documented.
    // As simultaneous VCI transactions are supported, the PKTID field is used:
    // - Write transactions : PKTID = 2*wbuf_index + 1 		(odd values)
    // - Read transactions  : PKTID = 4*cached + 2*instruction	(even values)
    //////////////////////////////////////////////////////////////////////////////

    switch (r_cmd_fsm) {

    case CMD_IDLE:
        if ( r_dcache_miss_req && r_wbuf.miss( r_dcache_addr_save ) )
        {
            r_cmd_fsm = CMD_DATA_MISS;
            r_dcache_miss_req = false;
            m_cpt_dmiss_transaction++;
        } 
        else if ( r_dcache_unc_req && r_wbuf.miss( r_dcache_addr_save ) ) 
        {
            r_cmd_fsm = CMD_DATA_UNC;
            r_dcache_unc_req = false;
            m_cpt_unc_transaction++;
        }
        else if ( r_icache_miss_req && r_wbuf.miss( r_dcache_addr_save ) ) 
        {
            r_cmd_fsm = CMD_INS_MISS;
            r_icache_miss_req = false;
            m_cpt_imiss_transaction++;
        } 
        else if ( r_icache_unc_req && r_wbuf.miss( r_dcache_addr_save ) ) 
        {
            r_cmd_fsm = CMD_INS_UNC;
            r_icache_unc_req = false;
            m_cpt_imiss_transaction++;
        } 
        else if ( r_wbuf.rok() ) 
        {
            r_cmd_fsm   = CMD_DATA_WRITE;
            r_cmd_min   = r_wbuf.getMin();
            r_cmd_max   = r_wbuf.getMax();
            r_cmd_cpt   = r_wbuf.getMin();
            m_cpt_write_transaction++;
            m_length_write_transaction += (r_wbuf.getMax() - r_wbuf.getMin() + 1);
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci.cmdack.read() ) 
        {
            r_cmd_cpt = r_cmd_cpt + 1;
            if (r_cmd_cpt == r_cmd_max) 
            {
                r_cmd_fsm = CMD_IDLE ;
                r_wbuf.sent() ;
            }
        }
        break;

    case CMD_DATA_MISS:
    case CMD_DATA_UNC:
    case CMD_INS_MISS:
    case CMD_INS_UNC:
        if ( p_vci.cmdack.read() )  r_cmd_fsm = CMD_IDLE;
        break;
    } // end  switch r_cmd_fsm

    //////////////////////////////////////////////////////////////////////////
    // The RSP FSM controls the following ressources:
    // - r_rsp_fsm:
    // - r_icache_miss_buf[m_icache_words]
    // - r_dcache_miss_buf[m_dcache_words]
    // - r_icache_miss_req reset
    // - r_icache_unc_req reset
    // - r_dcache_miss_req reset
    // - r_dcache_unc_req reset
    // - r_rsp_data_ok set
    // - r_rsp_ins_ok set
    // - r_rsp_data_error set
    // - r_rsp_ins_error set
    // - r_rsp_cpt
    //
    // VCI formats:
    // This component accepts single word or multi-word response packets for
    // write response packets.
    //
    // Error handling:
    // This FSM analyzes the VCI error code and signals the Write Bus Error.
    // In case of Read Data Error, the VCI_RSP FSM sets the r_rsp_data_error
    // flip_flop and the error is signaled by the DCACHE FSM.
    // In case of Instruction Error, the VCI_RSP FSM sets the r_rsp_ins_error
    // flip_flop and the error is signaled by the DCACHE FSM.
    //////////////////////////////////////////////////////////////////////////

    switch (r_rsp_fsm) {

    case RSP_IDLE:
        if( p_vci.rspval.read() ) 
        {
            r_rsp_cpt = 0;
            if ( p_vci.rpktid.read()%2 == 1 ) 			r_rsp_fsm = RSP_DATA_WRITE;
            else if ( p_vci.rpktid.read() == TYPE_DATA_MISS ) 	r_rsp_fsm = RSP_DATA_MISS;
            else if ( p_vci.rpktid.read() == TYPE_DATA_UNC ) 	r_rsp_fsm = RSP_DATA_UNC;
            else if ( p_vci.rpktid.read() == TYPE_INS_MISS ) 	r_rsp_fsm = RSP_INS_MISS;
            else if ( p_vci.rpktid.read() == TYPE_INS_UNC ) 	r_rsp_fsm = RSP_INS_UNC;
        } 
        break;

    case RSP_DATA_WRITE:
        if ( p_vci.rspval.read() )
        {
            if ( p_vci.reop.read() ) 
            {
                r_rsp_fsm = RSP_IDLE;
                r_wbuf.completed( p_vci.rpktid.read() >> 1 );
            }
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )  m_iss.setWriteBerr();
        }
        break;

    case RSP_INS_MISS:
        if ( p_vci.rspval.read() )
        {
            assert( (r_rsp_cpt < m_icache_words) &&
               "The VCI response packet for instruction miss is too long" );
            r_rsp_cpt = r_rsp_cpt + 1;
            r_icache_miss_buf[r_rsp_cpt] = (data_t)p_vci.rdata.read();
            if ( p_vci.reop.read() ) 
            {
                assert( (r_rsp_cpt == m_icache_words - 1) &&
                   "The VCI response packet for instruction miss is too short");
                r_rsp_ins_ok = true;
                r_rsp_fsm = RSP_IDLE;
            }
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_ins_error = true;
        }
        break;

    case RSP_INS_UNC:
        if ( p_vci.rspval.read() )
        {
            assert(p_vci.reop.read() &&
               "illegal VCI response packet for uncached instruction");
            r_icache_miss_buf[0] = (data_t)p_vci.rdata.read();
            r_rsp_ins_ok = true;
            r_rsp_fsm = RSP_IDLE;
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_ins_error = true;
        }
        break;

    case RSP_DATA_MISS:
        if ( p_vci.rspval.read() )
        {
            assert(r_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");
            r_rsp_cpt = r_rsp_cpt + 1;
            r_dcache_miss_buf[r_rsp_cpt] = (data_t)p_vci.rdata.read();
            if ( p_vci.reop.read() ) 
            {
                assert(r_rsp_cpt == m_dcache_words - 1 &&
                   "illegal VCI response packet for instruction miss");
                r_rsp_data_ok = true;
                r_rsp_fsm = RSP_IDLE;
            }
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_data_error = true;
        }
        break;

    case RSP_DATA_UNC:
        if ( p_vci.rspval.read() )
        {
            assert(p_vci.reop.read() &&
               "illegal VCI response packet for uncached read data");
            r_dcache_miss_buf[0] = (data_t)p_vci.rdata.read();
            r_rsp_data_ok = true;
            r_rsp_fsm = RSP_IDLE;
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_data_error = true;
        }
        break;
    } // end switch r_rsp_fsm

} // end transition()

//////////////////////////////////////////////////////////////////////////////////
tmpl(void)::genMoore()
//////////////////////////////////////////////////////////////////////////////////
{
    // VCI initiator response

    p_vci.rspack = ( r_rsp_fsm != RSP_IDLE );

    // VCI initiator command

    switch (r_cmd_fsm) {

    case CMD_IDLE:
        p_vci.cmdval  = false;
        p_vci.address = 0;
        p_vci.wdata   = 0;
        p_vci.be      = 0;
        p_vci.plen    = 0;
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.trdid   = 0;
        p_vci.pktid   = 0;
        p_vci.srcid   = 0;
        p_vci.cons    = false;
        p_vci.wrap    = false;
        p_vci.contig  = false;
        p_vci.clen    = 0;
        p_vci.cfixed  = false;
        p_vci.eop     = false;
        break;

    case CMD_DATA_WRITE:
        p_vci.cmdval  = true;
        p_vci.address = r_wbuf.getAddress(r_cmd_cpt);
        p_vci.wdata   = r_wbuf.getData(r_cmd_cpt);
        p_vci.be      = r_wbuf.getBe(r_cmd_cpt);
        p_vci.plen    = (r_cmd_max - r_cmd_min + 1)<<2;
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.trdid   = 0;
        p_vci.pktid   = (r_wbuf.getIndex() << 1) + 1;
        p_vci.srcid   = m_srcid;
        p_vci.cons    = false;
        p_vci.wrap    = false;
        p_vci.contig  = true;
        p_vci.clen    = 0;
        p_vci.cfixed  = false;
        p_vci.eop     = (r_cmd_cpt == r_cmd_max);
        break;

    case CMD_DATA_MISS:
        p_vci.cmdval = true;
        p_vci.address = r_dcache_addr_save & m_dcache_yzmask;
        p_vci.be     = 0xF;
        p_vci.plen   = m_dcache_words << 2;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = TYPE_DATA_MISS;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = true;
        break;

    case CMD_DATA_UNC:
        p_vci.cmdval = true;
        p_vci.address = r_dcache_addr_save & ~0x3;
        switch( r_dcache_type_save ) {
        case iss_t::DATA_READ:
            p_vci.wdata = 0;
            p_vci.be  = r_dcache_be_save.read();
            p_vci.cmd = vci_param::CMD_READ;
            break;
        case iss_t::DATA_LL:
            p_vci.wdata = 0;
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_LOCKED_READ;
            break;
        case iss_t::DATA_SC:
            p_vci.wdata = r_dcache_wdata_save.read();
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_STORE_COND;
            break;
        default:
            assert("this should not happen");
        }
        p_vci.plen = 4;
        p_vci.trdid  = 0;
        p_vci.pktid  = TYPE_DATA_UNC;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop    = true;
        break;

    case CMD_INS_MISS:
        p_vci.cmdval = true;
        p_vci.address = r_icache_addr_save & m_icache_yzmask;
        p_vci.be     = 0xF;
        p_vci.plen   = m_icache_words << 2;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = TYPE_INS_MISS;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = true;
        break;

    case CMD_INS_UNC:
        p_vci.cmdval = true;
        p_vci.address = r_icache_addr_save & ~0x3;
        p_vci.be     = 0xF;
        p_vci.plen   = 4;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = TYPE_INS_UNC;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = true;
        break;
    } // end switch r_cmd_fsm

} // end genMoore()

}} // end namespace

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4




