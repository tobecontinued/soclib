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
 * Maintainers: alain 
 *              eric.guthmuller@polytechnique.edu 
 *              nipo
 *              malek <abdelmalek.si-merabet@lip6.fr> 
 */

/////////////////////////////////////////////////////////////////////////////
// History
// - 25/04/2008
//   The existing vci_xcache component has been extended to include 
//   a VCI target port to support a directory based coherence protocol.
//   Three types of packets can be send by the L2 cache to the L1 cache
//   * MULTICAST INVALIDATE packets : length = 1
//   * MULTICAST UPDATE packets : length = n + 2   
//   * BROADCAST INVALIDATE packets : length = 1
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
// - 08/12/2009
//   The update instruction coherence request (code 0XC) are supported. 
// - 26/12/2009
//   The VCI_CC_XCACHE_WRAPPER_MULTI is derived from the VCI_CC_XCACHE_WRAPPER_V1
//   to support several concurrent VCI transactions, in order to improve the CPI.
//   A new write buffer supporting simultaneous write transactions has been 
//   introduced. The VCI command & response FSMs are not synchronized anymore:
//   The read requests can be transmitted before previous write requests
//   (if the missing address does not match apending write in the write buffer)
//   and several read & write requests can be simultaneously transmitted.
//   The transactions can complete in any order, depending on the network.
//   Two new flip-flops have been introduced to signal completion of the
//   read transactions from the RSP FSM to the DCACHE & ICACHE FSMs: 
//   r_rsp_data_ok, r_rsp_ins_ok
//   As simultaneous VCI transactions are supported, the TRDID & RTRDID fields
//   are used to transport the transaction index.
//   The transaction index has an odd value for a write transaction, depending
//   on the line index in the write buffer: PKTID = 2*wbuf_index + 1
//   The transaction index has an even value for a read transaction, with only
//   only four possible values, depending on cached/uncached & data/instruction.
//   A new CLEANUP FSM has been introduced to transmit the cleanups 
//   request from the DCACHE & ICACHE FSM on the coherence network.
//   The LL/SC requests are still uncached.
///////////////////////////////////////////////////////////////////////////////

#define SOCLIB_MODULE_DEBUG 1

#include <cassert>
#include "arithmetics.h"
#include "../include/vci_cc_xcache_wrapper_multi.h"

namespace soclib { 
namespace caba {

#if SOCLIB_MODULE_DEBUG
    namespace {
        const char *dcache_fsm_state_str[] = {
            "DCACHE_IDLE        ",
            "DCACHE_WRITE_UPDT  ",
            "DCACHE_WRITE_REQ   ",
            "DCACHE_MISS_SELECT ",
            "DCACHE_MISS_CLEANUP",
            "DCACHE_MISS_WAIT   ",
            "DCACHE_MISS_UPDT   ",
            "DCACHE_UNC_WAIT    ",
            "DCACHE_INVAL       ",
            "DCACHE_SYNC        ",
            "DCACHE_ERROR       ",
            "DCACHE_CC_CHECK    ",
            "DCACHE_CC_INVAL    ",
            "DCACHE_CC_UPDT     ",
        };
        const char *icache_fsm_state_str[] = {
            "ICACHE_IDLE        ",
            "ICACHE_MISS_SELECT ",
            "ICACHE_MISS_CLEANUP",
            "ICACHE_MISS_WAIT   ",
            "ICACHE_MISS_UPDT   ",
            "ICACHE_UNC_WAIT    ",
            "ICACHE_ERROR       ",
            "ICACHE_CC_CHECK    ",
            "ICACHE_CC_INVAL    ",
            "ICACHE_CC_UPDT     ",
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
        const char *tgt_fsm_state_str[] = {
            "TGT_IDLE         ",
            "TGT_UPDT_WORD    ",
            "TGT_UPDT_DATA    ",
            "TGT_REQ_BROADCAST",
            "TGT_REQ_ICACHE   ",
            "TGT_REQ_DCACHE   ",
            "TGT_RSP_BROADCAST",
            "TGT_RSP_ICACHE   ",
            "TGT_RSP_DCACHE   ",
        };
        const char *cleanup_fsm_state_str[] = {
            "CLEANUP_CMD",
            "CLEANUP_DCACHE_RSP",
            "CLEANUP_ICACHE_RSP",
        };
    }
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciCcXCacheWrapperMulti<vci_param, iss_t>

    using soclib::common::uint32_log2;

    /////////////////////////////////////
    tmpl(/**/)::VciCcXCacheWrapperMulti(
    /////////////////////////////////////
            sc_module_name name,
            int proc_id,
            const soclib::common::MappingTable &mtp,
            const soclib::common::MappingTable &mtc,
            const soclib::common::IntTab &initiator_index_p,
            const soclib::common::IntTab &initiator_index_c,
            const soclib::common::IntTab &target_index_c,
            size_t icache_ways,
            size_t icache_sets,
            size_t icache_words,
            size_t dcache_ways,
            size_t dcache_sets,
            size_t dcache_words,
            size_t wbuf_nwords,
            size_t wbuf_nlines )
        : 
            soclib::caba::BaseModule(name),

            p_clk("clk"),
            p_resetn("resetn"),
            p_vci_ini_d("vci_ini_p"),
            p_vci_ini_c("vci_ini_c"),
            p_vci_tgt_c("vci_tgt_c"),

            m_cacheability_table(mtp.getCacheabilityTable<addr_t>()),
            m_segment(mtc.getSegment(target_index_c)),
            m_iss(this->name(), proc_id),
            m_srcid_d(mtp.indexForId(initiator_index_p)),
            m_srcid_c(mtc.indexForId(initiator_index_c)),

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
            r_dcache_cleanup_save("r_dcache_cleanup_save"),
            r_dcache_way_save("r_dcache_way_save"),
            r_dcache_cleanup_req("r_dcache_cleanup_req"),
            r_dcache_cleanup_line("r_dcache_cleanup_line"),
            r_dcache_miss_req("r_dcache_miss_req"),
            r_dcache_unc_req("r_dcache_unc_req"),
            r_dcache_inval_pending("r_dcache_inval_pending"),

            r_icache_fsm("r_icache_fsm"),
            r_icache_fsm_save("r_icache_fsm_save"),
            r_icache_addr_save("r_icache_addr_save"),
            r_icache_cleanup_save("r_icache_cleanup_save"),
            r_icache_way_save("r_icache_way_save"),
            r_icache_miss_req("r_icache_miss_req"),
            r_icache_cleanup_req("r_icache_cleanup_req"),
            r_icache_cleanup_line("r_icache_cleanup_line"),
            r_icache_inval_pending("r_icache_inval_pending"),

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

            r_tgt_fsm("r_tgt_fsm"),
            r_tgt_addr("r_tgt_addr"),
            r_tgt_word("r_tgt_word"),
            r_tgt_update("r_tgt_update"),
            r_tgt_data("r_tgt_data"),
            r_tgt_srcid("r_tgt_srcid"),
            r_tgt_pktid("r_tgt_pktid"),
            r_tgt_trdid("r_tgt_trdid"),
            r_tgt_icache_req("r_tgt_icache_req"),
            r_tgt_dcache_req("r_tgt_dcache_req"),
            r_tgt_icache_rsp("r_tgt_icache_rsp"),
            r_tgt_dcache_rsp("r_tgt_dcache_rsp"),

            r_cleanup_fsm("r_cleanup_fsm"),

            r_wbuf("r_wbuf", wbuf_nwords, wbuf_nlines),
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

