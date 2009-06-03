/* -*- c++ -*-
 * File : vci_cc_vcache_wrapper2_ring.h
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
 
#ifndef SOCLIB_CABA_VCI_CC_VCACHE_WRAPPER2_RING_H
#define SOCLIB_CABA_VCI_CC_VCACHE_WRAPPER2_RING_H

#include <inttypes.h>
#include <systemc>
#include "caba_base_module.h"
#include "write_buffer.h"
#include "generic_cache.h"
#include "vci_initiator.h"
#include "vci_target.h"
#include "mapping_table.h"
#include "generic_tlb.h"
#include "static_assert.h"

namespace soclib {
namespace caba {

using namespace sc_core;

////////////////////////////////////////////
template<typename vci_param, typename iss_t>
class VciCcVCacheWrapper2Ring
////////////////////////////////////////////
    : public soclib::caba::BaseModule
{
    typedef uint32_t addr_t;
    typedef uint32_t data_t;
    typedef uint32_t tag_t;
    typedef uint32_t be_t;
    typedef uint32_t type_t;
    typedef typename iss_t::DataOperationType data_op_t;
    typedef sc_dt::sc_uint<36> addr36_t;

    enum icache_fsm_state_e {  
        ICACHE_IDLE,                // 00
        ICACHE_BIS,                 // 01
        ICACHE_TLB1_READ,           // 02
        ICACHE_TLB1_WRITE,          // 03
        ICACHE_TLB1_UPDT,           // 04
        ICACHE_TLB2_READ,           // 05
        ICACHE_TLB2_WRITE,          // 06
        ICACHE_TLB2_UPDT,           // 07
        ICACHE_SW_FLUSH1,           // 08
        ICACHE_SW_FLUSH2,           // 09
        ICACHE_CACHE_FLUSH,         // 0a
        ICACHE_TLB_INVAL,           // 0b
        ICACHE_TLB_INVAL_DONE,      // 0c
        ICACHE_CACHE_INVAL,         // 0d
        ICACHE_CACHE_INVAL_DONE,    // 0e
        ICACHE_MISS_WAIT,           // 0f
        ICACHE_UNC_WAIT,            // 10
        ICACHE_MISS_UPDT,           // 11
        ICACHE_ERROR,               // 12
        ICACHE_CC_INVAL,            // 13
        ICACHE_TLB_CC_INVAL,        // 14
        ICACHE_TLB_FLUSH,           // 15
    };

    enum dcache_fsm_state_e {  
        DCACHE_IDLE,                // 00
        DCACHE_BIS,                 // 01
        DCACHE_DTLB1_READ_CACHE,    // 02
        DCACHE_TLB1_READ,           // 03
        DCACHE_TLB1_READ_UPDT,      // 04
        DCACHE_TLB1_WRITE,          // 05
        DCACHE_TLB1_UPDT,           // 06
        DCACHE_DTLB2_READ_CACHE,    // 07
        DCACHE_TLB2_READ,           // 08
        DCACHE_TLB2_READ_UPDT,      // 09
        DCACHE_TLB2_WRITE,          // 0a
        DCACHE_TLB2_UPDT,           // 0b
        DCACHE_CTXT_SWITCH1,        // 0c
        DCACHE_CTXT_SWITCH2,        // 0d
        DCACHE_ICACHE_FLUSH,        // 0e
        DCACHE_DCACHE_FLUSH,        // 0f
        DCACHE_ITLB_INVAL,          // 10
        DCACHE_DTLB_INVAL,          // 11
        DCACHE_DTLB_INVAL_DONE,     // 12
        DCACHE_ICACHE_INVAL,        // 13
        DCACHE_DCACHE_INVAL,        // 14
        DCACHE_DCACHE_INVAL_DONE,   // 15
        DCACHE_WRITE_UPDT,          // 16
        DCACHE_WRITE_DIRTY,         // 17
        DCACHE_WRITE_REQ,           // 18
        DCACHE_MISS_WAIT,           // 19
        DCACHE_MISS_UPDT,           // 1a
        DCACHE_UNC_WAIT,            // 1b
        DCACHE_ERROR,               // 1c
        DCACHE_ITLB_READ,           // 1d
        DCACHE_ITLB_UPDT,           // 1e
        DCACHE_ITLB_ET_WRITE,       // 1f
        DCACHE_CC_CHECK,            // 20
        DCACHE_CC_INVAL,            // 21
        DCACHE_CC_UPDT,             // 22
        DCACHE_CC_NOP,              // 23
        DCACHE_TLB_CC_INVAL,        // 24
        DCACHE_ITLB_CLEANUP,        // 25
    };

    enum cmd_fsm_state_e {      
        CMD_IDLE,                   // 00
        CMD_ITLB_READ,              // 01
        CMD_ITLB_WRITE,             // 02
        CMD_INS_MISS,               // 03
        CMD_INS_UNC,                // 04
        CMD_DTLB_READ,              // 05
        CMD_DTLB_WRITE,             // 06
        CMD_DTLB_DIRTY,             // 07
        CMD_DATA_UNC,               // 08
        CMD_DATA_MISS,              // 09
        CMD_DATA_WRITE,             // 0a
    };

    enum rsp_fsm_state_e {       
        RSP_IDLE,                   // 00
        RSP_ITLB_READ,              // 01
        RSP_ITLB_WRITE,             // 02
        RSP_INS_MISS,               // 03
        RSP_INS_UNC,                // 04
        RSP_DTLB_READ,              // 05
        RSP_DTLB_WRITE,             // 06
        RSP_DTLB_DIRTY,             // 07
        RSP_DATA_MISS,              // 08
        RSP_DATA_UNC,               // 09
        RSP_DATA_WRITE,             // 0a
    };

    enum cmd_cleanup_fsm_state_e {      
        CMD_CLEANUP_INS_IDLE,       // 00
        CMD_CLEANUP_DATA_IDLE,      // 00
        CMD_CLEANUP_INS,            // 01
        CMD_CLEANUP_DATA,           // 02
    };

    enum rsp_cleanup_fsm_state_e {       
        RSP_CLEANUP_INS_IDLE,       // 00
        RSP_CLEANUP_DATA_IDLE,      // 00
        RSP_CLEANUP_INS,            // 01
        RSP_CLEANUP_DATA,           // 02
    };

    enum tgt_fsm_state_e {  
        TGT_IDLE,                   // 00
        TGT_UPDT_WORD,              // 01
        TGT_UPDT_DATA,              // 02
        TGT_REQ_BROADCAST,          // 03
        TGT_REQ_DCACHE,             // 04
        TGT_RSP_BROADCAST,          // 05
        TGT_RSP_DCACHE,             // 06
    };

    enum inval_itlb_fsm_state_e {
        INVAL_ITLB_IDLE,            // 00
        INVAL_ITLB_CHECK_FIRST,     // 01
        INVAL_ITLB_INVAL,           // 02
        INVAL_ITLB_CHECK,           // 03
        INVAL_ITLB_CLEAR,           // 04
    };

    enum inval_dtlb_fsm_state_e {
        INVAL_DTLB_IDLE,            // 00
        INVAL_DTLB_CHECK_FIRST,     // 01
        INVAL_DTLB_INVAL,           // 02
        INVAL_DTLB_CHECK,           // 03
        INVAL_DTLB_CLEAR,           // 04
    };

    // TLB Mode
    enum {
        TLBS_DEACTIVE,  // instruction TLB deactive and data TLB deactive
        ITLB_A_DTLB_D,  // instruction TLB   active and data TLB deactive
        ITLB_D_DTLB_A,  // instruction TLB deactive and data TLB   active
        TLBS_ACTIVE,    // instruction TLB   active and data TLB   active
    };

    // Error Type
    enum mmu_error_type_e {
        MMU_NONE                 = 0x0,   // None
        MMU_PT1_UNMAPPED         = 0x1,   // Page fault on Page Table 1           (non fatal)
        MMU_PT2_UNMAPPED         = 0x2,   // Page fault on Page Table 2           (non fatal)
        MMU_PRIVILEGE_VIOLATION  = 0x4,   // Protected access in user mode        (user error)
        MMU_WRITE_VIOLATION      = 0x8,   // write access to a non writable page  (user error)
        MMU_EXEC_VIOLATION       = 0x10,  // exec access to a non exec page       (user error)
        MMU_UNDEFINED_XTN        = 0x20,  // undefined external access address    (user error)
        MMU_PT1_ILLEGAL_ACCESS   = 0x40,  // Bus Error accessing Table 1          (kernel error)
        MMU_PT2_ILLEGAL_ACCESS   = 0x80,  // Bus Error accessing Table 2          (kernel error)
        MMU_CACHE_ILLEGAL_ACCESS = 0x100, // Bus Error in cache access            (kernel error)
    };

public:
    sc_in<bool>                             p_clk;
    sc_in<bool>                             p_resetn;
    sc_in<bool>                             p_irq[iss_t::n_irq];
    soclib::caba::VciInitiator<vci_param>   p_vci_ini_rw;
    soclib::caba::VciInitiator<vci_param>   p_vci_ini_c;
    soclib::caba::VciTarget<vci_param>      p_vci_tgt;

private:
    // STRUCTURAL PARAMETERS
    soclib::common::AddressDecodingTable<uint32_t, bool>    m_cacheability_table;
    const soclib::common::Segment                           m_segment;
    iss_t                                                   m_iss;   
    const uint32_t                                          m_srcid_rw;
    const uint32_t                                          m_srcid_c;

    const size_t  m_itlb_m_ways;
    const size_t  m_itlb_m_sets;
    const size_t  m_itlb_k_ways;
    const size_t  m_itlb_k_sets;

    const size_t  m_dtlb_m_ways;
    const size_t  m_dtlb_m_sets;
    const size_t  m_dtlb_k_ways;
    const size_t  m_dtlb_k_sets;

    const size_t  m_icache_ways;
    const size_t  m_icache_sets;
    const size_t  m_icache_yzmask;
    const size_t  m_icache_words;

    const size_t  m_dcache_ways;
    const size_t  m_dcache_sets;
    const size_t  m_dcache_yzmask;
    const size_t  m_dcache_words;

    // instruction and data vcache tlb instances 
    soclib::caba::GenericCcTlb<addr36_t>    icache_m_tlb;
    soclib::caba::GenericCcTlb<addr36_t>    icache_k_tlb;
    soclib::caba::GenericCcTlb<addr36_t>    dcache_m_tlb;
    soclib::caba::GenericCcTlb<addr36_t>    dcache_k_tlb;

    sc_signal<addr_t>       r_mmu_ptpr;             // page table pointer register
    sc_signal<int>          r_mmu_mode;             // tlb mode register

    // DCACHE FSM REGISTERS
    sc_signal<int>          r_dcache_fsm;               // state register
    sc_signal<addr36_t>     r_dcache_paddr_save;        // physical address
    sc_signal<data_t>       r_dcache_wdata_save;        // write data
    sc_signal<data_t>       r_dcache_rdata_save;        // read data
    sc_signal<type_t>       r_dcache_type_save;         // access type
    sc_signal<be_t>         r_dcache_be_save;           // byte enable
    sc_signal<bool>         r_dcache_cached_save;       // used by the write buffer
    sc_signal<addr36_t>     r_dcache_tlb_paddr;         // physical address of tlb miss
    sc_signal<bool>         r_dcache_dirty_save;        // used for TLB dirty bit update
    sc_signal<size_t>       r_dcache_tlb_set_save;      // used for TLB dirty bit update
    sc_signal<size_t>       r_dcache_tlb_way_save;      // used for TLB dirty bit update
    sc_signal<addr_t>       r_dcache_id1_save;          // used by the PT1 bypass
    sc_signal<addr36_t>     r_dcache_ptba_save;         // used by the PT1 bypass
    sc_signal<bool>         r_dcache_ptba_ok;           // used by the PT1 bypass
    sc_signal<data_t>       r_dcache_pte_update;        // used for page table update
    sc_signal<tag_t>        r_dcache_ppn_save;          // used for speculative cache access
    sc_signal<tag_t>        r_dcache_vpn_save;          // used for speculative cache access
    sc_signal<bool>         r_dcache_page_k_save;       // used for write dirty bit
    sc_signal<bool>         r_dtlb_translation_valid;   // used for speculative address
    sc_signal<bool>         r_dcache_buf_unc_valid;     // used for uncached read
    sc_signal<bool>         r_dcache_hit_p_save;        // used to save hit_p in case BIS

    sc_signal<data_t>       r_dcache_error_type;        // software visible register
    sc_signal<addr_t>       r_dcache_bad_vaddr;         // software visible register 

    sc_signal<bool>         r_dcache_miss_req;          // used for cached read miss
    sc_signal<bool>         r_dcache_unc_req;           // used for uncached read miss
    sc_signal<bool>         r_dcache_write_req;         // used for write 
    sc_signal<bool>         r_dcache_tlb_read_req;      // used for tlb ptba or pte read 
    sc_signal<bool>         r_dcache_tlb_et_req;        // used for tlb entry type update
    sc_signal<bool>         r_dcache_tlb_dirty_req;     // used for tlb dirty bit update 
    sc_signal<bool>         r_dcache_tlb_ptba_read;     // used for tlb ptba read when write dirty bit 
    sc_signal<bool>         r_dcache_xtn_req;           // used for xtn write for ICACHE

    bool                    *r_dcache_in_itlb;          // indicates some words of dcache line in ins TLB 
    bool                    *r_dcache_in_dtlb;          // indicates some words of dcache line in data TLB 

    // coherence registers
    sc_signal<int>          r_dcache_fsm_save;          // state save register
    sc_signal<size_t>       r_dcache_way;
    sc_signal<size_t>       r_dcache_set;
    sc_signal<bool>         r_dcache_cleanup_req;       // data cleanup request
    sc_signal<data_t>       r_dcache_cleanup_line;      // data cleanup NLINE
    sc_signal<bool>         r_dcache_inval_rsp;         // data cache invalidate

    // ICACHE FSM REGISTERS
    sc_signal<int>          r_icache_fsm;               // state register
    sc_signal<addr36_t>     r_icache_paddr_save;        // physical address
    sc_signal<addr_t>       r_icache_id1_save;          // used by the PT1 bypass
    sc_signal<addr36_t>     r_icache_ptba_save;         // used by the PT1 bypass
    sc_signal<bool>         r_icache_ptba_ok;           // used by the PT1 bypass
    sc_signal<data_t>       r_icache_pte_update;        // used for page table update
    sc_signal<tag_t>        r_icache_ppn_save;          // used for speculative cache access
    sc_signal<tag_t>        r_icache_vpn_save;          // used for speculative cache access
    sc_signal<bool>         r_icache_page_k_save;       // used for write dirty bit
    sc_signal<bool>         r_itlb_translation_valid;   // used for speculative physical address
    sc_signal<bool>         r_icache_buf_unc_valid;     // used for uncached read

    sc_signal<data_t>       r_icache_error_type;        // software visible registers
    sc_signal<addr_t>       r_icache_bad_vaddr;         // software visible registers

    sc_signal<bool>         r_icache_miss_req;          // used for cached read miss
    sc_signal<bool>         r_icache_unc_req;           // used for uncached read miss
    sc_signal<bool>         r_dcache_itlb_read_req;     // used for tlb ptba or pte read 
    sc_signal<bool>         r_dcache_itlb_et_req;       // used for tlb entry type update

    sc_signal<bool>	        r_icache_tlb_read_dcache_req;   // used for instruction tlb miss, request in data cache
    sc_signal<bool>	        r_icache_tlb_et_dcache_req;     // used for itlb update entry type bits via dcache
    sc_signal<bool>	        r_dcache_rsp_itlb_error;        // used for data cache rsp error when itlb miss
    sc_signal<data_t>	    r_dcache_rsp_itlb_miss;	        // used for dcache rsp data when itlb miss

    // coherence registers
    sc_signal<int>          r_icache_fsm_save;          // state save register
    sc_signal<size_t>       r_icache_way;
    sc_signal<size_t>       r_icache_set;
    sc_signal<bool>         r_icache_cleanup_req;       // ins cleanup request
    sc_signal<data_t>       r_icache_cleanup_line;      // ins cleanup NLINE
    sc_signal<bool>         r_icache_inval_rsp;         // ins cache invalidate

    // VCI_CMD FSM REGISTERS
    sc_signal<int>          r_vci_cmd_fsm;
    sc_signal<size_t>       r_vci_cmd_min;       
    sc_signal<size_t>       r_vci_cmd_max;       
    sc_signal<size_t>       r_vci_cmd_cpt;     

    // VCI_RSP FSM REGISTERS
    sc_signal<int>          r_vci_rsp_fsm;
    sc_signal<size_t>       r_vci_rsp_cpt;
    sc_signal<data_t>       r_vci_rsp_itlb_miss;
    sc_signal<data_t>       r_vci_rsp_dtlb_miss;
    sc_signal<bool>         r_vci_rsp_ins_error;
    sc_signal<bool>         r_vci_rsp_data_error;

    data_t                  *r_icache_miss_buf;    
    data_t                  *r_dcache_miss_buf;  

    // VCI CLEANUP FSM REGISTERS
    sc_signal<int>          r_vci_cmd_cleanup_fsm;
    sc_signal<int>          r_vci_rsp_cleanup_fsm;

    // VCI_TGT FSM REGISTERS
    data_t                  *r_tgt_buf;
    bool                    *r_tgt_val;

    sc_signal<int>          r_vci_tgt_fsm;
    sc_signal<addr36_t>     r_tgt_addr;
    sc_signal<size_t>       r_tgt_word;
    sc_signal<bool>         r_tgt_update;
    sc_signal<size_t>       r_tgt_srcid;
    sc_signal<size_t>       r_tgt_pktid;
    sc_signal<size_t>       r_tgt_trdid;
    sc_signal<size_t>       r_tgt_plen;
    sc_signal<bool>         r_tgt_req;
    sc_signal<bool>         r_tgt_icache_req;
    sc_signal<bool>         r_tgt_dcache_req;
    sc_signal<bool>         r_tgt_icache_rsp;
    sc_signal<bool>         r_tgt_dcache_rsp;

    // INVAL CHECK FSM
    sc_signal<int>          r_inval_itlb_fsm;          
    sc_signal<bool>         r_dcache_itlb_inval_req;
    sc_signal<data_t>       r_dcache_itlb_inval_line;
    sc_signal<bool>         r_itlb_cc_check_end;
    sc_signal<bool>         r_ccinval_k_itlb_req; 
    sc_signal<size_t>       r_ccinval_itlb_way; 
    sc_signal<size_t>       r_ccinval_itlb_set; 
    sc_signal<bool>         r_icache_inval_tlb_rsp;
    sc_signal<data_t>       r_icache_tlb_nline;

    sc_signal<int>          r_inval_dtlb_fsm;          
    sc_signal<bool>         r_dcache_dtlb_inval_req;
    sc_signal<data_t>       r_dcache_dtlb_inval_line;
    sc_signal<bool>         r_dtlb_cc_check_end;
    sc_signal<bool>         r_ccinval_k_dtlb_req; 
    sc_signal<size_t>       r_ccinval_dtlb_way; 
    sc_signal<size_t>       r_ccinval_dtlb_set; 
    sc_signal<bool>         r_dcache_inval_tlb_rsp;
    sc_signal<data_t>       r_dcache_tlb_nline;

    sc_signal<bool>         r_dcache_itlb_cleanup_req;
    sc_signal<data_t>       r_dcache_itlb_cleanup_line;

    sc_signal<bool>         r_dcache_dtlb_cleanup_req;
    sc_signal<data_t>       r_dcache_dtlb_cleanup_line;

    sc_signal<bool>         r_itlb_inval_req;
    sc_signal<bool>         r_dcache_cc_check;

    WriteBuffer<addr36_t>     r_wbuf;
    GenericCache<addr36_t>    r_icache;
    GenericCache<addr36_t>    r_dcache;

    // Activity counters
    uint32_t m_cpt_dcache_data_read;        // DCACHE DATA READ
    uint32_t m_cpt_dcache_data_write;       // DCACHE DATA WRITE
    uint32_t m_cpt_dcache_dir_read;         // DCACHE DIR READ
    uint32_t m_cpt_dcache_dir_write;        // DCACHE DIR WRITE

    uint32_t m_cpt_icache_data_read;        // ICACHE DATA READ
    uint32_t m_cpt_icache_data_write;       // ICACHE DATA WRITE
    uint32_t m_cpt_icache_dir_read;         // ICACHE DIR READ
    uint32_t m_cpt_icache_dir_write;        // ICACHE DIR WRITE

    uint32_t m_cpt_frz_cycles;	            // number of cycles where the cpu is frozen
    uint32_t m_cpt_total_cycles;	        // total number of cycles 

    // Cache activity counters
    uint32_t m_cpt_read;                    // total number of read data
    uint32_t m_cpt_write;                   // total number of write data
    uint32_t m_cpt_data_miss;               // number of read miss
    uint32_t m_cpt_ins_miss;                // number of instruction miss
    uint32_t m_cpt_unc_read;                // number of read uncached
    uint32_t m_cpt_write_cached;            // number of cached write
    uint32_t m_cpt_ins_read;                // number of instruction read

    uint32_t m_cost_write_frz;              // number of frozen cycles related to write buffer         
    uint32_t m_cost_data_miss_frz;          // number of frozen cycles related to data miss
    uint32_t m_cost_unc_read_frz;           // number of frozen cycles related to uncached read
    uint32_t m_cost_ins_miss_frz;           // number of frozen cycles related to ins miss

    uint32_t m_cpt_imiss_transaction;       // number of VCI instruction miss transactions
    uint32_t m_cpt_dmiss_transaction;       // number of VCI data miss transactions
    uint32_t m_cpt_unc_transaction;         // number of VCI uncached read transactions
    uint32_t m_cpt_write_transaction;       // number of VCI write transactions

    uint32_t m_cost_imiss_transaction;      // cumulated duration for VCI IMISS transactions
    uint32_t m_cost_dmiss_transaction;      // cumulated duration for VCI DMISS transactions
    uint32_t m_cost_unc_transaction;        // cumulated duration for VCI UNC transactions
    uint32_t m_cost_write_transaction;      // cumulated duration for VCI WRITE transactions
    uint32_t m_length_write_transaction;    // cumulated length for VCI WRITE transactions

    // TLB activity counters
    uint32_t m_cpt_ins_tlb_read;            // number of instruction tlb read
    uint32_t m_cpt_ins_tlb_miss;            // number of instruction tlb miss
    uint32_t m_cpt_ins_tlb_write_et;        // number of instruction tlb write ET

    uint32_t m_cpt_data_tlb_read;           // number of data tlb read
    uint32_t m_cpt_data_tlb_miss;           // number of data tlb miss
    uint32_t m_cpt_data_tlb_write_et;       // number of data tlb write ET
    uint32_t m_cpt_data_tlb_write_dirty;    // number of data tlb write dirty
    
    uint32_t m_cost_ins_tlb_miss_frz;       // number of frozen cycles related to instruction tlb miss
    uint32_t m_cost_data_tlb_miss_frz;      // number of frozen cycles related to data tlb miss

    uint32_t m_cost_ins_waste_wait_frz;     // number of frozen cycles related to ins wait coherence operate 
    uint32_t m_cost_ins_tlb_sw_frz;         // number of frozen cycles related to ins context switch
    uint32_t m_cost_ins_cache_flush_frz;    // number of frozen cycles related to ins cache flush 

    uint32_t m_cpt_ins_tlb_cleanup;         // number of ins tlb cleanup 
    uint32_t m_cost_data_waste_wait_frz;    // number of frozen cycles related to data wait coherence operate
    uint32_t m_cost_data_tlb_sw_frz;        // number of frozen cycles related to data context switch
    uint32_t m_cost_data_cache_flush_frz;   // number of frozen cycles related to data cache flush

    uint32_t m_cpt_itlbmiss_transaction;    // number of itlb miss transactions
    uint32_t m_cpt_itlb_write_transaction;  // number of itlb write ET transactions
    uint32_t m_cpt_dtlbmiss_transaction;    // number of dtlb miss transactions
    uint32_t m_cpt_dtlb_write_transaction;  // number of dtlb write ET and dirty transactions

    uint32_t m_cost_itlbmiss_transaction;   // cumulated duration for VCI instruction TLB miss transactions
    uint32_t m_cost_itlb_write_transaction; // cumulated duration for VCI instruction TLB write ET transactions
    uint32_t m_cost_dtlbmiss_transaction;   // cumulated duration for VCI data TLB miss transactions
    uint32_t m_cost_dtlb_write_transaction; // cumulated duration for VCI data TLB write transactions

    uint32_t m_cpt_cc_update;               // number of coherence update packets 
    uint32_t m_cpt_cc_inval;                // number of coherence inval packets
    uint32_t m_cpt_cc_broadcast;            // number of coherence broadcast packets

    uint32_t m_cost_ins_tlb_inval_frz;      // number of frozen cycles related to checking ins tlb invalidate
    uint32_t m_cpt_ins_tlb_inval;           // number of ins tlb invalidate

    uint32_t m_cost_data_tlb_inval_frz;     // number of frozen cycles related to checking data tlb invalidate    
    uint32_t m_cpt_data_tlb_inval;          // number of data tlb invalidate

protected:
    SC_HAS_PROCESS(VciCcVCacheWrapper2Ring);

public:
    VciCcVCacheWrapper2Ring(
        sc_module_name insname,
        int proc_id,
        const soclib::common::MappingTable &mtp,
        const soclib::common::MappingTable &mtc,
        const soclib::common::IntTab &initiator_index_rw,
        const soclib::common::IntTab &initiator_index_c,
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
        size_t dcache_words);

    ~VciCcVCacheWrapper2Ring();

    void print_cpi();
    void print_stats();

private:
    void transition();
    void genMoore();

//    static_assert((int)iss_t::SC_ATOMIC == (int)vci_param::STORE_COND_ATOMIC);
//    static_assert((int)iss_t::SC_NOT_ATOMIC == (int)vci_param::STORE_COND_NOT_ATOMIC);
};

}}

#endif /* SOCLIB_CABA_VCI_CC_VCACHE_WRAPPER2_RING_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4


