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
 *         Alain Greiner <alain.greiner@lip6.fr>, 2008
 *
 * Maintainers: alain eric.guthmuller@polytechnique.edu nipo
 */

/////////////////////////////////////////////////////////////////////////////
// History
// - 25/04/2008
//   The existing vci_xcache component has been extended to include 
//   a VCI target port to support a directory based coherence protocol.
//   Two types of packets can be send by the L2 cache to the L1 cache
//   * INVALIDATE packets : length = 1
//   * UPDATE packets : length = n + 2   
//   The CLEANUP packets are sent by the L1 cache to the L2 cache, 
//   to signal a replaced cache line.
// - 12/08/2008
//   The vci_cc_xcache_wrapper component instanciates directly the processsor 
//   iss, in order to supress the processor/cache interface.
//   According to the VCI advanced specification, this component uses one 
//   word VCI CMD packets for MISS transactions, and accept one word VCI RSP
//   packets for Write burst  transactions.
//   The write buffer has been modified to use the WriteBuffer object. 
//   A VCI write burst is constructed when two conditions are satisfied :
//   The processor make strictly successive write requests, and they are
//   in the same cache line. The write buffer performs re-ordering, to
//   respect the contiguous addresses VCI constraint. In case of several
//   WRITE_WORD requests in the same word, only the last request is conserved.
//   In case of several WRITE_HALF or WRITE_WORD requests in the same word,
//   the requests are merged in the same word. In case of uncached write
//   requests, each request is transmited as a single VCI transaction.
//   Both the data & instruction caches can be flushed in one single cycle.
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include "arithmetics.h"
#include "../include/vci_cc_xcache_wrapper.h"

#define DEBUG_CC_XCACHE_WRAPPER 0

namespace soclib { 
namespace caba {

#if DEBUG_CC_XCACHE_WRAPPER
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
    "DCACHE_CC_CHECK",
    "DCACHE_CC_INVAL",
    "DCACHE_CC_UPDT",
    "DCACHE_CC_NOP",
};
const char *icache_fsm_state_str[] = {
    "ICACHE_IDLE",
    "ICACHE_MISS_WAIT",
    "ICACHE_MISS_UPDT",
    "ICACHE_UNC_WAIT",
    "ICACHE_ERROR",
    "ICACHE_CC_INVAL",
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
const char *tgt_fsm_state_str[] = {
    "TGT_IDLE",
    "TGT_UPDT_WORD",
    "TGT_UPDT_DATA",
    "TGT_REQ_BROADCAST",
    "TGT_REQ_DCACHE",
    "TGT_RSP_BROADCAST",
    "TGT_RSP_DCACHE",
};
}
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciCcXcacheWrapper<vci_param, iss_t>

using soclib::common::uint32_log2;

/////////////////////////////////
tmpl(/**/)::VciCcXcacheWrapper(
                               /////////////////////////////////
                               sc_module_name name,
                               int proc_id,
                               const soclib::common::MappingTable &mtp,
                               const soclib::common::MappingTable &mtc,
                               const soclib::common::IntTab &initiator_index,
                               const soclib::common::IntTab &target_index,
                               size_t icache_ways,
                               size_t icache_sets,
                               size_t icache_words,
                               size_t dcache_ways,
                               size_t dcache_sets,
                               size_t dcache_words,
                               addr_t cleanup_offset )
           : 
           soclib::caba::BaseModule(name),

           p_clk("clk"),
           p_resetn("resetn"),
           p_vci_ini("vci_ini"),
           p_vci_tgt("vci_tgt"),

           m_cacheability_table(mtp.getCacheabilityTable()),
           m_segment(mtc.getSegment(target_index)),
           m_iss(this->name(), proc_id),
           m_srcid(mtp.indexForId(initiator_index)),
           m_cleanup_address(cleanup_offset),

           m_dcache_ways(dcache_ways),
           m_dcache_words(dcache_words),
           m_dcache_yzmask((~0)<<(uint32_log2(dcache_words) + 2)),
           m_icache_ways(icache_ways),
           m_icache_words(icache_words),
           m_icache_yzmask((~0)<<(uint32_log2(icache_words) + 2)),

           r_dcache_fsm("r_dcache_fsm"),
           r_dcache_fsm_save("r_dcache_fsm_save"),
           r_dcache_addr_save("r_dcache_addr_save"),
           r_dcache_wdata_save("r_dcache_wdata_save"),
           r_dcache_rdata_save("r_dcache_rdata_save"),
           r_dcache_type_save("r_dcache_type_save"),
           r_dcache_be_save("r_dcache_be_save"),
           r_dcache_cached_save("r_dcache_cached_save"),
           r_dcache_cleanup_req("r_dcache_cleanup_req"),
           r_dcache_cleanup_line("r_dcache_cleanup_line"),
           r_dcache_miss_req("r_dcache_miss_req"),
           r_dcache_unc_req("r_dcache_unc_req"),
           r_dcache_write_req("r_dcache_write_req"),

           r_icache_fsm("r_icache_fsm"),
           r_icache_fsm_save("r_icache_fsm_save"),
           r_icache_addr_save("r_icache_addr_save"),
           r_icache_miss_req("r_icache_miss_req"),
           r_icache_cleanup_req("r_icache_cleanup_req"),
           r_icache_cleanup_line("r_icache_cleanup_line"),

           r_vci_cmd_fsm("r_vci_cmd_fsm"),
           r_vci_cmd_min("r_vci_cmd_min"),
           r_vci_cmd_max("r_vci_cmd_max"),
           r_vci_cmd_cpt("r_vci_cmd_cpt"),

           r_vci_rsp_fsm("r_vci_rsp_fsm"),
           r_vci_rsp_ins_error("r_vci_rsp_ins_error"),
           r_vci_rsp_data_error("r_vci_rsp_data_error"),
           r_vci_rsp_cpt("r_vci_rsp_cpt"),

