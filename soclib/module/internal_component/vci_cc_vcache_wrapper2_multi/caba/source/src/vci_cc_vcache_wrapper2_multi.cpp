/* -*- c++ -*-
 * File : vci_cc_vcache_wrapper2_multi.cpp
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
#include "../include/vci_cc_vcache_wrapper2_multi.h"

namespace soclib { 
namespace caba {

//#define SOCLIB_MODULE_DEBUG
#ifdef SOCLIB_MODULE_DEBUG
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
        "ICACHE_SW_FLUSH", 
        "ICACHE_TLB_FLUSH", 
        "ICACHE_CACHE_FLUSH", 
        "ICACHE_TLB_INVAL",  
        "ICACHE_CACHE_INVAL",
	"ICACHE_CACHE_INVAL_PA",
        "ICACHE_MISS_WAIT",
        "ICACHE_UNC_WAIT",  
        "ICACHE_MISS_UPDT",  
        "ICACHE_ERROR", 
        "ICACHE_CC_INVAL", 
        "ICACHE_TLB_CC_INVAL",        
    };
const char *dcache_fsm_state_str[] = {
        "DCACHE_IDLE",       
        "DCACHE_BIS",   
        "DCACHE_DTLB1_READ_CACHE",
	"DCACHE_TLB1_LL_WAIT",
	"DCACHE_TLB1_SC_WAIT",    
        "DCACHE_TLB1_READ",
        "DCACHE_TLB1_READ_UPDT",  
        "DCACHE_TLB1_UPDT", 
        "DCACHE_DTLB2_READ_CACHE", 
	"DCACHE_TLB2_LL_WAIT",
	"DCACHE_TLB2_SC_WAIT",   
        "DCACHE_TLB2_READ",
        "DCACHE_TLB2_READ_UPDT",  
        "DCACHE_TLB2_UPDT",   
        "DCACHE_CTXT_SWITCH",   
        "DCACHE_ICACHE_FLUSH", 
        "DCACHE_DCACHE_FLUSH", 
        "DCACHE_ITLB_INVAL",
        "DCACHE_DTLB_INVAL",
        "DCACHE_ICACHE_INVAL",
        "DCACHE_DCACHE_INVAL",
	"DCACHE_ICACHE_INVAL_PA",
	"DCACHE_DCACHE_INVAL_PA",
	"DCACHE_DCACHE_SYNC",
	"DCACHE_LL_DIRTY_WAIT",
	"DCACHE_SC_DIRTY_WAIT",
        "DCACHE_WRITE_UPDT", 
        "DCACHE_WRITE_DIRTY",
        "DCACHE_WRITE_REQ",  
        "DCACHE_MISS_WAIT",  
        "DCACHE_MISS_UPDT",  
        "DCACHE_UNC_WAIT",   
        "DCACHE_ERROR", 
        "DCACHE_ITLB_READ",
        "DCACHE_ITLB_UPDT",
        "DCACHE_ITLB_LL_WAIT",        
        "DCACHE_ITLB_SC_WAIT",        
        "DCACHE_CC_CHECK",
        "DCACHE_CC_INVAL",
        "DCACHE_CC_UPDT",
        "DCACHE_CC_NOP",
        "DCACHE_TLB_CC_INVAL",
        "DCACHE_ITLB_CLEANUP",
    };
const char *cmd_fsm_state_str[] = {
        "CMD_IDLE",           
        "CMD_ITLB_READ", 
        "CMD_ITLB_ACC_LL",                
        "CMD_ITLB_ACC_SC",                
        "CMD_INS_MISS",     
        "CMD_INS_UNC",     
        "CMD_DTLB_READ",   
        "CMD_DTLB_ACC_LL",            
        "CMD_DTLB_ACC_SC",            
        "CMD_DTLB_DIRTY_LL",          
        "CMD_DTLB_DIRTY_SC",          
        "CMD_DATA_UNC",     
        "CMD_DATA_MISS",    
        "CMD_DATA_WRITE", 
    };
const char *rsp_fsm_state_str[] = {
        "RSP_IDLE",                  
        "RSP_ITLB_READ",             
        "RSP_ITLB_ACC_LL",                
        "RSP_ITLB_ACC_SC",                
        "RSP_INS_MISS",   
        "RSP_INS_UNC",           
        "RSP_DTLB_READ",            
        "RSP_DTLB_ACC_LL",            
        "RSP_DTLB_ACC_SC",            
        "RSP_DTLB_DIRTY_LL",          
        "RSP_DTLB_DIRTY_SC",          
        "RSP_DATA_MISS",             
        "RSP_DATA_UNC",              
        "RSP_DATA_WRITE",     
    };
const char *tgt_fsm_state_str[] = {
        "TGT_IDLE",
        "TGT_UPDT_WORD",
        "TGT_UPDT_DATA",
        "TGT_REQ_BROADCAST",
        "TGT_REQ_ICACHE",
        "TGT_REQ_DCACHE",
        "TGT_RSP_BROADCAST",
        "TGT_RSP_ICACHE",
        "TGT_RSP_DCACHE",
    };	
const char *inval_itlb_fsm_state_str[] = {
        "INVAL_ITLB_IDLE",        
        "INVAL_ITLB_CHECK"  , 
        "INVAL_ITLB_INVAL",      
        "INVAL_ITLB_CLEAR",           
    };
const char *inval_dtlb_fsm_state_str[] = {
        "INVAL_DTLB_IDLE",        
        "INVAL_DTLB_CHECK", 
        "INVAL_DTLB_INVAL",    
        "INVAL_DTLB_CLEAR",         
    };
const char *cleanup_fsm_state_str[] = {
        "CLEANUP_CMD",
	"CLEANUP_ICACHE_RSP",
        "CLEANUP_DCACHE_RSP",
    };
}
#endif

#define tmpl(...)  template<typename vci_param, typename iss_t> __VA_ARGS__ VciCcVCacheWrapper2Multi<vci_param, iss_t>

using soclib::common::uint32_log2;

/***********************************************/
tmpl(/**/)::VciCcVCacheWrapper2Multi(
    sc_module_name name,
    int proc_id,
    const soclib::common::MappingTable &mtp,
    const soclib::common::MappingTable &mtc,
    const soclib::common::IntTab &initiator_index_rw,
    const soclib::common::IntTab &initiator_index_c,
    const soclib::common::IntTab &target_index,
    size_t itlb_ways,
    size_t itlb_sets,
    size_t dtlb_ways,
    size_t dtlb_sets,
    size_t icache_ways,
    size_t icache_sets,
    size_t icache_words,
    size_t dcache_ways,
    size_t dcache_sets,
    size_t dcache_words,
    size_t wbuf_nwords,
    size_t wbuf_nlines )
/***********************************************/
    : soclib::caba::BaseModule(name),

      p_clk("clk"),
      p_resetn("resetn"),
      p_vci_ini_rw("vci_ini_rw"),
      p_vci_ini_c("vci_ini_c"),
      p_vci_tgt("vci_tgt"),

      m_cacheability_table(mtp.getCacheabilityTable()),
      m_segment(mtc.getSegment(target_index)),
      m_iss(this->name(), proc_id),
      m_srcid_rw(mtp.indexForId(initiator_index_rw)),
      m_srcid_c(mtp.indexForId(initiator_index_c)),

      m_itlb_ways(itlb_ways),
      m_itlb_sets(itlb_sets),

      m_dtlb_ways(dtlb_ways),
      m_dtlb_sets(dtlb_sets),

      m_icache_ways(icache_ways),
      m_icache_sets(icache_sets),
      m_icache_yzmask((~0)<<(uint32_log2(icache_words) + 2)),
      m_icache_words(icache_words),

      m_dcache_ways(dcache_ways),
      m_dcache_sets(dcache_sets),
      m_dcache_yzmask((~0)<<(uint32_log2(dcache_words) + 2)),
      m_dcache_words(dcache_words),

      m_wbuf_nlines(wbuf_nlines),
      m_paddr_nbits(vci_param::N),

      icache_tlb(itlb_ways,itlb_sets,vci_param::N),
      dcache_tlb(dtlb_ways,dtlb_sets,vci_param::N),

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

      r_dcache_tlb_ll_acc_req("r_dcache_tlb_ll_acc_req"),       
      r_dcache_tlb_sc_acc_req("r_dcache_tlb_sc_acc_req"),       
      r_dcache_tlb_ll_dirty_req("r_dcache_tlb_ll_dirty_req"),    
      r_dcache_tlb_sc_dirty_req("r_dcache_tlb_sc_dirty_req"), 
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
      r_dcache_itlb_ll_acc_req("r_dcache_itlb_ll_acc_req"),     
      r_dcache_itlb_sc_acc_req("r_dcache_itlb_sc_acc_req"),

      r_itlb_read_dcache_req("r_itlb_read_dcache_req"),
      r_itlb_k_read_dcache("r_itlb_k_read_dcache"),
      r_itlb_acc_dcache_req("r_itlb_acc_dcache_req"),
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
      r_vci_rsp_ins_error("r_vci_rsp_ins_error"),
      r_vci_rsp_data_error("r_vci_rsp_data_error"),
      r_vci_rsp_ins_ok("r_vci_rsp_ins_ok"),
      r_vci_rsp_data_ok("r_vci_rsp_data_ok"),

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
      r_dcache_itlb_inval_req("r_dcache_itlb_inval_req"),
      r_dcache_itlb_inval_line("r_dcache_itlb_inval_line"),
      r_itlb_cc_check_end("r_itlb_cc_check_end"),
      r_ccinval_itlb_way("r_ccinval_itlb_way"), 
      r_ccinval_itlb_set("r_ccinval_itlb_set"), 
      r_icache_inval_tlb_rsp("r_icache_inval_tlb_rsp"),
      r_icache_tlb_nline("r_icache_tlb_nline"),

      r_inval_dtlb_fsm("r_inval_dtlb_fsm"),          
      r_dcache_dtlb_inval_req("r_dcache_dtlb_inval_req"),
      r_dcache_dtlb_inval_line("r_dcache_dtlb_inval_line"),
      r_dtlb_cc_check_end("r_dtlb_cc_check_end"),
      r_ccinval_dtlb_way("r_ccinval_dtlb_way"), 
      r_ccinval_dtlb_set("r_ccinval_dtlb_set"), 
      r_dcache_inval_tlb_rsp("r_dcache_inval_tlb_rsp"),
      r_dcache_tlb_nline("r_dcache_tlb_nline"),

      r_dcache_itlb_cleanup_req("r_dcache_itlb_cleanup_req"),
      r_dcache_itlb_cleanup_line("r_dcache_itlb_cleanup_line"),

      r_dcache_dtlb_cleanup_req("r_dcache_dtlb_cleanup_req"),
      r_dcache_dtlb_cleanup_line("r_dcache_dtlb_cleanup_line"),
      r_cleanup_fsm("r_cleanup_fsm"),

      r_wbuf("wbuf", wbuf_nwords, wbuf_nlines ),
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

    typename iss_t::CacheInfo cache_info;
    cache_info.has_mmu = true;
    cache_info.icache_line_size = icache_words*sizeof(data_t);
    cache_info.icache_assoc = icache_ways;
    cache_info.icache_n_lines = icache_sets;
    cache_info.dcache_line_size = dcache_words*sizeof(data_t);
    cache_info.dcache_assoc = dcache_ways;
    cache_info.dcache_n_lines = dcache_sets;
    m_iss.setCacheInfo(cache_info);
}

/////////////////////////////////////
tmpl(/**/)::~VciCcVCacheWrapper2Multi()
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
    std::cout << name() << " CPI = " 
        << (float)m_cpt_total_cycles/(m_cpt_total_cycles - m_cpt_frz_cycles) << std::endl ;
}

