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
 *         Nicolas Pouillon <nipo@ssji.net>
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
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <limits>
#include "arithmetics.h"
#include "../include/vci_xcache_wrapper.h"

namespace soclib {
namespace caba {

#ifdef SOCLIB_MODULE_DEBUG
namespace {
const char *dcache_fsm_state_str[] = {
        "DCACHE_IDLE",
        "DCACHE_WRITE_UPDT",
        "DCACHE_WRITE_REQ",
        "DCACHE_MISS_WAIT",
        "DCACHE_MISS_UPDT",
        "DCACHE_UNC_WAIT",
        "DCACHE_INVAL",
        "DCACHE_ERROR",
    };
const char *icache_fsm_state_str[] = {
        "ICACHE_IDLE",
        "ICACHE_MISS_WAIT",
        "ICACHE_MISS_UPDT",
        "ICACHE_UNC_WAIT",
        "ICACHE_ERROR",
    };
const char *cmd_fsm_state_str[] = {
        "CMD_IDLE",
        "CMD_INS_MISS",
        "CMD_INS_UNC",
        "CMD_DATA_MISS",
        "CMD_DATA_UNC",
        "CMD_DATA_WRITE",
    };
const char *rsp_fsm_state_str[] = {
        "RSP_IDLE",
        "RSP_INS_MISS",
        "RSP_INS_UNC",
        "RSP_DATA_MISS",
        "RSP_DATA_UNC",
        "RSP_DATA_WRITE",
    };
}
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciXcacheWrapper<vci_param, iss_t>
 
/////////////////////////////////////////////////////////////////////////////////////////////////
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

using soclib::common::uint32_log2;

/////////////////////////////////
tmpl(/**/)::VciXcacheWrapper(
/////////////////////////////////
    sc_module_name name,
    int proc_id,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &index,
    size_t icache_ways,
    size_t icache_sets,
    size_t icache_words,
    size_t dcache_ways,
    size_t dcache_sets,
    size_t dcache_words )
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
      r_dcache_write_req("r_dcache_write_req"),

      r_icache_fsm("r_icache_fsm"),
      r_icache_addr_save("r_icache_addr_save"),
      r_icache_miss_req("r_icache_miss_req"),
      r_icache_unc_req("r_icache_unc_req"),

      r_vci_cmd_fsm("r_vci_cmd_fsm"),
      r_vci_cmd_min("r_vci_cmd_min"),
      r_vci_cmd_max("r_vci_cmd_max"),
      r_vci_cmd_cpt("r_vci_cmd_cpt"),

      r_vci_rsp_fsm("r_vci_rsp_fsm"),
      r_vci_rsp_ins_error("r_vci_rsp_ins_error"),
      r_vci_rsp_data_error("r_vci_rsp_data_error"),
      r_vci_rsp_cpt("r_vci_rsp_cpt"),

      r_icache_buf_unc_valid("r_icache_buf_unc_valid"),
      r_dcache_buf_unc_valid("r_dcache_buf_unc_valid"),

      r_wbuf("wbuf", dcache_words ),
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