    //////////////////////////////////////
    tmpl(/**/)::~VciCcXCacheWrapperMulti()
    //////////////////////////////////////
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
        std::cout << "CPU " << m_srcid_d << " : CPI = " 
            << (float)m_cpt_total_cycles/(m_cpt_total_cycles - m_cpt_frz_cycles) << std::endl ;
    }
    ////////////////////////
    tmpl(void)::print_stats()
        ////////////////////////
    {
        float run_cycles = (float)(m_cpt_total_cycles - m_cpt_frz_cycles);
        std::cout << "------------------------------------" << std:: dec << std::endl
        << "CPU " << m_srcid_d << " / Time = " << m_cpt_total_cycles << std::endl
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
            r_dcache_fsm 	= DCACHE_IDLE;
            r_icache_fsm 	= ICACHE_IDLE;
            r_cmd_fsm 		= CMD_IDLE;
            r_rsp_fsm 		= RSP_IDLE;
            r_tgt_fsm 		= TGT_IDLE;
            r_cleanup_fsm 	= CLEANUP_CMD;

            // write buffer & caches
            r_wbuf.reset();
            r_icache.reset();
            r_dcache.reset();

            // synchronisation flip-flops from ICACHE & DCACHE FSMs to CMD  FSM
            r_icache_miss_req    = false;
            r_icache_unc_req     = false;
            r_dcache_miss_req    = false;
            r_dcache_unc_req     = false;

            // synchronisation flip-flops from ICACHE & DCACHE FSMs to CLEANUP FSM
            r_icache_cleanup_req = false;
            r_dcache_cleanup_req = false;

            // synchronisation flip-flops from TGT FSM to ICACHE & DCACHE FSMs
            r_tgt_icache_req     = false;
            r_tgt_dcache_req     = false;
            r_tgt_icache_rsp     = false;
            r_tgt_dcache_rsp     = false;

            // internal messages in DCACHE et ICACHE FSMs
            r_icache_inval_pending   = false;
            r_dcache_inval_pending   = false;

            // synchronisation flip-flops from the RSP FSM to the ICACHE or DCACHE FSMs
            r_rsp_data_ok 	= false;
            r_rsp_ins_ok	= false;
            r_rsp_data_error   	= false;
            r_rsp_ins_error    	= false;

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

            return;
        }

#if SOCLIB_MODULE_DEBUG
if ( m_srcid_d == 0 )
{
std::cout << "--------------------------------------------" << std::endl
          << std::dec << "CACHE " << m_srcid_d << " / Time = " << m_cpt_total_cycles << std::endl
          << "  " << dcache_fsm_state_str[r_dcache_fsm] 
          << "  " << icache_fsm_state_str[r_icache_fsm]
          << "  " << cmd_fsm_state_str[r_cmd_fsm] 
          << "  " << rsp_fsm_state_str[r_rsp_fsm]
          << "  " << cleanup_fsm_state_str[r_cleanup_fsm]
          << "  " << tgt_fsm_state_str[r_tgt_fsm] << std::endl;
r_wbuf.print();
}
#endif

        m_cpt_total_cycles++;

        /////////////////////////////////////////////////////////////////////
        // The TGT_FSM controls the following ressources:
        // - r_tgt_fsm
        // - r_tgt_buf[nwords]
        // - r_tgt_val[nwords]
        // - r_tgt_update
        // - r_tgt_data
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
        // the ICACHE & DCACHE FSMs in the REQ states. It waits the completion
        // of the update or invalidate request in the RSP states.
        // -  for an invalidate request the VCI packet length is 1 word.
        // The WDATA field contains the line index (i.e. the Z & Y fields).
        // -  for an update request the VCI packet length is (n+2) words.
        // The WDATA field of the first VCI word contains the line index.
        // The WDATA field of the second VCI word contains the word index.
        // The WDATA field of the n following words contains the values.
        // -  for both invalidate & update requests, the VCI response
        // is one single word.
        // In case of errors in the VCI command packet, the simulation
        // is stopped with an error message.
        /////////////////////////////////////////////////////////////////////

        switch( r_tgt_fsm ) {

            case TGT_IDLE:
            {
                if ( p_vci_tgt_c.cmdval.read() ) 
                {
                    addr_t address = p_vci_tgt_c.address.read();

                    if ( p_vci_tgt_c.cmd.read() != vci_param::CMD_WRITE) 
                    {
                        std::cout << "error in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "the received coherence request from " << p_vci_tgt_c.srcid.read() 
                                  << " is not a write" << std::endl;
                        exit(0);
                    }

                    if ( (address != 0x3) && (! m_segment.contains(address)) ) 
                    {
                        std::cout << "error in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "out of segment coherence request" << std::endl;
                        exit(0);
                    }

                    r_tgt_addr = (((addr_t) ((p_vci_tgt_c.be.read() & 0x3) << 32)) | 
                                 ((addr_t) (p_vci_tgt_c.wdata.read()))) * m_dcache_words * 4;      
                    r_tgt_srcid = p_vci_tgt_c.srcid.read();
                    r_tgt_trdid = p_vci_tgt_c.trdid.read();
                    r_tgt_pktid = p_vci_tgt_c.pktid.read();
                    r_tgt_plen  = p_vci_tgt_c.plen.read();
                    
                    if ( address == 0x3 ) // broadcast invalidate 
                    {
                        if ( ! p_vci_tgt_c.eop.read() ) 
                        {
                            std::cout << "error in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                            std::cout << "a broascast request must be one word" << std::endl;
                            exit(0);
                        }
                        r_tgt_update = false; 
                        r_tgt_brdcast= true;
                        r_tgt_fsm = TGT_REQ_BROADCAST;
                        m_cpt_cc_inval++ ;
                    }
                    else                    // multicast-update or multicast-invalidate
                    { 
                        uint32_t cell = address - m_segment.baseAddress(); 
                        if (cell == 0)                  // invalidate data
                        {
                            if ( ! p_vci_tgt_c.eop.read() ) 
                            {
                                std::cout << "error in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                                std::cout << "an invalidate requestn must be one word" << std::endl;
                                exit(0);
                            }
                            r_tgt_brdcast = false;
                            r_tgt_update  = false; 
                            r_tgt_data    = true;
                            r_tgt_fsm = TGT_REQ_DCACHE;
                            m_cpt_cc_inval++ ;
                        } 
                        else if (cell == 4)             // update data
                        {                                
                            if ( p_vci_tgt_c.eop.read() ) 
                            {
                                std::cout << "error in VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                                std::cout << "an update request must be N+2 words" << std::endl;
                                exit(0);
                            }
                            r_tgt_brdcast  = false;
                            r_tgt_update   = true; 
                            r_tgt_data     = true;
                            r_tgt_fsm  = TGT_UPDT_WORD;
                            m_cpt_cc_update++ ;
                        } 
                        else if (cell == 8)              // invalidate instruction
                        {                         
                            if ( ! p_vci_tgt_c.eop.read() ) 
                            {
                                std::cout << "error in VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                                std::cout << "an invalidate request must be one word" << std::endl;
                                exit(0);
                            }
                            r_tgt_brdcast = false;
                            r_tgt_data    = false;
                            r_tgt_update  = false; 
                            r_tgt_fsm = TGT_REQ_ICACHE;
                            m_cpt_cc_inval++ ;
                        } 
                        else if (cell == 12)             // update ins
                        {                                
                            if ( p_vci_tgt_c.eop.read() ) 
                            {
                                std::cout << "error in VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                                std::cout << "an update request must be N+2 words" << std::endl;
                                exit(0);
                            }
                            r_tgt_brdcast = false;
                            r_tgt_update  = true; 
                            r_tgt_data    = false;
                            r_tgt_fsm = TGT_UPDT_WORD;
                            m_cpt_cc_update++ ;
                        } 

                    } // end if address
                } // end if cmdval
                break;
            }
            case TGT_UPDT_WORD:
            {
                if (p_vci_tgt_c.cmdval.read()) 
                {
                    if ( p_vci_tgt_c.eop.read() ) 
                    {
                        std::cout << "error in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "an update command length must be N+2 words" << std::endl;
                        exit(0);
                    }
                    for ( size_t i=0 ; i<m_dcache_words ; i++ ) r_tgt_val[i] = false;
                    r_tgt_word = p_vci_tgt_c.wdata.read(); // the first modified word index
                    r_tgt_fsm = TGT_UPDT_DATA;
                }
                break;
            }
            case TGT_UPDT_DATA:
            {
                if (p_vci_tgt_c.cmdval.read()) 
                {
                    size_t word = r_tgt_word.read();
                    if ( word >= m_dcache_words ) 
                    {
                        std::cout << "error in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "the word index in an update request is wrong" << std::endl;
                        exit(0);
                    }
                    r_tgt_buf[word] = p_vci_tgt_c.wdata.read();
                    if(p_vci_tgt_c.be.read())    r_tgt_val[word] = true;
                    r_tgt_word = word + 1;
                    if (p_vci_tgt_c.eop.read()) {
                        if (r_tgt_data.read())  r_tgt_fsm = TGT_REQ_DCACHE;
                        else                    r_tgt_fsm = TGT_REQ_ICACHE;
                    }
                }
                break;
            }
            case TGT_REQ_ICACHE:
            {
                if ( !r_tgt_icache_req.read() ) 
                {
                    r_tgt_fsm = TGT_RSP_ICACHE; 
                    r_tgt_icache_req = true;
                }
                break;
            }
            case TGT_REQ_DCACHE:
            {
                if ( !r_tgt_dcache_req.read() ) 
                {
                    r_tgt_fsm = TGT_RSP_DCACHE; 
                    r_tgt_dcache_req = true;
                }
                break;
            }
            case TGT_REQ_BROADCAST:
            {
                if ( !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() ) 
                {
                    r_tgt_fsm = TGT_RSP_BROADCAST; 
                    r_tgt_icache_req = true;
                    r_tgt_dcache_req = true;
                }
                break;
            }
            case TGT_RSP_BROADCAST:
            {
                if ( !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() ) 
                {
                    // one response
                    if ( !r_tgt_icache_rsp || !r_tgt_dcache_rsp )
                    {
                        if ( p_vci_tgt_c.rspack.read() )
                        {
                            r_tgt_fsm = TGT_IDLE; 
                            r_tgt_icache_rsp = false;
                            r_tgt_dcache_rsp = false;
                        }
                    }
                    //  two responses  
                    if ( r_tgt_icache_rsp && r_tgt_dcache_rsp )
                    {
                        if ( p_vci_tgt_c.rspack.read() )
                        {
                            r_tgt_icache_rsp = false; // reset one to prepare the second response
                        }
                    }
                    // no need for a response
                    if ( !r_tgt_icache_rsp && !r_tgt_dcache_rsp )
                    {
                        r_tgt_fsm = TGT_IDLE;
                    }
                }
                break;
            }
            case TGT_RSP_ICACHE:
            {
                if ( (p_vci_tgt_c.rspack.read() || !r_tgt_icache_rsp.read()) && !r_tgt_icache_req.read() ) 
                {
                    r_tgt_fsm = TGT_IDLE;
                    r_tgt_icache_rsp = false; 
                }
                break;
            }
            case TGT_RSP_DCACHE:
            {
                if ( (p_vci_tgt_c.rspack.read() || !r_tgt_dcache_rsp.read()) && !r_tgt_dcache_req.read() ) 
                {
                    r_tgt_fsm = TGT_IDLE;
                    r_tgt_dcache_rsp = false; 
                }
                break;
            }
        } // end switch TGT_FSM