////////////////////////
tmpl(void)::print_stats()
////////////////////////
{
    float run_cycles = (float)(m_cpt_total_cycles - m_cpt_frz_cycles);
    std::cout << name() << std::endl
    	      << "- CPI                    = " << (float)m_cpt_total_cycles/run_cycles << std::endl 
    	      << "- READ RATE              = " << (float)m_cpt_read/run_cycles << std::endl 
    	      << "- WRITE RATE             = " << (float)m_cpt_write/run_cycles << std::endl
    	      << "- UNCACHED READ RATE     = " << (float)m_cpt_unc_read/m_cpt_read << std::endl 
    	      << "- CACHED WRITE RATE      = " << (float)m_cpt_write_cached/m_cpt_write << std::endl 
    	      << "- IMISS_RATE             = " << (float)m_cpt_ins_miss/m_cpt_ins_read << std::endl    
    	      << "- DMISS RATE             = " << (float)m_cpt_data_miss/(m_cpt_read-m_cpt_unc_read) << std::endl 
    	      << "- INS MISS COST          = " << (float)m_cost_ins_miss_frz/m_cpt_ins_miss << std::endl
    	      << "- IMISS TRANSACTION      = " << (float)m_cost_imiss_transaction/m_cpt_imiss_transaction << std::endl
    	      << "- DMISS COST             = " << (float)m_cost_data_miss_frz/m_cpt_data_miss << std::endl
    	      << "- DMISS TRANSACTION      = " << (float)m_cost_dmiss_transaction/m_cpt_dmiss_transaction << std::endl
    	      << "- UNC COST               = " << (float)m_cost_unc_read_frz/m_cpt_unc_read << std::endl
    	      << "- UNC TRANSACTION        = " << (float)m_cost_unc_transaction/m_cpt_unc_transaction << std::endl
    	      << "- WRITE COST             = " << (float)m_cost_write_frz/m_cpt_write << std::endl
    	      << "- WRITE TRANSACTION      = " << (float)m_cost_write_transaction/m_cpt_write_transaction << std::endl
    	      << "- WRITE LENGTH           = " << (float)m_length_write_transaction/m_cpt_write_transaction << std::endl
    	      << "- INS TLB MISS RATE      = " << (float)m_cpt_ins_tlb_miss/m_cpt_ins_tlb_read << std::endl
    	      << "- DATA TLB MISS RATE     = " << (float)m_cpt_data_tlb_miss/m_cpt_data_tlb_read << std::endl
    	      << "- ITLB MISS TRANSACTION  = " << (float)m_cost_itlbmiss_transaction/m_cpt_itlbmiss_transaction << std::endl
    	      << "- ITLB WRITE TRANSACTION = " << (float)m_cost_itlb_write_transaction/m_cpt_itlb_write_transaction << std::endl
    	      << "- ITLB MISS COST         = " << (float)m_cost_ins_tlb_miss_frz/(m_cpt_ins_tlb_miss+m_cpt_ins_tlb_write_et) << std::endl
    	      << "- DTLB MISS TRANSACTION  = " << (float)m_cost_dtlbmiss_transaction/m_cpt_dtlbmiss_transaction << std::endl
    	      << "- DTLB WRITE TRANSACTION = " << (float)m_cost_dtlb_write_transaction/m_cpt_dtlb_write_transaction << std::endl
    	      << "- DTLB MISS COST         = " << (float)m_cost_data_tlb_miss_frz/(m_cpt_data_tlb_miss+m_cpt_data_tlb_write_et+m_cpt_data_tlb_write_dirty) << std::endl;
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
  	r_cleanup_fsm = CLEANUP_CMD;	

        // write buffer & caches
        r_wbuf.reset();
        r_icache.reset();
        r_dcache.reset();

        icache_tlb.reset();    
        dcache_tlb.reset();    

        std::memset(r_dcache_in_itlb, 0, sizeof(*r_dcache_in_itlb)*m_icache_ways*m_icache_sets);
        std::memset(r_dcache_in_dtlb, 0, sizeof(*r_dcache_in_dtlb)*m_dcache_ways*m_dcache_sets);

        //r_mmu_mode = ALL_DEACTIVE;
        r_mmu_mode = 0x3;
        r_mmu_params = (uint32_log2(m_dtlb_ways) << 29)   | (uint32_log2(m_dtlb_sets) << 25)   |
                       (uint32_log2(m_dcache_ways) << 22) | (uint32_log2(m_dcache_sets) << 18) |
                       (uint32_log2(m_itlb_ways) << 15)   | (uint32_log2(m_itlb_sets) << 11)   |
                       (uint32_log2(m_icache_ways) << 8)  | (uint32_log2(m_icache_sets) << 4)  |
                       (uint32_log2(m_icache_words * 4));
        r_mmu_release = (uint32_t)(1 << 16) | 0x1;

        r_icache_miss_req         = false;
        r_icache_unc_req          = false;
        r_dcache_itlb_read_req    = false;

        r_itlb_read_dcache_req    = false;      
        r_itlb_k_read_dcache      = false;      
        r_itlb_acc_dcache_req     = false;   
        r_dcache_rsp_itlb_error   = false;
 
        r_dcache_miss_req         = false;
        r_dcache_unc_req          = false;
        r_dcache_write_req        = false;
        r_dcache_tlb_read_req     = false;
        r_dcache_tlb_ptba_read    = false;
        r_dcache_xtn_req          = false;
	r_dcache_llsc_reserved    = false;

        r_dcache_tlb_ll_acc_req   = false;    
        r_dcache_tlb_sc_acc_req   = false;    
        r_dcache_tlb_ll_dirty_req = false;   
        r_dcache_tlb_sc_dirty_req = false;   
        r_dcache_itlb_ll_acc_req  = false;   
        r_dcache_itlb_sc_acc_req  = false;   

        r_icache_cleanup_req      = false;
        r_dcache_cleanup_req      = false;

        r_tgt_icache_req          = false;
        r_tgt_dcache_req          = false;

        r_icache_inval_rsp        = false;
        r_dcache_inval_rsp        = false;

        r_dcache_dirty_save       = false;
        r_dcache_hit_p_save       = false;

        r_vci_rsp_ins_error       = false;
        r_vci_rsp_data_error      = false;

	r_vci_rsp_ins_ok          = false;
        r_vci_rsp_data_ok         = false;

        r_icache_id1_save         = 0;
        r_icache_ppn_save         = 0;
        r_icache_vpn_save         = 0;
        r_itlb_translation_valid  = false;

        r_dcache_id1_save         = 0;
        r_dcache_ppn_save         = 0;
        r_dcache_vpn_save         = 0;
        r_dtlb_translation_valid  = false;

        r_icache_ptba_ok          = false;
        r_dcache_ptba_ok          = false;

        r_icache_error_type       = MMU_NONE;
        r_dcache_error_type       = MMU_NONE;

        // coherence registers
        r_icache_way              = 0;
        r_icache_set              = 0;
        r_icache_cleanup_req      = false;
        r_icache_inval_rsp        = false;

        r_dcache_way              = 0;
        r_dcache_set              = 0;
        r_dcache_cleanup_req      = false;
        r_dcache_inval_rsp        = false;

	r_itlb_inval_req 	  = false;
        r_dcache_itlb_inval_req   = false;
        r_itlb_cc_check_end       = false;
        r_ccinval_itlb_way        = 0; 
        r_ccinval_itlb_set        = 0; 
        r_icache_inval_tlb_rsp    = false;

        r_dcache_dtlb_inval_req   = false;
        r_dtlb_cc_check_end       = false;
        r_ccinval_dtlb_way        = 0; 
        r_ccinval_dtlb_set        = 0; 
        r_dcache_inval_tlb_rsp    = false;

        r_dcache_itlb_cleanup_req = false;
        r_dcache_dtlb_cleanup_req = false;

	r_dcache_cc_check         = false;

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

#ifdef SOCLIB_MODULE_DEBUG
std::cout << name() << "cycle = " << m_cpt_total_cycles  
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

#ifdef SOCLIB_MODULE_DEBUG
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
    //////////////
    case TGT_IDLE:
    {
        if ( p_vci_tgt.cmdval.read() ) 
        {
            paddr_t address = p_vci_tgt.address.read();

            if ( p_vci_tgt.cmd.read() != vci_param::CMD_WRITE) 
            {
                std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the received VCI command is not a write" << std::endl;
                exit(0);
            }

            // multi-update or multi-invalidate for data type
            if ( ( address != 0x3 ) && ( ! m_segment.contains(address)) ) 
            {
                std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                std::cout << "out of segment VCI command received for a multi-updt or multi-inval request" << std::endl;
                exit(0);
            }

            r_tgt_srcid = p_vci_tgt.srcid.read();
            r_tgt_trdid = p_vci_tgt.trdid.read();
            r_tgt_pktid = p_vci_tgt.pktid.read();
            r_tgt_plen  = p_vci_tgt.plen.read(); 
            r_tgt_addr  = (paddr_t)(p_vci_tgt.be.read() & 0x3) << 32 |
			  (paddr_t)p_vci_tgt.wdata.read() * m_dcache_words * 4; 

            if ( address == 0x3 ) // broadcast invalidate for data or instruction type
            {
                if ( ! p_vci_tgt.eop.read() ) 
                {
                    std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                    std::cout << "the BROADCAST INVALIDATE command length must be one word" << std::endl;
                    exit(0);
                }
                r_tgt_update = false; 
                r_vci_tgt_fsm = TGT_REQ_BROADCAST;
                m_cpt_cc_broadcast++;
            }
            else                // multi-update or multi-invalidate for data type
            {
                paddr_t cell = address - m_segment.baseAddress();   

                if (cell == 0)                      // invalidate   
                {                         
                    if ( ! p_vci_tgt.eop.read() ) 
                    {
                        std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "the MULTI-INVALIDATE command length must be one word" << std::endl;
                        exit(0);
                    }
                    r_tgt_update = false; 
                    r_vci_tgt_fsm = TGT_REQ_DCACHE;
                    m_cpt_cc_inval++ ;
                }
                else if (cell == 4)                // update 
                {                                
                    if ( p_vci_tgt.eop.read() ) 
                    {
                        std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "the MULTI-UPDATE command length must be N+2 words" << std::endl;
                        exit(0);
                    }
                    r_tgt_update = true; 
                    r_vci_tgt_fsm = TGT_UPDT_WORD;
                    m_cpt_cc_update++ ;
                }     
		else if (cell == 8)
		{
                    if ( ! p_vci_tgt.eop.read() ) 
                    {
                        std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                        std::cout << "the MULTI-INVALIDATE command length must be one word" << std::endl;
                        exit(0);
                    }
                    r_tgt_update = false; 
                    r_vci_tgt_fsm = TGT_REQ_ICACHE;
                    m_cpt_cc_inval++ ;

		}
            } // end if address    
        } // end if cmdval
        break;
    }
    ///////////////////
    case TGT_UPDT_WORD:
    {
        if (p_vci_tgt.cmdval.read()) 
        {
            if ( p_vci_tgt.eop.read() ) 
            {
                std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the MULTI-UPDATE command length must be N+2 words" << std::endl;
                exit(0);
            }
            for ( size_t i=0 ; i<m_dcache_words ; i++ ) r_tgt_val[i] = false;
            r_tgt_word = p_vci_tgt.wdata.read(); // the first modified word index
            r_vci_tgt_fsm = TGT_UPDT_DATA;
        }
        break;
    }
    ///////////////////
    case TGT_UPDT_DATA:
    {
        if (p_vci_tgt.cmdval.read()) 
        {
            size_t word = r_tgt_word.read();
            if (word >= m_dcache_words) 
            {
                std::cout << "error in component VCI_CC_VCACHE_WRAPPER " << name() << std::endl;
                std::cout << "the reveived MULTI-UPDATE command length is wrong" << std::endl;
                exit(0);
            }
            r_tgt_buf[word] = p_vci_tgt.wdata.read();
            if(p_vci_tgt.be.read())    r_tgt_val[word] = true;
            r_tgt_word = word + 1;
            if (p_vci_tgt.eop.read())  r_vci_tgt_fsm = TGT_REQ_DCACHE;
        }
        break;
    }
    ////////////////////////
    case TGT_REQ_BROADCAST:
    {
        if ( !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_RSP_BROADCAST; 
            r_tgt_icache_req = true;
            r_tgt_dcache_req = true;
        }
        break;
    }
    /////////////////////
    case TGT_REQ_ICACHE:
    {
        if ( !r_tgt_icache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_RSP_ICACHE; 
            r_tgt_icache_req = true;
        }
        break;
    }
    /////////////////////
    case TGT_REQ_DCACHE:
    {
        if ( !r_tgt_dcache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_RSP_DCACHE; 
            r_tgt_dcache_req = true;
        }
        break;
    }
    ///////////////////////
    case TGT_RSP_BROADCAST:
    {
        // no response
        if ( !r_tgt_icache_rsp.read() && !r_tgt_dcache_rsp.read() && !r_tgt_icache_req.read() && !r_tgt_dcache_req.read() )
        {
            r_vci_tgt_fsm = TGT_IDLE;
            break;
        }

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
    }
    /////////////////////
    case TGT_RSP_ICACHE:
    {
	if ( (p_vci_tgt.rspack.read() || !r_tgt_icache_rsp.read()) && !r_tgt_icache_req.read() ) 
        {
            r_vci_tgt_fsm = TGT_IDLE;
            r_tgt_icache_rsp = false; 
        }
        break;
    }
    /////////////////////
    case TGT_RSP_DCACHE:
    {
	if ( (p_vci_tgt.rspack.read() || !r_tgt_dcache_rsp.read()) && !r_tgt_dcache_req.read() )
        {
            r_vci_tgt_fsm = TGT_IDLE;
            r_tgt_dcache_rsp = false; 
        }
        break;
    }
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
        paddr_t     tlb_ipaddr       = 0;    	// physical address obtained from TLB                         
        paddr_t     spc_ipaddr       = 0;    	// physical adress obtained from PPN_save (speculative)       
        data_t      icache_ins       = 0;    	// read instruction
        bool        icache_hit_c     = false;	// Cache hit
        bool        icache_cached    = false;	// cacheable access (read)
        bool        icache_hit_t     = false;	// hit on TLB
        bool        icache_hit_x     = false;	// VPN unmodified (can use spc_dpaddr)
        bool        icache_hit_p     = false;	// PTP unmodified (can skip first level page table walk)
        size_t      icache_tlb_way   = 0;    	// selected way (in case of cache hit)
        size_t      icache_tlb_set   = 0;    	// selected set (Y field in address)
        paddr_t     icache_tlb_nline = 0;  	// TLB NLINE 

        // Decoding processor XTN requests
        // They are sent by DCACHE FSM  

        if (r_dcache_xtn_req)
        {
            if ( ireq.valid ) m_cost_ins_waste_wait_frz++;
            
            if ((int)r_dcache_type_save == (int)iss_t::XTN_PTPR )  
            {
                r_icache_way = 0;
                r_icache_set = 0;
                r_icache_fsm = ICACHE_SW_FLUSH;   
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
            if ((int)r_dcache_type_save == (int)iss_t::XTN_MMU_ICACHE_PA_INV) 
            {
                r_icache_fsm = ICACHE_CACHE_INVAL_PA;   
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
            if ( ireq.valid ) m_cost_ins_waste_wait_frz++;
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
            if ( ireq.valid ) m_cost_ins_waste_wait_frz++;
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            break;
        }

        // icache_hit_t_m, icache_hit_t_k, icache_hit_x, icache_hit_p 
        // icache_pte_info, icache_tlb_way, icache_tlb_set & ipaddr & cacheability 
        // - If MMU activated : cacheability is defined by the cachable bit in the TLB
        // - If MMU not activated : cacheability is defined by the segment table.

        if ( !(r_mmu_mode.read() & INS_TLB_MASK) )   // MMU not activated 
        {
            icache_hit_t  = true;         
            icache_hit_x  = true;         
            icache_hit_p  = true;         
            tlb_ipaddr    = ireq.addr;
            spc_ipaddr    = ireq.addr;
            icache_cached = m_cacheability_table[ireq.addr];
        } 
        else                                                                // MMU activated
        { 
            m_cpt_ins_tlb_read++;
            icache_hit_t  = icache_tlb.cctranslate(ireq.addr, &tlb_ipaddr, &icache_pte_info, 
                                                   &icache_tlb_nline, &icache_tlb_way, &icache_tlb_set); 
            icache_hit_x  = (((vaddr_t)r_icache_vpn_save << PAGE_K_NBITS) == (ireq.addr & ~PAGE_K_MASK)) && r_itlb_translation_valid;
            icache_hit_p  = (((ireq.addr >> PAGE_M_NBITS) == r_icache_id1_save) && r_icache_ptba_ok); 
            spc_ipaddr    = ((paddr_t)r_icache_ppn_save << PAGE_K_NBITS) | (paddr_t)(ireq.addr & PAGE_K_MASK);
            icache_cached = icache_pte_info.c; 
        }

        if ( !(r_mmu_mode.read() & INS_CACHE_MASK) )   // cache not actived
        {
            icache_cached = false;
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
                icache_hit_c = ( r_vci_rsp_ins_ok && (tlb_ipaddr == (paddr_t)r_icache_paddr_save) );
                icache_ins = r_icache_miss_buf[0];
            }

            if ( r_mmu_mode.read() & INS_TLB_MASK ) 
            {
                if ( icache_hit_t ) 
                {
                    // check access rights
                    if ( !icache_pte_info.u && (ireq.mode == iss_t::MODE_USER)) 
                    {
                        r_icache_error_type = MMU_READ_PRIVILEGE_VIOLATION;  
                        r_icache_bad_vaddr = ireq.addr;
                        irsp.valid = true;
                        irsp.error = true;
                        irsp.instruction = 0;
                        break;
                    }
                    if ( !icache_pte_info.x ) 
                    {
                        r_icache_error_type = MMU_READ_EXEC_VIOLATION;  
                        r_icache_bad_vaddr = ireq.addr;
                        irsp.valid = true;
                        irsp.error = true;
                        irsp.instruction = 0;
                        break;
                    }
                }

                // update LRU, save ppn, vpn and page type
                if ( icache_hit_t )
                {  
                    icache_tlb.setlru(icache_tlb_way,icache_tlb_set);     
                    r_icache_ppn_save = tlb_ipaddr >> PAGE_K_NBITS;
                    r_icache_vpn_save = ireq.addr >> PAGE_K_NBITS;
                    r_icache_tlb_nline = icache_tlb_nline;
                    r_itlb_translation_valid = true;
                }
                else
                {
                    r_itlb_translation_valid = false;
                }

            } // end if MMU activated

            // compute next state 
            if ( !icache_hit_t && !icache_hit_p )      // TLB miss
            {
                // walk page table  level 1
                r_icache_paddr_save = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((ireq.addr>>PAGE_M_NBITS)<<2);
                r_itlb_read_dcache_req = true;
		r_icache_vaddr_req = ireq.addr;
                r_icache_fsm = ICACHE_TLB1_READ;
                m_cpt_ins_tlb_miss++;
                m_cost_ins_tlb_miss_frz++;
            }
            else if ( !icache_hit_t && icache_hit_p )  // TLB Miss with possibility of bypass first level page
            {
                // walk page table level 2
                r_icache_paddr_save = (paddr_t)r_icache_ptba_save | 
                                      (paddr_t)(((ireq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 3);
                r_itlb_read_dcache_req = true;
		r_icache_vaddr_req = ireq.addr;
                r_icache_fsm = ICACHE_TLB2_READ;
                m_cpt_ins_tlb_miss++;
                m_cost_ins_tlb_miss_frz++;
            }
            else if ( icache_hit_t && !icache_hit_x && icache_cached ) // cached access with an ucorrect speculative physical address
            {
                r_icache_paddr_save = tlb_ipaddr;   // save actual physical address for BIS
		r_icache_vaddr_req = ireq.addr;
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
                    if ( icache_cached ) 
                    {
                        r_icache_miss_req = true;
			r_vci_rsp_ins_ok = false;
                    	r_icache_paddr_save = spc_ipaddr; 
			r_icache_vaddr_req = ireq.addr;
                        r_icache_fsm = ICACHE_MISS_WAIT;
                    } 
                    else 
                    {
                        r_icache_unc_req = true;
			r_vci_rsp_ins_ok = false;
                    	r_icache_paddr_save = tlb_ipaddr; 
			r_icache_vaddr_req = ireq.addr;
                        r_icache_fsm = ICACHE_UNC_WAIT;
                    } 
                } 
                else 
                {
		    r_vci_rsp_ins_ok = false;
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
        m_cost_ins_miss_frz++;
        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }
      
        // using page is invalidated 
        if ( r_icache_inval_tlb_rsp )
        {
            r_icache_inval_tlb_rsp = false;
            r_icache_fsm = ICACHE_IDLE;
            m_cost_ins_tlb_miss_frz++;
            break;
        }
 
        data_t	icache_ins = 0;
        bool	icache_hit_c = false;
        bool    icache_hit_t = false;
        paddr_t	tlb_ipaddr = 0;

	icache_hit_t = icache_tlb.translate(ireq.addr, &tlb_ipaddr);

	if ( (tlb_ipaddr == r_icache_paddr_save.read()) && ireq.valid && icache_hit_t )		// unmodified & valid
	{
            m_cpt_ins_read++;

            // acces is always cached in this state
            icache_hit_c = r_icache.read(r_icache_paddr_save, &icache_ins);

            if ( !icache_hit_c )
            {
                r_icache_miss_req = true;
		r_vci_rsp_ins_ok = false;
                r_icache_fsm = ICACHE_MISS_WAIT;
                m_cpt_ins_miss++;
            } 
            else
            {
                r_icache_fsm = ICACHE_IDLE; 
            }
            irsp.valid = icache_hit_c;
	    if (irsp.valid)
	      assert((r_icache_vaddr_req.read() == ireq.addr) &&
	          "vaddress should not be modified while ICACHE_BIS");
            irsp.error = false;
            irsp.instruction = icache_ins;
	}
	else	// modified or invalid
	{
            irsp.valid = false;
            irsp.error = false;
            irsp.instruction = 0;
            r_icache_fsm = ICACHE_IDLE;
	}
        break;
    }
    //////////////////////
    case ICACHE_TLB1_READ:
    {
        m_cost_ins_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

	if ( !r_itlb_read_dcache_req )
	{
	    if (r_icache_vaddr_req.read() != ireq.addr || !ireq.valid) 
	    {
		/* request modified, drop response and restart */
		r_icache_ptba_ok = false;
            	if ( r_icache_inval_tlb_rsp )	r_icache_inval_tlb_rsp = false;
                if ( r_dcache_rsp_itlb_error )	r_dcache_rsp_itlb_error = false;
		r_icache_fsm = ICACHE_IDLE;
		break;
	    }

            if ( !r_icache_inval_tlb_rsp ) // TLB miss read response and no invalidation
            {
                if ( !r_dcache_rsp_itlb_error ) // vci response ok
                {  
            	    if ( !(r_dcache_rsp_itlb_miss >> PTE_V_SHIFT) ) // unmapped
            	    {
                	r_icache_ptba_ok    = false;	
                        r_icache_error_type = MMU_READ_PT1_UNMAPPED;  
                        r_icache_bad_vaddr  = r_icache_vaddr_req.read();
                        r_icache_fsm        = ICACHE_ERROR;
            	    }
            	    else if ( (r_dcache_rsp_itlb_miss & PTE_T_MASK ) >> PTE_T_SHIFT ) // PTD
            	    {
                	r_icache_ptba_ok       = true;	
                        r_icache_ptba_save     = (paddr_t)(r_dcache_rsp_itlb_miss & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS; 
                        r_icache_id1_save      = r_icache_vaddr_req.read() >> PAGE_M_NBITS;
                        r_icache_paddr_save    = (paddr_t)(r_dcache_rsp_itlb_miss & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS |
                                                 (paddr_t)(((r_icache_vaddr_req.read() & PTD_ID2_MASK) >> PAGE_K_NBITS) << 3); 
                        r_itlb_read_dcache_req = true;
			r_itlb_k_read_dcache   = true;
                        r_icache_fsm           = ICACHE_TLB2_READ;
             	    }	
            	    else
            	    {
                	r_icache_ptba_ok = false;
            
            	        if ( (m_srcid_rw >> 4) == ((r_dcache_rsp_itlb_miss & ((1<<(m_paddr_nbits - PAGE_M_NBITS))-1)) >> (m_paddr_nbits - PAGE_M_NBITS -10)) ) // local
            	        {
            		    if ( (r_dcache_rsp_itlb_miss & PTE_L_MASK ) >> PTE_L_SHIFT ) // L bit is set
            		    {
                                r_icache_pte_update = r_dcache_rsp_itlb_miss;
                                r_icache_fsm        = ICACHE_TLB1_UPDT;
            		    }
            		    else
            		    {
                                r_icache_pte_update   = r_dcache_rsp_itlb_miss | PTE_L_MASK;
                                r_itlb_acc_dcache_req = true;
                                r_icache_fsm          = ICACHE_TLB1_WRITE;
                                m_cpt_ins_tlb_write_et++;
            		    }
                        }
            	        else // remotely
            	        {
            		    if ( (r_dcache_rsp_itlb_miss & PTE_R_MASK ) >> PTE_R_SHIFT ) // R bit is set
            		    {
                                r_icache_pte_update = r_dcache_rsp_itlb_miss;
                                r_icache_fsm        = ICACHE_TLB1_UPDT;
            		    }
            		    else
            		    {
                                r_icache_pte_update   = r_dcache_rsp_itlb_miss | PTE_R_MASK;
                                r_itlb_acc_dcache_req = true;
                                r_icache_fsm          = ICACHE_TLB1_WRITE;
                                m_cpt_ins_tlb_write_et++;
            		    }
            	        }
            	    }
                }
                else                        // vci response error
                {  
                    r_icache_fsm = ICACHE_ERROR;
                    r_icache_error_type = MMU_READ_PT1_ILLEGAL_ACCESS;    
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                }
            }

            if ( r_icache_inval_tlb_rsp ) // TLB miss read response and invalidation
            {
                if ( r_dcache_rsp_itlb_error ) 
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_error_type = MMU_READ_PT1_ILLEGAL_ACCESS;    
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                    r_icache_fsm = ICACHE_ERROR;
                } 
                else 
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_fsm = ICACHE_IDLE;  
                } 
            }
	}
        break;
    }
    ///////////////////////
    case ICACHE_TLB1_WRITE:  
    {
        m_cost_ins_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }
        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        if ( !r_itlb_acc_dcache_req ) // TLB access bits write response 
	{        
            if ( !r_icache_inval_tlb_rsp ) // TLB access bits write response and no invalidation        
            { 
                if ( r_dcache_rsp_itlb_error ) 
                {
                    r_icache_error_type = MMU_READ_PT1_ILLEGAL_ACCESS;  
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                    r_icache_fsm = ICACHE_ERROR;
                } 
                else  
                {
                    r_icache_fsm = ICACHE_TLB1_UPDT;  
                }
            } 

            if ( r_icache_inval_tlb_rsp) // TLB ET write response and invalidation     
            {   
                if ( r_dcache_rsp_itlb_error ) 
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_error_type = MMU_READ_PT1_ILLEGAL_ACCESS;  
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                    r_icache_fsm = ICACHE_ERROR;
                } 
                else  
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_fsm = ICACHE_IDLE;    
                }
            }
	}
        break;
    }
    //////////////////////
    case ICACHE_TLB1_UPDT:
    {
        m_cost_ins_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req ) 
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // TLB update and invalidate different PTE
        if ( !r_dcache_itlb_cleanup_req && !r_icache_inval_tlb_rsp )  
        {
            paddr_t victim_index = 0;
            r_dcache_itlb_cleanup_req = icache_tlb.update(r_icache_pte_update,r_icache_vaddr_req.read(),(r_icache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index);
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
        m_cost_ins_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        if ( !r_itlb_read_dcache_req  ) // TLB miss read response
	{ 
	    if (r_icache_vaddr_req.read() != ireq.addr || !ireq.valid) 
	    {
		/* request modified, drop response and restart */
		r_icache_ptba_ok = false;
            	if ( r_icache_inval_tlb_rsp )	r_icache_inval_tlb_rsp = false;
                if ( r_dcache_rsp_itlb_error )	r_dcache_rsp_itlb_error = false;
		r_icache_fsm = ICACHE_IDLE;
		break;
	    }

            if ( !r_icache_inval_tlb_rsp ) // TLB miss read response 
            {
                if ( !r_dcache_rsp_itlb_error ) // VCI response ok        
                {
            	    if ( !(r_dcache_rsp_itlb_miss >> PTE_V_SHIFT) ) // unmapped
            	    {
                        r_icache_error_type = MMU_READ_PT2_UNMAPPED;  
                        r_icache_bad_vaddr  = r_icache_vaddr_req.read();
                        r_icache_fsm = ICACHE_ERROR;
            	    }
            	    else
            	    {
            	        if ( (m_srcid_rw >> 4) == ((r_dcache_rsp_itlb_miss & ((1<<(m_paddr_nbits - PAGE_M_NBITS))-1)) >> (m_paddr_nbits - PAGE_M_NBITS -10)) ) // local
            	        {
            		    if ( (r_dcache_rsp_itlb_miss & PTE_L_MASK ) >> PTE_L_SHIFT ) // L bit is set
            		    {
                        	r_icache_fsm        = ICACHE_TLB2_UPDT;
                        	r_icache_pte_update = r_dcache_rsp_itlb_miss;
            		    }
            		    else
            		    {
                        	r_icache_pte_update   = r_dcache_rsp_itlb_miss | PTE_L_MASK;
                        	r_itlb_acc_dcache_req = true;
                        	r_icache_fsm          = ICACHE_TLB2_WRITE;
                         	m_cpt_ins_tlb_write_et++;
            		    }
                        }
            	    	else // remotely
            	    	{
            		    if ( (r_dcache_rsp_itlb_miss & PTE_R_MASK ) >> PTE_R_SHIFT ) // R bit is set
            		    {
                        	r_icache_fsm        = ICACHE_TLB2_UPDT;
                        	r_icache_pte_update = r_dcache_rsp_itlb_miss;
            		    }
            		    else
            		    {
                        	r_icache_pte_update   = r_dcache_rsp_itlb_miss | PTE_R_MASK;
                        	r_itlb_acc_dcache_req = true;
                        	r_icache_fsm          = ICACHE_TLB2_WRITE;
                        	m_cpt_ins_tlb_write_et++;
            		    }
            	        }
            	    }
                }
                else                            // VCI response error        
                {
                    r_icache_error_type = MMU_READ_PT2_ILLEGAL_ACCESS;
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                    r_icache_fsm = ICACHE_ERROR;
                }
            }
            
            if ( r_icache_inval_tlb_rsp ) // TLB miss read response and invalidation
            {
                if ( r_dcache_rsp_itlb_error ) 
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_error_type = MMU_READ_PT2_ILLEGAL_ACCESS;    
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                    r_icache_fsm = ICACHE_ERROR;
                } 
                else 
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_fsm = ICACHE_IDLE;  
                } 
            }
	}
        break;
    }
    /////////////////////////
    case ICACHE_TLB2_WRITE:
    {  
        m_cost_ins_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }
        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        if ( !r_itlb_acc_dcache_req ) // TLB access bits write response          
	{
            if ( !r_icache_inval_tlb_rsp ) // TLB access bits write response          
            {
                if ( r_dcache_rsp_itlb_error )             
                {
                    r_icache_error_type = MMU_READ_PT2_ILLEGAL_ACCESS;  
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                    r_icache_fsm = ICACHE_ERROR;
                } 
                else  
                {
                    r_icache_fsm = ICACHE_TLB2_UPDT;  
                }
            }
 
            if ( r_icache_inval_tlb_rsp ) // TLB ET write response and invalidation     
            {   
                if ( r_dcache_rsp_itlb_error ) 
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_error_type = MMU_READ_PT2_ILLEGAL_ACCESS;  
                    r_icache_bad_vaddr = r_icache_vaddr_req.read();
                    r_icache_fsm = ICACHE_ERROR;
                } 
                else  
                {
                    r_icache_inval_tlb_rsp = false;
                    r_icache_fsm = ICACHE_IDLE;    
                }
            }
	}
        break;
    }
    /////////////////////
    case ICACHE_TLB2_UPDT: 
    {
        m_cost_ins_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req ) 
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // TLB update and invalidate different PTE
        if ( !r_dcache_itlb_cleanup_req && !r_icache_inval_tlb_rsp ) 
        {
            paddr_t victim_index = 0;
            r_dcache_itlb_cleanup_req = icache_tlb.update(r_icache_pte_update,r_dcache_rsp_itlb_ppn,r_icache_vaddr_req.read(),(r_icache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index);
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
    case ICACHE_SW_FLUSH:
    {
        size_t way = r_icache_way;
        size_t set = r_icache_set;
        bool clean = false;

        if ( ireq.valid ) m_cost_ins_waste_wait_frz++;
        m_cost_ins_tlb_sw_frz++;

        // 4K page size TLB flush leads to cleanup req to data cache 
        if ( !r_dcache_itlb_cleanup_req )    // last cleanup finish
        {
            paddr_t victim_index = 0;
            for ( ; way < m_itlb_ways; way++)
            {
                for ( ; set < m_itlb_sets; set++)
                {
                    if(icache_tlb.checkcleanup(way, set, &victim_index))
                    {
                        clean = true;
                        r_dcache_itlb_cleanup_req = true;
                        r_dcache_itlb_cleanup_line = victim_index;
                        r_icache_way = way + ((set+1)/m_itlb_sets);
                        r_icache_set = (set+1) % m_itlb_sets;
                        break;
                    }
                }
                if (clean) break;
            }

            if (way == m_itlb_ways)
            {
                r_dcache_xtn_req = false;
        	r_itlb_translation_valid = false;
        	r_icache_ptba_ok = false;
                r_icache_fsm = ICACHE_IDLE;
                break;
            }
        }
        break;
    }
    /////////////////////
    case ICACHE_TLB_FLUSH:
    {   
        if ( ireq.valid ) m_cost_ins_waste_wait_frz++;

        // data cache flush leads to ins tlb flush, flush all tlb entry 
        icache_tlb.flush(true);    // global entries are invalidated
        r_dcache_xtn_req = false;
        r_itlb_translation_valid = false;
        r_icache_ptba_ok = false;
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

        m_cost_ins_cache_flush_frz++;
        if ( ireq.valid ) m_cost_ins_waste_wait_frz++;

        // cache flush and send cleanup to external
        if ( !r_icache_cleanup_req )
        {
            paddr_t victim_index = 0;
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
            if (way == m_icache_ways)
            {
                r_dcache_xtn_req = false;
                r_icache_fsm = ICACHE_IDLE;
                break;
            }
        }
        break;
    }
    /////////////////////
    case ICACHE_TLB_INVAL:  
    {
        if ( ireq.valid ) m_cost_ins_waste_wait_frz++;
        paddr_t victim_index = 0;

        if ( !r_dcache_itlb_cleanup_req )
        {
            r_dcache_itlb_cleanup_req = icache_tlb.inval(r_dcache_wdata_save, &victim_index);
            r_dcache_itlb_cleanup_line = victim_index;
            r_dcache_xtn_req = false;
            r_itlb_translation_valid = false;
            r_icache_ptba_ok = false;
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
    }
    ////////////////////////
    case ICACHE_CACHE_INVAL:
    {	
        if ( ireq.valid ) m_cost_ins_waste_wait_frz++;

        paddr_t ipaddr = 0;                     
        bool    icache_hit_t = false;

        if ( !r_icache_cleanup_req )
        {    
            if ( r_mmu_mode.read() & INS_TLB_MASK ) 
            {
                icache_hit_t = icache_tlb.translate(r_dcache_wdata_save, &ipaddr); 
            } 
            else 
            {
                ipaddr = (paddr_t)r_dcache_wdata_save;
                icache_hit_t = true;
            }
            if ( icache_hit_t )
            {
                // invalidate and cleanup if necessary
                r_icache_cleanup_req = r_icache.inval(ipaddr);
                r_icache_cleanup_line = ipaddr >> (uint32_log2(m_icache_words) + 2);   
            }
            r_dcache_xtn_req = false; 
            r_icache_fsm = ICACHE_IDLE;
        }
        break;
    }
    ////////////////////////
    case ICACHE_CACHE_INVAL_PA:
    {	
        paddr_t ipaddr = (paddr_t)r_mmu_word_hi.read() << 32 | r_mmu_word_lo.read();

        if ( !r_icache_cleanup_req )
        {    
            // invalidate and cleanup if necessary
            r_icache_cleanup_req = r_icache.inval(ipaddr);
            r_icache_cleanup_line = ipaddr >> (uint32_log2(m_icache_words) + 2);   
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
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }
       
	if ( r_vci_rsp_ins_ok )
	{
	    if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = MMU_READ_DATA_ILLEGAL_ACCESS; 
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;

            	if ( r_icache_inval_tlb_rsp ) r_icache_inval_tlb_rsp = false;
            	if ( r_icache_inval_rsp ) r_icache_inval_rsp = false;
		break;
            }

            if ( r_icache_inval_tlb_rsp ) // Miss read response and tlb invalidation
            {
		if ( r_icache_cleanup_req ) break;
                r_icache_cleanup_req = true;
                r_icache_cleanup_line = r_icache_paddr_save.read() >> (uint32_log2(m_icache_words) + 2);  
                r_icache_fsm = ICACHE_IDLE;
                r_icache_inval_tlb_rsp = false;
                if ( r_icache_inval_rsp ) r_icache_inval_rsp = false;
                m_cost_ins_tlb_miss_frz++;
                break;
            }
	
            if ( r_icache_inval_rsp ) // Miss read response and tlb invalidation
            {
		if ( r_icache_cleanup_req ) break;
                r_icache_cleanup_req = true;
                r_icache_cleanup_line = r_icache_paddr_save.read() >> (uint32_log2(m_icache_words) + 2);  
                r_icache_fsm = ICACHE_IDLE;
                r_icache_inval_rsp = false;
                break;
            }
            r_icache_fsm = ICACHE_MISS_UPDT;  
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
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_ins_ok )
	{
	    if ( r_vci_rsp_ins_error ) 
            {
                r_icache_error_type = MMU_READ_DATA_ILLEGAL_ACCESS;    
                r_icache_bad_vaddr = ireq.addr;
                r_icache_fsm = ICACHE_ERROR;

            	if ( r_icache_inval_tlb_rsp ) r_icache_inval_tlb_rsp = false;
		break;
            }

	    if ( r_icache_inval_tlb_rsp ) // Miss read response and tlb invalidation
            {
                r_icache_inval_tlb_rsp = false;
                r_icache_fsm = ICACHE_IDLE;
		break;
            }

	    // Miss read response and no invalidation
            r_icache_fsm = ICACHE_IDLE;
	}	
        break;
    }
    //////////////////////
    case ICACHE_MISS_UPDT:
    {
        m_cost_ins_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_icache_req )   
        {
            r_icache_fsm = ICACHE_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        // external tlb invalidate request
        if ( r_dcache_itlb_inval_req )
        {
	    r_itlb_inval_req = true;
            r_icache_fsm = ICACHE_TLB_CC_INVAL;
            r_icache_fsm_save = r_icache_fsm;
            m_cost_ins_waste_wait_frz++;
            break;
        }

        if ( r_icache_inval_tlb_rsp ) // tlb invalidation
        {
            if ( r_icache_cleanup_req ) break;
            r_icache_cleanup_req = true;
            r_icache_cleanup_line = r_icache_paddr_save.read() >> (uint32_log2(m_icache_words) + 2);  
            r_icache_inval_tlb_rsp = false;
            if ( r_icache_inval_rsp ) r_icache_inval_rsp = false;
            r_icache_fsm = ICACHE_IDLE;
            m_cost_ins_tlb_miss_frz++;
            break;
        }

        if ( !r_icache_cleanup_req ) // Miss update and no invalidation
        {
            if ( r_icache_inval_rsp ) // invalidation
            {
                r_icache_cleanup_req = true;
                r_icache_cleanup_line = r_icache_paddr_save.read() >> (uint32_log2(m_icache_words) + 2);  
                r_icache_fsm = ICACHE_IDLE;
                r_icache_inval_rsp = false;
            } 
	    else
	    {
                data_t* buf = r_icache_miss_buf;
                paddr_t  victim_index = 0;
                m_cpt_icache_dir_write++;
                m_cpt_icache_data_write++;

                r_icache_cleanup_req = r_icache.update(r_icache_paddr_save.read(), buf, &victim_index);
                r_icache_cleanup_line = victim_index;            

                r_icache_fsm = ICACHE_IDLE;
	    }
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
        if ( ireq.valid ) m_cost_ins_waste_wait_frz++;
        m_cpt_icache_dir_read += m_icache_ways;

        // invalidate cache
        if( (( r_icache_fsm_save == ICACHE_MISS_WAIT ) || ( r_icache_fsm_save == ICACHE_MISS_UPDT ) /*|| 
             ( r_icache_fsm_save == ICACHE_UNC_WAIT )*/ ) && 
            ((r_icache_paddr_save.read() & ~((m_icache_words<<2)-1)) == (r_tgt_addr.read() & ~((m_icache_words<<2)-1))) ) 
        {
            r_icache_inval_rsp = true;
	    r_tgt_icache_rsp = false;
        } 
        else 
        {
            r_tgt_icache_rsp = r_icache.inval(r_tgt_addr.read());
        }
        r_tgt_icache_req = false;
        r_icache_fsm = r_icache_fsm_save;
        break;
    }
    /////////////////////////
    case ICACHE_TLB_CC_INVAL:
    {
        if ( ireq.valid ) m_cost_ins_waste_wait_frz++;        

	if ( r_itlb_inval_req ) break;
        // invalidate cache
        if( (( r_icache_fsm_save == ICACHE_TLB1_READ ) || ( r_icache_fsm_save == ICACHE_TLB2_READ )  ||
          /* ( r_icache_fsm_save == ICACHE_TLB1_WRITE )|| ( r_icache_fsm_save == ICACHE_TLB2_WRITE ) ||*/
             ( r_icache_fsm_save == ICACHE_TLB1_UPDT ) || ( r_icache_fsm_save == ICACHE_TLB2_UPDT )) && 
            (((r_icache_paddr_save.read() & ~((m_icache_words<<2)-1)) >> (uint32_log2(m_icache_words) + 2) ) == r_dcache_itlb_inval_line.read()) ) 
        {
            r_icache_inval_tlb_rsp = true;
        } 
        else if (((r_icache_fsm_save == ICACHE_BIS)||(r_icache_fsm_save == ICACHE_MISS_WAIT) ||
              /*  (r_icache_fsm_save == ICACHE_UNC_WAIT)||*/(r_icache_fsm_save == ICACHE_MISS_UPDT)) && 
                (r_icache_tlb_nline == r_dcache_itlb_inval_line))
        {
            r_icache_inval_tlb_rsp = true;
        }
	r_dcache_itlb_inval_req = false;
        r_itlb_translation_valid = false;
        r_icache_ptba_ok = false;
        r_icache_fsm = r_icache_fsm_save;
        break;
    }
    } // end switch r_icache_fsm

#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Instruction Response: " << irsp << std::endl;
#endif

    ////////////////////////////////////////////////////////////////////////////////////
    //      INVAL ITLB CHECK FSM 
    ////////////////////////////////////////////////////////////////////////////////////////
    switch(r_inval_itlb_fsm) {
    /////////////////////
    case INVAL_ITLB_IDLE:
    {
        if ( r_itlb_inval_req )
        {
            r_ccinval_itlb_way = 0; 
            r_ccinval_itlb_set = 0;
            r_inval_itlb_fsm = INVAL_ITLB_CHECK;   
            m_cost_ins_tlb_inval_frz++; 
        }   
        break;
    }
    ////////////////////////////
    case INVAL_ITLB_CHECK:
    {
        m_cost_ins_tlb_inval_frz++; 

        size_t way = r_ccinval_itlb_way; 
        size_t set = r_ccinval_itlb_set;
        bool end = false;        
        bool tlb_hit = icache_tlb.cccheck(r_dcache_itlb_inval_line.read(), way, set, &way, &set, &end); 
    
        if ( tlb_hit )
        {
            r_ccinval_itlb_way = way; 
            r_ccinval_itlb_set = set;
            r_itlb_cc_check_end = end;
            r_inval_itlb_fsm = INVAL_ITLB_INVAL; 
            m_cpt_ins_tlb_inval++;   
        }        
        else
        {
            r_inval_itlb_fsm = INVAL_ITLB_CLEAR;    
        }
        break;
    }
    /////////////////////////
    case INVAL_ITLB_INVAL:
    {
        m_cost_ins_tlb_inval_frz++;
 
        icache_tlb.ccinval(r_ccinval_itlb_way, r_ccinval_itlb_set);

        if ( !r_itlb_cc_check_end )
        {
            r_inval_itlb_fsm = INVAL_ITLB_CHECK; 
        }
        else
        {
            r_inval_itlb_fsm = INVAL_ITLB_CLEAR;    
        }
        break;
    }
    ////////////////////
    case INVAL_ITLB_CLEAR:
    {
        r_itlb_inval_req = false;
        r_itlb_cc_check_end = false;
        r_ccinval_itlb_way = 0; 
        r_ccinval_itlb_set = 0; 
        r_inval_itlb_fsm = INVAL_ITLB_IDLE;    
        m_cost_ins_tlb_inval_frz++; 
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
            if ( dreq.valid ) m_cost_data_waste_wait_frz++; 
            break;
        }

	if ( !r_wbuf.wok(r_dcache_paddr_save) )
        {
            m_cost_write_frz++;
            drsp.valid = false;
            drsp.rdata = 0;
            break;
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
            r_dcache_fsm_save = DCACHE_IDLE;
            if ( dreq.valid ) m_cost_data_waste_wait_frz++;
            break;
        }        

        // ins tlb cleanup
        if ( r_dcache_itlb_cleanup_req )
        {
            r_dcache_fsm = DCACHE_ITLB_CLEANUP;
            m_cpt_ins_tlb_cleanup++;
            if ( dreq.valid ) m_cost_data_waste_wait_frz++;
            break;
        }    
        // ins tlb miss
    	if ( r_itlb_read_dcache_req )
    	{
            data_t rsp_itlb_miss;
	    data_t rsp_itlb_ppn;

    	    bool itlb_hit_dcache = r_dcache.read(r_icache_paddr_save, &rsp_itlb_miss);
	    if ( (r_icache_fsm == ICACHE_TLB2_READ) && itlb_hit_dcache )
	    {	
	        bool itlb_hit_ppn = r_dcache.read(r_icache_paddr_save.read()+4, &rsp_itlb_ppn);
		assert(itlb_hit_ppn && "Address of pte[64-32] and pte[31-0] should be successive");
	    }

            m_cpt_dcache_data_read += m_dcache_ways;
            m_cpt_dcache_dir_read += m_dcache_ways;

    	    if ( itlb_hit_dcache )  // ins TLB request hits in data cache 
    	    {
                r_dcache_rsp_itlb_miss = rsp_itlb_miss; 
                r_dcache_rsp_itlb_ppn = rsp_itlb_ppn; 
    	    	r_itlb_read_dcache_req = false;
                r_dcache_fsm = DCACHE_IDLE;
                
                // set TLB bit if it's a PTE
                if ( !((rsp_itlb_miss & PTE_T_MASK ) >> PTE_T_SHIFT) )
                {
                    r_dcache.setinbit(r_icache_paddr_save, r_dcache_in_itlb, true);
                }
    	    }
    	    else                    // ins TLB request miss in data cache
    	    {
                r_dcache_itlb_read_req = true;
		r_vci_rsp_data_ok = false;
                r_dcache_fsm = DCACHE_ITLB_READ;
            }
            if ( dreq.valid ) m_cost_data_waste_wait_frz++; 
    	}
    	else if ( r_itlb_acc_dcache_req ) // ins tlb write access bit
    	{
            bool write_hit = r_dcache.write(r_icache_paddr_save, r_icache_pte_update);
            assert(write_hit && "Write on miss ignores data");
            r_dcache_itlb_ll_acc_req = true;
	    r_vci_rsp_data_ok = false; 
	    r_dcache_fsm = DCACHE_ITLB_LL_WAIT;	    		
            if ( dreq.valid ) m_cost_data_waste_wait_frz++; 
    	}
        else if (dreq.valid) 
        {
            pte_info_t  dcache_pte_info;
            int         xtn_opcod        = (int)dreq.addr/4;
            paddr_t     tlb_dpaddr       = 0;        // physical address obtained from TLB
            paddr_t     spc_dpaddr       = 0;        // physical adress obtained from PPN_save (speculative)
            bool        dcache_hit_t     = false;    // hit on 4Kilo TLB
            bool        dcache_hit_x     = false;    // VPN unmodified (can use spc_dpaddr)
            bool        dcache_hit_p     = false;    // PTP unmodified (can skip first level page table walk)
            bool        dcache_hit_c     = false;    // Cache hit
            bool        dcache_cached    = false;    // cacheable access (read or write)
            size_t      dcache_tlb_way   = 0;        // selected way (in case of cache hit)
            size_t      dcache_tlb_set   = 0;        // selected set (Y field in address)
            data_t      dcache_rdata     = 0;        // read data
            paddr_t     dcache_tlb_nline = 0;       // TLB NLINE 

            m_cpt_dcache_data_read += m_dcache_ways;
            m_cpt_dcache_dir_read += m_dcache_ways;

            // Decoding READ XTN requests from processor
            // They are executed in this DCACHE_IDLE state

            if (dreq.type == iss_t::XTN_READ) 
            {
                switch(xtn_opcod) {
                case iss_t::XTN_INS_ERROR_TYPE:
                    drsp.rdata = (uint32_t)r_icache_error_type;
                    r_icache_error_type = MMU_NONE;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_DATA_ERROR_TYPE:
                    drsp.rdata = (uint32_t)r_dcache_error_type;
                    r_dcache_error_type = MMU_NONE;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_INS_BAD_VADDR:
                    drsp.rdata = (uint32_t)r_icache_bad_vaddr;       
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_DATA_BAD_VADDR:
                    drsp.rdata = (uint32_t)r_dcache_bad_vaddr;        
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_PTPR:
                    drsp.rdata = (uint32_t)r_mmu_ptpr;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_TLB_MODE:
                    drsp.rdata = (uint32_t)r_mmu_mode;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_MMU_PARAMS:
                    drsp.rdata = (uint32_t)r_mmu_params;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_MMU_RELEASE:
                    drsp.rdata = (uint32_t)r_mmu_release;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_MMU_WORD_LO:
                    drsp.rdata = (uint32_t)r_mmu_word_lo;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                case iss_t::XTN_MMU_WORD_HI:
                    drsp.rdata = (uint32_t)r_mmu_word_hi;
                    drsp.valid = true;
                    drsp.error = false;
                    break;
                default:
                    r_dcache_error_type = MMU_READ_UNDEFINED_XTN; 
                    r_dcache_bad_vaddr  = dreq.addr;
                    drsp.valid = true;
                    drsp.error = true;
                    break;
                }
                r_dcache_fsm = DCACHE_IDLE;
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
                    if ((dreq.mode == iss_t::MODE_HYPER) || (dreq.mode == iss_t::MODE_KERNEL)) 
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
                        r_dcache_error_type = MMU_WRITE_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                	r_dcache_fsm = DCACHE_IDLE;
                    }
                    break;

                case iss_t::XTN_TLB_MODE:     // modifying TLBs mode : checking the kernel mode
                    if ((dreq.mode == iss_t::MODE_HYPER) || (dreq.mode == iss_t::MODE_KERNEL)) 
                    {
                        r_mmu_mode = (int)dreq.wdata;
                        drsp.valid = true;
                    } 
                    else 
                    {
                        r_dcache_error_type = MMU_WRITE_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                    }
                    r_dcache_fsm = DCACHE_IDLE;
                    break;

                case iss_t::XTN_DTLB_INVAL:     //  checking the kernel mode
                    if ((dreq.mode == iss_t::MODE_HYPER) || (dreq.mode == iss_t::MODE_KERNEL)) 
                    {
                        r_dcache_fsm = DCACHE_DTLB_INVAL;  
                    } 
                    else 
                    {
                        r_dcache_error_type = MMU_WRITE_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                	r_dcache_fsm = DCACHE_IDLE;
                    }
                    break;

                case iss_t::XTN_ITLB_INVAL:     //  checking the kernel mode
                    if ((dreq.mode == iss_t::MODE_HYPER) || (dreq.mode == iss_t::MODE_KERNEL)) 
                    {
                        r_dcache_xtn_req = true;
                        r_dcache_type_save = dreq.addr/4;
                        r_dcache_fsm = DCACHE_ITLB_INVAL;  
                    } 
                    else 
                    {
                        r_dcache_error_type = MMU_WRITE_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                	r_dcache_fsm = DCACHE_IDLE;
                    }
                    break;

                case iss_t::XTN_DCACHE_INVAL:   // cache inval can be executed in user mode.
                    r_dcache_fsm = DCACHE_DCACHE_INVAL;
                    break;

                case iss_t::XTN_MMU_DCACHE_PA_INV:   // cache inval can be executed in user mode.
                    r_dcache_fsm = DCACHE_DCACHE_INVAL_PA;
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

                case iss_t::XTN_MMU_ICACHE_PA_INV:   // cache inval can be executed in user mode.
                    r_dcache_type_save = dreq.addr/4; 
                    r_dcache_xtn_req = true;
                    r_dcache_fsm = DCACHE_ICACHE_INVAL_PA; 
                    break;

                case iss_t::XTN_ICACHE_FLUSH:   // cache flush can be executed in user mode.
                    r_dcache_type_save = dreq.addr/4; 
                    r_dcache_xtn_req = true; 
                    r_dcache_fsm = DCACHE_ICACHE_FLUSH;
                    break;

                case iss_t::XTN_SYNC:           // cache synchronization can be executed in user mode.
                    if (r_wbuf.rok())
                    {
                        r_dcache_fsm = DCACHE_DCACHE_SYNC; 
                    }
                    else
                    {
                        drsp.valid = true;
                        r_dcache_fsm = DCACHE_IDLE;
                    }
		    break;

                case iss_t::XTN_MMU_WORD_LO: // modifying MMU misc registers
                    if ((dreq.mode == iss_t::MODE_HYPER) || (dreq.mode == iss_t::MODE_KERNEL)) 
                    {
                        r_mmu_word_lo = (int)dreq.wdata;
                        drsp.valid = true;
                    } 
                    else 
                    {
                        r_dcache_error_type = MMU_WRITE_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                    }
                    r_dcache_fsm = DCACHE_IDLE;
                    break;

                case iss_t::XTN_MMU_WORD_HI: // modifying MMU misc registers
                    if ((dreq.mode == iss_t::MODE_HYPER) || (dreq.mode == iss_t::MODE_KERNEL)) 
                    {
                        r_mmu_word_hi = (int)dreq.wdata;
                        drsp.valid = true;
                    } 
                    else 
                    {
                        r_dcache_error_type = MMU_WRITE_PRIVILEGE_VIOLATION; 
                        r_dcache_bad_vaddr  = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                    }
                    r_dcache_fsm = DCACHE_IDLE;
                    break;

		  case iss_t::XTN_ICACHE_PREFETCH:
		  case iss_t::XTN_DCACHE_PREFETCH:
		    drsp.valid = true;
                    drsp.error = false;
		    break;
	
                default:
                    r_dcache_error_type = MMU_WRITE_UNDEFINED_XTN; 
                    r_dcache_bad_vaddr  = dreq.addr;
                    drsp.valid = true;
                    drsp.error = true;
                    r_dcache_fsm = DCACHE_IDLE;
                    break;
                } // end switch xtn_opcod

                break;
            } // end if XTN_WRITE

            // Evaluating dcache_hit_t, dcache_hit_x, dcache_hit_p, dcache_hit_c,
            // dcache_pte_info, dcache_tlb_way, dcache_tlb_set & dpaddr & cacheability 
            // - If MMU activated : cacheability is defined by the cachable bit in the TLB
            // - If MMU not activated : cacheability is defined by the segment table.

            if ( !(r_mmu_mode.read() & DATA_TLB_MASK) ) // MMU not activated
            {
                dcache_hit_t  = true;       
                dcache_hit_x  = true;   
                dcache_hit_p  = true;  
                tlb_dpaddr    = dreq.addr; 
                spc_dpaddr    = dreq.addr;    
                dcache_cached = m_cacheability_table[dreq.addr] && 
                                ((dreq.type != iss_t::DATA_LL)  && (dreq.type != iss_t::DATA_SC) &&
                                 (dreq.type != iss_t::XTN_READ) && (dreq.type != iss_t::XTN_WRITE));     
            } 
            else                                                            // MMU activated
            {
                m_cpt_data_tlb_read++;
                dcache_hit_t  = dcache_tlb.cctranslate(dreq.addr, &tlb_dpaddr, &dcache_pte_info, 
                                                       &dcache_tlb_nline, &dcache_tlb_way, &dcache_tlb_set);                  
                dcache_hit_x  = (((vaddr_t)r_dcache_vpn_save << PAGE_K_NBITS) == (dreq.addr & ~PAGE_K_MASK)) && r_dtlb_translation_valid; 
                dcache_hit_p  = (((dreq.addr >> PAGE_M_NBITS) == r_dcache_id1_save) && r_dcache_ptba_ok );
                spc_dpaddr    = ((paddr_t)r_dcache_ppn_save << PAGE_K_NBITS) | (paddr_t)((dreq.addr & PAGE_K_MASK));
                dcache_cached = dcache_pte_info.c && 
                                ((dreq.type != iss_t::DATA_LL)  && (dreq.type != iss_t::DATA_SC) &&
                                 (dreq.type != iss_t::XTN_READ) && (dreq.type != iss_t::XTN_WRITE));    
            }

            if ( !(r_mmu_mode.read() & DATA_CACHE_MASK) )   // cache not actived
            {
                dcache_cached = false;
            }

            // dcache_hit_c & dcache_rdata
            if ( dcache_cached )    // using speculative physical address for cached access
            {
                dcache_hit_c = r_dcache.read(spc_dpaddr, &dcache_rdata);
            } 
            else                    // using actual physical address for uncached access
            {
                dcache_hit_c = ((tlb_dpaddr == (paddr_t)r_dcache_paddr_save) && r_vci_rsp_data_ok ); 
                dcache_rdata = r_dcache_miss_buf[0];
            }

            if ( r_mmu_mode.read() & DATA_TLB_MASK ) 
            {
                // Checking access rights
                if ( dcache_hit_t ) 
                {
                    if (!dcache_pte_info.u && (dreq.mode == iss_t::MODE_USER)) 
                    {
                        if ((dreq.type == iss_t::DATA_READ)||(dreq.type == iss_t::DATA_LL))
                        {
                            r_dcache_error_type = MMU_READ_PRIVILEGE_VIOLATION;
                        }
                        else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                        {
                            r_dcache_error_type = MMU_WRITE_PRIVILEGE_VIOLATION;
                        }  
                        r_dcache_bad_vaddr = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;
                        r_dcache_fsm = DCACHE_IDLE;
                        break;
                    }
                    if (!dcache_pte_info.w && ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))) 
                    {
                        r_dcache_error_type = MMU_WRITE_ACCES_VIOLATION;  
                        r_dcache_bad_vaddr = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;
                        r_dcache_fsm = DCACHE_IDLE;
                        break;
                    }
                }

                // update LRU, save ppn, vpn and page type
                if ( dcache_hit_t ) {
                    dcache_tlb.setlru(dcache_tlb_way,dcache_tlb_set); 
                    r_dcache_ppn_save = tlb_dpaddr >> PAGE_K_NBITS;
                    r_dcache_vpn_save = dreq.addr >> PAGE_K_NBITS;
                    r_dcache_tlb_nline = dcache_tlb_nline;
                    r_dtlb_translation_valid = true;
                }
                else
                {
                    r_dtlb_translation_valid = false;
                }

            } // end if MMU activated

            // compute next state 
            if ( !dcache_hit_p && !dcache_hit_t )  // TLB miss
            {
                r_dcache_tlb_paddr = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                r_dcache_fsm = DCACHE_DTLB1_READ_CACHE;
                m_cpt_data_tlb_miss++;
                m_cost_data_tlb_miss_frz++;
            }
            else if ( dcache_hit_p && !dcache_hit_t )  // TLB Miss with possibility of bypass first level page
            {
                // walk page table level 2
                r_dcache_tlb_paddr = (paddr_t)r_dcache_ptba_save | 
                                     (paddr_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 3); 
                r_dcache_fsm = DCACHE_DTLB2_READ_CACHE;
                m_cpt_data_tlb_miss++;
                m_cost_data_tlb_miss_frz++;
            }
            else if ( dcache_hit_t && !dcache_hit_x && dcache_cached )// cached access with an ucorrect speculative physical address
            {
                r_dcache_hit_p_save = dcache_hit_p;
                r_dcache_fsm = DCACHE_BIS;
                m_cost_data_miss_frz++;
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
                            r_vci_rsp_data_ok = false;
                            r_dcache_fsm = DCACHE_IDLE;
                            drsp.valid = true;
                            drsp.rdata = dcache_rdata;
                        } 
                        else 
                        {
                            if ( dcache_cached ) 
                            {
                                r_dcache_miss_req = true;
				r_vci_rsp_data_ok = false;
                                r_dcache_fsm = DCACHE_MISS_WAIT;
                                m_cpt_data_miss++;
                                m_cost_data_miss_frz++;
                            } 
                            else 
                            {
                                r_dcache_unc_req = true;
				r_vci_rsp_data_ok = false;
                                r_dcache_fsm = DCACHE_UNC_WAIT;
                                m_cpt_unc_read++;
                                m_cost_unc_read_frz++;
                            }
                        }
                        break;
