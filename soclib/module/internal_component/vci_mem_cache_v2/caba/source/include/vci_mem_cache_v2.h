/* -*- c++ -*-
 * File         : vci_mem_cache_v2.h
 * Date         : 26/10/2008
 * Copyright    : UPMC / LIP6
 * Authors      : Alain Greiner / Eric Guthmuller
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
 * Maintainers: alain eric.guthmuller@polytechnique.edu
 */
/*
 *
 * Modifications done by Christophe Choichillon on the 7/04/2009:
 * - Adding new states in the CLEANUP FSM : CLEANUP_UPT_LOCK and CLEANUP_UPT_WRITE
 * - Adding a new VCI target port for the CLEANUP network
 * - Adding new state in the ALLOC_UPT_FSM : ALLOC_UPT_CLEANUP
 * 
 * Modifications to do :
 * - Adding new variables used by the CLEANUP FSM
 *
 */

#ifndef SOCLIB_CABA_MEM_CACHE_V2_H
#define SOCLIB_CABA_MEM_CACHE_V2_H

#include <inttypes.h>
#include <systemc>
#include <list>
#include <cassert>
#include "arithmetics.h"
#include "alloc_elems.h"
#include "caba_base_module.h"
#include "vci_target.h"
#include "vci_initiator.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "int_tab.h"
#include "mem_cache_directory_v2.h"
#include "xram_transaction_v2.h"
#include "update_tab_v2.h"
#include "atomic_tab_v2.h"

#define TRANSACTION_TAB_LINES 4     // Number of lines in the transaction tab
#define UPDATE_TAB_LINES 4          // Number of lines in the update tab
#define BROADCAST_ADDR 0x0000000003 // Address to send the broadcast invalidate

namespace soclib {  namespace caba {
  using namespace sc_core;