        //////////////////////////////////////////////////////////////////////////////
        // The ICACHE FSM controls the following ressources:
        // - r_icache_fsm
        // - r_icache_fsm_save
        // - r_icache instruction cache access
        // - r_icache_addr_save
        // - r_icache_miss_req set
        // - r_icache_unc_req set
        // - r_rsp_ins_ok reset
        // - r_rsp_ins_error reset
        // - r_tgt_icache_req reset
        // - r_tgt_icache_rsp
        // - r_icache_cleanup_req set
        // - r_icache_cleanup_ine
        // - ireq & irsp structures for communication with the processor
        //
        // 1/ External requests (update or invalidate) 
        //    There is an external request when the r_tgt_icache_req flip-flop is set,
        //    These requests are taken into account in the IDLE and WAIT states.
        //    In case of external request the ICACHE FSM goes to the CC_CHECK
        //    state to test the external hit, and returns in the
        //    pre-empted state after this external request is completed.
        //
        // 2/ Processor requests are taken into account only in the IDLE state.
        //    In case of MISS, or in case of uncached instruction, the FSM 
        //    writes the missing address line in the  r_icache_addr_save register 
        //    and sets the r_icache_miss_req or the r_icache_unc_req flip-flops.
        //    These request flip-flops are reset by the CMD FSM.
        //    The r_rsp_ins_ok flip-flop is set by the RSP FSM when the 
        //    transaction is completed. In case of bus error, the RSP FSM sets 
        //    the r_rsp_ins_error flip-flop. They must be reset by the ICACHE FSM.
        //   - CACHED READ MISS => to the MISS_WAIT state, waiting the r_rsp_ins_ok signal.
        //     It can be delayed in the MISS_DELAY state in case of matching pending
        //     cleanup request.  Then it goes to the MISS_UPDT state, then to the CLEANUP_REQ
        //     state (if necessary), and finally to the IDLE state.
        //   - UNCACHED READ  => to the UNC_WAIT state (waiting the r_rsp_ins_ok signal), 
        //     and back to the IDLE state. 
        ////////////////////////////////////////////////////////////////////////////////////

        typename iss_t::InstructionRequest  ireq = ISS_IREQ_INITIALIZER;
        typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

        typename iss_t::DataRequest  dreq = ISS_DREQ_INITIALIZER;
        typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;

        m_iss.getRequests( ireq, dreq );