/*
                    case iss_t::DATA_READ:
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
                    case iss_t::DATA_LL:
			if (r_dcache_llsc_reserved && (r_dcache_llsc_addr_save == tlb_dpaddr) && r_dcache_buf_unc_valid)
			{
                            r_dcache_buf_unc_valid = false;
                            r_dcache_fsm = DCACHE_IDLE;
                            drsp.valid = true;
                            drsp.rdata = dcache_rdata;
			}
			else
			{
			    r_dcache_llsc_reserved = true;
			    r_dcache_llsc_addr_save = tlb_dpaddr;
                            r_dcache_unc_req = true;
                            r_dcache_fsm = DCACHE_UNC_WAIT;
			}
			break;
                    case iss_t::DATA_SC:
			if (r_dcache_llsc_reserved && (r_dcache_llsc_addr_save == tlb_dpaddr))
			{
			    r_dcache_llsc_reserved = false;
                            r_dcache_unc_req = true;
                            r_dcache_fsm = DCACHE_UNC_WAIT;
			}
			else
			{   
			    if ( r_dcache_buf_unc_valid )
			    {                         
			        r_dcache_llsc_reserved = false;
			        r_dcache_buf_unc_valid = false;
                                drsp.valid = true;
                                drsp.rdata = dcache_rdata;
			    }
                            r_dcache_fsm = DCACHE_IDLE;
			}			
			break;
*/
                    case iss_t::DATA_WRITE:
                        m_cpt_write++;
                        if ( dcache_cached ) m_cpt_write_cached++;

                        if ( dcache_hit_c && dcache_cached )    // cache update required
                        {
                            r_dcache_fsm = DCACHE_WRITE_UPDT;
                        } 
                        else if ( !dcache_pte_info.d && (r_mmu_mode.read() & DATA_TLB_MASK) )   // dirty bit update required
                        {
                            if ( dcache_tlb.getpagesize(dcache_tlb_way, dcache_tlb_set) )	// 2M page size, one level page table 
                            {
                                r_dcache_pte_update = dcache_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                r_dcache_tlb_paddr = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                                r_dcache_tlb_ll_dirty_req = true;
				r_vci_rsp_data_ok = false;
                                r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
                                m_cpt_data_tlb_write_dirty++;
                            }
                            else	// 4k page size, two levels page table 
                            {   
                                if (dcache_hit_p) 
                                {
                                    r_dcache_pte_update = dcache_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                    r_dcache_tlb_paddr = (paddr_t)r_dcache_ptba_save | (paddr_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 3);
                                    r_dcache_tlb_ll_dirty_req = true;
				    r_vci_rsp_data_ok = false;
                                    r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
                                    m_cpt_data_tlb_write_dirty++;
                                }
                                else    // get PTBA to calculate the physical address of PTE
                                {
                                    r_dcache_pte_update = dcache_tlb.getpte(dcache_tlb_way, dcache_tlb_set) | PTE_D_MASK;
                                    r_dcache_tlb_paddr = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                                    r_dcache_tlb_ptba_read = true;
                                    r_dcache_fsm = DCACHE_DTLB1_READ_CACHE;
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
            m_cost_data_waste_wait_frz++;
            break;
        }

        // Using tlb entry is invalidated 
        if ( r_dcache_inval_tlb_rsp )
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            m_cost_data_tlb_miss_frz++;
            break;
        }

        data_t  dcache_rdata = 0;
        bool    dcache_hit_c = false;
        bool    dcache_hit_t = false;
        paddr_t tlb_dpaddr   = 0;

	dcache_hit_t = dcache_tlb.translate(dreq.addr, &tlb_dpaddr);

	if ( (tlb_dpaddr == r_dcache_paddr_save.read()) && dreq.valid && dcache_hit_t ) 
	{
	    // acces always cached in this state
	    dcache_hit_c = r_dcache.read(r_dcache_paddr_save, &dcache_rdata);
	    
	    if ( dreq.type == iss_t::DATA_READ )  // cached read
	    {
	        m_cpt_read++;
	        if ( !dcache_hit_c ) 
	        {
	            r_dcache_miss_req = true;
		    r_vci_rsp_data_ok = false;
	            r_dcache_fsm = DCACHE_MISS_WAIT;
	            m_cpt_data_miss++;
	            m_cost_data_miss_frz++;
	        }
	        else
	        {
	            r_dcache_fsm = DCACHE_IDLE;
	        }
	        drsp.valid = dcache_hit_c;
	        drsp.error = false;
	        drsp.rdata = dcache_rdata;
	    }
	    else    // cached write
	    {
	        m_cpt_write++;
	        m_cpt_write_cached++;
	        if ( dcache_hit_c )    // cache update required
	        {
	            r_dcache_rdata_save = dcache_rdata;
	            r_dcache_fsm = DCACHE_WRITE_UPDT;
	        } 
	        else if (!r_dcache_dirty_save && (r_mmu_mode.read() & DATA_TLB_MASK))   // dirty bit update required
	        {
	            if (dcache_tlb.getpagesize(r_dcache_tlb_way_save, r_dcache_tlb_set_save)) 
	            {
	                r_dcache_pte_update = dcache_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
	                r_dcache_tlb_paddr = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((dreq.addr>>PAGE_M_NBITS)<<2);
	                r_dcache_tlb_ll_dirty_req = true;
			r_vci_rsp_data_ok = false;
	                r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
	                m_cpt_data_tlb_write_dirty++;
	            }
	            else
	            {   
	                if (r_dcache_hit_p_save) 
	                {
	                    r_dcache_pte_update = dcache_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
	                    r_dcache_tlb_paddr = (paddr_t)r_dcache_ptba_save|(paddr_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 3);
	                    r_dcache_tlb_ll_dirty_req = true;
			    r_vci_rsp_data_ok = false;
	                    r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
	                    m_cpt_data_tlb_write_dirty++;
	                }
	                else
	                {
	                    r_dcache_pte_update = dcache_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
	                    r_dcache_tlb_paddr = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((dreq.addr>>PAGE_M_NBITS)<<2);
	                    r_dcache_tlb_ptba_read = true;
	                    r_dcache_fsm = DCACHE_DTLB1_READ_CACHE;
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
	    }
	}
	else
	{
            drsp.valid = false;
            drsp.error = false;
            drsp.rdata = 0;
            r_dcache_fsm = DCACHE_IDLE;
	}
        break;
    }
    //////////////////////////
    case DCACHE_LL_DIRTY_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
		if (dcache_tlb.getpagesize(r_dcache_tlb_way_save, r_dcache_tlb_set_save)) 
		{
                    r_dcache_error_type = MMU_WRITE_PT1_ILLEGAL_ACCESS;     
		}
		else
		{
                    r_dcache_error_type = MMU_WRITE_PT2_ILLEGAL_ACCESS;     
		}
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;
            }
	    else
	    {
		if ( !(r_dcache_miss_buf[0] >> PTE_V_SHIFT) )	// unmapped
	        {
		    if (dcache_tlb.getpagesize(r_dcache_tlb_way_save, r_dcache_tlb_set_save))
		    { 
			r_dcache_error_type = MMU_WRITE_PT1_UNMAPPED;       
		    }
		    else
		    {
			r_dcache_error_type = MMU_WRITE_PT2_UNMAPPED;       
	  	    }
                    r_dcache_bad_vaddr = dreq.addr;
                    r_dcache_fsm = DCACHE_ERROR;

		    if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		    if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;
	        }
        	else if ( r_dcache_inval_tlb_rsp )
        	{
        	    r_dcache_inval_tlb_rsp = false;
        	    r_dcache_fsm = DCACHE_IDLE;
        	    m_cost_data_tlb_miss_frz++;
        	}
		else if ( r_dcache_inval_rsp )
		{
        	    r_dcache_inval_rsp = false;
		    r_dcache_fsm = DCACHE_IDLE;
		}
		else
		{
		    r_dcache_tlb_sc_dirty_req = true;
		    r_vci_rsp_data_ok = false;
		    r_dcache_pte_update = r_dcache_miss_buf[0] | r_dcache_pte_update.read();
                    r_dcache_fsm = DCACHE_SC_DIRTY_WAIT; 
		}
	    }
	}
	break;
    }
    //////////////////////////
    case DCACHE_SC_DIRTY_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
	    if ( r_vci_rsp_data_error ) // VCI response ko
	    {
	        if (dcache_tlb.getpagesize(r_dcache_tlb_way_save, r_dcache_tlb_set_save))
	        {
	            r_dcache_error_type = MMU_WRITE_PT1_ILLEGAL_ACCESS;    
	        }
	        else
	        {
	            r_dcache_error_type = MMU_WRITE_PT2_ILLEGAL_ACCESS;    
	        }
	        r_dcache_bad_vaddr = dreq.addr;
	        r_dcache_fsm = DCACHE_ERROR;

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;		 	    
	    }
	    else 
	    {
                // Using tlb entry is invalidated 
                if ( r_dcache_inval_tlb_rsp )
                {
                    r_dcache_inval_tlb_rsp = false;
                    r_dcache_fsm = DCACHE_IDLE;
                    m_cost_data_tlb_miss_frz++;
                }
		else if ( r_dcache_inval_rsp )
		{
        	    r_dcache_inval_rsp = false;
		    r_dcache_fsm = DCACHE_IDLE;
		}
		else if ( r_dcache_tlb_ll_dirty_req )
	        {
		    r_vci_rsp_data_ok = false;
	            r_dcache_fsm = DCACHE_LL_DIRTY_WAIT; 
	        }
		else
		{
		    r_vci_rsp_data_ok = false;	
	            r_dcache_fsm = DCACHE_WRITE_DIRTY; 
		}
	    }
	}
	break;
    }
    ////////////////////////////
    case DCACHE_DTLB1_READ_CACHE:
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

        // Using tlb entry is invalidated 
        if ( r_dcache_inval_tlb_rsp )
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            m_cost_data_tlb_miss_frz++;
            break;
        }

        data_t tlb_data = 0;
	bool write_hit = false;
        bool tlb_hit_cache = r_dcache.read(r_dcache_tlb_paddr, &tlb_data);

        // DTLB request hit in cache
        if ( tlb_hit_cache )
        {
	    if ( !(tlb_data >> PTE_V_SHIFT) )	// unmapped
	    {
                r_dcache_ptba_ok    = false;
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT1_UNMAPPED;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT1_UNMAPPED;
                }  
                r_dcache_bad_vaddr  = dreq.addr;
                r_dcache_fsm        = DCACHE_ERROR;
	    }
	    else if ( (tlb_data & PTE_T_MASK) >> PTE_T_SHIFT )	// PTD
	    {
                r_dcache_ptba_ok   = true;
                r_dcache_ptba_save = (paddr_t)(tlb_data & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS;  
                r_dcache_id1_save  = dreq.addr >> PAGE_M_NBITS;
                r_dcache_tlb_paddr = (paddr_t)(tlb_data & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS | 
                                     (paddr_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 3);
                if ( r_dcache_tlb_ptba_read )
                {
                    r_dcache_tlb_ptba_read = false;
                    write_hit = r_dcache.write(((paddr_t)(tlb_data & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS | 
                                                (paddr_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 3)), r_dcache_pte_update);
                    //assert(write_hit && "Write on miss ignores data");
                    r_dcache_tlb_ll_dirty_req = true;
		    r_vci_rsp_data_ok = false;
                    r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
                    m_cpt_data_tlb_write_dirty++;
                }
                else
                {
                    r_dcache_fsm = DCACHE_DTLB2_READ_CACHE;
                }
	    }
	    else	// PTE
	    {
                r_dcache_ptba_ok = false;
	        if ( (m_srcid_rw >> 4) == ((r_dcache_tlb_paddr.read() & ((1<<(m_paddr_nbits - PAGE_M_NBITS))-1)) >> (m_paddr_nbits - PAGE_M_NBITS -10)) ) // local
		{
		    if ( (tlb_data & PTE_L_MASK ) >> PTE_L_SHIFT ) // L bit is set
		    {
                        r_dcache_pte_update = tlb_data;
                        r_dcache_fsm = DCACHE_TLB1_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = tlb_data | PTE_L_MASK;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(tlb_data | PTE_L_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm = DCACHE_TLB1_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
    	        }
		else // remotely
		{
		    if ( (tlb_data & PTE_R_MASK ) >> PTE_R_SHIFT ) // R bit is set
		    {
                        r_dcache_pte_update = tlb_data;
                        r_dcache_fsm = DCACHE_TLB1_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = tlb_data | PTE_R_MASK;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(tlb_data | PTE_R_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm = DCACHE_TLB1_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
		}
	    }
        }
        else
        {
            // DTLB request miss in cache and walk page table level 1
            r_dcache_tlb_read_req = true;
	    r_vci_rsp_data_ok = false;
            r_dcache_fsm = DCACHE_TLB1_READ;
        }
        break;
    }
    ///////////////////////
    case DCACHE_TLB1_LL_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT1_ILLEGAL_ACCESS;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT1_ILLEGAL_ACCESS;
                } 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;		 
            }
	    else
	    {
		if ( !(r_dcache_miss_buf[0] >> PTE_V_SHIFT) )	// unmapped
	        {
               	    if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
               	    {
               	        r_dcache_error_type = MMU_READ_PT1_UNMAPPED;
               	    }
               	    else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
               	    {
               	        r_dcache_error_type = MMU_WRITE_PT1_UNMAPPED;
               	    }  
                    r_dcache_bad_vaddr  = dreq.addr;
                    r_dcache_fsm        = DCACHE_ERROR;

		    if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		    if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;	
	        }
       		else if ( r_dcache_inval_tlb_rsp )
       		{
       		    r_dcache_inval_tlb_rsp = false;
       		    r_dcache_fsm = DCACHE_IDLE;
       		    m_cost_data_tlb_miss_frz++;
       		}
		else if ( r_dcache_inval_rsp )
       		{
       		    r_dcache_inval_rsp = false;
       		    r_dcache_fsm = DCACHE_IDLE;
       		}
		else
		{
		    r_dcache_tlb_sc_acc_req = true;
		    r_vci_rsp_data_ok = false;
		    r_dcache_pte_update = r_dcache_miss_buf[0] | r_dcache_pte_update.read();
                    r_dcache_fsm = DCACHE_TLB1_SC_WAIT; 
		}
	    }
	}
	break;
    }
    ///////////////////////
    case DCACHE_TLB1_SC_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
	    if ( r_vci_rsp_data_error ) // VCI response ko
	    {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT1_ILLEGAL_ACCESS;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT1_ILLEGAL_ACCESS;
                } 
	        r_dcache_bad_vaddr = dreq.addr;
	        r_dcache_fsm = DCACHE_ERROR; 

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;	
	    }
	    else
	    {
        	// Using tlb entry is invalidated 
        	if ( r_dcache_inval_tlb_rsp )
        	{
        	    r_dcache_inval_tlb_rsp = false;
        	    r_dcache_fsm = DCACHE_IDLE;
        	    m_cost_data_tlb_miss_frz++;
        	}
		else if ( r_dcache_inval_rsp )
        	{
        	    r_dcache_inval_rsp = false;
        	    r_dcache_fsm = DCACHE_IDLE;
        	}
	    	else if ( r_dcache_tlb_ll_acc_req )
	    	{
		    r_vci_rsp_data_ok = false;
	    	    r_dcache_fsm = DCACHE_TLB1_LL_WAIT; 
	    	}
	    	else 
	    	{
		    r_vci_rsp_data_ok = false;	
	    	    r_dcache_fsm = DCACHE_TLB1_UPDT; 
	    	}
	    }
	}
	break;
    }
    //////////////////////
    case DCACHE_TLB1_READ:
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

	if ( r_vci_rsp_data_ok )
	{	
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT1_ILLEGAL_ACCESS;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT1_ILLEGAL_ACCESS;
                } 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;	
		break;
            }

            if ( r_dcache_inval_tlb_rsp )  // TLB miss response and invalidation
            {
                r_dcache_fsm = DCACHE_IDLE; 
            	r_dcache_inval_tlb_rsp = false;
		break;
            }

	    if ( r_dcache_inval_rsp ) // TLB miss response and cache invalidation
	    {
	        if ( r_dcache_cleanup_req ) break;
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
	    }

	    // TLB miss response and no invalidation
	    r_vci_rsp_data_ok = false;
	    r_dcache_fsm = DCACHE_TLB1_READ_UPDT;
	}
        break;
    }
    //////////////////////////
    case DCACHE_TLB1_READ_UPDT:
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

        // Using tlb entry is invalidated 
        if ( r_dcache_inval_tlb_rsp )
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            m_cost_data_tlb_miss_frz++;
            break;
        }

        if ( !r_dcache_cleanup_req ) // Miss update and no invalidation
        {
            // update dcache
            data_t   rsp_dtlb_miss = 0;
            paddr_t  victim_index = 0;
	    bool write_hit = false;
            size_t way = 0;
            size_t set = 0;

            // Using tlb entry is in the invalidated cache line  
            if ( r_dcache_inval_rsp )
            {
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
            }

            bool cleanup_req = r_dcache.find(r_dcache_tlb_paddr, r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, &victim_index);
            
	    if ( cleanup_req )
	    {	    
		// ins tlb invalidate verification    
                r_dcache_itlb_inval_req = r_dcache_in_itlb[m_dcache_sets*way+set];
                r_dcache_itlb_inval_line = victim_index;
                r_dcache_in_itlb[way*m_dcache_sets+set] = false;

                // data tlb invalidate verification
                r_dcache_dtlb_inval_req = r_dcache_in_dtlb[m_dcache_sets*way+set]; 
                r_dcache_dtlb_inval_line = victim_index;
                r_dcache_in_dtlb[way*m_dcache_sets+set] = false;

                r_dcache_cleanup_req = true; 
                r_dcache_cleanup_line = victim_index;
		r_dcache_fsm = DCACHE_TLB_CC_INVAL;
                r_dcache_fsm_save = r_dcache_fsm;
		break;
	    }

	    r_dcache.update(r_dcache_tlb_paddr, way, set, r_dcache_miss_buf);
            r_dcache.read(r_dcache_tlb_paddr, &rsp_dtlb_miss);	

	    if ( !(rsp_dtlb_miss >> PTE_V_SHIFT) )	// unmapped
	    {
                r_dcache_ptba_ok    = false;
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT1_UNMAPPED;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT1_UNMAPPED;
                } 
                r_dcache_bad_vaddr  = dreq.addr;
                r_dcache_fsm        = DCACHE_ERROR;
	    }
	    else if ( (rsp_dtlb_miss & PTE_T_MASK) >> PTE_T_SHIFT ) // PTD
	    {
                r_dcache_ptba_ok   = true;
                r_dcache_ptba_save = (paddr_t)(rsp_dtlb_miss & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS;  
                r_dcache_id1_save  = dreq.addr >> PAGE_M_NBITS;
                r_dcache_tlb_paddr = (paddr_t)(rsp_dtlb_miss & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS | 
                                     (paddr_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 3);
                if ( r_dcache_tlb_ptba_read )
                {
                    r_dcache_tlb_ptba_read = false;
                    write_hit = r_dcache.write(((paddr_t)(rsp_dtlb_miss & ((1<<(m_paddr_nbits - PAGE_K_NBITS))-1)) << PAGE_K_NBITS | 
                                                (paddr_t)(((dreq.addr & PTD_ID2_MASK) >> PAGE_K_NBITS) << 3)),r_dcache_pte_update);
                    //assert(write_hit && "Write on miss ignores data");
                    r_dcache_tlb_ll_dirty_req = true;
		    r_vci_rsp_data_ok = false;
                    r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
                    m_cpt_data_tlb_write_dirty++;
                }
                else
                {
                    r_dcache_fsm = DCACHE_DTLB2_READ_CACHE;
                }
	    }
	    else	// PTE
	    {
                r_dcache_ptba_ok = false;
	        if ( (m_srcid_rw >> 4) == ((r_dcache_tlb_paddr.read() & ((1<<(m_paddr_nbits - PAGE_M_NBITS))-1)) >> (m_paddr_nbits - PAGE_M_NBITS -10)) ) // local
		{
		    if ( (rsp_dtlb_miss & PTE_L_MASK ) >> PTE_L_SHIFT ) // L bit is set
		    {
                        r_dcache_pte_update = rsp_dtlb_miss;
                        r_dcache_fsm        = DCACHE_TLB1_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = rsp_dtlb_miss | PTE_L_MASK;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(rsp_dtlb_miss | PTE_L_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm        = DCACHE_TLB1_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
    	        }
		else // remotely
		{
		    if ( (rsp_dtlb_miss & PTE_R_MASK ) >> PTE_R_SHIFT ) // R bit is set
		    {
                        r_dcache_pte_update = rsp_dtlb_miss;
                        r_dcache_fsm        = DCACHE_TLB1_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = rsp_dtlb_miss | PTE_R_MASK;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(rsp_dtlb_miss | PTE_R_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm        = DCACHE_TLB1_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
		}
	    }
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB1_UPDT: 
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

        if ( !r_dcache_inval_tlb_rsp && !r_dcache_inval_rsp )
        {
            paddr_t victim_index = 0;
            if (dcache_tlb.update(r_dcache_pte_update,dreq.addr,(r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index))
            {
                r_dcache.setinbit((paddr_t)victim_index*m_dcache_words*2, r_dcache_in_dtlb, false);
            }
            r_dcache.setinbit(r_dcache_tlb_paddr, r_dcache_in_dtlb, true);
            r_dcache_fsm = DCACHE_IDLE;
        }
        else  
        {
            if ( r_dcache_inval_tlb_rsp ) r_dcache_inval_tlb_rsp = false;
            if ( r_dcache_inval_rsp ) r_dcache_inval_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    /////////////////////////////
    case DCACHE_DTLB2_READ_CACHE:
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

        // Using tlb entry is invalidated 
        if ( r_dcache_inval_tlb_rsp )
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            m_cost_data_tlb_miss_frz++;
            break;
        }

        data_t tlb_data = 0;
        data_t tlb_data_ppn = 0;
	bool write_hit = false;
        bool tlb_hit_cache = r_dcache.read(r_dcache_tlb_paddr, &tlb_data);

	if ( tlb_hit_cache )
	{
            bool tlb_hit_ppn = r_dcache.read(r_dcache_tlb_paddr.read()+4, &tlb_data_ppn);
	    assert(tlb_hit_ppn && "Address of pte[64-32] and pte[31-0] should be successive");
	}

        // DTLB request hit in cache
        if ( tlb_hit_cache )
        {
	    if ( !(tlb_data >> PTE_V_SHIFT) )	// unmapped
	    {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT2_UNMAPPED;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT2_UNMAPPED;
                } 
                r_dcache_bad_vaddr  = dreq.addr;
                r_dcache_fsm        = DCACHE_ERROR;
	    }
	    else if ( (tlb_data & PTE_T_MASK) >> PTE_T_SHIFT ) //PTD
	    {
                r_dcache_pte_update = tlb_data;
	        r_dcache_ppn_update = tlb_data_ppn;
		r_dcache_fsm = DCACHE_TLB2_UPDT;
	    }
	    else 
	    {
	        if ( (m_srcid_rw >> 4) == ((r_dcache_tlb_paddr.read() & ((1<<(m_paddr_nbits - PAGE_M_NBITS))-1)) >> (m_paddr_nbits - PAGE_M_NBITS -10)) ) // local
		{
		    if ( (tlb_data & PTE_L_MASK ) >> PTE_L_SHIFT ) // L bit is set
		    {
                        r_dcache_pte_update = tlb_data;
			r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_fsm        = DCACHE_TLB2_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = tlb_data | PTE_L_MASK;
	        	r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(tlb_data | PTE_L_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm = DCACHE_TLB2_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
    	        }
		else // remotely
		{
		    if ( (tlb_data & PTE_R_MASK ) >> PTE_R_SHIFT ) // R bit is set
		    {
                        r_dcache_pte_update = tlb_data;
			r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_fsm        = DCACHE_TLB2_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = tlb_data | PTE_R_MASK;
	        	r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(tlb_data | PTE_R_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm = DCACHE_TLB2_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
		}
	    }
        }
        else
        {
            // DTLB request miss in cache and walk page table level 2
            r_dcache_tlb_read_req = true;
	    r_vci_rsp_data_ok = false;
            r_dcache_fsm = DCACHE_TLB2_READ;
        }
        break;
    }
    ///////////////////////
    case DCACHE_TLB2_LL_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT2_ILLEGAL_ACCESS;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT2_ILLEGAL_ACCESS;
                } 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;
            }
	    else
	    {
	        if ( !(r_dcache_miss_buf[0] >> PTE_V_SHIFT) )	// unmapped
	        {
               	    if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
               	    {
               	        r_dcache_error_type = MMU_READ_PT2_UNMAPPED;
               	    }
               	    else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
               	    {
               	        r_dcache_error_type = MMU_WRITE_PT2_UNMAPPED;
               	    } 
                    r_dcache_bad_vaddr = dreq.addr;
                    r_dcache_fsm = DCACHE_ERROR;

		    if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		    if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;
	        }
        	else if ( r_dcache_inval_tlb_rsp )
        	{
        	    r_dcache_inval_tlb_rsp = false;
        	    r_dcache_fsm = DCACHE_IDLE;
        	    m_cost_data_tlb_miss_frz++;
        	}
		else if ( r_dcache_inval_rsp )
		{
        	    r_dcache_inval_rsp = false;
		    r_dcache_fsm = DCACHE_IDLE;
		}
		else
		{
		    r_dcache_tlb_sc_acc_req = true;
		    r_vci_rsp_data_ok = false;
		    r_dcache_pte_update = r_dcache_miss_buf[0] | r_dcache_pte_update.read();
                    r_dcache_fsm = DCACHE_TLB2_SC_WAIT; 
		}
	    }
	}
	break;
    }
    ///////////////////////
    case DCACHE_TLB2_SC_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
	    if ( r_vci_rsp_data_error ) // VCI response ko
	    {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT2_ILLEGAL_ACCESS;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT2_ILLEGAL_ACCESS;
                } 
	        r_dcache_bad_vaddr = dreq.addr;
	        r_dcache_fsm = DCACHE_ERROR; 

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;	
	    }
	    else
	    { 
                // Using tlb entry is invalidated 
                if ( r_dcache_inval_tlb_rsp )
                {
                    r_dcache_inval_tlb_rsp = false;
                    r_dcache_fsm = DCACHE_IDLE;
                    m_cost_data_tlb_miss_frz++;
                }
		else if ( r_dcache_inval_rsp )
		{
        	    r_dcache_inval_rsp = false;
		    r_dcache_fsm = DCACHE_IDLE;
		}
	        else if ( r_dcache_tlb_ll_acc_req )
	        {
		    r_vci_rsp_data_ok = false;
	            r_dcache_fsm = DCACHE_TLB2_LL_WAIT; 
	        }
	        else 
	        {
		    r_vci_rsp_data_ok = false;	
	            r_dcache_fsm = DCACHE_TLB2_UPDT; 
	        }
	    }
	}
	break;
    }
    /////////////////////
    case DCACHE_TLB2_READ:
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

	if ( r_vci_rsp_data_ok )
	{
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT2_ILLEGAL_ACCESS;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT2_ILLEGAL_ACCESS;
                } 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR; 

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;
		break;
            }	

            if ( r_dcache_inval_tlb_rsp )  // TLB miss response and invalidation
            {
                r_dcache_fsm = DCACHE_IDLE; 
                r_dcache_inval_tlb_rsp = false;
		break;
            }  

	    if ( r_dcache_inval_rsp ) // TLB miss response and cache invalidation
	    {
	        if ( r_dcache_cleanup_req ) break;
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
	    } 

	    // TLB miss response and no invalidation
	    r_vci_rsp_data_ok = false;
	    r_dcache_fsm = DCACHE_TLB2_READ_UPDT;
	}	
        break;
    }
    //////////////////////////
    case DCACHE_TLB2_READ_UPDT:
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

        // Using tlb entry is invalidated 
        if ( r_dcache_inval_tlb_rsp )
        {
            r_dcache_inval_tlb_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            m_cost_data_tlb_miss_frz++;
            break;
        }

        if ( !r_dcache_cleanup_req )
        {
            // update cache
            data_t rsp_dtlb_miss;
            data_t tlb_data_ppn;
	    bool write_hit = false;
            paddr_t  victim_index = 0;
            size_t way = 0;
            size_t set = 0;

            // Using tlb entry is in the invalidated cache line  
            if ( r_dcache_inval_rsp )
            {
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
            }

            bool cleanup_req = r_dcache.find(r_dcache_tlb_paddr, r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, &victim_index);

	    if ( cleanup_req )
	    {	    
		// ins tlb invalidate verification    
                r_dcache_itlb_inval_req = r_dcache_in_itlb[m_dcache_sets*way+set];
                r_dcache_itlb_inval_line = victim_index;
                r_dcache_in_itlb[way*m_dcache_sets+set] = false;

                // data tlb invalidate verification
                r_dcache_dtlb_inval_req = r_dcache_in_dtlb[m_dcache_sets*way+set]; 
                r_dcache_dtlb_inval_line = victim_index;
                r_dcache_in_dtlb[way*m_dcache_sets+set] = false;

                r_dcache_cleanup_req = true; 
                r_dcache_cleanup_line = victim_index;
		r_dcache_fsm = DCACHE_TLB_CC_INVAL;
                r_dcache_fsm_save = r_dcache_fsm;
		break;
	    }

	    r_dcache.update(r_dcache_tlb_paddr, way, set, r_dcache_miss_buf);
	    r_dcache.read(r_dcache_tlb_paddr, &rsp_dtlb_miss);

	    bool tlb_hit_ppn = r_dcache.read(r_dcache_tlb_paddr.read()+4, &tlb_data_ppn);
	    assert(tlb_hit_ppn && "Address of pte[64-32] and pte[31-0] should be successive");

	    if ( !(rsp_dtlb_miss >> PTE_V_SHIFT) )	// unmapped
	    {
                if ((r_dcache_type_save == iss_t::DATA_READ)||(r_dcache_type_save == iss_t::DATA_LL))
                {
                    r_dcache_error_type = MMU_READ_PT2_UNMAPPED;
                }
                else /*if ((dreq.type == iss_t::DATA_WRITE)||(dreq.type == iss_t::DATA_SC))*/
                {
                    r_dcache_error_type = MMU_WRITE_PT2_UNMAPPED;
                }  
                r_dcache_bad_vaddr  = dreq.addr;
                r_dcache_fsm        = DCACHE_ERROR;
	    }
	    else if ( (rsp_dtlb_miss & PTE_T_MASK) >> PTE_T_SHIFT ) // PTD
	    {
                r_dcache_pte_update = rsp_dtlb_miss;
	        r_dcache_ppn_update = tlb_data_ppn;
		r_dcache_fsm = DCACHE_TLB2_UPDT;
	    }
	    else
	    {
	        if ( (m_srcid_rw >> 4) == ((r_dcache_tlb_paddr.read() & ((1<<(m_paddr_nbits - PAGE_M_NBITS))-1)) >> (m_paddr_nbits - PAGE_M_NBITS -10)) ) // local
		{
		    if ( (rsp_dtlb_miss & PTE_L_MASK ) >> PTE_L_SHIFT ) // L bit is set
		    {
                        r_dcache_pte_update = rsp_dtlb_miss;
			r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_fsm        = DCACHE_TLB2_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = rsp_dtlb_miss | PTE_L_MASK;
	        	r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(rsp_dtlb_miss | PTE_L_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm = DCACHE_TLB2_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
    	        }
		else // remotely
		{
		    if ( (rsp_dtlb_miss & PTE_R_MASK ) >> PTE_R_SHIFT ) // R bit is set
		    {
                        r_dcache_pte_update = rsp_dtlb_miss;
			r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_fsm        = DCACHE_TLB2_UPDT;
		    }
		    else
		    {
                        r_dcache_pte_update = rsp_dtlb_miss | PTE_R_MASK;
	        	r_dcache_ppn_update = tlb_data_ppn;
                        r_dcache_tlb_ll_acc_req = true;
			r_vci_rsp_data_ok = false;
                	write_hit = r_dcache.write(r_dcache_tlb_paddr,(rsp_dtlb_miss | PTE_R_MASK));  
                	assert(write_hit && "Write on miss ignores data");  
                        r_dcache_fsm = DCACHE_TLB2_LL_WAIT;
                        m_cpt_ins_tlb_write_et++;
		    }
		}
	    }
        }
        break;
    }
    //////////////////////
    case DCACHE_TLB2_UPDT:  
    {
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }        

        if ( !r_dcache_inval_tlb_rsp && !r_dcache_inval_rsp )
        {
            paddr_t victim_index = 0;
            if (dcache_tlb.update(r_dcache_pte_update,r_dcache_ppn_update,dreq.addr,(r_dcache_tlb_paddr.read() >> (uint32_log2(m_dcache_words)+2)),&victim_index))
            {
                r_dcache.setinbit((paddr_t)victim_index*m_dcache_words*2, r_dcache_in_dtlb, false);
            }
            r_dcache.setinbit(r_dcache_tlb_paddr, r_dcache_in_dtlb, true);
            r_dcache_fsm = DCACHE_IDLE;
        }
        else  
        {
            if ( r_dcache_inval_tlb_rsp ) r_dcache_inval_tlb_rsp = false;
            if ( r_dcache_inval_rsp ) r_dcache_inval_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    ///////////////////////
    case DCACHE_CTXT_SWITCH:
    {
        // TLB flush leads to cleanup corresponding data cache line 
        m_cost_data_tlb_sw_frz++;

        paddr_t victim_index = 0;
        size_t way = 0;
        size_t set = 0;

        if ( r_dcache_itlb_cleanup_req )
        {    
            r_dcache.setinbit(((paddr_t)r_dcache_itlb_cleanup_line.read()*m_dcache_words*2), r_dcache_in_itlb, false);
            r_dcache_itlb_cleanup_req = false;
	}

        for ( way = 0; way < m_dtlb_ways; way++)
        {
            for ( set = 0; set < m_dtlb_sets; set++)
            {
                if(dcache_tlb.checkcleanup(way, set, &victim_index))
                {
                    r_dcache.setinbit((paddr_t)(victim_index << (m_dcache_words+2)), r_dcache_in_dtlb, false); 
                }
            }
        }

        if ( !r_dcache_xtn_req )
        {
            r_dcache_fsm = DCACHE_IDLE;
            r_dtlb_translation_valid = false;
            r_dcache_ptba_ok = false;
            drsp.valid = true;
        }
        break;
    }
    ////////////////////////
    case DCACHE_ICACHE_FLUSH:
    case DCACHE_ICACHE_INVAL:
    case DCACHE_ICACHE_INVAL_PA:
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

        size_t way = r_dcache_way;
        size_t set = r_dcache_set;
        bool clean = false;
        
        m_cost_data_cache_flush_frz++;

        // cache flush and send cleanup to external
        if ( !r_dcache_cleanup_req )
        {
            paddr_t victim_index = 0;
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

            if ((way == m_dcache_ways) && !r_dcache_xtn_req ) 
            {
                // data TLB flush 
                dcache_tlb.flush(true);      // global entries are not invalidated
            	r_dtlb_translation_valid = false;
            	r_dcache_ptba_ok = false;

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
	break;
    }
    //////////////////////
    case DCACHE_DTLB_INVAL: 
    {
        paddr_t victim_index = 0;
        // clean indicate data tlb bit
        if ( dcache_tlb.inval(r_dcache_wdata_save, &victim_index) )
        {  
            r_dcache.setinbit((paddr_t)(victim_index << (m_dcache_words+2)), r_dcache_in_dtlb, false); 
        }
        r_dtlb_translation_valid = false;
        r_dcache_ptba_ok = false;
        r_dcache_fsm = DCACHE_IDLE;
        drsp.valid = true;
        break;
    }
    ////////////////////////
    case DCACHE_DCACHE_INVAL:
    {
        m_cpt_dcache_dir_read += m_dcache_ways;
        vaddr_t invadr = dreq.wdata;
        paddr_t dpaddr = 0;
        bool dcache_hit_t = false; 
        size_t way = 0;
        size_t set = 0;

	if ( !r_dcache_cleanup_req )
	{
            if ( r_mmu_mode.read() & DATA_TLB_MASK ) 
            {
                dcache_hit_t = dcache_tlb.translate(invadr, &dpaddr); 
            } 
            else 
            {
                dpaddr = invadr;  
                dcache_hit_t = true;
            }

            if ( dcache_hit_t )
            {
		r_dcache_cleanup_req = r_dcache.inval(dpaddr, &way, &set);
		r_dcache_cleanup_line = dpaddr >> (uint32_log2(m_dcache_words)+2);
		
		if ( r_dcache_in_itlb[way*m_dcache_sets+set] || r_dcache_in_dtlb[way*m_dcache_sets+set] ) 
		{	
		    // ins tlb invalidate verification
		    r_dcache_itlb_inval_req = r_dcache_in_itlb[way*m_dcache_sets+set];
		    r_dcache_itlb_inval_line = dpaddr >> (uint32_log2(m_dcache_words)+2);
		    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
		
		    // data tlb invalidate verification
		    r_dcache_dtlb_inval_req = r_dcache_in_dtlb[way*m_dcache_sets+set];
		    r_dcache_dtlb_inval_line = dpaddr >> (uint32_log2(m_dcache_words)+2);
		    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
		    r_dcache_fsm = DCACHE_TLB_CC_INVAL;
		    r_dcache_fsm_save = r_dcache_fsm;
		    break;
		}
            }
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true;
	}
        break;
    }
    ////////////////////////
    case DCACHE_DCACHE_INVAL_PA:
    {
        m_cpt_dcache_dir_read += m_dcache_ways;
        paddr_t dpaddr = (paddr_t)r_mmu_word_hi.read() << 32 | r_mmu_word_lo.read();
        size_t way = 0;
        size_t set = 0;

	if ( !r_dcache_cleanup_req )
	{
	    r_dcache_cleanup_req = r_dcache.inval(dpaddr, &way, &set);
	    r_dcache_cleanup_line = dpaddr >> (uint32_log2(m_dcache_words)+2);
	    
	    if ( r_dcache_in_itlb[way*m_dcache_sets+set] || r_dcache_in_dtlb[way*m_dcache_sets+set] ) 
	    {	
	        // ins tlb invalidate verification
	        r_dcache_itlb_inval_req = r_dcache_in_itlb[way*m_dcache_sets+set];
	        r_dcache_itlb_inval_line = dpaddr >> (uint32_log2(m_dcache_words)+2);
	        r_dcache_in_itlb[way*m_dcache_sets+set] = false;
	    
	        // data tlb invalidate verification
	        r_dcache_dtlb_inval_req = r_dcache_in_dtlb[way*m_dcache_sets+set];
	        r_dcache_dtlb_inval_line = dpaddr >> (uint32_log2(m_dcache_words)+2);
	        r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
	        r_dcache_fsm = DCACHE_TLB_CC_INVAL;
	        r_dcache_fsm_save = r_dcache_fsm;
	        break;
	    }
            r_dcache_fsm = DCACHE_IDLE;
            drsp.valid = true;
	}
        break;
    }
    /////////////////////////
    case DCACHE_DCACHE_SYNC:
    {
        if ( r_wbuf.empty() ) 
        {    
            drsp.valid = true;
            r_dcache_fsm = DCACHE_IDLE;  
        }      
        break;
    }
    /////////////////////
    case DCACHE_MISS_WAIT:
    {
        m_cost_data_miss_frz++; 

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = MMU_READ_DATA_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;
		break;
            } 

            if ( r_dcache_inval_tlb_rsp ) // Miss read response and tlb invalidation
            {
                if ( r_dcache_cleanup_req ) break;
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_tlb_rsp = false;
                if ( r_dcache_inval_rsp ) r_dcache_inval_rsp = false;
                break;
            }	

	    if ( r_dcache_inval_rsp ) // TLB miss response and cache invalidation
	    {
                if ( r_dcache_cleanup_req ) break;
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
	    }
	    // Miss read response and no tlb invalidation
	    r_dcache_fsm = DCACHE_MISS_UPDT;
	}	
        break;
    }
    /////////////////////
    case DCACHE_MISS_UPDT:
    {
        m_cost_data_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

        if ( r_dcache_inval_tlb_rsp ) // tlb invalidation
        {
            if ( r_dcache_cleanup_req ) break;
            r_dcache_cleanup_req = true;
            r_dcache_cleanup_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words) + 2);  
            r_dcache_inval_tlb_rsp = false;
            r_dcache_inval_rsp = false;
            r_dcache_fsm = DCACHE_IDLE;
            m_cost_data_tlb_miss_frz++;
            break;
        }

        if (!r_dcache_cleanup_req ) // Miss update and no invalidation
        {
            paddr_t  victim_index = 0;
            size_t way = 0;
            size_t set = 0;

	    m_cpt_dcache_data_write++;
            m_cpt_dcache_dir_write++;

            // Using tlb entry is in the invalidated cache line  
            if ( r_dcache_inval_rsp )
            {
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
            }

            bool cleanup_req = r_dcache.find(r_dcache_paddr_save.read(), r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, &victim_index);

	    if ( cleanup_req )
	    {	    
		// ins tlb invalidate verification    
                r_dcache_itlb_inval_req = r_dcache_in_itlb[m_dcache_sets*way+set];
                r_dcache_itlb_inval_line = victim_index;
                r_dcache_in_itlb[way*m_dcache_sets+set] = false;

                // data tlb invalidate verification
                r_dcache_dtlb_inval_req = r_dcache_in_dtlb[m_dcache_sets*way+set]; 
                r_dcache_dtlb_inval_line = victim_index;
                r_dcache_in_dtlb[way*m_dcache_sets+set] = false;

                r_dcache_cleanup_req = true; 
                r_dcache_cleanup_line = victim_index;
		if ( r_dcache_in_itlb[m_dcache_sets*way+set] || r_dcache_in_dtlb[m_dcache_sets*way+set] )
		{
		    r_dcache_fsm = DCACHE_TLB_CC_INVAL;
                    r_dcache_fsm_save = r_dcache_fsm;
		    break;
		}
	    }

	    r_dcache.update(r_dcache_paddr_save.read(), way, set, r_dcache_miss_buf);
            r_dcache_fsm = DCACHE_IDLE;
        }
	break;
    }
    //////////////////////
    case DCACHE_UNC_WAIT:
    {
        m_cost_unc_read_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
            if ( r_vci_rsp_data_error ) 
            {
                r_dcache_error_type = MMU_READ_DATA_ILLEGAL_ACCESS; 
                r_dcache_bad_vaddr = dreq.addr;
                r_dcache_fsm = DCACHE_ERROR;

		if (r_dcache_inval_tlb_rsp) r_dcache_inval_tlb_rsp = false;
		break;
            }

            if ( r_dcache_inval_tlb_rsp ) // Miss read response and tlb invalidation
            {
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_tlb_rsp = false;
		break;
            }

            if(dreq.type == iss_t::DATA_SC) 
	    {
		size_t way = 0;
		size_t set = 0;
                // Simulate an invalidate request
		r_dcache_cleanup_req = r_dcache.inval(r_dcache_paddr_save, &way, &set);
		r_dcache_cleanup_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2);
		
		if ( r_dcache_in_itlb[way*m_dcache_sets+set] || r_dcache_in_dtlb[way*m_dcache_sets+set] ) 
		{	
		    // ins tlb invalidate verification
		    r_dcache_itlb_inval_req = r_dcache_in_itlb[way*m_dcache_sets+set];
		    r_dcache_itlb_inval_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2);
		    r_dcache_in_itlb[way*m_dcache_sets+set] = false;
		
		    // data tlb invalidate verification
		    r_dcache_dtlb_inval_req = r_dcache_in_dtlb[way*m_dcache_sets+set];
		    r_dcache_dtlb_inval_line = r_dcache_paddr_save.read() >> (uint32_log2(m_dcache_words)+2);
		    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
		    r_dcache_fsm = DCACHE_TLB_CC_INVAL;
		    r_dcache_fsm_save = r_dcache_fsm;
		    break;
		}

            }		
	    r_dcache_fsm = DCACHE_IDLE;
	}	
        break;
    }
    ///////////////////////
    case DCACHE_WRITE_UPDT:
    {
        m_cost_write_frz++;
        size_t way = 0;
        size_t set = 0;
	bool write_hit = false;
        data_t mask = vci_param::be2mask(r_dcache_be_save.read());
        data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
        write_hit = r_dcache.write(r_dcache_paddr_save, wdata, &way, &set);
        assert(write_hit && "Write on miss ignores data");
        
        if (r_dcache_in_itlb[way*m_dcache_sets+set] || r_dcache_in_dtlb[m_dcache_sets*way+set])
        {
	    // ins tlb invalidate verification    
            r_dcache_itlb_inval_req = r_dcache_in_itlb[m_dcache_sets*way+set];
            r_dcache_itlb_inval_line = (r_dcache.get_tag(way, set) * m_dcache_sets) + set;
            r_dcache_in_itlb[way*m_dcache_sets+set] = false;

            // data tlb invalidate verification
            r_dcache_dtlb_inval_req = r_dcache_in_dtlb[m_dcache_sets*way+set]; 
            r_dcache_dtlb_inval_line = (r_dcache.get_tag(way, set) * m_dcache_sets) + set;
            r_dcache_in_dtlb[way*m_dcache_sets+set] = false;

	    r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            r_dcache_fsm_save = r_dcache_fsm;
            break;
	}

        if ( !r_dcache_dirty_save && (r_mmu_mode.read() & DATA_TLB_MASK) )   
        {
            if ( dcache_tlb.getpagesize(r_dcache_tlb_way_save, r_dcache_tlb_set_save) )	// 2M page size, one level page table 
            {
                r_dcache_pte_update = dcache_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                r_dcache_tlb_paddr = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                r_dcache_tlb_ll_dirty_req = true;
		r_vci_rsp_data_ok = false;
                r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
                m_cpt_data_tlb_write_dirty++;
            }
            else
            {   
                if (r_dcache_hit_p_save) 
                {
                    r_dcache_pte_update = dcache_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                    r_dcache_tlb_paddr = (paddr_t)r_dcache_ptba_save|(paddr_t)(((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 3);
                    r_dcache_tlb_ll_dirty_req = true;
		    r_vci_rsp_data_ok = false;
                    r_dcache_fsm = DCACHE_LL_DIRTY_WAIT;
                    m_cpt_data_tlb_write_dirty++;
                }
                else
                {
                    r_dcache_pte_update = dcache_tlb.getpte(r_dcache_tlb_way_save, r_dcache_tlb_set_save) | PTE_D_MASK;
                    r_dcache_tlb_paddr = (paddr_t)r_mmu_ptpr << (INDEX1_NBITS+2) | (paddr_t)((dreq.addr>>PAGE_M_NBITS)<<2);
                    r_dcache_tlb_ptba_read = true;
                    r_dcache_fsm = DCACHE_DTLB1_READ_CACHE;
                }
            }
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
        m_cost_data_tlb_miss_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

        if ( r_dcache_inval_tlb_rsp ) // Miss read response and tlb invalidation
        {
            r_dcache_fsm = DCACHE_IDLE;
            r_dcache_inval_tlb_rsp = false;
	    break;
        }

	if ( r_dcache_inval_rsp ) // TLB miss response and cache invalidation
	{
            r_dcache_fsm = DCACHE_IDLE; 
            r_dcache_inval_rsp = false;
	    break;	    
	}

        r_dcache.write(r_dcache_tlb_paddr, r_dcache_pte_update);
        dcache_tlb.setdirty(r_dcache_tlb_way_save, r_dcache_tlb_set_save);
        r_dcache_fsm = DCACHE_WRITE_REQ;
        drsp.valid = true;
        drsp.rdata = 0;
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
        m_cost_data_waste_wait_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

    	if ( r_vci_rsp_data_ok ) // vci response ok
        {  
            if ( r_vci_rsp_data_error )
            {
                r_dcache_rsp_itlb_error = true;	
                r_itlb_read_dcache_req = false;
                r_vci_rsp_data_error = false;
                r_dcache_fsm = DCACHE_IDLE;

		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;
		break;
            }

	    if ( r_dcache_inval_rsp ) // TLB miss response and cache invalidation
	    {
	        if ( r_dcache_cleanup_req ) break;
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_icache_paddr_save.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
	    }

            r_dcache_fsm = DCACHE_ITLB_UPDT;
        }
	break;    	
    }
    //////////////////////
    case DCACHE_ITLB_UPDT:
    {
        m_cost_data_waste_wait_frz++;

        // external cache invalidate request
        if ( r_tgt_dcache_req ) 
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

        if ( !r_dcache_cleanup_req )
        {
            data_t rsp_itlb_miss = 0;
	    data_t rsp_itlb_ppn = 0;

            paddr_t  victim_index = 0;
            size_t way = 0;
            size_t set = 0;

	    if ( r_dcache_inval_rsp ) // TLB miss response and cache invalidation
	    {
                r_dcache_cleanup_req = true;
                r_dcache_cleanup_line = r_icache_paddr_save.read() >> (uint32_log2(m_dcache_words) + 2);  
                r_dcache_fsm = DCACHE_IDLE;
                r_dcache_inval_rsp = false;
                break;
	    }           
 
            bool cleanup = r_dcache.find(r_icache_paddr_save, r_dcache_in_itlb, r_dcache_in_dtlb, &way, &set, &victim_index);

	    if ( cleanup )
	    {	    
		// ins tlb invalidate verification    
                r_dcache_itlb_inval_req = r_dcache_in_itlb[m_dcache_sets*way+set];
                r_dcache_itlb_inval_line = victim_index;
                r_dcache_in_itlb[way*m_dcache_sets+set] = false;

                // data tlb invalidate verification
                r_dcache_dtlb_inval_req = r_dcache_in_dtlb[m_dcache_sets*way+set]; 
                r_dcache_dtlb_inval_line = victim_index;
                r_dcache_in_dtlb[way*m_dcache_sets+set] = false;

                r_dcache_cleanup_req = true; 
                r_dcache_cleanup_line = victim_index;
		if ( r_dcache_in_itlb[m_dcache_sets*way+set] || r_dcache_in_dtlb[m_dcache_sets*way+set] )
		{
		    r_dcache_fsm = DCACHE_TLB_CC_INVAL;
                    r_dcache_fsm_save = r_dcache_fsm;
		    break;
		}
	    }

            r_dcache.update(r_icache_paddr_save, way, set, r_dcache_miss_buf);

            r_dcache.setinbit(r_icache_paddr_save, r_dcache_in_itlb, true);
            bool itlb_hit_dcache = r_dcache.read(r_icache_paddr_save, &rsp_itlb_miss);	
 
	    if ( r_itlb_k_read_dcache && itlb_hit_dcache )
	    {	
		r_itlb_k_read_dcache = false;
            	bool itlb_hit_ppn = r_dcache.read(r_icache_paddr_save.read()+4, &rsp_itlb_ppn);
		assert(itlb_hit_ppn && "Address of pte[64-32] and pte[31-0] should be successive");
	    }

            r_dcache_rsp_itlb_miss = rsp_itlb_miss;
            r_dcache_rsp_itlb_ppn = rsp_itlb_ppn;
            r_dcache_rsp_itlb_error = false;	
            r_itlb_read_dcache_req = false;
            r_dcache_fsm = DCACHE_IDLE;
        }
        break;
    }
    //////////////////////////
    case DCACHE_ITLB_LL_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }

	if ( r_vci_rsp_data_ok )
	{
            if ( r_vci_rsp_data_error ) // VCI response ko
            {
                r_dcache_rsp_itlb_error = true;  
                r_vci_rsp_data_error = false;
                r_itlb_acc_dcache_req = false;
		r_dcache_fsm = DCACHE_IDLE;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;	
            }
	    else
	    {
	        if ( !(r_dcache_miss_buf[0] >> PTE_V_SHIFT) )	// unmapped
	        {
                    r_dcache_rsp_itlb_error = true;  
                    r_itlb_acc_dcache_req = false;
		    r_dcache_fsm = DCACHE_IDLE;	
		    if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;	
	        }
       		else if ( r_dcache_inval_rsp )
       		{
       		    r_dcache_inval_rsp = false;
       		    r_dcache_fsm = DCACHE_IDLE;
       		    m_cost_data_tlb_miss_frz++;
       		}
		else
		{
		    r_dcache_itlb_sc_acc_req = true;
		    r_vci_rsp_data_ok = false;
		    r_icache_pte_update = r_dcache_miss_buf[0] | r_icache_pte_update.read();	
                    r_dcache_fsm = DCACHE_ITLB_SC_WAIT; 
		}
	    }
	}
	break;
    }
    //////////////////////////
    case DCACHE_ITLB_SC_WAIT:
    {
        // external cache invalidate request
        if ( r_tgt_dcache_req )   
        {
            r_dcache_fsm = DCACHE_CC_CHECK;
            r_dcache_fsm_save = r_dcache_fsm;
            m_cost_data_waste_wait_frz++;
            break;
        }
	
        if ( r_vci_rsp_data_ok ) 
	{
	    if ( r_vci_rsp_data_error ) // VCI response ko
	    {
	        r_dcache_rsp_itlb_error = true;  
	        r_vci_rsp_data_error = false;
	        r_itlb_acc_dcache_req = false;
	        r_dcache_fsm = DCACHE_IDLE;
		if (r_dcache_inval_rsp) r_dcache_inval_rsp = false;		
	    }
	    else
	    {
	        if ( r_dcache_inval_rsp )
	        {
	            r_dcache_inval_rsp = false;
	            r_dcache_fsm = DCACHE_IDLE;
	            m_cost_data_tlb_miss_frz++;
	        }
	        else if ( r_dcache_itlb_ll_acc_req )
	        {
		    r_vci_rsp_data_ok = false;
	            r_dcache_fsm = DCACHE_ITLB_LL_WAIT; 
	        }
	        else 
	        {
	            r_itlb_acc_dcache_req = false;
	            r_dcache_fsm = DCACHE_IDLE; 
	        }
	    }
	}
	break;
    }
    /////////////////////
    case DCACHE_CC_CHECK:   // read directory in case of invalidate or update request
    {
        if ( dreq.valid ) m_cost_data_waste_wait_frz++;

        m_cpt_dcache_dir_read += m_dcache_ways;
        m_cpt_dcache_data_read += m_dcache_ways;

        if((( /*( r_dcache_fsm_save == DCACHE_UNC_WAIT ) ||*/
	     ( r_dcache_fsm_save == DCACHE_MISS_WAIT ) || ( r_dcache_fsm_save == DCACHE_MISS_UPDT ) ) && 
           ( (r_dcache_paddr_save.read() & ~((m_dcache_words<<2)-1)) == (r_tgt_addr.read() & ~((m_dcache_words<<2)-1)))) 
        || (( ( r_dcache_fsm_save == DCACHE_TLB1_READ )      || ( r_dcache_fsm_save == DCACHE_TLB2_READ )      ||
	     ( r_dcache_fsm_save == DCACHE_TLB1_READ_UPDT ) || ( r_dcache_fsm_save == DCACHE_TLB2_READ_UPDT ) ||
             ( r_dcache_fsm_save == DCACHE_TLB1_UPDT )      || ( r_dcache_fsm_save == DCACHE_TLB2_UPDT )  /*	  ||
	     ( r_dcache_fsm_save == DCACHE_TLB1_LL_WAIT )   || ( r_dcache_fsm_save == DCACHE_TLB2_LL_WAIT )   ||
	     ( r_dcache_fsm_save == DCACHE_TLB1_SC_WAIT )   || ( r_dcache_fsm_save == DCACHE_TLB2_SC_WAIT )   ||
	     ( r_dcache_fsm_save == DCACHE_LL_DIRTY_WAIT )  || ( r_dcache_fsm_save == DCACHE_SC_DIRTY_WAIT )  ||
	     ( r_dcache_fsm_save == DCACHE_WRITE_DIRTY ) */ ) && 
           ( (r_dcache_tlb_paddr.read() & ~((m_dcache_words<<2)-1)) == (r_tgt_addr.read() & ~((m_dcache_words<<2)-1))) ) 
        || (( ( r_dcache_fsm_save == DCACHE_ITLB_READ ) || ( r_dcache_fsm_save == DCACHE_ITLB_UPDT ) /*||
             ( r_dcache_fsm_save == DCACHE_ITLB_LL_WAIT ) || ( r_dcache_fsm_save == DCACHE_ITLB_SC_WAIT )*/ ) && 
           ( (r_icache_paddr_save.read() & ~((m_dcache_words<<2)-1)) == (r_tgt_addr.read() & ~((m_dcache_words<<2)-1))) ) ) 
        {
            r_dcache_inval_rsp = true;
            r_tgt_dcache_req = false;
            if ( r_tgt_update )
	    {    // Also send a cleanup and answer
                r_tgt_dcache_rsp = true;
            } 
	    else 
	    {            // Also send a cleanup but don't answer
                r_tgt_dcache_rsp = false;
            }
            r_dcache_fsm = r_dcache_fsm_save;
        }
	else
	{
            data_t dcache_rdata = 0;
            size_t way = 0;
            size_t set = 0;

            bool dcache_hit = r_dcache.read(r_tgt_addr.read(), &dcache_rdata, &way, &set);

            if ( dcache_hit )
            {
                if ( r_dcache_in_dtlb[m_dcache_sets*way+set] || r_dcache_in_itlb[m_dcache_sets*way+set] )
                {
            	    // ins tlb invalidate verification    
                    r_dcache_itlb_inval_req = r_dcache_in_itlb[m_dcache_sets*way+set];
                    r_dcache_itlb_inval_line = r_tgt_addr.read() >> (uint32_log2(m_dcache_words)+2);
                    r_dcache_in_itlb[way*m_dcache_sets+set] = false;

                    // data tlb invalidate verification
                    r_dcache_dtlb_inval_req = r_dcache_in_dtlb[m_dcache_sets*way+set]; 
                    r_dcache_dtlb_inval_line = r_tgt_addr.read() >> (uint32_log2(m_dcache_words)+2);
                    r_dcache_in_dtlb[way*m_dcache_sets+set] = false;
            	
            	    r_dcache_cc_check = true;
            	    r_dcache_fsm = DCACHE_TLB_CC_INVAL;
            	    break;
                }

                if ( r_tgt_update ) // update 
                {
                    r_dcache_fsm = DCACHE_CC_UPDT;
                } 
                else                // invalidate 
                {
                    r_dcache_fsm = DCACHE_CC_INVAL;
                }
            }
            else                    // nothing
            {
                r_dcache_fsm = DCACHE_CC_NOP;
            }
	}
        break;
    }
    ///////////////////
    case DCACHE_CC_UPDT:    // update directory and data cache        
    {
        if ( dreq.valid ) m_cost_data_waste_wait_frz++;

        m_cpt_dcache_dir_write++;
        m_cpt_dcache_data_write++;
        data_t* buf = r_tgt_buf;
        for( size_t i = 0; i < m_dcache_words; i++ )
        {
            if( r_tgt_val[i] ) r_dcache.write( r_tgt_addr.read() + i*4, buf[i] );
        }
           
        r_tgt_dcache_req = false;
	r_tgt_dcache_rsp = true;
        r_dcache_fsm = r_dcache_fsm_save;
        break;
    }
    /////////////////////
    case DCACHE_CC_INVAL:   // invalidate a cache line
    {
        if ( dreq.valid ) m_cost_data_waste_wait_frz++;

        r_tgt_dcache_rsp = r_dcache.inval(r_tgt_addr.read());
        r_tgt_dcache_req = false;
        r_dcache_fsm = r_dcache_fsm_save;
        break;
    }
    ///////////////////
    case DCACHE_CC_NOP:     // no external hit
    {
        if ( dreq.valid ) m_cost_data_waste_wait_frz++;

        r_tgt_dcache_req = false;
        if ( r_tgt_update )
	{
            r_tgt_dcache_rsp = true;
        } 
	else 
	{
            r_tgt_dcache_rsp = false;
        }

        r_dcache_fsm = r_dcache_fsm_save;
        break;
    }   
    /////////////////////////
    case DCACHE_TLB_CC_INVAL:
    {
        if ( dreq.valid ) m_cost_data_waste_wait_frz++;

	if ( r_dcache_itlb_inval_req || r_dcache_dtlb_inval_req ) break;

        if( (( r_dcache_fsm_save == DCACHE_TLB1_READ )        || ( r_dcache_fsm_save == DCACHE_TLB2_READ )        ||
             ( r_dcache_fsm_save == DCACHE_TLB1_READ_UPDT )   || ( r_dcache_fsm_save == DCACHE_TLB2_READ_UPDT )   ||
             ( r_dcache_fsm_save == DCACHE_TLB1_LL_WAIT )     || ( r_dcache_fsm_save == DCACHE_TLB2_LL_WAIT )     ||
             ( r_dcache_fsm_save == DCACHE_TLB1_SC_WAIT )     || ( r_dcache_fsm_save == DCACHE_TLB2_SC_WAIT )     ||
             ( r_dcache_fsm_save == DCACHE_TLB1_UPDT )        || ( r_dcache_fsm_save == DCACHE_TLB2_UPDT )        ||
             ( r_dcache_fsm_save == DCACHE_DTLB1_READ_CACHE ) || ( r_dcache_fsm_save == DCACHE_DTLB2_READ_CACHE ) ||
             ( r_dcache_fsm_save == DCACHE_LL_DIRTY_WAIT )    || ( r_dcache_fsm_save == DCACHE_SC_DIRTY_WAIT )    ||
	     ( r_dcache_fsm_save == DCACHE_WRITE_DIRTY )) && 
            (((r_dcache_tlb_paddr.read() & ~((m_dcache_words<<2)-1)) >> (uint32_log2(m_dcache_words) + 2)) == r_dcache_dtlb_inval_line.read()) ) 
        {
            r_dcache_inval_tlb_rsp = true;
        } 

        if (((r_dcache_fsm_save == DCACHE_BIS)||(r_dcache_fsm_save == DCACHE_MISS_WAIT) ||
             (r_dcache_fsm_save == DCACHE_UNC_WAIT)||(r_dcache_fsm_save == DCACHE_MISS_UPDT)) && 
             (r_dcache_tlb_nline == r_dcache_dtlb_inval_line))
        {
            r_dcache_inval_tlb_rsp = true;
        }

            r_dtlb_translation_valid = false;
            r_dcache_ptba_ok = false;
	if ( !r_dcache_cc_check )
	{
            r_dcache_fsm = r_dcache_fsm_save;
	}
	else
	{
            r_dcache_fsm = DCACHE_CC_CHECK;
	    r_dcache_cc_check = false;
	}
        break;
    }
    /////////////////////////
    case DCACHE_ITLB_CLEANUP:
    {
        if ( dreq.valid ) m_cost_data_waste_wait_frz++;

        r_dcache.setinbit(((paddr_t)r_dcache_itlb_cleanup_line.read()*m_dcache_words*2), r_dcache_in_itlb, false);
        r_dcache_itlb_cleanup_req = false;
        r_dcache_fsm = DCACHE_IDLE;
        break;
    }
    } // end switch r_dcache_fsm