  template<typename vci_param>
    class VciMemCacheV2
    : public soclib::caba::BaseModule
    {
      typedef sc_dt::sc_uint<40> addr_t;
      typedef typename vci_param::fast_addr_t vci_addr_t;
      typedef uint32_t data_t;
      typedef uint32_t tag_t;
      typedef uint32_t size_t;
      typedef uint32_t be_t;
      typedef uint32_t copy_t;

      /* States of the TGT_CMD fsm */
      enum tgt_cmd_fsm_state_e{
        TGT_CMD_IDLE,
        TGT_CMD_READ,
        TGT_CMD_READ_EOP,
        TGT_CMD_WRITE,
        TGT_CMD_ATOMIC,
      };

      /* States of the TGT_RSP fsm */
      enum tgt_rsp_fsm_state_e{
        TGT_RSP_READ_IDLE,
        TGT_RSP_WRITE_IDLE,
        TGT_RSP_LLSC_IDLE,
        TGT_RSP_XRAM_IDLE,
        TGT_RSP_INIT_IDLE,
        TGT_RSP_CLEANUP_IDLE,
        TGT_RSP_READ,
        TGT_RSP_WRITE,
        TGT_RSP_LLSC,
        TGT_RSP_XRAM,
        TGT_RSP_INIT,
        TGT_RSP_CLEANUP,
      };

      /* States of the INIT_CMD fsm */
      enum init_cmd_fsm_state_e{
        INIT_CMD_INVAL_IDLE,
        INIT_CMD_INVAL_SEL,
        INIT_CMD_INVAL_NLINE,
        INIT_CMD_UPDT_IDLE,
        INIT_CMD_UPDT_SEL,
        INIT_CMD_BRDCAST,
        INIT_CMD_UPDT_NLINE,
        INIT_CMD_UPDT_INDEX,
        INIT_CMD_UPDT_DATA,
        INIT_CMD_SC_UPDT_IDLE,
        INIT_CMD_SC_UPDT_SEL,
        INIT_CMD_SC_BRDCAST,
        INIT_CMD_SC_UPDT_NLINE,
        INIT_CMD_SC_UPDT_INDEX,
        INIT_CMD_SC_UPDT_DATA,
      };

      /* States of the INIT_RSP fsm */
      enum init_rsp_fsm_state_e{
        INIT_RSP_IDLE,
        INIT_RSP_UPT_LOCK,
        INIT_RSP_UPT_CLEAR,
        INIT_RSP_END,
      };

      /* States of the READ fsm */
      enum read_fsm_state_e{
        READ_IDLE,
        READ_DIR_LOCK,
        READ_DIR_HIT,
        READ_RSP,
        READ_TRT_LOCK,
        READ_TRT_SET,
        READ_XRAM_REQ,
      };

      /* States of the WRITE fsm */
      enum write_fsm_state_e{
        WRITE_IDLE,
        WRITE_NEXT,
        WRITE_DIR_LOCK,
        WRITE_DIR_HIT_READ,
        WRITE_DIR_HIT,
        WRITE_UPT_LOCK,
        WRITE_WAIT_UPT,
        WRITE_UPDATE,
        WRITE_RSP,
        WRITE_TRT_LOCK,
        WRITE_TRT_DATA,
        WRITE_TRT_SET,
        WRITE_WAIT_TRT,
        WRITE_XRAM_REQ,
        WRITE_TRT_WRITE_LOCK,
        WRITE_INVAL_LOCK,
        WRITE_DIR_INVAL,
        WRITE_INVAL,
        WRITE_XRAM_SEND,
      };

      /* States of the IXR_RSP fsm */
      enum ixr_rsp_fsm_state_e{
        IXR_RSP_IDLE,
        IXR_RSP_ACK,
        IXR_RSP_TRT_ERASE,
        IXR_RSP_TRT_READ,
      };

      /* States of the XRAM_RSP fsm */
      enum xram_rsp_fsm_state_e{
        XRAM_RSP_IDLE,
        XRAM_RSP_TRT_COPY,
        XRAM_RSP_TRT_DIRTY,
        XRAM_RSP_DIR_LOCK,
        XRAM_RSP_DIR_UPDT,
        XRAM_RSP_DIR_RSP,
        XRAM_RSP_INVAL_LOCK,
        XRAM_RSP_INVAL_WAIT,
        XRAM_RSP_INVAL,
        XRAM_RSP_WRITE_DIRTY,
      };

      /* States of the IXR_CMD fsm */
      enum ixr_cmd_fsm_state_e{
        IXR_CMD_READ_IDLE,
        IXR_CMD_WRITE_IDLE,
        IXR_CMD_LLSC_IDLE,
        IXR_CMD_XRAM_IDLE,
        IXR_CMD_READ_NLINE,
        IXR_CMD_WRITE_NLINE,
        IXR_CMD_LLSC_NLINE,
        IXR_CMD_XRAM_DATA,
      };

      /* States of the LLSC fsm */
      enum llsc_fsm_state_e{
        LLSC_IDLE,
        LL_DIR_LOCK,
        LL_DIR_HIT,
        LL_RSP,
        SC_DIR_LOCK,
        SC_DIR_HIT,
        SC_UPT_LOCK,
        SC_WAIT_UPT,
        SC_UPDATE,
        SC_TRT_LOCK,
        SC_INVAL_LOCK,
        SC_DIR_INVAL,
        SC_INVAL,
        SC_XRAM_SEND,
        SC_RSP_FALSE,
        SC_RSP_TRUE,
        LLSC_TRT_LOCK,
        LLSC_TRT_SET,
        LLSC_XRAM_REQ,
      };

      /* States of the CLEANUP fsm */
      enum cleanup_fsm_state_e{
        CLEANUP_IDLE,
        CLEANUP_DIR_LOCK,
        CLEANUP_DIR_WRITE,
        CLEANUP_UPT_LOCK,
        CLEANUP_UPT_WRITE,
        CLEANUP_WRITE_RSP,
        CLEANUP_RSP,
      };

      /* States of the ALLOC_DIR fsm */
      enum alloc_dir_fsm_state_e{
        ALLOC_DIR_READ,
        ALLOC_DIR_WRITE,
        ALLOC_DIR_LLSC,
        ALLOC_DIR_CLEANUP,
        ALLOC_DIR_XRAM_RSP,
      };

      /* States of the ALLOC_TRT fsm */
      enum alloc_trt_fsm_state_e{
        ALLOC_TRT_READ,
        ALLOC_TRT_WRITE,
        ALLOC_TRT_LLSC,
        ALLOC_TRT_XRAM_RSP,
        ALLOC_TRT_IXR_RSP,
      };

      /* States of the ALLOC_UPT fsm */
      enum alloc_upt_fsm_state_e{
        ALLOC_UPT_WRITE,
        ALLOC_UPT_XRAM_RSP,
        ALLOC_UPT_INIT_RSP,
        ALLOC_UPT_CLEANUP,
        ALLOC_UPT_LLSC,
      };

      uint32_t	   m_cpt_cycles;            // Counter of cycles 
      uint32_t	   m_cpt_read;              // Number of READ transactions
      uint32_t	   m_cpt_read_miss;         // Number of MISS READ 
      uint32_t	   m_cpt_write;             // Number of WRITE transactions
      uint32_t	   m_cpt_write_miss;        // Number of MISS WRITE
      uint32_t     m_cpt_write_cells;	    // Cumulated length for WRITE transactions
      uint32_t     m_cpt_write_dirty;	    // Cumulated length for WRITE transactions
      uint32_t	   m_cpt_update;            // Number of UPDATE transactions
      uint32_t	   m_cpt_update_mult;       // Number of targets for UPDATE
      uint32_t	   m_cpt_inval;             // Number of INVAL  transactions
      uint32_t	   m_cpt_inval_mult;        // Number of targets for INVAL  
      uint32_t	   m_cpt_inval_brdcast;     // Number of BROADCAST INVAL  
      uint32_t	   m_cpt_cleanup;           // Number of CLEANUP transactions
      uint32_t	   m_cpt_ll;                // Number of LL transactions
      uint32_t	   m_cpt_sc;                // Number of SC transactions

      protected:

      SC_HAS_PROCESS(VciMemCacheV2);

      public:
      sc_in<bool> 			  	p_clk;
      sc_in<bool> 			  	p_resetn;
      soclib::caba::VciTarget<vci_param>    	p_vci_tgt;
      soclib::caba::VciTarget<vci_param>    	p_vci_tgt_cleanup;
      soclib::caba::VciInitiator<vci_param> 	p_vci_ini;	
      soclib::caba::VciInitiator<vci_param> 	p_vci_ixr;

      VciMemCacheV2(
          sc_module_name name,                              // Instance Name 
          const soclib::common::MappingTable &mtp,          // Mapping table for primary requets
          const soclib::common::MappingTable &mtc,          // Mapping table for coherence requets
          const soclib::common::MappingTable &mtx,          // Mapping table for XRAM
          const soclib::common::IntTab &vci_ixr_index,      // VCI port to XRAM (initiator)
          const soclib::common::IntTab &vci_ini_index,      // VCI port to PROC (initiator)
          const soclib::common::IntTab &vci_tgt_index,      // VCI port to PROC (target)
          const soclib::common::IntTab &vci_tgt_index_cleanup,    // VCI port to PROC (target) for cleanup
          size_t nways,                                   // Number of ways per set 
          size_t nsets,                                   // Number of sets
          size_t nwords);                                 // Number of words per line

      ~VciMemCacheV2();

      void transition();

      void genMoore();

      void print_stats();

      private:

      // Component attributes
      const size_t              m_initiators;           // Number of initiators
      const size_t              m_ways;                 // Number of ways in a set
      const size_t              m_sets;                 // Number of cache sets
      const size_t              m_words;		        // Number of words in a line
      const size_t              m_srcid_ixr;		    // Srcid for requests to XRAM 
      const size_t              m_srcid_ini;		    // Srcid for requests to processors
      std::list<soclib::common::Segment>  m_seglist;    // memory cached into the cache
      std::list<soclib::common::Segment>  m_cseglist;   // coherence segment for the cache
      vci_addr_t        		*m_coherence_table; 	// address(srcid)
      AtomicTab                 m_atomic_tab;           // atomic access table
      TransactionTab      		m_transaction_tab;	    // xram transaction table
      UpdateTab                 m_update_tab;		    // pending update & invalidate 
      CacheDirectory			m_cache_directory;	    // data cache directory

      data_t	        	       ***m_cache_data;		// data array[set][way][word]

      // adress masks
      const soclib::common::AddressMaskingTable<vci_addr_t>   m_x;
      const soclib::common::AddressMaskingTable<vci_addr_t>   m_y;
      const soclib::common::AddressMaskingTable<vci_addr_t>   m_z;
      const soclib::common::AddressMaskingTable<vci_addr_t>   m_nline; 

      //////////////////////////////////////////////////
      // Others registers
      //////////////////////////////////////////////////
      sc_signal<size_t>   r_copies_limit; // Limit of the number of copies for one line

      //////////////////////////////////////////////////
      // Registers controlled by the TGT_CMD fsm
      //////////////////////////////////////////////////

      // Fifo between TGT_CMD fsm and READ fsm
      GenericFifo<uint64_t>  m_cmd_read_addr_fifo;
      GenericFifo<size_t>    m_cmd_read_length_fifo;
      GenericFifo<size_t>    m_cmd_read_srcid_fifo;
      GenericFifo<size_t>    m_cmd_read_trdid_fifo;
      GenericFifo<size_t>    m_cmd_read_pktid_fifo;

      // Fifo between TGT_CMD fsm and WRITE fsm    
      GenericFifo<uint64_t>  m_cmd_write_addr_fifo;
      GenericFifo<bool>      m_cmd_write_eop_fifo;
      GenericFifo<size_t>    m_cmd_write_srcid_fifo;
      GenericFifo<size_t>    m_cmd_write_trdid_fifo;
      GenericFifo<size_t>    m_cmd_write_pktid_fifo;
      GenericFifo<data_t>    m_cmd_write_data_fifo;
      GenericFifo<be_t>	     m_cmd_write_be_fifo;

      // Fifo between TGT_CMD fsm and LLSC fsm
      GenericFifo<uint64_t>  m_cmd_llsc_addr_fifo;
      GenericFifo<bool>      m_cmd_llsc_sc_fifo;
      GenericFifo<size_t>    m_cmd_llsc_srcid_fifo;
      GenericFifo<size_t>    m_cmd_llsc_trdid_fifo;
      GenericFifo<size_t>    m_cmd_llsc_pktid_fifo;
      GenericFifo<data_t>    m_cmd_llsc_wdata_fifo;

      sc_signal<int>         r_tgt_cmd_fsm;

      sc_signal<size_t>	     r_index;
      size_t nseg;
      size_t ncseg;
      soclib::common::Segment  **m_seg;
      soclib::common::Segment  **m_cseg;
      ///////////////////////////////////////////////////////
      // Registers controlled by the READ fsm
      ///////////////////////////////////////////////////////

      sc_signal<int>         r_read_fsm;        // FSM state 
      sc_signal<copy_t>      r_read_d_copies;   // bit-vector of copies 
      sc_signal<copy_t>      r_read_i_copies;   // bit-vector of copies 
      sc_signal<tag_t>       r_read_tag;	    // cache line tag (in directory)
      sc_signal<bool>        r_read_is_cnt;	    // is_cnt bit (in directory)
      sc_signal<bool>        r_read_lock;	    // lock bit (in directory)
      sc_signal<bool>        r_read_dirty;	    // dirty bit (in directory)
      sc_signal<size_t>      r_read_count;      // number of copies
      sc_signal<data_t>     *r_read_data;       // data (one cache line)
      sc_signal<size_t>      r_read_way;        // associative way (in cache)
      sc_signal<size_t>      r_read_trt_index;  // Transaction Table index

      // Buffer between READ fsm and IXR_CMD fsm (ask a missing cache line to XRAM)   
      sc_signal<bool>        r_read_to_ixr_cmd_req;     // valid request
      sc_signal<addr_t>      r_read_to_ixr_cmd_nline;   // cache line index
      sc_signal<size_t>      r_read_to_ixr_cmd_trdid;   // index in Transaction Table

      // Buffer between READ fsm and TGT_RSP fsm (send a hit read response to L1 cache)
      sc_signal<bool>	   r_read_to_tgt_rsp_req;       // valid request
      sc_signal<size_t>	   r_read_to_tgt_rsp_srcid;	    // Transaction srcid
      sc_signal<size_t>	   r_read_to_tgt_rsp_trdid;	    // Transaction trdid
      sc_signal<size_t>	   r_read_to_tgt_rsp_pktid;	    // Transaction pktid
      sc_signal<data_t>   *r_read_to_tgt_rsp_data;	    // data (one cache line)
      sc_signal<size_t>    r_read_to_tgt_rsp_word;      // first word of the response
      sc_signal<size_t>    r_read_to_tgt_rsp_length;    // length of the response

      ///////////////////////////////////////////////////////////////
      // Registers controlled by the WRITE fsm
      ///////////////////////////////////////////////////////////////

      sc_signal<int>       r_write_fsm;             // FSM state
      sc_signal<addr_t>	   r_write_address;         // first word address
      sc_signal<size_t>    r_write_word_index;      // first word index in line
      sc_signal<size_t>    r_write_word_count;      // number of words in line
      sc_signal<size_t>    r_write_srcid;           // transaction srcid
      sc_signal<size_t>    r_write_trdid;           // transaction trdid
      sc_signal<size_t>    r_write_pktid;           // transaction pktid
      sc_signal<data_t>	  *r_write_data;            // data (one cache line)	
      sc_signal<be_t>     *r_write_be;              // one byte enable per word
      sc_signal<bool>      r_write_byte;            // is it a byte write
      sc_signal<bool>      r_write_is_cnt;          // is_cnt bit (in directory)
      sc_signal<bool>      r_write_lock;            // lock bit (in directory)
      sc_signal<tag_t>     r_write_tag;             // cache line tag (in directory)
      sc_signal<copy_t>    r_write_d_copies;        // bit vector of copies
      sc_signal<copy_t>    r_write_i_copies;        // bit vector of copies
      sc_signal<size_t>	   r_write_count;           // number of copies
      sc_signal<size_t>	   r_write_way;		        // way of the line
      sc_signal<size_t>    r_write_trt_index;	    // index in Transaction Table
      sc_signal<size_t>    r_write_upt_index;	    // index in Update Table

      // Buffer between WRITE fsm and TGT_RSP fsm (acknowledge a write command from L1)
      sc_signal<bool>      r_write_to_tgt_rsp_req;		// valid request
      sc_signal<size_t>    r_write_to_tgt_rsp_srcid;    // transaction srcid
      sc_signal<size_t>    r_write_to_tgt_rsp_trdid;	// transaction trdid
      sc_signal<size_t>    r_write_to_tgt_rsp_pktid;	// transaction pktid

      // Buffer between WRITE fsm and IXR_CMD fsm (ask a missing cache line to XRAM) 
      sc_signal<bool>	   r_write_to_ixr_cmd_req;      // valid request
      sc_signal<bool>	   r_write_to_ixr_cmd_write;    // write request
      sc_signal<addr_t>    r_write_to_ixr_cmd_nline; 	// cache line index
      sc_signal<data_t>	  *r_write_to_ixr_cmd_data;	    // cache line data
      sc_signal<size_t>    r_write_to_ixr_cmd_trdid;	// index in Transaction Table

      // Buffer between WRITE fsm and INIT_CMD fsm (Update/Invalidate L1 caches)
      sc_signal<bool>	   r_write_to_init_cmd_req;         // valid request
      sc_signal<bool>	   r_write_to_init_cmd_brdcast;     // brdcast request
      sc_signal<addr_t>	   r_write_to_init_cmd_nline;	    // cache line index
      sc_signal<size_t>	   r_write_to_init_cmd_trdid;	    // index in Update Table
      sc_signal<copy_t>    r_write_to_init_cmd_d_copies;    // bit_vector of L1 to update
      sc_signal<data_t>   *r_write_to_init_cmd_data;	    // data (one cache line)
      sc_signal<bool>     *r_write_to_init_cmd_we;	        // word enable
      sc_signal<size_t>	   r_write_to_init_cmd_count;	    // number of words in line
      sc_signal<size_t>	   r_write_to_init_cmd_index;	    // index of first word in line

      /////////////////////////////////////////////////////////
      // Registers controlled by INIT_RSP fsm
      //////////////////////////////////////////////////////////

      sc_signal<int> 	   r_init_rsp_fsm;        // FSM state
      sc_signal<size_t>	   r_init_rsp_upt_index;  // index in the Update Table
      sc_signal<size_t>	   r_init_rsp_srcid;	  // pending write srcid      
      sc_signal<size_t>	   r_init_rsp_trdid;	  // pending write trdid      
      sc_signal<size_t>	   r_init_rsp_pktid;	  // pending write pktid      
      sc_signal<addr_t>	   r_init_rsp_nline;	  // pending write nline      

      // Buffer between INIT_RSP fsm and TGT_RSP fsm (complete write/update transaction)
      sc_signal<bool>	     r_init_rsp_to_tgt_rsp_req;   	// valid request
      sc_signal<size_t>	   r_init_rsp_to_tgt_rsp_srcid;		// Transaction srcid
      sc_signal<size_t>	   r_init_rsp_to_tgt_rsp_trdid;		// Transaction trdid
      sc_signal<size_t>	   r_init_rsp_to_tgt_rsp_pktid;		// Transaction pktid

      ///////////////////////////////////////////////////////
      // Registers controlled by CLEANUP fsm
      ///////////////////////////////////////////////////////

      sc_signal<int> 	     r_cleanup_fsm;         // FSM state
      sc_signal<size_t>      r_cleanup_srcid;       // transaction srcid
      sc_signal<size_t>      r_cleanup_trdid;       // transaction trdid
      sc_signal<size_t>      r_cleanup_pktid;       // transaction pktid
      sc_signal<addr_t>      r_cleanup_nline;       // cache line index

      sc_signal<copy_t>      r_cleanup_d_copies;    // bit-vector of copies 
      sc_signal<copy_t>      r_cleanup_i_copies;    // bit-vector of copies 
      sc_signal<copy_t>      r_cleanup_count;       // number of copies 
      sc_signal<tag_t>       r_cleanup_tag;	        // cache line tag (in directory)
      sc_signal<bool>        r_cleanup_is_cnt;      // inst bit (in directory)
      sc_signal<bool>        r_cleanup_lock;	    // lock bit (in directory)
      sc_signal<bool>        r_cleanup_dirty;	    // dirty bit (in directory)
      sc_signal<size_t>      r_cleanup_way;	        // associative way (in cache)

      sc_signal<size_t>      r_cleanup_write_srcid; // srcid of write response
      sc_signal<size_t>      r_cleanup_write_trdid; // trdid of write rsp
      sc_signal<size_t>      r_cleanup_write_pktid; // pktid of write rsp
      sc_signal<bool>        r_cleanup_need_rsp;    // needs a write rsp

      sc_signal<size_t>      r_cleanup_index;	    // index of the INVAL line (in the UPT)

      // Buffer between CLEANUP fsm and TGT_RSP fsm (acknowledge a write command from L1)
      sc_signal<bool>      r_cleanup_to_tgt_rsp_req;    // valid request
      sc_signal<size_t>    r_cleanup_to_tgt_rsp_srcid;  // transaction srcid
      sc_signal<size_t>    r_cleanup_to_tgt_rsp_trdid;	// transaction trdid
      sc_signal<size_t>    r_cleanup_to_tgt_rsp_pktid;	// transaction pktid

      ///////////////////////////////////////////////////////
      // Registers controlled by LLSC fsm
      ///////////////////////////////////////////////////////

      sc_signal<int>       r_llsc_fsm;          // FSM state
      sc_signal<data_t>	   r_llsc_data;		    // read data word
      sc_signal<copy_t>    r_llsc_i_copies;	    // bit_vector of copies
      sc_signal<copy_t>    r_llsc_d_copies;	    // bit_vector of copies
      sc_signal<copy_t>    r_llsc_count;	    // number of copies
      sc_signal<bool>      r_llsc_is_cnt;	    // is_cnt bit (in directory)
      sc_signal<bool>      r_llsc_dirty;	    // dirty bit (in directory)
      sc_signal<size_t>    r_llsc_way;		    // way in directory
      sc_signal<size_t>    r_llsc_set;		    // set in directory
      sc_signal<data_t>    r_llsc_tag;		    // cache line tag (in directory)
      sc_signal<size_t>    r_llsc_trt_index;    // Transaction Table index
      sc_signal<size_t>    r_llsc_upt_index;    // Update Table index

      // Buffer between LLSC fsm and INIT_CMD fsm (XRAM read)	
      sc_signal<bool>	   r_llsc_to_ixr_cmd_req;   // valid request
      sc_signal<addr_t>	   r_llsc_to_ixr_cmd_nline; // cache line index
      sc_signal<size_t>	   r_llsc_to_ixr_cmd_trdid; // index in Transaction Table
      sc_signal<bool>	   r_llsc_to_ixr_cmd_write; // write request
      sc_signal<data_t>	  *r_llsc_to_ixr_cmd_data;  // cache line data


      // Buffer between LLSC fsm and TGT_RSP fsm
      sc_signal<bool>	   r_llsc_to_tgt_rsp_req;   // valid request
      sc_signal<data_t>    r_llsc_to_tgt_rsp_data;  // read data word
      sc_signal<size_t>	   r_llsc_to_tgt_rsp_srcid; // Transaction srcid
      sc_signal<size_t>	   r_llsc_to_tgt_rsp_trdid; // Transaction trdid
      sc_signal<size_t>	   r_llsc_to_tgt_rsp_pktid; // Transaction pktid

      // Buffer between LLSC fsm and INIT_CMD fsm (Update/Invalidate L1 caches)
      sc_signal<bool>	   r_llsc_to_init_cmd_req;          // valid request
      sc_signal<bool>	   r_llsc_to_init_cmd_brdcast;      // brdcast request
      sc_signal<addr_t>	   r_llsc_to_init_cmd_nline;	    // cache line index
      sc_signal<size_t>	   r_llsc_to_init_cmd_trdid;	    // index in Update Table
      sc_signal<copy_t>    r_llsc_to_init_cmd_d_copies;     // bit_vector of L1 to update
      sc_signal<data_t>    r_llsc_to_init_cmd_wdata;        // data (one word)
      sc_signal<size_t>	   r_llsc_to_init_cmd_index;	    // index of the word in line

      ////////////////////////////////////////////////////
      // Registers controlled by the IXR_RSP fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 	   r_ixr_rsp_fsm;       // FSM state
      sc_signal<size_t>	   r_ixr_rsp_trt_index;	// TRT entry index
      sc_signal<size_t>    r_ixr_rsp_cpt;	    // word counter

      // Buffer between IXR_RSP fsm and XRAM_RSP fsm  (response from the XRAM)
      sc_signal<bool>	  *r_ixr_rsp_to_xram_rsp_rok;	// A xram response is ready

      ////////////////////////////////////////////////////
      // Registers controlled by the XRAM_RSP fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 	   r_xram_rsp_fsm;		        // FSM state
      sc_signal<size_t>	   r_xram_rsp_trt_index;	    // TRT entry index
      TransactionTabEntry  r_xram_rsp_trt_buf;		    // TRT entry local buffer
      sc_signal<bool>	   r_xram_rsp_victim_inval;	    // victim line invalidate 
      sc_signal<bool>	   r_xram_rsp_victim_is_cnt;    // victim line inst bit
      sc_signal<bool>	   r_xram_rsp_victim_dirty;	    // victim line dirty bit
      sc_signal<size_t>    r_xram_rsp_victim_way;	    // victim line way
      sc_signal<size_t>    r_xram_rsp_victim_set;	    // victim line set
      sc_signal<addr_t>    r_xram_rsp_victim_nline;     // victim line index
      sc_signal<copy_t>    r_xram_rsp_victim_d_copies;	// victim line copies
      sc_signal<copy_t>    r_xram_rsp_victim_i_copies;	// victim line copies
      sc_signal<copy_t>    r_xram_rsp_victim_count;	    // victim line number of copies
      sc_signal<data_t>	  *r_xram_rsp_victim_data;	    // victim line data
      sc_signal<size_t>	   r_xram_rsp_upt_index;	    // UPT entry index

      // Buffer between XRAM_RSP fsm and TGT_RSP fsm  (response to L1 cache)
      sc_signal<bool>	   r_xram_rsp_to_tgt_rsp_req;	// Valid request
      sc_signal<size_t>    r_xram_rsp_to_tgt_rsp_srcid;	// Transaction srcid
      sc_signal<size_t>    r_xram_rsp_to_tgt_rsp_trdid;	// Transaction trdid
      sc_signal<size_t>    r_xram_rsp_to_tgt_rsp_pktid;	// Transaction pktid
      sc_signal<data_t>   *r_xram_rsp_to_tgt_rsp_data;	// data (one cache line)
      sc_signal<size_t>    r_xram_rsp_to_tgt_rsp_word;  // first word index
      sc_signal<size_t>    r_xram_rsp_to_tgt_rsp_length;// length of the response

      // Buffer between XRAM_RSP fsm and INIT_CMD fsm (Inval L1 Caches) 
      sc_signal<bool>	     r_xram_rsp_to_init_cmd_req;    // Valid request
      sc_signal<bool>      r_xram_rsp_to_init_cmd_brdcast;  // Broadcast request
      sc_signal<addr_t>    r_xram_rsp_to_init_cmd_nline;    // cache line index;
      sc_signal<size_t>	   r_xram_rsp_to_init_cmd_trdid;    // index of UPT entry
      sc_signal<copy_t>    r_xram_rsp_to_init_cmd_d_copies; // bit_vector of copies
      sc_signal<copy_t>    r_xram_rsp_to_init_cmd_i_copies; // bit_vector of copies

      // Buffer between XRAM_RSP fsm and IXR_CMD fsm (XRAM write)
      sc_signal<bool>	   r_xram_rsp_to_ixr_cmd_req;	// Valid request
      sc_signal<addr_t>	   r_xram_rsp_to_ixr_cmd_nline;	// cache line index
      sc_signal<data_t>	  *r_xram_rsp_to_ixr_cmd_data;	// cache line data
      sc_signal<size_t>	   r_xram_rsp_to_ixr_cmd_trdid;	// index in transaction table

      ////////////////////////////////////////////////////
      // Registers controlled by the IXR_CMD fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 	   r_ixr_cmd_fsm;
      sc_signal<size_t>	   r_ixr_cmd_cpt;

      ////////////////////////////////////////////////////
      // Registers controlled by TGT_RSP fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 	   r_tgt_rsp_fsm;
      sc_signal<size_t>    r_tgt_rsp_cpt;

      ////////////////////////////////////////////////////
      // Registers controlled by INIT_CMD fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 	  r_init_cmd_fsm;
      sc_signal<size_t>   r_init_cmd_cpt;
      sc_signal<size_t>	  r_init_cmd_target;
      sc_signal<bool>     r_init_cmd_inst;

      ////////////////////////////////////////////////////
      // Registers controlled by ALLOC_DIR fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 		r_alloc_dir_fsm;

      ////////////////////////////////////////////////////
      // Registers controlled by ALLOC_TRT fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 		r_alloc_trt_fsm;

      ////////////////////////////////////////////////////
      // Registers controlled by ALLOC_UPT fsm
      ////////////////////////////////////////////////////

      sc_signal<int> 		r_alloc_upt_fsm;

    }; // end class VciMemCacheV2

}}

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