        switch(r_icache_fsm) {
 
            case ICACHE_IDLE:
            {
                if ( r_tgt_icache_req )    // external request
                {
                    if ( ireq.valid ) m_cost_ins_miss_frz++;
                    r_icache_fsm = ICACHE_CC_CHECK;
                    r_icache_fsm_save = r_icache_fsm;
                    break;
                } 
                if ( ireq.valid ) 
                {
                    data_t  icache_ins = 0;
                    bool    icache_hit = false;
                    bool    icache_cached = m_cacheability_table[(addr_t)ireq.addr];
                    // icache_hit & icache_ins evaluation
                    if ( icache_cached ) 
                    {
                        icache_hit = r_icache.read((addr_t)ireq.addr, &icache_ins);
                    } 
                    else 
                    {
                        icache_hit = (r_rsp_ins_ok && ( (addr_t)ireq.addr == (addr_t)r_icache_addr_save));
                        icache_ins = r_icache_miss_buf[0];
                    }
                    if ( ! icache_hit )  // miss
                    {
                        m_cpt_ins_miss++;
                        m_cost_ins_miss_frz++;
                        r_icache_addr_save = (addr_t)ireq.addr;
                        if ( icache_cached ) 	// cached line
                        {
                            // if the missing line corresponds to a pending cleanup
                            // the miss request to the CMD FSM must be delayed
                            if ( r_icache_cleanup_req && 
                               ((addr_t)r_icache_cleanup_line == (addr_t)(ireq.addr/(m_icache_words*4))) ) 
                            {
                                break;
                            }
                            else
                            {
                                r_icache_fsm 		= ICACHE_MISS_SELECT;
                                r_icache_miss_req 	= true;
                                r_rsp_ins_ok		= false;
                            }
                        } 
                        else 			// uncached line
                        {
                            r_icache_fsm 	= ICACHE_UNC_WAIT;
                            r_icache_unc_req 	= true;
                            r_rsp_ins_ok	= false;
                        }
                    } 
                    else  // hit : the uncached data should not be re-used
                    {
                        r_rsp_ins_ok = false;
                    } 
                    m_cpt_icache_dir_read += m_icache_ways;
                    m_cpt_icache_data_read += m_icache_ways;
                    irsp.valid          = icache_hit;
                    irsp.instruction    = icache_ins;
                } // end if ireq.valid
                break;
            }
            case ICACHE_MISS_SELECT:
            {
                m_cost_ins_miss_frz++;
                size_t	way;
                addr_t	index;
                addr_t  ad = r_icache_addr_save;
                if ( r_icache.select_before_update( ad, &way, &index) )
                {
                    r_icache_fsm          = ICACHE_MISS_CLEANUP; 
                    r_icache_cleanup_save = index;
                    r_icache_way_save     = way;
                }
                else
                {
                    r_icache_way_save     = way;
                    r_icache_fsm          = ICACHE_MISS_WAIT;
                }
                break;
            }
            case ICACHE_MISS_CLEANUP: // try to post a cleanup request to the CLEANUP FSM
            {
                m_cost_ins_miss_frz++;
                if ( r_tgt_icache_req )    // coherence request 
                {
                    r_icache_fsm = ICACHE_CC_CHECK;
                    r_icache_fsm_save = r_icache_fsm;
                    break;
                } 
                if ( !r_icache_cleanup_req )	// no pending cleanup
                {
                    r_icache_cleanup_req 	= true;
                    r_icache_cleanup_line       = r_icache_cleanup_save;
                    r_icache_fsm		= ICACHE_MISS_WAIT;
                }
                break;                
            }
            case ICACHE_MISS_WAIT:  // waiting the response from the RSP FSM
            {
                m_cost_ins_miss_frz++;
                if ( r_tgt_icache_req )    // coherence request
                {
                    r_icache_fsm = ICACHE_CC_CHECK;
                    r_icache_fsm_save = r_icache_fsm;
                    break;
                } 
                if ( r_rsp_ins_ok )  // there is a response
                {
                    if      ( r_rsp_ins_error )		r_icache_fsm = ICACHE_ERROR;
                    else if ( !r_icache_inval_pending )	r_icache_fsm = ICACHE_MISS_UPDT;
                    else
                    {
                        if ( r_icache_cleanup_req ) 
                        {
                            break;
                        }
                        else
                        {
                            r_icache_cleanup_req = true;
                            r_icache_cleanup_line = r_icache_addr_save;
                            r_icache_fsm = ICACHE_IDLE;
                            r_icache_inval_pending = false;
                        }
                    }
                }
                break;
            }
            case ICACHE_MISS_UPDT:  // update the cache 
            {
                m_cost_ins_miss_frz++;
                m_cpt_icache_dir_write++;
                m_cpt_icache_data_write++;
                addr_t 		ad   	= (addr_t) r_icache_addr_save;
                data_t*   	buf   	= (data_t*) r_icache_miss_buf;
                size_t		way	= (size_t) r_icache_way_save;
                r_icache.update_after_select( buf, way, ad );
                r_icache_fsm = ICACHE_IDLE;
                break;
            }
            case ICACHE_UNC_WAIT:
            {
                m_cost_ins_miss_frz++;
                if ( r_tgt_icache_req )    // external request
                {
                    r_icache_fsm = ICACHE_CC_CHECK;
                    r_icache_fsm_save = r_icache_fsm;
                    break;
                } 
                if ( r_rsp_ins_ok ) 
                {
                    if ( r_rsp_ins_error ) 	r_icache_fsm = ICACHE_ERROR;
                    else 			r_icache_fsm = ICACHE_IDLE;
                }
                break;
            }
            case ICACHE_ERROR:
            {
                r_icache_fsm = ICACHE_IDLE;
                r_rsp_ins_error     = false;
                irsp.error          = true;
                irsp.valid          = true;
                break;
            } 
            case ICACHE_CC_CHECK:   // read directory in case of external request
            {
                m_cpt_icache_dir_read += m_icache_ways;
                m_cpt_icache_data_read += m_icache_ways;
                if ( ( r_icache_fsm_save == ICACHE_MISS_WAIT ) && // external request matches a miss
                ((r_icache_addr_save & ~((m_icache_words<<2)-1))==(r_tgt_addr & ~((m_icache_words<<2)-1))))
                {
                    r_icache_inval_pending 	= true;
                    r_tgt_icache_req       	= false;
                    r_tgt_icache_rsp       	= r_tgt_update;  // always a response n case of update 
                    r_icache_fsm 		= r_icache_fsm_save;
                } 
                else  // the external request is not matching a pending miss
                {
                    data_t  data;
                    bool    icache_hit   	= r_icache.read(r_tgt_addr, &data);
                    if ( icache_hit && r_tgt_update )  // hit update
                    {
                        r_icache_fsm = ICACHE_CC_UPDT;
                    } 
                    else if ( icache_hit && !r_tgt_update ) // hit inval
                    {
                        r_icache_fsm = ICACHE_CC_INVAL;
                    } 
                    else	// miss 
                    { 
                        r_tgt_icache_req 	= false;
                        r_tgt_icache_rsp 	= r_tgt_update; // alaways a response in case of update
                        r_icache_fsm 		= r_icache_fsm_save;
                    }
                }
                break;
            }
            case ICACHE_CC_UPDT:    // update the cache line        
            {
                m_cpt_icache_dir_write++;
                m_cpt_icache_data_write++;
                for(size_t i=0; i<m_icache_words; i++)
                {
                    if(r_tgt_val[i]) r_icache.write( (r_tgt_addr + i*4), r_tgt_buf[i] );
                }
                r_tgt_icache_rsp = true;
                r_tgt_icache_req = false;
                r_icache_fsm = r_icache_fsm_save;
                break;
            }
            case ICACHE_CC_INVAL:   // invalidate a cache line
            {
                r_icache.inval(r_tgt_addr);
                r_tgt_icache_rsp = true;
                r_tgt_icache_req = false;
                r_icache_fsm = r_icache_fsm_save;
                break;
            }
        } // end switch r_icache_fsm

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
        // - r_dcache_miss_req set
        // - r_dcache_unc_req set
        // - r_rsp_data_ok reset
        // - r_rsp_data_error reset
        // - r_dcache_cleanup_req set
        // - r_dcache_cleanup_ine
        // - r_tgt_dcache_req reset
        // - r_tgt_dcache_rsp
        // - r_wbuf write()
        // - dreq & drsp structures for communication with the processor
        //
        // 1/ external request (invalidate or update)
        //    There is an external request when the r_tgt_dcache_req flip-flop is set.
        //    External requests are taken into account in the states  IDLE, WRITE_REQ,  
        //    UNC_WAIT, MISS_WAIT, and have the highest priority :
        //    The actions associated to the pre-empted state are not executed, the DCACHE FSM
        //    goes to the CC_CHECK state to execute the requested action, and returns to the
        //    pre-empted state.
        //
        //  2/ processor request  
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
        //   In case of processor request, there is six conditions to exit the IDLE state:
        //   - CACHED READ MISS => to the MISS_WAIT state, waiting the r_rsp_data_ok signal.
        //     It can be delayed in the MISS_DELAY state in case of matching pending
        //     cleanup request.  Then it goes to the MISS_UPDT state, then to the CLEANUP_REQ
        //     state (if necessary), and finally to the IDLE state.
        //   - UNCACHED READ  => to the UNC_WAIT state, waiting the r_rsp_data_ok signal, 
        //     and back to the IDLE state. LL & SC are handled as uncached read.
        //   - WRITE MISS => directly to the WRITE_REQ state to post a request in the
        //     write buffer.
        //   - WRITE HIT => to the WRITE_UPDT state, then to the WRITE_REQ state.
        //   - LINE INVALIDATE => to the INVAL state for one cycle, then to IDLE state.
        //   - SYNC REQUEST => to the SYNC state until the write buffer is empty.
        //
        // Error handling :  Read Bus Errors are synchronous events, but
        // Write Bus Errors are asynchronous events (processor is not frozen).
        // - If a Read Bus Error is detected, the VCI_RSP FSM sets the
        //   r_rsp_data_error flip-flop, and the synchronous error is signaled
        //   by the DCACHE FSM.
        // - If a Write Bus Error is detected, the VCI_RSP FSM  signals
        //   the asynchronous error using the setWriteBerr() method.
        ///////////////////////////////////////////////////////////////////////////////////