#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Data Response: " << drsp << std::endl;
#endif

    ////////////////////////////////////////////////////////////////////////////////////
    //      INVAL DTLB CHECK FSM 
    ////////////////////////////////////////////////////////////////////////////////////////
    switch(r_inval_dtlb_fsm) {
    /////////////////////
    case INVAL_DTLB_IDLE:
    {
        if ( r_dcache_dtlb_inval_req ) 
        {
            r_ccinval_dtlb_way = 0;
            r_ccinval_dtlb_set = 0;
            r_inval_dtlb_fsm = INVAL_DTLB_CHECK;    
            m_cost_data_tlb_inval_frz++;
        }   
        break;
    }
    ////////////////////////////
    case INVAL_DTLB_CHECK:
    {
        m_cost_data_tlb_inval_frz++;

        size_t way = r_ccinval_dtlb_way; 
        size_t set = r_ccinval_dtlb_set;
        bool end = false;        
        bool tlb_hit = dcache_tlb.cccheck(r_dcache_dtlb_inval_line.read(), way, set, &way, &set, &end); 
    
        if ( tlb_hit )
        {
            r_ccinval_dtlb_way = way; 
            r_ccinval_dtlb_set = set;
            r_dtlb_cc_check_end = end;
            r_inval_dtlb_fsm = INVAL_DTLB_INVAL;
            m_cpt_data_tlb_inval++;    
        }        
        else
        {
            r_inval_dtlb_fsm = INVAL_DTLB_CLEAR;    
        }
        break;
    }
    /////////////////////////
    case INVAL_DTLB_INVAL:
    {
        m_cost_data_tlb_inval_frz++;

        dcache_tlb.ccinval(r_ccinval_dtlb_way, r_ccinval_dtlb_set);

        if ( !r_dtlb_cc_check_end )
        {
            r_inval_dtlb_fsm = INVAL_DTLB_CHECK; 
        }
        else
        {
            r_inval_dtlb_fsm = INVAL_DTLB_CLEAR;    
        }
        break;
    }
    ////////////////////
    case INVAL_DTLB_CLEAR:
    {
        r_dcache_dtlb_inval_req = false;
        r_dtlb_cc_check_end = false;
        r_ccinval_dtlb_way = 0; 
        r_ccinval_dtlb_set = 0; 
        r_inval_dtlb_fsm = INVAL_DTLB_IDLE;   
        m_cpt_data_tlb_inval++;    
        break;
    }
    } // end switch r_inval_itlb_fsm

    if ( r_dcache_fsm == DCACHE_WRITE_REQ )
    {
        r_wbuf.write(true, r_dcache_paddr_save.read(), r_dcache_be_save.read(), r_dcache_wdata_save); 
    }
    else
    {
        r_wbuf.write(false, 0, 0, 0);
    }
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
    //////////////////	
    case CLEANUP_CMD:
    {
        if ( p_vci_ini_c.cmdack.read() )
        {
            if ( r_icache_cleanup_req ) 
	    {
	        r_cleanup_fsm = CLEANUP_ICACHE_RSP;
	    }	
            else if ( r_dcache_cleanup_req ) 
	    {
	        r_cleanup_fsm = CLEANUP_DCACHE_RSP;
	    }
	}    
        break;
    }
    ////////////////////////
    case CLEANUP_ICACHE_RSP:
    {
        if ( ! p_vci_ini_c.rspval.read() )
            break;
        assert( p_vci_ini_c.reop.read() && (p_vci_ini_c.rtrdid.read() == 1) &&
                "illegal VCI response packet for icache cleanup");
        assert( (p_vci_ini_c.rerror.read() == vci_param::ERR_NORMAL) &&
                "error in response packet for icache cleanup");

        r_icache_cleanup_req = false;
        r_cleanup_fsm = CLEANUP_CMD;
        break;
    }
    /////////////////////////		
    case CLEANUP_DCACHE_RSP:
    {
        if ( ! p_vci_ini_c.rspval.read() )
            break;
        assert( p_vci_ini_c.reop.read() && (p_vci_ini_c.rtrdid.read() == 0) &&
                "illegal VCI response packet for dcache cleanup");
        assert( (p_vci_ini_c.rerror.read() == vci_param::ERR_NORMAL) &&
                "error in response packet for dcache cleanup");

        r_dcache_cleanup_req = false;
        r_cleanup_fsm = CLEANUP_CMD;
        break;
    }
    } // end switch r_cleanup_fsm

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
        if (r_dcache_itlb_read_req && r_wbuf.miss( r_icache_paddr_save ))           
        {            
            r_vci_cmd_fsm = CMD_ITLB_READ;
	    r_dcache_itlb_read_req = false;
            m_cpt_itlbmiss_transaction++; 
        } 
	else if (r_dcache_itlb_ll_acc_req && r_wbuf.miss( r_icache_paddr_save ))
	{
	    r_vci_cmd_fsm = CMD_ITLB_ACC_LL;
	    r_dcache_itlb_ll_acc_req = false;
            m_cpt_itlb_write_transaction++; 
	}
	else if (r_dcache_itlb_sc_acc_req && r_wbuf.miss( r_icache_paddr_save ))
	{
	    r_vci_cmd_fsm = CMD_ITLB_ACC_SC;
	    r_dcache_itlb_sc_acc_req = false;	    
	    m_cpt_itlb_write_transaction++; 
	}
        else if (r_icache_miss_req && r_wbuf.miss( r_icache_paddr_save )) 
        {    
            r_vci_cmd_fsm = CMD_INS_MISS;
	    r_icache_miss_req = false;
            m_cpt_imiss_transaction++; 
        }
        else if (r_icache_unc_req && r_wbuf.miss( r_icache_paddr_save )) 
        {    
            r_vci_cmd_fsm = CMD_INS_UNC;
	    r_icache_unc_req = false;
            m_cpt_imiss_transaction++; 
        }  
        else if (r_dcache_tlb_read_req && r_wbuf.miss( r_dcache_tlb_paddr )) 
        {            
            r_vci_cmd_fsm = CMD_DTLB_READ;
	    r_dcache_tlb_read_req = false;	    
	    m_cpt_dtlbmiss_transaction++; 
        } 
        else if (r_dcache_tlb_ll_acc_req && r_wbuf.miss( r_dcache_tlb_paddr )) 
        {  
            r_vci_cmd_fsm = CMD_DTLB_ACC_LL;
	    r_dcache_tlb_ll_acc_req = false;
            m_cpt_dtlb_write_transaction++; 
        } 
        else if (r_dcache_tlb_sc_acc_req && r_wbuf.miss( r_dcache_tlb_paddr )) 
        {  
            r_vci_cmd_fsm = CMD_DTLB_ACC_SC;
	    r_dcache_tlb_sc_acc_req = false;
            m_cpt_dtlb_write_transaction++; 
        } 
        else if (r_dcache_tlb_ll_dirty_req && r_wbuf.miss( r_dcache_tlb_paddr )) 
        {  
            r_vci_cmd_fsm = CMD_DTLB_DIRTY_LL;
	    r_dcache_tlb_ll_dirty_req = false;
            m_cpt_dtlb_write_transaction++; 
        } 
        else if (r_dcache_tlb_sc_dirty_req && r_wbuf.miss( r_dcache_tlb_paddr )) 
        {  
            r_vci_cmd_fsm = CMD_DTLB_DIRTY_SC;
	    r_dcache_tlb_sc_dirty_req = false;
            m_cpt_dtlb_write_transaction++; 
        } 
        else if (r_dcache_miss_req && r_wbuf.miss( r_dcache_paddr_save ))  
        {
            r_vci_cmd_fsm = CMD_DATA_MISS;
	    r_dcache_miss_req = false;
            m_cpt_dmiss_transaction++; 
        }
        else if (r_dcache_unc_req && r_wbuf.miss( r_dcache_paddr_save ))  
        {
            r_vci_cmd_fsm = CMD_DATA_UNC;
	    r_dcache_unc_req = false;
            m_cpt_unc_transaction++; 
        }
	else if ( r_wbuf.rok() )
        {
            r_vci_cmd_fsm = CMD_DATA_WRITE;
            r_vci_cmd_cpt = r_wbuf.getMin();
            r_vci_cmd_min = r_wbuf.getMin();
            r_vci_cmd_max = r_wbuf.getMax(); 
            m_cpt_write_transaction++; 
            m_length_write_transaction += (r_wbuf.getMax() - r_wbuf.getMin() + 1);
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci_ini_rw.cmdack.read() ) 
        {
            r_vci_cmd_cpt = r_vci_cmd_cpt + 1;
            if (r_vci_cmd_cpt == r_vci_cmd_max) 
            {
                r_vci_cmd_fsm = CMD_IDLE;
                r_wbuf.sent();
            }
        }
        break;

    default:
        if ( p_vci_ini_rw.cmdack.read() )
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
	if ( p_vci_ini_rw.rspval.read() )
	{	
            r_vci_rsp_cpt = 0;
	    if ( (p_vci_ini_rw.rtrdid.read() & 0x20) != 0 )         // DCACHE write response
            {
                r_vci_rsp_fsm = RSP_DATA_WRITE;
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_INS_TLB )          // ITLB miss response
            {            
                r_vci_rsp_fsm = RSP_ITLB_READ;
            } 
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_INS_LL_ACC )   // ITLB linked load response
            {   
                r_vci_rsp_fsm = RSP_ITLB_ACC_LL;
            } 
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_INS_SC_ACC )   // ITLB store conditional response
            {   
                r_vci_rsp_fsm = RSP_ITLB_ACC_SC;
            } 
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_INS_MISS )          // ICACHE cached miss response
            {   
                r_vci_rsp_fsm = RSP_INS_MISS;
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_INS_UNC )           // ICACHE uncached miss response
            {   
                r_vci_rsp_fsm = RSP_INS_UNC;
            }  
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_DATA_TLB )      // ITLB miss response
            {
                r_vci_rsp_fsm = RSP_DTLB_READ; 
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_DATA_LL_ACC )    // DTLB access bits linked load response
            {
                r_vci_rsp_fsm = RSP_DTLB_ACC_LL; 
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_DATA_SC_ACC )    // DTLB access bits store conditional response
            {
                r_vci_rsp_fsm = RSP_DTLB_ACC_SC; 
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_DATA_LL_D )  // DTLB dirty bit linked load response
            {
                r_vci_rsp_fsm = RSP_DTLB_DIRTY_LL; 
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_DATA_SC_D )  // DTLB dirty bit store conditional response
            {
                r_vci_rsp_fsm = RSP_DTLB_DIRTY_SC; 
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_DATA_MISS )          // DCACHE read response
            {
                r_vci_rsp_fsm = RSP_DATA_MISS;
            }
            else if ( p_vci_ini_rw.rtrdid.read() == TYPE_DATA_UNC )           // DCACHE uncached read response
            {
                r_vci_rsp_fsm = RSP_DATA_UNC;
            }
	}
        break;

    case RSP_ITLB_READ:
        m_cost_itlbmiss_transaction++;
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini_rw.rdata.read();
        if ( p_vci_ini_rw.reop.read() ) 
        {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                    "illegal VCI response packet for data read miss");

	    r_vci_rsp_data_ok = true;
            r_vci_rsp_fsm = RSP_IDLE;
        } 
        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_data_error = true;
        }
        break;

    case RSP_ITLB_ACC_LL:
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for ll tlb");

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
	else
	{
	    r_dcache_miss_buf[0] = (data_t)p_vci_ini_rw.rdata.read();
	}
	r_vci_rsp_data_ok = true;
        r_vci_rsp_fsm = RSP_IDLE;
	break;

    case RSP_ITLB_ACC_SC:
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for sc tlb");

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
	else if ( p_vci_ini_rw.rdata.read() == 1 ) // store conditional is not successful
	{
	    r_dcache_itlb_ll_acc_req = true;
	}
	r_vci_rsp_data_ok = true;
        r_vci_rsp_fsm = RSP_IDLE;
	break;

    case RSP_INS_MISS:
        m_cost_imiss_transaction++;
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert( (r_vci_rsp_cpt < m_icache_words) && 
               "The VCI response packet for instruction miss is too long");
        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_icache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini_rw.rdata.read();

        if ( p_vci_ini_rw.reop.read() ) 
        {
            assert( (r_vci_rsp_cpt == m_icache_words - 1) &&
                       "The VCI response packet for instruction miss is too short");
	    r_vci_rsp_ins_ok = true;
            r_vci_rsp_fsm = RSP_IDLE;
                
        } 
        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_ins_error = true;
        }
        break;

    case RSP_INS_UNC:
        m_cost_imiss_transaction++;
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for uncached instruction");

        r_icache_miss_buf[0] = (data_t)p_vci_ini_rw.rdata.read();
	r_vci_rsp_ins_ok = true;
        r_vci_rsp_fsm = RSP_IDLE;

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_ins_error = true;
        }
        break;

    case RSP_DTLB_READ:
        m_cost_dtlbmiss_transaction++;
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini_rw.rdata.read();
        if ( p_vci_ini_rw.reop.read() ) 
        {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                    "illegal VCI response packet for data read miss");
	    r_vci_rsp_data_ok = true;
            r_vci_rsp_fsm = RSP_IDLE;
        } 
        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_data_error = true;
        }
        break;

    case RSP_DTLB_ACC_LL:
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for ll tlb");

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
	else
	{
	    r_dcache_miss_buf[0] = (data_t)p_vci_ini_rw.rdata.read();
	}
	r_vci_rsp_data_ok = true;
        r_vci_rsp_fsm = RSP_IDLE;
	break;

    case RSP_DTLB_ACC_SC:
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for sc tlb");

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
	else if ( p_vci_ini_rw.rdata.read() == 1 ) // store conditional is not successful
	{
	    r_dcache_tlb_ll_acc_req = true;
	}
	r_vci_rsp_data_ok = true;
        r_vci_rsp_fsm = RSP_IDLE;
	break;

    case RSP_DTLB_DIRTY_LL:
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for ll tlb");

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
	else
	{
	    r_dcache_miss_buf[0] = (data_t)p_vci_ini_rw.rdata.read();
	}
	r_vci_rsp_data_ok = true;
        r_vci_rsp_fsm = RSP_IDLE;
	break;

    case RSP_DTLB_DIRTY_SC:
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for sc tlb");

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
	else if ( p_vci_ini_rw.rdata.read() == 1 ) // store conditional is not successful
	{
	    r_dcache_tlb_ll_dirty_req = true;
	}
	r_vci_rsp_data_ok = true;
        r_vci_rsp_fsm = RSP_IDLE;
	break;

    case RSP_DATA_UNC:
        m_cost_unc_transaction++;
        if ( ! p_vci_ini_rw.rspval.read() ) 
            break;

        assert(p_vci_ini_rw.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_vci_rsp_data_error = true;
        }
        else
        {
            r_dcache_miss_buf[0] = (data_t)p_vci_ini_rw.rdata.read();
	    r_vci_rsp_data_ok = true;
        }
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_MISS:
        m_cost_dmiss_transaction++;
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        assert(r_vci_rsp_cpt != m_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_vci_rsp_cpt = r_vci_rsp_cpt + 1;
        r_dcache_miss_buf[r_vci_rsp_cpt] = (data_t)p_vci_ini_rw.rdata.read();
        if ( p_vci_ini_rw.reop.read() ) 
        {
            assert(r_vci_rsp_cpt == m_dcache_words - 1 &&
                    "illegal VCI response packet for data read miss");
	    r_vci_rsp_data_ok = true;
            r_vci_rsp_fsm = RSP_IDLE;
        } 
        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL )
        {
            r_vci_rsp_data_error = true;
        }
        break;

    case RSP_DATA_WRITE:
        m_cost_write_transaction++;
        if ( ! p_vci_ini_rw.rspval.read() )
            break;

        if ( p_vci_ini_rw.reop.read() ) 
        {
            r_vci_rsp_fsm = RSP_IDLE;
	    r_wbuf.completed( p_vci_ini_rw.rtrdid.read() & 0x1f );
        }
        if ( p_vci_ini_rw.rerror.read() != vci_param::ERR_NORMAL ) 
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
    switch ( r_cleanup_fsm.read() ) {

    case CLEANUP_CMD:
	p_vci_ini_c.rspack  = false;
        p_vci_ini_c.cmdval  = r_icache_cleanup_req || r_dcache_cleanup_req;
        if ( r_icache_cleanup_req ) 
	{
	    p_vci_ini_c.address = r_icache_cleanup_line.read() * (m_icache_words<<2);
            p_vci_ini_c.trdid  = 1; // cleanup instruction
	}
        else 
	{                                    
	    p_vci_ini_c.address = r_dcache_cleanup_line.read() * (m_dcache_words<<2);
            p_vci_ini_c.trdid  = 0; // cleanup data
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

    case CLEANUP_ICACHE_RSP:
    case CLEANUP_DCACHE_RSP:
        p_vci_ini_c.rspack  = true;
        p_vci_ini_c.cmdval  = false;
        p_vci_ini_c.address = 0;
        p_vci_ini_c.wdata   = 0;
        p_vci_ini_c.be      = 0;
        p_vci_ini_c.plen    = 0;
        p_vci_ini_c.cmd     = vci_param::CMD_NOP;
        p_vci_ini_c.trdid   = 0;
        p_vci_ini_c.pktid   = 0;
        p_vci_ini_c.srcid   = m_srcid_c;
        p_vci_ini_c.cons    = false;
        p_vci_ini_c.wrap    = false;
        p_vci_ini_c.contig  = false;
        p_vci_ini_c.clen    = 0;
        p_vci_ini_c.cfixed  = false;
        p_vci_ini_c.eop     = false;
	break;
    } // end switch r_cleanup_fsm

    // VCI initiator response
    p_vci_ini_rw.rspack = ( r_vci_rsp_fsm != RSP_IDLE );

    // VCI initiator command
    p_vci_ini_rw.srcid  = m_srcid_rw;
    p_vci_ini_rw.cons   = false;
    p_vci_ini_rw.wrap   = false;
    p_vci_ini_rw.contig = true;
    p_vci_ini_rw.clen   = 0;
    p_vci_ini_rw.cfixed = false;

    switch (r_vci_cmd_fsm) {

    case CMD_IDLE:
        p_vci_ini_rw.cmdval  = false;
        p_vci_ini_rw.address = 0;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0;
        p_vci_ini_rw.trdid   = 0;
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 0;
        p_vci_ini_rw.cmd     = vci_param::CMD_NOP;
        p_vci_ini_rw.eop     = false;
        break;

    case CMD_ITLB_READ:     
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_icache_paddr_save.read() & m_dcache_yzmask;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_INS_TLB; // via data cache cached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = m_dcache_words << 2;
        p_vci_ini_rw.cmd     = vci_param::CMD_READ;
        p_vci_ini_rw.eop     = true;
        break;

    case CMD_ITLB_ACC_LL:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_icache_paddr_save.read() & ~0x3;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_INS_LL_ACC; // data cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.cmd     = vci_param::CMD_LOCKED_READ;
        p_vci_ini_rw.eop     = true;
	break;

    case CMD_ITLB_ACC_SC:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_icache_paddr_save.read() & ~0x3;
        p_vci_ini_rw.wdata   = r_icache_pte_update.read();
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_INS_SC_ACC; // data cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.cmd     = vci_param::CMD_STORE_COND;
        p_vci_ini_rw.eop     = true;
	break;	

    case CMD_INS_MISS:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_icache_paddr_save.read() & m_icache_yzmask;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_INS_MISS; // ins cache cached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = m_icache_words << 2;
        p_vci_ini_rw.cmd     = vci_param::CMD_READ;
        p_vci_ini_rw.eop     = true;
        break;

    case CMD_INS_UNC:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_icache_paddr_save.read() & ~0x3;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_INS_UNC; // ins cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.cmd     = vci_param::CMD_READ;
        p_vci_ini_rw.eop     = true;
        break;

    case CMD_DTLB_READ:     
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_dcache_tlb_paddr.read() & m_dcache_yzmask;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_DATA_TLB; // via dcache cached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = m_dcache_words << 2;
        p_vci_ini_rw.cmd     = vci_param::CMD_READ;
        p_vci_ini_rw.eop     = true;
        break;

    case CMD_DTLB_ACC_LL:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_DATA_LL_ACC; // data cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.cmd     = vci_param::CMD_LOCKED_READ;
        p_vci_ini_rw.eop     = true;
	break;

    case CMD_DTLB_ACC_SC:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci_ini_rw.wdata   = r_dcache_pte_update.read();
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_DATA_SC_ACC; // data cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.cmd     = vci_param::CMD_STORE_COND;
        p_vci_ini_rw.eop     = true;
	break;	

    case CMD_DTLB_DIRTY_LL:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_DATA_LL_D; // data cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.cmd     = vci_param::CMD_LOCKED_READ;
        p_vci_ini_rw.eop     = true;
	break;

    case CMD_DTLB_DIRTY_SC:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_dcache_tlb_paddr.read() & ~0x3;
        p_vci_ini_rw.wdata   = r_dcache_pte_update.read();
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_DATA_SC_D; // data cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.cmd     = vci_param::CMD_STORE_COND;
        p_vci_ini_rw.eop     = true;
	break;	

    case CMD_DATA_UNC:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_dcache_paddr_save.read() & ~0x3;
        p_vci_ini_rw.trdid   = TYPE_DATA_UNC; // data cache uncached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = 4;
        p_vci_ini_rw.eop     = true;
        switch(r_dcache_type_save) {
        case iss_t::DATA_READ:
            p_vci_ini_rw.wdata = 0;
            p_vci_ini_rw.be    = r_dcache_be_save.read();
            p_vci_ini_rw.cmd   = vci_param::CMD_READ;
            break;
        case iss_t::DATA_LL:
            p_vci_ini_rw.wdata = 0;
            p_vci_ini_rw.be    = 0xF;
            p_vci_ini_rw.cmd   = vci_param::CMD_LOCKED_READ;
            break;
        case iss_t::DATA_SC:
            p_vci_ini_rw.wdata = r_dcache_wdata_save.read();
            p_vci_ini_rw.be    = 0xF;
            p_vci_ini_rw.cmd   = vci_param::CMD_STORE_COND;
            break;
        default:
            assert("this should not happen");
        }
        break;

    case CMD_DATA_WRITE:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_wbuf.getAddress(r_vci_cmd_cpt);
        p_vci_ini_rw.wdata   = r_wbuf.getData(r_vci_cmd_cpt);
        p_vci_ini_rw.be      = r_wbuf.getBe(r_vci_cmd_cpt);
        p_vci_ini_rw.trdid   = (r_wbuf.getIndex() | 0x20); // data cache write
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = (r_vci_cmd_max - r_vci_cmd_min + 1)<<2;
        p_vci_ini_rw.cmd     = vci_param::CMD_WRITE;
        p_vci_ini_rw.eop     = (r_vci_cmd_cpt == r_vci_cmd_max);
        break;

    case CMD_DATA_MISS:
        p_vci_ini_rw.cmdval  = true;
        p_vci_ini_rw.address = r_dcache_paddr_save.read() & m_dcache_yzmask;
        p_vci_ini_rw.wdata   = 0;
        p_vci_ini_rw.be      = 0xF;
        p_vci_ini_rw.trdid   = TYPE_DATA_MISS; // data cache cached read
	p_vci_ini_rw.pktid   = 0;
        p_vci_ini_rw.plen    = m_dcache_words << 2;
        p_vci_ini_rw.cmd     = vci_param::CMD_READ;
        p_vci_ini_rw.eop     = true;
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

    case TGT_RSP_ICACHE:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = !r_tgt_icache_req.read() && r_tgt_icache_rsp.read();
        p_vci_tgt.rsrcid  = r_tgt_srcid.read();
        p_vci_tgt.rpktid  = r_tgt_pktid.read();
        p_vci_tgt.rtrdid  = r_tgt_trdid.read();
        p_vci_tgt.rdata   = 0;
        p_vci_tgt.rerror  = 0;
        p_vci_tgt.reop    = true;
        break;

    case TGT_RSP_DCACHE:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = !r_tgt_dcache_req.read() && r_tgt_dcache_rsp.read();
        p_vci_tgt.rsrcid  = r_tgt_srcid.read();
        p_vci_tgt.rpktid  = r_tgt_pktid.read();
        p_vci_tgt.rtrdid  = r_tgt_trdid.read();
        p_vci_tgt.rdata   = 0;
        p_vci_tgt.rerror  = 0;
        p_vci_tgt.reop    = true;
        break;

    case TGT_REQ_BROADCAST:
    case TGT_REQ_ICACHE:
    case TGT_REQ_DCACHE:
        p_vci_tgt.cmdack  = false;
        p_vci_tgt.rspval  = false;
        break;

    } // end switch TGT_FSM

#ifdef SOCLIB_MODULE_DEBUG 
   std::cout << name() 
      	     << "Moore R/W:" << std::hex
      	     << " p_vci_ini_rw.cmdval: " << p_vci_ini_rw.cmdval
      	     << " p_vci_ini_rw.address: " << p_vci_ini_rw.address
      	     << " p_vci_ini_rw.wdata: " << p_vci_ini_rw.wdata
      	     << " p_vci_ini_rw.cmd: " << p_vci_ini_rw.cmd
      	     << " p_vci_ini_rw.trdid: " << p_vci_ini_rw.trdid
      	     << " p_vci_ini_rw.eop: " << p_vci_ini_rw.eop
      	     << std::endl;

   std::cout << name() 
      	     << "Moore Cleanup:" << std::hex
      	     << " p_vci_ini_c.cmdval: " << p_vci_ini_c.cmdval
      	     << " p_vci_ini_c.address: " << p_vci_ini_c.address
      	     << " p_vci_ini_c.trdid: " << p_vci_ini_c.trdid
      	     << " p_vci_ini_c.cmd: " << p_vci_ini_c.cmd
      	     << " p_vci_ini_c.eop: " << p_vci_ini_c.eop
      	     << std::endl;

   std::cout << name() 
      	     << "Moore TGT:" << std::hex
      	     << " p_vci_tgt.cmdack: " << p_vci_tgt.cmdack
      	     << " p_vci_tgt.rspval: " << p_vci_tgt.rspval
      	     << " p_vci_tgt.rtrdid: " << p_vci_tgt.rtrdid
      	     << " p_vci_tgt.rsrcid: " << p_vci_tgt.rsrcid
      	     << " p_vci_tgt.reop: " << p_vci_ini_c.reop
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