           r_dcache_buf_unc_valid("r_dcache_buf_unc_valid"),
           r_icache_buf_unc_valid("r_icache_buf_unc_valid"),

           r_vci_tgt_fsm("r_vci_tgt_fsm"),
           r_tgt_addr("r_tgt_addr"),
           r_tgt_word("r_tgt_word"),
           r_tgt_update("r_tgt_update"),
           r_tgt_srcid("r_tgt_srcid"),
           r_tgt_pktid("r_tgt_pktid"),
           r_tgt_trdid("r_tgt_trdid"),
           r_tgt_icache_req("r_tgt_icache_req"),
           r_tgt_dcache_req("r_tgt_dcache_req"),

           r_wbuf("r_wbuf", dcache_words ),
           r_icache("icache", icache_ways, icache_sets, icache_words),
           r_dcache("dcache", dcache_ways, dcache_sets, dcache_words)

{
    r_icache_miss_buf = new data_t[icache_words];
    r_dcache_miss_buf = new data_t[dcache_words];
    r_tgt_buf         = new data_t[dcache_words];
    r_tgt_val         = new bool[dcache_words];

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
} // end constructor

///////////////////////////////////
tmpl(/**/)::~VciCcXcacheWrapper()
           ///////////////////////////////////
{
    delete [] r_icache_miss_buf;
    delete [] r_dcache_miss_buf;
    delete [] r_tgt_val;
    delete [] r_tgt_buf;
}

////////////////////////
tmpl(void)::print_cpi()
           ////////////////////////
{
    std::cout << "CPU " << m_srcid << " : CPI = " 
              << (float)m_cpt_total_cycles/(m_cpt_total_cycles - m_cpt_frz_cycles) << std::endl ;
}
////////////////////////
tmpl(void)::print_stats()
           ////////////////////////
{
    float run_cycles = (float)(m_cpt_total_cycles - m_cpt_frz_cycles);
    std::cout << "------------------------------------" << std:: dec << std::endl;
    std::cout << "CPU " << m_srcid << " / Time = " << m_cpt_total_cycles << std::endl;
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
        r_vci_tgt_fsm = TGT_IDLE;

        // write buffer & caches
        r_wbuf.reset();
        r_icache.reset();
        r_dcache.reset();

        // synchronisation flip-flops from ICACHE & DCACHE FSMs to VCI  FSMs
        r_icache_miss_req    = false;
        r_icache_unc_req     = false;
        r_icache_cleanup_req = false;
        r_dcache_miss_req    = false;
        r_dcache_unc_req     = false;
        r_dcache_write_req   = false;
        r_dcache_cleanup_req = false;

        // synchronisation flip-flops from TGT FSM to ICACHE & DCACHE FSMs
        r_tgt_icache_req     = false;
        r_tgt_dcache_req     = false;

        // internal messages in DCACHE et ICACHE FSMs
        r_icache_inval_rsp   = false;
        r_dcache_inval_rsp   = false;

        // error signals from the VCI RSP FSM to the ICACHE or DCACHE FSMs
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

        m_cpt_cc_update = 0;
        m_cpt_cc_inval = 0;

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

#if DEBUG_CC_XCACHE_WRAPPER
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << std::dec << "CC_XCACHE_WRAPPER " << m_srcid << " / Time = " << m_cpt_total_cycles << std::endl;
    std::cout             << " tgt fsm    = " << tgt_fsm_state_str[r_vci_tgt_fsm] << std::endl
                          << " dcache fsm = " << dcache_fsm_state_str[r_dcache_fsm] << std::endl
                          << " icache fsm = " << icache_fsm_state_str[r_icache_fsm] << std::endl
                          << " cmd fsm    = " << cmd_fsm_state_str[r_vci_cmd_fsm] << std::endl
                          << " rsp fsm    = " << rsp_fsm_state_str[r_vci_rsp_fsm] << std::endl;
#endif

    m_cpt_total_cycles++;

    /////////////////////////////////////////////////////////////////////
    // The TGT_FSM controls the following ressources:
    // - r_vci_tgt_fsm
    // - r_tgt_buf[nwords]
    // - r_tgt_val[nwords]
    // - r_tgt_update
    // - r_tgt_word
    // - r_tgt_addr
    // - r_tgt_srcid
    // - r_tgt_trdid
    // - r_tgt_pktid
    // All VCI commands must be CMD_WRITE.
    // If the VCI address offset is null, the command is an invalidate 
    // request. It is an update request otherwise.
    // The VCI_TGT FSM stores the external request arguments in the
    // IDLE, UPDT_WORD & UPDT_DATA states. It sets the r_tgt_icache_req 
    // & r_tgt_dcache_req flip-flops to signal the external request to 
    // the ICACHE & DCACHE FSMs in the REQ state. It waits the completion
    // of the update or invalidate request in the RSP state.
    // -  for an invalidate request the VCI packet length is 1 word.
    // The WDATA field contains the line index (i.e. the Z & Y fields).
    // -  for an update request the VCI packet length is (n+2) words.
    // The WDATA field of the first VCI word contains the line number.
    // The WDATA field of the second VCI word contains the word index.
    // The WDATA field of the n following words contains the values.
    // -  for both invalidate & update requests, the VCI response
    // is one single word.
    // In case of errors in the VCI command packet, the simulation
    // is stopped with an error message.
    /////////////////////////////////////////////////////////////////////
    
    switch(r_vci_tgt_fsm) {

    case TGT_IDLE:
        if ( p_vci_tgt.cmdval.read() ) 
        {
            addr_t address = p_vci_tgt.address.read();

            if ( p_vci_tgt.cmd.read() != vci_param::CMD_WRITE) 
            {
                std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the received VCI command is not a write" << std::endl;
                exit(0);
            }

            // multi-update or multi-invalidate for data type
            if ( (address != 0x3) && (! m_segment.contains(address)) ) 
            {
                std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                std::cout << "out of segment VCI command received for a multi-updt or multi-inval request" << std::endl;
                exit(0);
            }

            r_tgt_srcid = p_vci_tgt.srcid.read();
            r_tgt_trdid = p_vci_tgt.trdid.read();
            r_tgt_pktid = p_vci_tgt.pktid.read();
            r_tgt_plen  = p_vci_tgt.plen.read();    // todo: wait L2 modification
            r_tgt_addr = (addr_t)p_vci_tgt.wdata.read() * m_dcache_words * 4;
            
            if ( address == 0x3 )   // broadcast invalidate for data or instruction type
            {
                if ( ! p_vci_tgt.eop.read() ) 
                {
                    std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                    std::cout << "the BROADCAST INVALIDATE command length must be one word" << std::endl;
                    exit(0);
                }
                r_tgt_update = false; 
                r_vci_tgt_fsm = TGT_REQ_BROADCAST;
                m_cpt_cc_inval++ ;
            }
            else                    // multi-update or multi-invalidate for data type
            { 
                addr_t cell = address - m_segment.baseAddress();
                if (cell == 0) 
                {                               // invalidate 
                    if ( ! p_vci_tgt.eop.read() ) 
                    {
                        std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "the MULTI-INVALIDATE command length must be one word" << std::endl;
                        exit(0);
                    }
                    r_tgt_update = false; 
                    r_vci_tgt_fsm = TGT_REQ_DCACHE;
                    m_cpt_cc_inval++ ;
                } 
                else 
                {                                // update
                    if ( p_vci_tgt.eop.read() ) 
                    {
                        std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "the MULTI-UPDATE command length must be N+2 words" << std::endl;
                        exit(0);
                    }
                    r_tgt_update = true; 
                    r_vci_tgt_fsm = TGT_UPDT_WORD;
                    m_cpt_cc_update++ ;
                }
            } // end if address
        } // end if cmdval
        break;

    case TGT_UPDT_WORD:
        if (p_vci_tgt.cmdval.read()) 
        {
            if ( p_vci_tgt.eop.read() ) 
            {
                std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the MULTI-UPDATE command length must be N+2 words" << std::endl;
                exit(0);
            }
            for ( size_t i=0 ; i<m_dcache_words ; i++ ) r_tgt_val[i] = false;
            r_tgt_word = p_vci_tgt.wdata.read(); // the first modified word index
            r_vci_tgt_fsm = TGT_UPDT_DATA;
        }
        break;

    case TGT_UPDT_DATA:
        if (p_vci_tgt.cmdval.read()) 
        {
            size_t word = r_tgt_word.read();
            if ( word >= m_dcache_words ) 
            {
                std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the reveived MULTI-UPDATE command length is wrong" << std::endl;
                exit(0);
            }
            r_tgt_buf[word] = p_vci_tgt.wdata.read();
            if(p_vci_tgt.be.read())    r_tgt_val[word] = true;
            r_tgt_word = word + 1;
            if (p_vci_tgt.eop.read())  r_vci_tgt_fsm = TGT_REQ_DCACHE;
        }
        break;

    case TGT_REQ_BROADCAST:
        if ( !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_RSP_BROADCAST; 
            r_tgt_icache_req = true;
            r_tgt_dcache_req = true;
        }
        break;

    case TGT_REQ_DCACHE:
        if ( !r_tgt_dcache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_RSP_DCACHE; 
            r_tgt_dcache_req = true;
        }
        break;

    case TGT_RSP_BROADCAST:
        if ( p_vci_tgt.rspack.read() && !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() ) 
        {
            // one response
            if ( !r_tgt_icache_rsp || !r_tgt_dcache_rsp )
            {
                r_vci_tgt_fsm = TGT_IDLE; 
                r_tgt_icache_rsp = false;
                r_tgt_dcache_rsp = false;
            }

            // if data and instruction have the inval line, need two responses  
            if ( r_tgt_icache_rsp && r_tgt_dcache_rsp )
            {
                r_tgt_icache_rsp = false; // only reset one for respond the second time 
            }
        }
        break;

    case TGT_RSP_DCACHE:
        if ( p_vci_tgt.rspack.read() && !r_tgt_dcache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_IDLE;
            r_tgt_dcache_rsp = false; 
        }
        break;

    } // end switch TGT_FSM

    /////////////////////////////////////////////////////////////////////
    // The ICACHE FSM controls the following ressources:
    // - r_icache_fsm
    // - r_icache_fsm_save
    // - r_icache instruction cache access
    // - r_icache_addr_save
    // - r_icache_miss_req set
    // - r_icache_unc_req set
    // - r_icache_buf_unc_valid set
    // - r_vci_rsp_ins_error reset
    // - r_tgt_icache_req reset
    // - ireq & irsp structures for communication with the processor
    //
    // 1/ External requests (update or invalidate) have highest priority.
    //    They are taken into account in the IDLE and WAIT states.
    //    As external hit should be extremly rare on the ICACHE,
    //    all external requests are handled as invalidate...
    //    In case of external request the ICACHE FSM goes to the CC_CHECK
    //    state to test the external hit, and returns in the
    //    pre-empted state after completion.
    // 2/ Processor requests are taken into account only in the IDLE state.
    //    In case of MISS, or in case of uncached instruction, the FSM 
    //    writes the missing address line in the  r_icache_addr_save register 
    //    and sets the r_icache_miss_req or the r_icache_unc_req flip-flops.
    //    These request flip-flops are reset by the VCI_RSP FSM
    //    when the VCI transaction is completed and the r_icache_buf_unc_valid
    //    is set in case of uncached access.
    //    In case of bus error, the VCI_RSP FSM sets the r_vci_rsp_ins_error 
    //    flip-flop. It is reset by the ICACHE FSM.
    ///////////////////////////////////////////////////////////////////////

    typename iss_t::InstructionRequest  ireq = ISS_IREQ_INITIALIZER;
    typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

    typename iss_t::DataRequest  dreq = ISS_DREQ_INITIALIZER;
    typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;

    m_iss.getRequests( ireq, dreq );

#if DEBUG_CC_XCACHE_WRAPPER
    std::cout << " Instruction Request: " << ireq << std::endl;
#endif

    switch(r_icache_fsm) {
        /////////////////
    case ICACHE_IDLE:
        {
            if ( r_tgt_icache_req ) {   // external request
                if ( ireq.valid ) m_cost_ins_miss_frz++;
                r_icache_fsm = ICACHE_CC_INVAL;
                r_icache_fsm_save = r_icache_fsm;
                break;
            } 
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
        }
        //////////////////////
    case ICACHE_MISS_WAIT:
        {
            m_cost_ins_miss_frz++;
            if ( r_tgt_icache_req ) {   // external request
                r_icache_fsm = ICACHE_CC_INVAL;
                r_icache_fsm_save = r_icache_fsm;
                break;
            } 
            if ( !r_icache_miss_req && !r_icache_inval_rsp ) { // Miss read response and no invalidation
                if ( r_vci_rsp_ins_error ) {
                    r_icache_fsm = ICACHE_ERROR;
                } else {
                    r_icache_fsm = ICACHE_MISS_UPDT;
                }
            }
            if ( !r_icache_miss_req && r_icache_inval_rsp ) { // Miss read response and invalidation
                if ( r_vci_rsp_ins_error ) {
                    r_icache_inval_rsp = false;
                    r_icache_fsm = ICACHE_ERROR;
                } else {
                    r_icache_inval_rsp = false;
                    r_icache_fsm = ICACHE_IDLE;
                }
            }
            break;
        }
        /////////////////////
    case ICACHE_UNC_WAIT:
        {
            m_cost_ins_miss_frz++;
            if ( r_tgt_icache_req ) {   // external request
                r_icache_fsm = ICACHE_CC_INVAL;
                r_icache_fsm_save = r_icache_fsm;
                break;
            } 
            if ( !r_icache_unc_req ) {
                if ( r_vci_rsp_ins_error ) {
                    r_icache_fsm = ICACHE_ERROR;
                } else {
                    r_icache_fsm = ICACHE_IDLE;
                    r_icache_buf_unc_valid = true;
                }
            }
            break;
        }
        //////////////////
    case ICACHE_ERROR:
        {
            r_icache_fsm = ICACHE_IDLE;
            r_vci_rsp_ins_error = false;
            irsp.error          = true;
            irsp.valid          = true;
            break;
        }
        //////////////////////
    case ICACHE_MISS_UPDT: 
        {
            if ( r_tgt_icache_req ) {   // external request
                r_icache_fsm = ICACHE_CC_INVAL;
                r_icache_fsm_save = r_icache_fsm;
                break;
            } 
            if(!r_icache_cleanup_req.read() && !r_icache_inval_rsp){
                addr_t    ad  = r_icache_addr_save;
                data_t*   buf = r_icache_miss_buf;
                data_t    victim_index = 0;
                m_cpt_icache_dir_write++;
                m_cpt_icache_data_write++;
                if ( ireq.valid ) m_cost_ins_miss_frz++;

                r_icache_cleanup_req = r_icache.update(ad, buf, &victim_index);
                r_icache_cleanup_line = victim_index;

                r_icache_fsm        = ICACHE_IDLE;
                break;
            }
            if(r_icache_inval_rsp){
                if ( ireq.valid ) m_cost_ins_miss_frz++;
                r_icache_inval_rsp  = false;
                r_icache_fsm        = ICACHE_IDLE;
                break;
            }
            if ( ireq.valid ) m_cost_ins_miss_frz++;
        }
        /////////////////////
    case ICACHE_CC_INVAL:  
        {                       
            addr_t    ad  = r_tgt_addr;
            if ( ireq.valid ) m_cost_ins_miss_frz++;
            m_cpt_icache_dir_read += m_icache_ways;
            if( (( r_icache_fsm_save == ICACHE_MISS_WAIT ) || ( r_icache_fsm_save == ICACHE_MISS_UPDT )) && 
                ((r_icache_addr_save.read() & ~((m_icache_words<<2)-1)) == (ad & ~((m_icache_words<<2)-1))) ) {
                r_icache_inval_rsp = true;
            } else {
                r_tgt_icache_rsp = r_icache.inval(ad);
            }
            r_tgt_icache_req = false;
            r_icache_fsm = r_icache_fsm_save;
            break;
        }

    } // end switch r_icache_fsm

#if DEBUG_CC_XCACHE_WRAPPER
    std::cout << " Instruction Response: " << irsp << std::endl;
#endif

    //////////////////////////////////////////////////////////////////////://///////////
    // The DCACHE FSM controls the following ressources:
    // - r_dcache_fsm
    // - r_dcache_fsm_save
    // - r_dcache (data cache access)
    // - r_dcache_addr_save
    // - r_dcache_wdata_save
    // - r_dcache_rdata_save
    // - r_dcache_type_save
    // - r_dcache_be_save
    // - r_dcache_cached_save
    // - r_dcache_miss_req set
    // - r_dcache_unc_req set
    // - r_dcache_write_req set
    // - r_dcache_cleanup_req set
    // - r_vci_rsp_data_error reset
    // - r_tgt_dcache_req reset
    // - r_wbuf write
    // - dreq & drsp structures for communication with the processor
    //
    // 1/ EXTERNAL REQUEST : 
    //    There is an external request when the tgt_dcache req flip-flop is set,
    //    requesting a line invalidation or a line update. 
    //    External requests are taken into account in the states  IDLE, WRITE_REQ,  
    //    UNC_WAIT, MISS_WAIT, and have the highest priority :
    //    The actions associated to the pre-empted state are not executed, the DCACHE FSM
    //    goes to the CC_CHECK state to execute the requested action, and returns to the
    //    pre-empted state.
    //  2/ PROCESSOR REQUEST : 
    //   In order to support VCI write burst, the processor requests are taken into account
    //   in the WRITE_REQ state as well as in the IDLE state.
    //   - In the IDLE state, the processor request cannot be satisfied if
    //   there is a cached read miss, or an uncached read.
    //   - In the WRITE_REQ state, the request cannot be satisfied if
    //   there is a cached read miss, or an uncached read,
    //   or when the write buffer is full.
    //   - In all other states, the processor request is not satisfied.
    //
    //   The cache access takes into account the cacheability_table.
    //   In case of processor request, there is five conditions to exit the IDLE state:
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
    ///////////////////////////////////////////////////////////////////////////////////

#if DEBUG_CC_XCACHE_WRAPPER
    std::cout << " Data Request: " << dreq << std::endl;
#endif

    //if( (m_cpt_total_cycles % 10000) ==0 ) std::cout << std::dec << "Proc " << m_srcid << " Data Request: " << dreq << std::endl;

    switch ( r_dcache_fsm ) {

        /////////////////////
    case DCACHE_WRITE_REQ:
        { 
            if ( r_tgt_dcache_req ) {   // external request
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            }

            // try to post the write request in the write buffer
            if ( !r_dcache_write_req ) {    // no previous write transaction     
                if ( r_wbuf.wok(r_dcache_addr_save) ) {   // write request in the same cache line 
                    r_wbuf.write(r_dcache_addr_save, r_dcache_be_save, r_dcache_wdata_save);
                    // close the write packet if uncached
                    if ( !r_dcache_cached_save ) r_dcache_write_req = true ;
                } else {    
                    // close the write packet if write request not in the same cache line
                    r_dcache_write_req = true;  
                    m_cost_write_frz++;
                    break;  // posting request not possible : stay in DCACHE_WRITEREQ state
                }
            } else {    //  previous write transaction not completed
                m_cost_write_frz++;
                break;  // posting request not possible : stay in DCACHE_WRITEREQ state  
            }

            // close the write packet if the next processor request is not a write 
            if ( !dreq.valid || (dreq.type != iss_t::DATA_WRITE) ) r_dcache_write_req = true ;

            // The next state and the processor request parameters are computed 
            // as in the DCACHE_IDLE state (see below ...)
        }
        /////////////////
    case DCACHE_IDLE:
        {
            if ( r_tgt_dcache_req ) {   // external request
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            } 

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
                    dcache_hit = ( !r_dcache_unc_req && (dreq.addr == r_dcache_addr_save) );
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
                    if ( dreq.addr/4 == iss_t::XTN_DCACHE_INVAL )
                        r_dcache_fsm = DCACHE_INVAL;
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

            } else {    // end if dreq.valid
                r_dcache_fsm = DCACHE_IDLE;
            }
            // processor request are not accepted in the WRITE_REQUEST state
            // when the write buffer is not writeable
            if ( (r_dcache_fsm == DCACHE_WRITE_REQ) &&
                 (r_dcache_write_req || !r_wbuf.wok(r_dcache_addr_save)) ) {
                drsp.valid = false;
                drsp.rdata = 0;
            }
            break;
        }
        ///////////////////////
    case DCACHE_WRITE_UPDT:
        {
            m_cpt_dcache_data_write++;
            data_t mask = vci_param::be2mask(r_dcache_be_save);
            data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
            r_dcache.write(r_dcache_addr_save, wdata);
            r_dcache_fsm = DCACHE_WRITE_REQ;
            break;
        }
        //////////////////////
    case DCACHE_MISS_WAIT:
        {
            if ( dreq.valid ) m_cost_data_miss_frz++;
            if ( r_tgt_dcache_req.read() ) {   // external request
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            }
            if ( !r_dcache_miss_req && !r_dcache_inval_rsp ) { // Miss read response and no invalidation
                if ( r_vci_rsp_data_error ) {
                    r_dcache_fsm = DCACHE_ERROR;
                } else {
                    r_dcache_fsm = DCACHE_MISS_UPDT;
                }
                break;
            }
            if ( !r_dcache_miss_req && r_dcache_inval_rsp ) { // Miss read response and invalidation
                if ( r_vci_rsp_data_error ) {
                    r_dcache_inval_rsp  = false;
                    r_dcache_fsm = DCACHE_ERROR;
                } else {
                    r_dcache_inval_rsp  = false;
                    r_dcache_fsm = DCACHE_IDLE;
                }
                break;
            }
            break;
        }
        //////////////////////
    case DCACHE_MISS_UPDT:
        {
            if ( r_tgt_dcache_req.read() ) {   // external request
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            }
            if( !r_dcache_cleanup_req.read() && !r_dcache_inval_rsp ){
                addr_t  ad  = r_dcache_addr_save;
                data_t* buf = new data_t[m_dcache_words];
                for(size_t i=0; i<m_dcache_words; i++) {
                    buf[i] = r_dcache_miss_buf[i];
                }
                data_t  victim_index = 0;
                if ( dreq.valid ) m_cost_data_miss_frz++;
                m_cpt_dcache_data_write++;
                m_cpt_dcache_dir_write++;

                r_dcache_cleanup_req = r_dcache.update(ad, buf, &victim_index);
                r_dcache_cleanup_line = victim_index;
            
                r_dcache_fsm = DCACHE_IDLE;
                delete [] buf;
                break;
            }
            if( r_dcache_inval_rsp ){
                r_dcache_inval_rsp  = false;
                r_dcache_fsm = DCACHE_IDLE;
                break;
            }
            break;
        }
        ////////////////////
    case DCACHE_UNC_WAIT:
        {
            if ( dreq.valid ) m_cost_unc_read_frz++;
            if ( r_tgt_dcache_req ) {   // external request
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            } 
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
        }
        //////////////////
    case DCACHE_ERROR:
        {
            r_dcache_fsm = DCACHE_IDLE;
            r_vci_rsp_data_error = false;
            drsp.error = true;
            drsp.valid = true;
            break;
        }
        /////////////////    
    case DCACHE_INVAL:
        {
            m_cpt_dcache_dir_read += m_dcache_ways;
            r_dcache.inval(r_dcache_addr_save);
            r_dcache_fsm = DCACHE_IDLE;
            break;
        }
        /////////////////////
    case DCACHE_CC_CHECK:   // read directory in case of invalidate or update request
        {
            m_cpt_dcache_dir_read += m_dcache_ways;
            m_cpt_dcache_data_read += m_dcache_ways;
            addr_t  ad           = r_tgt_addr;
            data_t  dcache_rdata = 0;

            if(( ( r_dcache_fsm_save == DCACHE_MISS_WAIT ) || ( r_dcache_fsm_save == DCACHE_MISS_UPDT ) ) && 
               ( (r_dcache_addr_save.read() & ~((m_dcache_words<<2)-1)) == (ad & ~((m_dcache_words<<2)-1)))) {
                r_dcache_inval_rsp = true;
                r_tgt_dcache_req = false;
                r_dcache_fsm = r_dcache_fsm_save;
            } else {
                bool    dcache_hit   = r_dcache.read(ad, &dcache_rdata);
                if ( dcache_hit && r_tgt_update ) {
                    r_dcache_fsm = DCACHE_CC_UPDT;
                    // complete the line buffer in case of update
                    for ( size_t word = 0 ; word < m_dcache_words ; word++ ) {
                        if ( !r_tgt_val[word] ) {
                            addr_t  ad = r_tgt_addr + (addr_t)word;
                            r_dcache.read(ad, &dcache_rdata);
                            r_tgt_buf[word] = dcache_rdata;
                        }
                    }
                } else if ( dcache_hit && !r_tgt_update ) {
                    r_dcache_fsm = DCACHE_CC_INVAL;
                } else {
                    r_dcache_fsm = DCACHE_CC_NOP;
                }
            }
            break;
        }
        ///////////////////
    case DCACHE_CC_UPDT:    // update directory and data cache        
        {
            m_cpt_dcache_dir_write++;
            m_cpt_dcache_data_write++;
            addr_t  ad      = r_tgt_addr;
            data_t* buf     = r_tgt_buf;
            for(size_t i=0; i<m_dcache_words; i++){
                if(r_tgt_val[i]) r_dcache.write( ad + i*4, buf[i]);
            }
            r_tgt_dcache_req = false;
            r_dcache_fsm = r_dcache_fsm_save;
            break;
        }
        /////////////////////
    case DCACHE_CC_INVAL:   // invalidate a cache line
        {
            addr_t  ad      = r_tgt_addr;
            r_tgt_dcache_rsp = r_dcache.inval(ad);
            r_tgt_dcache_req = false;
            r_dcache_fsm = r_dcache_fsm_save;
            break;
        }
        ///////////////////
    case DCACHE_CC_NOP:     // no external hit
        {
            r_tgt_dcache_req = false;
            r_dcache_fsm = r_dcache_fsm_save;
            break;
        }    
    } // end switch r_dcache_fsm

#if DEBUG_CC_XCACHE_WRAPPER
    std::cout << " Data Response: " << drsp << std::endl;
#endif