    m_iss.setICacheInfo( icache_words*sizeof(data_t), icache_ways, icache_sets );
    m_iss.setDCacheInfo( dcache_words*sizeof(data_t), dcache_ways, dcache_sets );
}

/////////////////////////////////
tmpl(/**/)::~VciXcacheWrapper()
/////////////////////////////////
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
    std::cout << name() << std::endl;
    std::cout << "- CPI                = " << (float)m_cpt_total_cycles/run_cycles << std::endl ;
    std::cout << "- READ RATE          = " << (float)m_cpt_read/run_cycles << std::endl ;
    std::cout << "- WRITE RATE         = " << (float)m_cpt_write/run_cycles << std::endl;
    std::cout << "- UNCACHED READ RATE = " << (float)m_cpt_unc_read/m_cpt_read << std::endl ;
    std::cout << "- CACHED WRITE RATE  = " << (float)m_cpt_write_cached/m_cpt_write << std::endl ;
    std::cout << "- IMISS_RATE         = " << (float)m_cpt_ins_miss/run_cycles << std::endl;
    std::cout << "- DMISS RATE         = " << (float)m_cpt_data_miss/(m_cpt_read-m_cpt_unc_read) << std::endl ;
    std::cout << "- INS MISS COST      = " << (float)m_cost_ins_miss_frz/m_cpt_ins_miss << std::endl;
    std::cout << "- IMISS TRANSACTION  = " << (float)m_cost_imiss_transaction/m_cpt_imiss_transaction << std::endl;
    std::cout << "- DMISS COST         = " << (float)m_cost_data_miss_frz/m_cpt_data_miss << std::endl;
    std::cout << "- DMISS TRANSACTION  = " << (float)m_cost_dmiss_transaction/m_cpt_dmiss_transaction << std::endl;
    std::cout << "- UNC COST           = " << (float)m_cost_unc_read_frz/m_cpt_unc_read << std::endl;
    std::cout << "- UNC TRANSACTION    = " << (float)m_cost_unc_transaction/m_cpt_unc_transaction << std::endl;
    std::cout << "- WRITE COST         = " << (float)m_cost_write_frz/m_cpt_write << std::endl;
    std::cout << "- WRITE TRANSACTION  = " << (float)m_cost_write_transaction/m_cpt_write_transaction << std::endl;
    std::cout << "- WRITE LENGTH       = " << (float)m_length_write_transaction/m_cpt_write_transaction << std::endl;
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
        r_vci_cmd_fsm = CMD_IDLE;
        r_vci_rsp_fsm = RSP_IDLE;

        // write buffer & caches
        r_wbuf.reset();
        r_icache.reset();
        r_dcache.reset();

        // synchronisation flip-flops from ICACHE & DCACHE FSMs to VCI  FSMs
        r_icache_miss_req    = false;
        r_dcache_miss_req    = false;
        r_dcache_unc_req     = false;
        r_dcache_write_req   = false;

        // signals from the VCI RSP FSM to the ICACHE or DCACHE FSMs
        r_dcache_buf_unc_valid = false;
        r_icache_buf_unc_valid = false;
        r_vci_rsp_data_error   = false;
        r_vci_rsp_ins_error    = false;

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

        m_cost_imiss_transaction = 0;
        m_cost_dmiss_transaction = 0;
        m_cost_unc_transaction = 0;
        m_cost_write_transaction = 0;
        m_length_write_transaction = 0;

        return;
    }

#ifdef SOCLIB_MODULE_DEBUG
    std::cout
        << name()
        << " dcache fsm: " << dcache_fsm_state_str[r_dcache_fsm]
        << " icache fsm: " << icache_fsm_state_str[r_icache_fsm]
        << " cmd fsm: " << cmd_fsm_state_str[r_vci_cmd_fsm]
        << " rsp fsm: " << rsp_fsm_state_str[r_vci_rsp_fsm] << std::endl;
#endif

    m_cpt_total_cycles++;

    /////////////////////////////////////////////////////////////////////
    // The ICACHE FSM controls the following ressources:
    // - r_icache_fsm
    // - r_icache (instruction cache access)
    // - r_icache_addr_save
    // - r_icache_buf_unc_valid 
    // - r_icache_miss_req set
    // - r_icache_unc_req set
    // - r_icache_unc_req set
    // - r_vci_rsp_ins_error reset
    // - ireq & irsp structures for communication with the processor
    //
    // Processor requests are taken into account only in the IDLE state.
    // In case of MISS, or in case of uncached instruction, the FSM 
    // writes the missing address line in the  r_icache_addr_save register 
    // and sets the r_icache_miss_req (or the r_icache_unc_req) flip-flop.
    // The request flip-flop is reset by the VCI_RSP FSM when the VCI 
    // transaction is completed. 
    // The r_icache_buf_unc_valid is set in case of uncached access.
    // In case of bus error, the VCI_RSP FSM sets the r_vci_rsp_ins_error
    // flip-flop. It is reset by the ICACHE FSM.
    ///////////////////////////////////////////////////////////////////////

    typename iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

    typename iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;

    m_iss.getRequests( ireq, dreq );

