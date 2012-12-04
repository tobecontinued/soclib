/* -*- c++ -*-C
 * File : vci_noc-mmu.cpp
 * Copyright (c) UPMC, Lip6, SoC
 * Author : Alain Greiner
 * Date : 11/11/2012
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
 */

#include <cassert>
#include "arithmetics.h"
#include "alloc_elems.h"
#include "../include/vci_noc_mmu.h"

/////////////////////////////////////////////////////////////////////////////////
// This component implement a multi channels NOC MMU service that can be used 
// with any interconnect respecting the VCI protocol: The virtual address is 
// translated to a physical address, and page based access rights are checked. 
// This component uses the SoCLib generic TLB, and relies on the same two levels
// page table organisation as the vci_vcache_wrapper component.
// The virtual address and the physical address must be 32 bits.
// 
// For each channel, there is a private TLB, a private set of configuration
// registers, a private prefetch buffer (one cache line), and a private storage
// for a complete VCI transaction (including a cache line for wdata).
// for each channel. The channel index is defined by the VCI TRDID field.
// 
// This component implements an hardware "table walk" to handle the TLB Miss.
// The page tables used by his component must only contain 4 KBytes pages
// (2 Mbytes pages are not supported).
// It implements also a prefetch buffer containing several contiguous PTE2
// The number of PTE2 fetched in case of TLB miss is a constructor parameter
// (usualy defined by the memory sub-system cache line width).
/////////////////////////////////////////////////////////////////////////////////
// Implementation note
// This component contains (N+3), where N is the number of channels.
//
//////   debug services   ///////////////////////////////////////////////////////
// All debug messages are conditionned by two variables:
// - compile time : DEBUG_*** : defined below
// - execution time : debug  : defined by constructor arguments
//    debug = (m_debug_ok) and (m_cpt_cycle > m_debug_start)
/////////////////////////////////////////////////////////////////////////////////

#define DEBUG_CMD_FSM	    	1
#define DEBUG_RSP_FSM	    	1
#define DEBUG_CONFIG_FSM		1

namespace soclib { 
namespace caba {

#define tmpl(...)  template<typename vci_param> __VA_ARGS__ VciNocMmu<vci_param>

//////////////////////
tmpl(/**/)::VciNocMmu(
    sc_module_name 			            name,
    const soclib::common::MappingTable 	&mt,
    const soclib::common::Segment       &seg,
    const soclib::common::IntTab 	    &tgtid,
    const soclib::common::IntTab 	    &srcid,
    size_t                              cache_words,
    size_t 				                tlb_ways,
    size_t 				                lb_sets,
    uint32_t				            debug_start,
    bool				                debug_ok)
    : soclib::caba::BaseModule(name),

      p_clk("p_clk"),
      p_resetn("p_resetn"),
      p_vci_ini("p_vci_ini"),
      p_vci_tgt("p_vci_tgt"),
      p_vci_tgt_config("p_vci_tgt_config"),

      m_words(cache_words),
      m_tag_mask(0xFFFFFFFF*cache_words*4),
      m_segment(seg),
      m_srcid(mt.indexForId(srcid)), 
      m_sets(tlb_sets),
      m_ways(tlb_ways),

      m_debug_start(debug_start),
      m_debug_ok(debug_ok),

      r_ptpr("r_ptpr"),
      r_active("r_active"),
      r_bvar("r_bvar"),
      r_xcode("r_xcode"),

      r_cmd_fsm("r_cmd_fsm"),
      r_cmd_srcid("r_cmd_srcid"),       
      r_cmd_trdid("r_cmd_trdid"),       
      r_cmd_pktid("r_cmd_pktid"),       
      r_cmd_buf_tag("r_cmd_buf_tag"),
      r_cmd_buf_val("r_cmd_buf_val"),
      r_cmd_tlb_way("r_cmd_tlb_way"),
      r_cmd_tlb_set("r_cmd_tlb_set"),
      r_cmd_ptd("r_cmd_ptd"),
      r_cmd_count("r_cmd_count"),

      r_cmd_fifo_address("r_cmd_fifo_address",2), 
      r_cmd_fifo_srcid("r_cmd_fifo_srcid",2), 
      r_cmd_fifo_trdid("r_cmd_fifo_trdid",2), 
      r_cmd_fifo_pktid("r_cmd_fifo_pktid",2), 
      r_cmd_fifo_be("r_cmd_fifo_be",2), 
      r_cmd_fifo_cmd("r_cmd_fifo_cmd",2), 
      r_cmd_fifo_wdata("r_cmd_fifo_wdata",2), 
      r_cmd_fifo_plen("r_cmd_fifo_plen",2), 
      r_cmd_fifo_contig("r_cmd_fifo_contig",2), 
      r_cmd_fifo_cons("r_cmd_fifo_cons",2), 
      r_cmd_fifo_wrap("r_cmd_fifo_wrap",2), 
      r_cmd_fifo_eop("r_cmd_fifo_eop",2),

      r_tlb( "tlb", 0, tlb_ways, tlb_sets, 32 ),  // physical address is 32 bits

      r_rsp_fsm("r_rsp_fsm"),