    /////////// execute one iss cycle /////////////////////////////////////////////
    {
        uint32_t it = 0;
        for (size_t i=0; i<(size_t)iss_t::n_irq; i++) 
            if(p_irq[i].read()) it |= (1<<i);
        m_iss.executeNCycles(1, irsp, drsp, it);
    }

    if ( (ireq.valid && !irsp.valid) || (dreq.valid && !drsp.valid) ) m_cpt_frz_cycles++;
    

    ////////////////////////////////////////////////////////////////////////////
    // The VCI_CMD FSM controls the following ressources:
    // - r_vci_cmd_fsm
    // - r_vci_cmd_min
    // - r_vci_cmd_max
    // - r_vci_cmd_cpt
    // - wbuf reset
    //
    // This FSM handles requests from both the DCACHE FSM & the ICACHE FSM.
    // There is 7 request types, with the following priorities : 
    // 1 - Instruction Miss     : r_icache_miss_req
    // 2 - Data Write           : r_dcache_write_req
    // 3 - Data Read Miss       : r_dcache_miss_req 
    // 4 - Data Read Uncached   : r_dcache_unc_req 
    // 5 - Instruction Cleanup  : r_icache_cleanup_req 
    // 6 - Data Cleanup         : r_dcache_cleanup_req 
    // There is at most one (CMD/RSP) VCI transaction, as both CMD_FSM 
    // and RSP_FSM exit simultaneously the IDLE state.
    //
    // VCI formats:
    // According to the VCI advanced specification, all read requests packets 
    // (read Uncached, Miss data, Miss instruction) are one word packets.
    // For write burst packets, all words must be in the same cache line,
    // and addresses must be contiguous (the BE field is 0 in case of "holes").
    //////////////////////////////////////////////////////////////////////////////