        switch ( r_dcache_fsm ) {

        case DCACHE_WRITE_REQ:
	{
            if ( r_tgt_dcache_req )    // coherence request
            {
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            }
            if( !r_wbuf.wok(r_dcache_addr_save) ) 
            {
                // stay in DCACHE_WRITEREQ state if the write request is not accepted 
                // by the write buffer 
                m_cost_write_frz++;
                drsp.valid = false;
                drsp.rdata = 0;
                break;
            }
                // If the write request is accepted by the write buffer
                // the next state and the response to processor request are evaluated
                // as in the DCACHE_IDLE state  below ...
        }     
        case DCACHE_IDLE:
        {
            if ( r_tgt_dcache_req )   // coherence request
            {
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            } 
            if ( dreq.valid ) 
            {              
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
                if ( dcache_cached ) 
                {
                    dcache_hit = r_dcache.read((addr_t) dreq.addr, &dcache_rdata);
                } 
                else 
                {
                    dcache_hit = ( r_rsp_data_ok && ( dreq.addr == r_dcache_addr_save) );
                    dcache_rdata = r_dcache_miss_buf[0];
                }

                // next state & response evaluation
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
                            r_rsp_data_ok = false;
                        } 
                        else 		
                        {
                            if ( dcache_cached )   // miss
                            {
                                m_cpt_data_miss++;
                                m_cost_data_miss_frz++;
                                // if the missing line corresponds to a pending cleanup
                                // the miss request to the CMD FSM must be delayed
                                if ( r_dcache_cleanup_req &&
                                   ((addr_t)r_dcache_cleanup_line == (addr_t)(dreq.addr/(m_dcache_words*4))) )
                                {
                                    break;
                                }
                                else
                                {
                                    r_dcache_miss_req = true;
                                    r_rsp_data_ok = false;
                                    r_dcache_fsm = DCACHE_MISS_WAIT;
                                }
                                drsp.valid = false;
                                drsp.rdata = 0;
                            } 
                            else 		// uncached
                            {
                                m_cpt_unc_read++;
                                m_cost_unc_read_frz++;
                                r_dcache_unc_req = true;
                                r_rsp_data_ok = false;
                                r_dcache_fsm = DCACHE_UNC_WAIT;
                                drsp.valid = false;
                                drsp.rdata = 0;
                            }
                        }
                        break;
                    case iss_t::XTN_READ:
                    case iss_t::XTN_WRITE:
                        // only DCACHE_INVAL & SYNC requests are supported
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
//                          std::cout << "warning in VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
//                          std::cout << "unsupported external access " << dreq.addr/4 << std::endl;
//                          std::cout << "only XTN_DCACHE_INVAL & XTN_SYNC are supported" << std::endl;
                        }
                        r_dcache_fsm = DCACHE_IDLE; 
                        drsp.valid = true;
                        drsp.rdata = 0;
                        break;
                    case iss_t::DATA_WRITE:
                        m_cpt_write++;
                        if ( dcache_hit && dcache_cached ) 
                        {
                            m_cpt_write_cached++;
                            r_dcache_fsm = DCACHE_WRITE_UPDT;
                            drsp.valid = true;
                            drsp.rdata = 0;
                        } 
                        else 
                        {
                            r_dcache_fsm = DCACHE_WRITE_REQ;
                            drsp.valid = true;
                            drsp.rdata = 0;
                        }
                        break;
                } // end switch dreq.type

                r_dcache_addr_save      = dreq.addr;
                r_dcache_type_save      = dreq.type;
                r_dcache_wdata_save     = dreq.wdata;
                r_dcache_be_save        = dreq.be;
                r_dcache_rdata_save     = dcache_rdata;
            } 
            else  // no dreq.valid
            {
                drsp.valid 	= false;
                drsp.rdata 	= 0;
                r_dcache_fsm 	= DCACHE_IDLE;
            }
            break;
        }       
        case DCACHE_WRITE_UPDT:
        {
            m_cpt_dcache_data_write++;
            data_t mask = vci_param::be2mask(r_dcache_be_save);
            data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
            r_dcache.write( r_dcache_addr_save, wdata );
            r_dcache_fsm = DCACHE_WRITE_REQ;
            break;
        }
        case DCACHE_MISS_SELECT: // select a victim
        {
            m_cost_data_miss_frz++;
            size_t	way;
            addr_t	index;
            addr_t  ad = r_dcache_addr_save;
            if ( r_dcache.select_before_update( ad, &way, &index) )
            {
                r_dcache_fsm          = DCACHE_MISS_CLEANUP; 
                r_dcache_cleanup_save = index;
                r_dcache_way_save     = way;
            }
            else
            {
                r_dcache_way_save     = way;
                r_dcache_fsm          = DCACHE_MISS_WAIT;
            }
            break;
        }
        case DCACHE_MISS_CLEANUP: // try to post a cleanup request to the CLEANUP FSM
        {
            m_cost_data_miss_frz++;
            if ( r_tgt_dcache_req )    // coherence request 
            {
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            } 
            if ( !r_dcache_cleanup_req )	// no pending cleanup
            {
                r_dcache_cleanup_req 	= true;
                r_dcache_cleanup_line   = r_dcache_cleanup_save;
                r_dcache_fsm		= DCACHE_MISS_WAIT;
            }
            break;                
        }
        case DCACHE_MISS_WAIT:  // waiting the response from the RSP FSM
        {
            m_cost_data_miss_frz++;
            if ( r_tgt_dcache_req )    // coherence request
            {
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            } 
            if ( r_rsp_data_ok )  // there is a response
            {
                if      ( r_rsp_data_error )		r_dcache_fsm = DCACHE_ERROR;
                else if ( !r_dcache_inval_pending )	r_dcache_fsm = DCACHE_MISS_UPDT;
                else
                {
                    if ( r_dcache_cleanup_req ) 
                    {
                        break;
                    }
                    else
                    {
                        r_dcache_cleanup_req = true;
                        r_dcache_cleanup_line = r_dcache_addr_save;
                        r_dcache_fsm = DCACHE_IDLE;
                        r_dcache_inval_pending = false;
                    }
                }
            }
            break;
        }
        case DCACHE_MISS_UPDT:  // update the cache 
        {
            m_cost_data_miss_frz++;
            m_cpt_dcache_dir_write++;
            m_cpt_dcache_data_write++;
            addr_t 	ad   	= (addr_t) r_dcache_addr_save;
            data_t*   	buf   	= (data_t*) r_dcache_miss_buf;
            size_t	way	= (size_t) r_dcache_way_save;
            r_dcache.update_after_select( buf, way, ad );
            r_dcache_fsm = DCACHE_IDLE;
            break;
        }
        case DCACHE_UNC_WAIT:
        {
            if ( dreq.valid ) m_cost_unc_read_frz++;
            if ( r_tgt_dcache_req )    // external request
            {
                r_dcache_fsm = DCACHE_CC_CHECK;
                r_dcache_fsm_save = r_dcache_fsm;
                break;
            } 
            if ( r_rsp_data_ok ) 
            {
                if ( r_rsp_data_error ) r_dcache_fsm = DCACHE_ERROR;
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
            if( !r_dcache_cleanup_req )
            {
                m_cpt_dcache_dir_read += m_dcache_ways;
                r_dcache_cleanup_req = r_dcache.inval( r_dcache_addr_save );
                r_dcache_cleanup_line = r_dcache_addr_save.read() >> (uint32_log2(m_dcache_words)+2);
                r_dcache_fsm = DCACHE_IDLE;
            }
            break;
        }
        case DCACHE_SYNC:
        {
            if ( r_wbuf.empty() ) r_dcache_fsm = DCACHE_IDLE; 
            break;
        }
        case DCACHE_CC_CHECK:   // read directory in case of external request
            m_cpt_dcache_dir_read += m_dcache_ways;
            m_cpt_dcache_data_read += m_dcache_ways;
            if ( ( r_dcache_fsm_save == DCACHE_MISS_WAIT ) && // external request matches a miss
            ((r_dcache_addr_save & ~((m_dcache_words<<2)-1))==(r_tgt_addr & ~((m_dcache_words<<2)-1))))
                {
                    r_dcache_inval_pending 	= true;
                    r_tgt_dcache_req       	= false;
                    r_tgt_dcache_rsp       	= r_tgt_update; // always a response to an update
                    r_dcache_fsm 		= r_dcache_fsm_save;
                } 
                else  // the external request is not matching a pending miss
                {
                    data_t  data;
                    bool    dcache_hit   	= r_dcache.read(r_tgt_addr, &data);
                    if ( dcache_hit && r_tgt_update )  // hit update
                    {
                        r_dcache_fsm = DCACHE_CC_UPDT;
                    } 
                    else if ( dcache_hit && !r_tgt_update ) // hit inval
                    {
                        r_dcache_fsm = DCACHE_CC_INVAL;
                    } 
                    else	// miss 
                    { 
                        r_tgt_dcache_req = false;
                        r_tgt_dcache_rsp = r_tgt_update;  // always a respons in case of update
                        r_dcache_fsm = r_dcache_fsm_save;
                    }
                }
                break;
            
            case DCACHE_CC_UPDT:    // update the cache line        
                m_cpt_dcache_dir_write++;
                m_cpt_dcache_data_write++;
                for(size_t i=0; i<m_dcache_words; i++)
                {
                    if(r_tgt_val[i]) r_dcache.write( (r_tgt_addr + i*4), r_tgt_buf[i] );
                }
                r_tgt_dcache_rsp = true;
                r_tgt_dcache_req = false;
                r_dcache_fsm = r_dcache_fsm_save;
                break;
 
            case DCACHE_CC_INVAL:   // invalidate a cache line
                r_dcache.inval(r_tgt_addr);
                r_tgt_dcache_rsp = true;
                r_tgt_dcache_req = false;
                r_dcache_fsm = r_dcache_fsm_save;
                break;
        } // end switch r_dcache_fsm

        ////////// write buffer handling //////////////////
        if( r_dcache_fsm == DCACHE_WRITE_REQ ) 
            r_wbuf.write(true, r_dcache_addr_save, r_dcache_be_save, r_dcache_wdata_save);
        else 
            r_wbuf.write(false, 0, 0, 0);

