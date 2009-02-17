/* -*- c++ -*-
 * File : vci_cc_vcache_wrapper2.cpp
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
#include "../include/vci_cc_vcache_wrapper2.h"

namespace soclib { 
namespace caba {

//#define CC_VCACHE_WRAPPER_DEBUG

#ifdef CC_VCACHE_WRAPPER_DEBUG
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
        "ICACHE_SW_FLUSH1", 
        "ICACHE_SW_FLUSH2", 
        "ICACHE_CACHE_FLUSH", 
        "ICACHE_TLB_INVAL",  
        "ICACHE_TLB_INVAL_DONE",  
        "ICACHE_CACHE_INVAL",
        "ICACHE_CACHE_INVAL_DONE",
        "ICACHE_MISS_WAIT",
        "ICACHE_UNC_WAIT",  
        "ICACHE_MISS_UPDT",  
        "ICACHE_ERROR", 	
        "ICACHE_CC_INVAL", 
        "ICACHE_TLB_CC_INVAL",        
        "ICACHE_TLB_FLUSH", 
    };
const char *dcache_fsm_state_str[] = {
        "DCACHE_IDLE",       
        "DCACHE_BIS",   
        "DCACHE_DTLB1_READ_CACHE",    
        "DCACHE_TLB1_READ",
        "DCACHE_TLB1_READ_UPDT",  
        "DCACHE_TLB1_WRITE", 
        "DCACHE_TLB1_UPDT", 
        "DCACHE_DTLB2_READ_CACHE",  
        "DCACHE_TLB2_READ",
        "DCACHE_TLB2_READ_UPDT",  
        "DCACHE_TLB2_WRITE", 
        "DCACHE_TLB2_UPDT",   
        "DCACHE_CTXT_SWITCH1",   
        "DCACHE_CTXT_SWITCH2",   
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
        "DCACHE_ITLB_READ",
        "DCACHE_ITLB_UPDT",
        "DCACHE_ITLB_ET_WRITE",
        "DCACHE_CC_CHECK",
        "DCACHE_CC_INVAL",
        "DCACHE_CC_UPDT",
        "DCACHE_CC_NOP",
        "DCACHE_TLB_CC_INVAL_WAIT",
        "DCACHE_TLB_CC_INVAL",
        "DCACHE_ITLB_CLEANUP",
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
        "CMD_INS_CLEANUP",    
        "CMD_DATA_CLEANUP",      
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
        "RSP_INS_CLEANUP",    
        "RSP_DATA_CLEANUP",        
    };
const char *tgt_fsm_state_str[] = {
        "TGT_IDLE",
        "TGT_UPDT_WORD",
        "TGT_UPDT_DATA",
        "TGT_REQ",
        "TGT_RSP",
    };	
const char *inval_itlb_fsm_state_str[] = {
        "INVAL_ITLB_IDLE",        
        "INVAL_ITLB_CHECK_FIRST"  , 
        "INVAL_ITLB_REQ_WAIT",      
        "INVAL_ITLB_CHECK",         
        "INVAL_ITLB_RSP",           
    };
const char *inval_dtlb_fsm_state_str[] = {
        "INVAL_DTLB_IDLE",        
        "INVAL_DTLB_CHECK_FIRST", 
        "INVAL_DTLB_REQ_WAIT",    
        "INVAL_DTLB_CHECK",       
        "INVAL_DTLB_RSP",         
    };
}
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciCcVCacheWrapper2<vci_param, iss_t>

tmpl(inline typename VciCcVCacheWrapper2<vci_param, iss_t>::data_t)::be_to_mask( typename iss_t::be_t be )
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
tmpl(/**/)::VciCcVCacheWrapper2(
    sc_module_name name,
    int proc_id,
    const soclib::common::MappingTable &mtp,
    const soclib::common::MappingTable &mtc,
    const soclib::common::IntTab &initiator_index,
    const soclib::common::IntTab &target_index,
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
    size_t dcache_words,
    addr_t cleanup_offset )
