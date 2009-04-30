/* -*- c++ -*-
 * File : vci_vcache_wrapper.cpp
 * Copyright (c) UPMC, Lip6, SoC
 * Authors : Alain GREINER, Yang GAO
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
#include "../include/vci_vcache_wrapper.h"

namespace soclib { 
namespace caba {

//#define VCACHE_WRAPPER_DEBUG

#ifdef VCACHE_WRAPPER_DEBUG
namespace {
const char *icache_fsm_state_str[] = {
        "ICACHE_IDLE",
        "ICACHE_BIS",       
        "ICACHE_TLB1_READ",  
        "ICACHE_TLB1_WRITE",  
        "ICACHE_TLB1_UPDT",  
        "ICACHE_TLB2_READ",  
        "ICACHE_TLB2_WRITE",  
        "ICACHE_TLB2_UPDT",  
        "ICACHE_TLB_FLUSH", 
        "ICACHE_CACHE_FLUSH", 
        "ICACHE_TLB_INVAL",  
        "ICACHE_TLB_INVAL_DONE",  
        "ICACHE_CACHE_INVAL",
        "ICACHE_CACHE_INVAL_DONE",
        "ICACHE_MISS_WAIT",
        "ICACHE_UNC_WAIT",  
        "ICACHE_MISS_UPDT",  
        "ICACHE_ERROR", 	
    };
const char *dcache_fsm_state_str[] = {
        "DCACHE_IDLE",       
        "DCACHE_BIS",       
        "DCACHE_TLB1_READ",  
        "DCACHE_TLB1_WRITE", 
        "DCACHE_TLB1_UPDT",  
        "DCACHE_TLB2_READ",  
        "DCACHE_TLB2_WRITE", 
        "DCACHE_TLB2_UPDT",   
        "DCACHE_CTXT_SWITCH",   
        "DCACHE_ICACHE_FLUSH", 
        "DCACHE_DCACHE_FLUSH", 
        "DCACHE_ITLB_INVAL",
        "DCACHE_DTLB_INVAL",
        "DCACHE_DTLB_INVAL_DONE",
        "DCACHE_ICACHE_INVAL",
        "DCACHE_DCACHE_INVAL",
        "DCACHE_DCACHE_INVAL_DONE",
        "DCACHE_WRITE_UPDT", 
        "DCACHE_WRITE_DIRTY",
        "DCACHE_WRITE_REQ",  
        "DCACHE_MISS_WAIT",  
        "DCACHE_MISS_UPDT",  
        "DCACHE_UNC_WAIT",   
        "DCACHE_ERROR",  
    };
const char *cmd_fsm_state_str[] = {
        "CMD_IDLE",           
        "CMD_ITLB_READ",      
        "CMD_ITLB_WRITE",       
        "CMD_INS_MISS",     
        "CMD_INS_UNC",     
        "CMD_DTLB_READ",    
        "CMD_DTLB_WRITE",       
        "CMD_DTLB_DIRTY",       
        "CMD_DATA_UNC",     
        "CMD_DATA_MISS",    
        "CMD_DATA_WRITE",    
    };
const char *rsp_fsm_state_str[] = {
        "RSP_IDLE",                  
        "RSP_ITLB_READ",             
        "RSP_ITLB_WRITE",               
        "RSP_INS_MISS",   
        "RSP_INS_UNC",           
        "RSP_DTLB_READ",            
        "RSP_DTLB_WRITE",             
        "RSP_DTLB_DIRTY",             
        "RSP_DATA_MISS",             
        "RSP_DATA_UNC",              
        "RSP_DATA_WRITE",            
    };	
}
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciVCacheWrapper<vci_param, iss_t>

tmpl(inline typename VciVCacheWrapper<vci_param, iss_t>::data_t)::be_to_mask( typename iss_t::be_t be )
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

/***********************************************/
tmpl(/**/)::VciVCacheWrapper(
    sc_module_name name,
    int proc_id,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &initiator_index,
    size_t itlb_m_ways,
    size_t itlb_m_sets,
    size_t itlb_k_ways,
    size_t itlb_k_sets,
    size_t dtlb_m_ways,
    size_t dtlb_m_sets,
    size_t dtlb_k_ways,
    size_t dtlb_k_sets,
    size_t icache_ways,
    size_t icache_sets,
    size_t icache_words,
    size_t dcache_ways,
    size_t dcache_sets,
    size_t dcache_words )
/***********************************************/
    : soclib::caba::BaseModule(name),

      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci"),

      m_cacheability_table(mt.getCacheabilityTable()),
      m_srcid(mt.indexForId(initiator_index)),
      m_iss(this->name(), proc_id),

      m_itlb_m_ways(itlb_m_ways),
      m_itlb_m_sets(itlb_m_sets),
      m_itlb_k_ways(itlb_k_ways),
      m_itlb_k_sets(itlb_k_sets),

      m_dtlb_m_ways(dtlb_m_ways),
      m_dtlb_m_sets(dtlb_m_sets),
      m_dtlb_k_ways(dtlb_k_ways),
      m_dtlb_k_sets(dtlb_k_sets),

      m_icache_ways(icache_ways),
      m_icache_yzmask((~0)<<(uint32_log2(icache_words) + 2)),
      m_icache_words(icache_words),

      m_dcache_ways(dcache_ways),
      m_dcache_yzmask((~0)<<(uint32_log2(dcache_words) + 2)),
      m_dcache_words(dcache_words),

      icache_m_tlb(itlb_m_ways,itlb_m_sets,PAGE_M_NBITS),
      icache_k_tlb(itlb_k_ways,itlb_k_sets,PAGE_K_NBITS),
      dcache_m_tlb(dtlb_m_ways,dtlb_m_sets,PAGE_M_NBITS),
      dcache_k_tlb(dtlb_k_ways,dtlb_k_sets,PAGE_K_NBITS),

      r_dcache_fsm("r_dcache_fsm"),
      r_dcache_paddr_save("r_dcache_paddr_save"),
      r_dcache_wdata_save("r_dcache_wdata_save"),
      r_dcache_rdata_save("r_dcache_rdata_save"),
      r_dcache_type_save("r_dcache_type_save"),
      r_dcache_be_save("r_dcache_be_save"),
      r_dcache_cached_save("r_dcache_cached_save"),
      r_dcache_tlb_paddr("r_dcache_tlb_paddr"),
      r_dcache_miss_req("r_dcache_miss_req"),
      r_dcache_unc_req("r_dcache_unc_req"),
      r_dcache_write_req("r_dcache_write_req"),
      r_dcache_tlb_read_req("r_dcache_tlb_read_req"),
      r_dcache_tlb_et_req("r_dcache_tlb_et_req"),
      r_dcache_tlb_dirty_req("r_dcache_tlb_dirty_req"),
      r_dcache_tlb_ptba_read("r_dcache_tlb_ptba_read"),
      r_dcache_xtn_req("r_dcache_xtn_req"),

      r_icache_fsm("r_icache_fsm"),
      r_icache_paddr_save("r_icache_paddr_save"),
      r_icache_miss_req("r_icache_miss_req"),
      r_icache_unc_req("r_icache_unc_req"),
      r_icache_tlb_read_req("r_icache_tlb_read_req"),
      r_icache_tlb_et_req("r_icache_tlb_et_req"),

      r_vci_cmd_fsm("r_vci_cmd_fsm"),
      r_vci_cmd_min("r_vci_cmd_min"),
      r_vci_cmd_max("r_vci_cmd_max"),
      r_vci_cmd_cpt("r_vci_cmd_cpt"),

      r_vci_rsp_fsm("r_vci_rsp_fsm"),
      r_vci_rsp_cpt("r_vci_rsp_cpt"),
      r_vci_rsp_itlb_miss("r_vci_rsp_itlb_miss"),
      r_vci_rsp_dtlb_miss("r_vci_rsp_dtlb_miss"),
      r_vci_rsp_ins_error("r_vci_rsp_ins_error"),
      r_vci_rsp_data_error("r_vci_rsp_data_error"),

      r_wbuf("wbuf", dcache_words ),
      r_icache("icache", icache_ways, icache_sets, icache_words),
      r_dcache("dcache", dcache_ways, dcache_sets, dcache_words)
{
    r_icache_miss_buf = new data_t[icache_words];
    r_dcache_miss_buf = new data_t[dcache_words];

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();
  
    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();

    m_iss.setICacheInfo( icache_words*sizeof(data_t), icache_ways, icache_sets );
    m_iss.setDCacheInfo( dcache_words*sizeof(data_t), dcache_ways, dcache_sets );
}

/////////////////////////////////////
tmpl(/**/)::~VciVCacheWrapper()
/////////////////////////////////////
{
    delete [] r_icache_miss_buf;
    delete [] r_dcache_miss_buf;
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
    std::cout << "CPU " << m_srcid << std::endl;
    std::cout << "- CPI                    = " << (float)m_cpt_total_cycles/run_cycles << std::endl ;
    std::cout << "- READ RATE              = " << (float)m_cpt_read/run_cycles << std::endl ;
    std::cout << "- WRITE RATE             = " << (float)m_cpt_write/run_cycles << std::endl;
    std::cout << "- UNCACHED READ RATE     = " << (float)m_cpt_unc_read/m_cpt_read << std::endl ;
    std::cout << "- CACHED WRITE RATE      = " << (float)m_cpt_write_cached/m_cpt_write << std::endl ;
    std::cout << "- IMISS_RATE             = " << (float)m_cpt_ins_miss/m_cpt_ins_read << std::endl;     
    std::cout << "- DMISS RATE             = " << (float)m_cpt_data_miss/(m_cpt_read-m_cpt_unc_read) << std::endl ;
    std::cout << "- INS MISS COST          = " << (float)m_cost_ins_miss_frz/m_cpt_ins_miss << std::endl;
    std::cout << "- IMISS TRANSACTION      = " << (float)m_cost_imiss_transaction/m_cpt_imiss_transaction << std::endl;
    std::cout << "- DMISS COST             = " << (float)m_cost_data_miss_frz/m_cpt_data_miss << std::endl;
    std::cout << "- DMISS TRANSACTION      = " << (float)m_cost_dmiss_transaction/m_cpt_dmiss_transaction << std::endl;
    std::cout << "- UNC COST               = " << (float)m_cost_unc_read_frz/m_cpt_unc_read << std::endl;
    std::cout << "- UNC TRANSACTION        = " << (float)m_cost_unc_transaction/m_cpt_unc_transaction << std::endl;
    std::cout << "- WRITE COST             = " << (float)m_cost_write_frz/m_cpt_write << std::endl;
    std::cout << "- WRITE TRANSACTION      = " << (float)m_cost_write_transaction/m_cpt_write_transaction << std::endl;
    std::cout << "- WRITE LENGTH           = " << (float)m_length_write_transaction/m_cpt_write_transaction << std::endl;
    std::cout << "- INS TLB MISS RATE      = " << (float)m_cpt_ins_tlb_miss/m_cpt_ins_tlb_read << std::endl;
    std::cout << "- DATA TLB MISS RATE     = " << (float)m_cpt_data_tlb_miss/m_cpt_data_tlb_read << std::endl;
    std::cout << "- ITLB MISS TRANSACTION  = " << (float)m_cost_itlbmiss_transaction/m_cpt_itlbmiss_transaction << std::endl;
    std::cout << "- ITLB WRITE TRANSACTION = " << (float)m_cost_itlb_write_transaction/m_cpt_itlb_write_transaction << std::endl;
    std::cout << "- ITLB MISS COST         = " << (float)m_cost_ins_tlb_miss_frz/(m_cpt_ins_tlb_miss+m_cpt_ins_tlb_write_et) << std::endl;
    std::cout << "- DTLB MISS TRANSACTION  = " << (float)m_cost_dtlbmiss_transaction/m_cpt_dtlbmiss_transaction << std::endl;
    std::cout << "- DTLB WRITE TRANSACTION = " << (float)m_cost_dtlb_write_transaction/m_cpt_dtlb_write_transaction << std::endl;
    std::cout << "- DTLB MISS COST         = " << (float)m_cost_data_tlb_miss_frz/(m_cpt_data_tlb_miss+m_cpt_data_tlb_write_et+m_cpt_data_tlb_write_dirty) << std::endl;
}