#if SOCLIB_MODULE_DEBUG
if ( m_srcid_d == 0 )
std::cout << ireq << std::endl << irsp << std::endl << dreq << std::endl << drsp << std::endl;
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
        // This CLEANUP FSM controls the transmission of the cleanup transactions
        // on the coherence network. It controls the following ressources:
        // - r_cleanup_fsm
        // - r_dcache_cleanup_req reset
        // - r_icache_cleanup_req reset
        // 
        // This FSM handles cleanup requests from both the DCACHE FSM & ICACHE FSM
        // 1 - Instruction Cleanup  : r_icache_cleanup_req 
        // 2 - Data Cleanup         : r_dcache_cleanup_req 
        // In case of simultaneous requests, the data request have highest priority.
        // There is only one cleanup transaction at a given time (sequencial behavior)
        // because the same FSM controls both command & response. 
        // The the r_icache_cleanup_req & r_dcache_cleanup_req are reset only
        // when the response packet is received.
        // Error handling :
        // As the coherence trafic is controled by hardware, errors are not reported
        // to software : In case of errors, the simulation stops.
        ////////////////////////////////////////////////////////////////////////////

        switch (r_cleanup_fsm) {

            case CLEANUP_CMD:
            {    
                if ( p_vci_ini_c.cmdack )
                {
                    if      (r_dcache_cleanup_req) 	r_cleanup_fsm = CLEANUP_DCACHE_RSP;
                    else if (r_icache_cleanup_req) 	r_cleanup_fsm = CLEANUP_ICACHE_RSP;
                }
                break;
            }
            case CLEANUP_DCACHE_RSP:
            {
                if ( p_vci_ini_c.rspval )
                {
                    assert( p_vci_ini_c.reop && (p_vci_ini_c.rtrdid.read() == 0) &&
                      "illegal response packet received for a cleanup transaction");
                    assert( (p_vci_ini_c.rerror.read() == vci_param::ERR_NORMAL) && 
                      "error signaled in a cleanup response" );
                    
                    r_cleanup_fsm = CLEANUP_CMD;
                    r_dcache_cleanup_req = false;
                }
                break;
            }
            case CLEANUP_ICACHE_RSP:
            {
                if ( p_vci_ini_c.rspval )
                {
                    assert( p_vci_ini_c.reop && (p_vci_ini_c.rtrdid.read() == 1) &&
                      "illegal response packet received for a cleanup transaction");
                    assert( (p_vci_ini_c.rerror.read() == vci_param::ERR_NORMAL) && 
                      "error signaled in a cleanup response" );
                    
                    r_cleanup_fsm = CLEANUP_CMD;
                    r_icache_cleanup_req = false;
                }
                break;
            }
        } // end switch r_cleanup_fsm    
        
        ////////////////////////////////////////////////////////////////////////////
        // The CMD FSM controls the transmission of read & write requests
        // on the direct network. It controls the following ressources:
        // - r_cmd_fsm
        // - r_cmd_min
        // - r_cmd_max
        // - r_cmd_cpt
        // - r_dcache_miss_req reset
        // - r_dcache_unc_req reset
        // - r_icache_miss_req reset
        // - r_icache_unc_req reset
        // - wbuf sent()
        //
        // This FSM handles requests from both the DCACHE FSM & the ICACHE FSM.
        // There is 5 request types, with the following priorities : 
        // 1 - Data Read Miss       : r_dcache_miss_req (if no hit in the write buffer)
        // 2 - Data Read Uncached   : r_dcache_unc_req (if no hit in the write buffer)
        // 3 - Instruction Miss     : r_icache_miss_req (if no hit in the write buffer)
        // 4 - Instruction Uncached : r_icache_unc_req (if no hit in the write buffer)
        // 5 - Data Write           : r_wbuf.rok()      
        // The read requests have highest priority, because the processor is blocked.
        //
        // VCI formats:
        // According to the VCI advanced specification, all read requests packets 
        // (read Uncached, Miss data, Miss instruction) are one word packets.
        // For write burst packets, all words must be in the same cache line,
        // and addresses must be contiguous (the BE field is 0 in case of "holes").
        // The PLEN VCI field is always documented.
        // - Read transactions  : index = 4*cached + 2*instruction  (even values)
        //////////////////////////////////////////////////////////////////////////////

        switch (r_cmd_fsm) {

            case CMD_IDLE:
                if ( r_dcache_miss_req & r_wbuf.miss( r_dcache_addr_save ) )
                { 
                    r_cmd_fsm = CMD_DATA_MISS;
                    r_dcache_miss_req = false;
                    m_cpt_dmiss_transaction++; 
                } 
                else if ( r_dcache_unc_req & r_wbuf.miss( r_dcache_addr_save ) )
                { 
                    r_cmd_fsm = CMD_DATA_UNC;
                    r_dcache_unc_req = false;
                    m_cpt_unc_transaction++; 
                }
                else if ( r_icache_miss_req & r_wbuf.miss( r_icache_addr_save ) )
                { 
                    r_cmd_fsm = CMD_INS_MISS; 
                    r_icache_miss_req = false;
                    m_cpt_imiss_transaction++;
                } 
                else if ( r_icache_unc_req & r_wbuf.miss( r_icache_addr_save ) )
                { 
                    r_cmd_fsm = CMD_INS_UNC; 
                    r_icache_unc_req = false;
                    m_cpt_imiss_transaction++;
                }
                else if ( r_wbuf.rok() ) 
                { 
                    r_cmd_fsm = CMD_DATA_WRITE;
                    r_cmd_cpt = r_wbuf.getMin();
                    r_cmd_min = r_wbuf.getMin();
                    r_cmd_max = r_wbuf.getMax(); 
                    m_cpt_write_transaction++; 
                    m_length_write_transaction += (r_wbuf.getMax() - r_wbuf.getMin() + 1); 
                } 
                break;

            case CMD_DATA_WRITE:
                if ( p_vci_ini_d.cmdack.read() ) 
                {
                    r_cmd_cpt = r_cmd_cpt + 1;
                    if (r_cmd_cpt == r_cmd_max) 
                    {
                        r_cmd_fsm = CMD_IDLE ;
                        r_wbuf.sent() ;
                    }
                }
                break;

            case CMD_INS_MISS:
            case CMD_INS_UNC:
            case CMD_DATA_MISS:
            case CMD_DATA_UNC:
                if ( p_vci_ini_d.cmdack.read() )  r_cmd_fsm = CMD_IDLE;
                break;
        } // end  switch r_cmd_fsm

        //////////////////////////////////////////////////////////////////////////
        // The RSP FSM receive the response packets on the direct network.
        // It controls the following ressources:
        // - r_rsp_fsm:
        // - r_icache_miss_buf[m_icache_words]
        // - r_dcache_miss_buf[m_dcache_words]
        // - r_icache_miss_req reset
        // - r_icache_unc_req reset
        // - r_dcache_miss_req reset
        // - r_dcache_unc_req reset
        // - r_icache_cleanup_req reset
        // - r_dcache_cleanup_req reset
        // - r_rsp_data_ok set
        // - r_rsp_ins_ok set
        // - r_rsp_data_error set
        // - r_rsp_ins_error set
        // - r_rsp_cpt
        // 
        // VCI formats:
        // This component accepts only single word write response packets. 
        //
        // Error handling:
        // This FSM analyzes the VCI error code and signals the  Write Bus Error. 
        // In case of Read Data Error, the VCI_RSP FSM sets the r_rsp_data_error 
        // flip_flop and the error is signaled by the DCACHE FSM.  
        // In case of Instruction Error, the VCI_RSP FSM sets the r_rsp_ins_error 
        // flip_flop and the error is signaled by the DCACHE FSM.  
        //////////////////////////////////////////////////////////////////////////

        switch (r_rsp_fsm) {

        case RSP_IDLE:
            if( p_vci_ini_d.rspval.read() )
            {
                r_rsp_cpt = 0;
                if ( p_vci_ini_d.rpktid.read()%2 == 1 )			r_rsp_fsm = RSP_DATA_WRITE;
                else if ( p_vci_ini_d.rpktid.read() == TYPE_DATA_MISS ) r_rsp_fsm = RSP_DATA_MISS;
                else if ( p_vci_ini_d.rpktid.read() == TYPE_DATA_UNC ) 	r_rsp_fsm = RSP_DATA_UNC;
                else if ( p_vci_ini_d.rpktid.read() == TYPE_INS_MISS ) 	r_rsp_fsm = RSP_INS_MISS;
                else if ( p_vci_ini_d.rpktid.read() == TYPE_INS_UNC ) 	r_rsp_fsm = RSP_INS_UNC;
            }
            break;

        case RSP_DATA_WRITE:
            if ( p_vci_ini_d.rspval.read() )
            {
                assert(p_vci_ini_d.reop.read() &&
                   "illegal VCI response packet for a write transaction");
                r_rsp_fsm = RSP_IDLE;
                r_wbuf.completed( p_vci_ini_d.rpktid.read() >> 1 );
                if ( p_vci_ini_d.rerror.read() != vci_param::ERR_NORMAL ) m_iss.setWriteBerr();
            }
            break;

        case RSP_INS_MISS:
            if ( p_vci_ini_d.rspval.read() )
            {  
                assert( (r_rsp_cpt < m_icache_words) &&
                        "The VCI response packet for instruction miss is too long" );
                r_rsp_cpt = r_rsp_cpt + 1;
                r_icache_miss_buf[r_rsp_cpt] = (data_t)p_vci_ini_d.rdata.read();
                if ( p_vci_ini_d.reop.read() ) {
                    assert( (r_rsp_cpt == m_icache_words - 1) &&
                            "The VCI response packet for instruction miss is too short");
                    r_rsp_ins_ok = true;
                    r_rsp_fsm = RSP_IDLE;
                }
                if ( p_vci_ini_d.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_ins_error = true;
            }
            break;

        case RSP_INS_UNC:
            if ( p_vci_ini_d.rspval.read() )
            {
                assert(p_vci_ini_d.reop.read() &&
                   "illegal VCI response packet for uncached instruction");
                r_icache_miss_buf[0] = (data_t)p_vci_ini_d.rdata.read();
                r_rsp_ins_ok = true;
                r_rsp_fsm = RSP_IDLE;
                if ( p_vci_ini_d.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_ins_error = true;
            }
            break;

        case RSP_DATA_MISS:
            if ( p_vci_ini_d.rspval.read() )
            {
                assert(r_rsp_cpt != m_dcache_words &&
                   "illegal VCI response packet for data read miss");
                r_rsp_cpt = r_rsp_cpt + 1;
                r_dcache_miss_buf[r_rsp_cpt] = (data_t)p_vci_ini_d.rdata.read();
                if ( p_vci_ini_d.reop.read() )
                {
                    assert(r_rsp_cpt == m_dcache_words - 1 &&
                       "illegal VCI response packet for instruction miss");
                    r_rsp_data_ok = true;
                    r_rsp_fsm = RSP_IDLE;
                }
                if ( p_vci_ini_d.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_data_error = true;
            }
            break;

        case RSP_DATA_UNC:
            if ( p_vci_ini_d.rspval.read() )
            {
                assert(p_vci_ini_d.reop.read() &&
                   "illegal VCI response packet for uncached read data");
                r_dcache_miss_buf[0] = (data_t)p_vci_ini_d.rdata.read();
                r_rsp_data_ok = true;
                r_rsp_fsm = RSP_IDLE;
                if ( p_vci_ini_d.rerror.read() != vci_param::ERR_NORMAL ) r_rsp_data_error = true;
            }
            break;
        } // end switch r_rsp_fsm

    } // end transition()

    //////////////////////////////////////////////////////////////////////////////////
    tmpl(void)::genMoore()
    //////////////////////////////////////////////////////////////////////////////////
    {
        // Coherence network (initiator port)

        switch ( r_cleanup_fsm.read() ) {

            case CLEANUP_CMD:
                p_vci_ini_c.rspack  = false;
                p_vci_ini_c.cmdval  = r_icache_cleanup_req || r_dcache_cleanup_req;
                if ( r_dcache_cleanup_req )
                {
                    p_vci_ini_c.address =  r_dcache_cleanup_line.read() * m_dcache_words * 4;
                    p_vci_ini_c.trdid   = 0;
                }
                else
                {
                    p_vci_ini_c.address =  r_icache_cleanup_line.read() * m_icache_words * 4;
                    p_vci_ini_c.trdid   = 1;
                }
                p_vci_ini_c.wdata  = 0;
                p_vci_ini_c.be     = 0xF;
                p_vci_ini_c.plen   = 4;
                p_vci_ini_c.cmd    = vci_param::CMD_WRITE;
                p_vci_ini_c.pktid  = 0;
                p_vci_ini_c.srcid  = m_srcid_c;
                p_vci_ini_c.cons   = false;
                p_vci_ini_c.wrap   = false;
                p_vci_ini_c.contig = false;
                p_vci_ini_c.clen   = 0;
                p_vci_ini_c.cfixed = false;
                p_vci_ini_c.eop = true;
                break;

           case CLEANUP_DCACHE_RSP:
                p_vci_ini_c.rspack  = true;
                p_vci_ini_c.cmdval  = false;
                p_vci_ini_c.address = 0;
                p_vci_ini_c.wdata  = 0;
                p_vci_ini_c.be     = 0;
                p_vci_ini_c.plen   = 0;
                p_vci_ini_c.cmd    = vci_param::CMD_WRITE;
                p_vci_ini_c.trdid  = 0; 
                p_vci_ini_c.pktid  = 0;
                p_vci_ini_c.srcid  = 0;
                p_vci_ini_c.cons   = false;
                p_vci_ini_c.wrap   = false;
                p_vci_ini_c.contig = false;
                p_vci_ini_c.clen   = 0;
                p_vci_ini_c.cfixed = false;
                p_vci_ini_c.eop = false;
                break;

           case CLEANUP_ICACHE_RSP:
                p_vci_ini_c.rspack  = true;
                p_vci_ini_c.cmdval  = false;
                p_vci_ini_c.address = 0;
                p_vci_ini_c.wdata  = 0;
                p_vci_ini_c.be     = 0;
                p_vci_ini_c.plen   = 0;
                p_vci_ini_c.cmd    = vci_param::CMD_WRITE;
                p_vci_ini_c.trdid  = 0; 
                p_vci_ini_c.pktid  = 0;
                p_vci_ini_c.srcid  = 0;
                p_vci_ini_c.cons   = false;
                p_vci_ini_c.wrap   = false;
                p_vci_ini_c.contig = false;
                p_vci_ini_c.clen   = 0;
                p_vci_ini_c.cfixed = false;
                p_vci_ini_c.eop = false;
                break;
           } // end switch r_cleanup_fsm

        // Direct network initiator response

        p_vci_ini_d.rspack = ( r_rsp_fsm != RSP_IDLE );

        // Direct network initiator command

        switch ( r_cmd_fsm ) {

            case CMD_IDLE:
                p_vci_ini_d.cmdval  = false;
                p_vci_ini_d.address = 0;
                p_vci_ini_d.wdata   = 0;
                p_vci_ini_d.be      = 0;
                p_vci_ini_d.plen    = 0;
                p_vci_ini_d.cmd     = vci_param::CMD_WRITE;
                p_vci_ini_d.trdid   = 0;
                p_vci_ini_d.pktid   = 0;
                p_vci_ini_d.srcid   = 0;
                p_vci_ini_d.cons    = false;
                p_vci_ini_d.wrap    = false;
                p_vci_ini_d.contig  = false;
                p_vci_ini_d.clen    = 0;
                p_vci_ini_d.cfixed  = false;
                p_vci_ini_d.eop     = false;
                break;

            case CMD_DATA_WRITE:
                p_vci_ini_d.cmdval  = true;
                p_vci_ini_d.address = r_wbuf.getAddress(r_cmd_cpt)&~0x3;
                p_vci_ini_d.wdata   = r_wbuf.getData(r_cmd_cpt);
                p_vci_ini_d.be      = r_wbuf.getBe(r_cmd_cpt);
                p_vci_ini_d.plen    = (r_cmd_max - r_cmd_min + 1)<<2;
                p_vci_ini_d.cmd     = vci_param::CMD_WRITE;
                p_vci_ini_d.pktid   = (r_wbuf.getIndex() << 1) + 1;
                p_vci_ini_d.trdid   = 0;
                p_vci_ini_d.srcid   = m_srcid_d;
                p_vci_ini_d.cons    = false;
                p_vci_ini_d.wrap    = false;
                p_vci_ini_d.contig  = true;
                p_vci_ini_d.clen    = 0;
                p_vci_ini_d.cfixed  = false;
                p_vci_ini_d.eop     = (r_cmd_cpt == r_cmd_max);
                break;

            case CMD_DATA_UNC:
                p_vci_ini_d.cmdval = true;
                p_vci_ini_d.address = r_dcache_addr_save & ~0x3;
                switch( r_dcache_type_save ) {
                    case iss_t::DATA_READ:
                        p_vci_ini_d.wdata = 0;
                        p_vci_ini_d.be  = r_dcache_be_save.read();
                        p_vci_ini_d.cmd = vci_param::CMD_READ;
                        break;
                    case iss_t::DATA_LL:
                        p_vci_ini_d.wdata = 0;
                        p_vci_ini_d.be  = 0xF;
                        p_vci_ini_d.cmd = vci_param::CMD_LOCKED_READ;
                        break;
                    case iss_t::DATA_SC:
                        p_vci_ini_d.wdata = r_dcache_wdata_save.read();
                        p_vci_ini_d.be  = 0xF;
                        p_vci_ini_d.cmd = vci_param::CMD_STORE_COND;
                        break;
                    default:
                        assert("this should not happen");
                }
                p_vci_ini_d.plen = 4;
                p_vci_ini_d.trdid  = 0;
                p_vci_ini_d.pktid  = TYPE_DATA_UNC;
                p_vci_ini_d.srcid  = m_srcid_d;
                p_vci_ini_d.cons   = false;
                p_vci_ini_d.wrap   = false;
                p_vci_ini_d.contig = true;
                p_vci_ini_d.clen   = 0;
                p_vci_ini_d.cfixed = false;
                p_vci_ini_d.eop    = true;
                break;

            case CMD_DATA_MISS:
                p_vci_ini_d.cmdval = true;
                p_vci_ini_d.address = r_dcache_addr_save.read() & (addr_t) m_dcache_yzmask;
                p_vci_ini_d.be     = 0xF;
                p_vci_ini_d.plen   = m_dcache_words << 2;
                p_vci_ini_d.cmd    = vci_param::CMD_READ;
                p_vci_ini_d.trdid  = 0;
                p_vci_ini_d.pktid  = TYPE_DATA_MISS;
                p_vci_ini_d.srcid  = m_srcid_d;
                p_vci_ini_d.cons   = false;
                p_vci_ini_d.wrap   = false;
                p_vci_ini_d.contig = true;
                p_vci_ini_d.clen   = 0;
                p_vci_ini_d.cfixed = false;
                p_vci_ini_d.eop    = true;
                break;

            case CMD_INS_MISS:
                p_vci_ini_d.cmdval = true;
                p_vci_ini_d.address = r_icache_addr_save & (addr_t) m_icache_yzmask;
                p_vci_ini_d.be     = 0xF;
                p_vci_ini_d.plen   = m_icache_words << 2;
                p_vci_ini_d.cmd    = vci_param::CMD_READ;
                p_vci_ini_d.trdid  = 0;
                p_vci_ini_d.pktid  = TYPE_INS_MISS;
                p_vci_ini_d.srcid  = m_srcid_d;
                p_vci_ini_d.cons   = false;
                p_vci_ini_d.wrap   = false;
                p_vci_ini_d.contig = true;
                p_vci_ini_d.clen   = 0;
                p_vci_ini_d.cfixed = false;
                p_vci_ini_d.eop    = true;
                break;

            case CMD_INS_UNC:
                p_vci_ini_d.cmdval = true;
                p_vci_ini_d.address = r_icache_addr_save.read() & ~0x3;
                p_vci_ini_d.be     = 0xF;
                p_vci_ini_d.plen   = 4;
                p_vci_ini_d.cmd    = vci_param::CMD_READ;
                p_vci_ini_c.trdid  = 0;
                p_vci_ini_d.pktid  = TYPE_INS_UNC;
                p_vci_ini_d.srcid  = m_srcid_d;
                p_vci_ini_d.cons   = false;
                p_vci_ini_d.wrap   = false;
                p_vci_ini_d.contig = true;
                p_vci_ini_d.clen   = 0;
                p_vci_ini_d.cfixed = false;
                p_vci_ini_d.eop    = true;
                break;
        } // end switch r_cmd_fsm

        // coherence network : target port

        switch ( r_tgt_fsm.read() ) {

            case TGT_IDLE:
            case TGT_UPDT_WORD:
            case TGT_UPDT_DATA:
                p_vci_tgt_c.cmdack  = true;
                p_vci_tgt_c.rspval  = false;
                break;

            case TGT_RSP_BROADCAST:
                p_vci_tgt_c.cmdack  = false;
                p_vci_tgt_c.rspval  = !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() 
                                       &&  ( r_tgt_icache_rsp | r_tgt_dcache_rsp );
                p_vci_tgt_c.rsrcid  = r_tgt_srcid.read();
                p_vci_tgt_c.rpktid  = r_tgt_pktid.read();
                p_vci_tgt_c.rtrdid  = r_tgt_trdid.read();
                p_vci_tgt_c.rdata   = 0;
                p_vci_tgt_c.rerror  = 0;
                p_vci_tgt_c.reop    = true;
                break;

            case TGT_RSP_ICACHE:
                p_vci_tgt_c.cmdack  = false;
                p_vci_tgt_c.rspval  = !r_tgt_icache_req.read() && r_tgt_icache_rsp.read();
                p_vci_tgt_c.rsrcid  = r_tgt_srcid.read();
                p_vci_tgt_c.rpktid  = r_tgt_pktid.read();
                p_vci_tgt_c.rtrdid  = r_tgt_trdid.read();
                p_vci_tgt_c.rdata   = 0;
                p_vci_tgt_c.rerror  = 0;
                p_vci_tgt_c.reop    = true;
                break;

            case TGT_RSP_DCACHE:
                p_vci_tgt_c.cmdack  = false;
                p_vci_tgt_c.rspval  = !r_tgt_dcache_req.read() && r_tgt_dcache_rsp.read();
                p_vci_tgt_c.rsrcid  = r_tgt_srcid.read();
                p_vci_tgt_c.rpktid  = r_tgt_pktid.read();
                p_vci_tgt_c.rtrdid  = r_tgt_trdid.read();
                p_vci_tgt_c.rdata   = 0;
                p_vci_tgt_c.rerror  = 0;
                p_vci_tgt_c.reop    = true;
                break;

            case TGT_REQ_BROADCAST:
            case TGT_REQ_ICACHE:
            case TGT_REQ_DCACHE:
                p_vci_tgt_c.cmdack  = false;
                p_vci_tgt_c.rspval  = false;
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