/***********************************************/
    : soclib::caba::BaseModule(name),

      p_clk("clk"),
      p_resetn("resetn"),
      p_vci_ini("vci_ini"),
      p_vci_tgt("vci_tgt"),

      m_cacheability_table(mtp.getCacheabilityTable()),
      m_segment(mtc.getSegment(target_index)),
      m_iss(this->name(), proc_id),
      m_srcid(mtp.indexForId(initiator_index)),
      m_cleanup_address(cleanup_offset),

      m_itlb_m_ways(itlb_m_ways),
      m_itlb_m_sets(itlb_m_sets),
      m_itlb_k_ways(itlb_k_ways),
      m_itlb_k_sets(itlb_k_sets),

      m_dtlb_m_ways(dtlb_m_ways),
      m_dtlb_m_sets(dtlb_m_sets),
      m_dtlb_k_ways(dtlb_k_ways),
      m_dtlb_k_sets(dtlb_k_sets),

      m_icache_ways(icache_ways),
      m_icache_sets(icache_sets),
      m_icache_yzmask((~0)<<(uint32_log2(icache_words) + 2)),
      m_icache_words(icache_words),

      m_dcache_ways(dcache_ways),
      m_dcache_sets(dcache_sets),
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

      r_dcache_fsm_save("r_dcache_fsm_save"),
      r_dcache_cleanup_req("r_dcache_cleanup_req"),
      r_dcache_inval_rsp("r_dcache_inval_rsp"),

      r_icache_fsm("r_icache_fsm"),
      r_icache_paddr_save("r_icache_paddr_save"),
      r_icache_miss_req("r_icache_miss_req"),
      r_icache_unc_req("r_icache_unc_req"),
      r_dcache_itlb_read_req("r_dcache_itlb_read_req"),
      r_dcache_itlb_et_req("r_dcache_itlb_et_req"),

      r_icache_tlb_read_dcache_req("r_icache_tlb_read_dcache_req"),
      r_icache_tlb_et_dcache_req("r_icache_tlb_et_dcache_req"),
      r_dcache_rsp_itlb_error("r_dcache_rsp_itlb_error"),

      r_icache_fsm_save("r_icache_fsm_save"),
      r_icache_cleanup_req("r_icache_cleanup_req"),
      r_icache_inval_rsp("r_icache_inval_rsp"),

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

      r_vci_tgt_fsm("r_vci_tgt_fsm"),
      r_tgt_addr("r_tgt_addr"),
      r_tgt_word("r_tgt_word"),
      r_tgt_update("r_tgt_update"),
      r_tgt_srcid("r_tgt_srcid"),
      r_tgt_pktid("r_tgt_pktid"),
      r_tgt_trdid("r_tgt_trdid"),
      r_tgt_icache_req("r_tgt_icache_req"),
      r_tgt_dcache_req("r_tgt_dcache_req"),

      r_inval_itlb_fsm("r_inval_itlb_fsm"),          
      r_dcache_itlb_ccinval_check_req("r_dcache_itlb_ccinval_check_req"),
      r_dcache_itlb_ccinval_check_line("r_dcache_itlb_ccinval_check_line"),
      r_dcache_itlb_ccinval_check_line_save("r_dcache_itlb_ccinval_check_line_save"),
      r_ccinval_k_itlb_req("r_ccinval_k_itlb_req"),
      r_ccinval_m_itlb_req("r_ccinval_m_itlb_req"),
      r_itlb_cc_check_end("r_itlb_cc_check_end"),
      r_ccinval_itlb_k("r_ccinval_itlb_k"), 
      r_ccinval_itlb_way("r_ccinval_itlb_way"), 
      r_ccinval_itlb_set("r_ccinval_itlb_set"), 
      r_icache_inval_tlb_rsp("r_icache_inval_tlb_rsp"),
      r_icache_tlb_nline("r_icache_tlb_nline"),

      r_inval_dtlb_fsm("r_inval_dtlb_fsm"),          
      r_dcache_dtlb_ccinval_check_req("r_dcache_dtlb_ccinval_check_req"),
      r_dcache_dtlb_ccinval_check_line("r_dcache_dtlb_ccinval_check_line"),
      r_dcache_dtlb_ccinval_check_line_save("r_dcache_dtlb_ccinval_check_line_save"),
      r_ccinval_k_dtlb_req("r_ccinval_k_dtlb_req"),
      r_ccinval_m_dtlb_req("r_ccinval_m_dtlb_req"),
      r_dtlb_cc_check_end("r_dtlb_cc_check_end"),
      r_ccinval_dtlb_k("r_ccinval_dtlb_k"), 
      r_ccinval_dtlb_way("r_ccinval_dtlb_way"), 
      r_ccinval_dtlb_set("r_ccinval_dtlb_set"), 
      r_dcache_inval_tlb_rsp("r_dcache_inval_tlb_rsp"),
      r_dcache_tlb_nline("r_dcache_tlb_nline"),

      r_dcache_itlb_ccinval_check_wait("r_dcache_itlb_ccinval_check_wait"),
      r_dcache_dtlb_ccinval_check_wait("r_dcache_dtlb_ccinval_check_wait"),

      r_dcache_itlb_cleanup_req("r_dcache_itlb_cleanup_req"),
      r_dcache_itlb_cleanup_line("r_dcache_itlb_cleanup_line"),
      r_dcache_itlb_cleanup_line_save("r_dcache_itlb_cleanup_line_save"),

      r_dcache_dtlb_cleanup_req("r_dcache_dtlb_cleanup_req"),
      r_dcache_dtlb_cleanup_line("r_dcache_dtlb_cleanup_line"),
      r_dcache_dtlb_cleanup_line_save("r_dcache_dtlb_cleanup_line_save"),

      r_wbuf("wbuf", dcache_words ),
      r_icache("icache", icache_ways, icache_sets, icache_words),
      r_dcache("dcache", dcache_ways, dcache_sets, dcache_words)
{
    r_icache_miss_buf = new data_t[icache_words];
    r_dcache_miss_buf = new data_t[dcache_words];
    r_tgt_buf         = new data_t[dcache_words];
    r_tgt_val         = new bool[dcache_words];
    r_dcache_in_itlb  = new bool[dcache_ways*dcache_sets];           
    r_dcache_in_dtlb  = new bool[dcache_ways*dcache_sets];          

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
tmpl(/**/)::~VciCcVCacheWrapper2()
/////////////////////////////////////
{
    delete [] r_icache_miss_buf;
    delete [] r_dcache_miss_buf;
    delete [] r_tgt_val;
    delete [] r_tgt_buf;
    delete [] r_dcache_in_itlb;           
    delete [] r_dcache_in_dtlb;          
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
        r_vci_tgt_fsm = TGT_IDLE;
        r_inval_itlb_fsm = INVAL_ITLB_IDLE;          
        r_inval_dtlb_fsm = INVAL_DTLB_IDLE;          

        // write buffer & caches
        r_wbuf.reset();
        r_icache.reset();
        r_dcache.reset();

        icache_m_tlb.reset();    
        icache_k_tlb.reset();    
        dcache_m_tlb.reset();    
        dcache_k_tlb.reset();   

        std::memset(r_dcache_in_itlb, 0, sizeof(*r_dcache_in_itlb)*m_icache_ways*m_icache_sets);
        std::memset(r_dcache_in_dtlb, 0, sizeof(*r_dcache_in_dtlb)*m_dcache_ways*m_dcache_sets);

        r_mmu_mode = TLBS_DEACTIVE;

        r_icache_miss_req        = false;
        r_icache_unc_req         = false;
        r_dcache_itlb_read_req   = false;
        r_dcache_itlb_et_req     = false;

        r_icache_tlb_read_dcache_req = false;      
        r_icache_tlb_et_dcache_req   = false;   
        r_dcache_rsp_itlb_error      = false;
 
        r_dcache_miss_req        = false;
        r_dcache_unc_req         = false;
        r_dcache_write_req       = false;
        r_dcache_tlb_read_req    = false;
        r_dcache_tlb_et_req      = false;
        r_dcache_tlb_dirty_req   = false;
        r_dcache_tlb_ptba_read   = false;
        r_dcache_xtn_req         = false;

        r_icache_cleanup_req     = false;
        r_dcache_cleanup_req     = false;

        r_tgt_icache_req         = false;
        r_tgt_dcache_req         = false;

        r_icache_inval_rsp       = false;
        r_dcache_inval_rsp       = false;

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

        // coherence registers
        r_icache_way                     = 0;
        r_icache_set                     = 0;
        r_icache_cleanup_req             = false;
        r_icache_inval_rsp               = false;

        r_dcache_way                     = 0;
        r_dcache_set                     = 0;
        r_dcache_cleanup_req             = false;
        r_dcache_inval_rsp               = false;

        r_dcache_itlb_ccinval_check_req  = false;
        r_ccinval_k_itlb_req             = false;
        r_ccinval_m_itlb_req             = false;
        r_itlb_cc_check_end              = false;
        r_ccinval_itlb_way               = 0; 
        r_ccinval_itlb_set               = 0; 
        r_icache_inval_tlb_rsp           = false;

        r_dcache_dtlb_ccinval_check_req  = false;
        r_ccinval_k_dtlb_req             = false;
        r_ccinval_m_dtlb_req             = false;
        r_dtlb_cc_check_end              = false;
        r_ccinval_dtlb_way               = 0; 
        r_ccinval_dtlb_set               = 0; 
        r_dcache_inval_tlb_rsp           = false;

        r_dcache_itlb_ccinval_check_wait = false;
        r_dcache_dtlb_ccinval_check_wait = false;

        r_dcache_itlb_cleanup_req        = false;
        r_dcache_dtlb_cleanup_req        = false;

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

#ifdef CC_VCACHE_WRAPPER_DEBUG
std::cout << "cycle = " << m_cpt_total_cycles << " processor " << name() 
        << " tgt fsm: " << tgt_fsm_state_str[r_vci_tgt_fsm]
        << " dcache fsm: " << dcache_fsm_state_str[r_dcache_fsm]
        << " icache fsm: " << icache_fsm_state_str[r_icache_fsm]
        << " cmd fsm: " << cmd_fsm_state_str[r_vci_cmd_fsm]
        << " rsp fsm: " << rsp_fsm_state_str[r_vci_rsp_fsm] 
        << " inval itlb fsm: " << inval_itlb_fsm_state_str[r_inval_itlb_fsm] 
        << " inval dtlb fsm: " << inval_dtlb_fsm_state_str[r_inval_dtlb_fsm] << std::endl;
#endif

    m_cpt_total_cycles++;

    typename iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

    typename iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;

    m_iss.getRequests( ireq, dreq );

#ifdef CC_VCACHE_WRAPPER_DEBUG
std::cout << name() << " Instruction Request: " << ireq << std::endl;
std::cout << name() << " Data Request: " << dreq << std::endl;
#endif

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
            addr36_t address = p_vci_tgt.address.read();
            addr36_t cell    = address - m_segment.baseAddress();
            if ( ! m_segment.contains(address) ) 
            {
                std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                std::cout << "out of segment VCI command received" << std::endl;
                exit(0);
            }
            if ( p_vci_tgt.cmd.read() != vci_param::CMD_WRITE) 
            {
                std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the received VCI command is not a write" << std::endl;
                exit(0);
            }
            r_tgt_srcid = p_vci_tgt.srcid.read();
            r_tgt_trdid = p_vci_tgt.trdid.read();
            r_tgt_pktid = p_vci_tgt.pktid.read();
            r_tgt_addr = (addr36_t)p_vci_tgt.wdata.read() * m_dcache_words * 4; 

            if (cell == 0)                      // invalidate   
            {                         
                if ( ! p_vci_tgt.eop.read() ) 
                {
                    std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                    std::cout << "the INVALIDATE command length must be one word" << std::endl;
                    exit(0);
                }
                r_tgt_update = false; 
                r_vci_tgt_fsm = TGT_REQ;
                m_cpt_cc_inval++ ;
            } 
            else                                // update
            {                                
                if ( p_vci_tgt.eop.read() ) 
                {
                    std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                    std::cout << "the UPDATE command length must be N+2 words" << std::endl;
                    exit(0);
                }
                r_tgt_update = true; 
                r_vci_tgt_fsm = TGT_UPDT_WORD;
                m_cpt_cc_update++ ;
            }
        } // end if cmdval
        break;

    case TGT_UPDT_WORD:
        if (p_vci_tgt.cmdval.read()) 
        {
            if ( p_vci_tgt.eop.read() ) 
            {
                std::cout << "error in component VCI_CC_XCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the UPDATE command length must be N+2 words" << std::endl;
                exit(0);
            }
            for ( size_t i=0 ; i<m_dcache_words ; i++ ) r_tgt_val[i] = false;
            r_tgt_word = p_vci_tgt.wdata.read();
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
                std::cout << "the reveived UPDATE command length is wrong" << std::endl;
                exit(0);
            }
            r_tgt_buf[word] = p_vci_tgt.wdata.read();
            if(p_vci_tgt.be.read())    r_tgt_val[word] = true;
            r_tgt_word = word + 1;
            if (p_vci_tgt.eop.read())  r_vci_tgt_fsm = TGT_REQ;
        }
        break;

    case TGT_REQ:
        if ( !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_RSP; 
            r_tgt_icache_req = true;
            r_tgt_dcache_req = true;
        }
        break;

    case TGT_RSP:
        if ( p_vci_tgt.rspack.read() && !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_IDLE; 
        }
        break;

    } // end switch TGT_FSM

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
    // and sets the corresponding request flip-flop : r_dcache_itlb_read_req, r_icache_miss_req 
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
        size_t      icache_tlb_nline = 0;      // TLB NLINE 

        // Decoding processor XTN requests
        // They are sent by DCACHE FSM  

        if (r_dcache_xtn_req)
        {
            if ((int)r_dcache_type_save == (int)iss_t::XTN_PTPR )  
            {
                r_icache_way = 0;
                r_icache_set = 0;
                r_icache_fsm = ICACHE_SW_FLUSH1;   
                break;
            }
            if ((int)r_dcache_type_save == (int)iss_t::XTN_ICACHE_FLUSH)
            {
                r_icache_way = 0;
                r_icache_set = 0;
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
            if ((int)r_dcache_type_save == (int)iss_t::XTN_DCACHE_FLUSH )  
            {
                // special for ins tlb miss via data cache
                r_icache_fsm = ICACHE_TLB_FLUSH;   
                break;
            }
        } // end if xtn_req

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            if ( ireq.valid ) m_cost_ins_miss_frz++;
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            if ( ireq.valid ) m_cost_ins_miss_frz++;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

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
            icache_hit_t_m = icache_m_tlb.cctranslate(ireq.addr, &tlb_ipaddr, &icache_pte_info, 
                                                    &icache_tlb_nline, 
                                                    &icache_tlb_way, &icache_tlb_set); 
            icache_hit_t_k = icache_k_tlb.cctranslate(ireq.addr, &tlb_ipaddr, &icache_pte_info, 
                                                    &icache_tlb_nline, 
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
                    if (r_dcache_itlb_ccinval_check_req && (r_dcache_itlb_ccinval_check_line == icache_tlb_nline))
                    {
                        break;
                    }

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
                    r_icache_tlb_nline = icache_tlb_nline;
                    r_itlb_translation_valid = true;
                    r_icache_page_k_save = false;
                }
                else if ( icache_hit_t_k )
                {
                    icache_k_tlb.setlru(icache_tlb_way,icache_tlb_set); 
                    r_icache_ppn_save = tlb_ipaddr >> PAGE_K_NBITS;
                    r_icache_vpn_save = ireq.addr >> PAGE_K_NBITS;
                    r_icache_tlb_nline = icache_tlb_nline;
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
                r_icache_tlb_read_dcache_req = true;
                r_icache_fsm = ICACHE_TLB1_READ;
                m_cpt_ins_tlb_miss++;
                m_cost_ins_tlb_miss_frz++;
            }
            else if ( !icache_hit_t_m && !icache_hit_t_k && icache_hit_p )  // TLB Miss with possibility of bypass first level page
            {
                // walk page table level 2
                r_icache_paddr_save = (addr36_t)r_icache_ptba_save | 
                                      (addr36_t)(((ireq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2);
                r_icache_tlb_read_dcache_req = true;
                r_icache_fsm = ICACHE_TLB2_READ;
                m_cpt_ins_tlb_miss++;
                m_cost_ins_tlb_miss_frz++;
            }
            else if ( (icache_hit_t_m || icache_hit_t_k) && !icache_hit_x && icache_cached ) // cached access with an ucorrect speculative physical address
            {
                r_icache_paddr_save = tlb_ipaddr;   // save actual physical address for BIS
                r_icache_fsm = ICACHE_BIS;
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
        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            if ( ireq.valid ) m_cost_ins_miss_frz++;
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }
        
        if ( r_icache_inval_tlb_rsp )
        {
            r_icache_inval_tlb_rsp = false;
            r_icache_fsm = ICACHE_IDLE;
            break;
        }

        data_t  icache_ins = 0;
        bool    icache_hit = false;

        // acces is always cached in this state
        icache_hit = r_icache.read(r_icache_paddr_save, &icache_ins);
        if ( r_icache_inval_rsp )   // only used for cc_ins_inval being broadcast
        {
            icache_hit = false;
            r_icache_inval_rsp = false;
        } 

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
        if ( ireq.valid ) m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        if ( !r_icache_tlb_read_dcache_req && !r_icache_inval_tlb_rsp ) // TLB miss read response and no invalidation
        {
            if ( !r_dcache_rsp_itlb_error ) // vci response ok
            {  
                switch((r_dcache_rsp_itlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) { 
                case PTD:               // 4K page TLB
            	    r_icache_ptba_ok    = true;	
                    r_icache_ptba_save  = (addr36_t)(r_dcache_rsp_itlb_miss & PTD_PTP_MASK) << PAGE_K_NBITS; 
                    r_icache_id1_save   = ireq.addr >> PAGE_M_NBITS;
                    r_icache_paddr_save = (addr36_t)(r_dcache_rsp_itlb_miss & PTD_PTP_MASK) << PAGE_K_NBITS |
                                            (addr36_t)(((ireq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2); 
                    r_icache_tlb_read_dcache_req = true;
                    r_icache_fsm        = ICACHE_TLB2_READ;
                    break;
                case PTE_NEW:           // 4M page TLB (not marked)   
            	    r_icache_ptba_ok    = false;	
                    r_icache_pte_update = r_dcache_rsp_itlb_miss | PTE_ET_MASK;
                    r_icache_tlb_et_dcache_req = true;
                    r_icache_fsm        = ICACHE_TLB1_WRITE;
                    m_cpt_ins_tlb_write_et++;
                    break; 
                case PTE_OLD:           // 4M page TLB (already marked)
            	    r_icache_ptba_ok    = false;	
                    r_icache_pte_update = r_dcache_rsp_itlb_miss;
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
            else                        // vci response error
            {  
                r_icache_fsm = ICACHE_ERROR;
                r_icache_error_type = r_icache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
                r_icache_bad_vaddr = ireq.addr;
            }
        }

        if ( !r_icache_tlb_read_dcache_req && r_icache_inval_tlb_rsp ) // TLB miss read response and invalidation
        {
            if ( r_dcache_rsp_itlb_error ) 
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_error_type = r_icache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else 
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_fsm = ICACHE_IDLE;  
            } 
        }
        break;
    }
    ///////////////////////
    case ICACHE_TLB1_WRITE:  
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;
        if ( ireq.valid ) m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }
        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        if (!r_icache_tlb_et_dcache_req && !r_icache_inval_tlb_rsp) // TLB ET write response and no invalidation        
        { 
            if ( r_dcache_rsp_itlb_error ) 
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
        
        if (!r_icache_tlb_et_dcache_req && r_icache_inval_tlb_rsp) // TLB ET write response and invalidation     
        {   
            if ( r_dcache_rsp_itlb_error ) 
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_error_type = r_icache_error_type | MMU_PT1_ILLEGAL_ACCESS;  
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else  
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_fsm = ICACHE_IDLE;    
            }
        }
        break;
    }
    //////////////////////
    case ICACHE_TLB1_UPDT:
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;
        if ( ireq.valid ) m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req ) 
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // TLB update and invalidate different PTE
        if ( !r_icache_inval_tlb_rsp && !r_dcache_itlb_cleanup_req )  
        {
            size_t victim_index = 0;
            r_dcache_itlb_cleanup_req = icache_m_tlb.update(r_icache_pte_update,ireq.addr,(r_icache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index);
            r_dcache_itlb_cleanup_line = victim_index;
            r_icache_fsm = ICACHE_IDLE;
        }

        // TLB update and invalidate same PTE
        if ( r_icache_inval_tlb_rsp )                                 
        {
            r_icache_inval_tlb_rsp = false;
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
    }
    /////////////////////
    case ICACHE_TLB2_READ:
    {
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;
        if ( ireq.valid ) m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        if ( !r_icache_tlb_read_dcache_req && !r_icache_inval_tlb_rsp ) // TLB miss read response and no invalidation
        {
            if ( !r_dcache_rsp_itlb_error ) // VCI response ok        
            {
                switch((r_dcache_rsp_itlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
                case PTE_NEW:               // not marked
                    r_icache_pte_update = r_dcache_rsp_itlb_miss | PTE_ET_MASK;
                    r_icache_tlb_et_dcache_req = true;
                    r_icache_fsm        = ICACHE_TLB2_WRITE;
                    m_cpt_ins_tlb_write_et++;
                    break;  
                case PTE_OLD:               // already marked
                    r_icache_fsm        = ICACHE_TLB2_UPDT;
                    r_icache_pte_update = r_dcache_rsp_itlb_miss;
                    break;
                default:                    // unmapped
                    r_icache_error_type = r_icache_error_type | MMU_PT2_UNMAPPED;  
                    r_icache_bad_vaddr  = ireq.addr;
                    r_icache_fsm = ICACHE_ERROR;
                    break;
                }
            }
            else                            // VCI response error        
            {
                r_icache_error_type = r_icache_error_type | MMU_PT2_ILLEGAL_ACCESS;
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            }
        }
        if ( !r_icache_tlb_read_dcache_req && r_icache_inval_tlb_rsp ) // TLB miss read response and invalidation
        {
            if ( r_dcache_rsp_itlb_error ) 
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_error_type = r_icache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else 
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_fsm = ICACHE_IDLE;  
            } 
        }
        break;
    }
    /////////////////////////
    case ICACHE_TLB2_WRITE:
    {  
        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;
        if ( ireq.valid ) m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }
        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        if (!r_icache_tlb_et_dcache_req && !r_icache_inval_tlb_rsp) // TLB ET write response and no invalidation         
        {
            if ( r_dcache_rsp_itlb_error )             
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

        if (!r_icache_tlb_et_dcache_req && r_icache_inval_tlb_rsp) // TLB ET write response and invalidation     
        {   
            if ( r_dcache_rsp_itlb_error ) 
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_error_type = r_icache_error_type | MMU_PT1_ILLEGAL_ACCESS;  
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else  
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_fsm = ICACHE_IDLE;    
            }
        }
        break;
    }
    /////////////////////
    case ICACHE_TLB2_UPDT: 
    {

        if ( ireq.valid ) m_cost_ins_tlb_miss_frz++;
        if ( ireq.valid ) m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req ) 
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // TLB update and invalidate different PTE
        if ( !r_icache_inval_tlb_rsp && !r_dcache_itlb_cleanup_req ) 
        {
            size_t victim_index = 0;
            r_dcache_itlb_cleanup_req = icache_k_tlb.update(r_icache_pte_update,ireq.addr,(r_icache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index);
            r_dcache_itlb_cleanup_line = victim_index;
            r_icache_fsm = ICACHE_IDLE;
        }

        // TLB update and invalidate same PTE
        if ( r_icache_inval_tlb_rsp )                            
        {
            r_icache_inval_tlb_rsp = false;
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
    }
    /////////////////////////////
    case ICACHE_SW_FLUSH1:
    {
        // reset this register and let dcache_fsm return idle for accept cleanup request
        r_dcache_xtn_req = false;
        size_t way = r_icache_way;
        size_t set = r_icache_set;
        bool clean = false;

        // 4M page size TLB flush leads to cleanup req to data cache 
        if ( !r_dcache_itlb_cleanup_req )    // last cleanup finish
        {
            size_t victim_index = 0;
            for ( ; way < m_itlb_m_ways; way++)
            {
                for ( ; set < m_itlb_m_sets; set++)
                {
                    if(icache_m_tlb.checkcleanup(way, set, &victim_index))
                    {
                        clean = true;
                        r_dcache_itlb_cleanup_req = true;
                        r_dcache_itlb_cleanup_line = victim_index;
                        r_icache_way = way + ((set+1)/m_itlb_m_sets);
                        r_icache_set = (set+1) % m_itlb_m_sets;
                        break;
                    }
                }
                if (clean) break;
            }

            if ((way == (m_itlb_m_ways-1)) && (set == (m_itlb_m_sets-1)))
            {
                r_icache_way = 0;
                r_icache_set = 0;
                r_icache_fsm = ICACHE_SW_FLUSH2;
                break;
            }
        }
    }
    /////////////////////////////
    case ICACHE_SW_FLUSH2:
    {
        size_t way = r_icache_way;
        size_t set = r_icache_set;
        bool clean = false;

        // 4K page size TLB flush leads to cleanup req to data cache 
        if ( !r_dcache_itlb_cleanup_req )    // last cleanup finish
        {
            size_t victim_index = 0;
            for ( ; way < m_itlb_k_ways; way++)
            {
                for ( ; set < m_itlb_k_sets; set++)
                {
                    if(icache_k_tlb.checkcleanup(way, set, &victim_index))
                    {
                        clean = true;
                        r_dcache_itlb_cleanup_req = true;
                        r_dcache_itlb_cleanup_line = victim_index;
                        r_icache_way = way + ((set+1)/m_itlb_k_sets);
                        r_icache_set = (set+1) % m_itlb_k_sets;
                        break;
                    }
                }
                if (clean) break;
            }

            if ((way == (m_itlb_k_ways-1)) && (set == (m_itlb_k_sets-1)))
            {
                r_icache_fsm = ICACHE_IDLE;
                break;
            }
        }
    }
    /////////////////////
    case ICACHE_TLB_FLUSH:
    {   
        // data cache flush leads to ins tlb flush, flush all tlb entry 
        icache_m_tlb.flush(true);    // global entries are invalidated
        icache_k_tlb.flush(true);    // global entries are invalidated
        r_dcache_xtn_req = false;
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    ////////////////////////
    case ICACHE_CACHE_FLUSH:
    {
        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_tgt_icache_req = false;
        }

        size_t way = r_icache_way;
        size_t set = r_icache_set;
        bool clean = false;

        // cache flush and send cleanup to external
        if ( !r_icache_cleanup_req )
        {
            size_t victim_index = 0;
            for ( ; way < m_icache_ways; way++ )
            {    
                for ( ; set < m_icache_sets; set++ )
                {    
                    if ( r_icache.flush(way, set, &victim_index) )
                    {
                        clean = true;
                        r_icache_cleanup_req = true;
                        r_icache_cleanup_line = victim_index;
                        r_icache_way = way + ((set+1)/m_icache_sets);
                        r_icache_set = (set+1) % m_icache_sets;
                        break;
                    }
                }
                if (clean) break;
            }
            if ((way == (m_icache_ways-1)) && (set == (m_icache_sets-1)))
            {
                r_dcache_xtn_req = false;
                r_icache_fsm = ICACHE_IDLE;
                break;
            }
        }
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
        size_t victim_index = 0;

        if ( !r_dcache_itlb_cleanup_req )
        {
	        if (r_icache_page_k_save)   
            {
                r_dcache_itlb_cleanup_req = icache_k_tlb.inval(r_dcache_wdata_save,&victim_index);
            }
            else
            {
                r_dcache_itlb_cleanup_req = icache_m_tlb.inval(r_dcache_wdata_save,&victim_index);
            }

            r_dcache_itlb_cleanup_line = victim_index;
            r_dcache_xtn_req = false;
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
    }
    ////////////////////////
    case ICACHE_CACHE_INVAL:
    {	
        addr36_t    ipaddr = 0;                     
        bool        icache_hit_t_m = false;
        bool        icache_hit_t_k = false;

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
        if ( !r_icache_cleanup_req )
        {    
            // invalidate and cleanup if necessary
            r_icache_cleanup_req = r_icache.inval(r_icache_paddr_save);
            r_icache_cleanup_line = r_icache_paddr_save.read() >> (uint32_log2(m_icache_words) + 2);   
            r_dcache_xtn_req = false; 
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
    }
    ///////////////////////
    case ICACHE_MISS_WAIT:
    {
        m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )     
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }
        
        if ( !r_icache_miss_req && !r_icache_inval_rsp ) // Miss read response and no cache invalidation
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

        // r_icache_inval_rsp only used for cc_ins_inval being broadcast
        if ( !r_icache_miss_req && r_icache_inval_rsp ) // Miss read response and cache invalidation
        {
            if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = r_icache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_icache_bad_vaddr = ireq.addr;
                r_icache_inval_rsp = false;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else 
            {
                r_icache_inval_rsp = false;
                r_icache_fsm = ICACHE_IDLE;  
            } 
        }

        if ( !r_icache_miss_req && r_icache_inval_tlb_rsp ) // Miss read response and tlb invalidation
        {
            if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = r_icache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_fsm = ICACHE_IDLE;
            }
        }
        break;
    }
    ////////////////////
    case ICACHE_UNC_WAIT:
    {
        m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req ) 
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        if ( !r_icache_unc_req && !r_icache_inval_tlb_rsp ) // Miss read response and no tlb invalidation
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

        if ( !r_icache_unc_req && r_icache_inval_tlb_rsp ) // Miss read response and tlb invalidation
        {
            if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = r_icache_error_type | MMU_CACHE_ILLEGAL_ACCESS;    
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;
            } 
            else 
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_fsm = ICACHE_IDLE;
            }
        }
        break;
    }
    //////////////////////
    case ICACHE_MISS_UPDT:
    {
        if ( ireq.valid ) m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )   
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_itlb_req || r_ccinval_m_itlb_req )
        {
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        if ( r_icache_inval_tlb_rsp ) // tlb invalidation
        {
            r_icache_inval_tlb_rsp = false;
            r_icache_fsm = ICACHE_IDLE;
            break;
        }

        if ( !r_icache_cleanup_req && !r_icache_inval_rsp ) // Miss update and no invalidation
        {
            data_t* buf = r_icache_miss_buf;
            addr36_t  victim_index = 0;
            m_cpt_icache_dir_write++;
            m_cpt_icache_data_write++;

            r_icache_cleanup_req = r_icache.update(r_icache_paddr_save.read(), buf, &victim_index);
            r_icache_cleanup_line = victim_index;            

            r_icache_fsm = ICACHE_IDLE;
            break;
        }
        if ( r_icache_inval_rsp )   // Miss update and invalidation, only used for cc_ins_inval being broadcast
        {
            r_icache_inval_rsp = false;
            r_icache_fsm = ICACHE_IDLE;
            break;
        }
        break;
    }
    ///////////////////
    case ICACHE_ERROR:
    {
        r_vci_rsp_ins_error = false;
        r_dcache_rsp_itlb_error = false;
        irsp.valid = true;
        irsp.error = true;
        irsp.instruction = 0; 
        r_icache_fsm = ICACHE_IDLE;
        break;
    }
    /////////////////////
    case ICACHE_CC_INVAL:  
    {                       
        if ( ireq.valid ) m_cost_ins_miss_frz++;
        m_cpt_icache_dir_read += m_icache_ways;

        // invalidate cache
        if( (( r_icache_fsm_save == ICACHE_MISS_WAIT ) || ( r_icache_fsm_save == ICACHE_MISS_UPDT ) || ( r_icache_fsm_save == ICACHE_BIS )) && 
            ((r_icache_paddr_save.read() & ~((m_icache_words<<2)-1)) == (r_tgt_addr.read() & ~((m_icache_words<<2)-1))) ) 
        {
            r_icache_inval_rsp = true;
        } 
        else 
        {
            r_icache.inval(r_tgt_addr.read());
        }
        r_tgt_icache_req = false;
        r_icache_fsm = r_icache_fsm_save;
        break;
    }
    /////////////////////////
    case ICACHE_TLB_CC_INVAL:
    {
        size_t icache_tlb_nline = 0;

        if ( ireq.valid ) m_cost_ins_miss_frz++;        

        // invalidate cache
        if( (( r_icache_fsm_save == ICACHE_TLB1_READ ) || ( r_icache_fsm_save == ICACHE_TLB2_READ )  ||
             ( r_icache_fsm_save == ICACHE_TLB1_WRITE )|| ( r_icache_fsm_save == ICACHE_TLB2_WRITE ) ||
             ( r_icache_fsm_save == ICACHE_TLB1_UPDT ) || ( r_icache_fsm_save == ICACHE_TLB2_UPDT )) && 
            ((r_icache_paddr_save.read() & ~((m_icache_words<<2)-1)) == r_tgt_addr.read()) ) 
        {
            r_icache_inval_tlb_rsp = true;
        } 
        else 
        {
            if ( r_ccinval_m_itlb_req )
            { 
                icache_tlb_nline = icache_m_tlb.getnline(r_ccinval_itlb_way, r_ccinval_itlb_set);    
                icache_m_tlb.ccinval(r_ccinval_itlb_way, r_ccinval_itlb_set);
            }
            else
            {
                icache_tlb_nline = icache_k_tlb.getnline(r_ccinval_itlb_way, r_ccinval_itlb_set);    
                icache_k_tlb.ccinval(r_ccinval_itlb_way, r_ccinval_itlb_set);
            }

            if (((r_icache_fsm_save == ICACHE_BIS)||(r_icache_fsm_save == ICACHE_MISS_WAIT) ||
                 (r_icache_fsm_save == ICACHE_UNC_WAIT)||(r_icache_fsm_save == ICACHE_MISS_UPDT)) && 
                (r_icache_tlb_nline == icache_tlb_nline))
            {
                r_icache_inval_tlb_rsp = true;
            }
        }
        r_ccinval_m_itlb_req = false;
        r_ccinval_k_itlb_req = false;
        r_icache_fsm = r_icache_fsm_save;
        break;
    }
    } // end switch r_icache_fsm