/*************************************************/
tmpl(void)::transition()
/*************************************************/
{
    if ( ! p_resetn.read() ) 
    {
        m_iss.reset();

        r_dcache_fsm = DCACHE_IDLE;
        r_icache_fsm = ICACHE_IDLE;
        r_vci_cmd_fsm = CMD_IDLE;
        r_vci_rsp_fsm = RSP_IDLE;

        // write buffer & caches
        r_wbuf.reset();
        r_icache.reset();
        r_dcache.reset();

        icache_m_tlb.reset();    
        icache_k_tlb.reset();    
        dcache_m_tlb.reset();    
        dcache_k_tlb.reset();   

        r_mmu_mode = TLBS_DEACTIVE;

        r_icache_miss_req        = false;
        r_icache_unc_req         = false;
        r_icache_tlb_read_req    = false;
        r_icache_tlb_et_req      = false;

        r_dcache_miss_req        = false;
        r_dcache_unc_req         = false;
        r_dcache_write_req       = false;
        r_dcache_tlb_read_req    = false;
        r_dcache_tlb_et_req      = false;
        r_dcache_tlb_dirty_req   = false;
        r_dcache_tlb_ptba_read   = false;
        r_dcache_xtn_req         = false;

        r_icache_page_k_save     = false;
        r_dcache_page_k_save     = false;
        r_dcache_dirty_save      = false;
        r_dcache_hit_p_save      = false;

        r_icache_buf_unc_valid   = false;
        r_dcache_buf_unc_valid   = false;

        r_vci_rsp_ins_error      = false;
        r_vci_rsp_data_error     = false;

        r_icache_id1_save        = 0;
        r_icache_ppn_save        = 0;
        r_icache_vpn_save        = 0;
        r_itlb_translation_valid = false;

        r_dcache_id1_save        = 0;
        r_dcache_ppn_save        = 0;
        r_dcache_vpn_save        = 0;
        r_dtlb_translation_valid = false;

        r_icache_ptba_ok         = false;
        r_dcache_ptba_ok         = false;

        r_icache_error_type      = MMU_NONE;
        r_dcache_error_type      = MMU_NONE;

        // activity counters
        m_cpt_dcache_data_read  = 0;
        m_cpt_dcache_data_write = 0;
        m_cpt_dcache_dir_read   = 0;
        m_cpt_dcache_dir_write  = 0;
        m_cpt_icache_data_read  = 0;
        m_cpt_icache_data_write = 0;
        m_cpt_icache_dir_read   = 0;
        m_cpt_icache_dir_write  = 0;

	    m_cpt_frz_cycles   = 0;
        m_cpt_total_cycles = 0;

        m_cpt_read         = 0;
        m_cpt_write        = 0;
        m_cpt_data_miss    = 0;
        m_cpt_ins_miss     = 0;
        m_cpt_unc_read     = 0;
        m_cpt_write_cached = 0;
        m_cpt_ins_read     = 0;  

        m_cost_write_frz     = 0;
        m_cost_data_miss_frz = 0;
        m_cost_unc_read_frz  = 0;
        m_cost_ins_miss_frz  = 0;

        m_cpt_imiss_transaction = 0;
        m_cpt_dmiss_transaction = 0;
        m_cpt_unc_transaction   = 0;
        m_cpt_write_transaction = 0;

        m_cost_imiss_transaction   = 0;
        m_cost_dmiss_transaction   = 0;
        m_cost_unc_transaction     = 0;
        m_cost_write_transaction   = 0;
        m_length_write_transaction = 0;

        m_cpt_ins_tlb_read         = 0;             
        m_cpt_ins_tlb_miss         = 0;             
        m_cpt_ins_tlb_write_et     = 0;         

        m_cpt_data_tlb_read        = 0;           
        m_cpt_data_tlb_miss        = 0;           
        m_cpt_data_tlb_write_et    = 0;       
        m_cpt_data_tlb_write_dirty = 0;    

        m_cost_ins_tlb_miss_frz    = 0;      
        m_cost_data_tlb_miss_frz   = 0;      

        m_cpt_itlbmiss_transaction   = 0;    
        m_cpt_itlb_write_transaction = 0;  
        m_cpt_dtlbmiss_transaction   = 0;  
        m_cpt_dtlb_write_transaction = 0;  
 
        m_cost_itlbmiss_transaction   = 0;   
        m_cost_itlb_write_transaction = 0;  
        m_cost_dtlbmiss_transaction   = 0;   
        m_cost_dtlb_write_transaction = 0;  
        return;
    }

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << "cycle = " << m_cpt_total_cycles << " processor " << name() 
        << " dcache fsm: " << dcache_fsm_state_str[r_dcache_fsm]
        << " icache fsm: " << icache_fsm_state_str[r_icache_fsm]
        << " cmd fsm: " << cmd_fsm_state_str[r_vci_cmd_fsm]
        << " rsp fsm: " << rsp_fsm_state_str[r_vci_rsp_fsm] << std::endl;
#endif

    m_cpt_total_cycles++;

    typename iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

    typename iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;

    m_iss.getRequests( ireq, dreq );

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << name() << " Instruction Request: " << ireq << std::endl;
std::cout << name() << " Data Request: " << dreq << std::endl;
#endif

    ////////////////////////////////////////////////////////////////////////////////////////
    //      ICACHE_FSM
    //
    // There is 9 mutually exclusive conditions to exit the IDLE state.
    // Four configurations corresponding to an XTN request from processor,
    // - Flush TLB (in case of Context switch) => TLB_FLUSH state 
    // - Flush cache => CACHE_FLUSH state 
    // - Invalidate a TLB entry => TLB_INVAL state
    // - Invalidate a cache line => CACHE_INVAL state
    // Five configurations corresponding to various TLB or cache MISS :
    // - TLB miss(in case hit_p miss) => TLB1_READ state
    // - TLB miss(in case hit_p hit) => TLB2_READ state
    // - Hit in TLB but VPN changed => BIS state
    // - Cached read miss => MISS_REQ state
    // - Uncache read miss => UNC_REQ state
    // 
    // In case of MISS, the controller writes a request in the r_icache_paddr_save register 
    // and sets the corresponding request flip-flop : r_icache_tlb_read_req, r_icache_miss_req 
    // or r_icache_unc_req. These request flip-flops are reset by the VCI_RSP controller 
    // when the response is ready in the ICACHE buffer.
    //
    // The DCACHE FSM signals XTN processor requests using the r_dcache_xtn_req flip-flop. 
    // The request opcod and the address to be invalidated are transmitted
    // in the r_dcache_paddr_save & r_dcache_wdata_save registers respectively.
    // The request flip-flop is reset by the ICACHE_FSM when the operation is completed.
    //
    // The r_vci_rsp_ins_error flip-flop is set by the VCI_RSP FSM and reset
    // by the ICACHE-FSM in the ICACHE_ERROR state.
    //
    //----------------------------------------------------------------------------------- 
    // Instruction TLB: 
    //  
    // - int        ET          (00: unmapped; 01: unused or PTD)
    //                          (10: PTE new;  11: PTE old      )
    // - bool       cachable    (cached bit)
    // - bool       writable    (** not used alwayse false) 
    // - bool       executable  (executable bit)
    // - bool       user        (access in user mode allowed)
    // - bool       global      (PTE not invalidated by a TLB flush)
    // - bool       dirty       (** not used alwayse false) 
    // - uint32_t   vpn         (virtual page number)
    // - uint32_t   ppn         (physical page number)
    ////////////////////////////////////////////////////////////////////////////////////////

    switch(r_icache_fsm) {

    ////////////////
    case ICACHE_IDLE:
    {
        pte_info_t  icache_pte_info;
        addr36_t    tlb_ipaddr     = 0;        // physical address obtained from TLB                                
        addr36_t    spc_ipaddr     = 0;        // physical adress obtained from PPN_save (speculative)                         
        data_t      icache_ins     = 0;        // read instruction
        bool        icache_hit_c   = false;    // Cache hit
        bool        icache_cached  = false;    // cacheable access (read)
        bool        icache_hit_t_m = false;    // hit on 4Mega TLB
        bool        icache_hit_t_k = false;    // hit on 4Kilo TLB
        bool        icache_hit_x   = false;    // VPN unmodified (can use spc_dpaddr)
        bool        icache_hit_p   = false;    // PTP unmodified (can skip first level page table walk)
        size_t      icache_tlb_way = 0;        // selected way (in case of cache hit)
        size_t      icache_tlb_set = 0;        // selected set (Y field in address)

        // Decoding processor XTN requests
        // They are sent by DCACHE FSM  

        if (r_dcache_xtn_req)
        {
            if ( ireq.valid ) m_cost_ins_miss_frz++;

            if ((int)r_dcache_type_save == (int)iss_t::XTN_PTPR)  
            {
                r_icache_fsm = ICACHE_TLB_FLUSH;   
                break;
            }
            if ((int)r_dcache_type_save == (int)iss_t::XTN_ICACHE_FLUSH)
            {
                r_icache_fsm = ICACHE_CACHE_FLUSH;   
                break;
            }
            if ((int)r_dcache_type_save == (int)iss_t::XTN_ITLB_INVAL) 
            {
                r_icache_fsm = ICACHE_TLB_INVAL;   
                break;
            }
            if ((int)r_dcache_type_save == (int)iss_t::XTN_ICACHE_INVAL) 
            {
                r_icache_fsm = ICACHE_CACHE_INVAL;   
                break;
            }
        } // end if xtn_req

        // icache_hit_t_m, icache_hit_t_k, icache_hit_x, icache_hit_p 
        // icache_pte_info, icache_tlb_way, icache_tlb_set & ipaddr & cacheability 
        // - If MMU activated : cacheability is defined by the cachable bit in the TLB
        // - If MMU not activated : cacheability is defined by the segment table.

        if ( r_mmu_mode == TLBS_DEACTIVE || r_mmu_mode == ITLB_D_DTLB_A )   // MMU not activated 
        {
            icache_hit_t_m = true;         
            icache_hit_t_k = true;         
            icache_hit_x   = true;         
            icache_hit_p   = true;         
            tlb_ipaddr     = ireq.addr;
            spc_ipaddr     = ireq.addr;
            icache_cached  = m_cacheability_table[ireq.addr];
        } 
        else                                                                // MMU activated
        { 
            m_cpt_ins_tlb_read++;
            icache_hit_t_m = icache_m_tlb.translate(ireq.addr, &tlb_ipaddr, &icache_pte_info, 
                                                    &icache_tlb_way, &icache_tlb_set); 
            icache_hit_t_k = icache_k_tlb.translate(ireq.addr, &tlb_ipaddr, &icache_pte_info, 
                                                    &icache_tlb_way, &icache_tlb_set); 
            icache_hit_x   = (((addr_t)r_icache_vpn_save << PAGE_K_NBITS) == (ireq.addr & ~OFFSET_K_MASK)) && r_itlb_translation_valid;
            icache_hit_p   = (((ireq.addr >> PAGE_M_NBITS) == r_icache_id1_save) && r_icache_ptba_ok); 
            spc_ipaddr     = ((addr36_t)r_icache_ppn_save << PAGE_K_NBITS) | (addr36_t)(ireq.addr & OFFSET_K_MASK);
            icache_cached  = icache_pte_info.c; 
        }

        if ( ireq.valid ) 
        {
            m_cpt_icache_dir_read += m_icache_ways;
            m_cpt_icache_data_read += m_icache_ways;

            // icache_hit_c & icache_ins
            if ( icache_cached )    // using speculative physical address for cached access
            {
                icache_hit_c = r_icache.read(spc_ipaddr, &icache_ins);
            }
            else                    // using actual physical address for uncached access
            {
                icache_hit_c = ( r_icache_buf_unc_valid && (tlb_ipaddr == r_icache_paddr_save) );
                icache_ins = r_icache_miss_buf[0];
            }

            if ( r_mmu_mode == TLBS_ACTIVE || r_mmu_mode == ITLB_A_DTLB_D ) 
            {
                if ( icache_hit_t_m || icache_hit_t_k ) 
                {
                    // check access rights

                    if ( !icache_pte_info.u && (ireq.mode == iss_t::MODE_USER)) 
                    {
                        r_icache_error_type = r_icache_error_type | MMU_PRIVILEGE_VIOLATION;  
                        r_icache_bad_vaddr = ireq.addr;
                        irsp.valid = true;
                        irsp.error = true;
                        irsp.instruction = 0;
                        break;
                    }
                    if ( !icache_pte_info.x ) 
                    {
                        r_icache_error_type = r_icache_error_type | MMU_EXEC_VIOLATION;  
                        r_icache_bad_vaddr = ireq.addr;
                        irsp.valid = true;
                        irsp.error = true;
                        irsp.instruction = 0;
                        break;
                    }
                }

                // update LRU, save ppn, vpn and page type

                if ( icache_hit_t_m )
                {  
                    icache_m_tlb.setlru(icache_tlb_way,icache_tlb_set);     
                    r_icache_ppn_save = tlb_ipaddr >> PAGE_K_NBITS;
                    r_icache_vpn_save = ireq.addr >> PAGE_K_NBITS;
                    r_itlb_translation_valid = true;
                    r_icache_page_k_save = false;
                }
                else if ( icache_hit_t_k )
                {
                    icache_k_tlb.setlru(icache_tlb_way,icache_tlb_set); 
                    r_icache_ppn_save = tlb_ipaddr >> PAGE_K_NBITS;
                    r_icache_vpn_save = ireq.addr >> PAGE_K_NBITS;
                    r_itlb_translation_valid = true;   
                    r_icache_page_k_save = true;
                }
                else
                {
                    r_itlb_translation_valid = false;
                }

            } // end if MMU activated

            // compute next state 

            if ( !icache_hit_t_m && !icache_hit_t_k && !icache_hit_p )      // TLB miss
            {
                // walk page table  level 1
                r_icache_paddr_save = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((ireq.addr>>PAGE_M_NBITS)<<2);
                r_icache_tlb_read_req = true;
                r_icache_fsm = ICACHE_TLB1_READ;
                m_cpt_ins_tlb_miss++;
                m_cost_ins_tlb_miss_frz++;
            }
            else if ( !icache_hit_t_m && !icache_hit_t_k && icache_hit_p )  // TLB Miss with possibility of bypass first level page
            {
                // walk page table level 2
                r_icache_paddr_save = (addr36_t)r_icache_ptba_save | 
                                      (addr36_t)(((ireq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2);
                r_icache_tlb_read_req = true;
                r_icache_fsm = ICACHE_TLB2_READ;
                m_cpt_ins_tlb_miss++;
                m_cost_ins_tlb_miss_frz++;
            }
            else if ( (icache_hit_t_m || icache_hit_t_k) && !icache_hit_x && icache_cached ) // cached access with an ucorrect speculative physical address
            {
                r_icache_paddr_save = tlb_ipaddr;   // save actual physical address for BIS
                r_icache_fsm = ICACHE_BIS;
                m_cost_ins_miss_frz++;
            }
            else    // cached or uncached access with a correct speculative physical address 
            {   
                m_cpt_ins_read++;      
                if ( !icache_hit_c ) 
                {
                    m_cpt_ins_miss++;
                    m_cost_ins_miss_frz++;
                    r_icache_paddr_save = spc_ipaddr; 
                    if ( icache_cached ) 
                    {
                        r_icache_miss_req = true;
                        r_icache_fsm = ICACHE_MISS_WAIT;
                    } 
                    else 
                    {
                        r_icache_unc_req = true;
                        r_icache_buf_unc_valid = false;
                        r_icache_fsm = ICACHE_UNC_WAIT;
                    } 
                } 
                else 
                {
                    r_icache_buf_unc_valid = false;
                    r_icache_fsm = ICACHE_IDLE;
                }
                irsp.valid = icache_hit_c;
                irsp.instruction = icache_ins;
            } // end if next states
            
        } // end if ireq.valid
        break;
    }
    ////////////////
    case ICACHE_BIS: 
    {
        data_t  icache_ins = 0;
        bool    icache_hit = false;

        // acces is always cached in this state
        icache_hit = r_icache.read(r_icache_paddr_save, &icache_ins);
 
        m_cpt_ins_read++;      
        if ( !icache_hit )
        {
            r_icache_miss_req = true;
            r_icache_fsm = ICACHE_MISS_WAIT;
            m_cpt_ins_miss++;
            m_cost_ins_miss_frz++;
        } 
        else
        {
            r_icache_fsm = ICACHE_IDLE; 
        }

        irsp.valid = icache_hit;
        irsp.error = false;
        irsp.instruction = icache_ins;
        break;
    }
    //////////////////////
    case ICACHE_TLB1_READ:
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;

        if ( !r_icache_tlb_read_req && !r_vci_rsp_ins_error) // vci response ok
        {  
            switch((r_vci_rsp_itlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) { 
            case PTD:               // 4K page TLB
        	    r_icache_ptba_ok      = true;	
                r_icache_ptba_save    = (addr36_t)((r_vci_rsp_itlb_miss & PTD_PTP_MASK)>>PTD_SHIFT) << PAGE_K_NBITS; 
                r_icache_id1_save     = ireq.addr >> PAGE_M_NBITS;
                r_icache_paddr_save   = (addr36_t)((r_vci_rsp_itlb_miss & PTD_PTP_MASK)>>PTD_SHIFT) << PAGE_K_NBITS |
                                        (addr36_t)(((ireq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2); 
                r_icache_tlb_read_req = true;
                r_icache_fsm          = ICACHE_TLB2_READ;
                break;
            case PTE_NEW:           // 4M page TLB (not marked)   
        	    r_icache_ptba_ok    = false;	
                r_icache_pte_update = r_vci_rsp_itlb_miss | PTE_ET_MASK;
                r_icache_tlb_et_req = true;
                r_icache_fsm        = ICACHE_TLB1_WRITE;
                m_cpt_ins_tlb_write_et++;
                break; 
            case PTE_OLD:           // 4M page TLB (already marked)
        	    r_icache_ptba_ok    = false;	
                r_icache_pte_update = r_vci_rsp_itlb_miss;
                r_icache_fsm        = ICACHE_TLB1_UPDT;
                break;
            default:                // unmapped
        	    r_icache_ptba_ok    = false;	
                r_icache_error_type = r_icache_error_type | MMU_PT1_UNMAPPED;  
                r_icache_bad_vaddr  = ireq.addr;
                r_icache_fsm        = ICACHE_ERROR;
                break;
            } // end switch ET
        }

        if ( !r_icache_tlb_read_req && r_vci_rsp_ins_error ) // vci response error
        {  
            r_icache_fsm = ICACHE_ERROR;
            r_icache_error_type = r_icache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
            r_icache_bad_vaddr = ireq.addr;
        }
        break;
    }
    ///////////////////////
    case ICACHE_TLB1_WRITE:  
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;

        if (!r_icache_tlb_et_req)
        { 
            if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = r_icache_error_type | MMU_PT1_ILLEGAL_ACCESS;  
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else  
            {
                r_icache_fsm = ICACHE_TLB1_UPDT;  
            }
        } 
        break;
    }
    //////////////////////
    case ICACHE_TLB1_UPDT:
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;

        icache_m_tlb.update(r_icache_pte_update,ireq.addr);
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    /////////////////////
    case ICACHE_TLB2_READ:
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;

        if ( !r_icache_tlb_read_req && !r_vci_rsp_ins_error) // VCI response ok
        {
            switch((r_vci_rsp_itlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTE_NEW:               // not marked
                r_icache_pte_update = r_vci_rsp_itlb_miss | PTE_ET_MASK;
                r_icache_tlb_et_req = true;
                r_icache_fsm        = ICACHE_TLB2_WRITE;
                m_cpt_ins_tlb_write_et++;
                break;  
            case PTE_OLD:               // already marked
                r_icache_fsm        = ICACHE_TLB2_UPDT;
                r_icache_pte_update = r_vci_rsp_itlb_miss;
                break;
            default:                    // unmapped
                r_icache_error_type = r_icache_error_type | MMU_PT2_UNMAPPED;  
                r_icache_bad_vaddr  = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
                break;
            }
        }

        if ( !r_icache_tlb_read_req && r_vci_rsp_ins_error ) // VCI response error
        {
            r_icache_error_type = r_icache_error_type | MMU_PT2_ILLEGAL_ACCESS;
            r_icache_bad_vaddr = ireq.addr;
            r_icache_fsm = ICACHE_ERROR;
        }
        break;
    }
    /////////////////////////
    case ICACHE_TLB2_WRITE:
    {  
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;

        if (!r_icache_tlb_et_req) 
        {
            if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = r_icache_error_type | MMU_PT2_ILLEGAL_ACCESS;  
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else  
            {
                r_icache_fsm = ICACHE_TLB2_UPDT;  
            }
        } 
        break;
    }
    /////////////////////
    case ICACHE_TLB2_UPDT: 
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;

        icache_k_tlb.update(r_icache_pte_update,ireq.addr); 
        r_icache_fsm = ICACHE_IDLE;  
        break;
    }
    /////////////////////
    case ICACHE_TLB_FLUSH:
    {
        icache_m_tlb.flush(false);    // global entries are not invalidated
        icache_k_tlb.flush(false);    // global entries are not invalidated
        r_dcache_xtn_req = false;
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    ////////////////////////
    case ICACHE_CACHE_FLUSH:
    {
        r_icache.reset();
        r_dcache_xtn_req = false;
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    /////////////////////
    case ICACHE_TLB_INVAL:  
    {
        if ( icache_m_tlb.translate(r_dcache_wdata_save) )
        {
            r_icache_page_k_save = false; 
            r_icache_fsm = ICACHE_TLB_INVAL_DONE;
        }
        else if ( icache_k_tlb.translate(r_dcache_wdata_save) )
        {
            r_icache_page_k_save = true; 
            r_icache_fsm = ICACHE_TLB_INVAL_DONE;
        }
        else
        {
            r_dcache_xtn_req = false;
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
	}
    ///////////////////////////
    case ICACHE_TLB_INVAL_DONE:
    {
		if (r_icache_page_k_save)   
        {
            icache_k_tlb.inval(r_dcache_wdata_save);
        }
        else
        {
            icache_m_tlb.inval(r_dcache_wdata_save);
        }
        r_dcache_xtn_req = false;
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    ////////////////////////
    case ICACHE_CACHE_INVAL:
    {	
        addr36_t    ipaddr;                     
        bool        icache_hit_t_m;
        bool        icache_hit_t_k;

        if ( r_mmu_mode == TLBS_ACTIVE || r_mmu_mode == ITLB_A_DTLB_D ) 
        {
            icache_hit_t_m = icache_m_tlb.translate(r_dcache_wdata_save, &ipaddr); 
            icache_hit_t_k = icache_k_tlb.translate(r_dcache_wdata_save, &ipaddr); 
        } 
        else 
        {
            ipaddr = (addr36_t)r_dcache_wdata_save;
            icache_hit_t_m = true; 
            icache_hit_t_k = true;
        }
        if ( icache_hit_t_m || icache_hit_t_k )
        {
            r_icache_paddr_save = ipaddr;
            r_icache_fsm = ICACHE_CACHE_INVAL_DONE;
        }
        else
        {
            r_dcache_xtn_req = false; 
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
    }
    /////////////////////////////
    case ICACHE_CACHE_INVAL_DONE:
    {
        r_icache.inval(r_icache_paddr_save);  
        r_dcache_xtn_req = false; 
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    ///////////////////////
    case ICACHE_MISS_WAIT:
    {
        m_cost_ins_miss_frz++;
        if ( !r_icache_miss_req )
        {
            if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = r_icache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else 
            {
                r_icache_fsm = ICACHE_MISS_UPDT;  
            } 
        }
        break;
    }
    ////////////////////
    case ICACHE_UNC_WAIT:
    {
        m_cost_ins_miss_frz++;
        if ( !r_icache_unc_req ) 
        {
            if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = r_icache_error_type | MMU_CACHE_ILLEGAL_ACCESS;    
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else 
            {
                r_icache_buf_unc_valid = true;
                r_icache_fsm = ICACHE_IDLE;
            }
        }
        break;
    }
    //////////////////////
    case ICACHE_MISS_UPDT:
    {
        addr36_t ipaddr = r_icache_paddr_save;
        data_t* buf = r_icache_miss_buf;
        addr36_t  victim_index = 0;
        m_cpt_icache_dir_write++;
        m_cpt_icache_data_write++;
        m_cost_ins_miss_frz++;
        r_icache.update(ipaddr, buf, &victim_index);
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    ///////////////////
    case ICACHE_ERROR:
    {
        r_vci_rsp_ins_error = false;
        irsp.valid = true;
        irsp.error = true;
        irsp.instruction = 0; 
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    } // end switch r_icache_fsm

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << name() << " Instruction Response: " << irsp << std::endl;
#endif

    ////////////////////////////////////////////////////////////////////////////////////
    //      DCACHE FSM 
    //
    // Both the Cacheability Table, and the MMU cached bit are used to define
    // the cacheability.
    // 
    // There is 14 mutually exclusive conditions to exit the IDLE state.
    // Seven configurations corresponding to an XTN request from processor:
    // - Context switch => CTXT_SWITCH state
    // - Flush dcache => DCACHE_FLUSH state 
    // - Flush icache => ICACHE_FLUSH state 
    // - Invalidate a dtlb entry => DTLB_INVAL state
    // - Invalidate a itlb entry => ITLB_INVAL state
    // - Invalidate a dcache line => DCACHE_INVAL state
    // - Invalidate a icache line => ICACHE_INVAL state
    // Seven configurations corresponding to various read miss or write requests: 
    // - TLB miss(in case hit_p miss) => TLB1_READ state
    // - TLB miss(in case hit_p hit) => TLB2_READ state
    // - Hit in TLB but VPN changed => BIS state
    // - Cached read miss => MISS_REQ state
    // - Uncache read miss => UNC_REQ state
    // - Write hit => WRITE_UPDT state
    // - Write miss => WRITE_REQ
    //
    // The r_vci_rsp_data_error flip-flop is set by the VCI_RSP controller and reset 
    // by DCACHE-FSM when its state is in DCACHE_ERROR. 
    //--------------------------------------------------------------------- 
    // Data TLB: 
    //  
    // - int        ET          (00: unmapped; 01: unused or PTD)
    //                          (10: PTE new;  11: PTE old      )
    // - bool       cachable    (cached bit)
    // - bool       writable    (writable bit) 
    // - bool       executable  (** not used alwayse false)
    // - bool       user        (access in user mode allowed)
    // - bool       global      (PTE not invalidated by a TLB flush)
    // - bool       dirty       (page has been modified) 
    // - uint32_t   vpn         (virtual page number)
    // - uint32_t   ppn         (physical page number)
    ////////////////////////////////////////////////////////////////////////////////////////

    switch (r_dcache_fsm) {

    //////////////////////
    case DCACHE_WRITE_REQ:
    {
        // try to post the write request in the write buffer
        if ( !r_dcache_write_req )     // no previous write transaction     
        {
            if ( r_wbuf.wok(r_dcache_paddr_save) )   // write request in the same cache line 
            {    
                r_wbuf.write(r_dcache_paddr_save, r_dcache_be_save, r_dcache_wdata_save);
                // closing the write packet if uncached
                if ( !r_dcache_cached_save )
                { 
                    r_dcache_write_req = true;
                }
            } 
            else 
            {    // close the write packet if write request not in the same cache line
                r_dcache_write_req = true;
                m_cost_write_frz++;
                break;  //  posting not possible : stay in DCACHE_WRITEREQ state
            }
        } 
        else     //  previous write transaction not completed
        {
            m_cost_write_frz++;
            break;  //  posting not possible : stay in DCACHE_WRITEREQ state
        }

        // close the write packet if the next processor request is not a write 
        if ( !dreq.valid || (dreq.type != iss_t::DATA_WRITE)) 
        {
            r_dcache_write_req = true;
        }
        
        // The next state and the processor request parameters are computed 
        // as in the DCACHE_IDLE state (see below ...)
    }
    /////////////////
    case DCACHE_IDLE:
    {
        if (dreq.valid) 
        {
            pte_info_t  dcache_pte_info;
            int         xtn_opcod       = (int)dreq.addr/4;
            addr36_t    tlb_dpaddr      = 0;        // physical address obtained from TLB
            addr36_t    spc_dpaddr      = 0;        // physical adress obtained from PPN_save (speculative)
            bool        dcache_hit_t_m  = false;    // hit on 4Mega TLB
            bool        dcache_hit_t_k  = false;    // hit on 4Kilo TLB
            bool        dcache_hit_x    = false;    // VPN unmodified (can use spc_dpaddr)
            bool        dcache_hit_p    = false;    // PTP unmodified (can skip first level page table walk)
            size_t      dcache_tlb_way  = 0;        // selected way (in case of cache hit)
            size_t      dcache_tlb_set  = 0;        // selected set (Y field in address)
            bool        dcache_hit_c    = false;    // Cache hit
            data_t      dcache_rdata    = 0;        // read data
            bool        dcache_cached   = false;    // cacheable access (read or write)

            m_cpt_dcache_data_read += m_dcache_ways;
            m_cpt_dcache_dir_read += m_dcache_ways;

            // Decoding READ XTN requests from processor
            // They are executed in this DCACHE_IDLE state

            if (dreq.type == iss_t::XTN_READ) 
            {
                switch(xtn_opcod) {
                case iss_t::XTN_INS_ERROR_TYPE:
                    dcache_rdata = (uint32_t)r_icache_error_type;
                    r_icache_error_type = MMU_NONE;
                    break;
                case iss_t::XTN_DATA_ERROR_TYPE:
                    dcache_rdata = (uint32_t)r_dcache_error_type;
                    r_dcache_error_type = MMU_NONE;
                    break;
                case iss_t::XTN_INS_BAD_VADDR:
                    dcache_rdata = (uint32_t)r_icache_bad_vaddr;       
                    break;
                case iss_t::XTN_DATA_BAD_VADDR:
                    dcache_rdata = (uint32_t)r_dcache_bad_vaddr;        
                    break;
                case iss_t::XTN_PTPR:
                    dcache_rdata = (uint32_t)r_mmu_ptpr;
                    break;
                case iss_t::XTN_TLB_MODE:
                    dcache_rdata = (uint32_t)r_mmu_mode;
                    break;
                default:
                    break;
                }
                drsp.valid = true;
                drsp.error = false;
                drsp.rdata = dcache_rdata;
                break;
            }

            // Decoding WRITE XTN requests from processor
            // If there is no privilege violation, they are not executed in this DCACHE_IDLE state,
            // but in the next state, because they generally require access to the caches or the TLBs 

            if (dreq.type == iss_t::XTN_WRITE) 
            {
                drsp.valid = false;
                drsp.error = false;
                drsp.rdata = 0;
                r_dcache_wdata_save = dreq.wdata;   
                switch(xtn_opcod) {     

                case iss_t::XTN_PTPR:       // context switch : checking the kernel mode
                                            // both instruction & data TLBs must be flushed
                    if (dreq.mode == iss_t::MODE_KERNEL) 
                    {
                        r_mmu_ptpr = dreq.wdata;
                        r_icache_error_type = MMU_NONE;
                        r_dcache_error_type = MMU_NONE;
                        r_dcache_type_save = dreq.addr/4; 
                        r_dcache_xtn_req = true;
                        r_dcache_fsm = DCACHE_CTXT_SWITCH;
                    } 
                    else 
                    { 
                        r_dcache_error_type = r_dcache_error_type | MMU_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                    }
                    break;

                case iss_t::XTN_TLB_MODE:     // modifying TLBs mode : checking the kernel mode
                    if (dreq.mode == iss_t::MODE_KERNEL) 
                    {
                        r_mmu_mode = (int)dreq.wdata;
                        drsp.valid = true;
                    } 
                    else 
                    {
                        r_dcache_error_type = r_dcache_error_type | MMU_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                    }
                    break;

                case iss_t::XTN_DTLB_INVAL:     //  checking the kernel mode
                    if (dreq.mode == iss_t::MODE_KERNEL) 
                    {
                        r_dcache_fsm = DCACHE_DTLB_INVAL;  
                    } 
                    else 
                    {
                        r_dcache_error_type = r_dcache_error_type | MMU_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                    }
                    break;

                case iss_t::XTN_ITLB_INVAL:     //  checking the kernel mode
                    if (dreq.mode == iss_t::MODE_KERNEL) 
                    {
                        r_dcache_xtn_req = true;
                        r_dcache_type_save = dreq.addr/4;
                        r_dcache_fsm = DCACHE_ITLB_INVAL;  
                    } 
                    else 
                    {
                        r_dcache_error_type = r_dcache_error_type | MMU_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                    }
                    break;

                case iss_t::XTN_DCACHE_INVAL:   // cache inval can be executed in user mode.
                    r_dcache_fsm = DCACHE_DCACHE_INVAL;
                    break;

                case iss_t::XTN_DCACHE_FLUSH:   // cache flush can be executed in user mode.
                    r_dcache_fsm = DCACHE_DCACHE_FLUSH; 
                    break;

                case iss_t::XTN_ICACHE_INVAL:   // cache inval can be executed in user mode.
                    r_dcache_type_save = dreq.addr/4; 
                    r_dcache_xtn_req = true;
                    r_dcache_fsm = DCACHE_ICACHE_INVAL; 
                    break;

                case iss_t::XTN_ICACHE_FLUSH:   // cache flush can be executed in user mode.
                    r_dcache_type_save = dreq.addr/4; 
                    r_dcache_xtn_req = true; 
                    r_dcache_fsm = DCACHE_ICACHE_FLUSH;
                    break;

                default:
                    r_dcache_error_type = r_dcache_error_type | MMU_UNDEFINED_XTN; 
                    r_dcache_bad_vaddr  = dreq.addr;
                    drsp.valid = true;
                    drsp.error = true;
                    break;
                } // end switch xtn_opcod

                break;
            } // end if XTN_WRITE

            // Evaluating dcache_hit_t, dcache_hit_x, dcache_hit_p, dcache_hit_c,
            // dcache_pte_info, dcache_tlb_way, dcache_tlb_set & dpaddr & cacheability 
            // - If MMU activated : cacheability is defined by the cachable bit in the TLB
            // - If MMU not activated : cacheability is defined by the segment table.

            if ( r_mmu_mode == TLBS_DEACTIVE || r_mmu_mode == ITLB_A_DTLB_D ) // MMU not activated
            {
                dcache_hit_t_m  = true;         
                dcache_hit_t_k  = true;       
                dcache_hit_x    = true;   
                dcache_hit_p    = true;  
                tlb_dpaddr      = dreq.addr; 
                spc_dpaddr      = dreq.addr;    
                dcache_cached   = m_cacheability_table[dreq.addr] && 
                                  ((dreq.type != iss_t::DATA_LL)  && (dreq.type != iss_t::DATA_SC) &&
                                   (dreq.type != iss_t::XTN_READ) && (dreq.type != iss_t::XTN_WRITE));     
            } 
            else                                                            // MMU activated
            {
                m_cpt_data_tlb_read++;
                dcache_hit_t_m = dcache_m_tlb.translate(dreq.addr, &tlb_dpaddr, &dcache_pte_info, 
                                                        &dcache_tlb_way, &dcache_tlb_set); 
                dcache_hit_t_k = dcache_k_tlb.translate(dreq.addr, &tlb_dpaddr, &dcache_pte_info, 
                                                        &dcache_tlb_way, &dcache_tlb_set);
                  
                spc_dpaddr     = ((addr36_t)r_dcache_ppn_save << PAGE_K_NBITS) | (addr36_t)((dreq.addr & OFFSET_K_MASK));
                dcache_hit_x   = (((addr_t)r_dcache_vpn_save << PAGE_K_NBITS) == (dreq.addr & ~OFFSET_K_MASK)) && r_dtlb_translation_valid; 
                dcache_hit_p   = (((dreq.addr >> PAGE_M_NBITS) == r_dcache_id1_save) && r_dcache_ptba_ok );
                dcache_cached  = dcache_pte_info.c && 
                                 ((dreq.type != iss_t::DATA_LL)  && (dreq.type != iss_t::DATA_SC) &&
                                  (dreq.type != iss_t::XTN_READ) && (dreq.type != iss_t::XTN_WRITE));;    
            }

            // dcache_hit_c & dcache_rdata
            if ( dcache_cached )    // using speculative physical address for cached access
            {
                dcache_hit_c = r_dcache.read(spc_dpaddr, &dcache_rdata);
            } 
            else                    // using actual physical address for uncached access
            {
                dcache_hit_c = ((tlb_dpaddr == r_dcache_paddr_save) && r_dcache_buf_unc_valid ); 
                dcache_rdata = r_dcache_miss_buf[0];
            }

            if ((r_mmu_mode == TLBS_ACTIVE) || (r_mmu_mode == ITLB_D_DTLB_A)) 
            {
                // Checking access rights

                if ( dcache_hit_t_m || dcache_hit_t_k ) 
                {
                    if (!dcache_pte_info.u && (dreq.mode == iss_t::MODE_USER)) 
                    {
                        r_dcache_error_type = r_dcache_error_type | MMU_PRIVILEGE_VIOLATION;  
                        r_dcache_bad_vaddr = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;
                        break;
                    }
                    if (!dcache_pte_info.w && (dreq.type == iss_t::DATA_WRITE)) 
                    {
                        r_dcache_error_type = r_dcache_error_type | MMU_WRITE_VIOLATION;  
                        r_dcache_bad_vaddr = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;
                        break;
                    }
                }

                // update LRU, save ppn, vpn and page type
                if ( dcache_hit_t_m ) {
                    dcache_m_tlb.setlru(dcache_tlb_way,dcache_tlb_set); 
                    r_dcache_ppn_save = tlb_dpaddr >> PAGE_K_NBITS;
                    r_dcache_vpn_save = dreq.addr >> PAGE_K_NBITS;
                    r_dtlb_translation_valid = true;
                    r_dcache_page_k_save = false;
                }
                else if ( dcache_hit_t_k ) 
                {
                    dcache_k_tlb.setlru(dcache_tlb_way,dcache_tlb_set);
                    r_dcache_ppn_save = tlb_dpaddr >> PAGE_K_NBITS;
                    r_dcache_vpn_save = dreq.addr >> PAGE_K_NBITS;
                    r_dtlb_translation_valid = true;
                    r_dcache_page_k_save = true;
                }
                else
                {
                    r_dtlb_translation_valid = false;
                }

            } // end if MMU activated

            // compute next state 

            if ( !dcache_hit_p && !dcache_hit_t_m && !dcache_hit_t_k )  // TLB miss
            {
                // walk page table level 1
                r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                r_dcache_tlb_read_req = true;
                r_dcache_fsm = DCACHE_TLB1_READ;
                m_cpt_data_tlb_miss++;
                m_cost_data_tlb_miss_frz++;
            }
            else if ( dcache_hit_p && !dcache_hit_t_m && !dcache_hit_t_k )  // TLB Miss with possibility of bypass first level page
            {
                // walk page table level 2
                r_dcache_tlb_paddr = (addr36_t)r_dcache_ptba_save | 
                                        (addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2); 
                r_dcache_tlb_read_req = true;
                r_dcache_fsm = DCACHE_TLB2_READ;
                m_cpt_data_tlb_miss++;
                m_cost_data_tlb_miss_frz++;
            }
            else if ( (dcache_hit_t_m || dcache_hit_t_k) && !dcache_hit_x && dcache_cached )// cached access with an ucorrect speculative physical address
            {
                r_dcache_hit_p_save = dcache_hit_p;
                r_dcache_fsm = DCACHE_BIS;
            }
            else  // cached or uncached access with a correct speculative physical address
            {
                switch( dreq.type ) {
                    case iss_t::DATA_READ:
                    case iss_t::DATA_LL:
                    case iss_t::DATA_SC:
                        m_cpt_read++;
                        if ( dcache_hit_c ) 
                        {
                            r_dcache_buf_unc_valid = false;
                            r_dcache_fsm = DCACHE_IDLE;
                            drsp.valid = true;
                            drsp.rdata = dcache_rdata;
                        } 
                        else 
                        {
                            if ( dcache_cached ) 
                            {
                                r_dcache_miss_req = true;
                                r_dcache_fsm = DCACHE_MISS_WAIT;
                                m_cpt_data_miss++;
                                m_cost_data_miss_frz++;
                            } 
                            else 
                            {
                                r_dcache_unc_req = true;
                                r_dcache_fsm = DCACHE_UNC_WAIT;
                                m_cpt_unc_read++;
                                m_cost_unc_read_frz++;
                            }
                        }
                        break;
                    case iss_t::DATA_WRITE:
                        m_cpt_write++;
                        if ( dcache_cached ) m_cpt_write_cached++;

                        if ( dcache_hit_c && dcache_cached )    // cache update required
                        {
                            r_dcache_fsm = DCACHE_WRITE_UPDT;

                        } 
                        else if ( !dcache_pte_info.d && ((r_mmu_mode == TLBS_ACTIVE)||(r_mmu_mode == ITLB_D_DTLB_A)))   // dirty bit update required
                        {
                            if (dcache_hit_t_m) 
                            {
                                r_dcache_pte_update = dcache_m_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                                r_dcache_tlb_dirty_req = true;
                                r_dcache_fsm = DCACHE_WRITE_DIRTY;
                                m_cpt_data_tlb_write_dirty++;
                            }
                            else
                            {   
                                if (dcache_hit_p) 
                                {
                                    r_dcache_pte_update = dcache_k_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                    r_dcache_tlb_paddr = (addr36_t)r_dcache_ptba_save | (addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2);
                                    r_dcache_tlb_dirty_req = true;
                                    r_dcache_fsm = DCACHE_WRITE_DIRTY;
                                    m_cpt_data_tlb_write_dirty++;
                                }
                                else    // get PTBA to calculate the physical address of PTE
                                {
                                    r_dcache_pte_update = dcache_k_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                    r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                                    r_dcache_tlb_read_req = true;
                                    r_dcache_tlb_ptba_read = true;
                                    r_dcache_fsm = DCACHE_TLB1_READ;
                                }
                            }
                            m_cost_data_tlb_miss_frz++;
                        }
                        else                                    // no cache update, not dirty bit update
                        {
                            r_dcache_fsm = DCACHE_WRITE_REQ;
                            drsp.valid = true;
                            drsp.rdata = 0;
                        }
                        break;
                    default:
                        break;
                } // end switch dreq.type
            } // end if next states

            // save values for the next states
            r_dcache_paddr_save   = tlb_dpaddr;
            r_dcache_type_save    = dreq.type;
            r_dcache_wdata_save   = dreq.wdata;
            r_dcache_be_save      = dreq.be;
            r_dcache_rdata_save   = dcache_rdata;
            r_dcache_cached_save  = dcache_cached;
            r_dcache_dirty_save   = dcache_pte_info.d;
            r_dcache_tlb_set_save = dcache_tlb_set;
            r_dcache_tlb_way_save = dcache_tlb_way;

        } // end if dreq.valid
        else 
        {   
            r_dcache_fsm = DCACHE_IDLE;
        }

        // processor request are not accepted in the WRITE_REQ state 
        // when the write buffer is not writeable

        if ((r_dcache_fsm == DCACHE_WRITE_REQ) && 
            (r_dcache_write_req || !r_wbuf.wok(r_dcache_paddr_save))) 
        {
            drsp.valid = false;
        }
        break;
    }
    /////////////////
    case DCACHE_BIS:
    {
        bool    dcache_hit   = false;
        data_t  dcache_rdata = 0;

        // acces always cached in this state
        dcache_hit = r_dcache.read(r_dcache_paddr_save, &dcache_rdata);

        if ( dreq.type == iss_t::DATA_READ )  // cached read
        {
            m_cpt_read++;
            if ( !dcache_hit ) 
            {
                r_dcache_miss_req = true;
                r_dcache_fsm = DCACHE_MISS_WAIT;
                m_cpt_data_miss++;
                m_cost_data_miss_frz++;
            }
            else
            {
                r_dcache_fsm = DCACHE_IDLE;
            }
            drsp.valid = dcache_hit;
            drsp.error = false;
            drsp.rdata = dcache_rdata;
        }
        else    // cached write
        {
            m_cpt_write++;
            m_cpt_write_cached++;
            if ( dcache_hit )    // cache update required
            {
                r_dcache_fsm = DCACHE_WRITE_UPDT;

            } 
            else if ( !r_dcache_dirty_save && ((r_mmu_mode == TLBS_ACTIVE)||(r_mmu_mode == ITLB_D_DTLB_A)))   // dirty bit update required
            {
                if (!r_dcache_page_k_save) 
                {
                    r_dcache_pte_update = dcache_m_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                    r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                    r_dcache_tlb_dirty_req = true;
                    r_dcache_fsm = DCACHE_WRITE_DIRTY;
                    m_cpt_data_tlb_write_dirty++;
                }
                else
                {   
                    if (r_dcache_hit_p_save) 
                    {
                        r_dcache_pte_update = dcache_k_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                        r_dcache_tlb_paddr = (addr36_t)r_dcache_ptba_save|(addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2);
                        r_dcache_tlb_dirty_req = true;
                        r_dcache_fsm = DCACHE_WRITE_DIRTY;
                        m_cpt_data_tlb_write_dirty++;
                    }
                    else
                    {
                        r_dcache_pte_update = dcache_k_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                        r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                        r_dcache_tlb_read_req = true;
                        r_dcache_tlb_ptba_read = true;
                        r_dcache_fsm = DCACHE_TLB1_READ;
                    }
                }
            }
            else                                    // no cache update, not dirty bit update
            {
                r_dcache_fsm = DCACHE_WRITE_REQ;
                drsp.valid = true;
                drsp.rdata = 0;
            }
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB1_READ:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        if ( !r_dcache_tlb_read_req && !r_vci_rsp_data_error ) // VCI response ok
        {
            switch((r_vci_rsp_dtlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTD:                   // 4K page
                r_dcache_ptba_ok   = true;
                r_dcache_ptba_save = (addr36_t)((r_vci_rsp_dtlb_miss & PTD_PTP_MASK)>>PTD_SHIFT) << PAGE_K_NBITS;  
                r_dcache_id1_save  = dreq.addr >> PAGE_M_NBITS;
                r_dcache_tlb_paddr = (addr36_t)(((r_vci_rsp_dtlb_miss & PTD_PTP_MASK)>>PTD_SHIFT) << PAGE_K_NBITS) | 
                                     (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2);
                if ( r_dcache_tlb_ptba_read )
                {
                    r_dcache_tlb_ptba_read = false;
                    r_dcache_tlb_dirty_req = true;
                    r_dcache_fsm           = DCACHE_WRITE_DIRTY;
                    m_cpt_data_tlb_write_dirty++;
                }
                else
                {
                    r_dcache_tlb_read_req = true;
                    r_dcache_fsm          = DCACHE_TLB2_READ;
                }
                break;
            case PTE_NEW:               // 4M page (not marked)
                r_dcache_ptba_ok    = false;
                r_dcache_pte_update = r_vci_rsp_dtlb_miss | PTE_ET_MASK;  
                r_dcache_tlb_et_req = true;
                r_dcache_fsm        = DCACHE_TLB1_WRITE;
                m_cpt_data_tlb_write_et++;
                break;  
            case PTE_OLD:               // 4M page (already marked)
                r_dcache_ptba_ok    = false;
                r_dcache_pte_update = r_vci_rsp_dtlb_miss;
                r_dcache_fsm        = DCACHE_TLB1_UPDT;
                break;
            default:                    // unmapped
                r_dcache_ptba_ok    = false;
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_UNMAPPED;       
                r_dcache_bad_vaddr  = dreq.addr;
                r_dcache_fsm        = DCACHE_ERROR;
                break;
            } // end switch ET
        }

        if (!r_dcache_tlb_read_req && r_vci_rsp_data_error) // VCI response error 
        {
            r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
            r_dcache_bad_vaddr = dreq.addr;
            r_dcache_fsm = DCACHE_ERROR; 
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB1_WRITE:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        if (!r_dcache_tlb_et_req) 
        {
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;  
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;  
            } 
            else
            {
                r_dcache_fsm = DCACHE_TLB1_UPDT; 
            }
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB1_UPDT: 
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        dcache_m_tlb.update(r_dcache_pte_update,dreq.addr);
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }
    /////////////////////
    case DCACHE_TLB2_READ:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        if (!r_dcache_tlb_read_req && !r_vci_rsp_data_error) // VCI response ok
        {
            switch((r_vci_rsp_dtlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTE_NEW:               // not marked  
                r_dcache_tlb_et_req = true;
                r_dcache_pte_update = r_vci_rsp_dtlb_miss | PTE_ET_MASK;     
                r_dcache_fsm        = DCACHE_TLB2_WRITE;
                m_cpt_data_tlb_write_et++;
                break;  
            case PTE_OLD:               // already marked
                r_dcache_pte_update = r_vci_rsp_dtlb_miss;   
                r_dcache_fsm        = DCACHE_TLB2_UPDT;
                break;
            default:    
                r_dcache_error_type = r_dcache_error_type | MMU_PT2_UNMAPPED; 
                r_dcache_bad_vaddr  = dreq.addr;
                r_dcache_fsm        = DCACHE_ERROR;
                break;
            }
        }

        if (!r_dcache_tlb_read_req && r_vci_rsp_data_error)  // VCI response error
        {
            r_dcache_error_type = r_dcache_error_type | MMU_PT2_ILLEGAL_ACCESS; 
            r_dcache_bad_vaddr = dreq.addr;
            r_dcache_fsm = DCACHE_ERROR;
        }
        break;
    }
    ////////////////////////
    case DCACHE_TLB2_WRITE:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        if (!r_dcache_tlb_et_req) 
        {
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT2_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 
            } 
            else  
            {
                r_dcache_fsm = DCACHE_TLB2_UPDT; 
            }
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB2_UPDT:  
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        dcache_k_tlb.update(r_dcache_pte_update,dreq.addr);
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }
    ///////////////////////
    case DCACHE_CTXT_SWITCH:
    {
        dcache_m_tlb.flush(false);      // global entries are not invalidated   
        dcache_k_tlb.flush(false);      // global entries are not invalidated
        if ( !r_dcache_xtn_req ) 
        {
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true; 
        }
        break;
    }
    ////////////////////////
    case DCACHE_ICACHE_FLUSH:
    case DCACHE_ICACHE_INVAL:
    case DCACHE_ITLB_INVAL:
    {
        if ( !r_dcache_xtn_req ) 
        {
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true;
        }
        break;
    }
    ////////////////////////
    case DCACHE_DCACHE_FLUSH:
    {
        r_dcache.reset();
        r_dcache_fsm = DCACHE_IDLE;
        drsp.valid = true;
        break;
    }
    //////////////////////
    case DCACHE_DTLB_INVAL: 
    {
        if ( dcache_m_tlb.translate(r_dcache_wdata_save) ) 
        {
            r_dcache_page_k_save = false;
            r_dcache_fsm         = DCACHE_DTLB_INVAL_DONE;
        }
        else if ( dcache_k_tlb.translate(r_dcache_wdata_save) ) 
        {
            r_dcache_page_k_save = true;
            r_dcache_fsm         = DCACHE_DTLB_INVAL_DONE;
        }
        else 
        {
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true;
        }
        break;
    }
    ////////////////////////////
    case DCACHE_DTLB_INVAL_DONE:
    {
        if ( r_dcache_page_k_save ) 
        {
            dcache_k_tlb.inval(r_dcache_wdata_save);
        }
        else
        {
            dcache_m_tlb.inval(r_dcache_wdata_save);
        }

        r_dcache_fsm = DCACHE_IDLE;
        drsp.valid = true;
        break;
    }
    ////////////////////////
    case DCACHE_DCACHE_INVAL:
    {
        m_cpt_dcache_dir_read += m_dcache_ways;
        addr_t invadr = dreq.wdata;
        addr36_t dpaddr;
        bool dcache_hit_t_m, dcache_hit_t_k; 

        if ( r_mmu_mode == TLBS_ACTIVE || r_mmu_mode == ITLB_D_DTLB_A ) 
        {
            dcache_hit_t_m = dcache_m_tlb.translate(invadr, &dpaddr); 
            dcache_hit_t_k = dcache_k_tlb.translate(invadr, &dpaddr); 
        } 
        else 
        {
            dpaddr = invadr;  
            dcache_hit_t_m = true; 
            dcache_hit_t_k = true;
        }

        if ( dcache_hit_t_m || dcache_hit_t_k )
        {
            r_dcache_paddr_save = dpaddr;
            r_dcache_fsm = DCACHE_DCACHE_INVAL_DONE;
        }
        else
        {
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true;
        }
        break;
    }
    /////////////////////////////
    case DCACHE_DCACHE_INVAL_DONE:
        r_dcache.inval(r_dcache_paddr_save);
        r_dcache_fsm = DCACHE_IDLE;
        drsp.valid = true;
        break;

    /////////////////////
    case DCACHE_MISS_WAIT:
        if ( dreq.valid ) m_cost_data_miss_frz++; 

        if ( !r_dcache_miss_req ) 
        {
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;
            } 
            else 
            {
                r_dcache_fsm = DCACHE_MISS_UPDT; 
            } 
        } 
        break;
    /////////////////////
    case DCACHE_MISS_UPDT:
    {
        addr36_t ad = r_dcache_paddr_save;
        data_t* buf = r_dcache_miss_buf;
        addr36_t  victim_index = 0;
        if ( dreq.valid )
            m_cost_data_miss_frz++;
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        r_dcache.update(ad, buf, &victim_index);
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }
    //////////////////////
    case DCACHE_UNC_WAIT:
    {
        if ( dreq.valid ) m_cost_unc_read_frz++;

        if ( !r_dcache_unc_req ) 
        {
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;
            } 
            else 
            {
                r_dcache_fsm = DCACHE_IDLE;
                // Special case : if request was a DATA_SC, we need to invalidate 
                // the corresponding cache line, so that subsequent access to this line
                // are correctly directed to RAM
                if(dreq.type == iss_t::DATA_SC) {
                    // Simulate an invalidate request
                    r_dcache.inval(r_dcache_paddr_save);
                }
                r_dcache_buf_unc_valid = true; 
            } 
        }
        break;
    }
    ///////////////////////
    case DCACHE_WRITE_UPDT:
    {
        m_cpt_dcache_data_write++;
        data_t mask = be_to_mask(r_dcache_be_save);
        data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
        assert(r_dcache.write(r_dcache_paddr_save, wdata) && "Write on miss ignores data");
        if ( !r_dcache_dirty_save && ((r_mmu_mode == TLBS_ACTIVE)||(r_mmu_mode == ITLB_D_DTLB_A)))   
        {
            if ( r_dcache_page_k_save )
            { 
                r_dcache_tlb_paddr = (addr36_t)r_dcache_ptba_save | (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2);
                r_dcache_pte_update = dcache_k_tlb.getpte(r_dcache_tlb_way_save,r_dcache_tlb_set_save) | PTE_D_MASK;
            }
            else
            {
                r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                r_dcache_pte_update = dcache_m_tlb.getpte(r_dcache_tlb_way_save,r_dcache_tlb_set_save) | PTE_D_MASK;
            }
            r_dcache_tlb_dirty_req  = true;
            r_dcache_fsm = DCACHE_WRITE_DIRTY;
            m_cpt_data_tlb_write_dirty++;
        }
        else
        {
            r_dcache_fsm = DCACHE_WRITE_REQ;
            drsp.valid = true;
            drsp.rdata = 0;
        }
        break;
    }
    ////////////////////////
    case DCACHE_WRITE_DIRTY:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        if ( r_dcache_page_k_save ) 
        {
            dcache_k_tlb.setdirty(r_dcache_tlb_way_save, r_dcache_tlb_set_save);
        }
        else
        {
            dcache_m_tlb.setdirty(r_dcache_tlb_way_save, r_dcache_tlb_set_save);
        }

        if ( !r_dcache_tlb_dirty_req ) 
        {
            if ( r_vci_rsp_data_error )
            {
                r_dcache_error_type = r_dcache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;
            }
            else
            {
                r_dcache_fsm = DCACHE_WRITE_REQ;
                drsp.valid = true;
                drsp.rdata = 0;
            }
        }
        break;
    }
    /////////////////
    case DCACHE_ERROR:
    {
        r_vci_rsp_data_error = false;
        drsp.valid = true;
        drsp.error = true;
        drsp.rdata = 0;
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }   
    } // end switch r_dcache_fsm


#ifdef VCACHE_WRAPPER_DEBUG
std::cout << " Data Response: " << drsp << std::endl;
#endif

    /////////// execute one iss cycle /////////////////////////////////
    {
    uint32_t it = 0;
    for (size_t i=0; i<(size_t)iss_t::n_irq; i++) if(p_irq[i].read()) it |= (1<<i);
    m_iss.executeNCycles(1, irsp, drsp, it);
    }

    ////////////// number of frozen cycles //////////////////////////
    if ( (ireq.valid && !irsp.valid) || (dreq.valid && !drsp.valid) )
    {
        m_cpt_frz_cycles++;
    }

    ////////////////////////////////////////////////////////////////////////////
    //     VCI_CMD FSM 
    //
    // This FSM handles requests from both the DCACHE controler
    // (request registers) and the ICACHE controler (request registers).
    // There is 10 VCI transaction types :
    // - INS_TLB_READ
    // - INS_TLB_WRITE
    // - INS_MISS
    // - INS_UNC_MISS
    // - DATA_TLB_READ
    // - DATA_TLB_WRITE
    // - DATA_TLB_DIRTY
    // - DATA_MISS
    // - DATA_UNC 
    // - DATA_WRITE
    // The ICACHE requests have the highest priority.
    // There is at most one (CMD/RSP) VCI transaction, as both CMD_FSM and RSP_FSM
    // exit simultaneously the IDLE state.
    //////////////////////////////////////////////////////////////////////////////

    switch (r_vci_cmd_fsm) {
    
    case CMD_IDLE:
        if (r_vci_rsp_fsm != RSP_IDLE)
            break;

        r_vci_cmd_cpt = 0;

        if (r_icache_tlb_read_req)           
        {            
            r_vci_cmd_fsm = CMD_ITLB_READ;
            m_cpt_itlbmiss_transaction++; 
        } 
        else if (r_icache_tlb_et_req)      
        {  
            r_vci_cmd_fsm = CMD_ITLB_WRITE;
            m_cpt_itlb_write_transaction++; 
        } 
        else if (r_icache_miss_req) 
        {    
            r_vci_cmd_fsm = CMD_INS_MISS;
            m_cpt_imiss_transaction++; 
        }
        else if (r_icache_unc_req) 
        {    
            r_vci_cmd_fsm = CMD_INS_UNC;
            m_cpt_imiss_transaction++; 
        }  
        else if (r_dcache_tlb_read_req) 
        {            
            r_vci_cmd_fsm = CMD_DTLB_READ;
            m_cpt_dtlbmiss_transaction++; 
        } 
        else if (r_dcache_tlb_et_req) 
        {  
            r_vci_cmd_fsm = CMD_DTLB_WRITE;
            m_cpt_dtlb_write_transaction++; 
        } 
        else if (r_dcache_tlb_dirty_req) 
        {  
            r_vci_cmd_fsm = CMD_DTLB_DIRTY;
            m_cpt_dtlb_write_transaction++; 
        } 
        else if (r_dcache_write_req)
        {
            r_vci_cmd_fsm = CMD_DATA_WRITE;
            r_vci_cmd_cpt = r_wbuf.getMin();
            r_vci_cmd_min = r_wbuf.getMin();
            r_vci_cmd_max = r_wbuf.getMax(); 
            m_cpt_write_transaction++; 
            m_length_write_transaction += (r_wbuf.getMax() - r_wbuf.getMin() + 1);
        }
        else if (r_dcache_miss_req)  
        {
            r_vci_cmd_fsm = CMD_DATA_MISS;
            m_cpt_dmiss_transaction++; 
        }
        else if (r_dcache_unc_req)  
        {
            r_vci_cmd_fsm = CMD_DATA_UNC;
            m_cpt_unc_transaction++; 
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci.cmdack.read() ) 
        {
            r_vci_cmd_cpt = r_vci_cmd_cpt + 1;
            if (r_vci_cmd_cpt == r_vci_cmd_max) 
            {
                r_vci_cmd_fsm = CMD_IDLE;
                r_wbuf.reset();
            }
        }
        break;

    default:
        if ( p_vci.cmdack.read() )
        {  
            r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    } // end  switch r_vci_cmd_fsm

    //////////////////////////////////////////////////////////////////////////
    //      VCI_RSP FSM 
    //
    // This FSM is synchronized with the VCI_CMD FSM, as both FSMs exit the
    // IDLE state simultaneously.
    //////////////////////////////////////////////////////////////////////////

    switch (r_vci_rsp_fsm) {

    case RSP_IDLE:
        assert( ! p_vci.rspval.read() && "Unexpected response" );

        if (r_vci_cmd_fsm != CMD_IDLE)
            break;

        r_vci_rsp_cpt = 0;
        if (r_icache_tlb_read_req)          // ITLB miss response
        {            
            r_vci_rsp_fsm = RSP_ITLB_READ;
        } 
        else if (r_icache_tlb_et_req)       // ITLB linked load response
        {   
            r_vci_rsp_fsm = RSP_ITLB_WRITE;
        } 
        else if (r_icache_miss_req)         // ICACHE cached miss response
        {   
            r_vci_rsp_fsm = RSP_INS_MISS;
        }
        else if (r_icache_unc_req)          // ICACHE uncached miss response
        {   
            r_vci_rsp_fsm = RSP_INS_UNC;
        }  
        else if (r_dcache_tlb_read_req)     // ITLB miss response
        {
            r_vci_rsp_fsm = RSP_DTLB_READ; 
        }
        else if (r_dcache_tlb_et_req)       // ITLB linked load response
        {
            r_vci_rsp_fsm = RSP_DTLB_WRITE; 
        }
        else if (r_dcache_tlb_dirty_req)    // ITLB store conditional response
        {
            r_vci_rsp_fsm = RSP_DTLB_DIRTY; 
        }
        else if (r_dcache_write_req)        // DCACHE write request
        {
            r_vci_rsp_fsm = RSP_DATA_WRITE;
        }
        else if (r_dcache_miss_req)         // DCACHE read request
        {
            r_vci_rsp_fsm = RSP_DATA_MISS;
        }
        else if (r_dcache_unc_req)          // DCACHE uncached read request
        {
            r_vci_rsp_fsm = RSP_DATA_UNC;
        }
        break;

    case RSP_ITLB_READ:
        m_cost_itlbmiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( !p_vci.rerror.read() ) 
        {
            r_vci_rsp_itlb_miss = (int)p_vci.rdata.read();
        } 
        else 
        {
            r_vci_rsp_ins_error = true;
        }
        r_icache_tlb_read_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_ITLB_WRITE:
        m_cost_itlb_write_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for write tlb");

        if ( p_vci.rerror.read() ) 
        {
            r_vci_rsp_ins_error = true;
        }
        r_icache_tlb_et_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_INS_MISS:
        m_cost_imiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert( (r_vci_rsp_cpt < m_icache_words) && 
               "The VCI response packet for instruction miss is too long");
        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_icache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci.rdata.read();

        if ( p_vci.reop.read() ) 
        {
            assert( (r_vci_rsp_cpt == m_icache_words - 1) &&
                       "The VCI response packet for instruction miss is too short");
            r_icache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
                
        } 
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_ins_error = true;
        }
        break;

    case RSP_INS_UNC:
        m_cost_imiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for uncached instruction");

        r_icache_miss_buf[0] = (data_t)p_vci.rdata.read();
        r_icache_buf_unc_valid = true;
        r_icache_unc_req = false;
        r_vci_rsp_fsm = RSP_IDLE;

        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_ins_error = true;
        }
        break;

    case RSP_DTLB_READ:
        m_cost_dtlbmiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( !p_vci.rerror.read() ) 
        {
            r_vci_rsp_dtlb_miss = (int)p_vci.rdata.read();
        }
        else 
        {
            r_vci_rsp_data_error = true;
        }
        r_dcache_tlb_read_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DTLB_WRITE:
        m_cost_dtlb_write_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for write tlb");

        if ( p_vci.rerror.read() ) 
        {   
            r_vci_rsp_data_error = true;
        }
        r_dcache_tlb_et_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DTLB_DIRTY:
        m_cost_dtlb_write_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for write tlb");

        if ( p_vci.rerror.read() ) 
        {   
            r_vci_rsp_data_error = true;
        }
        r_dcache_tlb_dirty_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_UNC:
        m_cost_unc_transaction++;
        if ( ! p_vci.rspval.read() ) 
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
        else
        {
            r_dcache_miss_buf[0] = (data_t)p_vci.rdata.read();
            r_dcache_buf_unc_valid = true;
        }
        r_dcache_unc_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_MISS:
        m_cost_dmiss_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci.rdata.read();
        if ( p_vci.reop.read() ) 
        {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                    "illegal VCI response packet for data read miss");
            r_dcache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        } 
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_data_error = true;
        }
        break;

    case RSP_DATA_WRITE:
        m_cost_write_transaction++;
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() ) 
        {
            r_vci_rsp_fsm = RSP_IDLE;
            r_dcache_write_req = false;
        }
        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            m_iss.setWriteBerr();
        }
        break;

    } // end switch r_vci_rsp_fsm
} // end transition()

///////////////////////
tmpl(void)::genMoore()
///////////////////////
{
    // VCI initiator response

    p_vci.rspack = true;

    // VCI initiator command

    p_vci.trdid  = 0;
    p_vci.pktid  = 0;
    p_vci.srcid  = m_srcid;
    p_vci.cons   = false;
    p_vci.wrap   = false;
    p_vci.contig = true;
    p_vci.clen   = 0;
    p_vci.cfixed = false;

    switch (r_vci_cmd_fsm) {

    case CMD_IDLE:
        p_vci.cmdval  = false;
        p_vci.address = 0;
        p_vci.wdata   = 0;
        p_vci.be      = 0;
        p_vci.plen    = 0;
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.eop     = false;
        break;

    case CMD_ITLB_READ:     
        p_vci.cmdval  = true;
        p_vci.address = r_icache_paddr_save.read() & ~0x3;
        p_vci.wdata   = 0;
        p_vci.be      = 0xF;
        p_vci.plen    = 4;
        p_vci.cmd     = vci_param::CMD_READ;
        p_vci.eop     = true;
        break;

    case CMD_ITLB_WRITE: 
        p_vci.cmdval  = true;
        p_vci.address = r_icache_paddr_save.read() & ~0x3;
        p_vci.wdata   = r_icache_pte_update.read();
        p_vci.be      = 0x8;
        p_vci.plen    = 4;
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.eop     = true;
        break;

    case CMD_INS_MISS:
        p_vci.cmdval  = true;
        p_vci.address = r_icache_paddr_save.read() & m_icache_yzmask;
        p_vci.wdata   = 0;
        p_vci.be      = 0xF;
        p_vci.plen    = m_icache_words << 2;
        p_vci.cmd     = vci_param::CMD_READ;
        p_vci.eop     = true;
        break;

    case CMD_INS_UNC:
        p_vci.cmdval  = true;
        p_vci.address = r_icache_paddr_save.read() & ~0x3;
        p_vci.wdata   = 0;
        p_vci.be      = 0xF;
        p_vci.plen    = 4;
        p_vci.cmd     = vci_param::CMD_READ;
        p_vci.eop     = true;
        break;

    case CMD_DTLB_READ:     
        p_vci.cmdval  = true;
        p_vci.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci.wdata   = 0;
        p_vci.be      = 0xF;
        p_vci.plen    = 4;
        p_vci.cmd     = vci_param::CMD_READ;
        p_vci.eop     = true;
        break;

    case CMD_DTLB_WRITE:     
        p_vci.cmdval  = true;
        p_vci.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci.wdata   = r_dcache_pte_update.read();
        p_vci.be      = 0x8;
        p_vci.plen    = 4;
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.eop     = true;
        break;

    case CMD_DTLB_DIRTY:     
        p_vci.cmdval  = true;
        p_vci.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci.wdata   = r_dcache_pte_update.read();
        p_vci.be      = 0x1;
        p_vci.plen    = 4;
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.eop     = true;
        break;

    case CMD_DATA_UNC:
        p_vci.cmdval  = true;
        p_vci.address = r_dcache_paddr_save.read() & ~0x3;
        p_vci.plen    = 4;
        p_vci.eop     = true;
        switch(r_dcache_type_save) {
        case iss_t::DATA_READ:
            p_vci.wdata = 0;
            p_vci.be    = r_dcache_be_save.read();
            p_vci.cmd   = vci_param::CMD_READ;
            break;
        case iss_t::DATA_LL:
            p_vci.wdata = 0;
            p_vci.be    = 0xF;
            p_vci.cmd   = vci_param::CMD_LOCKED_READ;
            break;
        case iss_t::DATA_SC:
            p_vci.wdata = r_dcache_wdata_save.read();
            p_vci.be    = 0xF;
            p_vci.cmd   = vci_param::CMD_STORE_COND;
            break;
        default:
            assert("this should not happen");
        }
        break;

    case CMD_DATA_WRITE:
        p_vci.cmdval  = true;
        p_vci.address = r_wbuf.getAddress(r_vci_cmd_cpt);
        p_vci.wdata   = r_wbuf.getData(r_vci_cmd_cpt);
        p_vci.be      = r_wbuf.getBe(r_vci_cmd_cpt);
        p_vci.plen    = (r_vci_cmd_max - r_vci_cmd_min + 1)<<2;
        p_vci.cmd     = vci_param::CMD_WRITE;
        p_vci.eop     = (r_vci_cmd_cpt == r_vci_cmd_max);
        break;

    case CMD_DATA_MISS:
        p_vci.cmdval  = true;
        p_vci.address = r_dcache_paddr_save.read() & m_dcache_yzmask;
        p_vci.wdata   = 0;
        p_vci.be      = 0xF;
        p_vci.plen    = m_dcache_words << 2;
        p_vci.cmd     = vci_param::CMD_READ;
        p_vci.eop     = true;
        break;

    } // end switch r_vci_cmd_fsm

#ifdef VCACHE_WRAPPER_DEBUG 
   std::cout 
       << "Moore:" << std::hex
       << "p_vci.cmdval:" << p_vci.cmdval
       << "p_vci.address:" << p_vci.address
       << "p_vci.wdata:" << p_vci.wdata
       << "p_vci.cmd:" << p_vci.cmd
       << "p_vci.eop:" << p_vci.eop
       << std::endl;
#endif
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4