      r_rsp_fifo_rdata("r_rsp_fifo_rdata",2),
      r_rsp_fifo_rsrcid("r_rsp_fifo_rsrcid",2),
      r_rsp_fifo_rtrdid("r_rsp_fifo_rtrdid",2),
      r_rsp_fifo_rpktid("r_rsp_fifo_rpktid",2),
      r_rsp_fifo_rerror("r_rsp_fifo_rerror",2),
      r_rsp_fifo_reop("r_rsp_fifo_reop",2),
      
      r_config_fsm("r_config_fsm"),
     
      r_rsp2cmd_fifo("r_rsp2cmd_fifo",2)
{
    assert( (vci_param::N ==  32) and
            "VCI ADDRESS field must have 32 bits");
    assert( (vci_param::B ==  4) and
            "VCI DATA field must have 32 bits");
    
    r_cmd_buf_ppn      = new vci_data_t[dcache_words/2];
    r_cmd_buf_flags    = new vci_data_t[dcache_words/2];

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();
  
    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();

} // end constructor

/////////////////////////////////////
tmpl(/**/)::~VciNocMmu()
/////////////////////////////////////
{
    delete [] r_cmd_buf_ppn;
    delete [] r_cmd_buf_flags;
}

////////////////////////////////////
tmpl(void)::print_trace(size_t mode)
////////////////////////////////////
{
    const char *cmd_fsm_str[] = 
    {
        "CMD_IDLE",
        "CMD_SEND",
        "CMD_FAIL_WAIT",
        "CMD_FAIL_LAST",
        "CMD_MISS_CHECK_BUFFER",
        "CMD_MISS_CHECK_BYPASS",
        "CMD_MISS_READ_PTD",
        "CMD_MISS_WAIT_PTD",
        "CMD_MISS_READ_PTE",
        "CMD_MISS_WAIT_PTE",
        "CMD_MISS_TLB_UPDT",
    };
    const char *rsp_fsm_str[] = 
    {
        "RSP_IDLE",
        "RSP_SEND",
        "RSP_ERROR",
    };
    const char *config_fsm_str[] = 
    {
        "CONFIG_IDLE",
        "CONFIG_PTPR_WRITE",
        "CONFIG_MODE_WRITE",
        "CONFIG_PTPR_READ",
        "CONFIG_MODE_READ",
        "CONFIG_BVAR_READ",
        "CONFIG_XCODE_READ",
        "CONFIG_INVAL_PTE",
        "CONFIG_ERROR",
    };

    std::cout << std::dec << "NOC_MMU " << name() 
            t << "   " << cmd_fsm_str[r_cmd_fsm.read()]
              << " | " << rsp_fsm_str[r_rsp_fsm.read()]
              << " | " << config_fsm_str[r_config_fsm.read()] << std::endl;

    if( mode )
    {
        r_tlb.printTrace();
    }
}

////////////////////////
tmpl(void)::print_stats()
////////////////////////
{
    std::cout << name() << std::endl
    << "- TLB MISS RATE = " << (float)m_cpt_tlb_miss/m_cpt_tlb_read << std::endl
    << "- TLB MISS COST = " << (float)m_cost_tlb_miss/m_cpt_tlb_miss << std::endl;
}

////////////////////////
tmpl(void)::clear_stats()
////////////////////////
{
    m_cpt_tlb_read       = 0;             
    m_cpt_tlb_miss       = 0;             
    m_cost_tlb_miss      = 0;
}

/////////////////////////
tmpl(void)::transition()
/////////////////////////
{
    if ( not p_resetn.read() ) 
    {
        // FSMs
        r_cmd_fsm		 = CMD_IDLE;
        r_rsp_fsm		 = RSP_IDLE;
        r_config_fsm	 = CONFIG_IDLE;

        // component mode
		r_mode = MODE_BLOCKED;

        // output CMD FIFOs
        r_cmd_fifo_address.init();
        r_cmd_fifo_srcid.init();
        r_cmd_fifo_trdid.init();
        r_cmd_fifo_pktid.init();
        r_cmd_fifo_be.init();
        r_cmd_fifo_cmd.init();
        r_cmd_fifo_contig.init();
        r_cmd_fifo_wdata.init();
        r_cmd_fifo_eop.init();
        r_cmd_fifo_cons.init();
        r_cmd_fifo_plen.init();
        r_cmd_fifo_wrap.init();
        
        // output RSP FIFOs
        r_rsp_fifo_rsrcid.init();
        r_rsp_fifo_rtrdid.init();
        r_rsp_fifo_rpktid.init();
        r_rsp_fifo_rdata.init();
        r_rsp_fifo_rerror.init();
        r_rsp_fifo_reop.init();
        
        // RSP to CMD FIFO
        r_rsp2cmd.init();

        // Communication from CMD to RSP
        r_cmd_error_req		   = false;

        // Communication from CONFIG to CMD
        r_config_inval_req     = false;

	    // activity counters
	    m_cpt_total_cycles     = 0;
        m_cpt_tlb_read         = 0;             
        m_cpt_tlb_miss         = 0;             
        m_cost_tlb_miss        = 0;
        
        return;
    }

    // default values for CMD FIFO
    bool          cmd_fifo_put = false;
    paddr_t       cmd_fifo_address;
    vci_srcid_t   cmd_fifo_srcid;
    vci_trdid_t   cmd_fifo_trdid;
    vci_pktid_t   cmd_fifo_pktid;
    vci_be_t      cmd_fifo_be;
    vci_cmd_t     cmd_fifo_cmd;
    vci_data_t    cmd_fifo_wdata;
    vci_plen_t    cmd_fifo_plen;
    vci_contig_t  cmd_fifo_contig;
    vci_cons_t    cmd_fifo_cons;
    vci_wrap_t    cmd_fifo_wrap;
    bool          cmd_fifo_eop;
    bool          cmd_fifo_get = false;

    // default values for RSP FIFO
    bool          rsp_fifo_put = false; 
    vci_data_t    rsp_fifo_rdata;
    vci_rerror_t  rsp_fifo_rerror;
    vci_srcid_t   rsp_fifo_rsrcid;
    vci_trdid_t   rsp_fifo_rtrdid;
    vci_pktid_t   rsp_fifo_rpktid;
    bool          rsp_fifo_reop;
    bool          rsp_fifo_get = false; 
    
    // default values for RSP2CMD FIFO
    bool          rsp2cmd_fifo_put = false;
    uint32_t      rsp2cmd_fifo_wdata;
    bool          rsp2cmd_fifo_get = false;

    m_cpt_total_cycles++;

    bool debug = (m_cpt_total_cycles > m_debug_start_cycle) and m_debug_ok;

    ////////////////////////////////////////////////////////////////////////////////////
    // The CMD FSM handles the commands from the VCI initiator.
    // It performs address translation and access rights checking.
    // It requests an error response to RSP FSM in case of access right violation,
    // or in case of illegal address (unmaped).
    // It handles the TLB miss, looking in the prefetch buffer first.
    // It directly access the page table to fill the prefetch buffer. 
    // The prefetch buffer contains all PTE2 contained in a cache line (m_words/2).
    // The prefetch buffer tag is the common part (not shifted) of the physical adress
    // for all PTE2 contained in the write buffer.
    // Finally the CMD FSM handles PTE invalidation requests from CONFIG FSM,
    // invalidating both the TLB entry (if hit) and the prefetch buffer (if hit).
    // The cmdack signal is asserted on ly in the CMDIDLE and CMD_ERROR states.
    ////////////////////////////////////////////////////////////////////////////////////

    switch( r_cmd_fsm.read() ) 
    {
    //////////////
    case CMD_IDLE:  // waiting a VCI command 
    {
        if ( r_mode.read() == MODE_BLOCKED )   // does nothing if not activated
        {
#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_IDLE> NOC_MMU blocked" << std::endl;
}
#endif
            break;
        }

        if ( r_config_inval_req.read() ) // invalidation request from CONFIG FSM
                                         // both TLB and prefetch buffer are cleared
        {
            bool tlb_hit = r_tlb.inval( r_config_wdata.read() );
            bool buf_hit = ((r_config_wdata.read() & BUF_TAG_MASK) == r_cmd_buf_tag.read()); 
            if ( buf_hit ) r_cmd_buf_val = false;

            // reset request flip-flop
            r_config_inval_req = false;

#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_IDLE> PTE invalidation request :";
    if ( tlb_hit ) std::cout << " TLB cleared" 
    if ( buf_hit ) std::cout << " / prefetch buffer cleared"
    std::cout << std::endl;
}
#endif
        }
        else if ( p_vci_tgt.cmdval.read() ) // VCI command valid
        { 
            // We compute physical address and check access rights :
            // - If TLB not activated : the physical address is equal to the virtual 
            //   address (identity mapping) and there is no access rights checking
            // - If TLB activated : the physical address is obtained from the TLB, 
            //   and the access rights are defined by the W bit in the PTE.
            if ( r_mode.read() == MODE_IDENTITY )  // identity mapping 
            {
                r_cmd_paddr = p_vci_tgt.address.read();
                r_cmd_fsm   = CMD_SEND;
            }
            else                                    // TLB activated
            {
                vci_addr_t	paddr;
                pte_info_t  flags; 
                size_t      way;  
                size_t      set;
                paddr_t     nline;
                bool		hit;  

                r_cmd_trdid     = p_vci_tgt.trdid.read(); 
                r_cmd_pktid     = p_vci_tgt.pktid.read(); 
                r_cmd_srcid     = p_vci_tgt.srcid.read(); 

                hit = r_tlb.translate( p_vci_tgt.address.read(),
                                       &paddr,
                                       &flags,
                                       &nline,   // unused
                                       &way,	 // unused
                                       &set );   // unused
            
                if ( hit )	// TLB hit : access rights checking
                { 
                    if ( not flags.w and (p_vci_tgt.cmd.read() == vci_param_io::CMD_WRITE) ) 
                    {
                        //  a VCI response error must be returned by the RSP FSM
                        r_xcode   = WRITE_ACCES_VIOLATION;  
                        r_bvar    = p_vci_tgt.address.read();
                        if ( p_vci_tgt.eop.read() ) r_cmd_fsm = CMD_FAIL_LAST;
                        else                        r_cmd_fsm = CMD_FAIL_WAIT;
                        r_cmd_error_req = true;
#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_IDLE> TLB HIT, but writable violation" << std::endl;
}
#endif
                    }
                    else  // TLB HIT and no access rights violation
                    {
#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_IDLE> TLB HIT, access rights OK" << std::endl;
}
#endif
                        r_cmd_paddr = paddr;
                        r_cmd_fsm   = CMD_SEND;
                    }
                }
                else      // TLB miss: we enter TLB_MISS treatment sub-fsm
                {
                    r_cmd_fsm = CMD_MISS;
#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_IDLE> TLB MISS" << std::endl;
}
#endif
                }
            } // end if tlb activated
        } // end if cmdval
        break;
    }
    //////////////
    case CMD_SEND:	// write the VCI command in r_cmd-fifos
    {
        if ( p_vci_tgt.cmdval && r_cmd_fifo_address.wok() ) 
        {
            cmd_fifo_put     = true;
            cmd_fifo_address = r_cmd_paddr.read();
            cmd_fifo_srcid   = p_vci_tgt.srcid.read();
            cmd_fifo_trdid   = p_vci_tgt.trdid.read();
            cmd_fifo_pktid   = p_vci_tgt.pktid.read();
            cmd_fifo_be      = p_vci_tgt.be.read();
            cmd_fifo_cmd     = p_vci_tgt.cmd.read();
            cmd_fifo_wdata   = p_vci_tgt.wdata.read();
            cmd_fifo_plen    = p_vci_tgt.plen.read();
            cmd_fifo_contig  = p_vci_tgt.contig.read();
            cmd_fifo_cons    = p_vci_tgt.cons.read();
            cmd_fifo_wrap    = p_vci_tgt.wrap.read();
            cmd_fifo_eop     = p_vci_tgt.eop.read();

            if(  p_vci_tgt.eop )    r_cmd_fsm = CMD_IDLE;
           
#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_SEND> Push VCI command into cmd_fifo :" 
              << " address = " << std::hex << cmd_fifo_address
              << " srcid = "   << std::hex << cmd_fifo_srcid
              << " trdid = "   << std::hex << cmd_fifo_trdid
              << " pktid = "   << std::hex << cmd_fifo_pktid
              << " wdata = "   << std::hex << cmd_fifo_wdata
              << " be = "      << std::hex << cmd_fifo_be
              << " cmd = "     << std::dec << cmd_fifo_cmd 
              << " plen = "    << std::dec << cmd_fifo_plen
              << " eop = "     << std::dec << cmd_fifo_eop << std::endl; 
}
#endif
        }
        break;
    }
    ///////////////////
    case CMD_FAIL_WAIT:     // waiting  last flit of failing VCI command
    {
        if ( p_vci_tgt.cmdval.read() and p_vci_tgt.eop.read() )
        {
            r_cmd_fsm = CMD_FAIL_LAST;
        }
        break;
    }
    ///////////////////
    case CMD_FAIL_LAST:     // returns to IDLE state when the response
                            // has been handled by the RSP FSM
    {
        if( r_cmd_error_req.read() )
        { 
            r_cmd_fsm = CMD_IDLE;
        }
        break;
    }
    ////////////////////
    case CMD_MISS_CHECK:   // handle the TLB miss: the first step is to select
                           // a victim slot in TLB, and check if the missing
                           // PTE2 is in prefetch buffer
    {
        uint32_t vaddr = p_vci_tgt.address.read();
        uint32_t tag   = vaddr & m_tag_mask;
        size_t   way;
        size_t   set;
        bool     buf_hit = (tag == r_cmd_buf_tag.read()) and r_cmd_buf_val.read();

        // select a slot in TLB
        r_tlb.select( vaddr,
                      false,  // its a PTE2
                      &way,
                      &set )

        r_cmd_tlb_way = way;
        r_cmd_tlb_set = set;

        if ( buf_hit ) r_cmd_fsm = CMD_MISS_TLB_UPDT;
        else           r_cmd_fsm = CMD_MISS_GET_PTD;

#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_MISS_CHECK> ";
    if (hit) std::cout << "Hit in prefetch buffer" << std::endl;
    else     std::cout << "Miss in prefetch buffer" << std::endl;
}
#endif
        break;
    }
    ///////////////////////
    case CMD_MISS_READ_PTD:  // post a PTD read request in r_cmd_fifo
    {
        if ( r_cmd_fifo_address.wok() ) 
        {
            uint32_t vaddr   = p_vci_tgt.address.read();
            uint32_t ix1     = vaddr >> 21;

            cmd_fifo_put     = true;
            cmd_fifo_address = (r_ptpr.read() << 13) | (ix1 << 2);
            cmd_fifo_srcid   = m_srcid;
            cmd_fifo_trdid   = 0;
            cmd_fifo_pktid   = 0;
            cmd_fifo_be      = 0xF;
            cmd_fifo_cmd     = vci_param::CMD_READ;
            cmd_fifo_wdata   = 0;
            cmd_fifo_plen    = 4;
            cmd_fifo_contig  = true;
            cmd_fifo_cons    = false;
            cmd_fifo_wrap    = false;
            cmd_fifo_eop     = true;

            r_cmd_fsm = CMD_IDLE;

#if DEBUG_CMD_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CMD_MISS_READ_PTD> Push PTD read into cmd_fifo :" 
              << " address = " << std::hex << cmd_fifo_address
              << " srcid = "   << std::hex << cmd_fifo_srcid
              << " trdid = "   << std::hex << cmd_fifo_trdid
              << " pktid = "   << std::hex << cmd_fifo_pktid
              << " wdata = "   << std::hex << cmd_fifo_wdata
              << " be = "      << std::hex << cmd_fifo_be
              << " cmd = "     << std::dec << cmd_fifo_cmd 
              << " plen = "    << std::dec << cmd_fifo_plen
              << " eop = "     << std::dec << cmd_fifo_eop << std::endl; 
}
#endif

        }
    }
    ///////////////////////
    case CMD_MISS_WAIT_PTD:  // wait the PTD value 
    {
        if ( r_rsp_error.read() )   // bus error
        {
            r_xcode    =  
            r_bvar     = 
            r_cmd_fsm  = CMD_ERROR;
        }
        else if ( r_rsp2cmd_fifo.rok() )  
        {
            if ( not ( pte_flags & PTE_V_MASK) )	// unmapped
            {
                std::cout << "NOC_MMU ERROR : " << name() << std::hex
                          << " PT1 entry unmapped for vaddr = " << p_vci_ini.address.read()
                          << std::endl;
                       
                r_xcode   = NOC_MMU_PT1_ENTRY_UNMAPPED;
                r_bvar    = p_vci_ini.address.read();
                r_cmd_fsm = CMD_ERROR;
            }
            else if ( TBD )                          // big page PTD
            {
                std::cout << "NOC_MMU ERROR : " << name() << std::hex
                          << " unexpected PTD for vaddr " << p_vci_ini.address.read()
                          << std::endl;
                       
                r_xcode   = NOC_MMU_UNSUPPORTED_BIG_PAGE;
                r_bvar    = p_vci_ini.address.read();
                r_cmd_fsm = CMD_ERROR;
            }
            else                                   // PTD
            {
                r_cmd_ptd = r_rsp2cmd_fifo.read();
                r_cmd_fsm  = CMD_MISS_READ_PTE;
            }
        }
        break;
    }
    ///////////////////////
    case CMD_MISS_READ_PTE:  // post a PTE read burst request in r_cmd_fifo
    {
        if ( r_cmd_fifo_address.wok() ) 
        {
            uint32_t vaddr   = p_vci_tgt.address.read();
            uint32_t ix2     = (vaddr >> I1_SHIFT) & I1_MASK;

            cmd_fifo_put     = true;
            cmd_fifo_address = r_ptpr.read() | (ix2 << 2);
            cmd_fifo_srcid   = m_srcid;
            cmd_fifo_trdid   = 0;
            cmd_fifo_pktid   = 0;
            cmd_fifo_be      = 0xF;
            cmd_fifo_cmd     = vci_param::CMD_READ;
            cmd_fifo_wdata   = 0;
            cmd_fifo_plen    = (m_words << 2);
            cmd_fifo_contig  = true;
            cmd_fifo_cons    = false;
            cmd_fifo_wrap    = false;
            cmd_fifo_eop     = true;

            r_cmd_fsm        = CMD_MISS_WAIT_PTE;
            r_cmd_count      = 0;
        }
        break;
    }
    ////////////////////////
    case CMD_MISS_WAIT_PTE:    // wait the PTE values
    {
        if ( r_rsp_error.read() )
        {
            r_xcode    =  
            r_bvar     = 
            r_cmd_fsm  = CMD_ERROR;
        }
        else if ( r_rsp2cmd_fifo.rok() )
        {
            bool    ppn  = r_cmd_count.read() & 0x1;
            size_t index = r_cmd_count.read() >> 1;

            if ( ppn ) r_cmd_buf_ppn[index]   = r_rsp2cmd_fifo.read();
            else       r_cmd_buf_flags[index] = r_rsp2cmd_fifo.read();

            r_cmd_count = r_cmd_count.read() + 1;

            if ( r_cmd_count.read() == m_words ) r_cmd_fsm = CMD_MISS_TLB_UPDT;
        }
        break;
    }
    ///////////////////////
    case CMD_MISS_TLB_UPDT:  // write a new entry in TLB
    {
        uint32_t vaddr = p_vci_ini.address.read();
        uint32_t index = (vaddr >> INDEX_SHIFT) & INDEX_MASK;

        r_tlb.write( false,     // it's not a big page
                     r_cmd_buf_flags[index].read(),
                     r_cmd_buf_ppn[index].read(),
                     vaddr,
                     r_cmd_tlb_way.read(),
                     r_cmd_tlb_set.read(),
                     0 );       // nline is not used in this component
#if DEBUG_CMD_FSM
if( debug ) 
{
    std::cout << "  <NOC_MMU.CMD_MISS_TLB_UPDT> Write a new PTE2 in TLB:"
}
        break;
    }
    } // end switch CMD_FSM

    /////////////////////////////////////////////////////////////////////
    // The RSP FSM handles both VCI responses from the NOC, and
    // requests from the CMD FSM in case of write access violation.
    /////////////////////////////////////////////////////////////////////
    switch( r_rsp_fsm.read() ) 
    {
    //////////////
    case RSP_IDLE:  // waiting responses from NOC or requests from CMD FSM
    {    	
        if( r_cmd_error_req.read() ) 
        {
            r_dma_rsp_fsm = RSP_ERROR;
        }
        else if( p_vci_ini.rspval.read() ) 
		{
			r_dma_rsp_fsm = RSP_SEND;
		}
		break;
    }
    //////////////
    case RSP_SEND:  // push the NOC VCI response into the RSP FIFO
    {
        if( p_vci_ini.rspval.read() && r_rsp_fifo_rdata.wok() )
    	{
            rsp_fifo_put    = true;
            rsp_fifo_rdata  = p_vci_ini.rdata.read();
            rsp_fifo_rerror = p_vci_ini.rerror.read();
            rsp_fifo_rsrcid = p_vci_ini.rpktid.read();
            rsp_fifo_rtrdid = p_vci_ini.rpktid.read();
            rsp_fifo_rpktid = p_vci_ini.rpktid.read();
            rsp_fifo_reop   = p_vci_ini.reop.read();

            if( p_vci_ini.reop.read() ) r_rsp_fsm = RSP_IDLE;	

#if DEBUG_RSP_FSM
if( debug ) 
{
    std::cout << "  <NOC_MMU.RSP_FIFO_PUT> Push into rsp_fifo:" 
              << " rsrcid = " << std::hex << p_vci_ini.rsrcid.read()
              << " rtrdid = " << std::hex << p_vci_ini.rtrdid.read()
              << " rpktid = " << std::hex << p_vci_ini.rpktid.read()
              << " rerror = " << std::hex << p_vci_ini.rerror.read()
              << " rdata = " << std::hex << p_vci_ini.rdata.read() << std::endl;
}
#endif
    	}
    	break;
    }
    ///////////////
    case RSP_ERROR:  // push a single flit error response into the RSP FIFO
                     // when the last flit of illegal command is arrived
    {
        if( r_rsp_fifo_rdata.wok() )
    	{
            rsp_fifo_put    = true;
            rsp_fifo_rdata  = 0;
            rsp_fifo_rerror = 0x1;
            rsp_fifo_rsrcid = r_cmd_srcid.read();
            rsp_fifo_rtrdid = r_cmd_trdid.read();
            rsp_fifo_rpktid = r_cmd_pktid.read();
            rsp_fifo_reop   = true;

            // reset request flip_flop
            r_cmd_error_req = false;
            r_rsp_fsm       = RSP_IDLE;	

#if DEBUG_RSP_FSM
if( debug ) 
{
    std::cout << "  <NOC_MMU.RSP_ERROR> Push into rsp_fifo:" 
              << " rsrcid = " << std::hex << r_cmd_srcid.read()
              << " rtrdid = " << std::hex << r_cmd_trdid.read()
              << " rpktid = " << std::hex << r_cmd_pktid.read()
              << " rerror = 0x1"
              << " rdata = 0x0" << std::endl;
}
#endif
    	}
    	break;
    }
    } // end switch RSP_FSM 

    
    /////////////////////////////////////////////////////////////////////
    // The CONFIG FSM handles the configuration requests
    // All configuration access (read or write) must be one single flit.
    /////////////////////////////////////////////////////////////////////

    switch( r_config_fsm.read() ) 
    {
    /////////////////
    case CONFIG_IDLE:
    {
        if ( p_vci_tgt_config.cmdval.read() ) 
        {
#if DEBUG_CONFIG_FSM
if( debug )
{
    std::cout << "  <NOC_MMU.CONFIG_IDLE> Configuration command received!" <<std::endl;
    std::cout << " address = " << std::hex << p_vci_tgt_config.address.read()
              << " srcid = "   << std::hex << p_vci_tgt_config.srcid.read()
              << " trdid = "   << std::hex << p_vci_tgt_config.trdid.read()
              << " pktid = "   << std::hex << p_vci_tgt_config.pktid.read()
              << " wdata = "   << std::hex << p_vci_tgt_config.wdata.read()
              << " cmd = "     << std::hex << p_vci_tgt_config.cmd.read()
              << " plen = "    << std::dec << p_vci_tgt_config.plen.read() << std::endl;
}
#endif
            vci_paddr_t address = p_vci_tgt_config.address.read();
            bool        read  = (p_vci_tgt_config.cmd.read() == vci_param_d::CMD_READ);
            uint32_t    cell  = (uint32_t)((address & 0xFFF)>>2); 
            
            if( m_segment.contains(address) iand p_vci_tgt_config.eop.read() )
            {
                r_config_wdata = p_vci_tgt_config.wdata.read();
                r_config_srcid = p_vci_tgt_config.srcid.read();
                r_config_trdid = p_vci_tgt_config.trdid.read();
                r_config_pktid = p_vci_tgt_config.pktid.read();

                if     (not read and (cell == NOC_MMU_PTPR))  r_config_fsm = CONFIG_PTPR_WRITE;
                else if(not read and (cell == NOC_MMU_MODE))  r_config_fsm = CONFIG_MODE_WRITE;
                else if(read     and (cell == NOC_MMU_PTPR))  r_config_fsm = CONFIG_PTPR_READ;
                else if(read     and (cell == NOC_MMU_MODE))  r_config_fsm = CONFIG_MODE_READ;
                else if(read     and (cell == NOC_MMU_BVAR))  r_config_fsm = CONFIG_BVAR_READ;
                else if(read     and (cell == NOC_MMU_ERROR)) r_config_fsm = CONFIG_ERROR_READ;
                else if(not read and (cell == NOC_MMU_INVAL))   
                {
                    r_config_inval_req = true;
                    r_config_fsm       = CONFIG_INVAL_PTE;
                }
                else     //Error. Wrong address, or invalid operation.
                {
                    r_config_fsm = CONFIG_ERROR; 
                }
            }
            else
            {
                r_config_fsm = CONFIG_ERROR;
            }
        }
        break;
    }
    ///////////////////////
    case CONFIG_PTPR_WRITE:  // The word received is in the format:
                             // 00000 BASE_ADDRESS[39:13]
                             // does nothing if the response cannot be returned
    {
        if ( p_vci_tgt_config.rspack.read() )
        {
            r_ptpr       = (uint32_t)(p_vci_tgt_config.wdata.read()); 
            r_config_fsm = CONFIG_IDLE;

#if DEBUG_CONFIG_FSM
if( debug ) 
{
    std::cout << "  <NOC_MMU.CONFIG_PTPR_WRITE> PTPR = " 
              <<  std::hex << (p_vci_tgt_config.wdata.read()) <<std::endl;
}
#endif
        }
        break;
    }
    ///////////////////////
    case CONFIG_MODE_WRITE: // TLB active if wdata > 1
                            // does nothing if the response cannot be returned
    {
        if ( p_vci_tgt_config.rspack.read() )
        {
            uint32_t mode = (uint32_t)(p_vci_tgt_config.wdata.read());

            if     ( mode == MODE_BLOCKED      ) r_mode = MODE_BLOCKED;
            else if( mode == MODE_IDENTITY_MAP ) r_mode = MODE_IDENTITY_MAP;
            else                                 r_mode = MODE_TLB_ACTIVE;
            r_config_fsm  = CONFIG_IDLE;

#if DEBUG_CONFIG_FSM
if( debug ) 
{
    std::cout << "  <NOC_MMU.CONFIG_MODE_WRITE> MODE = ";
    if ( mode == MODE_BLOCKED )           std::cout << "BLOCKED"      << std::endl;
    else if ( mode == MODE_IDENTITY_MAP ) std::cout << "IDENTITY_MAP" << std::endl;
    else                                  std::cout << "TLB_ACTIVE"   << std::endl;
}
#endif
        }
        break;
    }
    //////////////////////
    case CONFIG_INVAL_PTE:  // waiting invalidation acknowledge by CMD FSM
                            // does nothing if the response cannot be returned
    {   
        if ( p_vci_tgt_config.rspack.read() and not r_config_inval_req.read() )
        {
            r_config_cmd_fsm = CONFIG_IDLE;
        } 
        break;
    }
    //////////////////////
    case CONFIG_PTPR_READ: 
    case CONFIG_MODE_READ:
    case CONFIG_BVAR_READ:
    case CONFIG_XCODE_READ:
    {
        if ( p_vci_tgt_config.rspack.read() )
        {
            r_config_fsm = CONFIG_CMD_IDLE;

#if DEBUG_CONFIG_FSM
if( debug ) 
{
    std::cout << "  <NOC_MMU.CONFIG_READ>";
    if ( r_config_fsm.read == CONFIG_PTPR_READ )
    std::cout << " PTPR = " << std::hex << r_ptpr.read() << std::endl;
    if ( r_config_fsm.read == CONFIG_MODE_READ )
    std::cout << " MODE = " << std::hex << r_mode.read() << std::endl;
    if ( r_config_fsm.read == CONFIG_BVAR_READ )
    std::cout << " BVAR = " << std::hex << r_bvar.read() << std::endl;
    if ( r_config_fsm.read == CONFIG_XCODE_READ )
    std::cout << " XCODE = " << std::hex << r_xcode.read() << std::endl;
}
#endif
        } 
        break;
    }
    //////////////////////
    case CONFIG_CMD_ERROR:  // The error response should be send only when the
                            // last command flit has been received (eop) 
    {
        if ( p_vci_tgt_config.rspack.read() and p_vci_tgt_config.eop.read())
        {
            r_config_fsm = CONFIG_IDLE;

#if DEBUG_CONFIG_FSM
if( debug ) 
{
    std::cout << "  <NOC_MMU.CONFIG_READ_ERROR>" << std::endl;
}
#endif
        }
        break;
    }

    } // end switch CONFIG FSM

    // CMD FIFOs update
    r_cmd_fifo_address.update( cmd_fifo_get, cmd_fifo_put, cmd_fifo_address );
    r_cmd_fifo_cmd.update(     cmd_fifo_get, cmd_fifo_put, cmd_fifo_cmd );
    r_cmd_fifo_contig.update(  cmd_fifo_get, cmd_fifo_put, cmd_fifo_contig );
    r_cmd_fifo_cons.update(    cmd_fifo_get, cmd_fifo_put, cmd_fifo_cons );
    r_cmd_fifo_plen.update(    cmd_fifo_get, cmd_fifo_put, cmd_fifo_plen );
    r_cmd_fifo_wrap.update(    cmd_fifo_get, cmd_fifo_put, cmd_fifo_wrap );
    r_cmd_fifo_srcid.update(   cmd_fifo_get, cmd_fifo_put, cmd_fifo_srcid );
    r_cmd_fifo_trdid.update(   cmd_fifo_get, cmd_fifo_put, cmd_fifo_trdid );
    r_cmd_fifo_pktid.update(   cmd_fifo_get, cmd_fifo_put, cmd_fifo_pktid );
    r_cmd_fifo_wdata.update(   cmd_fifo_get, cmd_fifo_put, cmd_fifo_wdata );
    r_cmd_fifo_be.update(      cmd_fifo_get, cmd_fifo_put, cmd_fifo_be );
    r_cmd_fifo_eop.update(     cmd_fifo_get, cmd_fifo_put, cmd_fifo_eop );


    // RSP FIFOs update
    r_rsp_fifo_rdata.update(   rsp_fifo_get, rsp_fifo_put, rsp_fifo_rdata );
    r_rsp_fifo_rsrcid.update(  rsp_fifo_get, rsp_fifo_put, rsp_fifo_rsrcid ); 
    r_rsp_fifo_rtrdid.update(  rsp_fifo_get, rsp_fifo_put, rsp_fifo_trdid ); 
    r_rsp_fifo_rpktid.update(  rsp_fifo_get, rsp_fifo_put, rsp_fifo_pktid );
    r_rsp_fifo_rerror.update(  rsp_fifo_get, rsp_fifo_put, rsp_fifo_rerror );
    r_rsp_fifo_reop.update(    rsp_fifo_get, rsp_fifo_put, rsp_fifo_reop );
    
    // RSP2CMD FIFO update
    r_rsp2cmd_fifo.update(     rsp2cmd_fifo_get, rsp2cmd_fifo_put, rsp2cmd_fifo_wdata );

} // end transition()