    switch (r_vci_cmd_fsm) {
    
    case CMD_IDLE:
        if (r_vci_rsp_fsm != RSP_IDLE) break;

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
        } else if ( r_icache_cleanup_req ) { 
            r_vci_cmd_fsm = CMD_INS_CLEANUP; 
        } else if ( r_dcache_cleanup_req ) { 
            r_vci_cmd_fsm = CMD_DATA_CLEANUP;
        }
        break;
            
    case CMD_DATA_WRITE:
        if ( p_vci_ini.cmdack.read() ) {
            r_vci_cmd_cpt = r_vci_cmd_cpt + 1;
            if (r_vci_cmd_cpt == r_vci_cmd_max) {
                r_vci_cmd_fsm = CMD_IDLE ;
                r_wbuf.reset() ;
            }
        }
        break;

    case CMD_INS_MISS:
    case CMD_INS_UNC:
    case CMD_DATA_MISS:
    case CMD_DATA_UNC:
    case CMD_INS_CLEANUP:
    case CMD_DATA_CLEANUP:
        if ( p_vci_ini.cmdack.read() ) {
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
    // - r_icache_cleanup_req reset
    // - r_dcache_cleanup_req reset
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
        if(p_vci_ini.rspval.read())
            {
                std::cout << "CC_XCache " << m_srcid << " Unexpected response" << std::endl;
            }
        assert( ! p_vci_ini.rspval.read() && "Unexpected response" );
        if (r_vci_cmd_fsm != CMD_IDLE) break;

        r_vci_rsp_cpt = 0;
        if      ( r_icache_miss_req )       r_vci_rsp_fsm = RSP_INS_MISS;
        else if ( r_icache_unc_req )        r_vci_rsp_fsm = RSP_DATA_WRITE;
        else if ( r_dcache_write_req )      r_vci_rsp_fsm = RSP_DATA_WRITE;
        else if ( r_dcache_miss_req )       r_vci_rsp_fsm = RSP_DATA_MISS;
        else if ( r_dcache_unc_req )        r_vci_rsp_fsm = RSP_DATA_UNC;
        else if ( r_icache_cleanup_req )    r_vci_rsp_fsm = RSP_INS_CLEANUP;
        else if ( r_dcache_cleanup_req )    r_vci_rsp_fsm = RSP_DATA_CLEANUP;
        break;

    case RSP_INS_MISS:
        m_cost_imiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;
        assert( (r_vci_rsp_cpt < m_icache_words) &&
                "The VCI response packet for instruction miss is too long" );
        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_icache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini.rdata.read();

        if ( p_vci_ini.reop.read() ) {
            assert( (r_vci_rsp_cpt == m_icache_words - 1) &&
                    "The VCI response packet for instruction miss is too short");
            r_icache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        }
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL ) r_vci_rsp_ins_error = true;
        break;