#ifdef CC_VCACHE_WRAPPER_DEBUG
std::cout << name() << " Instruction Response: " << irsp << std::endl;
#endif

    ////////////////////////////////////////////////////////////////////////////////////
    //      INVAL ITLB CHECK FSM 
    ////////////////////////////////////////////////////////////////////////////////////////
    switch(r_inval_itlb_fsm) {
    
    case INVAL_ITLB_IDLE:
    {
        if ( r_dcache_itlb_ccinval_check_req )
        {
            r_ccinval_itlb_way = 0; 
            r_ccinval_itlb_set = 0;
            r_inval_itlb_fsm = INVAL_ITLB_CHECK_FIRST;    
        }   
        break;
    }
    ////////////////////////////
    case INVAL_ITLB_CHECK_FIRST:
    {
        size_t way = r_ccinval_itlb_way; 
        size_t set = r_ccinval_itlb_set;
        bool end = false;        

        // r_tgt_addr is number of line
        bool m_hit = icache_m_tlb.cccheck(r_dcache_itlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
        bool k_hit = icache_k_tlb.cccheck(r_dcache_itlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
    
        if ( m_hit )
        {
            r_ccinval_m_itlb_req = true;
            r_ccinval_itlb_way = way; 
            r_ccinval_itlb_set = set;
            r_itlb_cc_check_end = end;
            r_ccinval_itlb_k = false; 
            r_inval_itlb_fsm = INVAL_ITLB_REQ_WAIT;    
        }        
        else if ( k_hit )
        {
            r_ccinval_k_itlb_req = true;
            r_ccinval_itlb_way = way; 
            r_ccinval_itlb_set = set; 
            r_itlb_cc_check_end = end;
            r_ccinval_itlb_k = true; 
            r_inval_itlb_fsm = INVAL_ITLB_REQ_WAIT;    
        }
        else
        {
            r_dcache_itlb_ccinval_check_req = false;
            r_inval_itlb_fsm = INVAL_ITLB_IDLE;    
        }
        break;
    }
    /////////////////////////
    case INVAL_ITLB_REQ_WAIT:
    {
        if ( !r_ccinval_k_itlb_req && !r_ccinval_m_itlb_req ) 
        {
            if ( !r_itlb_cc_check_end )
            {
                r_inval_itlb_fsm = INVAL_ITLB_CHECK; 
                   
            }
            else
            {
                r_inval_itlb_fsm = INVAL_ITLB_RSP;    
            }
        }
        break;
    }
    //////////////////////
    case INVAL_ITLB_CHECK:
    {
        size_t way = r_ccinval_itlb_way; 
        size_t set = r_ccinval_itlb_set;
        bool m_hit = false; 
        bool k_hit = false;
        bool end = false;
 
        // r_tgt_addr is number of line
        if ( !r_ccinval_itlb_k )
        {
            icache_m_tlb.findpost( way, set, &way, &set);
            m_hit = icache_m_tlb.cccheck(r_dcache_itlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
        }
        else
        {
            icache_k_tlb.findpost( way, set, &way, &set);
            k_hit = icache_k_tlb.cccheck(r_dcache_itlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
        }
    
        if ( m_hit )
        {
            r_ccinval_m_itlb_req = true;
            r_ccinval_itlb_way = way; 
            r_ccinval_itlb_set = set;
            r_itlb_cc_check_end = end;
            r_ccinval_itlb_k = false; 
            r_inval_itlb_fsm = INVAL_ITLB_REQ_WAIT;    
        }        
        else if ( k_hit )
        {
            r_ccinval_k_itlb_req = true;
            r_ccinval_itlb_way = way; 
            r_ccinval_itlb_set = set; 
            r_itlb_cc_check_end = end;
            r_ccinval_itlb_k = true; 
            r_inval_itlb_fsm = INVAL_ITLB_REQ_WAIT;    
        }
        else
        {
            r_dcache_itlb_ccinval_check_req = false;
            r_inval_itlb_fsm = INVAL_ITLB_IDLE;    
        }
        break;
    }
    ////////////////////
    case INVAL_ITLB_RSP:
    {
        r_dcache_itlb_ccinval_check_req = false;
        r_itlb_cc_check_end = false;
        r_ccinval_itlb_k = false; 
        r_ccinval_itlb_way = 0; 
        r_ccinval_itlb_set = 0; 
        r_inval_itlb_fsm = INVAL_ITLB_IDLE;    
        break;
    }
    } // end switch r_inval_itlb_fsm

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
        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

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
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        // ins tlb cleanup
        if ( r_dcache_itlb_cleanup_req )
        {
            r_dcache_fsm = DCACHE_ITLB_CLEANUP;
            break;
        }    
        // ins tlb miss
    	if ( r_icache_tlb_read_dcache_req )
    	{
            uint32_t rsp_itlb_miss;
    	    bool itlb_hit_dcache = r_dcache.read(r_icache_paddr_save, &rsp_itlb_miss);	

    	    if ( itlb_hit_dcache )  // ins TLB request hits in data cache 
    	    {
                r_dcache_rsp_itlb_miss = rsp_itlb_miss; 
    	    	r_icache_tlb_read_dcache_req = false;
                r_dcache.setinbit(r_icache_paddr_save, r_dcache_in_itlb, true);
    	    }
    	    else                    // ins TLB request miss in data cache
    	    {
                r_dcache_itlb_read_req = true;
                r_dcache_fsm = DCACHE_ITLB_READ;
            }
    	}
    	else if ( r_icache_tlb_et_dcache_req ) // ins tlb write ET
    	{
            assert(r_dcache.write(r_icache_paddr_save, r_icache_pte_update) && "Write on miss ignores data");
            r_dcache_itlb_et_req = true;
	        r_dcache_fsm = DCACHE_ITLB_ET_WRITE;	    		
    	}
        else if (dreq.valid) 
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
            size_t      dcache_tlb_nline = 0;       // TLB NLINE 

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
                        r_dcache_fsm = DCACHE_CTXT_SWITCH1;
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
                    r_dcache_type_save = dreq.addr/4; 
                    r_dcache_xtn_req = true;
                    r_dcache_way = 0;
                    r_dcache_set = 0;
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
                dcache_hit_t_m = dcache_m_tlb.cctranslate(dreq.addr, &tlb_dpaddr, &dcache_pte_info,
                                                        &dcache_tlb_nline, 
                                                        &dcache_tlb_way, &dcache_tlb_set); 
                dcache_hit_t_k = dcache_k_tlb.cctranslate(dreq.addr, &tlb_dpaddr, &dcache_pte_info, 
                                                        &dcache_tlb_nline, 
                                                        &dcache_tlb_way, &dcache_tlb_set);
                  
                spc_dpaddr     = ((addr36_t)r_dcache_ppn_save << PAGE_K_NBITS) | (addr36_t)((dreq.addr & OFFSET_K_MASK));
                dcache_hit_x   = (((addr_t)r_dcache_vpn_save << PAGE_K_NBITS) == (dreq.addr & ~OFFSET_K_MASK)) && r_dtlb_translation_valid; 
                dcache_hit_p   = (((dreq.addr >> PAGE_M_NBITS) == r_dcache_id1_save) && r_dcache_ptba_ok );
                dcache_cached  = dcache_pte_info.c;    
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
                    if (r_dcache_dtlb_ccinval_check_req && (r_dcache_dtlb_ccinval_check_line == dcache_tlb_nline))
                    {
                        break;
                    }

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
                    r_dcache_tlb_nline = dcache_tlb_nline;
                    r_dtlb_translation_valid = true;
                    r_dcache_page_k_save = false;
                }
                else if ( dcache_hit_t_k ) 
                {
                    dcache_k_tlb.setlru(dcache_tlb_way,dcache_tlb_set);
                    r_dcache_ppn_save = tlb_dpaddr >> PAGE_K_NBITS;
                    r_dcache_vpn_save = dreq.addr >> PAGE_K_NBITS;
                    r_dcache_tlb_nline = dcache_tlb_nline;
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
                r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                r_dcache_fsm = DCACHE_DTLB1_READ_CACHE;
                m_cpt_data_tlb_miss++;
                m_cost_data_tlb_miss_frz++;
            }
            else if ( dcache_hit_p && !dcache_hit_t_m && !dcache_hit_t_k )  // TLB Miss with possibility of bypass first level page
            {
                // walk page table level 2
                r_dcache_tlb_paddr = (addr36_t)r_dcache_ptba_save | 
                                     (addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2); 
                r_dcache_fsm = DCACHE_DTLB2_READ_CACHE;
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
                                assert(r_dcache.write((addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2), 
                                                      (dcache_m_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK)) && "Write on miss ignores data");
                                r_dcache_tlb_dirty_req = true;
                                r_dcache_fsm = DCACHE_WRITE_DIRTY;
                                m_cpt_data_tlb_write_dirty++;
                            }
                            else
                            {   
                                if (dcache_hit_p) 
                                {
                                    addr36_t addr = ((addr36_t)r_dcache_ptba_save | (addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2));
                                    r_dcache_pte_update = dcache_k_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                    r_dcache_tlb_paddr = (addr36_t)r_dcache_ptba_save | (addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2);
                                    assert(r_dcache.write(((addr36_t)r_dcache_ptba_save | (addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2)), 
                                                       (dcache_k_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK)) && "Write on miss ignores data");
                                    r_dcache_tlb_dirty_req = true;
                                    r_dcache_fsm = DCACHE_WRITE_DIRTY;
                                    m_cpt_data_tlb_write_dirty++;
                                }
                                else    // get PTBA to calculate the physical address of PTE
                                {
                                    r_dcache_pte_update = dcache_k_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                    r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                                    r_dcache_tlb_ptba_read = true;
                                    r_dcache_fsm = DCACHE_DTLB1_READ_CACHE;
                                }
                            }
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
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }
        // Using tlb entry is invalidated 
        if ( r_dcache_inval_tlb_rsp )
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            break;
        }

        bool    dcache_hit   = false;
        data_t  dcache_rdata = 0;

        // acces always cached in this state
        dcache_hit = r_dcache.read(r_dcache_paddr_save, &dcache_rdata);
        if ( r_dcache_inval_rsp )
        {
            dcache_hit = false;
            r_dcache_inval_rsp = false;
        }    

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
            else if (!r_dcache_dirty_save && ((r_mmu_mode == TLBS_ACTIVE)||(r_mmu_mode == ITLB_D_DTLB_A)))   // dirty bit update required
            {
                if (!r_dcache_page_k_save) 
                {
                    r_dcache_pte_update = dcache_m_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                    r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                    assert(r_dcache.write((addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2), 
                                         (dcache_m_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK)) && "Write on miss ignores data");
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
                        assert(r_dcache.write(((addr36_t)r_dcache_ptba_save|(addr36_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2)), 
                                         (dcache_k_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK)) && "Write on miss ignores data");
                        r_dcache_tlb_dirty_req = true;
                        r_dcache_fsm = DCACHE_WRITE_DIRTY;
                        m_cpt_data_tlb_write_dirty++;
                    }
                    else
                    {
                        r_dcache_pte_update = dcache_k_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                        r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                        r_dcache_tlb_ptba_read = true;
                        r_dcache_fsm = DCACHE_DTLB1_READ_CACHE;
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
    ////////////////////////////
    case DCACHE_DTLB1_READ_CACHE:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        data_t tlb_data = 0;
        bool tlb_hit_cache = r_dcache.read(r_dcache_tlb_paddr, &tlb_data);
        if ( r_dcache_inval_rsp )
        {
            tlb_hit_cache = false;
            r_dcache_inval_rsp = false;
        }

        // DTLB request hit in cache
        if ( tlb_hit_cache )
        {
            switch((tlb_data & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTD:                   // 4K page
                r_dcache_ptba_ok   = true;
                r_dcache_ptba_save = (addr36_t)(tlb_data & PTD_PTP_MASK) << PAGE_K_NBITS;  
                r_dcache_id1_save  = dreq.addr >> PAGE_M_NBITS;
                r_dcache_tlb_paddr = (addr36_t)((tlb_data & PTD_PTP_MASK) << PAGE_K_NBITS) | 
                                     (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2);
                if ( r_dcache_tlb_ptba_read )
                {
                    r_dcache_tlb_ptba_read = false;
                    r_dcache_tlb_dirty_req = true;
                    assert(r_dcache.write(((addr36_t)((tlb_data & PTD_PTP_MASK) << PAGE_K_NBITS) | 
                                           (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2)), 
                                          r_dcache_pte_update) && "Write on miss ignores data");
                    r_dcache_fsm = DCACHE_WRITE_DIRTY;
                    m_cpt_data_tlb_write_dirty++;
                }
                else
                {
                    r_dcache_fsm = DCACHE_DTLB2_READ_CACHE;
                }
                break;
            case PTE_NEW:               // 4M page (not marked)
                r_dcache_ptba_ok    = false;
                r_dcache_pte_update = tlb_data | PTE_ET_MASK;
                assert(r_dcache.write(r_dcache_tlb_paddr, (tlb_data | PTE_ET_MASK)) && "Write on miss ignores data");  
                r_dcache_tlb_et_req = true;
                r_dcache_fsm        = DCACHE_TLB1_WRITE;
                m_cpt_data_tlb_write_et++;
                break;  
            case PTE_OLD:               // 4M page (already marked)
                r_dcache_ptba_ok    = false;
                r_dcache_pte_update = tlb_data;
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
        else
        {
            // DTLB request miss in cache and walk page table level 1
            r_dcache_tlb_read_req = true;
            r_dcache_fsm = DCACHE_TLB1_READ;
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB1_READ:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req ) 
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }
   
        if ( !r_dcache_tlb_read_req && !r_dcache_inval_tlb_rsp ) // TLB miss response and no invalidation      
        {
            if ( r_vci_rsp_data_error ) // VCI response error
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 
            }
            else                        // VCI response ok
            {
                r_dcache_fsm = DCACHE_TLB1_READ_UPDT; 
            }
        }

        if ( !r_dcache_tlb_read_req && r_dcache_inval_tlb_rsp )  // TLB miss response and invalidation
        {
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 
            }
            else
            {
                r_dcache_inval_tlb_rsp = false;
                r_dcache_fsm = DCACHE_IDLE; 
            }
        }   
        break;
    }
    //////////////////////////
    case DCACHE_TLB1_READ_UPDT:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req ) 
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_cleanup_req && !r_dcache_inval_rsp ) // TLB update in cache and no invalidate 
        {
            // update dcache
            uint32_t rsp_dtlb_miss = 0;
            addr36_t  victim_index = 0;
            size_t way = 0;
            size_t set = 0;
            int dcache_fsm = 0;

            r_dcache_cleanup_req = r_dcache.update(r_dcache_tlb_paddr, r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, r_dcache_miss_buf, &victim_index);
            r_dcache_cleanup_line = victim_index;
            r_dcache.read(r_dcache_tlb_paddr, &rsp_dtlb_miss);	

            switch((rsp_dtlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTD:                   // 4K page
                r_dcache_ptba_ok   = true;
                r_dcache_ptba_save = (addr36_t)(rsp_dtlb_miss & PTD_PTP_MASK) << PAGE_K_NBITS;  
                r_dcache_id1_save  = dreq.addr >> PAGE_M_NBITS;
                r_dcache_tlb_paddr = (addr36_t)((rsp_dtlb_miss & PTD_PTP_MASK) << PAGE_K_NBITS) | 
                                     (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2);
                if ( r_dcache_tlb_ptba_read )
                {
                    r_dcache_tlb_ptba_read = false;
                    r_dcache_tlb_dirty_req = true;
                    assert(r_dcache.write(((addr36_t)((rsp_dtlb_miss & PTD_PTP_MASK) << PAGE_K_NBITS) | 
                                           (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2)), 
                                          r_dcache_pte_update) && "Write on miss ignores data");
                    dcache_fsm = DCACHE_WRITE_DIRTY;
                    m_cpt_data_tlb_write_dirty++;
                }
                else
                {
                    dcache_fsm = DCACHE_DTLB2_READ_CACHE;
                }
                break;
            case PTE_NEW:               // 4M page (not marked)
                r_dcache_ptba_ok    = false;
                r_dcache_pte_update = rsp_dtlb_miss | PTE_ET_MASK; 
                assert(r_dcache.write(r_dcache_tlb_paddr, (rsp_dtlb_miss | PTE_ET_MASK)) && "Write on miss ignores data"); 
                r_dcache_tlb_et_req = true;
                dcache_fsm        = DCACHE_TLB1_WRITE;
                m_cpt_data_tlb_write_et++;
                break;  
            case PTE_OLD:             // 4M page (already marked)
                r_dcache_ptba_ok    = false;
                r_dcache_pte_update = rsp_dtlb_miss;
                dcache_fsm        = DCACHE_TLB1_UPDT;
                break;
            default:                 // unmapped
                r_dcache_ptba_ok    = false;
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_UNMAPPED;       
                r_dcache_bad_vaddr  = dreq.addr;
                dcache_fsm        = DCACHE_ERROR;
                break;
            } // end switch ET

            r_dcache_fsm = dcache_fsm;

            // ins tlb invalidate verification
            if (r_dcache_in_itlb[m_dcache_sets*way+set]) 
            {
                if (!r_dcache_itlb_ccinval_check_req)
                {    
                    r_dcache_itlb_ccinval_check_req = true;
                    r_dcache_itlb_ccinval_check_line = victim_index;
                    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
                }
                else
                {
                    // goto wait state
                    r_dcache_itlb_ccinval_check_wait = true;
                    r_dcache_itlb_ccinval_check_line = victim_index;
                    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
                    r_dcache_fsm_save = dcache_fsm;
                    r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
                }    
            }

            // data tlb invalidate verification
            if (r_dcache_in_dtlb[m_dcache_sets*way+set])
            {
                if (!r_dcache_dtlb_ccinval_check_req)
                {    
                    r_dcache_dtlb_ccinval_check_req = true; 
                    r_dcache_dtlb_ccinval_check_line = victim_index;
                    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
                }
                else
                {
                    // goto wait state
                    r_dcache_dtlb_ccinval_check_wait = true;
                    r_dcache_dtlb_ccinval_check_line = victim_index;
                    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
                    r_dcache_fsm_save = dcache_fsm;
                    r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
                }    
            }
        }

        if ( r_dcache_inval_rsp )
        {
            r_dcache_inval_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB1_WRITE:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req ) 
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_tlb_et_req && !r_dcache_inval_tlb_rsp ) // TLB ET write response and no invalidation 
        {
            if ( r_vci_rsp_data_error ) // VCI response error 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;  
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;  
            } 
            else                        // VCI response ok
            {
                r_dcache_fsm = DCACHE_TLB1_UPDT; 
            }
        }
        if ( !r_dcache_tlb_et_req && r_dcache_inval_tlb_rsp ) // TLB ET write response and invalidation
        {
            if ( r_vci_rsp_data_error ) // VCI response error 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;  
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;  
            } 
            else                        // VCI response ok
            {
                r_dcache_inval_tlb_rsp = false;
                r_dcache_fsm = DCACHE_IDLE; 
            }
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB1_UPDT: 
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req ) 
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_inval_tlb_rsp )
        {
            size_t victim_index = 0;
            if (dcache_m_tlb.update(r_dcache_pte_update,dreq.addr,(r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index))
            {
                r_dcache.setinbit((addr36_t)victim_index*m_dcache_words*2, r_dcache_in_dtlb, false);
            }
            r_dcache.setinbit(r_dcache_tlb_paddr, r_dcache_in_dtlb, true);
            r_dcache_fsm = DCACHE_IDLE;
        }
        else  
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    /////////////////////////////
    case DCACHE_DTLB2_READ_CACHE:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        data_t tlb_data = 0;
        bool tlb_hit_cache = r_dcache.read(r_dcache_tlb_paddr, &tlb_data);
        if ( r_dcache_inval_rsp )
        {
            tlb_hit_cache = false;
            r_dcache_inval_rsp = false;
        }

        // DTLB request hit in cache
        if ( tlb_hit_cache )
        {
            switch((tlb_data & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTE_NEW:               // not marked  
                r_dcache_tlb_et_req = true;
                assert(r_dcache.write(r_dcache_tlb_paddr, (tlb_data | PTE_ET_MASK)) && "Write on miss ignores data");
                r_dcache_pte_update = tlb_data | PTE_ET_MASK;     
                r_dcache_fsm        = DCACHE_TLB2_WRITE;
                m_cpt_data_tlb_write_et++;
                break;  
            case PTE_OLD:               // already marked
                r_dcache_pte_update = tlb_data;   
                r_dcache_fsm        = DCACHE_TLB2_UPDT;
                break;
            default:    
                r_dcache_error_type = r_dcache_error_type | MMU_PT2_UNMAPPED; 
                r_dcache_bad_vaddr  = dreq.addr;
                r_dcache_fsm        = DCACHE_ERROR;
                break;
            }
        }
        else
        {
            // DTLB request miss in cache and walk page table level 2
            r_dcache_tlb_read_req = true;
            r_dcache_fsm = DCACHE_TLB2_READ;
        }
        break;
    }
    /////////////////////
    case DCACHE_TLB2_READ:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_tlb_read_req && !r_dcache_inval_tlb_rsp )  
        {
            if ( r_vci_rsp_data_error ) // VCI response error
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT2_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;
            }
            else                        // VCI response ok
            {
                r_dcache_fsm = DCACHE_TLB2_READ_UPDT;
            }
        }
        if ( !r_dcache_tlb_read_req && r_dcache_inval_tlb_rsp )  // TLB miss response and invalidation
        {
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 
            }
            else
            {
                r_dcache_inval_tlb_rsp = false;
                r_dcache_fsm = DCACHE_IDLE; 
            }
        }   
        break;
    }
    //////////////////////////
    case DCACHE_TLB2_READ_UPDT:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_cleanup_req && !r_dcache_inval_rsp ) // TLB update in cache and no invalidate
        {
            // update cache
            uint32_t rsp_dtlb_miss;
            addr36_t  victim_index = 0;
            size_t way = 0;
            size_t set = 0;
            int dcache_fsm = 0;

            r_dcache_cleanup_req = r_dcache.update(r_dcache_tlb_paddr, r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, r_dcache_miss_buf, &victim_index);
            r_dcache_cleanup_line = victim_index;
            r_dcache.read(r_dcache_tlb_paddr, &rsp_dtlb_miss);

            switch((rsp_dtlb_miss & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTE_NEW:               // not marked  
                r_dcache_tlb_et_req = true;
                r_dcache_pte_update = rsp_dtlb_miss | PTE_ET_MASK;  
                assert(r_dcache.write(r_dcache_tlb_paddr, (rsp_dtlb_miss | PTE_ET_MASK)) && "Write on miss ignores data"); 
                dcache_fsm        = DCACHE_TLB2_WRITE;
                m_cpt_data_tlb_write_et++;
                break;  
            case PTE_OLD:               // already marked
                r_dcache_pte_update = rsp_dtlb_miss;   
                dcache_fsm        = DCACHE_TLB2_UPDT;
                break;
            default:    
                r_dcache_error_type = r_dcache_error_type | MMU_PT2_UNMAPPED; 
                r_dcache_bad_vaddr  = dreq.addr;
                dcache_fsm        = DCACHE_ERROR;
                break;
            }

            r_dcache_fsm = dcache_fsm;

            // ins tlb invalidate verification
            if (r_dcache_in_itlb[m_dcache_sets*way+set])
            {
                if (!r_dcache_itlb_ccinval_check_req)
                {    
                    r_dcache_itlb_ccinval_check_req = true;
                    r_dcache_itlb_ccinval_check_line = victim_index;
                    r_dcache_in_itlb[m_dcache_sets*way+set] = false;
                }
                else
                {
                    // goto wait state
                    r_dcache_itlb_ccinval_check_wait = true;
                    r_dcache_itlb_ccinval_check_line = victim_index;
                    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
                    r_dcache_fsm_save = dcache_fsm;
                    r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
                }    
            }

            // data tlb invalidate verification
            if (r_dcache_in_dtlb[m_dcache_sets*way+set])
            {
                if (!r_dcache_dtlb_ccinval_check_req)
                {    
                    r_dcache_dtlb_ccinval_check_req = true;
                    r_dcache_dtlb_ccinval_check_line = victim_index;
                    r_dcache_in_dtlb[m_dcache_sets*way+set] = false;
                }
                else
                {
                    // goto wait state
                    r_dcache_dtlb_ccinval_check_wait = true;
                    r_dcache_dtlb_ccinval_check_line = victim_index;
                    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
                    r_dcache_fsm_save = dcache_fsm;
                    r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
                }    
            }            
        }

        if ( r_dcache_inval_rsp )
        {
            r_dcache_inval_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    ////////////////////////
    case DCACHE_TLB2_WRITE:
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if (!r_dcache_tlb_et_req && !r_dcache_inval_tlb_rsp ) // TLB ET write response and no invalidation 
        {
            if ( r_vci_rsp_data_error ) // VCI response error 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT2_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 
            } 
            else                        // VCI response ok
            {
                r_dcache_fsm = DCACHE_TLB2_UPDT; 
            }
        }
        if ( !r_dcache_tlb_et_req && r_dcache_inval_tlb_rsp ) // TLB ET write response and invalidation 
        {
            if ( r_vci_rsp_data_error ) // VCI response error 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_PT1_ILLEGAL_ACCESS;  
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;  
            } 
            else                        // VCI response ok
            {
                r_dcache_inval_tlb_rsp = false;
                r_dcache_fsm = DCACHE_IDLE; 
            }
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB2_UPDT:  
    {
        if ( dreq.valid ) m_cost_data_tlb_miss_frz++;
        if ( dreq.valid ) m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }        

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_inval_tlb_rsp )
        {
            size_t victim_index = 0;
            if (dcache_k_tlb.update(r_dcache_pte_update,dreq.addr,(r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index))
            {
                r_dcache.setinbit((addr36_t)victim_index*m_dcache_words*2, r_dcache_in_dtlb, false);
            }
            r_dcache.setinbit(r_dcache_tlb_paddr, r_dcache_in_dtlb, true);
            r_dcache_fsm = DCACHE_IDLE;
        }
        else  
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    ///////////////////////
    case DCACHE_CTXT_SWITCH1:
    {
        // 4M page size TLB flush leads to cleanup corresponding data cache line 
        size_t victim_index = 0;
        size_t way = 0;
        size_t set = 0;

        for ( way = 0; way < m_dtlb_m_ways; way++)
        {
            for ( set = 0; set < m_dtlb_m_sets; set++)
            {
                if(dcache_m_tlb.checkcleanup(way, set, &victim_index))
                {
                    r_dcache.setinbit((addr36_t)(victim_index << (m_dcache_words+2)), r_dcache_in_dtlb, false); 
                }
            }
        }

        if ((way == m_dtlb_m_ways) && (set == m_dtlb_m_sets))
        {
            r_dcache_fsm = DCACHE_CTXT_SWITCH2;
        }
        break;
    }
    ///////////////////////
    case DCACHE_CTXT_SWITCH2:
    {
        // 4K page size TLB flush leads to cleanup corresponding data cache line 
        size_t victim_index = 0;
        size_t way = 0;
        size_t set = 0;

        for ( way = 0; way < m_dtlb_k_ways; way++)
        {
            for ( set = 0; set < m_dtlb_k_sets; set++)
            {
                if(dcache_k_tlb.checkcleanup(way, set, &victim_index))
                {
                    r_dcache.setinbit((addr36_t)(victim_index << (m_dcache_words+2)), r_dcache_in_dtlb, false); 
                }
            }
        }

        if ((way == m_dtlb_k_ways) && (set == m_dtlb_k_sets) && (!r_dcache_xtn_req))
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
        // external cache invalidate request
        if ( r_tgt_dcache_req )
        {
            r_tgt_dcache_req = false;
        }   

        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_ccinval_k_dtlb_req = false;
            r_ccinval_m_dtlb_req = false;
        }
        size_t way = r_dcache_way;
        size_t set = r_dcache_set;
        bool clean = false;
        
        // cache flush and send cleanup to external
        if ( !r_dcache_cleanup_req )
        {
            size_t victim_index = 0;
            for ( ; way < m_dcache_ways; way++ )
            {    
                for ( ; set < m_dcache_sets; set++ )
                {  
                    if ( r_dcache.flush(way, set, &victim_index) )
                    {
                        clean = true;
                        r_dcache_cleanup_req = true;
                        r_dcache_cleanup_line = victim_index;
                        r_dcache_way = way + ((set+1)/m_dcache_sets);
                        r_dcache_set = (set+1) % m_dcache_sets;
                        break;
                    }
                }
                if (clean) break;
            }

            if ((way == (m_dcache_ways-1)) && (set == (m_dcache_sets-1)) && !r_dcache_xtn_req ) 
            {
                // data TLB flush 
                dcache_m_tlb.flush(true);      // global entries are not invalidated   
                dcache_k_tlb.flush(true);      // global entries are not invalidated

                for (size_t line = 0; line < m_dcache_ways*m_dcache_sets; line++)
                {
                    r_dcache_in_itlb[line] = false;
                    r_dcache_in_dtlb[line] = false;
                }

                r_dcache_fsm = DCACHE_IDLE;
                drsp.valid = true;
                break;
            }
        }
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
        bool cleanup = false;
        size_t victim_index = 0;

		if (r_dcache_page_k_save)   
        {
            cleanup = dcache_k_tlb.inval(r_dcache_wdata_save, &victim_index);
        }
        else
        {
            cleanup = dcache_m_tlb.inval(r_dcache_wdata_save, &victim_index);
        }
        
        // clean indicate data tlb bit
        if ( cleanup )
        {  
            r_dcache.setinbit((addr36_t)(victim_index << (m_dcache_words+2)), r_dcache_in_dtlb, false); 
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
        addr36_t dpaddr = 0;
        bool dcache_hit_t_m = false;
        bool dcache_hit_t_k = false; 

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
    {
        size_t way = 0;
        size_t set = 0;
        bool cleanup = r_dcache.inval(r_dcache_paddr_save.read(), &way, &set);

        // ins tlb invalidate verification
        if (r_dcache_in_itlb[way*m_dcache_sets+set])
        {
            if (!r_dcache_itlb_ccinval_check_req)
            {       
                r_dcache_itlb_ccinval_check_req = true;
                r_dcache_itlb_ccinval_check_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2);
                r_dcache_in_itlb[way*m_dcache_sets+set] = false;
            }
            else
            {
                // wait 
                r_dcache_fsm = DCACHE_DCACHE_INVAL_DONE;
                break;
            }    
        }

        // ins tlb invalidate verification
        if (r_dcache_in_dtlb[way*m_dcache_sets+set])
        {
            if (!r_dcache_dtlb_ccinval_check_req)
            {       
                r_dcache_dtlb_ccinval_check_req = true;
                r_dcache_dtlb_ccinval_check_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2);
                r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
            }
            else
            {
                // wait 
                r_dcache_fsm = DCACHE_DCACHE_INVAL_DONE;
                break;
            }    
        }

        if (!r_dcache_cleanup_req) 
        {
            r_dcache_cleanup_req = cleanup;
            r_dcache_cleanup_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2);
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true;
            break;
        }
    }
    /////////////////////
    case DCACHE_MISS_WAIT:
    {
        if ( dreq.valid ) m_cost_data_miss_frz++; 

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }
        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_miss_req && !r_dcache_inval_rsp ) // Miss read response and no invalidation 
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

        if ( !r_dcache_miss_req && r_dcache_inval_rsp )  // Miss read response and invalidation
        {
            if ( r_vci_rsp_data_error )
            {
                r_dcache_error_type = r_dcache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_inval_rsp = false;
                r_dcache_fsm = DCACHE_ERROR;
            }
            else
            {
                r_dcache_inval_rsp = false;
                r_dcache_fsm = DCACHE_IDLE;
            }
        }

        if ( !r_dcache_miss_req && r_dcache_inval_tlb_rsp ) // Miss read response and tlb invalidation
        {
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;
            } 
            else
            {
                r_dcache_inval_tlb_rsp = false;
                r_dcache_fsm = DCACHE_IDLE;
            }
        }
        break;
    }
    /////////////////////
    case DCACHE_MISS_UPDT:
    {
        if ( dreq.valid ) m_cost_data_miss_frz++;

        size_t way = 0;
        size_t set = 0;

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }
        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( r_dcache_inval_tlb_rsp ) // tlb invalidation
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            break;
        }

        if ( !r_dcache_cleanup_req && !r_dcache_inval_rsp ) // Miss update and no invalidation
        {
            data_t* buf = r_dcache_miss_buf;
            addr36_t  victim_index = 0;

            m_cpt_dcache_data_write++;
            m_cpt_dcache_dir_write++;

            r_dcache_cleanup_req = r_dcache.update(r_dcache_paddr_save.read(), r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, buf, &victim_index);
            r_dcache_cleanup_line = victim_index;

            r_dcache_fsm = DCACHE_IDLE;
            // ins tlb invalidate verification
            if (r_dcache_in_itlb[way*m_dcache_sets+set])
            {
                if (!r_dcache_itlb_ccinval_check_req)
                {       
                    r_dcache_itlb_ccinval_check_req = true;
                    r_dcache_itlb_ccinval_check_line = victim_index;
                    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
                }
                else
                {
                    // goto wait state
                    r_dcache_itlb_ccinval_check_wait = true;
                    r_dcache_itlb_ccinval_check_line = victim_index;
                    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
                    r_dcache_fsm_save = DCACHE_IDLE;
                    r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
                }    
            }

            // data tlb invalidate verification
            if (r_dcache_in_dtlb[way*m_dcache_sets+set])
            {
                if (!r_dcache_dtlb_ccinval_check_req)
                {       
                    r_dcache_dtlb_ccinval_check_req = true;
                    r_dcache_dtlb_ccinval_check_line = victim_index;
                    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
                }
                else
                {
                    // goto wait state
                    r_dcache_dtlb_ccinval_check_wait = true;
                    r_dcache_dtlb_ccinval_check_line = victim_index;
                    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
                    r_dcache_fsm_save = DCACHE_IDLE;
                    r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
                }    
            }
            break;
        }

        if ( r_dcache_inval_rsp )   // Miss update and invalidation, data cache has not broadcast, can be deleted
        {
            r_dcache_inval_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            break;
        }
    }
    //////////////////////
    case DCACHE_UNC_WAIT:
    {
        if ( dreq.valid ) m_cost_unc_read_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }
        // external tlb invalidate request
        if ( r_ccinval_k_dtlb_req || r_ccinval_m_dtlb_req )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
        }

        if ( !r_dcache_unc_req && !r_dcache_inval_tlb_rsp ) // Miss read response and no tlb invalidation
        {
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_CACHE_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;
            } 
            else 
            {                
                r_dcache_buf_unc_valid = true;
                r_dcache_fsm = DCACHE_IDLE; 
            } 
        }

        if ( !r_dcache_unc_req && r_dcache_inval_tlb_rsp ) // Miss read response and tlb invalidation
        {
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = r_dcache_error_type | MMU_CACHE_ILLEGAL_ACCESS;    
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;
            } 
            else 
            {
                r_dcache_inval_tlb_rsp = false;
                r_dcache_fsm = DCACHE_IDLE;
            }
        }
        break;
    }
    ///////////////////////
    case DCACHE_WRITE_UPDT:
    {
        m_cpt_dcache_data_write++;
        size_t way = 0;
        size_t set = 0;
        data_t mask = be_to_mask(r_dcache_be_save);
        data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
        assert(r_dcache.write(r_dcache_paddr_save, wdata, &way, &set) && "Write on miss ignores data");
        
        // ins tlb invalidate verification
        if (r_dcache_in_itlb[way*m_dcache_sets+set])
        {
            if (!r_dcache_itlb_ccinval_check_req)
            {       
                r_dcache_itlb_ccinval_check_req = true;
                r_dcache_itlb_ccinval_check_line = (r_dcache.get_tag(way, set) * m_dcache_sets) + set;
                r_dcache_in_itlb[way*m_dcache_sets+set] = false;
            }
            else
            {
                // wait
                r_dcache_fsm = DCACHE_WRITE_UPDT;
                break;
            }    
        }

        // data tlb invalidate verification
        if (r_dcache_in_dtlb[way*m_dcache_sets+set])
        {
            if (!r_dcache_dtlb_ccinval_check_req)
            {       
                r_dcache_dtlb_ccinval_check_req = true;
                r_dcache_dtlb_ccinval_check_line = (r_dcache.get_tag(way, set) * m_dcache_sets) + set;
                r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
            }
            else
            {
                // wait 
                r_dcache_fsm = DCACHE_WRITE_UPDT;
                break;
            }    
        }

        if ( !r_dcache_dirty_save && ((r_mmu_mode == TLBS_ACTIVE)||(r_mmu_mode == ITLB_D_DTLB_A)))   
        {
            if ( r_dcache_page_k_save )
            { 
                r_dcache_tlb_paddr = (addr36_t)r_dcache_ptba_save | (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2);
                r_dcache_pte_update = dcache_k_tlb.getpte(r_dcache_tlb_way_save,r_dcache_tlb_set_save) | PTE_D_MASK;
                assert(r_dcache.write(((addr36_t)r_dcache_ptba_save | (addr36_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 2)), 
                                      (dcache_k_tlb.getpte(r_dcache_tlb_way_save,r_dcache_tlb_set_save) | PTE_D_MASK)) && "Write on miss ignores data");
            }
            else
            {
                r_dcache_tlb_paddr = (addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                r_dcache_pte_update = dcache_m_tlb.getpte(r_dcache_tlb_way_save,r_dcache_tlb_set_save) | PTE_D_MASK;
                assert(r_dcache.write(((addr36_t)(r_mmu_ptpr << 4) | (addr36_t)((dreq.addr>>PAGE_M_NBITS)<<2)), 
                                      (dcache_m_tlb.getpte(r_dcache_tlb_way_save,r_dcache_tlb_set_save) | PTE_D_MASK)) && "Write on miss ignores data");
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
    ////////////////////// 
    case DCACHE_ITLB_READ:
    {
    	if ( !r_dcache_itlb_read_req ) // vci response ok
        {  
            if ( r_vci_rsp_data_error )
            {
                r_dcache_rsp_itlb_error = true;	
                r_icache_tlb_read_dcache_req = false;
                r_dcache_fsm = DCACHE_IDLE;
            }
            else 
            {
                r_dcache_fsm = DCACHE_ITLB_UPDT;
            }
        }
	    break;    	
    }
    //////////////////////
    case DCACHE_ITLB_UPDT:
    {
        if ( !r_dcache_cleanup_req )
        {
            uint32_t rsp_itlb_miss = 0;
            addr36_t  victim_index = 0;
            size_t way = 0;
            size_t set = 0;
            
            bool cleanup = r_dcache.update(r_icache_paddr_save, r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, r_dcache_miss_buf, &victim_index);

            // ins tlb invalidate verification
            if (r_dcache_in_itlb[m_dcache_sets*way+set])
            {
                if (!r_dcache_itlb_ccinval_check_req)
                {    
                    r_dcache_itlb_ccinval_check_req = true;
                    r_dcache_itlb_ccinval_check_line = victim_index;
                    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
                }
                else
                {
                    // wait 
                    r_dcache_fsm = DCACHE_ITLB_UPDT;
                    break;
                }    
            }

            // data tlb invalidate verification
            if (r_dcache_in_dtlb[m_dcache_sets*way+set])
            {
                if (!r_dcache_dtlb_ccinval_check_req)
                {    
                    r_dcache_dtlb_ccinval_check_req = true;
                    r_dcache_dtlb_ccinval_check_line = victim_index;
                    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
                }
                else
                {
                    // wait
                    r_dcache_fsm = DCACHE_ITLB_UPDT;
                    break;
                }    
            }

            r_dcache_cleanup_req = cleanup;
            r_dcache_cleanup_line = victim_index; 

            r_dcache.setinbit(r_icache_paddr_save, r_dcache_in_itlb, true);
            r_dcache.read(r_icache_paddr_save, &rsp_itlb_miss);	
            
            r_dcache_rsp_itlb_miss = rsp_itlb_miss;
            r_dcache_rsp_itlb_error = false;	
            r_icache_tlb_read_dcache_req = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    //////////////////////
    case DCACHE_ITLB_ET_WRITE:
    {
        if ( !r_dcache_itlb_et_req )      
        { 
            r_icache_tlb_et_dcache_req = false;	
            r_dcache_rsp_itlb_error = r_vci_rsp_data_error;  
            r_dcache_fsm = DCACHE_IDLE;
        } 
   	    break;
    }  
    /////////////////////
    case DCACHE_CC_CHECK:   // read directory in case of invalidate or update request
    {
        m_cpt_dcache_dir_read += m_dcache_ways;
        m_cpt_dcache_data_read += m_dcache_ways;

        data_t dcache_rdata = 0;
        size_t way = 0;
        size_t set = 0;

        if(( ( r_dcache_fsm_save == DCACHE_MISS_WAIT ) || ( r_dcache_fsm_save == DCACHE_MISS_UPDT ) || (r_dcache_fsm_save == DCACHE_BIS ) ) && 
           ( (r_dcache_paddr_save.read() & ~((m_dcache_words<<2)-1)) == (r_tgt_addr.read() & ~((m_dcache_words<<2)-1)))) 
        {
            r_dcache_inval_rsp = true;
            r_tgt_dcache_req = false;
            r_dcache_fsm = r_dcache_fsm_save;
        } 
        else if(( ( r_dcache_fsm_save == DCACHE_TLB1_READ_UPDT ) || ( r_dcache_fsm_save == DCACHE_TLB2_READ_UPDT ) ) && 
           ( (r_dcache_tlb_paddr.read() & ~((m_dcache_words<<2)-1)) == (r_tgt_addr.read() & ~((m_dcache_words<<2)-1)))) 
        {
            r_dcache_inval_rsp = true;
            r_tgt_dcache_req = false;
            r_dcache_fsm = r_dcache_fsm_save;
        } 
        else 
        {
            bool dcache_hit = r_dcache.read(r_tgt_addr.read(), &dcache_rdata, &way, &set);

            if  ( dcache_hit )
            {
                if ( r_dcache_in_itlb[m_dcache_sets*way+set] )
                {
                    if ( !r_dcache_itlb_ccinval_check_req )
                    {
                        r_dcache_itlb_ccinval_check_req = true;
                        r_dcache_itlb_ccinval_check_line = r_tgt_addr.read() >> (uint32_log2(m_dcache_words)+2);
                        r_dcache_in_itlb[m_dcache_sets*way+set] = false;
                    }
                    else
                    {
                        r_dcache_itlb_ccinval_check_wait = true;
                        r_dcache_itlb_ccinval_check_line_save = r_tgt_addr.read() >> (uint32_log2(m_dcache_words)+2);
                        r_dcache_in_itlb[m_dcache_sets*way+set] = false;
                    }
                }
                
                if ( r_dcache_in_dtlb[m_dcache_sets*way+set] )
                {
                    if ( !r_dcache_dtlb_ccinval_check_req )
                    {
                        r_dcache_dtlb_ccinval_check_req = true;
                        r_dcache_dtlb_ccinval_check_line = r_tgt_addr.read() >> (uint32_log2(m_dcache_words)+2);
                        r_dcache_in_dtlb[m_dcache_sets*way+set] = false;
                    }
                    else
                    {
                        r_dcache_dtlb_ccinval_check_wait = true;
                        r_dcache_dtlb_ccinval_check_line_save = r_tgt_addr.read() >> (uint32_log2(m_dcache_words)+2);
                        r_dcache_in_dtlb[m_dcache_sets*way+set] = false;
                    }
                }
                
                if ( r_tgt_update ) // update 
                {
                    r_dcache_fsm = DCACHE_CC_UPDT;
                    // complete the line buffer in case of update
                    for ( size_t word = 0 ; word < m_dcache_words ; word++ ) 
                    {
                        if ( !r_tgt_val[word] ) 
                        {
                            r_dcache.read(r_tgt_addr.read() + word, &dcache_rdata);
                            r_tgt_buf[word] = dcache_rdata;
                        }
                    }
                } 
                else                // invalidate 
                {
                    r_dcache_fsm = DCACHE_CC_INVAL;
                }
            }
            else                                    // nothing
            {
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
        data_t* buf = r_tgt_buf;
        for( size_t i = 0; i < m_dcache_words; i++ )
        {
            if( r_tgt_val[i] ) r_dcache.write( r_tgt_addr.read() + i*4, buf[i] );
        }
        
        if ( r_dcache_itlb_ccinval_check_wait && !r_dcache_itlb_ccinval_check_req )
        {
            r_dcache_itlb_ccinval_check_req = true;
            r_dcache_itlb_ccinval_check_line = r_dcache_itlb_ccinval_check_line_save;
            r_dcache_itlb_ccinval_check_wait = false;
        }

        if ( r_dcache_dtlb_ccinval_check_wait && !r_dcache_dtlb_ccinval_check_req )
        {
            r_dcache_dtlb_ccinval_check_req = true;
            r_dcache_dtlb_ccinval_check_line = r_dcache_dtlb_ccinval_check_line_save;
            r_dcache_dtlb_ccinval_check_wait = false;
        }
        
        if ( (r_dcache_itlb_ccinval_check_wait && r_dcache_itlb_ccinval_check_req) ||
             (r_dcache_dtlb_ccinval_check_wait && r_dcache_dtlb_ccinval_check_req) )
        {
            r_tgt_req = true;
            r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
        }
        else
        {
            r_tgt_dcache_req = false;
            r_dcache_fsm = r_dcache_fsm_save;
        }
        break;
    }
    /////////////////////
    case DCACHE_CC_INVAL:   // invalidate a cache line
    {
        r_dcache.inval(r_tgt_addr.read());

        if ( r_dcache_itlb_ccinval_check_wait && !r_dcache_itlb_ccinval_check_req )
        {
            r_dcache_itlb_ccinval_check_req = true;
            r_dcache_itlb_ccinval_check_line = r_dcache_itlb_ccinval_check_line_save;
            r_dcache_itlb_ccinval_check_wait = false;
        }

        if ( r_dcache_dtlb_ccinval_check_wait && !r_dcache_dtlb_ccinval_check_req )
        {
            r_dcache_dtlb_ccinval_check_req = true;
            r_dcache_dtlb_ccinval_check_line = r_dcache_dtlb_ccinval_check_line_save;
            r_dcache_dtlb_ccinval_check_wait = false;
        }
        
        if ( (r_dcache_itlb_ccinval_check_wait && r_dcache_itlb_ccinval_check_req) ||
             (r_dcache_dtlb_ccinval_check_wait && r_dcache_dtlb_ccinval_check_req) )
        {
            r_tgt_req = true;
            r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
        }
        else
        {
            r_tgt_dcache_req = false;
            r_dcache_fsm = r_dcache_fsm_save;
        }
        break;
    }
    ///////////////////
    case DCACHE_CC_NOP:     // no external hit
    {
        r_tgt_dcache_req = false;
        r_dcache_fsm = r_dcache_fsm_save;
        break;
    }    
    //////////////////////////////
    case DCACHE_TLB_CC_INVAL_WAIT:   // invalidate a cache line
    {
        if ( r_dcache_itlb_ccinval_check_wait && !r_dcache_itlb_ccinval_check_req )
        {
            r_dcache_itlb_ccinval_check_req = true;
            r_dcache_itlb_ccinval_check_line = r_dcache_itlb_ccinval_check_line_save;
            r_dcache_itlb_ccinval_check_wait = false;
        }

        if ( r_dcache_dtlb_ccinval_check_wait && !r_dcache_dtlb_ccinval_check_req )
        {
            r_dcache_dtlb_ccinval_check_req = true;
            r_dcache_dtlb_ccinval_check_line = r_dcache_dtlb_ccinval_check_line_save;
            r_dcache_dtlb_ccinval_check_wait = false;
        }
        
        if ( (r_dcache_itlb_ccinval_check_wait && r_dcache_itlb_ccinval_check_req) ||
             (r_dcache_dtlb_ccinval_check_wait && r_dcache_dtlb_ccinval_check_req) )
        {
            r_dcache_fsm = DCACHE_TLB_CC_INVAL_WAIT;
        }
        else
        {
            // if come from DCACHE_MISS_UPDT or DCACHE_TLB1/2_READ_UPDT, don't reset r_tgt_dcache_req 
            if (r_tgt_req)
            { 
                r_tgt_req = false;
                r_tgt_dcache_req = false;
            }
            r_dcache_fsm = r_dcache_fsm_save;
        }
        break;
    }
    /////////////////////////
    case DCACHE_TLB_CC_INVAL:
    {
        size_t dcache_tlb_nline = 0;

        if ( dreq.valid ) m_cost_data_miss_frz++;        

        if( (( r_dcache_fsm_save == DCACHE_TLB1_READ )      || ( r_dcache_fsm_save == DCACHE_TLB2_READ ) ||
             ( r_dcache_fsm_save == DCACHE_TLB1_READ_UPDT ) || ( r_dcache_fsm_save == DCACHE_TLB1_READ_UPDT ) ||
             ( r_dcache_fsm_save == DCACHE_TLB1_WRITE )     || ( r_dcache_fsm_save == DCACHE_TLB2_WRITE ) ||
             ( r_dcache_fsm_save == DCACHE_TLB1_UPDT )      || ( r_dcache_fsm_save == DCACHE_TLB2_UPDT )) && 
            ((r_dcache_tlb_paddr.read() & ~((m_dcache_words<<2)-1)) == r_tgt_addr.read()) ) 
        {
            r_dcache_inval_tlb_rsp = true;
        } 
        else 
        {
            if ( r_ccinval_m_dtlb_req )
            { 
                dcache_tlb_nline = dcache_m_tlb.getnline(r_ccinval_dtlb_way, r_ccinval_dtlb_set);    
                dcache_m_tlb.ccinval(r_ccinval_dtlb_way, r_ccinval_dtlb_set);
            }
            else
            {
                dcache_tlb_nline = dcache_k_tlb.getnline(r_ccinval_dtlb_way, r_ccinval_dtlb_set);    
                dcache_k_tlb.ccinval(r_ccinval_dtlb_way, r_ccinval_dtlb_set);
            }

            if (((r_dcache_fsm_save == DCACHE_BIS)||(r_dcache_fsm_save == DCACHE_MISS_WAIT) ||
                 (r_dcache_fsm_save == DCACHE_UNC_WAIT)||(r_dcache_fsm_save == DCACHE_MISS_UPDT)) && 
                (r_dcache_tlb_nline == dcache_tlb_nline))
            {
                r_dcache_inval_tlb_rsp = true;
            }
        }
        r_ccinval_m_dtlb_req = false;
        r_ccinval_k_dtlb_req = false;
        r_dcache_fsm = r_dcache_fsm_save;
        break;
    }
    /////////////////////////
    case DCACHE_ITLB_CLEANUP:
    {
        r_dcache.setinbit(((addr36_t)r_dcache_itlb_cleanup_line.read()*m_dcache_words*2), r_dcache_in_itlb, false);
        r_dcache_itlb_cleanup_req = false;
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }
    } // end switch r_dcache_fsm

#ifdef CC_VCACHE_WRAPPER_DEBUG
std::cout << " Data Response: " << drsp << std::endl;
#endif

    ////////////////////////////////////////////////////////////////////////////////////
    //      INVAL DTLB CHECK FSM 
    ////////////////////////////////////////////////////////////////////////////////////////
    switch(r_inval_dtlb_fsm) {
    
    case INVAL_DTLB_IDLE:
    {
        if ( r_dcache_dtlb_ccinval_check_req )
        {
            r_inval_dtlb_fsm = INVAL_DTLB_CHECK_FIRST;    
        }   
        break;
    }
    ////////////////////////////
    case INVAL_DTLB_CHECK_FIRST:
    {
        size_t way = 0; 
        size_t set = 0;
        bool end = false;        

        // r_tgt_addr is number of line
        bool m_hit = dcache_m_tlb.cccheck(r_dcache_dtlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
        bool k_hit = dcache_k_tlb.cccheck(r_dcache_dtlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
    
        if ( m_hit )
        {
            r_ccinval_m_dtlb_req = true;
            r_ccinval_dtlb_way = way; 
            r_ccinval_dtlb_set = set;
            r_dtlb_cc_check_end = end;
            r_ccinval_dtlb_k = false; 
            r_inval_dtlb_fsm = INVAL_DTLB_REQ_WAIT;    
        }        
        else if ( k_hit )
        {
            r_ccinval_k_dtlb_req = true;
            r_ccinval_dtlb_way = way; 
            r_ccinval_dtlb_set = set; 
            r_dtlb_cc_check_end = end;
            r_ccinval_dtlb_k = true; 
            r_inval_dtlb_fsm = INVAL_DTLB_REQ_WAIT;    
        }
        else
        {
            r_dcache_dtlb_ccinval_check_req = false;
            r_inval_dtlb_fsm = INVAL_DTLB_IDLE;    
        }
        break;
    }
    /////////////////////////
    case INVAL_DTLB_REQ_WAIT:
    {
        if ( !r_ccinval_k_dtlb_req && !r_ccinval_m_dtlb_req ) 
        {
            if ( !r_dtlb_cc_check_end )
            {
                r_inval_dtlb_fsm = INVAL_DTLB_CHECK; 
                   
            }
            else
            {
                r_inval_dtlb_fsm = INVAL_DTLB_RSP;    
            }
        }
        break;
    }
    //////////////////////
    case INVAL_DTLB_CHECK:
    {
        size_t way = r_ccinval_dtlb_way; 
        size_t set = r_ccinval_dtlb_set;
        bool m_hit = false; 
        bool k_hit = false;
        bool end = false;
 
        // r_tgt_addr is number of line
        if ( !r_ccinval_dtlb_k )
        {
            dcache_m_tlb.findpost( way, set, &way, &set);
            m_hit = dcache_m_tlb.cccheck(r_dcache_dtlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
        }
        else
        {
            dcache_k_tlb.findpost( way, set, &way, &set);
            k_hit = dcache_k_tlb.cccheck(r_dcache_dtlb_ccinval_check_line.read(), way, set, &way, &set, &end); 
        }
    
        if ( m_hit )
        {
            r_ccinval_m_dtlb_req = true;
            r_ccinval_dtlb_way = way; 
            r_ccinval_dtlb_set = set;
            r_dtlb_cc_check_end = end;
            r_ccinval_dtlb_k = false; 
            r_inval_dtlb_fsm = INVAL_DTLB_REQ_WAIT;    
        }        
        else if ( k_hit )
        {
            r_ccinval_k_dtlb_req = true;
            r_ccinval_dtlb_way = way; 
            r_ccinval_dtlb_set = set; 
            r_dtlb_cc_check_end = end;
            r_ccinval_dtlb_k = true; 
            r_inval_dtlb_fsm = INVAL_DTLB_REQ_WAIT;    
        }
        else
        {
            r_dcache_dtlb_ccinval_check_req = false;
            r_inval_dtlb_fsm = INVAL_DTLB_IDLE;    
        }
        break;
    }
    ////////////////////
    case INVAL_DTLB_RSP:
    {
        r_dcache_dtlb_ccinval_check_req = false;
        r_dtlb_cc_check_end = false;
        r_ccinval_dtlb_k = false; 
        r_ccinval_dtlb_way = 0; 
        r_ccinval_dtlb_set = 0; 
        r_inval_dtlb_fsm = INVAL_DTLB_IDLE;    
        break;
    }
    } // end switch r_inval_itlb_fsm

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

        if (r_dcache_itlb_read_req)           
        {            
            r_vci_cmd_fsm = CMD_ITLB_READ;
            m_cpt_itlbmiss_transaction++; 
        } 
        else if (r_dcache_itlb_et_req)      
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
        else if (r_icache_cleanup_req)
        {
            r_vci_cmd_fsm = CMD_INS_CLEANUP;
        }
        else if (r_dcache_cleanup_req)
        {
            r_vci_cmd_fsm = CMD_DATA_CLEANUP;
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci_ini.cmdack.read() ) 
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
        if ( p_vci_ini.cmdack.read() )
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
        assert( ! p_vci_ini.rspval.read() && "Unexpected response" );

        if (r_vci_cmd_fsm != CMD_IDLE)
            break;

        r_vci_rsp_cpt = 0;
        if (r_dcache_itlb_read_req)          // ITLB miss response
        {            
            r_vci_rsp_fsm = RSP_ITLB_READ;
        } 
        else if (r_dcache_itlb_et_req)       // ITLB linked load response
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
        else if (r_icache_cleanup_req)
        {
            r_vci_rsp_fsm = RSP_INS_CLEANUP;
        }
        else if (r_dcache_cleanup_req)
        {
            r_vci_rsp_fsm = RSP_DATA_CLEANUP;
        }
        break;

    case RSP_ITLB_READ:
        m_cost_itlbmiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini.rdata.read();
        if ( p_vci_ini.reop.read() ) 
        {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                    "illegal VCI response packet for data read miss");
            r_dcache_itlb_read_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        } 
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_data_error = true;
        }
        break;

    case RSP_ITLB_WRITE:
        m_cost_itlb_write_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert(p_vci_ini.reop.read() &&
               "illegal VCI response packet for write tlb");

        if ( p_vci_ini.rerror.read() ) 
        {
            r_vci_rsp_data_error = true;
        }
        r_dcache_itlb_et_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_INS_MISS:
        m_cost_imiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert( (r_vci_rsp_cpt < m_icache_words) && 
               "The VCI response packet for instruction miss is too long");
        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_icache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini.rdata.read();

        if ( p_vci_ini.reop.read() ) 
        {
            assert( (r_vci_rsp_cpt == m_icache_words - 1) &&
                       "The VCI response packet for instruction miss is too short");
            r_icache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
                
        } 
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_ins_error = true;
        }
        break;

    case RSP_INS_UNC:
        m_cost_imiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert(p_vci_ini.reop.read() &&
               "illegal VCI response packet for uncached instruction");

        r_icache_miss_buf[0] = (data_t)p_vci_ini.rdata.read();
        r_icache_buf_unc_valid = true;
        r_icache_unc_req = false;
        r_vci_rsp_fsm = RSP_IDLE;

        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_ins_error = true;
        }
        break;

    case RSP_DTLB_READ:
        m_cost_dtlbmiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini.rdata.read();
        if ( p_vci_ini.reop.read() ) 
        {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                    "illegal VCI response packet for data read miss");
            r_dcache_tlb_read_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        } 
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_data_error = true;
        }
        break;

    case RSP_DTLB_WRITE:
        m_cost_dtlb_write_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert(p_vci_ini.reop.read() &&
               "illegal VCI response packet for write tlb");

        if ( p_vci_ini.rerror.read() ) 
        {   
            r_vci_rsp_data_error = true;
        }
        r_dcache_tlb_et_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DTLB_DIRTY:
        m_cost_dtlb_write_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert(p_vci_ini.reop.read() &&
               "illegal VCI response packet for write tlb");

        if ( p_vci_ini.rerror.read() ) 
        {   
            r_vci_rsp_data_error = true;
        }
        r_dcache_tlb_dirty_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_UNC:
        m_cost_unc_transaction++;
        if ( ! p_vci_ini.rspval.read() ) 
            break;

        assert(p_vci_ini.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
        else
        {
            r_dcache_miss_buf[0] = (data_t)p_vci_ini.rdata.read();
            r_dcache_buf_unc_valid = true;
        }
        r_dcache_unc_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_MISS:
        m_cost_dmiss_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini.rdata.read();
        if ( p_vci_ini.reop.read() ) 
        {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                    "illegal VCI response packet for data read miss");
            r_dcache_miss_req = false;
            r_vci_rsp_fsm = RSP_IDLE;
        } 
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_data_error = true;
        }
        break;

    case RSP_DATA_WRITE:
        m_cost_write_transaction++;
        if ( ! p_vci_ini.rspval.read() )
            break;

        if ( p_vci_ini.reop.read() ) 
        {
            r_vci_rsp_fsm = RSP_IDLE;
            r_dcache_write_req = false;
        }
        if ( p_vci_ini.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            m_iss.setWriteBerr();
        }
        break;

    case RSP_INS_CLEANUP:
    case RSP_DATA_CLEANUP:
        if ( ! p_vci_ini.rspval.read() )
            break;
        assert( p_vci_ini.reop.read() &&
                "illegal VCI response packet for icache cleanup");
        assert( (p_vci_ini.rerror.read() == vci_param::ERR_NORMAL) &&
                "error in response packet for icache cleanup");

        if ( r_vci_rsp_fsm == RSP_INS_CLEANUP ) 
        {
            r_icache_cleanup_req = false;
        }
        else
        {                                    
            r_dcache_cleanup_req = false;
        }
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    } // end switch r_vci_rsp_fsm
} // end transition()

///////////////////////
tmpl(void)::genMoore()
///////////////////////
{
    // VCI initiator response

    p_vci_ini.rspack = true;

    // VCI initiator command

    p_vci_ini.trdid  = 0;
    p_vci_ini.pktid  = 0;
    p_vci_ini.srcid  = m_srcid;
    p_vci_ini.cons   = false;
    p_vci_ini.wrap   = false;
    p_vci_ini.contig = true;
    p_vci_ini.clen   = 0;
    p_vci_ini.cfixed = false;

    switch (r_vci_cmd_fsm) {

    case CMD_IDLE:
        p_vci_ini.cmdval  = false;
        p_vci_ini.address = 0;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0;
        p_vci_ini.plen    = 0;
        p_vci_ini.cmd     = vci_param::CMD_WRITE;
        p_vci_ini.eop     = false;
        break;

    case CMD_ITLB_READ:     
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_icache_paddr_save.read() & m_dcache_yzmask;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = m_dcache_words << 2;
        p_vci_ini.cmd     = vci_param::CMD_READ;
        p_vci_ini.eop     = true;
        break;

    case CMD_ITLB_WRITE: 
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_icache_paddr_save.read() & ~0x3;
        p_vci_ini.wdata   = r_icache_pte_update.read();
        p_vci_ini.be      = 0x8;
        p_vci_ini.plen    = 4;
        p_vci_ini.cmd     = vci_param::CMD_WRITE;
        p_vci_ini.eop     = true;
        break;

    case CMD_INS_MISS:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_icache_paddr_save.read() & m_icache_yzmask;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = m_icache_words << 2;
        p_vci_ini.cmd     = vci_param::CMD_READ;
        p_vci_ini.eop     = true;
        break;

    case CMD_INS_UNC:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_icache_paddr_save.read() & ~0x3;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = 4;
        p_vci_ini.cmd     = vci_param::CMD_READ;
        p_vci_ini.eop     = true;
        break;

    case CMD_DTLB_READ:     
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_dcache_tlb_paddr.read() & m_dcache_yzmask;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = m_dcache_words << 2;
        p_vci_ini.cmd     = vci_param::CMD_READ;
        p_vci_ini.eop     = true;
        break;

    case CMD_DTLB_WRITE:     
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci_ini.wdata   = r_dcache_pte_update.read();
        p_vci_ini.be      = 0x8;
        p_vci_ini.plen    = 4;
        p_vci_ini.cmd     = vci_param::CMD_WRITE;
        p_vci_ini.eop     = true;
        break;

    case CMD_DTLB_DIRTY:     
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci_ini.wdata   = r_dcache_pte_update.read();
        p_vci_ini.be      = 0x1;
        p_vci_ini.plen    = 4;
        p_vci_ini.cmd     = vci_param::CMD_WRITE;
        p_vci_ini.eop     = true;
        break;

    case CMD_DATA_UNC:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_dcache_paddr_save.read() & ~0x3;
        p_vci_ini.plen    = 4;
        p_vci_ini.eop     = true;
        switch(r_dcache_type_save) {
        case iss_t::DATA_READ:
            p_vci_ini.wdata = 0;
            p_vci_ini.be    = r_dcache_be_save.read();
            p_vci_ini.cmd   = vci_param::CMD_READ;
            break;
        case iss_t::DATA_LL:
            p_vci_ini.wdata = 0;
            p_vci_ini.be    = 0xF;
            p_vci_ini.cmd   = vci_param::CMD_LOCKED_READ;
            break;
        case iss_t::DATA_SC:
            p_vci_ini.wdata = r_dcache_wdata_save.read();
            p_vci_ini.be    = 0xF;
            p_vci_ini.cmd   = vci_param::CMD_STORE_COND;
            break;
        default:
            assert("this should not happen");
        }
        break;

    case CMD_DATA_WRITE:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_wbuf.getAddress(r_vci_cmd_cpt);
        p_vci_ini.wdata   = r_wbuf.getData(r_vci_cmd_cpt);
        p_vci_ini.be      = r_wbuf.getBe(r_vci_cmd_cpt);
        p_vci_ini.plen    = (r_vci_cmd_max - r_vci_cmd_min + 1)<<2;
        p_vci_ini.cmd     = vci_param::CMD_WRITE;
        p_vci_ini.eop     = (r_vci_cmd_cpt == r_vci_cmd_max);
        break;

    case CMD_DATA_MISS:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = r_dcache_paddr_save.read() & m_dcache_yzmask;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = m_dcache_words << 2;
        p_vci_ini.cmd     = vci_param::CMD_READ;
        p_vci_ini.eop     = true;
        break;

    case CMD_INS_CLEANUP:
    case CMD_DATA_CLEANUP:
        p_vci_ini.cmdval = true;
        // todo : confirm why set p_vci_ini.address like this ? doesn't find L2 ??
        //if ( r_vci_cmd_fsm == CMD_INS_CLEANUP ) p_vci_ini.address = m_cleanup_address | ((r_icache_cleanup_line.read() *(m_icache_words<<2)) & 0xC0000000 );
        //else                                    p_vci_ini.address = m_cleanup_address | ((r_dcache_cleanup_line.read() *(m_dcache_words<<2)) & 0xC0000000 );
        //if ( r_vci_cmd_fsm == CMD_INS_CLEANUP ) p_vci_ini.wdata = r_icache_cleanup_line.read();
        //else                                    p_vci_ini.wdata = r_dcache_cleanup_line.read();

        if ( r_vci_cmd_fsm == CMD_INS_CLEANUP ) p_vci_ini.address = m_cleanup_address;
        else                                    p_vci_ini.address = m_cleanup_address;
        if ( r_vci_cmd_fsm == CMD_INS_CLEANUP ) p_vci_ini.wdata = r_icache_cleanup_line.read();
        else                                    p_vci_ini.wdata = r_dcache_cleanup_line.read();

        p_vci_ini.be     = 0;
        p_vci_ini.plen   = 4;
        p_vci_ini.cmd    = vci_param::CMD_WRITE;
        p_vci_ini.contig = false;
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

    case TGT_RSP:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = !r_tgt_icache_req.read() && !r_tgt_dcache_req.read();
        p_vci_tgt.rsrcid  = r_tgt_srcid.read();
        p_vci_tgt.rpktid  = r_tgt_pktid.read();
        p_vci_tgt.rtrdid  = r_tgt_trdid.read();
        p_vci_tgt.rdata   = 0;
        p_vci_tgt.rerror  = 0;
        p_vci_tgt.reop    = true;
        break;

    case TGT_REQ:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = false;
        break;

    } // end switch TGT_FSM

#ifdef CC_VCACHE_WRAPPER_DEBUG 
   std::cout 
       << "Moore:" << std::hex
       << "p_vci_ini.cmdval:" << p_vci_ini.cmdval
       << "p_vci_ini.address:" << p_vci_ini.address
       << "p_vci_ini.wdata:" << p_vci_ini.wdata
       << "p_vci_ini.cmd:" << p_vci_ini.cmd
       << "p_vci_ini.eop:" << p_vci_ini.eop
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