#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Instruction Request: " << ireq << std::endl;
#endif

    switch(r_icache_fsm) {

    case ICACHE_IDLE:
        if ( ireq.valid ) {
            data_t  icache_ins = 0;
            bool    icache_hit = false;
            bool    icache_cached = m_cacheability_table[ireq.addr];
            // icache_hit & icache_ins evaluation
            if ( icache_cached ) {
                icache_hit = r_icache.read(ireq.addr, &icache_ins);
            } else {
                icache_hit = ( r_icache_buf_unc_valid && (ireq.addr == r_icache_addr_save) );
                icache_ins = r_icache_miss_buf[0];
            }
            if ( ! icache_hit ) {
                m_cpt_ins_miss++;
                m_cost_ins_miss_frz++;
                r_icache_addr_save = ireq.addr;
                if ( icache_cached ) {
                    r_icache_fsm = ICACHE_MISS_WAIT;
                    r_icache_miss_req = true;
                } else {
                    r_icache_fsm = ICACHE_UNC_WAIT;
                    r_icache_unc_req = true;
                    r_icache_buf_unc_valid = false;
                } 
            } else {
                r_icache_buf_unc_valid = false;
            }
            m_cpt_icache_dir_read += m_icache_ways;
            m_cpt_icache_data_read += m_icache_ways;
            irsp.valid          = icache_hit;
            irsp.instruction    = icache_ins;
        }
        break;

    case ICACHE_MISS_WAIT:
        m_cost_ins_miss_frz++;
        if ( !r_icache_miss_req ) {
            if ( r_vci_rsp_ins_error ) {
                r_icache_fsm = ICACHE_ERROR;
                r_vci_rsp_ins_error = false;
            } else {
                r_icache_fsm = ICACHE_MISS_UPDT;
            }
        }
        break;

    case ICACHE_UNC_WAIT:
        m_cost_ins_miss_frz++;
        if ( !r_icache_unc_req ) {
            if ( r_vci_rsp_ins_error ) {
                r_icache_fsm = ICACHE_ERROR;
                r_vci_rsp_ins_error = false;
            } else {
                r_icache_fsm = ICACHE_IDLE;
                r_icache_buf_unc_valid = true;
            }
        }
        break;

    case ICACHE_ERROR:
        r_icache_fsm = ICACHE_IDLE;
        r_vci_rsp_ins_error = false;
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
    // - r_dcache_buf_unc_valid
    // - r_dcache_miss_req set
    // - r_dcache_unc_req set
    // - r_dcache_write_req set
    // - r_vci_rsp_data_error reset
    // - r_wbuf write
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
    //   - CACHED READ MISS => to the MISS_WAIT state (waiting the r_miss_ok signal),
    //     then to the MISS_UPDT state, and finally to the IDLE state.
    //   - UNCACHED READ  => to the UNC_WAIT state (waiting the r_miss_ok signal),
    //     and to the IDLE state.
    //   - CACHE INVALIDATE HIT => to the INVAL state for one cycle, then to IDLE state.
    //   - WRITE MISS => directly to the WRITE_REQ state to access the write buffer.
    //   - WRITE HIT => to the WRITE_UPDT state, then to the WRITE_REQ state.
    //
    // Error handling :  Read Bus Errors are synchronous events, but
    // Write Bus Errors are asynchronous events (processor is not frozen).
    // - If a Read Bus Error is detected, the VCI_RSP FSM sets the
    //   r_vci_rsp_data_error flip-flop, and the synchronous error is signaled
    //   by the DCACHE FSM.
    // - If a Write Bus Error is detected, the VCI_RSP FSM  signals
    //   the asynchronous error using the setWriteBerr() method.
    ////////////////////////////////////////////////////////////////////////

#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Data Request: " << dreq << std::endl;
#endif

    switch ( r_dcache_fsm ) {

    case DCACHE_WRITE_REQ:
        // try to post the write request in the write buffer
        if ( !r_dcache_write_req ) {
            // no previous write transaction
            if ( r_wbuf.wok(r_dcache_addr_save) ) {
                // write request in the same cache line
                r_wbuf.write(r_dcache_addr_save, r_dcache_be_save, r_dcache_wdata_save);

                // closing the write packet if uncached
                if ( !r_dcache_cached_save )
                    r_dcache_write_req = true ;
            } else {
                // close the write packet if write request not in the same cache line
                r_dcache_write_req = true;
                m_cost_write_frz++;
                break;
                //  posting not possible : stay in DCACHE_WRITEREQ state
            }
        } else {
            //  previous write transaction not completed
            m_cost_write_frz++;
            break;
            //  posting not possible : stay in DCACHE_WRITEREQ state
        }

        // close the write packet if the next processor request is not a write
        if ( !dreq.valid || dreq.type != iss_t::DATA_WRITE )
            r_dcache_write_req = true ;

        // The next state and the processor request parameters are computed
        // as in the DCACHE_IDLE state (see below ...)

    case DCACHE_IDLE:
        if ( dreq.valid ) {
            bool        dcache_hit     = false;
            data_t      dcache_rdata   = 0;
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
            if ( dcache_cached ) {
                dcache_hit = r_dcache.read(dreq.addr, &dcache_rdata);
            } else {
                dcache_hit = ( (dreq.addr == r_dcache_addr_save) && r_dcache_buf_unc_valid );
                dcache_rdata = r_dcache_miss_buf[0];
            }

            switch( dreq.type ) {
                case iss_t::DATA_READ:
                case iss_t::DATA_LL:
                case iss_t::DATA_SC:
                    m_cpt_read++;
                    if ( dcache_hit ) {
                        r_dcache_fsm = DCACHE_IDLE;
                        drsp.valid = true;
                        drsp.rdata = dcache_rdata;
                        r_dcache_buf_unc_valid = false;
                    } else {
                        if ( dcache_cached ) {
                            m_cpt_data_miss++;
                            m_cost_data_miss_frz++;
                            r_dcache_miss_req = true;
                            r_dcache_fsm = DCACHE_MISS_WAIT;
                        } else {
                            m_cpt_unc_read++;
                            m_cost_unc_read_frz++;
                            r_dcache_unc_req = true;
                            r_dcache_fsm = DCACHE_UNC_WAIT;
                        }
                    }
                    break;
                case iss_t::XTN_READ:
                case iss_t::XTN_WRITE:
                    // only DCACHE INVALIDATE request are supported
                    switch ( dreq.addr/4 ) {
                    case iss_t::XTN_DCACHE_INVAL:
                        r_dcache_fsm = DCACHE_INVAL;
                    case iss_t::XTN_SYNC:
                    default:
                        drsp.valid = true;
                        drsp.rdata = 0;
                        break;
                    }
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

        } else {    // if no dcache_req
            r_dcache_fsm = DCACHE_IDLE;
        }
        // processor request are not accepted in the WRITE_REQUEST state
        // when the write buffer is not writeable
        if ( (r_dcache_fsm == DCACHE_WRITE_REQ) &&
             (r_dcache_write_req || !r_wbuf.wok(r_dcache_addr_save)) ) {
            drsp.valid = false;
        }
        break;

    case DCACHE_WRITE_UPDT:
    {
        m_cpt_dcache_data_write++;
        data_t mask = be_to_mask(r_dcache_be_save);
        data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
        r_dcache.write(r_dcache_addr_save, wdata);
        r_dcache_fsm = DCACHE_WRITE_REQ;
        break;
    }

    case DCACHE_MISS_WAIT:
        if ( dreq.valid ) m_cost_data_miss_frz++;
        if ( !r_dcache_miss_req ) {
            if ( r_vci_rsp_data_error )
                r_dcache_fsm = DCACHE_ERROR;
            else
                r_dcache_fsm = DCACHE_MISS_UPDT;
        }
        break;

    case DCACHE_MISS_UPDT:
    {
        addr_t  ad  = r_dcache_addr_save;
        data_t* buf = r_dcache_miss_buf;
        data_t  victim_index = 0;
        if ( dreq.valid )
            m_cost_data_miss_frz++;
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        r_dcache.update(ad, buf, &victim_index);
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }

    case DCACHE_UNC_WAIT:
        if ( dreq.valid ) m_cost_unc_read_frz++;
        if ( !r_dcache_unc_req ) {
            if ( r_vci_rsp_data_error ) {
                r_dcache_fsm = DCACHE_ERROR;
            } else {
                r_dcache_fsm = DCACHE_IDLE;
                // If request was a DATA_SC we need to invalidate the corresponding cache line, 
                // so that subsequent access to this line are read from RAM
                if (dreq.type == iss_t::DATA_SC) {
                    r_dcache_fsm = DCACHE_INVAL;
                    r_dcache_wdata_save = r_dcache_addr_save;
                }
                r_dcache_buf_unc_valid = true;
            }
        }
        break;

    case DCACHE_ERROR:
        r_dcache_fsm = DCACHE_IDLE;
        r_vci_rsp_data_error = false;
        drsp.error = true;
        drsp.valid = true;
        break;

    case DCACHE_INVAL:
        m_cpt_dcache_dir_read += m_dcache_ways;
        r_dcache.inval(r_dcache_wdata_save);
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }


#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Data Response: " << drsp << std::endl;
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
    // The VCI_CMD FSM controls the following ressources:
    // - r_vci_cmd_fsm
    // - r_vci_cmd_min
    // - r_vci_cmd_max
    // - r_vci_cmd_cpt
    // - wbuf reset
    //
    // This FSM handles requests from both the DCACHE FSM & the ICACHE FSM.
    // There is 4 request types, with the following priorities :
    // 1 - Instruction Miss     : r_icache_miss_req
    // 2 - Instruction Uncached : r_icache_unc_req
    // 3 - Data Write           : r_dcache_write_req
    // 4 - Data Read Miss       : r_dcache_miss_req
    // 5 - Data Read Uncached   : r_dcache_unc_req
    // There is at most one (CMD/RSP) VCI transaction, as both CMD_FSM
    // and RSP_FSM exit simultaneously the IDLE state.
    //
    // VCI formats:
    // According to the VCI advanced specification, all read command packets
    // (Uncached, Miss data, Miss instruction) are one word packets.
    // For write burst packets, all words must be in the same cache line,
    // and addresses must be contiguous (the BE field is 0 in case of "holes").
    // The PLEN VCI field is always documented.
    //////////////////////////////////////////////////////////////////////////////

    switch (r_vci_cmd_fsm) {

    case CMD_IDLE:
        if (r_vci_rsp_fsm != RSP_IDLE)
            break;

        r_vci_cmd_cpt = 0;
        if ( r_icache_miss_req ) {
            r_vci_cmd_fsm = CMD_INS_MISS;
            m_cpt_imiss_transaction++;
        } else if ( r_icache_unc_req ) {
            r_vci_cmd_fsm = CMD_INS_UNC;
            m_cpt_imiss_transaction++;
        } else if ( r_dcache_write_req ) {
            r_vci_cmd_fsm = CMD_DATA_WRITE;
            r_vci_cmd_cpt = r_wbuf.getMin();
            r_vci_cmd_min = r_wbuf.getMin();
            r_vci_cmd_max = r_wbuf.getMax();
            m_cpt_write_transaction++;
            m_length_write_transaction += (r_wbuf.getMax() - r_wbuf.getMin() + 1);
        } else if ( r_dcache_miss_req ) {
            r_vci_cmd_fsm = CMD_DATA_MISS;
            m_cpt_dmiss_transaction++;
        } else if ( r_dcache_unc_req ) {
            r_vci_cmd_fsm = CMD_DATA_UNC;
            m_cpt_unc_transaction++;
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci.cmdack.read() ) {
            r_vci_cmd_cpt = r_vci_cmd_cpt + 1;
            if (r_vci_cmd_cpt == r_vci_cmd_max) {
                r_vci_cmd_fsm = CMD_IDLE ;
                r_wbuf.reset() ;
            }
        }
        break;

    case CMD_DATA_MISS:
    case CMD_DATA_UNC:
    case CMD_INS_MISS:
    case CMD_INS_UNC:
        if ( p_vci.cmdack.read() ) {
            r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    } // end  switch r_vci_cmd_fsm

    //////////////////////////////////////////////////////////////////////////
    // The VCI_RSP FSM controls the following ressources:
    // - r_vci_rsp_fsm:
    // - r_icache_miss_buf[m_icache_words]
    // - r_dcache_miss_buf[m_dcache_words]
    // - r_icache_miss_req reset
    // - r_icache_unc_req reset
    // - r_dcache_miss_req reset
    // - r_dcache_unc_req reset
    // - r_icache_write_req reset
    // - r_vci_rsp_data_error set
    // - r_vci_rsp_ins_error set
    // - r_vci_rsp_cpt
    // In order to have only one active VCI transaction, this VCI_RSP_FSM
    // is synchronized with the VCI_CMD FSM, and both FSMs exit the
    // IDLE state simultaneously.
    //
    // VCI formats:
    // This component accepts single word or multi-word response packets for
    // write response packets.
    //
    // Error handling:
    // This FSM analyzes the VCI error code and signals directly the
    // Write Bus Error.
    // In case of Read Data Error, the VCI_RSP FSM sets the r_vci_rsp_data_error
    // flip_flop and the error is signaled by the DCACHE FSM.
    // In case of Instruction Error, the VCI_RSP FSM sets the r_vci_rsp_ins_error
    // flip_flop and the error is signaled by the DCACHE FSM.
    // In case of Cleanup Error, the simulation stops with an error message...
    //////////////////////////////////////////////////////////////////////////

    switch (r_vci_rsp_fsm) {

    case RSP_IDLE:
        assert( ! p_vci.rspval.read() && "Unexpected response" );
        if (r_vci_cmd_fsm != CMD_IDLE) break;

        r_vci_rsp_cpt = 0;
        if      ( r_icache_miss_req )       r_vci_rsp_fsm = RSP_INS_MISS;
        else if ( r_icache_unc_req )        r_vci_rsp_fsm = RSP_INS_UNC;
        else if ( r_dcache_write_req )      r_vci_rsp_fsm = RSP_DATA_WRITE;
        else if ( r_dcache_miss_req )       r_vci_rsp_fsm = RSP_DATA_MISS;
        else if ( r_dcache_unc_req )        r_vci_rsp_fsm = RSP_DATA_UNC;
        break;

    case RSP_INS_MISS:
        m_cost_imiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;
        assert( (r_vci_rsp_cpt < m_icache_words) &&
               "The VCI response packet for instruction miss is too long" );
        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_icache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            assert( (r_vci_rsp_cpt == m_icache_words - 1) &&
                   "The VCI response packet for instruction miss is too short");
            r_icache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        }
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
            r_vci_rsp_ins_error = true;
        break;

    case RSP_INS_UNC:
        m_cost_imiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;
        assert(p_vci.reop.read() &&
               "illegal VCI response packet for uncached instruction");
        r_icache_miss_buf[0] = (data_t)p_vci.rdata.read();
        r_icache_buf_unc_valid = true;
        r_vci_rsp_fsm = RSP_IDLE;
        r_icache_unc_req = false;
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
            r_vci_rsp_ins_error = true;
        break;

    case RSP_DATA_MISS:
        m_cost_dmiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;
        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");
        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                   "illegal VCI response packet for instruction miss");
            r_dcache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        }
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
            r_vci_rsp_data_error = true;
        break;

    case RSP_DATA_WRITE:
        m_cost_write_transaction++;
        if ( ! p_vci.rspval.read() )
            break;
        if ( p_vci.reop.read() ) {
            r_vci_rsp_fsm = RSP_IDLE;
            r_dcache_write_req = false;
        }
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
            m_iss.setWriteBerr();
        break;

    case RSP_DATA_UNC:
        m_cost_unc_transaction++;
        if ( ! p_vci.rspval.read() )
            break;
        assert(p_vci.reop.read() &&
               "illegal VCI response packet for uncached read data");
        r_dcache_miss_buf[0] = (data_t)p_vci.rdata.read();
        r_vci_rsp_fsm = RSP_IDLE;
        r_dcache_unc_req = false;
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
            r_vci_rsp_data_error = true;
        break;

    } // end switch r_vci_rsp_fsm

} // end transition()