    case RSP_INS_UNC:
        m_cost_imiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;
        assert(p_vci_ini.reop.read() &&
               "illegal VCI response packet for uncached instruction");
        r_icache_miss_buf[0] = (data_t)p_vci_ini.rdata.read();
        r_vci_rsp_fsm = RSP_IDLE;
        r_icache_unc_req = false;
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL ) r_vci_rsp_ins_error = true;
        break;

    case RSP_DATA_MISS:
        m_cost_dmiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;
        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");
        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini.rdata.read();
        if ( p_vci_ini.reop.read() ) {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                   "illegal VCI response packet for data read miss");
            r_dcache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        }
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL ) r_vci_rsp_data_error = true;
        break;

    case RSP_DATA_WRITE:
        m_cost_write_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;
        if ( p_vci_ini.reop.read() ) {
            r_vci_rsp_fsm = RSP_IDLE;
            r_dcache_write_req = false;
        }
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL ) m_iss.setWriteBerr();
        break;

    case RSP_DATA_UNC:
        m_cost_unc_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;
        assert(p_vci_ini.reop.read() &&
               "illegal VCI response packet for data read uncached");
        r_dcache_miss_buf[0] = (data_t)p_vci_ini.rdata.read();
        r_vci_rsp_fsm = RSP_IDLE;
        r_dcache_unc_req = false;
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL ) r_vci_rsp_data_error = true;
        break;

    case RSP_INS_CLEANUP:
    case RSP_DATA_CLEANUP:
        if ( ! p_vci_ini.rspval.read() )
            break;
        assert( p_vci_ini.reop.read() &&
                "illegal VCI response packet for icache cleanup");
        assert( (p_vci_ini.rerror.read() == vci_param::ERR_NORMAL) &&
                "error in response packet for icache cleanup");
        if ( r_vci_rsp_fsm == RSP_INS_CLEANUP ) r_icache_cleanup_req = false;
        else                                    r_dcache_cleanup_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;
        
    } // end switch r_vci_rsp_fsm

} // end transition()

