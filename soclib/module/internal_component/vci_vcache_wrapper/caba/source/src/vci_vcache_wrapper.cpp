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

/* -*- version : Speculatif Virtual CACHE -*- */

#include <cassert>
#include "arithmetics.h"
#include "../include/vci_vcache_wrapper.h"

namespace soclib { 
namespace caba {

//#define VCACHE_WRAPPER_DEBUG

#ifdef VCACHE_WRAPPER_DEBUG
namespace {
const char *ivcache_fsm_state_str[] = {
        "IVCACHE_IDLE",
        "IVCACHE_BIS",       
        "IVCACHE_TLB1_WAIT",  
        "IVCACHE_T1_LL_WAIT",  
        "IVCACHE_T1_SC_WAIT",  
        "IVCACHE_TLB1_UPDT",  
        "IVCACHE_TLB2_WAIT",  
        "IVCACHE_T2_LL_WAIT",  
        "IVCACHE_T2_SC_WAIT",  
        "IVCACHE_TLB2_UPDT",  
        "IVCACHE_TLB_FLUSH", 
        "IVCACHE_CACHE_FLUSH", 
        "IVCACHE_TLB_INVAL",  
        "IVCACHE_CACHE_INVAL1",
        "IVCACHE_CACHE_INVAL2",
        "IVCACHE_MISS_WAIT",
        "IVCACHE_UNC_WAIT",  
        "IVCACHE_MISS_UPDT",  
        "IVCACHE_ERROR", 	
    };
const char *dvcache_fsm_state_str[] = {
        "DVCACHE_IDLE",       
        "DVCACHE_BIS",       
        "DVCACHE_TLB1_WAIT",  
        "DVCACHE_T1_LL_WAIT", 
        "DVCACHE_T1_SC_WAIT",  
        "DVCACHE_TLB1_UPDT",  
        "DVCACHE_TLB2_WAIT",  
        "DVCACHE_T2_LL_WAIT", 
        "DVCACHE_T2_SC_WAIT",  
        "DVCACHE_TLB2_UPDT",   
        "DVCACHE_TLB_FLUSH",   
        "DVCACHE_CACHE_FLUSH", 
        "DVCACHE_TLB_INVAL",
        "DVCACHE_CACHE_INVAL1",
        "DVCACHE_CACHE_INVAL2",
        "DVCACHE_WRITE_UPDT", 
        "DVCACHE_WRITE_REQ",  
        "DVCACHE_MISS_WAIT",  
        "DVCACHE_MISS_UPDT",  
        "DVCACHE_UNC_WAIT",   
        "DVCACHE_ERROR",  
    };
const char *cmd_fsm_state_str[] = {
        "CMD_IDLE",           
        "CMD_ITLB_MISS",      
        "CMD_ITLB_LL",       
        "CMD_ITLB_SC",       
        "CMD_INS_MISS",     
        "CMD_INS_UNC",     
        "CMD_DTLB_MISS",    
        "CMD_DTLB_LL",       
        "CMD_DTLB_SC",        
        "CMD_DATA_UNC",     
        "CMD_DATA_MISS",    
        "CMD_DATA_WRITE",    
    };
const char *rsp_fsm_state_str[] = {
        "RSP_IDLE",                  
        "RSP_ITLB_MISS",             
        "RSP_ITLB_LL",               
        "RSP_ITLB_SC",               
        "RSP_INS_MISS",   
        "RSP_INS_UNC",           
        "RSP_DTLB_MISS",            
        "RSP_DTLB_LL",             
        "RSP_DTLB_SC",              
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

      ivcache_m_tlb(itlb_m_ways,itlb_m_sets,PAGE_M_NBITS),
      ivcache_k_tlb(itlb_k_ways,itlb_k_sets,PAGE_K_NBITS),
      dvcache_m_tlb(dtlb_m_ways,dtlb_m_sets,PAGE_M_NBITS),
      dvcache_k_tlb(dtlb_k_ways,dtlb_k_sets,PAGE_K_NBITS),

      r_dvcache_fsm("r_dvcache_fsm"),
      r_dcache_addr_save("r_dcache_addr_save"),
      r_dcache_wdata_save("r_dcache_wdata_save"),
      r_dcache_rdata_save("r_dcache_rdata_save"),
      r_dcache_type_save("r_dcache_type_save"),
      r_dcache_be_save("r_dcache_be_save"),
      r_dcache_cached_save("r_dcache_cached_save"),

      r_ivcache_fsm("r_ivcache_fsm"),
      r_ivcache_miss_addr("r_ivcache_miss_addr"),

      r_vci_cmd_fsm("r_vci_cmd_fsm"),
      r_vci_cmd_min("r_vci_cmd_min"),
      r_vci_cmd_max("r_vci_cmd_max"),
      r_vci_cmd_cpt("r_vci_cmd_cpt"),

      r_vci_rsp_fsm("r_vci_rsp_fsm"),
      r_vci_rsp_cpt("r_vci_rsp_cpt"),
      r_ivcache_rsp_error("r_ivcache_rsp_error"),
      r_dvcache_rsp_error("r_dvcache_rsp_error"),

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

/*************************************************/
tmpl(/**/)::~VciVCacheWrapper()
/*************************************************/
{
    delete [] r_icache_miss_buf;
    delete [] r_dcache_miss_buf;
}

/************************************************/
tmpl(inline bool)::is_write(data_op_t cmd)
/************************************************/
{
    return cmd == iss_t::DATA_WRITE;
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

/*************************************************/
tmpl(void)::transition()
/*************************************************/
{
    if ( ! p_resetn.read() ) 
    {
        m_iss.reset();

        r_dvcache_fsm = DVCACHE_IDLE;
        r_ivcache_fsm = IVCACHE_IDLE;
        r_vci_cmd_fsm = CMD_IDLE;
        r_vci_rsp_fsm = RSP_IDLE;

        // write buffer & caches
        r_wbuf.reset();
        r_icache.reset();
        r_dcache.reset();

        r_tlb_mode = TLBS_DEACTIVE;

        r_icache_miss_req = false;
        r_icache_unc_req = false;
        r_itlb_req    = false;
        r_itlb_ll_req = false;
        r_itlb_sc_req = false;

        r_dcache_miss_req  = false;
        r_dcache_unc_req   = false;
        r_dcache_write_req = false;
        r_dtlb_req         = false;
        r_dtlb_ll_req      = false;
        r_dtlb_sc_req      = false;

        r_dcache_buf_unc_valid = false;
        r_icache_buf_unc_valid = false;

        r_ivcache_rsp_error = false;
        r_dvcache_rsp_error = false;

        r_itlb_ptpr_save = 0;
        r_itlb_id1_save = 0;
        r_itlb_et_save = 0;

        r_dtlb_ptpr_save = 0;
        r_dtlb_id1_save = 0;
        r_dtlb_et_save = 0;

        r_icache_xtn_end = false;

        r_ivcache_error_type = MMU_NONE;
        r_dvcache_error_type = MMU_NONE;

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

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << "cycle = " << m_cpt_total_cycles << " processor " << name() 
        << " dvcache fsm: " << dvcache_fsm_state_str[r_dvcache_fsm]
        << " ivcache fsm: " << ivcache_fsm_state_str[r_ivcache_fsm]
        << " cmd fsm: " << cmd_fsm_state_str[r_vci_cmd_fsm]
        << " rsp fsm: " << rsp_fsm_state_str[r_vci_rsp_fsm] << std::endl;
#endif

    m_cpt_total_cycles++;

    typename iss_t::InstructionRequest ireq = ISS_IREQ_INITIALIZER;
    typename iss_t::InstructionResponse irsp = ISS_IRSP_INITIALIZER;

    m_iss.getInstructionRequest( ireq );

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << name() << " Instruction Request: " << ireq << std::endl;
#endif

    typename iss_t::DataRequest dreq = ISS_DREQ_INITIALIZER;
    typename iss_t::DataResponse drsp = ISS_DRSP_INITIALIZER;

    m_iss.getDataRequest( dreq );
    typename iss_t::ExternalAccessType xtn_opcod = (typename iss_t::ExternalAccessType)(dreq.addr/4);

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << name() << " Data Request: " << dreq << std::endl;
#endif

    ////////////////////////////////////////////////////////////////////////////////////////
    // The IVCACHE FSM controls the following ressources:
    // - r_ivcache_fsm
    // - r_ivcache_miss_addr 
    // - r_ivcache_miss_data
    // - r_itlb_ppn_save
    // - r_itlb_et_save 	
    // - r_itlb_ptpr_save 
    // - r_itlb_id1_save 
    // - r_itlb_ptp
    // - r_itlb_req
    // - r_icache_miss_req
    // - r_icache_unc_req
    // - r_itlb_ll_req
    // - r_itlb_sc_req
    // - r_icache_buf_unc_valid
    //
    // - r_ivcache_rsp_error
    // - r_ivcache_error_type
    // - r_ivcache_bad_vaddr
    //
    // In case of MISS, the controller writes a request in the r_ivcache_miss_addr register and
    // sets the request flip-flop(r_itlb_req, r_icache_miss_req or r_icache_unc_req) and these 
    // request flip-flops are reset by the VCI_RSP controller, when the reponse instruction is 
    // ready in the IVCACHE buffer.
    //
    // There is 9 mutually exclusive conditions to exit the IDLE state:
    // - Context switch => TLB_FLUSH state (flush TLB)
    // - Flush cache => CACHE_FLUSH state (flush cache)
    // - Invalidate a TLB entry => TLB_INVAL
    // - Invalidate a cache line => CACHE_INVAL1
    // - TLB miss(in case hit_p miss) => TLB1_WAIT
    // - TLB miss(in case hit_p hit) => TLB2_WAIT
    // - Hit in TLB but PPN changed => BIS
    // - Cached read miss => MISS_REQ
    // - Uncache read miss => UNC_REQ
    // 
    // The r_ivcache_rsp_error flip-flop is set by the VCI_RSP controller and reset by IVCACHE-FSM 
    // when its state is in IVCACHE_ERROR. 
    //
    //--------------------------------------------------------------------- 
    // Instruction TLB: 
    //  
    // - int        ET          (00: unmapped; 01: unused or PTD)
    //                          (10: PTE new;  11: PTE old      )
    // - bool       uncachable  (uncached bit)
    // - bool       writable    (** not used alwayse false) 
    // - bool       executable  (executable bit)
    // - bool       user        (access in user mode allowed)
    // - bool       global      (PTE not invalidated by a TLB flush)
    // - bool       dirty       (** not used alwayse false) 
    // - uint32_t   vpn         (virtual page number)
    // - uint32_t   ppn         (physical page number)
    ////////////////////////////////////////////////////////////////////////////////////////

    switch(r_ivcache_fsm) {

    case IVCACHE_IDLE:
    {
        addr36_t ipaddr;                                // instruction physique address
        data_t  icache_ins = 0;
        bool ivcache_hit_c = false;
        bool icache_cached;

        pte_info_t ipte_info;
        bool ivcache_hit_t_m,ivcache_hit_t_k,ivcache_hit_x,ivcache_hit_p;
        size_t i_way = 0;
        size_t i_set = 0;
        addr_t itlb_ppn_save = r_itlb_ppn_save; 

        // icache_hit_t_m, icache_hit_t_k, icache_hit_x, icache_hit_p & ipaddr computation
        if ( r_tlb_mode == TLBS_DEACTIVE || r_tlb_mode == ITLB_D_DTLB_A ) 
        {
            ipaddr = ireq.addr;         // instruction physique address
            ivcache_hit_t_m = true;     // 4M page tlb hit
            ivcache_hit_t_k = true;     // 4K page tlb hit
            ivcache_hit_x = true;       // physique page is not changed
            ivcache_hit_p = true;       // PTP is not changed
        } 
        else 
        { 
            // hit tlb
            ivcache_hit_t_m = ivcache_m_tlb.translate(ireq.addr, &ipaddr, &ipte_info, &i_way, &i_set); // 4M page tlb hit
            ivcache_hit_t_k = ivcache_k_tlb.translate(ireq.addr, &ipaddr, &ipte_info, &i_way, &i_set); // 4K page tlb hit
            
            // hit_x if the page is not changed
            if (ivcache_hit_t_m) 
            {
                ivcache_hit_x = (r_itlb_ppn_save == ivcache_m_tlb.getppn(ireq.addr));
                ipaddr = (addr36_t)(((addr36_t)itlb_ppn_save << PAGE_M_NBITS) | (ireq.addr & OFFSET_M_MASK));
            } 
            else if (ivcache_hit_t_k) 
            {
                ivcache_hit_x = (r_itlb_ppn_save == ivcache_k_tlb.getppn(ireq.addr));
                ipaddr = (addr36_t)(((addr36_t)itlb_ppn_save << PAGE_K_NBITS) | (ireq.addr & OFFSET_K_MASK));
            } 
            else 
            {
                ivcache_hit_x = false;
                ipaddr = 0;
            }
        
            //hix_p if the PTP is not changed
            ivcache_hit_p = ((r_tlb_ptpr == r_itlb_ptpr_save) && ((ireq.addr>>PAGE_M_NBITS) == r_itlb_id1_save) && (r_itlb_et_save == PTD)); 
        }

        // tlb flush
        if (dreq.valid && 
            (dreq.type == iss_t::XTN_WRITE) && (xtn_opcod == iss_t::XTN_PTPR)) 
        {
                r_ivcache_fsm = IVCACHE_TLB_FLUSH;   
                break;
        }
        // ivcache flush
        if (dreq.valid && 
            (dreq.type == iss_t::XTN_WRITE) && (xtn_opcod == iss_t::XTN_ICACHE_FLUSH)) 
        {
                r_ivcache_fsm = IVCACHE_CACHE_FLUSH;   
                r_icache_xtn_end = true;
                break;
        }
        // itlb inval
        if (dreq.valid && 
            (dreq.type == iss_t::XTN_WRITE) && (xtn_opcod == iss_t::XTN_ITLB_INVAL)) 
        {
                r_ivcache_fsm = IVCACHE_TLB_INVAL;   
                r_icache_xtn_end = true;
                break;
        }
        // ivcache inval
        if (dreq.valid && 
            (dreq.type == iss_t::XTN_WRITE) && (xtn_opcod == iss_t::XTN_ICACHE_INVAL)) 
        {
                r_ivcache_fsm = IVCACHE_CACHE_INVAL1;   
                r_icache_xtn_end = true;
                break;
        }


        if ( ireq.valid ) 
        {
            m_cpt_icache_dir_read += m_icache_ways;
            m_cpt_icache_data_read += m_icache_ways;

            icache_cached = m_cacheability_table[ipaddr];   // verify cached or not  
            // hit cache
            if ( icache_cached )
            {
                ivcache_hit_c = r_icache.read(ipaddr, &icache_ins);
            }
            else
            {
                ivcache_hit_c = ( r_icache_buf_unc_valid && (ipaddr == r_ivcache_miss_addr) );
                icache_ins = r_icache_miss_buf[0];
            }

            if ( r_tlb_mode == TLBS_ACTIVE || r_tlb_mode == ITLB_A_DTLB_D ) 
            {
                if ( ivcache_hit_t_m || ivcache_hit_t_k ) 
                {
                    // check access rights
                    if ( !ipte_info.u && (ireq.mode == iss_t::MODE_USER)) 
                    {
                        r_ivcache_error_type = r_ivcache_error_type | MMU_PRIVILEGE_VIOLATION;  // Privilege violation error
                        r_ivcache_bad_vaddr = ireq.addr;
                        irsp.valid = true;
                        irsp.error = true;
                        irsp.instruction = 0;
                        break;
                    }
                    if ( !ipte_info.x ) 
                    {
                        r_ivcache_error_type = r_ivcache_error_type | MMU_EXEC_VIOLATION;  // Protection error
                        r_ivcache_bad_vaddr = ireq.addr;
                        irsp.valid = true;
                        irsp.error = true;
                        irsp.instruction = 0;
                        break;
                    }
                }

                // update LRU and write ppn save register
                if ( ivcache_hit_t_m )
                {  
                    ivcache_m_tlb.setlru(i_way,i_set);     
                    r_itlb_ppn_save = ivcache_m_tlb.getppn(ireq.addr);
                    r_itlb_page_k_save = false;
                }
                else if ( ivcache_hit_t_k )
                {
                    ivcache_k_tlb.setlru(i_way,i_set);    
                    r_itlb_ppn_save = ivcache_k_tlb.getppn(ireq.addr);
                    r_itlb_page_k_save = true;
                }

                if ((!ivcache_hit_t_m && !ivcache_hit_t_k) && !ivcache_hit_p) 
                {
                    // walk page table 
                    r_ivcache_fsm = IVCACHE_TLB1_WAIT;
                    r_ivcache_miss_addr = (addr36_t)(r_tlb_ptpr | ((ireq.addr>>PAGE_M_NBITS)<<2));
                    r_itlb_req = true;
                    break;
                }

                if ((!ivcache_hit_t_m && !ivcache_hit_t_k) && ivcache_hit_p ) 
                {
                    // walk page table 
                    addr36_t itlb_ptp = r_itlb_ptp;
                    r_ivcache_miss_addr = (addr36_t)(itlb_ptp | (((ireq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2));
                    r_itlb_req = true;
                    r_ivcache_fsm = IVCACHE_TLB2_WAIT;
                    break;
                }

                if ( (ivcache_hit_t_m || ivcache_hit_t_k) && !ivcache_hit_x ) 
                {
                    r_ivcache_fsm = IVCACHE_BIS;
                    break;
                }
            }

            if ((ivcache_hit_t_m || ivcache_hit_t_k) && ivcache_hit_x ) 
            {
                if ( ! ivcache_hit_c ) 
                {
                    m_cpt_ins_miss++;
                    m_cost_ins_miss_frz++;
                    r_ivcache_miss_addr = ipaddr;
                    if ( icache_cached ) 
                    {
                        r_ivcache_fsm = IVCACHE_MISS_WAIT;
                        r_icache_miss_req = true;
                    } 
                    else 
                    {
                        r_ivcache_fsm = IVCACHE_UNC_WAIT;
                        r_icache_unc_req = true;
                        r_icache_buf_unc_valid = false;
                    } 
                } 
                else 
                {
                    r_icache_buf_unc_valid = false;
                    r_ivcache_fsm = IVCACHE_IDLE;
                }
                irsp.valid = (ivcache_hit_c && ivcache_hit_x && (ivcache_hit_t_m || ivcache_hit_t_k));
                irsp.instruction = icache_ins;
            }
            
        }
        break;
    }

    case IVCACHE_BIS: 
    {
        addr36_t ipaddr;                                // instruction physique address
        data_t  icache_ins = 0;
        bool icache_cached = false;
        bool ivcache_hit_c = false;
        addr_t itlb_ppn_save = r_itlb_ppn_save;

        if ( r_itlb_page_k_save )
        {
            ipaddr = (addr36_t)(((addr36_t)itlb_ppn_save << PAGE_K_NBITS) | (ireq.addr & OFFSET_K_MASK));
        }
        else 
        {
            ipaddr = (addr36_t)(((addr36_t)itlb_ppn_save << PAGE_M_NBITS) | (ireq.addr & OFFSET_M_MASK));
        }

        icache_cached = m_cacheability_table[ipaddr];
        if ( icache_cached )
        {
            ivcache_hit_c = r_icache.read(ipaddr, &icache_ins);
        }
        else
        {
            ivcache_hit_c = ( r_icache_buf_unc_valid && (ipaddr == r_ivcache_miss_addr) );
            icache_ins = r_icache_miss_buf[0];
        }

        if ( ! ivcache_hit_c ) 
        {
            m_cpt_ins_miss++;
            m_cost_ins_miss_frz++;
            r_ivcache_miss_addr = ipaddr;
            if ( icache_cached ) 
            {
                r_ivcache_fsm = IVCACHE_MISS_WAIT;
                r_icache_miss_req = true;
            } 
            else 
            {
                r_ivcache_fsm = IVCACHE_UNC_WAIT;
                r_icache_unc_req = true;
                r_icache_buf_unc_valid = false;
            } 
        } 
        else
        {
            r_icache_buf_unc_valid = false;
            r_ivcache_fsm = IVCACHE_IDLE; 
        }

        irsp.valid = ivcache_hit_c;
        irsp.instruction = icache_ins;
        break;
    }

    case IVCACHE_TLB1_WAIT:
    {
        if ( !r_itlb_req && !r_ivcache_rsp_error) // vci response ok
        {  
            // renew tlb save register
        	r_itlb_et_save = (r_itlb_miss_rsp & PTE_ET_MASK ) >> PTE_ET_SHIFT;	
            r_itlb_ptpr_save = r_tlb_ptpr;
            r_itlb_id1_save = ireq.addr>>PAGE_M_NBITS;
	
            switch((r_itlb_miss_rsp & PTE_ET_MASK ) >> PTE_ET_SHIFT) { 
            case PTD:    // 4K page TLB
            {
                addr36_t itlb_ptp = (addr36_t)(r_itlb_miss_rsp & PTD_PTP_MASK) << 12; 
                r_itlb_ptp = itlb_ptp;
                r_ivcache_miss_addr = (addr36_t)(itlb_ptp | (((ireq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2));
                r_itlb_req = true;
                r_ivcache_fsm = IVCACHE_TLB2_WAIT;
                break;
            }
            case PTE_NEW:// 4M page TLB (not marked)   
                r_ivcache_fsm = IVCACHE_T1_LL_WAIT;
                r_itlb_ll_req = true;
                break;  
            case PTE_OLD:// 4M page TLB (already marked)
                r_ivcache_fsm = IVCACHE_TLB1_UPDT;
                break;
            default:     // unmapped
                r_ivcache_error_type = r_ivcache_error_type | MMU_PT1_UNMAPPED;  
                r_ivcache_bad_vaddr = ireq.addr;
                r_ivcache_fsm = IVCACHE_ERROR;
                break;
            }
        }

        if ( !r_itlb_req && r_ivcache_rsp_error ) // vci response error
        {  
            r_ivcache_fsm = IVCACHE_ERROR;
            r_ivcache_error_type = r_ivcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    
            r_ivcache_bad_vaddr = ireq.addr;
        }
        break;
    }

    case IVCACHE_T1_LL_WAIT:  
        if (!r_itlb_ll_req)
        { 
            if ( r_ivcache_rsp_error ) 
            {
                r_ivcache_fsm = IVCACHE_ERROR;
                r_ivcache_error_type = r_ivcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    // Access bus error
                r_ivcache_bad_vaddr = ireq.addr;
            } 
            else  
            {
                r_ivcache_fsm = IVCACHE_T1_SC_WAIT;  
                r_itlb_sc_req = true;
                r_ivcache_miss_data = r_itlb_miss_rsp | PTE_ET_MASK;
            }
        } 
        break;

    case IVCACHE_T1_SC_WAIT:  
        if (!r_itlb_sc_req) 
        {
            if ( r_ivcache_rsp_error ) 
            {
                r_ivcache_fsm = IVCACHE_ERROR;
                r_ivcache_error_type = r_ivcache_error_type | MMU_PT1_ILLEGAL_ACCESS;      // Access bus error
                r_ivcache_bad_vaddr = ireq.addr;
            } 
            else if ( r_itlb_ll_req ) // ll-sc not successful
            {         
                r_ivcache_fsm = IVCACHE_T1_LL_WAIT;
            } 
            else  
            {
                r_ivcache_fsm = IVCACHE_TLB1_UPDT;    // TLB update
            }
        } 
        break;

    case IVCACHE_TLB1_UPDT: // update 4M page TLB
    {
        bool write = false;
        ivcache_m_tlb.update(r_itlb_miss_rsp,ireq.addr,write);
        r_ivcache_fsm = IVCACHE_IDLE;
        break;
    }

    case IVCACHE_TLB2_WAIT:
    {
        if ( !r_itlb_req && !r_ivcache_rsp_error) 
        {
            switch((r_itlb_miss_rsp & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTE_NEW:   // to set PTE accessed  
                r_ivcache_fsm = IVCACHE_T2_LL_WAIT;
                r_itlb_ll_req = true;
                break;  
            case PTE_OLD:   
                r_ivcache_fsm = IVCACHE_TLB2_UPDT;
                break;
            default:
                r_ivcache_error_type = r_ivcache_error_type | MMU_PT2_UNMAPPED;      // Translation error
                r_ivcache_bad_vaddr = ireq.addr;
                r_ivcache_fsm = IVCACHE_ERROR;
                break;
            }
        }

        if ( !r_itlb_req && r_ivcache_rsp_error ) 
        {
            r_ivcache_fsm = IVCACHE_ERROR;
            r_ivcache_error_type = r_ivcache_error_type | MMU_PT2_ILLEGAL_ACCESS;    // Access bus error
            r_ivcache_bad_vaddr = ireq.addr;
        }
        break;
    }

    case IVCACHE_T2_LL_WAIT:  
        if (!r_itlb_ll_req) 
        {
            if ( r_ivcache_rsp_error ) 
            {
                r_ivcache_fsm = IVCACHE_ERROR;
                r_ivcache_error_type = r_ivcache_error_type | MMU_PT2_ILLEGAL_ACCESS;    // Access bus error
                r_ivcache_bad_vaddr = ireq.addr;
            } 
            else  
            {
                r_ivcache_fsm = IVCACHE_T2_SC_WAIT;  
                r_itlb_sc_req = true;
                r_ivcache_miss_data = r_itlb_miss_rsp | PTE_ET_MASK;
            }
        } 
        break;

    case IVCACHE_T2_SC_WAIT:  
        if (!r_itlb_sc_req)
        { 
            if ( r_ivcache_rsp_error ) 
            {
                r_ivcache_fsm = IVCACHE_ERROR;
                r_ivcache_error_type = r_ivcache_error_type | MMU_PT2_ILLEGAL_ACCESS;    // Access bus error
                r_ivcache_bad_vaddr = ireq.addr;
            } 
            else if (r_itlb_ll_req)     // ll-sc not successful
            {         
                r_ivcache_fsm = IVCACHE_T2_LL_WAIT;
            } 
            else  
            {
                r_ivcache_fsm = IVCACHE_TLB2_UPDT;  // TLB update
            }
        } 
        break;

    case IVCACHE_TLB2_UPDT: // update 4K page TLB
    {
        bool write = false;
        ivcache_k_tlb.update(r_itlb_miss_rsp,ireq.addr,write);
        r_ivcache_fsm = IVCACHE_IDLE;
        break;
    }

    case IVCACHE_TLB_FLUSH:
    {
        bool all = (bool)!r_context_sw_itlb;
        ivcache_m_tlb.flush(all);    // initiale instruction TLB
        ivcache_k_tlb.flush(all);    // initiale instruction TLB
        r_ivcache_fsm = IVCACHE_IDLE;
        r_context_sw_itlb = false;
        break;
    }

    case IVCACHE_CACHE_FLUSH:
        r_icache.reset();
        r_ivcache_fsm = IVCACHE_IDLE;
        break;

    case IVCACHE_TLB_INVAL:  
    {
    	addr_t invadr = dreq.wdata;
        bool itlb_m_hit = ivcache_m_tlb.translate(invadr); 
        bool itlb_k_hit = ivcache_k_tlb.translate(invadr);
 
		if (itlb_m_hit)
        {
            ivcache_m_tlb.inval(invadr);
        }
        else if (itlb_k_hit)
        {
        	ivcache_k_tlb.inval(invadr);
        }
	
        r_ivcache_fsm = IVCACHE_IDLE;
        break;
	}

    case IVCACHE_CACHE_INVAL1:
    {	
        addr_t invadr = dreq.wdata;
        addr36_t  ipaddr;                       // instruction physique address
        bool itlb_m_hit, itlb_k_hit;

        if ( r_tlb_mode == TLBS_ACTIVE || r_tlb_mode == ITLB_A_DTLB_D ) 
        {
            itlb_m_hit = ivcache_m_tlb.translate(invadr, &ipaddr); 
            itlb_k_hit = ivcache_k_tlb.translate(invadr, &ipaddr); 
        } 
        else 
        {
            ipaddr = invadr;                    // instruction physique address
            itlb_m_hit = true; 
            itlb_k_hit = true;
        }

        if ( itlb_m_hit || itlb_k_hit )
        {
            r_ivcache_fsm = IVCACHE_CACHE_INVAL2;
            r_ivcache_miss_addr = ipaddr;
        }
        else
        {
            r_ivcache_fsm = IVCACHE_IDLE;
        }
        break;
    }

    case IVCACHE_CACHE_INVAL2:
        r_icache.inval(r_ivcache_miss_addr);   
        r_ivcache_fsm = IVCACHE_IDLE;
        break;

    case IVCACHE_MISS_WAIT:
        m_cost_ins_miss_frz++;
        if ( !r_icache_miss_req )
        {
            if ( r_ivcache_rsp_error ) 
            {
                r_ivcache_fsm = IVCACHE_ERROR;
                r_ivcache_error_type = r_ivcache_error_type | MMU_CACHE_ILLEGAL_ACCESS;    // Access bus error
                r_ivcache_bad_vaddr = ireq.addr;
            } 
            else 
            {
                r_ivcache_fsm = IVCACHE_MISS_UPDT;  // TLB update
            } 
        }
        break;

    case IVCACHE_UNC_WAIT:
        m_cost_ins_miss_frz++;
        if ( !r_icache_unc_req ) 
        {
            if ( r_ivcache_rsp_error ) 
            {
                r_ivcache_fsm = IVCACHE_ERROR;
                r_ivcache_error_type = r_ivcache_error_type | MMU_CACHE_ILLEGAL_ACCESS;    // Access bus error
                r_ivcache_bad_vaddr = ireq.addr;
            } 
            else 
            {
                r_ivcache_fsm = IVCACHE_IDLE;
                r_icache_buf_unc_valid = true;
            }
        }
        break;

    case IVCACHE_MISS_UPDT:
    {
        addr36_t ipaddr = r_ivcache_miss_addr;
        data_t* buf = r_icache_miss_buf;
        addr36_t  victim_index = 0;
        m_cpt_icache_dir_write++;
        m_cpt_icache_data_write++;
        m_cost_ins_miss_frz++;
        r_icache.update(ipaddr, buf, &victim_index);
        r_ivcache_fsm = IVCACHE_IDLE;
        break;
    }

    case IVCACHE_ERROR:
        r_ivcache_rsp_error = false;
        irsp.valid = true;
        irsp.error = true;
        irsp.instruction = 0; 
        r_ivcache_fsm = IVCACHE_IDLE;
        break;
    
    } // end switch r_ivcache_fsm

    m_iss.setInstruction( irsp );

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << name() << " Instruction Response: " << irsp << std::endl;
#endif

    //////////////////////////////////////////////////////////////////////://///////////
    // The DVCACHE FSM controls the following ressources:
    // - r_dvcache_fsm
    // - r_dcache_addr_save
    // - r_dcache_type_save
    // - r_dcache_wdata_save
    // - r_dcache_be_save
    // - r_dcache_rdata_save
    // - r_dcache_cached_save
    // - r_dcache_buf_unc_valid 
    // 
    // - r_tlb_ptpr
    // - r_tlb_mode
    // - r_dtlb_ppn_save
    // - r_dtlb_et_save 	
    // - r_dtlb_ptpr_save 
    // - r_dtlb_id1_save 
    // - r_dtlb_ptp
    // - r_dtlb_req
    // - r_dcache_miss_req
    // - r_dcache_unc_req
    // - r_dcache_write_req
    // - r_dtlb_ll_req
    // - r_dtlb_sc_req
    //
    // - r_dvcache_rsp_error
    // - r_dvcache_error_type
    // - r_dvcache_bad_vaddr
    //
    // In the IDLE state, the processor request is saved in r_dcache_save.
    // The request type takes into account the cacheability_table and uncached
    // bit of PTE.
    //
    // There is 11 mutually exclusive conditions to exit the IDLE state:
    // - Context switch => TLB_FLUSH state (flush TLB)
    // - Flush cache => CACHE_FLUSH state (flush cache)
    // - Invalidate a TLB entry => TLB_INVAL
    // - Invalidate a cache line => CACHE_INVAL1
    // - TLB miss(in case hit_p miss) => TLB1_WAIT
    // - TLB miss(in case hit_p hit) => TLB2_WAIT
    // - Hit in TLB but PPN changed => BIS
    // - Cached read miss => MISS_REQ
    // - Uncache read miss => UNC_REQ
    // - Write hit => WRITE_UPDT
    // - Write miss or after update cache when write hit => WRITE_REQ
    //
    // The r_dvcache_rsp_error flip-flop is set by the VCI_RSP controller and reset 
    // by DVCACHE-FSM when its state is in DVCACHE_ERROR. 
    //--------------------------------------------------------------------- 
    // Data TLB: 
    //  
    // - int        ET          (00: unmapped; 01: unused or PTD)
    //                          (10: PTE new;  11: PTE old      )
    // - bool       uncachable  (uncached bit)
    // - bool       writable    (writable bit) 
    // - bool       executable  (** not used alwayse false)
    // - bool       user        (access in user mode allowed)
    // - bool       global      (PTE not invalidated by a TLB flush)
    // - bool       dirty       (page has been modified) 
    // - uint32_t   vpn         (virtual page number)
    // - uint32_t   ppn         (physical page number)
    ////////////////////////////////////////////////////////////////////////////////////////

    switch (r_dvcache_fsm) {

    case DVCACHE_WRITE_REQ:
        // try to post the write request in the write buffer
        if ( !r_dcache_write_req )     // no previous write transaction     
        {
            if ( r_wbuf.wok(r_dcache_addr_save) )   // write request in the same cache line 
            {    
                r_wbuf.write(r_dcache_addr_save, r_dcache_be_save, r_dcache_wdata_save);
                // closing the write packet if uncached
                if ( !r_dcache_cached_save )
                { 
                    r_dcache_write_req = true ;
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
        if ( !dreq.valid || !is_write(dreq.type)) 
        {
            r_dcache_write_req = true ;
        }
        
        // The next state and the processor request parameters are computed 
        // as in the DCACHE_IDLE state (see below ...)

    case DVCACHE_IDLE:
    {
        if (dreq.valid) 
        {
            addr36_t  dpaddr;                           // data physique address
            pte_info_t dpte_info;
            bool dvcache_hit_t_m,dvcache_hit_t_k,dvcache_hit_x,dvcache_hit_p;
            size_t d_way = 0;
            size_t d_set = 0;
            
            bool dvcache_hit_c = false;
            data_t dcache_rdata = 0;
            bool dcache_cached; 
            addr_t dtlb_ppn_save = r_dtlb_ppn_save;

            m_cpt_dcache_data_read += m_dcache_ways;
            m_cpt_dcache_dir_read += m_dcache_ways;

            if ( r_tlb_mode == TLBS_DEACTIVE || r_tlb_mode == ITLB_A_DTLB_D ) 
            {
                dpaddr = dreq.addr;              // data physique address
                dvcache_hit_t_m = true;          // 4M page tlb hit
                dvcache_hit_t_k = true;          // 4K page tlb hit
                dvcache_hit_x = true;            // ppn no change hit
                dvcache_hit_p = true;            // ptp no change hit
            } 
            else 
            {
                //hix_t if hit in TLB 
                dvcache_hit_t_m = dvcache_m_tlb.translate(dreq.addr, &dpaddr, &dpte_info, &d_way, &d_set); // 4M page tlb hit
                dvcache_hit_t_k = dvcache_k_tlb.translate(dreq.addr, &dpaddr, &dpte_info, &d_way, &d_set); // 4K page tlb hit
                //hit_x if the page is not changed
                if (dvcache_hit_t_m) 
                {
                    dvcache_hit_x = (r_dtlb_ppn_save == dvcache_m_tlb.getppn(dreq.addr));
                    dpaddr = (addr36_t)(((addr36_t)dtlb_ppn_save << PAGE_M_NBITS) | (dreq.addr & OFFSET_M_MASK));
                } 
                else if (dvcache_hit_t_k) 
                {
                    dvcache_hit_x = (r_dtlb_ppn_save == dvcache_k_tlb.getppn(dreq.addr));
                    dpaddr = (addr36_t)(((addr36_t)dtlb_ppn_save << PAGE_K_NBITS) | (dreq.addr & OFFSET_K_MASK));
                } 
                else 
                {
                    dvcache_hit_x = false;
                    dpaddr = 0;
                }

                //hix_p if the PTP is not changed
                dvcache_hit_p = ((r_tlb_ptpr == r_dtlb_ptpr_save) && ((dreq.addr>>PAGE_M_NBITS) == r_dtlb_id1_save) && (r_dtlb_et_save == PTD)); 
            }
 
            if (dreq.type == iss_t::XTN_READ) 
            {
                switch(xtn_opcod) {
                case iss_t::XTN_INS_ERROR_TYPE:
                    dcache_rdata = (uint32_t)r_ivcache_error_type;
                    r_ivcache_error_type = MMU_NONE;
                    break;
                case iss_t::XTN_DATA_ERROR_TYPE:
                    dcache_rdata = (uint32_t)r_dvcache_error_type;
                    r_dvcache_error_type = MMU_NONE;
                    break;
                case iss_t::XTN_INS_BAD_VADDR:
                    dcache_rdata = (uint32_t)r_ivcache_bad_vaddr;       
                    break;
                case iss_t::XTN_DATA_BAD_VADDR:
                    dcache_rdata = (uint32_t)r_dvcache_bad_vaddr;        
                    break;
                case iss_t::XTN_PTPR:
                    dcache_rdata = (uint32_t)r_tlb_ptpr;
                    break;
                default:
                    break;
                }
                drsp.valid = true;
                drsp.rdata = dcache_rdata;
                break;
            }

            if (dreq.type == iss_t::XTN_WRITE) 
            {
                switch(xtn_opcod) {    // address indicate to do which operation
                case iss_t::XTN_PTPR:    // context switch flush TLB
                    if (dreq.mode == iss_t::MODE_KERNEL) 
                    {
                        r_tlb_ptpr = dreq.wdata;
                        r_dvcache_fsm = DVCACHE_TLB_FLUSH;
                        r_context_sw_itlb = true;
                        r_context_sw_dtlb = true;
                        r_ivcache_error_type = MMU_NONE;
                        r_dvcache_error_type = MMU_NONE;
                    } 
                    else 
                    { 
                        r_dvcache_error_type = MMU_PRIVILEGE_VIOLATION;  // Privilege violation error
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;

                    }
                    break;
                case iss_t::XTN_TLB_EN:
                    if (dreq.mode == iss_t::MODE_KERNEL) 
                    {
                        r_tlb_mode = (int)dreq.wdata;
                        drsp.valid = true;
                    } 
                    else 
                    {
                        r_dvcache_error_type = r_dvcache_error_type | MMU_PRIVILEGE_VIOLATION;  // Privilege violation error
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;

                    }
                    break;
                case iss_t::XTN_DCACHE_FLUSH:
                    r_dvcache_fsm = DVCACHE_CACHE_FLUSH;   // dvcache flush
                    break;
                case iss_t::XTN_DTLB_INVAL:
                    r_dvcache_fsm = DVCACHE_TLB_INVAL;   // ivcache inval
                    break;
                case iss_t::XTN_DCACHE_INVAL:
                    r_dvcache_fsm = DVCACHE_CACHE_INVAL1;   // ivcache inval
                    break;
                default:
                    if ( r_icache_xtn_end )
                    {
                        drsp.valid = true;
                        r_icache_xtn_end = false;
                    }
                    break;
                }
                break;
            }

            // dcache_cached evaluation
            if ((dreq.type == iss_t::DATA_LL) || (dreq.type == iss_t::DATA_SC) ||
                (dreq.type == iss_t::XTN_READ)|| (dreq.type == iss_t::XTN_WRITE)) 
            {
                dcache_cached = false;
            } 
            else 
            {
                dcache_cached = m_cacheability_table[dpaddr];     
            }

            if ( r_tlb_mode == TLBS_ACTIVE || r_tlb_mode == ITLB_D_DTLB_A ) 
            {
                if ( dvcache_hit_t_m || dvcache_hit_t_k ) 
                {
                    // check access rights
                    if (!dpte_info.u && (dreq.mode == iss_t::MODE_USER)) 
                    {
                        r_dvcache_error_type = r_dvcache_error_type | MMU_PRIVILEGE_VIOLATION;  // Privilege violation error
                        r_dvcache_bad_vaddr = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;
                        break;
                    }
                    // TLB hit, verify the access right
                    if ( is_write(dreq.type) && !dpte_info.w ) 
                    {
                        r_dvcache_error_type = r_dvcache_error_type | MMU_EXEC_VIOLATION;  // Protection error
                        r_dvcache_bad_vaddr = dreq.addr;
                        drsp.valid = true;
                        drsp.error = true;
                        drsp.rdata = 0;
                        break;
                    }

                    if ( dpte_info.uc )   // set is non cacheable
                    {
                        dcache_cached = false;
                    }    
                    r_dcache_cached_save = dcache_cached;               
                }

                // update LRU, dirty and write ppn save register
                if (dvcache_hit_t_m) 
                {
                    dvcache_m_tlb.setdirty(d_way,d_set,is_write(dreq.type));
                    dvcache_m_tlb.setlru(d_way,d_set);
                    r_dtlb_ppn_save = dvcache_m_tlb.getppn(dreq.addr);
                    r_dtlb_page_k_save = false;
                } 
                else if ( dvcache_hit_t_k ) 
                {
                    dvcache_k_tlb.setdirty(d_way,d_set,is_write(dreq.type));
                    dvcache_k_tlb.setlru(d_way,d_set);
                    r_dtlb_ppn_save = dvcache_k_tlb.getppn(dreq.addr);
                    r_dtlb_page_k_save = true;
                }

                if ((!dvcache_hit_p && (!dvcache_hit_t_m && !dvcache_hit_t_k)) 
                 || ((dvcache_hit_t_m || dvcache_hit_t_k) && is_write(dreq.type) && !dpte_info.d))
                {
                    // save_data and save_type need not be set
                    r_dcache_addr_save = (addr36_t)(r_tlb_ptpr | ((dreq.addr>>PAGE_M_NBITS)<<2));
                    r_dcache_type_save      = iss_t::DATA_READ;
                    r_dcache_wdata_save     = 0xFFFFFFFF;
                    r_dcache_be_save        = 0xF;
                    r_dcache_rdata_save     = 0xFFFFFFFF;
                    r_dcache_cached_save    = false;
                    r_dtlb_req              = true;
                    r_dvcache_fsm = DVCACHE_TLB1_WAIT;
                    break;
                }

                if ( dvcache_hit_p && (!dvcache_hit_t_m && !dvcache_hit_t_k))
                {
                    // walk page table 
                    addr36_t dtlb_ptp = r_dtlb_ptp; 
                    r_dtlb_pte_addr = (addr36_t)(dtlb_ptp | (((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2));
                    r_dcache_addr_save = (addr36_t)(dtlb_ptp | (((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2));
                    r_dcache_type_save      = iss_t::DATA_READ;
                    r_dcache_wdata_save     = 0xFFFFFFFF;
                    r_dcache_be_save        = 0xF;
                    r_dcache_rdata_save     = 0xFFFFFFFF;
                    r_dcache_cached_save    = false;
                    r_dtlb_req              = true;
                    r_dvcache_fsm = DVCACHE_TLB2_WAIT;
                    break;
                }

                if ( (dvcache_hit_t_m || dvcache_hit_t_k) && !dvcache_hit_x )
                {
                    r_dvcache_fsm = DVCACHE_BIS;
                    break;
                }
            }

            if ( dcache_cached ) 
            {
                dvcache_hit_c = r_dcache.read(dpaddr, &dcache_rdata);
            } 
            else 
            {
                dvcache_hit_c = ((dpaddr == r_dcache_addr_save) && r_dcache_buf_unc_valid ); 
                dcache_rdata = r_dcache_miss_buf[0];
            }

            if ((dvcache_hit_t_m || dvcache_hit_t_k) && dvcache_hit_x)
            {
                switch( dreq.type ) {
                    case iss_t::DATA_READ:
                    case iss_t::DATA_LL:
                    case iss_t::DATA_SC:
                        m_cpt_read++;
                        if ( dvcache_hit_c ) 
                        {
                            r_dvcache_fsm = DVCACHE_IDLE;
                            drsp.valid = true;
                            drsp.rdata = dcache_rdata;
                            r_dcache_buf_unc_valid = false;
                        } 
                        else 
                        {
                            if ( dcache_cached ) 
                            {
                                m_cpt_data_miss++;
                                m_cost_data_miss_frz++;
                                r_dcache_miss_req = true;
                                r_dvcache_fsm = DVCACHE_MISS_WAIT;
                            } 
                            else 
                            {
                                m_cpt_unc_read++;
                                m_cost_unc_read_frz++;
                                r_dcache_unc_req = true;
                                r_dvcache_fsm = DVCACHE_UNC_WAIT;
                            }
                        }
                        break;
                    case iss_t::XTN_READ:
                    case iss_t::XTN_WRITE:
                        // only DCACHE INVALIDATE request are supported
                        //r_dcache_fsm = DCACHE_INVAL;
                        //drsp.valid = true;
                        break;
                    case iss_t::DATA_WRITE:
                        m_cpt_write++;
                        if ( dvcache_hit_c && dcache_cached ) 
                        {
                            r_dvcache_fsm = DVCACHE_WRITE_UPDT;
                            m_cpt_write_cached++;
                        } 
                        else 
                        {
                            r_dvcache_fsm = DVCACHE_WRITE_REQ;
                        }
                        drsp.valid = true;
                        drsp.rdata = 0;
                        break;
                } // end switch dreq.type

                r_dcache_addr_save      = dpaddr;
                r_dcache_type_save      = dreq.type;
                r_dcache_wdata_save     = dreq.wdata;
                r_dcache_be_save        = dreq.be;
                r_dcache_rdata_save     = dcache_rdata;
                r_dcache_cached_save    = dcache_cached;
            }
        }
        else 
        {    // end if dcache_req
            r_dvcache_fsm = DVCACHE_IDLE;
        }
        // processor request are not accepted in the WRITE_REQUEST state 
        // when the write buffer is not writeable
        if ((r_dvcache_fsm == DVCACHE_WRITE_REQ) && 
            (r_dcache_write_req || !r_wbuf.wok(r_dcache_addr_save))) 
        {
            drsp.valid = false;
        }
        break;
    }

    case DVCACHE_BIS:
    {
        addr36_t  dpaddr;                           // data physique address
        
        bool dvcache_hit_c = false;
        data_t dcache_rdata = 0;
        bool dcache_cached = false;
        addr_t dtlb_ppn_save = r_dtlb_ppn_save;

        if ( r_dtlb_page_k_save ) 
        {
            dpaddr = (addr36_t)(((addr36_t)dtlb_ppn_save << PAGE_K_NBITS) | (dreq.addr & OFFSET_K_MASK));
        } 
        else  
        {
            dpaddr = (addr36_t)(((addr36_t)dtlb_ppn_save << PAGE_M_NBITS) | (dreq.addr & OFFSET_M_MASK));
        }
 
        dcache_cached = (m_cacheability_table[dpaddr] && r_dcache_cached_save );
        if ( dcache_cached ) 
        {
            dvcache_hit_c = r_dcache.read(dpaddr, &dcache_rdata);
        } 
        else 
        {
            dvcache_hit_c = ((dpaddr == r_dcache_addr_save) && r_dcache_buf_unc_valid ); 
            dcache_rdata = r_dcache_miss_buf[0];
        }

        switch( dreq.type ) {
            case iss_t::DATA_READ:
            case iss_t::DATA_LL:
            case iss_t::DATA_SC:
                m_cpt_read++;
                if ( dvcache_hit_c ) 
                {
                    r_dvcache_fsm = DVCACHE_IDLE;
                    drsp.valid = true;
                    drsp.rdata = dcache_rdata;
                    r_dcache_buf_unc_valid = false;
                } 
                else 
                {
                    if ( dcache_cached ) 
                    {
                        m_cpt_data_miss++;
                        m_cost_data_miss_frz++;
                        r_dcache_miss_req = true;
                        r_dvcache_fsm = DVCACHE_MISS_WAIT;
                    } 
                    else 
                    {
                        m_cpt_unc_read++;
                        m_cost_unc_read_frz++;
                        r_dcache_unc_req = true;
                        r_dvcache_fsm = DVCACHE_UNC_WAIT;
                    }
                }
                break;
            case iss_t::XTN_READ:
            case iss_t::XTN_WRITE:
                // only DCACHE INVALIDATE request are supported
                //r_dcache_fsm = DCACHE_INVAL;
                //drsp.valid = true;
                break;
            case iss_t::DATA_WRITE:
                m_cpt_write++;
                if ( dvcache_hit_c && dcache_cached ) 
                {
                    r_dvcache_fsm = DVCACHE_WRITE_UPDT;
                    m_cpt_write_cached++;
                } 
                else 
                {
                    r_dvcache_fsm = DVCACHE_WRITE_REQ;
                }
                drsp.valid = true;
                drsp.rdata = 0;
                break;
        } // end switch dreq.type

        r_dcache_addr_save      = dpaddr;
        r_dcache_type_save      = dreq.type;
        r_dcache_wdata_save     = dreq.wdata;
        r_dcache_be_save        = dreq.be;
        r_dcache_rdata_save     = dcache_rdata;
        r_dcache_cached_save    = dcache_cached;
        break;
    }

    case DVCACHE_TLB1_WAIT:
    {
        if ( !r_dtlb_req && !r_dvcache_rsp_error ) 
        {
            bool d = (((r_dtlb_miss_rsp & PTE_D_MASK)>> PTE_D_SHIFT) == 1) ? true : false;
            // renew tlb save register
            r_dtlb_et_save = (r_dtlb_miss_rsp & PTE_ET_MASK ) >> PTE_ET_SHIFT;
            r_dtlb_ptpr_save = r_tlb_ptpr;
            r_dtlb_id1_save = dreq.addr>>PAGE_M_NBITS;

            switch((r_dtlb_miss_rsp & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTD:
            {
                addr36_t dtlb_ptp = (addr36_t)(r_dtlb_miss_rsp & PTD_PTP_MASK) << 12;
                r_dtlb_ptp = dtlb_ptp;
                r_dcache_addr_save = (addr36_t)(dtlb_ptp | (((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2));
                r_dtlb_pte_addr = (addr36_t)(dtlb_ptp | (((dreq.addr&PTD_ID2_MASK)>>PAGE_K_NBITS) << 2));
                r_dcache_type_save      = iss_t::DATA_READ;
                r_dcache_wdata_save     = 0xFFFFFFFF;
                r_dcache_be_save        = 0xF;
                r_dcache_rdata_save     = 0xFFFFFFFF;
                r_dcache_cached_save    = false;
                r_dtlb_req              = true;
                r_dvcache_fsm = DVCACHE_TLB2_WAIT;
                break;
            }
            case PTE_NEW:   // to set PTE accessed  
                r_dcache_addr_save = (addr36_t)(r_tlb_ptpr | ((dreq.addr>>PAGE_M_NBITS)<<2));
                r_dcache_type_save      = iss_t::DATA_LL;
                r_dcache_wdata_save     = 0xFFFFFFFF;
                r_dcache_be_save        = 0xF;
                r_dcache_rdata_save     = 0xFFFFFFFF;
                r_dcache_cached_save    = false;
                r_dtlb_ll_req           = true;
                r_dvcache_fsm = DVCACHE_T1_LL_WAIT;
                break;  
            case PTE_OLD:                
                if ( is_write(dreq.type) && !d ) 
                { 
                    r_dcache_addr_save = (addr36_t)(r_tlb_ptpr | ((dreq.addr>>PAGE_M_NBITS)<<2));
                    r_dcache_type_save      = iss_t::DATA_LL;
                    r_dcache_wdata_save     = 0xFFFFFFFF;
                    r_dcache_be_save        = 0xF;
                    r_dcache_rdata_save     = 0xFFFFFFFF;
                    r_dcache_cached_save    = false;
                    r_dtlb_ll_req           = true;
                    r_dvcache_fsm = DVCACHE_T1_LL_WAIT;
                } 
                else 
                { 
                    r_dvcache_fsm = DVCACHE_TLB1_UPDT;
                }
                break;
            default:    // unmapped
                r_dvcache_error_type = r_dvcache_error_type | MMU_PT1_UNMAPPED;  // Invalid address error     
                r_dvcache_bad_vaddr = dreq.addr;
                r_dvcache_fsm = DVCACHE_ERROR;
                break;
            }
        }

        if (!r_dtlb_req && r_dvcache_rsp_error) 
        {
            r_dvcache_fsm = DVCACHE_ERROR; 
            r_dvcache_error_type = r_dvcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    // Access bus error 
            r_dvcache_bad_vaddr = dreq.addr;
        }
        break;
    }

    case DVCACHE_T1_LL_WAIT:
        if (!r_dtlb_ll_req) 
        {
            if ( r_dvcache_rsp_error ) 
            {
                r_dvcache_fsm = DVCACHE_ERROR;  
                r_dvcache_error_type = r_dvcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    // Access bus error 
                r_dvcache_bad_vaddr = dreq.addr;
            } 
            else
            {
                r_dcache_addr_save = (addr36_t)(r_tlb_ptpr | ((dreq.addr>>PAGE_M_NBITS)<<2));
                r_dcache_type_save = iss_t::DATA_SC;
                if ( is_write(dreq.type) )
                {
                    r_dcache_wdata_save = r_dtlb_miss_rsp | PTE_ET_MASK | PTE_D_MASK;
                }
                else
                { 
                    r_dcache_wdata_save = r_dtlb_miss_rsp | PTE_ET_MASK;
                }
                r_dcache_be_save = 0xF;
                r_dcache_cached_save = false;
                r_dtlb_sc_req = true;
                r_dvcache_fsm = DVCACHE_T1_SC_WAIT; 
            }
        }
        break;

    case DVCACHE_T1_SC_WAIT:  
        if (!r_dtlb_sc_req) 
        {
            if ( r_dvcache_rsp_error ) 
            {
                r_dvcache_fsm = DVCACHE_ERROR;
                r_dvcache_error_type = r_dvcache_error_type | MMU_PT1_ILLEGAL_ACCESS;    // Access bus error 
                r_dvcache_bad_vaddr = dreq.addr;
            } 
            else if ( r_dtlb_ll_req ) 
            {
                r_dcache_addr_save = (addr36_t)(r_tlb_ptpr | ((dreq.addr>>PAGE_M_NBITS)<<2));
                r_dcache_type_save      = iss_t::DATA_LL;
                r_dcache_wdata_save     = 0xFFFFFFFF;
                r_dcache_be_save        = 0xF;
                r_dcache_rdata_save     = 0xFFFFFFFF;
                r_dcache_cached_save    = false;
                r_dvcache_fsm = DVCACHE_T1_LL_WAIT;
            } 
            else 
            {
                r_dvcache_fsm = DVCACHE_TLB1_UPDT;  // TLB update
            }
        } 
        break;

    case DVCACHE_TLB1_UPDT: // update data TLB
        dvcache_m_tlb.update(r_dtlb_miss_rsp,dreq.addr,is_write(dreq.type));
        r_dvcache_fsm = DVCACHE_IDLE;
        break;

    case DVCACHE_TLB2_WAIT:
    {
        if (!r_dtlb_req && !r_dvcache_rsp_error) 
        {
            bool d  = (((r_dtlb_miss_rsp & PTE_D_MASK)>> PTE_D_SHIFT) == 1) ? true : false;
            switch((r_dtlb_miss_rsp & PTE_ET_MASK ) >> PTE_ET_SHIFT) {
            case PTE_NEW:   // to set PTE accessed  
                r_dcache_addr_save   = r_dtlb_pte_addr;
                r_dcache_type_save   = iss_t::DATA_LL;
                r_dcache_wdata_save  = 0xFFFFFFFF;
                r_dcache_be_save     = 0xF;
                r_dcache_rdata_save  = 0xFFFFFFFF;
                r_dcache_cached_save = false;
                r_dtlb_ll_req        = true;
                r_dvcache_fsm = DVCACHE_T2_LL_WAIT;
                break;  
            case PTE_OLD:
                if ( is_write(dreq.type) && !d ) 
                { 
                    r_dcache_addr_save   = r_dtlb_pte_addr;
                    r_dcache_type_save   = iss_t::DATA_LL;
                    r_dcache_wdata_save  = 0xFFFFFFFF;
                    r_dcache_be_save     = 0xF;
                    r_dcache_rdata_save  = 0xFFFFFFFF;
                    r_dcache_cached_save = false;
                    r_dtlb_ll_req        = true;
                    r_dvcache_fsm = DVCACHE_T2_LL_WAIT;
                } 
                else 
                { 
                    r_dvcache_fsm = DVCACHE_TLB2_UPDT;
                }
                break;
            default:    
                r_dvcache_error_type = r_dvcache_error_type | MMU_PT2_UNMAPPED;  // Translation error
                r_dvcache_bad_vaddr = dreq.addr;
                r_dvcache_fsm = DVCACHE_ERROR;
                break;
            }
        }

        if (!r_dtlb_req && r_dvcache_rsp_error) 
        {
            r_dvcache_fsm = DVCACHE_ERROR;
            r_dvcache_error_type = r_dvcache_error_type | MMU_PT2_ILLEGAL_ACCESS;  // Access bus error
            r_dvcache_bad_vaddr = dreq.addr;
        }
        break;
    }

    case DVCACHE_T2_LL_WAIT:
        if (!r_dtlb_ll_req) 
        {
            if ( r_dvcache_rsp_error ) 
            {
                r_dvcache_fsm = DVCACHE_ERROR;  
                r_dvcache_error_type = r_dvcache_error_type | MMU_PT2_ILLEGAL_ACCESS;  // Access bus error
                r_dvcache_bad_vaddr = dreq.addr;
            } 
            else  
            {
                r_dcache_addr_save = r_dtlb_pte_addr;
                r_dcache_type_save = iss_t::DATA_SC;
                if ( is_write(dreq.type) )
                {
                    r_dcache_wdata_save = r_dtlb_miss_rsp | PTE_ET_MASK | PTE_D_MASK;
                }
                else
                { 
                    r_dcache_wdata_save = r_dtlb_miss_rsp | PTE_ET_MASK;
                }
                r_dcache_be_save = 0xF;
                r_dcache_cached_save = false;
                r_dtlb_sc_req = true;
                r_dvcache_fsm = DVCACHE_T2_SC_WAIT; 
            }
        }
        break;

    case DVCACHE_T2_SC_WAIT:  
        if (!r_dtlb_sc_req) 
        {
            if ( r_dvcache_rsp_error ) 
            {
                r_dvcache_fsm = DVCACHE_ERROR;
                r_dvcache_error_type = r_dvcache_error_type | MMU_PT2_ILLEGAL_ACCESS;  // Access bus error
                r_dvcache_bad_vaddr = dreq.addr;
            } 
            else if ( r_dtlb_ll_req ) 
            {
                r_dcache_addr_save   = r_dtlb_pte_addr;
                r_dcache_type_save   = iss_t::DATA_LL;
                r_dcache_wdata_save  = 0xFFFFFFFF;
                r_dcache_be_save     = 0xF;
                r_dcache_rdata_save  = 0xFFFFFFFF;
                r_dcache_cached_save = false;
                r_dvcache_fsm = DVCACHE_T2_LL_WAIT;
            } 
            else  
            {
                r_dvcache_fsm = DVCACHE_TLB2_UPDT;  // TLB update
            }
        } 
        break;

    case DVCACHE_TLB2_UPDT:   // update TLB
        dvcache_k_tlb.update(r_dtlb_miss_rsp,dreq.addr,is_write(dreq.type));
        r_dvcache_fsm = DVCACHE_IDLE;
        break;

    case DVCACHE_TLB_FLUSH:
    {
        bool all = (bool)!r_context_sw_dtlb;
        dvcache_m_tlb.flush(all);     // initiale 4M page TLB
        dvcache_k_tlb.flush(all);     // initiale 4K page TLB
        r_dvcache_fsm = DVCACHE_IDLE;
        if (r_context_sw_dtlb) 
        { 
            drsp.valid = true; 
        }
        r_context_sw_dtlb = false;
        break;
    }

    case DVCACHE_CACHE_FLUSH:
        r_dcache.reset();
        r_dvcache_fsm = DVCACHE_IDLE;
        drsp.valid = true;
        break;

    case DVCACHE_TLB_INVAL: // the page swap out 
    {
        addr_t invadr = dreq.wdata;
        bool dvcache_hit_t_m = dvcache_m_tlb.translate(invadr); 
        bool dvcache_hit_t_k = dvcache_k_tlb.translate(invadr); 
		if (dvcache_hit_t_m)
        {
            dvcache_m_tlb.inval(invadr);
        }
        else if (dvcache_hit_t_k)
        {
        	dvcache_k_tlb.inval(invadr);
        }
        r_dvcache_fsm = DVCACHE_IDLE;
        drsp.valid = true;
        break;
    }

    case DVCACHE_CACHE_INVAL1:
    {
        m_cpt_dcache_dir_read += m_dcache_ways;
        addr_t invadr = dreq.wdata;
        addr36_t dpaddr;
        bool dtlb_m_hit, dtlb_k_hit; 

        if ( r_tlb_mode == TLBS_ACTIVE || r_tlb_mode == ITLB_D_DTLB_A ) 
        {
            dtlb_m_hit = dvcache_m_tlb.translate(invadr, &dpaddr); 
            dtlb_k_hit = dvcache_k_tlb.translate(invadr, &dpaddr); 
        } 
        else 
        {
            dpaddr = invadr;                    // instruction physique address
            dtlb_m_hit = true; 
            dtlb_k_hit = true;
        }

        if ( dtlb_m_hit || dtlb_k_hit )
        {
            r_ivcache_fsm = IVCACHE_CACHE_INVAL2;
            r_dcache_addr_save = dpaddr;
        }
        else
        {
            r_ivcache_fsm = IVCACHE_IDLE;
            drsp.valid = true;
        }

        break;
    }

    case DVCACHE_CACHE_INVAL2:
        r_dcache.inval(r_dcache_addr_save);
        drsp.valid = true;
        r_dvcache_fsm = DVCACHE_IDLE;
        break;

    case DVCACHE_WRITE_UPDT:
    {
        m_cpt_dcache_data_write++;
        data_t mask = be_to_mask(r_dcache_be_save);
        data_t wdata = (mask & r_dcache_wdata_save) | (~mask & r_dcache_rdata_save);
        assert(r_dcache.write(r_dcache_addr_save, wdata) && "Write on miss ignores data");
        r_dvcache_fsm = DVCACHE_WRITE_REQ;
        break;
    }
 
    case DVCACHE_MISS_WAIT:
        if ( dreq.valid ) 
        {
            m_cost_data_miss_frz++;
        }

        if ( !r_dcache_miss_req ) 
        {
            if ( r_dvcache_rsp_error ) 
            {
                r_dvcache_fsm = DVCACHE_ERROR;
                r_dvcache_error_type = r_dvcache_error_type | MMU_CACHE_ILLEGAL_ACCESS;  // Access bus error
                r_dvcache_bad_vaddr = dreq.addr;
            } 
            else 
            {
                r_dvcache_fsm = DVCACHE_MISS_UPDT;  // TLB update
            } 
        } 
        break;

    case DVCACHE_MISS_UPDT:
    {
        addr36_t ad = r_dcache_addr_save;
        data_t* buf = r_dcache_miss_buf;
        addr36_t  victim_index = 0;
        if ( dreq.valid )
        { 
            m_cost_data_miss_frz++;
        }
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        r_dcache.update(ad, buf, &victim_index);
        r_dvcache_fsm = DVCACHE_IDLE;
        break;
    }
 
    case DVCACHE_UNC_WAIT:
        if ( dreq.valid )
        { 
            m_cost_unc_read_frz++;
        }
        if ( !r_dcache_unc_req ) 
        {
            if ( r_dvcache_rsp_error ) 
            {
                r_dvcache_fsm = DVCACHE_ERROR;
                r_dvcache_error_type = r_dvcache_error_type | MMU_CACHE_ILLEGAL_ACCESS;  // Access bus error
                r_dvcache_bad_vaddr = dreq.addr;
            } 
            else 
            {
                r_dvcache_fsm = DVCACHE_IDLE;  // TLB update
            } 
        }
        break;

    case DVCACHE_ERROR:
        r_dvcache_rsp_error = false;
        drsp.valid = true;
        drsp.error = true;
        drsp.rdata = 0;
        r_dvcache_fsm = DVCACHE_IDLE;
        break;
        
    } // end switch r_dvcache_fsm

    m_iss.setData( drsp );

#ifdef VCACHE_WRAPPER_DEBUG
std::cout << " Data Response: " << drsp << std::endl;
#endif

    /////////// execute one iss cycle /////////////////////////////////
    {
    uint32_t it = 0;
    for (size_t i=0; i<(size_t)iss_t::n_irq; i++) if(p_irq[i].read()) it |= (1<<i);
    m_iss.executeNCycles(1,it);
    }

    ////////////// number of frozen cycles //////////////////////////
    if ( (ireq.valid && !irsp.valid) || (dreq.valid && !drsp.valid) )
    {
        m_cpt_frz_cycles++;
    }

    ////////////////////////////////////////////////////////////////////////////
    // The VCI_CMD FSM controls the following ressources:
    // - r_vci_cmd_fsm
    // - r_vci_cmd_cpt
    // - r_vci_cmd_min
    // - r_vci_cmd_max
    //
    // This FSM handles requests from both the DVCACHE controler
    // (request registers) and the IVCACHE controler (request registers).
    // There is 11 VCI transaction types :
    // - INS_TLB_MISS
    // - INS_TLB_LL
    // - INS_TLB_SC
    // - INS_MISS
    // - INS_UNC_MISS
    // - DATA_TLB_MISS
    // - DATA_TLB_LL
    // - DATA_TLB_SC
    // - DATA_MISS
    // - DATA_UNC 
    // - DATA_WRITE
    // The IVCACHE requests have the highest priority.
    // There is at most one (CMD/RSP) VCI transaction, as both CMD_FSM and RSP_FSM
    // exit simultaneously the IDLE state.
    //////////////////////////////////////////////////////////////////////////////

    switch (r_vci_cmd_fsm.read()) {
    
    case CMD_IDLE:
        if (r_vci_rsp_fsm != RSP_IDLE)
            break;

        r_vci_cmd_cpt = 0;
        if (r_itlb_req.read())              // ITLB miss request 
        {            
            r_vci_cmd_fsm = CMD_ITLB_MISS;
            m_cpt_itlbmiss_transaction++; 
        } 
        else if (r_itlb_ll_req.read())      // ITLB Linked Load request
        {  
            r_vci_cmd_fsm = CMD_ITLB_LL;
            m_cpt_itlbmiss_transaction++; 
        } 
        else if (r_itlb_sc_req.read())      // ITLB store conditional request
        {  
            r_vci_cmd_fsm = CMD_ITLB_SC;
            m_cpt_itlbmiss_transaction++; 
        } 
        else if (r_icache_miss_req.read())   // ICACHE instruction miss request
        {    
            r_vci_cmd_fsm = CMD_INS_MISS;
            m_cpt_imiss_transaction++; 
        }
        else if (r_icache_unc_req.read())   // ICACHE instruction uncached miss request
        {    
            r_vci_cmd_fsm = CMD_INS_UNC;
            m_cpt_imiss_transaction++; 
        }  
        else if (r_dtlb_req.read())         // DTLB miss request
        {            
            r_vci_cmd_fsm = CMD_DTLB_MISS;
            m_cpt_dtlbmiss_transaction++; 
        } 
        else if (r_dtlb_ll_req.read())      // DTLB Linked Load request
        {  
            r_vci_cmd_fsm = CMD_DTLB_LL;
            m_cpt_dtlbmiss_transaction++; 
        } 
        else if (r_dtlb_sc_req.read())      // DTLB store conditional request
        {  
            r_vci_cmd_fsm = CMD_DTLB_SC;
            m_cpt_dtlbmiss_transaction++; 
        } 
        else if (r_dcache_write_req.read()) // DCACHE write request
        {
            r_vci_cmd_fsm = CMD_DATA_WRITE;
            r_vci_cmd_cpt = r_wbuf.getMin();
            r_vci_cmd_min = r_wbuf.getMin();
            r_vci_cmd_max = r_wbuf.getMax(); 
            m_cpt_write_transaction++; 
            m_length_write_transaction += (r_wbuf.getMax() - r_wbuf.getMin() + 1);
        }
        else if (r_dcache_miss_req.read())  // DCACHE read request
        {
            r_vci_cmd_fsm = CMD_DATA_MISS;
            m_cpt_dmiss_transaction++; 
        }
        else if (r_dcache_unc_req.read())   // DCACHE uncached read request
        {
            r_vci_cmd_fsm = CMD_DATA_UNC;
            m_cpt_unc_transaction++; 
        }
        break;

    case CMD_ITLB_MISS:     
    case CMD_ITLB_LL: 
    case CMD_ITLB_SC:    
    case CMD_INS_MISS: 
    case CMD_INS_UNC:
        if (p_vci.cmdack.read())
        {
            r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DTLB_MISS:     
    case CMD_DTLB_LL: 
    case CMD_DTLB_SC:     
    case CMD_DATA_UNC:
    case CMD_DATA_MISS:
        if ( p_vci.cmdack.read() )
        {
            r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci.cmdack.read() ) 
        {
            r_vci_cmd_cpt = r_vci_cmd_cpt + 1;
            if (r_vci_cmd_cpt == r_vci_cmd_max) 
            {
                r_vci_cmd_fsm = CMD_IDLE ;
                r_wbuf.reset() ;
            }
        }
        break;
    } // end  switch r_vci_cmd_fsm

    //////////////////////////////////////////////////////////////////////////
    // The VCI_RSP FSM controls the following ressources:
    // - r_vci_rsp_fsm:
    // - r_icache_miss_buf[icache_words]
    // - r_dcache_miss_buf[dcache_words]
    // - r_icache_buf_unc_valid set
    // - r_dcache_buf_unc_valid set
    // - r_itlb_req reset
    // - r_icache_miss_req reset
    // - r_icache_unc_req reset
    // - r_itlb_ll_req reset
    // - r_itlb_sc_req reset
    // - r_dtlb_req reset
    // - r_dcache_write_req reset
    // - r_dcache_miss_req reset
    // - r_dcache_unc_req reset
    // - r_dtlb_ll_req reset
    // - r_dtlb_sc_req reset
    // - r_vci_rsp_cpt
    // - r_itlb_miss_rsp set
    // - r_dtlb_miss_rsp set
    // - r_ivcache_rsp_error set
    // - r_dvcache_rsp_error set
    //
    // This FSM is synchronized with the VCI_CMD FSM, as both FSMs exit the
    // IDLE state simultaneously.
    //////////////////////////////////////////////////////////////////////////

    switch (r_vci_rsp_fsm.read()) {

    case RSP_IDLE:
        assert( ! p_vci.rspval.read() && "Unexpected response" );

        if (r_vci_cmd_fsm != CMD_IDLE)
            break;

        r_vci_rsp_cpt = 0;
        if (r_itlb_req.read())              // ITLB miss response
        {            
            r_vci_rsp_fsm = RSP_ITLB_MISS;
        } 
        else if (r_itlb_ll_req.read())      // ITLB linked load response
        {   
            r_vci_rsp_fsm = RSP_ITLB_LL;
        } 
        else if (r_itlb_sc_req.read())      // ITLB store conditional response
        {   
            r_vci_rsp_fsm = RSP_ITLB_SC;
        } 
        else if (r_icache_miss_req.read())   // ICACHE cached miss response
        {   
            r_vci_rsp_fsm = RSP_INS_MISS;
        }
        else if (r_icache_unc_req.read())   // ICACHE uncached miss response
        {   
            r_vci_rsp_fsm = RSP_INS_UNC;
        }  
        else if (r_dtlb_req.read())         // ITLB miss response
        {
            r_vci_rsp_fsm = RSP_DTLB_MISS; 
        }
        else if (r_dtlb_ll_req.read())      // ITLB linked load response
        {
            r_vci_rsp_fsm = RSP_DTLB_LL; 
        }
        else if (r_dtlb_sc_req.read())      // ITLB store conditional response
        {
            r_vci_rsp_fsm = RSP_DTLB_SC; 
        }
        else if (r_dcache_write_req.read()) // DCACHE write request
        {
            r_vci_rsp_fsm = RSP_DATA_WRITE;
        }
        else if (r_dcache_miss_req.read())  // DCACHE read request
        {
            r_vci_rsp_fsm = RSP_DATA_MISS;
        }
        else if (r_dcache_unc_req.read())   // DCACHE uncached read request
        {
            r_vci_rsp_fsm = RSP_DATA_UNC;
        }
        break;

    case RSP_ITLB_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( !p_vci.rerror.read() ) 
        {
            r_itlb_miss_rsp = (int)p_vci.rdata.read();
        } 
        else 
        {
            r_ivcache_rsp_error = true;
        }
        r_itlb_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_ITLB_LL:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( !p_vci.rerror.read() ) // linked load is successful
        {   
            r_itlb_miss_rsp = (int)p_vci.rdata.read();
        } 
        else 
        {
            r_ivcache_rsp_error = true;
        }
        r_itlb_ll_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_ITLB_SC:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci.rerror.read() ) 
        {
            r_ivcache_rsp_error = true;
        }
        //} else if ((int)p_vci.rdata.read() == 0) {  // store conditional is not successful 
        else if ((int)p_vci.rdata.read() == 1) // store conditional is not successful, this for debug ??? after we should modify it
        {  
            r_itlb_ll_req = true;
        }
        r_itlb_sc_req = false;
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
            r_ivcache_rsp_error = true;
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
            r_ivcache_rsp_error = true;
        }
        break;

    case RSP_DTLB_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( !p_vci.rerror.read() ) 
        {
            r_dtlb_miss_rsp = (int)p_vci.rdata.read();
        }
        else 
        {
            r_dvcache_rsp_error = true;
        }
        r_dtlb_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DTLB_LL:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( !p_vci.rerror.read() ) // linked load is successful
        {   
            r_dtlb_miss_rsp = (int)p_vci.rdata.read();
        } 
        else 
        {
            r_dvcache_rsp_error = true;
        }
        r_dtlb_ll_req = false;
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DTLB_SC:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci.rerror.read() ) 
        {
            r_dvcache_rsp_error = true;
        }
        //} else if ((int)p_vci.rdata.read() == 0) {  // store conditional is not successful
        else if ((int)p_vci.rdata.read() == 1) // store conditional is not successful, this for debug ??? after we should modify it
        {  
            r_dtlb_ll_req = true;
        }
        r_dtlb_sc_req = false;
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
            r_dvcache_rsp_error = true;
        }
        break;

    case RSP_DATA_WRITE:
        m_cost_dmiss_transaction++;
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

    case RSP_DATA_UNC:
        m_cost_dmiss_transaction++;
        if ( ! p_vci.rspval.read() ) 
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci.rerror.read() != vci_param::ERR_NORMAL ) 
        {
            r_dvcache_rsp_error = true;
        }
        else
        {
            r_dcache_miss_buf[0] = (data_t)p_vci.rdata.read();
            r_dcache_buf_unc_valid = true;
        }
        r_vci_rsp_fsm = RSP_IDLE;
        r_dcache_unc_req = false;
        break;

    } // end switch r_vci_rsp_fsm
}
//////////////////////////////////////////////////////////////////////////////////
//   genMoore method 
//   The Moore signals are p_vci 
//////////////////////////////////////////////////////////////////////////////////
tmpl(void)::genMoore()
{
    // VCI initiator response
    p_vci.rspack = true;

    // VCI initiator command
    addr36_t ivcache_req_addr = r_ivcache_miss_addr.read(); 
    addr36_t dvcache_req_addr = r_dcache_addr_save.read();

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

    case CMD_ITLB_MISS:     
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)ivcache_req_addr & ~0x3;
        p_vci.wdata   = 0;
        p_vci.be = 0xF;
        p_vci.plen = 4;
        p_vci.cmd = vci_param::CMD_READ;
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

    case CMD_ITLB_LL: 
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)ivcache_req_addr & ~0x3;
        p_vci.wdata   = 0;
        p_vci.be = 0xF;
        p_vci.plen = 4;
        p_vci.cmd = vci_param::CMD_LOCKED_READ;
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

    case CMD_ITLB_SC:   
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)ivcache_req_addr & ~0x3;   // address of PTE
        p_vci.wdata   = (data_t)r_ivcache_miss_data;   
        p_vci.be = 0xF;
        p_vci.plen = 4;
        p_vci.cmd = vci_param::CMD_STORE_COND;
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

    case CMD_INS_MISS:
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)(ivcache_req_addr & m_icache_yzmask);
        p_vci.wdata   = 0;
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
        p_vci.address = (addr36_t)ivcache_req_addr & ~0x3;
        p_vci.wdata   = 0;
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

    case CMD_DTLB_MISS:     
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)dvcache_req_addr & ~0x3;
        p_vci.wdata   = 0;
        p_vci.be = 0xF;
        p_vci.plen = 4;
        p_vci.cmd = vci_param::CMD_READ;
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

    case CMD_DTLB_LL:     
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)dvcache_req_addr & ~0x3;
        p_vci.wdata   = 0;
        p_vci.be = 0xF;
        p_vci.plen = 4;
        p_vci.cmd = vci_param::CMD_LOCKED_READ;
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

    case CMD_DTLB_SC:     
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)dvcache_req_addr & ~0x3;     // address of PTE
        p_vci.wdata   = r_dcache_wdata_save.read();     
        p_vci.be = 0xF;
        p_vci.plen = 4;
        p_vci.cmd = vci_param::CMD_STORE_COND;
        p_vci.trdid  = 0;
        p_vci.pktid  = 0;
        p_vci.srcid  = m_srcid;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = false;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop    = true;
        break;

    case CMD_DATA_UNC:
        p_vci.cmdval = true;
        p_vci.address = (addr36_t)dvcache_req_addr & ~0x3;
        switch(r_dcache_type_save) {
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
        p_vci.plen    = (r_vci_cmd_max - r_vci_cmd_min + 1)<<2;
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
        p_vci.address = (addr36_t)(dvcache_req_addr & m_dcache_yzmask);
        p_vci.wdata   = 0;
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