//////////////////////////////////////////////////////////////////////////////////
tmpl(void)::genMoore()
//////////////////////////////////////////////////////////////////////////////////
{
    // VCI initiator response

    p_vci.rspack = true;

    // VCI initiator command

    switch (r_vci_cmd_fsm) {

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

    case CMD_DATA_UNC:
        p_vci.cmdval = true;
        p_vci.address = r_dcache_addr_save & ~0x3;
        switch( r_dcache_type_save ) {
        case iss_t::DATA_READ:
            p_vci.wdata = 0;
            p_vci.be  = r_dcache_be_save.read();
            p_vci.cmd = vci_param::CMD_READ;
#ifdef I_WANT_ILLEGAL_VCI
            p_vci.plen = 4;
#else
            p_vci.plen = soclib::common::fls(r_dcache_be_save.read())-ffs(r_dcache_be_save.read())+1;
#endif
            break;
        case iss_t::DATA_LL:
            p_vci.wdata = 0;
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_LOCKED_READ;
            p_vci.plen = 4;
            break;
        case iss_t::DATA_SC:
            p_vci.wdata = r_dcache_wdata_save.read();
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_STORE_COND;
            p_vci.plen = 4;
            break;
        default:
            assert("this should not happen");
        }
        p_vci.trdid  = 0;
        p_vci.pktid  = 0;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop    = true;
        break;

    case CMD_DATA_WRITE:
        p_vci.cmdval  = true;
        p_vci.address = r_wbuf.getAddress(r_vci_cmd_cpt);
        p_vci.wdata   = r_wbuf.getData(r_vci_cmd_cpt);
        p_vci.be      = r_wbuf.getBe(r_vci_cmd_cpt);
#ifdef I_WANT_ILLEGAL_VCI
        p_vci.plen    = (r_vci_cmd_max - r_vci_cmd_min + 1)<<2;
#else
        p_vci.plen    = soclib::common::fls(r_wbuf.getBe(r_vci_cmd_max))
            - ffs(r_wbuf.getBe(r_vci_cmd_min))
            + (r_vci_cmd_max - r_vci_cmd_min) * vci_param::B
            + 1;
#endif
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.trdid   = 0;
        p_vci.pktid   = 0;
        p_vci.srcid   = m_srcid;
        p_vci.cons    = false;
        p_vci.wrap    = false;
        p_vci.contig  = true;
        p_vci.clen    = 0;
        p_vci.cfixed  = false;
        p_vci.eop     = (r_vci_cmd_cpt == r_vci_cmd_max);
        break;

    case CMD_DATA_MISS:
        p_vci.cmdval = true;
        p_vci.address = r_dcache_addr_save & m_dcache_yzmask;
        p_vci.be     = 0xF;
        p_vci.plen   = m_dcache_words << 2;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = 0;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = true;
        break;

    case CMD_INS_MISS:
        p_vci.cmdval = true;
        p_vci.address = r_icache_addr_save & m_icache_yzmask;
        p_vci.be     = 0xF;
        p_vci.plen   = m_icache_words << 2;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = 0;
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
        p_vci.pktid  = 0;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = true;
        break;

    } // end switch r_vci_cmd_fsm

} // end genMoore()

}} // end namespace

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4