//////////////////////////////////////////////////////////////////////////////////
tmpl(void)::genMoore()
           //////////////////////////////////////////////////////////////////////////////////
{
    // VCI initiator response

    p_vci_ini.rspack = true;

    // VCI initiator command

    switch (r_vci_cmd_fsm.read() ) {

    case CMD_IDLE:
        p_vci_ini.cmdval  = false;
        p_vci_ini.address = 0;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0;
        p_vci_ini.plen    = 0;
        p_vci_ini.cmd     = vci_param::CMD_NOP;
        p_vci_ini.trdid   = 0;
        p_vci_ini.pktid   = 0;
        p_vci_ini.srcid   = 0;
        p_vci_ini.cons    = false;
        p_vci_ini.wrap    = false;
        p_vci_ini.contig  = false;
        p_vci_ini.clen    = 0;
        p_vci_ini.cfixed  = false;
        p_vci_ini.eop     = false;
        break;

    case CMD_DATA_UNC:
        p_vci_ini.cmdval = true;
        p_vci_ini.address = r_dcache_addr_save & ~0x3;
        switch( r_dcache_type_save ) {
        case iss_t::DATA_READ:
            p_vci_ini.wdata = 0;
            p_vci_ini.be  = r_dcache_be_save.read();
            p_vci_ini.cmd = vci_param::CMD_READ;
            break;
        case iss_t::DATA_LL:
            p_vci_ini.wdata = 0;
            p_vci_ini.be  = 0xF;
            p_vci_ini.cmd = vci_param::CMD_LOCKED_READ;
            break;
        case iss_t::DATA_SC:
            p_vci_ini.wdata = r_dcache_wdata_save.read();
            p_vci_ini.be  = 0xF;
            p_vci_ini.cmd = vci_param::CMD_STORE_COND;
            break;
        default:
            assert("this should not happen");
        }
        p_vci_ini.plen = 4;
        p_vci_ini.trdid  = 0;   // data cache uncached read
        p_vci_ini.pktid  = 0;
        p_vci_ini.srcid  = m_srcid;
        p_vci_ini.cons   = false;
        p_vci_ini.wrap   = false;
        p_vci_ini.contig = true;
        p_vci_ini.clen   = 0;
        p_vci_ini.cfixed = false;
        p_vci_ini.eop    = true;
        break;

    case CMD_DATA_WRITE:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_wbuf.getAddress(r_vci_cmd_cpt);
        p_vci_ini.wdata   = r_wbuf.getData(r_vci_cmd_cpt);
        p_vci_ini.be      = r_wbuf.getBe(r_vci_cmd_cpt);
        p_vci_ini.plen    = (r_vci_cmd_max - r_vci_cmd_min + 1)<<2;
        p_vci_ini.cmd     = vci_param::CMD_WRITE;
        p_vci_ini.trdid   = 0;  // data cache write
        p_vci_ini.pktid   = 0;
        p_vci_ini.srcid   = m_srcid;
        p_vci_ini.cons    = false;
        p_vci_ini.wrap    = false;
        p_vci_ini.contig  = true;
        p_vci_ini.clen    = 0;
        p_vci_ini.cfixed  = false;
        p_vci_ini.eop     = (r_vci_cmd_cpt == r_vci_cmd_max);
        break;

    case CMD_DATA_MISS:
        p_vci_ini.cmdval = true;
        p_vci_ini.address = r_dcache_addr_save & m_dcache_yzmask;
        p_vci_ini.be     = 0xF;
        p_vci_ini.plen   = m_dcache_words << 2;
        p_vci_ini.cmd    = vci_param::CMD_READ;
        p_vci_ini.trdid  = 1;   // data cache cached read
        p_vci_ini.pktid  = 0;
        p_vci_ini.srcid  = m_srcid;
        p_vci_ini.cons   = false;
        p_vci_ini.wrap   = false;
        p_vci_ini.contig = true;
        p_vci_ini.clen   = 0;
        p_vci_ini.cfixed = false;
        p_vci_ini.eop = true;
        break;

    case CMD_INS_MISS:
        p_vci_ini.cmdval = true;
        p_vci_ini.address = r_icache_addr_save & m_icache_yzmask;
        p_vci_ini.be     = 0xF;
        p_vci_ini.plen   = m_icache_words << 2;
        p_vci_ini.cmd    = vci_param::CMD_READ;
        p_vci_ini.trdid  = 3;   // ins cache cached read
        p_vci_ini.pktid  = 0;
        p_vci_ini.srcid  = m_srcid;
        p_vci_ini.cons   = false;
        p_vci_ini.wrap   = false;
        p_vci_ini.contig = true;
        p_vci_ini.clen   = 0;
        p_vci_ini.cfixed = false;
        p_vci_ini.eop = true;
        break;

    case CMD_INS_UNC:
        p_vci_ini.cmdval = true;
        p_vci_ini.address = r_icache_addr_save & ~0x3;
        p_vci_ini.be     = 0xF;
        p_vci_ini.plen   = 4;
        p_vci_ini.cmd    = vci_param::CMD_READ;
        p_vci_ini.trdid  = 2;   // ins cache uncached read
        p_vci_ini.pktid  = 0;
        p_vci_ini.srcid  = m_srcid;
        p_vci_ini.cons   = false;
        p_vci_ini.wrap   = false;
        p_vci_ini.contig = true;
        p_vci_ini.clen   = 0;
        p_vci_ini.cfixed = false;
        p_vci_ini.eop = true;
        break;

    case CMD_INS_CLEANUP:
    case CMD_DATA_CLEANUP:
        p_vci_ini.cmdval = true;
        if ( r_vci_cmd_fsm == CMD_INS_CLEANUP ) p_vci_ini.address = r_icache_cleanup_line.read() * (m_icache_words<<2);
        else                                    p_vci_ini.address = r_dcache_cleanup_line.read() * (m_icache_words<<2);
        p_vci_ini.wdata  = 0;
        p_vci_ini.be     = 0;
        p_vci_ini.plen   = 4;
        p_vci_ini.cmd    = vci_param::CMD_WRITE;
        p_vci_ini.trdid  = 1; // cleanup for distinguish from a write
        p_vci_ini.pktid  = 0;
        p_vci_ini.srcid  = m_srcid;
        p_vci_ini.cons   = false;
        p_vci_ini.wrap   = false;
        p_vci_ini.contig = false;
        p_vci_ini.clen   = 0;
        p_vci_ini.cfixed = false;
        p_vci_ini.eop = true;
        break;

    } // end switch r_vci_cmd_fsm

    // VCI_TGT

    switch ( r_vci_tgt_fsm.read() ) {

    case TGT_IDLE:
    case TGT_UPDT_WORD:
    case TGT_UPDT_DATA:
        p_vci_tgt.cmdack  = true;
        p_vci_tgt.rspval  = false;
        break;

    case TGT_RSP_BROADCAST:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() && ( r_tgt_icache_rsp | r_tgt_dcache_rsp );
        p_vci_tgt.rsrcid  = r_tgt_srcid.read();
        p_vci_tgt.rpktid  = r_tgt_pktid.read();
        p_vci_tgt.rtrdid  = r_tgt_trdid.read();
        p_vci_tgt.rdata   = 0;
        p_vci_tgt.rerror  = 0;
        p_vci_tgt.reop    = true;
        break;

    case TGT_RSP_DCACHE:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = !r_tgt_dcache_req.read();
        p_vci_tgt.rsrcid  = r_tgt_srcid.read();
        p_vci_tgt.rpktid  = r_tgt_pktid.read();
        p_vci_tgt.rtrdid  = r_tgt_trdid.read();
        p_vci_tgt.rdata   = 0;
        p_vci_tgt.rerror  = 0;
        p_vci_tgt.reop    = true;
        break;

    case TGT_REQ_BROADCAST:
    case TGT_REQ_DCACHE:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = false;
        break;

    } // end switch TGT_FSM
} // end genMoore()

}} // end namespace

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4