///////////////////////
tmpl(void)::genMoore()
///////////////////////
{
    // VCI initiator port to the NOC
     
    p_vci_ini.cmdval  = r_cmd_fifo_address.rok(); 

    p_vci_ini.address = r_cmd_fifo_address.read();
    p_vci_ini.be      = r_cmd_fifo_be.read();
    p_vci_ini.cmd     = r_cmd_fifo_cmd.read();
    p_vci_ini.wdata   = r_cmd_fifo_wdata.read();
    p_vci_ini.srcid   = r_cmd_fifo_srcid.read();
    p_vci_ini.trdid   = r_cmd_fifo_trdid.read();
    p_vci_ini.pktid   = r_cmd_fifo_pktid.read();
    p_vci_ini.plen    = r_cmd_fifo_plen.read();
    p_vci_ini.contig  = r_cmd_fifo_contig.read();
    p_vci_ini.cons    = r_cmd_fifo_cons.read();
    p_vci_ini.wrap    = r_cmd_fifo_wrap.read();
    p_vci_ini.cfixed  = false;
    p_vci_ini.clen    = 0;
    p_vci_ini.eop     = r_cmd_fifo_eop.read();
    
    p_vci_ini.rspack  = (r_rsp_fsm.read() == RSP_SEND) and r_rsp_fifo_wdata.wok();

    // VCI target port to VCI initiator

    p_vci_tgt.rspval  = r_rsp_fifo_rdata.rok();
    p_vci_tgt.rdata   = r_rsp_fifo_rdata.read();
    p_vci_tgt.rerror  = r_rsp_fifo_rerror.read();
    p_vci_tgt.rsrcid  = r_rsp_fifo_rsrcid.read();
    p_vci_tgt.rtrdid  = r_rsp_fifo_rtrdid.read();
    p_vci_tgt.rpktid  = r_rsp_fifo_rpktid.read();
    p_vci_tgt.reop    = r_rsp_fifo_reop.read();

    p_vci_tgt.cmdack =  (r_cmd_fsm.read() == CMD_SEND) or 
                        (r_cmd_fsm.read() == CMD_FAIL_WAIT) or
                        (r_cmd_fsm.read() == CMD_FAIL_LAST);
    
    // VCI target port to the NOC (config)

    p_vci_tgt_config.rspval = (r_config_fsm.read() != CONFIG_IDLE);

    if      (r_config_fsm.read() == CONFIG_PTPR_READ)  p_vci_tgt_config.rdata = r_ptpr.read();
    else if (r_config_fsm.read() == CONFIG_MODE_READ)  p_vci_tgt_config.rdata = r_mode.read();
    else if (r_config_fsm.read() == CONFIG_BVAR_READ)  p_vci_tgt_config.rdata = r_bvar.read();
    else if (r_config_fsm.read() == CONFIG_XCODE_READ) p_vci_tgt_config.rdata = r_xcode.read();
    else                                               p_vci_tgt_config.rdata = 0;
    p_vci_tgt_config.rsrcid = r_config_rsrcid.read();
    p_vci_tgt_config.rtrdid = r_config_rtrdid.read();
    p_vci_tgt_config.rpktid = r_config_rpktid.read();
    if (r_config_fsm.read() == CONFIG_ERROR ) p_vci_tgt_config.rerror =
    else                                      p_vci_tgt_config.rerror =

} // end genMoore

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
