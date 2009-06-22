/* -*- c++ -*-
 * File 	: vci_mem_cache_v1.cpp
 * Date 	: 30/10/2008
 * Copyright 	: UPMC / LIP6
 * Authors 	: Alain Greiner / Eric Guthmuller
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
 * Modifications done by Christophe Choichillon:
 * - Adding a new VCI target port for the CLEANUP network : p_vci_tgt_cleanup
 * - Add a new VCI target port for the CLEANUP network
 * - Modify the CLEANUP FSM to add the fact that a CLEANUP can arrive to the
 * - Modify the ALLOC_UPT_FSM for allocating the UPT/INVAL Table to the 
 * CLEANUP FSM MEM_CACHE when a the line is going to be INVALIDATE
 *
 * Modifications to do : 
 *
 */
#include "../include/vci_mem_cache_v1.h"

#define DEBUG_VCI_MEM_CACHE 0

namespace soclib { namespace caba {

#ifdef DEBUG_VCI_MEM_CACHE 
  const char *tgt_cmd_fsm_str[] = {
    "TGT_CMD_IDLE",
    "TGT_CMD_READ",
    "TGT_CMD_READ_EOP",
    "TGT_CMD_WRITE",
    "TGT_CMD_ATOMIC",
  };
  const char *tgt_rsp_fsm_str[] = {
    "TGT_RSP_READ_IDLE",
    "TGT_RSP_WRITE_IDLE",
    "TGT_RSP_LLSC_IDLE",
    "TGT_RSP_XRAM_IDLE",
    "TGT_RSP_INIT_IDLE",
    "TGT_RSP_CLEANUP_IDLE",
    "TGT_RSP_READ_TEST",
    "TGT_RSP_READ_WORD",
    "TGT_RSP_READ_LINE",
    "TGT_RSP_WRITE",
    "TGT_RSP_LLSC",
    "TGT_RSP_XRAM_TEST",
    "TGT_RSP_XRAM_WORD",
    "TGT_RSP_XRAM_LINE",
    "TGT_RSP_INIT",
    "TGT_RSP_CLEANUP",
  };
  const char *init_cmd_fsm_str[] = {
    "INIT_CMD_INVAL_IDLE",
    "INIT_CMD_INVAL_SEL",
    "INIT_CMD_INVAL_NLINE",
    "INIT_CMD_UPDT_IDLE",
    "INIT_CMD_UPDT_SEL",
    "INIT_CMD_BRDCAST",
    "INIT_CMD_UPDT_NLINE",
    "INIT_CMD_UPDT_INDEX",
    "INIT_CMD_UPDT_DATA",
  };
  const char *init_rsp_fsm_str[] = {
    "INIT_RSP_IDLE",
    "INIT_RSP_UPT_LOCK",
    "INIT_RSP_UPT_CLEAR",
    "INIT_RSP_END",
  };
  const char *read_fsm_str[] = {
    "READ_IDLE",
    "READ_DIR_LOCK",
    "READ_DIR_HIT",
    "READ_RSP",
    "READ_TRT_LOCK",
    "READ_TRT_SET",
    "READ_XRAM_REQ",
  };
  const char *write_fsm_str[] = {
    "WRITE_IDLE",
    "WRITE_NEXT",
    "WRITE_DIR_LOCK",
    "WRITE_DIR_HIT_READ",
    "WRITE_DIR_HIT",
    "WRITE_UPT_LOCK",
    "WRITE_WAIT_UPT",
    "WRITE_UPDATE",
    "WRITE_RSP",
    "WRITE_TRT_LOCK",
    "WRITE_TRT_DATA",
    "WRITE_TRT_SET",
    "WRITE_WAIT_TRT",
    "WRITE_XRAM_REQ",
    "WRITE_TRT_WRITE_LOCK",
    "WRITE_INVAL_LOCK",
    "WRITE_DIR_INVAL",
    "WRITE_INVAL",
    "WRITE_XRAM_SEND",
  };
  const char *ixr_rsp_fsm_str[] = {
    "IXR_RSP_IDLE",
    "IXR_RSP_ACK",
    "IXR_RSP_TRT_ERASE",
    "IXR_RSP_TRT_READ",
  };
  const char *xram_rsp_fsm_str[] = {
    "XRAM_RSP_IDLE",
    "XRAM_RSP_TRT_COPY",
    "XRAM_RSP_TRT_DIRTY",
    "XRAM_RSP_DIR_LOCK",
    "XRAM_RSP_DIR_UPDT",
    "XRAM_RSP_DIR_RSP",
    "XRAM_RSP_INVAL_LOCK",
    "XRAM_RSP_INVAL_WAIT",
    "XRAM_RSP_UPT_LOCK",
    "XRAM_RSP_WAIT",
    "XRAM_RSP_INVAL",
    "XRAM_RSP_WRITE_DIRTY",
  };
  const char *xram_cmd_fsm_str[] = {
    "XRAM_CMD_READ_IDLE",
    "XRAM_CMD_WRITE_IDLE",
    "XRAM_CMD_LLSC_IDLE",
    "XRAM_CMD_XRAM_IDLE",
    "XRAM_CMD_READ_NLINE",
    "XRAM_CMD_WRITE_NLINE",
    "XRAM_CMD_LLSC_NLINE",
    "XRAM_CMD_XRAM_DATA",
  };
  const char *llsc_fsm_str[] = {
    "LLSC_IDLE",
    "LL_DIR_LOCK",
    "LL_DIR_HIT",
    "LL_RSP",
    "SC_DIR_LOCK",
    "SC_DIR_HIT",
    "SC_RSP_FALSE",
    "SC_RSP_TRUE",
    "LLSC_TRT_LOCK",
    "LLSC_TRT_SET",
    "LLSC_XRAM_REQ",
  };
  const char *cleanup_fsm_str[] = {
    "CLEANUP_IDLE",
    "CLEANUP_DIR_LOCK",
    "CLEANUP_DIR_WRITE",
    "CLEANUP_UPT_LOCK",
    "CLEANUP_UPT_WRITE",
    "CLEANUP_WRITE_RSP",
    "CLEANUP_RSP",
  };
  const char *alloc_dir_fsm_str[] = {
    "ALLOC_DIR_READ",
    "ALLOC_DIR_WRITE",
    "ALLOC_DIR_LLSC",
    "ALLOC_DIR_CLEANUP",
    "ALLOC_DIR_XRAM_RSP",
  };
  const char *alloc_trt_fsm_str[] = {
    "ALLOC_TRT_READ",
    "ALLOC_TRT_WRITE",
    "ALLOC_TRT_LLSC",
    "ALLOC_TRT_XRAM_RSP",
    "ALLOC_TRT_IXR_RSP",
  };
  const char *alloc_upt_fsm_str[] = {
    "ALLOC_UPT_WRITE",
    "ALLOC_UPT_XRAM_RSP",
    "ALLOC_UPT_INIT_RSP",
    "ALLOC_UPT_CLEANUP",
  };
#endif

#define tmpl(x) template<typename vci_param> x VciMemCacheV1<vci_param>

  using soclib::common::uint32_log2;

  ////////////////////////////////
  // 	Constructor 
  ////////////////////////////////

  tmpl(/**/)::VciMemCacheV1( 
      sc_module_name name,
      const soclib::common::MappingTable &mtp,
      const soclib::common::MappingTable &mtc,
      const soclib::common::MappingTable &mtx,
      const soclib::common::IntTab &vci_ixr_index,
      const soclib::common::IntTab &vci_ini_index,
      const soclib::common::IntTab &vci_tgt_index,
      const soclib::common::IntTab &vci_tgt_index_cleanup,
      size_t nways,
      size_t nsets,
      size_t nwords)

    : soclib::caba::BaseModule(name),

    p_clk("clk"),
    p_resetn("resetn"),
    p_vci_tgt("vci_tgt"),
    p_vci_tgt_cleanup("vci_tgt_cleanup"),
    p_vci_ini("vci_ini"),
    p_vci_ixr("vci_ixr"),

    m_initiators( 32 ),
    m_ways( nways ),
    m_sets( nsets ),
    m_words( nwords ),
    m_srcid_ixr( mtx.indexForId(vci_ixr_index) ),
    m_srcid_ini( mtc.indexForId(vci_ini_index) ),
    //m_mem_segment("bidon",0,0,soclib::common::IntTab(),false),
    m_seglist(mtp.getSegmentList(vci_tgt_index)),
    m_reg_segment("bidon",0,0,soclib::common::IntTab(),false),
    m_coherence_table( mtc.getCoherenceTable() ),
    m_atomic_tab( m_initiators ),
    m_transaction_tab( TRANSACTION_TAB_LINES, nwords ),
    m_update_tab( UPDATE_TAB_LINES ),
    m_cache_directory( nways, nsets, nwords, vci_param::N ),
    nseg(0),	
#define L2 soclib::common::uint32_log2
    m_x( L2(m_words), 2),
    m_y( L2(m_sets), L2(m_words) + 2),
    m_z( vci_param::N - L2(m_sets) - L2(m_words) - 2, L2(m_sets) + L2(m_words) + 2),
    m_nline( vci_param::N - L2(m_words) - 2, L2(m_words) + 2),
#undef L2

    //  FIFOs 
    m_cmd_read_addr_fifo("m_cmd_read_addr_fifo", 4),
    m_cmd_read_word_fifo("m_cmd_read_word_fifo", 4),
    m_cmd_read_srcid_fifo("m_cmd_read_srcid_fifo", 4),
    m_cmd_read_trdid_fifo("m_cmd_read_trdid_fifo", 4),
    m_cmd_read_pktid_fifo("m_cmd_read_pktid_fifo", 4),

    m_cmd_write_addr_fifo("m_cmd_write_addr_fifo",8),
    m_cmd_write_eop_fifo("m_cmd_write_eop_fifo",8),
    m_cmd_write_srcid_fifo("m_cmd_write_srcid_fifo",8),
    m_cmd_write_trdid_fifo("m_cmd_write_trdid_fifo",8),
    m_cmd_write_pktid_fifo("m_cmd_write_pktid_fifo",8),
    m_cmd_write_data_fifo("m_cmd_write_data_fifo",8),
    m_cmd_write_be_fifo("m_cmd_write_be_fifo",8),

    m_cmd_llsc_addr_fifo("m_cmd_llsc_addr_fifo",4),
    m_cmd_llsc_sc_fifo("m_cmd_llsc_sc_fifo",4),
    m_cmd_llsc_srcid_fifo("m_cmd_llsc_srcid_fifo",4),
    m_cmd_llsc_trdid_fifo("m_cmd_llsc_trdid_fifo",4),
    m_cmd_llsc_pktid_fifo("m_cmd_llsc_pktid_fifo",4),
    m_cmd_llsc_wdata_fifo("m_cmd_llsc_wdata_fifo",4),

    //	       m_cmd_cleanup_srcid_fifo("m_cmd_cleanup_srcid_fifo",4),
    //	       m_cmd_cleanup_trdid_fifo("m_cmd_cleanup_trdid_fifo",4),
    //	       m_cmd_cleanup_pktid_fifo("m_cmd_cleanup_pktid_fifo",4),
    //	       m_cmd_cleanup_nline_fifo("m_cmd_cleanup_nline_fifo",4),

    r_tgt_cmd_fsm("r_tgt_cmd_fsm"),
    r_read_fsm("r_read_fsm"),
    r_write_fsm("r_write_fsm"),
    r_init_rsp_fsm("r_init_rsp_fsm"),
    r_cleanup_fsm("r_cleanup_fsm"),
    r_llsc_fsm("r_llsc_fsm"),
    r_ixr_rsp_fsm("r_ixr_rsp_fsm"),
    r_xram_rsp_fsm("r_xram_rsp_fsm"),
    r_xram_cmd_fsm("r_xram_cmd_fsm"),
    r_tgt_rsp_fsm("r_tgt_rsp_fsm"),
    r_init_cmd_fsm("r_init_cmd_fsm"),
    r_alloc_dir_fsm("r_alloc_dir_fsm"),
    r_alloc_trt_fsm("r_alloc_trt_fsm"),
    r_alloc_upt_fsm("r_alloc_upt_fsm")
    {
      assert(IS_POW_OF_2(nsets));
      assert(IS_POW_OF_2(nwords));
      assert(IS_POW_OF_2(nways));
      assert(nsets);
      assert(nwords);
      assert(nways);
      assert(nsets <= 1024);
      assert(nwords <= 32);
      assert(nways <= 32);


      // Get the segments associated to the MemCache 
      //std::list<soclib::common::Segment> segList(mtp.getSegmentList(vci_tgt_index));
      std::list<soclib::common::Segment>::iterator seg;
      /*
         for(seg = segList.begin(); seg != segList.end() ; seg++) {
         if( seg->size() > 8 ) m_mem_segment = *seg; 
         else                  m_reg_segment = *seg;
         nseg++;
         }
         */

      for(seg = m_seglist.begin(); seg != m_seglist.end() ; seg++) {
        if( seg->size() > 8 ) nseg++;
      }
      //assert( (nseg == 2) && (m_reg_segment.size() == 8) );

      m_seg = new soclib::common::Segment*[nseg];
      size_t i = 0;
      for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ ) { 
        if ( seg->size() > 8 ) 
        {
          m_seg[i] = &(*seg);
          i++;

        }
        else
        {
          m_reg_segment = *seg; // a supprimer

        }		
      }

      assert( (m_reg_segment.size() == 8) );

      // Memory cache allocation & initialisation
      m_cache_data = new data_t**[nways];
      for ( size_t i=0 ; i<nways ; ++i ) {
        m_cache_data[i] = new data_t*[nsets];
      }
      for ( size_t i=0; i<nways; ++i ) {
        for ( size_t j=0; j<nsets; ++j ) {
          m_cache_data[i][j] = new data_t[nwords];
          for ( size_t k=0; k<nwords; k++){
            m_cache_data[i][j][k]=0;
          }	
        }
      }

      // Allocation for IXR_RSP FSM
      r_ixr_rsp_to_xram_rsp_rok	    	= new sc_signal<bool>[TRANSACTION_TAB_LINES];

      // Allocation for XRAM_RSP FSM
      r_xram_rsp_victim_data        	= new sc_signal<data_t>[nwords];
      r_xram_rsp_to_tgt_rsp_data    	= new sc_signal<data_t>[nwords];
      r_xram_rsp_to_tgt_rsp_val     	= new sc_signal<bool>[nwords];
      r_xram_rsp_to_xram_cmd_data   	= new sc_signal<data_t>[nwords];

      // Allocation for READ FSM
      r_read_data 			= new sc_signal<data_t>[nwords];
      r_read_to_tgt_rsp_data 		= new sc_signal<data_t>[nwords];
      r_read_to_tgt_rsp_val 		= new sc_signal<bool>[nwords];

      // Allocation for WRITE FSM
      r_write_data 			= new sc_signal<data_t>[nwords];
      r_write_be 			= new sc_signal<be_t>[nwords];
      r_write_to_init_cmd_data 		= new sc_signal<data_t>[nwords];
      r_write_to_init_cmd_we		= new sc_signal<bool>[nwords];
      r_write_to_xram_cmd_data	= new sc_signal<data_t>[nwords];

      // Simulation

      SC_METHOD(transition);
      dont_initialize();
      sensitive << p_clk.pos();

      SC_METHOD(genMoore);
      dont_initialize();
      sensitive << p_clk.neg();

    } // end constructor

  /////////////////////////////////////////
  // This function prints the statistics 
  /////////////////////////////////////////

  tmpl(void)::print_stats()
  {
    std::cout << "----------------------------------" << std::dec << std::endl;
    std::cout << "MEM_CACHE " << m_srcid_ini << " / Time = " << m_cpt_cycles << std::endl
      << "- READ RATE           = " << (double)m_cpt_read/m_cpt_cycles << std::endl
      << "- READ MISS RATE      = " << (double)m_cpt_read_miss/m_cpt_read << std::endl
      << "- WRITE RATE          = " << (double)m_cpt_write/m_cpt_cycles << std::endl
      << "- WRITE MISS RATE     = " << (double)m_cpt_write_miss/m_cpt_write << std::endl
      << "- WRITE BURST LENGTH  = " << (double)m_cpt_write_cells/m_cpt_write << std::endl
      << "- UPDATE RATE         = " << (double)m_cpt_update/m_cpt_cycles << std::endl
      << "- UPDATE ARITY        = " << (double)m_cpt_update_mult/m_cpt_update << std::endl
      << "- INVAL MULTICAST RATE          = " << (double)(m_cpt_inval-m_cpt_inval_brdcast)/m_cpt_cycles << std::endl
      << "- INVAL MULTICAST ARITY         = " << (double)m_cpt_inval_mult/(m_cpt_inval-m_cpt_inval_brdcast) << std::endl
      << "- INVAL BROADCAST RATE          = " << (double)m_cpt_inval_brdcast/m_cpt_cycles << std::endl
      << "- SAVE DIRTY RATE     = " << (double)m_cpt_write_dirty/m_cpt_cycles << std::endl
      << "- CLEANUP RATE        = " << (double)m_cpt_cleanup/m_cpt_cycles << std::endl
      << "- LL RATE             = " << (double)m_cpt_ll/m_cpt_cycles << std::endl
      << "- SC RATE             = " << (double)m_cpt_sc/m_cpt_cycles << std::endl;
  }

  /////////////////////////////////
  tmpl(/**/)::~VciMemCacheV1()
    /////////////////////////////////
  {
    for(size_t i=0; i<m_ways ; i++){
      for(size_t j=0; j<m_sets ; j++){
        delete [] m_cache_data[i][j];
      }
    }
    for(size_t i=0; i<m_ways ; i++){
      delete [] m_cache_data[i];
    }
    delete [] m_cache_data;
    delete [] m_coherence_table;

    delete [] r_ixr_rsp_to_xram_rsp_rok;

    delete [] r_xram_rsp_victim_data;
    delete [] r_xram_rsp_to_tgt_rsp_data;
    delete [] r_xram_rsp_to_tgt_rsp_val;
    delete [] r_xram_rsp_to_xram_cmd_data;

    delete [] r_read_data;
    delete [] r_read_to_tgt_rsp_data;
    delete [] r_read_to_tgt_rsp_val;

    delete [] r_write_data;
    delete [] r_write_be;
    delete [] r_write_to_init_cmd_data;
  }

  //////////////////////////////////
  tmpl(void)::transition()
    //////////////////////////////////
  {
    using soclib::common::uint32_log2;
    //  RESET          
    if ( ! p_resetn.read() ) {

      //     Initializing FSMs
      r_tgt_cmd_fsm 	= TGT_CMD_IDLE;
      r_tgt_rsp_fsm 	= TGT_RSP_READ_IDLE;
      r_init_cmd_fsm 	= INIT_CMD_INVAL_IDLE;
      r_init_rsp_fsm 	= INIT_RSP_IDLE;
      r_read_fsm 	= READ_IDLE;
      r_write_fsm 	= WRITE_IDLE;
      r_llsc_fsm 	= LLSC_IDLE;
      r_cleanup_fsm 	= CLEANUP_IDLE;
      r_alloc_dir_fsm = ALLOC_DIR_READ;
      r_alloc_trt_fsm = ALLOC_TRT_READ;
      r_alloc_upt_fsm = ALLOC_UPT_WRITE;
      r_ixr_rsp_fsm 	= IXR_RSP_IDLE;
      r_xram_rsp_fsm 	= XRAM_RSP_IDLE;
      r_xram_cmd_fsm 	= XRAM_CMD_READ_IDLE;

      //  Initializing Tables
      m_cache_directory.init();
      m_atomic_tab.init();	
      m_transaction_tab.init();

      // initializing FIFOs and communication Buffers

      m_cmd_read_addr_fifo.init();
      m_cmd_read_word_fifo.init();
      m_cmd_read_srcid_fifo.init();
      m_cmd_read_trdid_fifo.init();
      m_cmd_read_pktid_fifo.init();

      m_cmd_write_addr_fifo.init();
      m_cmd_write_eop_fifo.init();
      m_cmd_write_srcid_fifo.init();
      m_cmd_write_trdid_fifo.init();
      m_cmd_write_pktid_fifo.init();
      m_cmd_write_data_fifo.init();

      m_cmd_llsc_addr_fifo.init();
      m_cmd_llsc_srcid_fifo.init();
      m_cmd_llsc_trdid_fifo.init();
      m_cmd_llsc_pktid_fifo.init();
      m_cmd_llsc_wdata_fifo.init();
      m_cmd_llsc_sc_fifo.init();

      r_read_to_tgt_rsp_req		= false;
      r_read_to_xram_cmd_req		= false;

      r_write_to_tgt_rsp_req		= false;
      r_write_to_xram_cmd_req		= false;
      r_write_to_init_cmd_req		= false;

      r_cleanup_to_tgt_rsp_req	= false;

      r_init_rsp_to_tgt_rsp_req	= false;

      r_llsc_to_tgt_rsp_req		= false;
      r_llsc_to_xram_cmd_req		= false;

      for(size_t i=0; i<TRANSACTION_TAB_LINES ; i++){
        r_ixr_rsp_to_xram_rsp_rok[i]= false;
      }

      r_xram_rsp_to_tgt_rsp_req	= false;
      r_xram_rsp_to_init_cmd_req	= false;
      r_xram_rsp_to_xram_cmd_req	= false;
      r_xram_rsp_trt_index		= 0;

      r_xram_cmd_cpt = 0;

      r_copies_limit = 2;

      // Activity counters
      m_cpt_cycles		= 0;
      m_cpt_read		= 0;
      m_cpt_read_miss		= 0;
      m_cpt_write		= 0;
      m_cpt_write_miss	= 0;
      m_cpt_write_cells	= 0;
      m_cpt_write_dirty	= 0;
      m_cpt_update		= 0;
      m_cpt_update_mult 	= 0;
      m_cpt_inval_brdcast 	= 0;
      m_cpt_inval 		= 0;
      m_cpt_inval_mult 	= 0;
      m_cpt_cleanup		= 0;
      m_cpt_ll     		= 0;
      m_cpt_sc     		= 0;

      return;
    }

    bool    cmd_read_fifo_put = false;
    bool    cmd_read_fifo_get = false;

    bool    cmd_write_fifo_put = false;
    bool    cmd_write_fifo_get = false;

    bool    cmd_llsc_fifo_put = false;
    bool    cmd_llsc_fifo_get = false;

#if DEBUG_VCI_MEM_CACHE 
    std::cout << "---------------------------------------------" << std::dec << std::endl;
    std::cout << "MEM_CACHE " << m_srcid_ini << " ; Time = " << m_cpt_cycles << std::endl
      << " - TGT_CMD FSM   = " << tgt_cmd_fsm_str[r_tgt_cmd_fsm] << std::endl
      << " - TGT_RSP FSM   = " << tgt_rsp_fsm_str[r_tgt_rsp_fsm] << std::endl
      << " - INIT_CMD FSM  = " << init_cmd_fsm_str[r_init_cmd_fsm] << std::endl
      << " - INIT_RSP FSM  = " << init_rsp_fsm_str[r_init_rsp_fsm] << std::endl
      << " - READ FSM      = " << read_fsm_str[r_read_fsm] << std::endl
      << " - WRITE FSM     = " << write_fsm_str[r_write_fsm] << std::endl
      << " - LLSC FSM      = " << llsc_fsm_str[r_llsc_fsm] << std::endl
      << " - CLEANUP FSM   = " << cleanup_fsm_str[r_cleanup_fsm] << std::endl
      << " - XRAM_CMD FSM  = " << xram_cmd_fsm_str[r_xram_cmd_fsm] << std::endl
      << " - IXR_RSP FSM   = " << ixr_rsp_fsm_str[r_ixr_rsp_fsm] << std::endl
      << " - XRAM_RSP FSM  = " << xram_rsp_fsm_str[r_xram_rsp_fsm] << std::endl
      << " - ALLOC_DIR FSM = " << alloc_dir_fsm_str[r_alloc_dir_fsm] << std::endl
      << " - ALLOC_TRT FSM = " << alloc_trt_fsm_str[r_alloc_trt_fsm] << std::endl
      << " - ALLOC_UPT FSM = " << alloc_upt_fsm_str[r_alloc_upt_fsm] << std::endl;
#endif


    ////////////////////////////////////////////////////////////////////////////////////
    //		TGT_CMD FSM
    ////////////////////////////////////////////////////////////////////////////////////
    // The TGT_CMD_FSM controls the incoming VCI command pakets from the processors
    //
    // There is 4 types of packets for the m_mem_segment :
    // - READ    : a READ request has a length of 1 VCI cell. It can be a single word 
    //             or an entire cache line, depending on the PLEN value.
    // - WRITE   : a WRITE request has a maximum length of 16 cells, and can only
    //             concern words in a same line.
    // - LL      : The LL request has a length of 1 cell.
    // - SC      : The SC request has a length of 1 cell. 
    //             The WDATA field contains the data to write.
    //
    ////////////////////////////////////////////////////////////////////////////////////

    switch ( r_tgt_cmd_fsm.read() ) {

      //////////////////
      case TGT_CMD_IDLE:
        {
          if ( p_vci_tgt.cmdval ) {
            assert( (p_vci_tgt.srcid.read() < m_initiators)
                && "VCI_MEM_CACHE error in VCI_MEM_CACHE : The received SRCID is larger than 31");

            bool reached = false;
            for ( size_t index = 0 ; index < nseg && !reached ; index++) 
            {
              if ( m_seg[index]->contains(p_vci_tgt.address.read()) ) {
                reached = true;
                r_index = index;
              }
            }


            if ( !reached ) 
            { 
              std::cout << "VCI_MEM_CACHE Out of segment access in VCI_MEM_CACHE" << std::endl;
              std::cout << "Faulty address = " << p_vci_tgt.address.read() << std::endl;
              std::cout << "Faulty initiator = " << p_vci_tgt.srcid.read() << std::endl;
              exit(0);
            } 
            else if ( p_vci_tgt.cmd.read() == vci_param::CMD_READ ) 
            {
              r_tgt_cmd_fsm = TGT_CMD_READ;
            } 
            else if (( p_vci_tgt.cmd.read() == vci_param::CMD_WRITE ) && ( p_vci_tgt.trdid.read() == 0x0 ))
            {  
              r_tgt_cmd_fsm = TGT_CMD_WRITE;
            } 
            else if ((p_vci_tgt.cmd.read() == vci_param::CMD_LOCKED_READ) || 
                (p_vci_tgt.cmd.read() == vci_param::CMD_STORE_COND) ) 
            {
              r_tgt_cmd_fsm = TGT_CMD_ATOMIC;
            } 
            //	    else if (( p_vci_tgt.cmd.read() == vci_param::CMD_WRITE ) && ( p_vci_tgt.trdid.read() == 0x1 ))
            //	    {  
            //	      r_tgt_cmd_fsm = TGT_CMD_CLEANUP;
            //	    } 
          }
          break;
        }
        //////////////////
      case TGT_CMD_READ:

        {
          assert(((p_vci_tgt.plen.read() == 4) || (p_vci_tgt.plen.read() == m_words*4))
              && "VCI_MEM_CACHE All read request to the MemCache must have PLEN = 4 or PLEN = 4*nwords"); 

          if ( p_vci_tgt.cmdval && m_cmd_read_addr_fifo.wok() ) {
            cmd_read_fifo_put = true;
            if ( p_vci_tgt.eop )  r_tgt_cmd_fsm = TGT_CMD_IDLE;
            else                  r_tgt_cmd_fsm = TGT_CMD_READ_EOP;		
          } 
          break;
        }
        //////////////////////
      case TGT_CMD_READ_EOP:
        {
          if ( p_vci_tgt.cmdval && p_vci_tgt.eop ){
            r_tgt_cmd_fsm = TGT_CMD_IDLE;
          }
          break;
        }
        ///////////////////
      case TGT_CMD_WRITE:
        {

          if ( p_vci_tgt.cmdval && m_cmd_write_addr_fifo.wok() ) {
            cmd_write_fifo_put = true;
            if(  p_vci_tgt.eop )  r_tgt_cmd_fsm = TGT_CMD_IDLE;

          }
          break;
        }
        ////////////////////
      case TGT_CMD_ATOMIC:
        {
          assert(p_vci_tgt.eop && "Memory Cache Error: LL or SC command with length > 1 ");

          if ( p_vci_tgt.cmdval && m_cmd_llsc_addr_fifo.wok() ) {
            cmd_llsc_fifo_put = true;
            r_tgt_cmd_fsm = TGT_CMD_IDLE;
          }
          break;
        }
    } // end switch tgt_cmd_fsm

    /////////////////////////////////////////////////////////////////////////
    //		INIT_RSP FSM
    /////////////////////////////////////////////////////////////////////////
    // This FSM controls the response to the update or invalidate requests
    // sent by the memory cache to the L1 caches :
    //
    // - update request initiated by the WRITE FSM.  
    //   The FSM decrements the proper entry in the Update/Inval Table.
    //   It sends a request to the TGT_RSP FSM to complete the pending 
    //   write transaction (acknowledge response to the writer processor), 
    //   and clear the UPT entry when all responses have been received.  
    // - invalidate request initiated by the XRAM_RSP FSM.
    //   The FSM decrements the proper entry in the Update/Inval_Table,
    //   and clear the entry when all responses have been received.
    //
    // All those response packets are one word, compact
    // packets complying with the VCI advanced format. 
    // The index in the Table is defined in the RTRDID field, and
    // the Transaction type is defined in the Update/Inval Table.
    /////////////////////////////////////////////////////////////////////

    switch ( r_init_rsp_fsm.read() ) {

      ///////////////////
      case INIT_RSP_IDLE:
        {

          if ( p_vci_ini.rspval ) {

            assert ( ( p_vci_ini.rtrdid.read() < m_update_tab.size() )
                && "VCI_MEM_CACHE UPT index too large in VCI response paquet received by memory cache" );
            assert ( p_vci_ini.reop 
                && "VCI_MEM_CACHE All response packets to update/invalidate requests must be one cell" );
            r_init_rsp_upt_index = p_vci_ini.rtrdid.read(); 
            r_init_rsp_fsm = INIT_RSP_UPT_LOCK;
          }
          break;
        }
        ///////////////////////
      case INIT_RSP_UPT_LOCK:	// decrement the number of expected responses
        {

          if ( r_alloc_upt_fsm.read() == ALLOC_UPT_INIT_RSP ) { 
            size_t count = 0;
            bool valid  = m_update_tab.decrement(r_init_rsp_upt_index.read(), count);

            assert ( valid 
                && "VCI_MEM_CACHE Invalid UPT entry in VCI response paquet received by memory cache" );

            if ( count == 0 ) r_init_rsp_fsm = INIT_RSP_UPT_CLEAR;
            else              r_init_rsp_fsm = INIT_RSP_IDLE;
          }
          break;
        }
        ////////////////////////
      case INIT_RSP_UPT_CLEAR:	// clear the UPT entry
        {
          if ( r_alloc_upt_fsm.read() == ALLOC_UPT_INIT_RSP ) {
            r_init_rsp_srcid = m_update_tab.srcid(r_init_rsp_upt_index.read());
            r_init_rsp_trdid = m_update_tab.trdid(r_init_rsp_upt_index.read());
            r_init_rsp_pktid = m_update_tab.pktid(r_init_rsp_upt_index.read());
            r_init_rsp_nline = m_update_tab.nline(r_init_rsp_upt_index.read());
            bool need_rsp = m_update_tab.need_rsp(r_init_rsp_upt_index.read());
            if ( need_rsp ) r_init_rsp_fsm = INIT_RSP_END;
            else            r_init_rsp_fsm = INIT_RSP_IDLE;
            m_update_tab.clear(r_init_rsp_upt_index.read());
          }
          break;
        }
        //////////////////
      case INIT_RSP_END:
        {

          if ( !r_init_rsp_to_tgt_rsp_req ) {
            r_init_rsp_to_tgt_rsp_req = true;
            r_init_rsp_to_tgt_rsp_srcid = r_init_rsp_srcid.read();
            r_init_rsp_to_tgt_rsp_trdid = r_init_rsp_trdid.read();
            r_init_rsp_to_tgt_rsp_pktid = r_init_rsp_pktid.read();
            r_init_rsp_fsm = INIT_RSP_IDLE;
          }
          break;
        }
    } // end switch r_init_rsp_fsm

    ////////////////////////////////////////////////////////////////////////////////////
    //		READ FSM
    ////////////////////////////////////////////////////////////////////////////////////
    // The READ FSM controls the read requests sent by processors.
    // It takes the lock protecting the cache directory to check the cache line status:
    // - In case of HIT, the fsm copies the data (one line, or one single word)
    //   in the r_read_to_tgt_rsp buffer. It waits if this buffer is not empty.
    //   The requesting initiator is registered in the cache directory.
    // - In case of MISS, the READ fsm takes the lock protecting the transaction tab.
    //   If a read transaction to the XRAM for this line already exists,
    //   or if the transaction tab is full, the fsm is stalled.
    //   If a transaction entry is free, the READ fsm sends a request to the XRAM.
    ////////////////////////////////////////////////////////////////////////////////////

    switch ( r_read_fsm.read() ) {

      ///////////////
      case READ_IDLE:
        {
          if (m_cmd_read_addr_fifo.rok()) {
            m_cpt_read++;
            r_read_fsm = READ_DIR_LOCK;
          }
          break;
        }
        ///////////////////
      case READ_DIR_LOCK:	// check directory for hit / miss
        {
          if( r_alloc_dir_fsm.read() == ALLOC_DIR_READ ) {
            size_t way = 0;
            DirectoryEntry entry = m_cache_directory.read(m_cmd_read_addr_fifo.read(), way);

            r_read_is_cnt   = entry.is_cnt;
            r_read_dirty	  = entry.dirty;
            r_read_tag	    = entry.tag;
            r_read_lock	    = entry.lock;
            r_read_way	    = way;
            r_read_word	    = m_cmd_read_word_fifo.read();
            r_read_d_copies = entry.d_copies; 
            r_read_i_copies = entry.i_copies; 
            r_read_count    = entry.count;

            // In case of hit, the read acces must be registered in the copies bit-vector
            if( entry.valid )  { 
              r_read_fsm = READ_DIR_HIT;
            } else {
              r_read_fsm = READ_TRT_LOCK;
              m_cpt_read_miss++;
            }
          }
          break;
        }
        //////////////////
      case READ_DIR_HIT:	// read hit : update the memory cache
        {
          if( r_alloc_dir_fsm.read() == ALLOC_DIR_READ ) {
            // signals generation
            bool inst_read = (m_cmd_read_trdid_fifo.read() & 0x2);
            bool cached_read = (m_cmd_read_trdid_fifo.read() & 0x1);
            bool is_cnt = ((r_read_count.read() >= r_copies_limit.read()) && cached_read) || r_read_is_cnt.read();

            // read data in the cache
            size_t set = m_y[m_cmd_read_addr_fifo.read()];
            size_t way = r_read_way.read();
            for ( size_t i=0 ; i<m_words ; i++ ) {
              r_read_data[i] = m_cache_data[way][set][i];
            }

            // update the cache directory (for the copies)
            DirectoryEntry entry;
            entry.valid	  = true;
            entry.is_cnt  = is_cnt; // when we reach the limit of copies
            entry.dirty	  = r_read_dirty.read();
            entry.tag	    = r_read_tag.read();
            entry.lock	  = r_read_lock.read();
            if(cached_read){  // Cached read, we update the copies vector
              if(inst_read && !is_cnt){ // Data read, vector mode
                entry.d_copies  = r_read_d_copies.read();
                entry.i_copies  = r_read_i_copies.read() | (0x1 << m_cmd_read_srcid_fifo.read()); 
                entry.count     = r_read_count.read() + 1;
              }
              if(!inst_read && !is_cnt){ // Instruction read, vector mode
                entry.d_copies  = r_read_d_copies.read() | (0x1 << m_cmd_read_srcid_fifo.read()); 
                entry.i_copies  = r_read_i_copies.read();
                entry.count     = r_read_count.read() + 1;
              }
              if( is_cnt ) { // Counter mode
                entry.count     = r_read_count.read() + 1;
                entry.d_copies  = 0;
                entry.i_copies  = 0;
              } 
            } else { // Uncached read
              entry.d_copies = r_read_d_copies.read();
              entry.i_copies = r_read_i_copies.read();
              entry.count    = r_read_count.read();
            }

            m_cache_directory.write(set, way, entry);
            r_read_fsm    = READ_RSP;
          }
          break;
        }
        //////////////
      case READ_RSP: 		//  request the TGT_RSP FSM to return data
        {
          if( !r_read_to_tgt_rsp_req ) {	
            for ( size_t i=0 ; i<m_words ; i++ ) {
              r_read_to_tgt_rsp_data[i] = r_read_data[i];
              if ( r_read_word ) {	// single word
                r_read_to_tgt_rsp_val[i] = (i == m_x[m_cmd_read_addr_fifo.read()]);
              } else {			// cache line
                r_read_to_tgt_rsp_val[i] = true;
              }
            }
            cmd_read_fifo_get 		= true;
            r_read_to_tgt_rsp_req		= true;
            r_read_to_tgt_rsp_srcid	= m_cmd_read_srcid_fifo.read();
            r_read_to_tgt_rsp_trdid	= m_cmd_read_trdid_fifo.read();
            r_read_to_tgt_rsp_pktid	= m_cmd_read_pktid_fifo.read();
            r_read_fsm 			= READ_IDLE;  
          }
          break;
        }
        ///////////////////
      case READ_TRT_LOCK:	// read miss : check the Transaction Table
        {
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_READ ) {
            size_t index = 0;
            bool   hit = m_transaction_tab.hit_read(m_nline[m_cmd_read_addr_fifo.read()], index);
            bool   wok = !m_transaction_tab.full(index);
            if( hit || !wok ) {  // missing line already requested or no space
              r_read_fsm = READ_IDLE;
            } else {			   // missing line is requested to the XRAM
              r_read_trt_index = index;
              r_read_fsm       = READ_TRT_SET;
            }
          }
          break;
        }
        //////////////////
      case READ_TRT_SET:
        {
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_READ ) {
            m_transaction_tab.set(r_read_trt_index.read(),
                true,
                m_nline[m_cmd_read_addr_fifo.read()],
                m_cmd_read_srcid_fifo.read(),
                m_cmd_read_trdid_fifo.read(),
                m_cmd_read_pktid_fifo.read(),
                true,
                m_cmd_read_word_fifo.read(),
                m_x[m_cmd_read_addr_fifo.read()],
                std::vector<be_t>(m_words,0),
                std::vector<data_t>(m_words,0));
            r_read_fsm 	 = READ_XRAM_REQ;
          }
          break;
        }
        /////////////////////
      case READ_XRAM_REQ:
        {
          if( !r_read_to_xram_cmd_req ) {
            cmd_read_fifo_get		      = true;
            r_read_to_xram_cmd_req  	= true;
            r_read_to_xram_cmd_nline 	= m_nline[m_cmd_read_addr_fifo.read()];
            r_read_to_xram_cmd_trdid 	= r_read_trt_index.read();
            r_read_fsm 			          = READ_IDLE;
          }
          break;
        }
    } // end switch read_fsm

    ///////////////////////////////////////////////////////////////////////////////////
    //		WRITE FSM
    ///////////////////////////////////////////////////////////////////////////////////
    // The WRITE FSM handles the write bursts sent by the processors.
    // All addresses in a burst must be in the same cache line.
    // A complete write burst is consumed in the FIFO & copied to a local buffer.
    // Then the FSM takes the lock protecting the cache directory, to check
    // if the line is in the cache.
    //
    // - In case of HIT, the cache is updated.
    //   If there is no other copy, an acknowledge response is immediately
    //   returned to the writing processor.
    //   if the data is cached by other processoris, the FSM takes the lock
    //   protecting the Update Table (UPT) to register this update transaction.
    //   If the UPT is full, it releases the lock  and waits. Then, it sends 
    //   a multi-update request to all owners of the line (but the writer), 
    //   through the INIT_CMD FSM. In case of multi-update transaction, the WRITE FSM 
    //   does not respond to the writing processor, as this response will be sent by 
    //   the INIT_RSP FSM when all update responses have been received.
    //
    // - In case of MISS, the WRITE FSM takes the lock protecting the transaction
    //   table (TRT). If a read transaction to the XRAM for this line already exists, 
    //   it writes in the TRT (write buffer). Otherwise, if a TRT entry is free, 
    //   the WRITE FSM register a new transaction in TRT, and sends a read line request 
    //   to the XRAM. If the TRT is full, it releases the lock, and waits.
    //   Finally, the WRITE FSM returns an aknowledge response to the writing processor.
    /////////////////////////////////////////////////////////////////////////////////////

    switch ( r_write_fsm.read() ) {

      ////////////////
      case WRITE_IDLE:	// copy first word of a write burst in local buffer	
        {
          if ( m_cmd_write_addr_fifo.rok()) {
            m_cpt_write++;
            m_cpt_write_cells++;
            // consume a word in the FIFO & write it in the local buffer 
            cmd_write_fifo_get	= true;
            size_t index 		    = m_x[m_cmd_write_addr_fifo.read()];
            r_write_address	    = m_cmd_write_addr_fifo.read();
            r_write_word_index	= index;
            r_write_word_count	= 1;
            r_write_data[index]	= m_cmd_write_data_fifo.read();
            r_write_srcid		    = m_cmd_write_srcid_fifo.read();
            r_write_trdid		    = m_cmd_write_trdid_fifo.read();
            r_write_pktid		    = m_cmd_write_pktid_fifo.read();

            // the be field must be set for all words 
            for ( size_t i=0 ; i<m_words ; i++ ) {
              if ( i == index ) 	r_write_be[i] = m_cmd_write_be_fifo.read();
              else		r_write_be[i] = 0x0;
            }
            if( !((m_cmd_write_be_fifo.read() == 0x0)||(m_cmd_write_be_fifo.read() == 0xF)) )
              r_write_byte=true;
            else  r_write_byte=false;

            if( m_cmd_write_eop_fifo.read() )  r_write_fsm = WRITE_DIR_LOCK;
            else                               r_write_fsm = WRITE_NEXT;
          }
          break;
        }
        ////////////////
      case WRITE_NEXT:	// copy next word of a write burst in local buffer
        {
          if ( m_cmd_write_addr_fifo.rok() ) {
            m_cpt_write_cells++;

            // check that the next word is in the same cache line
            assert( (m_nline[r_write_address.read()] == m_nline[m_cmd_write_addr_fifo.read()])  
                && "VCI_MEM_CACHE write error in vci_mem_cache : write burst over a line" );
            // consume a word in the FIFO & write it in the local buffer 
            cmd_write_fifo_get=true;
            size_t index 		= r_write_word_index.read() + r_write_word_count.read();
            r_write_be[index]   	= m_cmd_write_be_fifo.read();
            r_write_data[index] 	= m_cmd_write_data_fifo.read();
            r_write_word_count 	  = r_write_word_count.read() + 1;
            if( !((m_cmd_write_be_fifo.read() == 0x0)||(m_cmd_write_be_fifo.read() == 0xF)) )
              r_write_byte=true;
            if ( m_cmd_write_eop_fifo.read() )  r_write_fsm = WRITE_DIR_LOCK;
          }
          break;
        }
        ////////////////////
      case WRITE_DIR_LOCK:	// access directory to check hit/miss
        {
          if ( r_alloc_dir_fsm.read() == ALLOC_DIR_WRITE ) {
            size_t  way = 0;
            DirectoryEntry entry(m_cache_directory.read(r_write_address.read(), way));

            // copy directory entry in local buffers in case of hit
            if ( entry.valid )  {	
              r_write_is_cnt     = entry.is_cnt;
              r_write_lock	     = entry.lock;
              r_write_tag        = entry.tag;
              r_write_d_copies   = entry.d_copies;
              r_write_i_copies   = entry.i_copies;
              r_write_count      = entry.count;
              r_write_way  	     = way;
              if(entry.is_cnt){
                r_write_fsm      = WRITE_DIR_HIT_READ;
              } else {
                if(r_write_byte.read())
                  r_write_fsm      = WRITE_DIR_HIT_READ;
                else  r_write_fsm  = WRITE_DIR_HIT;
              }
            } else {
              r_write_fsm		     = WRITE_TRT_LOCK;
              m_cpt_write_miss++;
            }
          }
          break;
        }
        ///////////////////
      case WRITE_DIR_HIT_READ:	// read the cache and complete the buffer (data, when be!=0xF)
        {
          // update local buffer
          size_t set	= m_y[r_write_address.read()];
          size_t way	= r_write_way.read();
          for(size_t i=0 ; i<m_words ; i++) {
            data_t mask      = 0;
            if  (r_write_be[i].read() & 0x1) mask = mask | 0x000000FF;
            if  (r_write_be[i].read() & 0x2) mask = mask | 0x0000FF00;
            if  (r_write_be[i].read() & 0x4) mask = mask | 0x00FF0000;
            if  (r_write_be[i].read() & 0x8) mask = mask | 0xFF000000;
            if(r_write_be[i].read()||r_write_is_cnt.read()) { // complete only if mask is not null (for energy consumption)
              r_write_data[i]  = (r_write_data[i].read() & mask) | 
                (m_cache_data[way][set][i] & ~mask);
              r_write_be[i]=0xF;
            }
          } // end for

          if(r_write_is_cnt.read()){
            r_write_fsm            = WRITE_TRT_WRITE_LOCK;
          } else {
            r_write_fsm            = WRITE_DIR_HIT;
          }
          break;
        }
        ///////////////////
      case WRITE_DIR_HIT:	// update the cache (data & dirty bit)
        {
          copy_t sub_mask=0x1;
          size_t sub_count=0;
          for(size_t i=0; i<32;i++){
            if(r_write_i_copies.read()&sub_mask)
              sub_count++;
            sub_mask = sub_mask << 1;
          }

          // update directory with Dirty bit
          DirectoryEntry entry;
          entry.valid	   = true;
          entry.dirty	   = true;
          entry.tag	     = r_write_tag.read();
          entry.is_cnt   = r_write_is_cnt.read();
          entry.lock	   = r_write_lock.read();
          entry.d_copies = r_write_d_copies.read();
          entry.i_copies = 0;
          entry.count    = r_write_count.read()-sub_count;
          size_t set	   = m_y[r_write_address.read()];
          size_t way	   = r_write_way.read();
          m_cache_directory.write(set, way, entry);

          // write data in cache
          for(size_t i=0 ; i<m_words ; i++) {
            if  ( r_write_be[i].read() ) {
              m_cache_data[way][set][i]  = r_write_data[i].read();
            }
          } // end for

          // compute the actual number of copies & the modified bit vector
          bool owner = false;
          copy_t  mask    = 0x1;
          copy_t  d_copies  = r_write_d_copies.read();
          for ( size_t i=0 ; i<32 ; i++) {
            if ( i == r_write_srcid.read() ) {
              if(d_copies & mask)
                owner = true;
              d_copies = d_copies &  ~mask;
            }
            mask = (mask << 1);
          }
          r_write_d_copies    = d_copies;
          size_t count_signal = r_write_count.read();
          if(owner){
            count_signal      = count_signal - 1;
          }
          r_write_count = count_signal;

          if ( count_signal == 0 )   r_write_fsm = WRITE_RSP;
          else                       r_write_fsm = WRITE_UPT_LOCK;
          break;
        }
        /////////////////////
      case WRITE_UPT_LOCK: 	// Try to register the request in Update Table
        {

          if ( r_alloc_upt_fsm.read() == ALLOC_UPT_WRITE ) {
            bool          wok        = false;
            size_t        index      = 0;
            size_t        srcid      = r_write_srcid.read();
            size_t        trdid      = r_write_trdid.read();
            size_t        pktid      = r_write_pktid.read();
            size_t	      nline      = m_nline[r_write_address.read()];
            size_t        nb_copies  = r_write_count.read();

            wok =m_update_tab.set(true,	// it's an update transaction
                false,                  // it's not a broadcast
                true,                   // it needs a response
                srcid,
                trdid,
                pktid,
                nline,
                nb_copies,
                index);
            if(wok){
//              std::cout << "WRITE : record update, time = " << std::dec << m_cpt_cycles << std::endl;
//              m_update_tab.print(); 
            }
            r_write_upt_index = index;
            //  releases the lock protecting Update Table if no entry...
            if ( wok ) r_write_fsm = WRITE_UPDATE;
            else       r_write_fsm = WRITE_WAIT_UPT;
          }
          break;
        }
        ////////////////////
      case WRITE_WAIT_UPT:	// release the lock protecting UPT
        {

          r_write_fsm = WRITE_UPT_LOCK;
          break;
        }
        //////////////////
      case WRITE_UPDATE:	// send a multi-update request to INIT_CMD fsm
        {

          if ( !r_write_to_init_cmd_req ) {
            r_write_to_init_cmd_req      = true;
            r_write_to_init_cmd_brdcast  = false;
            r_write_to_init_cmd_trdid    = r_write_upt_index.read();
            r_write_to_init_cmd_nline    = m_nline[r_write_address.read()];
            r_write_to_init_cmd_index    = r_write_word_index.read();
            r_write_to_init_cmd_count    = r_write_word_count.read();
            r_write_to_init_cmd_d_copies = r_write_d_copies.read();
            r_write_to_init_cmd_i_copies = r_write_i_copies.read();

            for(size_t i=0; i<m_words ; i++){
              assert( ((r_write_be[i].read() == 0xF)||(r_write_be[i].read() == 0x0)) &&
                  "VCI_MEM_CACHE write error in vci_mem_cache : invalid BE");
              if(r_write_be[i].read())  r_write_to_init_cmd_we[i]=true;
              else                      r_write_to_init_cmd_we[i]=false;
            }

            size_t min = r_write_word_index.read();
            size_t max = r_write_word_index.read() + r_write_word_count.read();
            for (size_t i=min ; i<max ; i++) {
              r_write_to_init_cmd_data[i] = r_write_data[i];
            }
            r_write_fsm = WRITE_IDLE; // Response will be sent after receiving
            // all update responses
          }
          break;
        }
        ///////////////
      case WRITE_RSP:		// send a request to TGT_RSP FSM to acknowledge the write
        {
          if ( !r_write_to_tgt_rsp_req.read() ) {
            r_write_to_tgt_rsp_req	= true;
            r_write_to_tgt_rsp_srcid	= r_write_srcid.read();
            r_write_to_tgt_rsp_trdid	= r_write_trdid.read();
            r_write_to_tgt_rsp_pktid	= r_write_pktid.read();
            r_write_fsm 		= WRITE_IDLE;
          }
          break;
        }
        ////////////////////
      case WRITE_TRT_LOCK:	// Miss : check Transaction Table
        {
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_WRITE ) {
            size_t hit_index = 0;
            size_t wok_index = 0;
            bool hit = m_transaction_tab.hit_read(m_nline[r_write_address.read()],hit_index);
            bool wok = !m_transaction_tab.full(wok_index);
            if ( hit ) {		// register the modified data in TRT 
              r_write_trt_index = hit_index;
              r_write_fsm       = WRITE_TRT_DATA;
            } else if ( wok ) {	// set a new entry in TRT
              r_write_trt_index = wok_index;
              r_write_fsm       = WRITE_TRT_SET;
            } else {		// wait an empty entry in TRT
              r_write_fsm       = WRITE_WAIT_TRT;
            }
          }
          break;
        }
        ////////////////////
      case WRITE_WAIT_TRT:	// release the lock protecting TRT
        { 
          r_write_fsm = WRITE_DIR_LOCK;
          break;
        }
        ///////////////////
      case WRITE_TRT_SET:	// register a new transaction in TRT (Write Buffer)
        {  
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_WRITE ) 
          {
            std::vector<be_t> be_vector;
            std::vector<data_t> data_vector;
            be_vector.clear();
            data_vector.clear();
            for ( size_t i=0; i<m_words; i++ ) 
            {
              be_vector.push_back(r_write_be[i]);
              data_vector.push_back(r_write_data[i]);
            }
            m_transaction_tab.set(r_write_trt_index.read(),
                true,				// read request to XRAM
                m_nline[r_write_address.read()],
                r_write_srcid.read(),
                r_write_trdid.read(),
                r_write_pktid.read(),
                false,				// not a processor read
                false,				// not a single word 
                0,			        	// word index
                be_vector,
                data_vector);

            r_write_fsm = WRITE_XRAM_REQ;
          }
          break;
        }  
        ///////////////////
      case WRITE_TRT_DATA:	// update an entry in TRT (Write Buffer)
        { 
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_WRITE ) {
            std::vector<be_t> be_vector;
            std::vector<data_t> data_vector;
            be_vector.clear();
            data_vector.clear();
            for ( size_t i=0; i<m_words; i++ ) {
              be_vector.push_back(r_write_be[i]);
              data_vector.push_back(r_write_data[i]);
            }
            m_transaction_tab.write_data_mask(r_write_trt_index.read(),
                be_vector,
                data_vector);
            r_write_fsm = WRITE_RSP;
          }
          break;
        }
        ////////////////////
      case WRITE_XRAM_REQ:	// send a request to XRAM_CMD FSM
        {  

          if ( !r_write_to_xram_cmd_req ) {
            r_write_to_xram_cmd_req     = true;
            r_write_to_xram_cmd_write   = false;
            r_write_to_xram_cmd_nline   = m_nline[r_write_address.read()];
            r_write_to_xram_cmd_trdid   = r_write_trt_index.read();
            r_write_fsm                 = WRITE_RSP;
          }
          break;
        }
        ////////////////////
      case WRITE_TRT_WRITE_LOCK:
        {
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_WRITE ) {
            size_t wok_index = 0;
            bool wok = !m_transaction_tab.full(wok_index);
            if ( wok ) {	// set a new entry in TRT
              r_write_trt_index = wok_index;
              r_write_fsm       = WRITE_INVAL_LOCK;
            } else {		// wait an empty entry in TRT
              r_write_fsm       = WRITE_WAIT_TRT;
            }
          }

          break;
        }
        ////////////////////
      case WRITE_INVAL_LOCK:
        {
          if ( r_alloc_upt_fsm.read() == ALLOC_UPT_WRITE ) {
            bool          wok        = false;
            size_t        index      = 0;
            size_t        srcid      = r_write_srcid.read();
            size_t        trdid      = r_write_trdid.read();
            size_t        pktid      = r_write_pktid.read();
            size_t	      nline      = m_nline[r_write_address.read()];
            size_t        nb_copies  = r_write_count.read();

            wok =m_update_tab.set(false,	// it's an inval transaction
                true,                     // it's a broadcast
                true,                     // it needs a response
                srcid,
                trdid,
                pktid,
                nline,
                nb_copies,
                index);
            if(wok){
//              std::cout << "WRITE : record invalidate, time = " << std::dec << m_cpt_cycles << std::endl;
//              m_update_tab.print(); 
            }
            r_write_upt_index = index;
            //  releases the lock protecting Update Table if no entry...
            if ( wok ) r_write_fsm = WRITE_DIR_INVAL;
            else       r_write_fsm = WRITE_WAIT_TRT;
          }

          break;
        }
        ////////////////////
      case WRITE_DIR_INVAL:
        {
          if ( (r_alloc_trt_fsm.read() == ALLOC_TRT_WRITE ) ||
              (r_alloc_upt_fsm.read() == ALLOC_UPT_WRITE ) )
          {
            std::vector<be_t> be_vector;
            std::vector<data_t> data_vector;
            be_vector.clear();
            data_vector.clear();
            for ( size_t i=0; i<m_words; i++ ) 
            {
              be_vector.push_back(r_write_be[i]);
              data_vector.push_back(r_write_data[i]);
            }
            m_transaction_tab.set(r_write_trt_index.read(),
                false,				// write request to XRAM
                m_nline[r_write_address.read()],
                0,
                0,
                0,
                false,				// not a processor read
                false,				// not a single word 
                0,			        	// word index
                be_vector,
                data_vector);
            // invalidate directory entry
            DirectoryEntry entry;
            entry.valid	   = false;
            entry.dirty	   = false;
            entry.tag	     = 0;
            entry.is_cnt   = false;
            entry.lock	   = false;
            entry.d_copies = 0;
            entry.i_copies = 0;
            entry.count    = 0;
            size_t set	   = m_y[r_write_address.read()];
            size_t way	   = r_write_way.read();
            m_cache_directory.write(set, way, entry);

            r_write_fsm = WRITE_INVAL;
          } else {
            assert(false && "LOCK ERROR in WRITE_FSM, STATE = WRITE_DIR_INVAL");
          }

          break;
        }
        ////////////////////
      case WRITE_INVAL:
        {
          if ( !r_write_to_init_cmd_req ) {
            r_write_to_init_cmd_req      = true;
            r_write_to_init_cmd_brdcast  = true;
            r_write_to_init_cmd_trdid    = r_write_upt_index.read();
            r_write_to_init_cmd_nline    = m_nline[r_write_address.read()];
            r_write_to_init_cmd_index    = 0;
            r_write_to_init_cmd_count    = 0;
            r_write_to_init_cmd_d_copies = 0;
            r_write_to_init_cmd_i_copies = 0;

            for(size_t i=0; i<m_words ; i++){
              r_write_to_init_cmd_we[i]=false;
              r_write_to_init_cmd_data[i] = 0;
            }
            r_write_fsm = WRITE_XRAM_SEND;
            // all update responses
          }

          break;
        }
        ////////////////////
      case WRITE_XRAM_SEND:
        {
          if ( !r_write_to_xram_cmd_req ) {
            r_write_to_xram_cmd_req     = true;
            r_write_to_xram_cmd_write   = true;
            r_write_to_xram_cmd_nline   = m_nline[r_write_address.read()];
            r_write_to_xram_cmd_trdid   = r_write_trt_index.read();
            for(size_t i=0; i<m_words; i++){
              r_write_to_xram_cmd_data[i] = r_write_data[i];
            }
            r_write_fsm                 = WRITE_IDLE;
          }
          break;
        }
    } // end switch r_write_fsm

    ///////////////////////////////////////////////////////////////////////
    //		XRAM_CMD FSM
    ///////////////////////////////////////////////////////////////////////
    // The XRAM_CMD fsm controls the command packets to the XRAM :
    // - It sends a single cell VCI read to the XRAM in case of MISS request
    // posted by the READ, WRITE or LLSC FSMs : the TRDID field contains 
    // the Transaction Tab index.
    // The VCI response is a multi-cell packet : the N cells contain
    // the N data words.
    // - It sends a multi-cell VCI write when the XRAM_RSP FSM request 
    // to save a dirty line to the XRAM. 
    // The VCI response is a single cell packet.
    // This FSM handles requests from the READ, WRITE, LLSC & XRAM_RSP FSMs 
    // with a round-robin priority.
    ////////////////////////////////////////////////////////////////////////

    switch ( r_xram_cmd_fsm.read() ) {
      //////////////////////// 
      case XRAM_CMD_READ_IDLE:
        if      ( r_write_to_xram_cmd_req )     r_xram_cmd_fsm = XRAM_CMD_WRITE_NLINE;
        else if ( r_llsc_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_LLSC_NLINE;
        else if ( r_xram_rsp_to_xram_cmd_req  ) r_xram_cmd_fsm = XRAM_CMD_XRAM_DATA;
        else if ( r_read_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_READ_NLINE;
        break;
        //////////////////////// 
      case XRAM_CMD_WRITE_IDLE:
        if      ( r_llsc_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_LLSC_NLINE;
        else if ( r_xram_rsp_to_xram_cmd_req  ) r_xram_cmd_fsm = XRAM_CMD_XRAM_DATA;
        else if ( r_read_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_READ_NLINE;
        else if ( r_write_to_xram_cmd_req )     r_xram_cmd_fsm = XRAM_CMD_WRITE_NLINE;
        break;
        //////////////////////// 
      case XRAM_CMD_LLSC_IDLE:
        if      ( r_xram_rsp_to_xram_cmd_req  ) r_xram_cmd_fsm = XRAM_CMD_XRAM_DATA;
        else if ( r_read_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_READ_NLINE;
        else if ( r_write_to_xram_cmd_req )     r_xram_cmd_fsm = XRAM_CMD_WRITE_NLINE;
        else if ( r_llsc_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_LLSC_NLINE;
        break;
        //////////////////////// 
      case XRAM_CMD_XRAM_IDLE:
        if      ( r_read_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_READ_NLINE;
        else if ( r_write_to_xram_cmd_req )     r_xram_cmd_fsm = XRAM_CMD_WRITE_NLINE;
        else if ( r_llsc_to_xram_cmd_req  )     r_xram_cmd_fsm = XRAM_CMD_LLSC_NLINE;
        else if ( r_xram_rsp_to_xram_cmd_req  ) r_xram_cmd_fsm = XRAM_CMD_XRAM_DATA;
        break;
        /////////////////////////
      case XRAM_CMD_READ_NLINE:
        if ( p_vci_ixr.cmdack ) {
          r_xram_cmd_fsm = XRAM_CMD_READ_IDLE;		
          r_read_to_xram_cmd_req = false;
        }
        break;
        //////////////////////////
      case XRAM_CMD_WRITE_NLINE:
        if ( p_vci_ixr.cmdack ) {
          if( r_write_to_xram_cmd_write.read()){
            if ( r_xram_cmd_cpt.read() == (m_words - 1) ) {
              r_xram_cmd_cpt = 0;
              r_xram_cmd_fsm = XRAM_CMD_WRITE_IDLE;
              r_write_to_xram_cmd_req = false;
            } else {
              r_xram_cmd_cpt = r_xram_cmd_cpt + 1;
            }
          } else {
            r_xram_cmd_fsm = XRAM_CMD_WRITE_IDLE;		
            r_write_to_xram_cmd_req = false;
          }
        }
        break;
        /////////////////////////
      case XRAM_CMD_LLSC_NLINE:
        if ( p_vci_ixr.cmdack ) {
          r_xram_cmd_fsm = XRAM_CMD_LLSC_IDLE;		
          r_llsc_to_xram_cmd_req = false;
        }
        break;
        ////////////////////////
      case XRAM_CMD_XRAM_DATA:
        if ( p_vci_ixr.cmdack ) {
          if ( r_xram_cmd_cpt.read() == (m_words - 1) ) {
            r_xram_cmd_cpt = 0;
            r_xram_cmd_fsm = XRAM_CMD_XRAM_IDLE;
            r_xram_rsp_to_xram_cmd_req = false;
          } else {
            r_xram_cmd_cpt = r_xram_cmd_cpt + 1;
          }
        }
        break;

    } // end switch r_xram_cmd_fsm

    ////////////////////////////////////////////////////////////////////////////
    //                IXR_RSP FSM
    ////////////////////////////////////////////////////////////////////////////
    // The IXR_RSP FSM receives the response packets from the XRAM,
    // for both write transaction, and read transaction.
    //
    // - A response to a write request is a single-cell VCI packet.
    // The Transaction Tab index is contained in the RTRDID field.
    // The FSM takes the lock protecting the TRT, and the corresponding
    // entry is erased.
    //	
    // - A response to a read request is a multi-cell VCI packet.
    // The Transaction Tab index is contained in the RTRDID field.
    // The N cells contain the N words of the cache line in the RDATA field.
    // The FSM takes the lock protecting the TRT to store the line in the TRT
    // (taking into account the write requests already stored in the TRT).
    // When the line is completely written, the corresponding rok signal is set.
    ///////////////////////////////////////////////////////////////////////////////

    switch ( r_ixr_rsp_fsm.read() ) {

      ///////////////////
      case IXR_RSP_IDLE:	// test if it's a read or a write transaction
        {
          if ( p_vci_ixr.rspval ) {
            r_ixr_rsp_cpt   = 0;
            r_ixr_rsp_trt_index = p_vci_ixr.rtrdid.read();
            if ( p_vci_ixr.reop )  r_ixr_rsp_fsm = IXR_RSP_ACK;
            else                   r_ixr_rsp_fsm = IXR_RSP_TRT_READ;
          }
          break;  
        }
        ////////////////////////
      case IXR_RSP_ACK:        // Acknowledge the vci response
        r_ixr_rsp_fsm = IXR_RSP_TRT_ERASE;
        break;
        ////////////////////////
      case IXR_RSP_TRT_ERASE: 	// erase the entry in the TRT
        {
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_IXR_RSP ) {
            m_transaction_tab.erase(r_ixr_rsp_trt_index.read());
            r_ixr_rsp_fsm = IXR_RSP_IDLE;
          }
          break;
        }
        ///////////////////////
      case IXR_RSP_TRT_READ:		// write data in the TRT
        {
          if ( (r_alloc_trt_fsm.read() == ALLOC_TRT_IXR_RSP) &&  p_vci_ixr.rspval ) {
            bool   eop  	= p_vci_ixr.reop.read();
            data_t data 	= p_vci_ixr.rdata.read();
            size_t index        = r_ixr_rsp_trt_index.read();
            assert( eop == (r_ixr_rsp_cpt.read() == (m_words-1)) 
                && "Error in VCI_MEM_CACHE : invalid length for a response from XRAM");
            m_transaction_tab.write_rsp(index, r_ixr_rsp_cpt.read(), data);
            r_ixr_rsp_cpt = r_ixr_rsp_cpt.read() + 1;
            if ( eop ) {
              r_ixr_rsp_to_xram_rsp_rok[r_ixr_rsp_trt_index.read()]=true;
              r_ixr_rsp_fsm = IXR_RSP_IDLE;
            }
          }
          break;
        }
    } // end swich r_ixr_rsp_fsm


    ////////////////////////////////////////////////////////////////////////////
    //                XRAM_RSP FSM
    ////////////////////////////////////////////////////////////////////////////
    // The XRAM_RSP FSM handles the incoming cache lines from the XRAM.
    // The cache line has been written in the TRT buffer by the IXR_FSM.
    //
    // When a response is available, the corresponding TRT entry 
    // is copied in a local buffer to be written in the cache. 
    // Then, the FSM releases the lock protecting the TRT, and takes the lock 
    // protecting the cache directory.
    // It selects a cache slot and writes the line in the cache.
    // If it was a read MISS, the XRAM_RSP FSM send a request to the TGT_RSP
    // FSM to return the cache line to the registered processor.
    // If there is no empty slot, a victim line is evicted, and
    // invalidate requests are sent to the L1 caches containing copies.
    // If this line is dirty, the XRAM_RSP FSM send a request to the XRAM_CMD 
    // FSM to save the victim line to the XRAM, and register the write transaction
    // in the TRT (using the entry previously used by the read transaction).
    ///////////////////////////////////////////////////////////////////////////////

    switch ( r_xram_rsp_fsm.read() ) {

      ///////////////////
      case XRAM_RSP_IDLE:	// test if there is a response with a round robin priority
        {
          size_t ptr   = r_xram_rsp_trt_index.read();
          size_t lines = TRANSACTION_TAB_LINES;
          for(size_t i=0; i<lines; i++){
            size_t index=(i+ptr+1)%lines;
            if(r_ixr_rsp_to_xram_rsp_rok[index]){
              r_xram_rsp_trt_index=index;
              r_ixr_rsp_to_xram_rsp_rok[index]=false;
              r_xram_rsp_fsm           = XRAM_RSP_DIR_LOCK;
              break;
            }
          }
          break;  
        }
        ///////////////////////
      case XRAM_RSP_DIR_LOCK: 	// Take the lock on the directory
        {
          if( r_alloc_dir_fsm.read() == ALLOC_DIR_XRAM_RSP ) {
            r_xram_rsp_fsm           = XRAM_RSP_TRT_COPY;
          }
          break;
        }
        ///////////////////////
      case XRAM_RSP_TRT_COPY:		// Copy the TRT entry in the local buffer and eviction of a cache line
        {
          if ( (r_alloc_trt_fsm.read() == ALLOC_TRT_XRAM_RSP) ) {
            size_t index = r_xram_rsp_trt_index.read();
            TransactionTabEntry    trt_entry(m_transaction_tab.read(index));	

            r_xram_rsp_trt_buf.copy(trt_entry);  // TRT entry local buffer

            // selects & extracts a victim line from cache
            size_t way = 0;
            size_t set = m_y[trt_entry.nline * m_words * 4];
            DirectoryEntry victim(m_cache_directory.select(set, way));

            for (size_t i=0 ; i<m_words ; i++) r_xram_rsp_victim_data[i] = m_cache_data[way][set][i];

            r_xram_rsp_victim_d_copies = victim.d_copies;
            r_xram_rsp_victim_i_copies = victim.i_copies;
            r_xram_rsp_victim_count    = victim.count;
            r_xram_rsp_victim_way      = way;
            r_xram_rsp_victim_set      = set;
            r_xram_rsp_victim_nline    = victim.tag*m_sets + set;
            r_xram_rsp_victim_is_cnt   = victim.is_cnt;
            r_xram_rsp_victim_inval    = victim.valid && 
              ((victim.d_copies != 0) || 
               (victim.i_copies != 0)   );
            r_xram_rsp_victim_dirty    = victim.dirty;

            r_xram_rsp_fsm = XRAM_RSP_INVAL_LOCK;
          }
          break;
        }
        ///////////////////////
      case XRAM_RSP_INVAL_LOCK:
        {
          if ( r_alloc_upt_fsm == ALLOC_UPT_XRAM_RSP ) {
            size_t index;
            if(m_update_tab.search_brdcast(r_xram_rsp_trt_buf.nline, index)){
              r_xram_rsp_fsm = XRAM_RSP_INVAL_WAIT;
            }
            else {
              r_xram_rsp_fsm = XRAM_RSP_DIR_UPDT;
            }
          }
          break;
        }
        ///////////////////////
      case XRAM_RSP_INVAL_WAIT:
        {
          r_xram_rsp_fsm = XRAM_RSP_DIR_LOCK;
          break;
        }
        ///////////////////////
      case XRAM_RSP_DIR_UPDT:		// updates the cache (both data & directory)
        {
          // signals generation
          bool inst_read = (r_xram_rsp_trt_buf.trdid & 0x2) && r_xram_rsp_trt_buf.proc_read; // It is an instruction read
          bool cached_read = (r_xram_rsp_trt_buf.trdid & 0x1) && r_xram_rsp_trt_buf.proc_read ;
          // update data
          size_t set   = r_xram_rsp_victim_set.read();
          size_t way   = r_xram_rsp_victim_way.read();
          for(size_t i=0; i<m_words ; i++){
            m_cache_data[way][set][i] = r_xram_rsp_trt_buf.wdata[i];
          }
          // compute dirty 
          bool dirty = false;
          for(size_t i=0; i<m_words;i++){
            dirty = dirty || (r_xram_rsp_trt_buf.wdata_be[i] != 0);
          }

          // update directory
          DirectoryEntry entry;
          entry.valid	  = true;
          entry.is_cnt  = false;
          entry.lock	  = false;
          entry.dirty	  = dirty;
          entry.tag	    = r_xram_rsp_trt_buf.nline / m_sets;
          if(cached_read) {
            if(inst_read) {
              entry.i_copies = 0x1 << r_xram_rsp_trt_buf.srcid;
              entry.d_copies = 0x0;
              entry.count    = 1;
            } else {
              entry.i_copies = 0x0;
              entry.d_copies = 0x1 << r_xram_rsp_trt_buf.srcid;
              entry.count    = 1;
            }
          } else {
            entry.d_copies = 0;
            entry.i_copies = 0;
            entry.count    = 0;
          }
          m_cache_directory.write(set, way, entry);
          // If the victim is not dirty, we erase the entry in the TRT
          if      (!r_xram_rsp_victim_dirty.read()) m_transaction_tab.erase(r_xram_rsp_trt_index.read());
          // Next state
          if      ( r_xram_rsp_victim_dirty.read())       r_xram_rsp_fsm = XRAM_RSP_TRT_DIRTY;
          else if ( r_xram_rsp_trt_buf.proc_read  )       r_xram_rsp_fsm = XRAM_RSP_DIR_RSP;
          else if ( r_xram_rsp_victim_inval.read())       r_xram_rsp_fsm = XRAM_RSP_UPT_LOCK;
          else                                            r_xram_rsp_fsm = XRAM_RSP_IDLE;
          break;
        }
        ////////////////////////
      case XRAM_RSP_TRT_DIRTY:		// set the TRT entry (write line to XRAM) if the victim is dirty
        {
          if ( r_alloc_trt_fsm.read() == ALLOC_TRT_XRAM_RSP ) {
            m_transaction_tab.set(r_xram_rsp_trt_index.read(),
                false,				// write to XRAM
                r_xram_rsp_victim_nline.read(), // line index
                0,
                0,
                0,
                false,
                false,
                0,
                std::vector<be_t>(m_words,0),
                std::vector<data_t>(m_words,0) );
            if      ( r_xram_rsp_trt_buf.proc_read  )       r_xram_rsp_fsm = XRAM_RSP_DIR_RSP;
            else if ( r_xram_rsp_victim_inval.read())       r_xram_rsp_fsm = XRAM_RSP_UPT_LOCK;
            else                                            r_xram_rsp_fsm = XRAM_RSP_WRITE_DIRTY;
          }
          break;
        }
        //////////////////////
      case XRAM_RSP_DIR_RSP:     // send a request to TGT_RSP FSM in case of read
        {
          if ( !r_xram_rsp_to_tgt_rsp_req ) {
            r_xram_rsp_to_tgt_rsp_srcid = r_xram_rsp_trt_buf.srcid;
            r_xram_rsp_to_tgt_rsp_trdid = r_xram_rsp_trt_buf.trdid;
            r_xram_rsp_to_tgt_rsp_pktid = r_xram_rsp_trt_buf.pktid;
            for (size_t i=0; i < m_words; i++) {
              r_xram_rsp_to_tgt_rsp_data[i] = r_xram_rsp_trt_buf.wdata[i];
              if( r_xram_rsp_trt_buf.single_word ) {
                r_xram_rsp_to_tgt_rsp_val[i] = (r_xram_rsp_trt_buf.word_index == i);
              } else {
                r_xram_rsp_to_tgt_rsp_val[i] = true;
              }
            } 
            r_xram_rsp_to_tgt_rsp_req   = true;

            if      ( r_xram_rsp_victim_inval ) r_xram_rsp_fsm = XRAM_RSP_UPT_LOCK;
            else if ( r_xram_rsp_victim_dirty ) r_xram_rsp_fsm = XRAM_RSP_WRITE_DIRTY; 
            else                                r_xram_rsp_fsm = XRAM_RSP_IDLE;
          }
          break;
        }
        ///////////////////////
      case XRAM_RSP_UPT_LOCK:	// Try to register the inval transaction in UPT
        {
          if ( r_alloc_upt_fsm == ALLOC_UPT_XRAM_RSP ) {
            bool    brdcast = r_xram_rsp_victim_is_cnt.read();
            size_t index;
            size_t count_copies = r_xram_rsp_victim_count.read();

            bool	 wok = m_update_tab.set(false,	// it's an inval transaction
                brdcast,                          // set brdcast bit
                false,  // it does not need a response
                0,
                0,
                0,
                r_xram_rsp_victim_nline.read(),
                count_copies,
                index);
            if(wok) {
//              std::cout << "XRAM_RSP : record invalidation, time = " << std::dec << m_cpt_cycles << std::endl;
//              m_update_tab.print(); 
            }
            if ( wok ) {
              r_xram_rsp_upt_index = index;
              r_xram_rsp_fsm       = XRAM_RSP_INVAL;
            } else {
              r_xram_rsp_fsm       = XRAM_RSP_WAIT;
            }
          }
          break;
        }
        ///////////////////
      case XRAM_RSP_WAIT:	// releases UPT lock for one cycle
        {
          r_xram_rsp_fsm = XRAM_RSP_UPT_LOCK;
          break;
        }
        ////////////////////
      case XRAM_RSP_INVAL:	// send invalidate request to INIT_CMD FSM
        {
          if( !r_xram_rsp_to_init_cmd_req ) {	      
            r_xram_rsp_to_init_cmd_req      = true; 
            r_xram_rsp_to_init_cmd_brdcast  = r_xram_rsp_victim_is_cnt.read();
            r_xram_rsp_to_init_cmd_nline    = r_xram_rsp_victim_nline.read();
            r_xram_rsp_to_init_cmd_trdid    = r_xram_rsp_upt_index;
            r_xram_rsp_to_init_cmd_d_copies = r_xram_rsp_victim_d_copies ;
            r_xram_rsp_to_init_cmd_i_copies = r_xram_rsp_victim_i_copies ;
            if ( r_xram_rsp_victim_dirty ) r_xram_rsp_fsm = XRAM_RSP_WRITE_DIRTY;
            else		                       r_xram_rsp_fsm = XRAM_RSP_IDLE;
          }
          break;
        }
        //////////////////////////
      case XRAM_RSP_WRITE_DIRTY:	// send a write request to XRAM_CMD FSM
        {
          if ( !r_xram_rsp_to_xram_cmd_req ) {
            r_xram_rsp_to_xram_cmd_req = true;
            r_xram_rsp_to_xram_cmd_nline = r_xram_rsp_victim_nline.read();
            r_xram_rsp_to_xram_cmd_trdid = r_xram_rsp_trt_index.read();
            for(size_t i=0; i<m_words ; i++) {
              r_xram_rsp_to_xram_cmd_data[i] = r_xram_rsp_victim_data[i];
            }
            m_cpt_write_dirty++;
            r_xram_rsp_fsm = XRAM_RSP_IDLE;
          }
          break;
        }
    } // end swich r_xram_rsp_fsm

    ////////////////////////////////////////////////////////////////////////////////////
    //		CLEANUP FSM
    ////////////////////////////////////////////////////////////////////////////////////
    // The CLEANUP FSM handles the cleanup request from L1 caches.
    // It accesses the cache directory to update the list of copies.
    //
    // !!!!!!!! The actual cleanup of the cache directory has to be done...
    //
    ////////////////////////////////////////////////////////////////////////////////////
    /*******************/
    /* NEW CLEANUP FSM */
    /*******************/
    switch ( r_cleanup_fsm.read() ) {

      ///////////////////
      case CLEANUP_IDLE:
        {

          if ( p_vci_tgt_cleanup.cmdval.read() ) {
            assert( (p_vci_tgt_cleanup.srcid.read() < m_initiators) &&
                "VCI_MEM_CACHE error in VCI_MEM_CACHE in the CLEANUP network : The received SRCID is larger than 31");
            bool reached = false;
            for ( size_t index = 0 ; index < nseg && !reached ; index++ ){
              if ( m_seg[index]->contains(p_vci_tgt_cleanup.address.read()) ){
                reached = true;
              }
            }
            if ( (p_vci_tgt_cleanup.cmd.read() == vci_param::CMD_WRITE) &&
                (p_vci_tgt_cleanup.address.read() != BROADCAST_ADDR) &&
                reached) {

              m_cpt_cleanup++;

              r_cleanup_nline      = m_nline[p_vci_tgt_cleanup.address.read()] ;
              r_cleanup_srcid      = p_vci_tgt_cleanup.srcid.read();
              r_cleanup_trdid      = p_vci_tgt_cleanup.trdid.read();
              r_cleanup_pktid      = p_vci_tgt_cleanup.pktid.read();

              r_cleanup_fsm        = CLEANUP_DIR_LOCK;
            }
          }
          break;
        }
        //////////////////////
      case CLEANUP_DIR_LOCK:
        {
          if ( r_alloc_dir_fsm.read() == ALLOC_DIR_CLEANUP ) {

            // Read the directory
            size_t way = 0;
#define L2 soclib::common::uint32_log2
            DirectoryEntry entry = m_cache_directory.read(r_cleanup_nline.read() << (L2(m_words) +2) , way);
#undef L2

            r_cleanup_is_cnt    = entry.is_cnt;
            r_cleanup_dirty	    = entry.dirty;
            r_cleanup_tag	      = entry.tag;
            r_cleanup_lock	    = entry.lock;
            r_cleanup_way	      = way;
            r_cleanup_d_copies  = entry.d_copies;
            r_cleanup_i_copies  = entry.i_copies;
            r_cleanup_count     = entry.count;

            // In case of hit, the copy must be cleaned in the copies bit-vector
            if( entry.valid )  { 
              r_cleanup_fsm = CLEANUP_DIR_WRITE;
            } else {
              r_cleanup_fsm = CLEANUP_UPT_LOCK;
            }
          }
          break;
        }
        ///////////////////////
      case CLEANUP_DIR_WRITE:
        {
          size_t way      = r_cleanup_way.read();
#define L2 soclib::common::uint32_log2
          size_t set      = m_y[r_cleanup_nline.read() << (L2(m_words) +2)];
#undef L2
          bool cleanup_inst  = r_cleanup_trdid.read() & 0x1; 

          // update the cache directory (for the copies)
          DirectoryEntry entry;
          entry.valid	  = true;
          entry.is_cnt  = r_cleanup_is_cnt.read();
          entry.dirty	  = r_cleanup_dirty.read();
          entry.tag	    = r_cleanup_tag.read();
          entry.lock	  = r_cleanup_lock.read();
          if(r_cleanup_is_cnt.read()) { // Directory is a counter
            entry.count  = r_cleanup_count.read() -1;
            entry.i_copies = 0;
            entry.d_copies = 0;
            // response to the cache      
            r_cleanup_fsm = CLEANUP_RSP;
          }
          else{                         // Directory is a vector
            if(cleanup_inst){           // Cleanup from a ICACHE
              if(r_cleanup_i_copies.read() & (0x1 << r_cleanup_srcid.read())){ // hit
                entry.count  = r_cleanup_count.read() -1;
                r_cleanup_fsm = CLEANUP_RSP;
              } else { // miss
                entry.count  = r_cleanup_count.read();
                r_cleanup_fsm = CLEANUP_UPT_LOCK;
              }
              entry.i_copies  = r_cleanup_i_copies.read() & ~(0x1 << r_cleanup_srcid.read());
              entry.d_copies  = r_cleanup_d_copies.read();
            } else {                    // Cleanup from a DCACHE
              if(r_cleanup_d_copies.read() & (0x1 << r_cleanup_srcid.read())){ // hit
                entry.count  = r_cleanup_count.read() -1;
                r_cleanup_fsm = CLEANUP_RSP; 
              } else { // miss
                entry.count  = r_cleanup_count.read();
                r_cleanup_fsm = CLEANUP_UPT_LOCK;
              }
              entry.i_copies  = r_cleanup_i_copies.read();
              entry.d_copies  = r_cleanup_d_copies.read() & ~(0x1 << r_cleanup_srcid.read());
            }
          }
          m_cache_directory.write(set, way, entry);  

          // response to the cache      
          r_cleanup_fsm = CLEANUP_RSP;

          break;
        }
        /////////////////
      case CLEANUP_UPT_LOCK:
        {
          if( r_alloc_upt_fsm.read() == ALLOC_UPT_CLEANUP )
          {
            size_t index;
            bool hit_brdcast;
            hit_brdcast = m_update_tab.search_brdcast(r_cleanup_nline.read(),index);
            if(!hit_brdcast) {
              std::cout << "MEM_CACHE WARNING: cleanup with no corresponding entry at address : " << std::hex << (r_cleanup_nline.read()*2*m_words) << std::dec << std::endl;
              r_cleanup_fsm = CLEANUP_RSP;
            } else {
              r_cleanup_write_srcid = m_update_tab.srcid(index);
              r_cleanup_write_trdid = m_update_tab.trdid(index);
              r_cleanup_write_pktid = m_update_tab.pktid(index);
              r_cleanup_need_rsp    = m_update_tab.need_rsp(index);
              r_cleanup_fsm = CLEANUP_UPT_WRITE;
            }
            r_cleanup_index.write(index) ; 
          }
          break;
        }
        /////////////////
      case CLEANUP_UPT_WRITE:
        {
          size_t count = 0;
          m_update_tab.decrement(r_cleanup_index.read(), count); // &count
          if(count == 0){
            m_update_tab.clear(r_cleanup_index.read());
            if(r_cleanup_need_rsp.read()){
              r_cleanup_fsm = CLEANUP_WRITE_RSP ;
            } else {
              r_cleanup_fsm = CLEANUP_RSP;
            }
          } else {
            r_cleanup_fsm = CLEANUP_RSP ;
          }
//          std::cout << "CLEANUP : decrement entry, time = " << std::dec << m_cpt_cycles << std::endl;
//          m_update_tab.print(); 
          break;
        }
        /////////////////
      case CLEANUP_WRITE_RSP:
        {
          if( !r_cleanup_to_tgt_rsp_req.read()) {
            r_cleanup_to_tgt_rsp_req     = true;
            r_cleanup_to_tgt_rsp_srcid   = r_cleanup_write_srcid.read();
            r_cleanup_to_tgt_rsp_trdid   = r_cleanup_write_trdid.read();
            r_cleanup_to_tgt_rsp_pktid   = r_cleanup_write_pktid.read();
            r_cleanup_fsm = CLEANUP_RSP;
          }
          break;
        }
        /////////////////
      case CLEANUP_RSP:
        {
          if(p_vci_tgt_cleanup.rspack)
            r_cleanup_fsm = CLEANUP_IDLE;
          break;
        }
    } // end switch cleanup fsm


    ////////////////////////////////////////////////////////////////////////////////////
    //		LLSC FSM
    ////////////////////////////////////////////////////////////////////////////////////
    // The LLSC FSM handles the LL & SC atomic access.
    //
    // For a LL :
    // It access the directory to check hit / miss.
    // - In case of hit, the LL request is registered in the Atomic Table and the
    // response is sent to the requesting processor.
    // - In case of miss, the LLSC FSM accesses the transaction table. 
    // If a read transaction to the XRAM for this line already exists, 
    // or if the transaction table is full, it returns to IDLE state.
    // Otherwise, a new transaction to the XRAM is initiated.
    // In both cases, the LL request is not consumed in the FIFO.
    //
    // For a SC :
    // It access the directory to check hit / miss.
    // - In case of hit, the Atomic Table is checked and the proper response
    // (true or false is sent to the requesting processor.
    // - In case of miss, the LLSC FSM accesses the transaction table. 
    // If a read transaction to the XRAM for this line already exists, 
    // or if the transaction table is full, it returns to IDLE state. 
    // Otherwise, a new transaction to the XRAM is initiated.
    // In both cases, the SC request is not consumed in the FIFO.
    /////////////////////////////////////////////////////////////////////

    switch ( r_llsc_fsm.read() ) {

      ///////////////
      case LLSC_IDLE:		// test LL / SC
        {
          if( m_cmd_llsc_addr_fifo.rok() ) {
            if(m_cmd_llsc_sc_fifo.read()){
              m_cpt_sc++;
              r_llsc_fsm = SC_DIR_LOCK;
            }
            else{
              m_cpt_ll++;
              r_llsc_fsm = LL_DIR_LOCK;
            }
          }	
          break;
        }
        /////////////////
      case LL_DIR_LOCK:		// check directory for hit / miss
        {
          if( r_alloc_dir_fsm.read() == ALLOC_DIR_LLSC ) {
            size_t way = 0;
            DirectoryEntry entry(m_cache_directory.read(m_cmd_llsc_addr_fifo.read(), way));
            r_llsc_is_cnt     = entry.is_cnt;
            r_llsc_dirty      = entry.dirty;
            r_llsc_tag        = entry.tag;
            r_llsc_way        = way;
            r_llsc_d_copies   = entry.d_copies;
            r_llsc_i_copies   = entry.i_copies ;
            r_llsc_count      = entry.count ;

            if ( entry.valid )  r_llsc_fsm = LL_DIR_HIT;
            else                r_llsc_fsm = LLSC_TRT_LOCK;
          }
          break;
        }
        ////////////////
      case LL_DIR_HIT:		// read hit : update the memory cache
        {
          size_t way	= r_llsc_way.read();
          size_t set	= m_y[m_cmd_llsc_addr_fifo.read()];
          size_t word	= m_x[m_cmd_llsc_addr_fifo.read()];

          // update directory (lock bit & copies)
          DirectoryEntry entry;
          entry.valid	    = true;
          entry.is_cnt    = r_llsc_is_cnt.read();
          entry.dirty	    = r_llsc_dirty.read();
          entry.lock	    = true;
          entry.tag	      = r_llsc_tag.read();
          entry.d_copies  = r_llsc_d_copies.read();
          entry.i_copies  = r_llsc_i_copies.read();
          entry.count     = r_llsc_count.read();
          m_cache_directory.write(set, way, entry);

          // read data in cache
          r_llsc_data	= m_cache_data[way][set][word];

          // set Atomic Table
          m_atomic_tab.set(m_cmd_llsc_srcid_fifo.read(), m_cmd_llsc_addr_fifo.read());

          r_llsc_fsm 	= LL_RSP;
          break;
        }
        ////////////
      case LL_RSP:		// request the TGT_RSP FSM to return data
        {
          if ( !r_llsc_to_tgt_rsp_req ) {
            cmd_llsc_fifo_get		= true;
            r_llsc_to_tgt_rsp_data	= r_llsc_data.read();
            r_llsc_to_tgt_rsp_srcid	= m_cmd_llsc_srcid_fifo.read();
            r_llsc_to_tgt_rsp_trdid	= m_cmd_llsc_trdid_fifo.read();
            r_llsc_to_tgt_rsp_pktid	= m_cmd_llsc_pktid_fifo.read();
            r_llsc_to_tgt_rsp_req		= true;
            r_llsc_fsm = LLSC_IDLE;
          }
          break;
        }
        /////////////////
      case SC_DIR_LOCK:
        {
          if( r_alloc_dir_fsm.read() == ALLOC_DIR_LLSC ) {
            size_t way = 0;
            DirectoryEntry entry(m_cache_directory.read(m_cmd_llsc_addr_fifo.read(), way));
            bool ok = m_atomic_tab.isatomic(m_cmd_llsc_srcid_fifo.read(),m_cmd_llsc_addr_fifo.read());
            if( ok ) {
              r_llsc_is_cnt   = entry.is_cnt;
              r_llsc_dirty 	  = entry.dirty;
              r_llsc_tag		  = entry.tag;
              r_llsc_way		  = way;
              r_llsc_d_copies = entry.d_copies;
              r_llsc_i_copies = entry.i_copies;
              r_llsc_count    = entry.count;
              if ( entry.valid )  r_llsc_fsm = SC_DIR_HIT;
              else                r_llsc_fsm = LLSC_TRT_LOCK;
            } else {
              r_llsc_fsm = SC_RSP_FALSE;
            }
          }
          break;
        }
        ////////////////
      case SC_DIR_HIT:
        {
          size_t way	= r_llsc_way.read();
          size_t set	= m_y[m_cmd_llsc_addr_fifo.read()]; 
          size_t word	= m_x[m_cmd_llsc_addr_fifo.read()]; 

          // update directory (lock & dirty bits
          DirectoryEntry entry;
          entry.valid	    = true;
          entry.is_cnt    = r_llsc_is_cnt.read();
          entry.dirty	    = true;
          entry.lock	    = true;
          entry.tag	      = r_llsc_tag.read();
          entry.d_copies  = r_llsc_d_copies.read();
          entry.i_copies  = r_llsc_i_copies.read();
          entry.count     = r_llsc_count.read();
          m_cache_directory.write(set, way, entry);

          // write data in cache
          m_cache_data[way][set][word] = m_cmd_llsc_wdata_fifo.read();

          // reset Atomic Table
          m_atomic_tab.reset(m_cmd_llsc_addr_fifo.read());

          r_llsc_fsm = SC_RSP_TRUE;
          break;
        }
        //////////////////
      case SC_RSP_FALSE:
        {
          if( !r_llsc_to_tgt_rsp_req ) {
            cmd_llsc_fifo_get		= true;
            r_llsc_to_tgt_rsp_req		= true;
            r_llsc_to_tgt_rsp_data	= 1;
            r_llsc_to_tgt_rsp_srcid	= m_cmd_llsc_srcid_fifo.read();
            r_llsc_to_tgt_rsp_trdid	= m_cmd_llsc_trdid_fifo.read();
            r_llsc_to_tgt_rsp_pktid	= m_cmd_llsc_pktid_fifo.read();
            r_llsc_fsm 			= LLSC_IDLE;
          }
          break;
        }
        /////////////////
      case SC_RSP_TRUE:
        {
          if( !r_llsc_to_tgt_rsp_req ) {
            cmd_llsc_fifo_get		    = true;
            r_llsc_to_tgt_rsp_req		= true;
            r_llsc_to_tgt_rsp_data	= 0;
            r_llsc_to_tgt_rsp_srcid	= m_cmd_llsc_srcid_fifo.read();
            r_llsc_to_tgt_rsp_trdid	= m_cmd_llsc_trdid_fifo.read();
            r_llsc_to_tgt_rsp_pktid	= m_cmd_llsc_pktid_fifo.read();
            r_llsc_fsm 			        = LLSC_IDLE;
          }
          break;
        }
        ///////////////////
      case LLSC_TRT_LOCK:         // read or write miss : check the Transaction Table
        {
          if( r_alloc_trt_fsm.read() == ALLOC_TRT_LLSC ) {
            size_t   index = 0;
            bool hit = m_transaction_tab.hit_read(m_nline[m_cmd_llsc_addr_fifo.read()],index);
            bool wok = !m_transaction_tab.full(index);

            if ( hit || !wok ) {  // missing line already requested or no space in TRT
              r_llsc_fsm = LLSC_IDLE;
            } else {
              r_llsc_trt_index = index;
              r_llsc_fsm       = LLSC_TRT_SET;
            }
          }
          break;
        }
        //////////////////
      case LLSC_TRT_SET:	// register the XRAM transaction in Transaction Table
        {
          if( r_alloc_trt_fsm.read() == ALLOC_TRT_LLSC ) {
            m_transaction_tab.set(r_llsc_trt_index.read(),
                true,
                m_nline[m_cmd_llsc_addr_fifo.read()],
                m_cmd_llsc_srcid_fifo.read(),
                m_cmd_llsc_trdid_fifo.read(),
                m_cmd_llsc_pktid_fifo.read(),
                false,
                false,
                0,
                std::vector<be_t>(m_words,0),
                std::vector<data_t>(m_words,0));
            r_llsc_fsm = LLSC_XRAM_REQ;
          }
          break;
        }
        ///////////////////
      case LLSC_XRAM_REQ:	// request the XRAM_CMD FSM to fetch the missing line
        {
          if ( !r_llsc_to_xram_cmd_req ) {
            r_llsc_to_xram_cmd_req        = true;
            r_llsc_to_xram_cmd_trdid      = r_llsc_trt_index.read();
            r_llsc_to_xram_cmd_nline      = m_nline[m_cmd_llsc_addr_fifo.read()];
            if( m_cmd_llsc_sc_fifo.read() ) {
              r_llsc_fsm                    = SC_RSP_FALSE;
            } else {
              cmd_llsc_fifo_get             = true;
              r_llsc_fsm                    = LLSC_IDLE;
            }
          }
          break;
        }
    } // end switch r_llsc_fsm


    //////////////////////////////////////////////////////////////////////////////
    //		INIT_CMD FSM
    //////////////////////////////////////////////////////////////////////////////
    // The INIT_CMD fsm controls the VCI CMD initiator port, used to update
    // or invalidate cache lines in L1 caches.
    // It implements a round-robin priority between the two following requests:
    // - r_write_to_init_cmd_req : update request from WRITE FSM 
    // - r_xram_rsp_to_init_cmd_req : invalidate request from XRAM_RSP FSM 
    // The inval request is a single cell VCI write command containing the 
    // index of the line to be invalidated.
    // The update request is a multi-cells VCI write command : The first cell 
    // contains the index of the cache line to be updated. The second cell contains 
    // the index of the first modified word in the line. The following cells
    // contain the data.
    ///////////////////////////////////////////////////////////////////////////////

    switch ( r_init_cmd_fsm.read() ) {

      //////////////////////// 
      case INIT_CMD_UPDT_IDLE:	// Invalidate requests have highest priority
        {

          if ( r_xram_rsp_to_init_cmd_req.read() ) {
            r_init_cmd_fsm = INIT_CMD_INVAL_SEL;
            m_cpt_inval++;
          } else if ( r_write_to_init_cmd_req.read() ) {
            if(r_write_to_init_cmd_brdcast.read()){
              r_init_cmd_fsm = INIT_CMD_BRDCAST;
              m_cpt_inval++;
              m_cpt_inval_brdcast++;
            } else {
              r_init_cmd_fsm = INIT_CMD_UPDT_SEL;
              m_cpt_update++;
            }
          }
          break;
        }
        /////////////////////////
      case INIT_CMD_INVAL_IDLE:	// Update requests have highest priority
        {
          if ( r_write_to_init_cmd_req.read() ) {
            if(r_write_to_init_cmd_brdcast.read()){
              r_init_cmd_fsm = INIT_CMD_BRDCAST;
              m_cpt_inval++;
              m_cpt_inval_brdcast++;
            } else {
              r_init_cmd_fsm = INIT_CMD_UPDT_SEL;
              m_cpt_update++;
            }
          } else if ( r_xram_rsp_to_init_cmd_req.read() ) {
            r_init_cmd_fsm = INIT_CMD_INVAL_SEL;
            m_cpt_inval++;
          }
          break;
        }
        ////////////////////////
      case INIT_CMD_INVAL_SEL:	// selects L1 caches
        {
          if(r_xram_rsp_to_init_cmd_brdcast.read()){
            m_cpt_inval_brdcast++;
            r_init_cmd_fsm = INIT_CMD_INVAL_NLINE;
            break;
          }
          if ((r_xram_rsp_to_init_cmd_d_copies.read() == 0) &&
              (r_xram_rsp_to_init_cmd_i_copies.read() == 0)) {	// no more copies
            r_xram_rsp_to_init_cmd_req = false;
            r_init_cmd_fsm = INIT_CMD_INVAL_IDLE;
            break;
          }
          m_cpt_inval_mult++;
          copy_t copies;
          bool inst = false;
          if(r_xram_rsp_to_init_cmd_i_copies.read()){
            copies = r_xram_rsp_to_init_cmd_i_copies.read();
            inst = true;
          } else {
            copies = r_xram_rsp_to_init_cmd_d_copies.read();
          }
          copy_t mask   = 0x1;
          for ( size_t i=0 ; i<8*sizeof(copy_t) ; i++ ) {
            if ( copies & mask ) {
              r_init_cmd_target = i;
              break;
            }
            mask = mask << 1;
          } // end for 
          r_init_cmd_fsm = INIT_CMD_INVAL_NLINE;
          r_init_cmd_inst = inst;
          if(inst){
            r_xram_rsp_to_init_cmd_i_copies = copies & ~mask;
          } else {
            r_xram_rsp_to_init_cmd_d_copies = copies & ~mask;
          }
          break;
        }
        ////////////////////////
      case INIT_CMD_INVAL_NLINE:	// send the cache line index
        {
          if ( p_vci_ini.cmdack ) {
            if ( r_xram_rsp_to_init_cmd_brdcast.read() ) {
              r_init_cmd_fsm = INIT_CMD_INVAL_SEL;
              r_xram_rsp_to_init_cmd_req = false;
            }
            else r_init_cmd_fsm = INIT_CMD_INVAL_IDLE;
          }
          break;
        }
        ///////////////////////
      case INIT_CMD_UPDT_SEL:	// selects the next L1 cache
        {
          if ((r_write_to_init_cmd_i_copies.read() == 0) &&
              (r_write_to_init_cmd_d_copies.read() == 0)) {	// no more copies
            r_write_to_init_cmd_req = false;
            r_init_cmd_fsm    = INIT_CMD_UPDT_IDLE;
          } else {						// select the first target
            copy_t copies;
            bool inst = false;
            if(r_write_to_init_cmd_i_copies.read()){
              inst = true;
              copies = r_write_to_init_cmd_i_copies.read();
            } else {
              copies = r_write_to_init_cmd_d_copies.read();
            } 
            copy_t mask   = 0x1;
            for ( size_t i=0 ; i<8*sizeof(copy_t) ; i++ ) {
              if ( copies & mask ) {
                r_init_cmd_target = i;
                break;
              }
              mask = mask << 1;
            } // end for 
            r_init_cmd_fsm    = INIT_CMD_UPDT_NLINE;
            r_init_cmd_inst = inst;
            if(inst){
              r_write_to_init_cmd_i_copies = copies & ~mask;
            } else {
              r_write_to_init_cmd_d_copies = copies & ~mask;
            }
            r_init_cmd_cpt    = 0;
            m_cpt_update_mult++;
          }
          break;
        }
        /////////////////////////
      case INIT_CMD_BRDCAST:
        {
          if( p_vci_ini.cmdack ) {
            r_write_to_init_cmd_req = false;
            r_init_cmd_fsm = INIT_CMD_UPDT_IDLE;
          }
          break;
        }
        /////////////////////////
      case INIT_CMD_UPDT_NLINE:	// send the cache line index
        {
          if ( p_vci_ini.cmdack ){
            if(r_init_cmd_inst.read()){
              r_init_cmd_fsm = INIT_CMD_UPDT_SEL;
            } else {
              r_init_cmd_fsm = INIT_CMD_UPDT_INDEX;
            }
          }
          break;
        }
        /////////////////////////
      case INIT_CMD_UPDT_INDEX:	// send the first word index
        {

          if ( p_vci_ini.cmdack )  r_init_cmd_fsm = INIT_CMD_UPDT_DATA;
          break;
        }
        ////////////////////////
      case INIT_CMD_UPDT_DATA:	// send the data
        {
          if ( p_vci_ini.cmdack ) {
            if ( r_init_cmd_cpt.read() == (r_write_to_init_cmd_count.read()-1) ) {
              r_init_cmd_fsm = INIT_CMD_UPDT_SEL;
            } else {
              r_init_cmd_cpt = r_init_cmd_cpt.read() + 1;
            }
          }
          break;
        }
    } // end switch r_init_cmd_fsm

    /////////////////////////////////////////////////////////////////////
    //		TGT_RSP FSM
    /////////////////////////////////////////////////////////////////////
    // The TGT_RSP fsm sends the responses on the VCI target port
    // with a round robin priority between six requests :
    // - r_read_to_tgt_rsp_req
    // - r_write_to_tgt_rsp_req
    // - r_llsc_to_tgt_rsp_req
    // - r_cleanup_to_tgt_rsp_req
    // - r_init_rsp_to_tgt_rsp_req
    // - r_xram_rsp_to_tgt_rsp_req
    // The  ordering is :  read > write > llsc > cleanup > xram > init
    /////////////////////////////////////////////////////////////////////

    switch ( r_tgt_rsp_fsm.read() ) {

      ///////////////////////
      case TGT_RSP_READ_IDLE:		// write requests have the highest priority
        {

          if      ( r_write_to_tgt_rsp_req    ) r_tgt_rsp_fsm = TGT_RSP_WRITE;
          else if ( r_llsc_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_LLSC;
          else if ( r_xram_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_XRAM_TEST;
          else if ( r_init_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_INIT;
          else if ( r_cleanup_to_tgt_rsp_req  ) r_tgt_rsp_fsm = TGT_RSP_CLEANUP;
          else if ( r_read_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_READ_TEST;
          break;
        }
        ////////////////////////
      case TGT_RSP_WRITE_IDLE:		// llsc requests have the highest priority
        {

          if      ( r_llsc_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_LLSC;
          else if ( r_xram_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_XRAM_TEST;
          else if ( r_init_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_INIT;
          else if ( r_cleanup_to_tgt_rsp_req  ) r_tgt_rsp_fsm = TGT_RSP_CLEANUP;
          else if ( r_read_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_READ_TEST;
          else if ( r_write_to_tgt_rsp_req    ) r_tgt_rsp_fsm = TGT_RSP_WRITE;
          break;
        }
        ///////////////////////
      case TGT_RSP_LLSC_IDLE:		// cleanup requests have the highest priority
        {

          if ( r_xram_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_XRAM_TEST;
          else if ( r_init_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_INIT;
          else if ( r_cleanup_to_tgt_rsp_req  ) r_tgt_rsp_fsm = TGT_RSP_CLEANUP;
          else if ( r_read_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_READ_TEST;
          else if ( r_write_to_tgt_rsp_req    ) r_tgt_rsp_fsm = TGT_RSP_WRITE;
          else if ( r_llsc_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_LLSC;
          break;
        }
      case TGT_RSP_XRAM_IDLE:		// init requests have the highest priority
        {

          if      ( r_init_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_INIT;
          else if ( r_cleanup_to_tgt_rsp_req  ) r_tgt_rsp_fsm = TGT_RSP_CLEANUP;
          else if ( r_read_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_READ_TEST;
          else if ( r_write_to_tgt_rsp_req    ) r_tgt_rsp_fsm = TGT_RSP_WRITE;
          else if ( r_llsc_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_LLSC;
          else if ( r_xram_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_XRAM_TEST;
          break;
        }
        ///////////////////////
      case TGT_RSP_INIT_IDLE:		// cleanup requests have the highest priority
        {
          if      ( r_cleanup_to_tgt_rsp_req  ) r_tgt_rsp_fsm = TGT_RSP_CLEANUP;
          else if ( r_read_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_READ_TEST;
          else if ( r_write_to_tgt_rsp_req    ) r_tgt_rsp_fsm = TGT_RSP_WRITE;
          else if ( r_llsc_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_LLSC;
          else if ( r_xram_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_XRAM_TEST;
          else if ( r_init_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_INIT;
          break;
        }
        ///////////////////////
      case TGT_RSP_CLEANUP_IDLE:		// read requests have the highest priority
        {
          if      ( r_read_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_READ_TEST;
          else if ( r_write_to_tgt_rsp_req    ) r_tgt_rsp_fsm = TGT_RSP_WRITE;
          else if ( r_llsc_to_tgt_rsp_req     ) r_tgt_rsp_fsm = TGT_RSP_LLSC;
          else if ( r_xram_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_XRAM_TEST;
          else if ( r_init_rsp_to_tgt_rsp_req ) r_tgt_rsp_fsm = TGT_RSP_INIT;
          else if ( r_cleanup_to_tgt_rsp_req  ) r_tgt_rsp_fsm = TGT_RSP_CLEANUP;
          break;
        }
        ///////////////////////
      case TGT_RSP_READ_TEST:		// test if word or cache line
        {
          bool 		line = true;
          size_t	index;
          for ( size_t i=0; i< m_words ; i++ ) {
            line = line && r_read_to_tgt_rsp_val[i];
            if ( r_read_to_tgt_rsp_val[i] ) index = i;
          }
          if ( line ) {
            r_tgt_rsp_cpt = 0;
            r_tgt_rsp_fsm = TGT_RSP_READ_LINE;
          } else {
            r_tgt_rsp_cpt = index;
            r_tgt_rsp_fsm = TGT_RSP_READ_WORD;
          } 
          break;
        }
        ///////////////////////
      case TGT_RSP_READ_WORD:		// send one word response
        {
          if ( p_vci_tgt.rspack ) {
            r_tgt_rsp_fsm = TGT_RSP_READ_IDLE;
            r_read_to_tgt_rsp_req = false;
          }
          break;
        }
        ///////////////////////
      case TGT_RSP_READ_LINE:		// send one complete cache line
        {
          if ( p_vci_tgt.rspack ) {
            if ( r_tgt_rsp_cpt.read() == (m_words-1) ) {
              r_tgt_rsp_fsm = TGT_RSP_READ_IDLE;
              r_read_to_tgt_rsp_req = false;
            } else {
              r_tgt_rsp_cpt = r_tgt_rsp_cpt.read() + 1;
            }
          }
          break;
        }
        ///////////////////
      case TGT_RSP_WRITE:		// send the write acknowledge
        {
          if ( p_vci_tgt.rspack ) {
            r_tgt_rsp_fsm = TGT_RSP_WRITE_IDLE;
            r_write_to_tgt_rsp_req = false;
          }
          break;
        }
        ///////////////////
      case TGT_RSP_CLEANUP:		// send the write acknowledge
        {
          if ( p_vci_tgt.rspack ) {
            r_tgt_rsp_fsm = TGT_RSP_CLEANUP_IDLE;
            r_cleanup_to_tgt_rsp_req = false;
          }
          break;
        }
        //////////////////
      case TGT_RSP_LLSC:		// send one atomic word response
        {
          if ( p_vci_tgt.rspack ) {
            r_tgt_rsp_fsm = TGT_RSP_LLSC_IDLE;
            r_llsc_to_tgt_rsp_req = false;
          }
          break;
        }

      case TGT_RSP_XRAM_TEST:		// test if word or cache line
        {
          bool 	line = true;
          size_t	index;
          for ( size_t i=0; i< m_words ; i++ ) {
            line = line && r_xram_rsp_to_tgt_rsp_val[i];
            if ( r_xram_rsp_to_tgt_rsp_val[i] ) index = i;
          }
          if ( line ) {
            r_tgt_rsp_cpt = 0;
            r_tgt_rsp_fsm = TGT_RSP_XRAM_LINE;
          } else {
            r_tgt_rsp_cpt = index;
            r_tgt_rsp_fsm = TGT_RSP_XRAM_WORD;
          } 
          break;
        }
        ///////////////////////
      case TGT_RSP_XRAM_WORD:		// send one word response
        {
          if ( p_vci_tgt.rspack ) {
            r_tgt_rsp_fsm = TGT_RSP_XRAM_IDLE;
            r_xram_rsp_to_tgt_rsp_req = false;
          }
          break;
        }
        ///////////////////////
      case TGT_RSP_XRAM_LINE:		// send one complete cache line
        {
          if ( p_vci_tgt.rspack ) {
            if ( r_tgt_rsp_cpt.read() == (m_words-1) ) {
              r_tgt_rsp_fsm = TGT_RSP_XRAM_IDLE;
              r_xram_rsp_to_tgt_rsp_req = false;
            } else {
              r_tgt_rsp_cpt = r_tgt_rsp_cpt.read() + 1;
            }
          }
          break;
        }
        ///////////////////
      case TGT_RSP_INIT:		// send the pending write acknowledge


        {
          if ( p_vci_tgt.rspack ) {
            r_tgt_rsp_fsm = TGT_RSP_INIT_IDLE;
            r_init_rsp_to_tgt_rsp_req = false;
          }
          break;
        }
    } // end switch tgt_rsp_fsm
    ////////////////////////////////////////////////////////////////////////////////////
    //		NEW ALLOC_UPT FSM
    ////////////////////////////////////////////////////////////////////////////////////
    // The ALLOC_UPT FSM allocates the access to the Update/Inval Table (UPT).
    // with a round robin priority between three FSMs : INIT_RSP > WRITE > XRAM_RSP > CLEANUP 
    // - The WRITE FSM initiates update transactions and sets  new entry in UPT.
    // - The XRAM_RSP FSM initiates inval transactions and sets  new entry in UPT.
    // - The INIT_RSP FSM complete those trasactions and erase the UPT entry.
    // - The CLEANUP  FSM decrement an entry in UPT.
    // The resource is always allocated.
    /////////////////////////////////////////////////////////////////////////////////////

    switch ( r_alloc_upt_fsm.read() ) {

      ////////////////////////
      case ALLOC_UPT_INIT_RSP:
        if ( (r_init_rsp_fsm.read() != INIT_RSP_UPT_LOCK) && 
            (r_init_rsp_fsm.read() != INIT_RSP_UPT_CLEAR) ) 
        {
          if      ((r_write_fsm.read() == WRITE_UPT_LOCK) ||
                   (r_write_fsm.read() == WRITE_INVAL_LOCK))  r_alloc_upt_fsm = ALLOC_UPT_WRITE;
          else if ((r_xram_rsp_fsm.read() == XRAM_RSP_UPT_LOCK) ||
              (r_xram_rsp_fsm.read() == XRAM_RSP_INVAL_LOCK)) r_alloc_upt_fsm = ALLOC_UPT_XRAM_RSP;
          else if (r_cleanup_fsm.read() == CLEANUP_UPT_LOCK)   r_alloc_upt_fsm = ALLOC_UPT_CLEANUP;
        }
        break;

        /////////////////////
      case ALLOC_UPT_WRITE:
        if ( (r_write_fsm.read() != WRITE_UPT_LOCK) &&
             (r_write_fsm.read() != WRITE_INVAL_LOCK)) 
        {
          if      ((r_xram_rsp_fsm.read() == XRAM_RSP_UPT_LOCK) ||
              (r_xram_rsp_fsm.read() == XRAM_RSP_INVAL_LOCK)) r_alloc_upt_fsm = ALLOC_UPT_XRAM_RSP;
          else if (r_cleanup_fsm.read() == CLEANUP_UPT_LOCK)   r_alloc_upt_fsm = ALLOC_UPT_CLEANUP;
          else if (r_init_rsp_fsm.read() == INIT_RSP_UPT_LOCK) r_alloc_upt_fsm = ALLOC_UPT_INIT_RSP;
        }
        break;

        ////////////////////////
      case ALLOC_UPT_XRAM_RSP:
        if ( (r_xram_rsp_fsm.read() != XRAM_RSP_UPT_LOCK) &&
            (r_xram_rsp_fsm.read() != XRAM_RSP_INVAL_LOCK) ) 
        { 
          if (r_cleanup_fsm.read() == CLEANUP_UPT_LOCK)   r_alloc_upt_fsm = ALLOC_UPT_CLEANUP;
          else if      (r_init_rsp_fsm.read() == INIT_RSP_UPT_LOCK) r_alloc_upt_fsm = ALLOC_UPT_INIT_RSP;
          else if ((r_write_fsm.read() == WRITE_UPT_LOCK)   ||
                   (r_write_fsm.read() == WRITE_INVAL_LOCK))     r_alloc_upt_fsm = ALLOC_UPT_WRITE;
        }
        break;

        //////////////////////////
      case ALLOC_UPT_CLEANUP:
        if(r_cleanup_fsm.read() != CLEANUP_UPT_LOCK 
            || r_cleanup_fsm.read() != CLEANUP_UPT_WRITE)
        {
          if 	    (r_init_rsp_fsm.read() == INIT_RSP_UPT_LOCK)   r_alloc_upt_fsm = ALLOC_UPT_INIT_RSP;
          else if ((r_write_fsm.read() == WRITE_UPT_LOCK) 	 ||
                   (r_write_fsm.read() == WRITE_INVAL_LOCK))   r_alloc_upt_fsm = ALLOC_UPT_WRITE;
          else if ((r_xram_rsp_fsm.read() == XRAM_RSP_UPT_LOCK) ||
              (r_xram_rsp_fsm.read() == XRAM_RSP_INVAL_LOCK))   r_alloc_upt_fsm = ALLOC_UPT_XRAM_RSP;
        }
        break;

    } // end switch r_alloc_upt_fsm

    ////////////////////////////////////////////////////////////////////////////////////
    //		ALLOC_DIR FSM
    ////////////////////////////////////////////////////////////////////////////////////
    // The ALLOC_DIR FSM allocates the access to the directory and
    // the data cache with a round robin priority between 5 user FSMs :
    // The cyclic ordering is READ > WRITE > LLSC > CLEANUP > XRAM_RSP
    // The ressource is always allocated.
    /////////////////////////////////////////////////////////////////////////////////////

    switch ( r_alloc_dir_fsm.read() ) {

      ////////////////////
      case ALLOC_DIR_READ:
        if ( ( (r_read_fsm.read() != READ_DIR_LOCK) &&
              (r_read_fsm.read() != READ_TRT_LOCK)     )
            ||
            ( (r_read_fsm.read()      == READ_TRT_LOCK)  &&
              (r_alloc_trt_fsm.read() == ALLOC_TRT_READ)    )  ) 
        {
          if      (r_write_fsm.read() == WRITE_DIR_LOCK)       r_alloc_dir_fsm = ALLOC_DIR_WRITE;
          else if ((r_llsc_fsm.read() == LL_DIR_LOCK) || 
              (r_llsc_fsm.read() == SC_DIR_LOCK))         r_alloc_dir_fsm = ALLOC_DIR_LLSC;
          else if (r_cleanup_fsm.read() == CLEANUP_DIR_LOCK)   r_alloc_dir_fsm = ALLOC_DIR_CLEANUP;
          else if (r_xram_rsp_fsm.read() == XRAM_RSP_DIR_LOCK) r_alloc_dir_fsm = ALLOC_DIR_XRAM_RSP;
        }
        break;

        /////////////////////
      case ALLOC_DIR_WRITE:
        if ( ( (r_write_fsm.read() != WRITE_DIR_LOCK)     &&
              (r_write_fsm.read() != WRITE_TRT_LOCK)     &&
              (r_write_fsm.read() != WRITE_DIR_HIT_READ) &&
              (r_write_fsm.read() != WRITE_TRT_WRITE_LOCK) &&
              (r_write_fsm.read() != WRITE_INVAL_LOCK) )
            ||
            ( (r_write_fsm.read()     == WRITE_TRT_LOCK) &&
              (r_alloc_trt_fsm.read() == ALLOC_TRT_WRITE)   )   )
        {
          if      ((r_llsc_fsm.read() == LL_DIR_LOCK) || 
              (r_llsc_fsm.read() == SC_DIR_LOCK))         r_alloc_dir_fsm = ALLOC_DIR_LLSC;
          else if (r_cleanup_fsm.read() == CLEANUP_DIR_LOCK)   r_alloc_dir_fsm = ALLOC_DIR_CLEANUP;
          else if (r_xram_rsp_fsm.read() == XRAM_RSP_DIR_LOCK) r_alloc_dir_fsm = ALLOC_DIR_XRAM_RSP;
          else if (r_read_fsm.read() == READ_DIR_LOCK) 	     r_alloc_dir_fsm = ALLOC_DIR_READ;
        }
        break;

        ////////////////////
      case ALLOC_DIR_LLSC:
        if ( ( (r_llsc_fsm.read() != LL_DIR_LOCK)    &&
              (r_llsc_fsm.read() != LL_DIR_HIT )    &&
              (r_llsc_fsm.read() != SC_DIR_LOCK)    &&
              (r_llsc_fsm.read() != SC_DIR_HIT )    &&
              (r_llsc_fsm.read() != LLSC_TRT_LOCK )    )
            || 
            ( (r_llsc_fsm.read()      == LLSC_TRT_LOCK ) &&
              (r_alloc_trt_fsm.read() == ALLOC_TRT_LLSC)    ) )
        {
          if      (r_cleanup_fsm.read() == CLEANUP_DIR_LOCK)   r_alloc_dir_fsm = ALLOC_DIR_CLEANUP;
          else if (r_xram_rsp_fsm.read() == XRAM_RSP_DIR_LOCK) r_alloc_dir_fsm = ALLOC_DIR_XRAM_RSP;
          else if (r_read_fsm.read() == READ_DIR_LOCK) 	     r_alloc_dir_fsm = ALLOC_DIR_READ;
          else if (r_write_fsm.read() == WRITE_DIR_LOCK)       r_alloc_dir_fsm = ALLOC_DIR_WRITE;
        }
        break;

        ///////////////////////
      case ALLOC_DIR_CLEANUP:
        if ( (r_cleanup_fsm.read() != CLEANUP_DIR_LOCK) &&
            (r_cleanup_fsm.read() != CLEANUP_DIR_WRITE) )
        {
          if      (r_xram_rsp_fsm.read() == XRAM_RSP_DIR_LOCK) r_alloc_dir_fsm = ALLOC_DIR_XRAM_RSP;
          else if (r_read_fsm.read() == READ_DIR_LOCK) 	     r_alloc_dir_fsm = ALLOC_DIR_READ;
          else if (r_write_fsm.read() == WRITE_DIR_LOCK)       r_alloc_dir_fsm = ALLOC_DIR_WRITE;
          else if ((r_llsc_fsm.read() == LL_DIR_LOCK) || 
              (r_llsc_fsm.read() == SC_DIR_LOCK))         r_alloc_dir_fsm = ALLOC_DIR_LLSC;
        }
        break;
        ////////////////////////
      case ALLOC_DIR_XRAM_RSP:
        if ( (r_xram_rsp_fsm.read() != XRAM_RSP_DIR_LOCK)  &&
            (r_xram_rsp_fsm.read() != XRAM_RSP_TRT_COPY)   && 
            (r_xram_rsp_fsm.read() != XRAM_RSP_INVAL_LOCK))
        {
          if      (r_read_fsm.read() == READ_DIR_LOCK) 	     r_alloc_dir_fsm = ALLOC_DIR_READ;
          else if (r_write_fsm.read() == WRITE_DIR_LOCK)     r_alloc_dir_fsm = ALLOC_DIR_WRITE;
          else if ((r_llsc_fsm.read() == LL_DIR_LOCK) || 
              (r_llsc_fsm.read() == SC_DIR_LOCK))       r_alloc_dir_fsm = ALLOC_DIR_LLSC;
          else if (r_cleanup_fsm.read() == CLEANUP_DIR_LOCK) r_alloc_dir_fsm = ALLOC_DIR_CLEANUP;
        }
        break;

    } // end switch alloc_dir_fsm

    ////////////////////////////////////////////////////////////////////////////////////
    //		ALLOC_TRT FSM
    ////////////////////////////////////////////////////////////////////////////////////
    // The ALLOC_TRT fsm allocates the access to the Transaction Table (write buffer)
    // with a round robin priority between 4 user FSMs :
    // The cyclic priority is READ > WRITE > LLSC > XRAM_RSP
    // The ressource is always allocated.
    ///////////////////////////////////////////////////////////////////////////////////

    switch (r_alloc_trt_fsm) {

      ////////////////////
      case ALLOC_TRT_READ:
        if ( r_read_fsm.read() != READ_TRT_LOCK ) 
        {
          if      ((r_write_fsm.read() == WRITE_TRT_LOCK)   ||
                   (r_write_fsm.read() == WRITE_TRT_WRITE_LOCK))    r_alloc_trt_fsm = ALLOC_TRT_WRITE;
          else if (r_llsc_fsm.read() == LLSC_TRT_LOCK)              r_alloc_trt_fsm = ALLOC_TRT_LLSC;
          else if (r_xram_rsp_fsm.read() == XRAM_RSP_TRT_COPY)      r_alloc_trt_fsm = ALLOC_TRT_XRAM_RSP;
          else if ((r_ixr_rsp_fsm.read() == IXR_RSP_TRT_ERASE) ||
              (r_ixr_rsp_fsm.read() == IXR_RSP_TRT_READ))       r_alloc_trt_fsm = ALLOC_TRT_IXR_RSP;
        }
        break;
        /////////////////////
      case ALLOC_TRT_WRITE:
        if ( (r_write_fsm.read() != WRITE_TRT_LOCK) &&
             (r_write_fsm.read() != WRITE_TRT_WRITE_LOCK) &&
             (r_write_fsm.read() != WRITE_INVAL_LOCK)) 
        {
          if      (r_llsc_fsm.read() == LLSC_TRT_LOCK) 	          r_alloc_trt_fsm = ALLOC_TRT_LLSC;
          else if (r_xram_rsp_fsm.read() == XRAM_RSP_TRT_COPY)      r_alloc_trt_fsm = ALLOC_TRT_XRAM_RSP;
          else if ((r_ixr_rsp_fsm.read() == IXR_RSP_TRT_ERASE) ||
              (r_ixr_rsp_fsm.read() == IXR_RSP_TRT_READ))       r_alloc_trt_fsm = ALLOC_TRT_IXR_RSP;
          else if (r_read_fsm.read() == READ_TRT_LOCK) 	          r_alloc_trt_fsm = ALLOC_TRT_READ;
        }
        break;
        ////////////////////
      case ALLOC_TRT_LLSC:
        if ( r_llsc_fsm.read() != LLSC_TRT_LOCK )
        { 
          if      (r_xram_rsp_fsm.read() == XRAM_RSP_TRT_COPY)      r_alloc_trt_fsm = ALLOC_TRT_XRAM_RSP;
          else if ((r_ixr_rsp_fsm.read() == IXR_RSP_TRT_ERASE) ||
              (r_ixr_rsp_fsm.read() == IXR_RSP_TRT_READ))       r_alloc_trt_fsm = ALLOC_TRT_IXR_RSP;
          else if (r_read_fsm.read() == READ_TRT_LOCK) 	          r_alloc_trt_fsm = ALLOC_TRT_READ;
          else if ((r_write_fsm.read() == WRITE_TRT_LOCK)     ||
                   (r_write_fsm.read() == WRITE_TRT_WRITE_LOCK))  r_alloc_trt_fsm = ALLOC_TRT_WRITE;
        }
        break;
        ////////////////////////
      case ALLOC_TRT_XRAM_RSP:
        if ( (r_xram_rsp_fsm.read() != XRAM_RSP_TRT_COPY)  &&
            (r_xram_rsp_fsm.read() != XRAM_RSP_DIR_UPDT)   &&
            (r_xram_rsp_fsm.read() != XRAM_RSP_INVAL_LOCK)) {
          if      ((r_ixr_rsp_fsm.read() == IXR_RSP_TRT_ERASE) ||
              (r_ixr_rsp_fsm.read() == IXR_RSP_TRT_READ))       r_alloc_trt_fsm = ALLOC_TRT_IXR_RSP;
          else if (r_read_fsm.read() == READ_TRT_LOCK) 	          r_alloc_trt_fsm = ALLOC_TRT_READ;
          else if ((r_write_fsm.read() == WRITE_TRT_LOCK)    ||
                   (r_write_fsm.read() == WRITE_TRT_WRITE_LOCK))  r_alloc_trt_fsm = ALLOC_TRT_WRITE;
          else if (r_llsc_fsm.read() == LLSC_TRT_LOCK) 	          r_alloc_trt_fsm = ALLOC_TRT_LLSC;
        }
        break;
        ////////////////////////
      case ALLOC_TRT_IXR_RSP:
        if ( (r_ixr_rsp_fsm.read() != IXR_RSP_TRT_ERASE) &&
            (r_ixr_rsp_fsm.read() != IXR_RSP_TRT_READ) ) {
          if      (r_read_fsm.read() == READ_TRT_LOCK) 	          r_alloc_trt_fsm = ALLOC_TRT_READ;
          else if ((r_write_fsm.read() == WRITE_TRT_LOCK)   ||
                   (r_write_fsm.read() == WRITE_TRT_WRITE_LOCK))  r_alloc_trt_fsm = ALLOC_TRT_WRITE;
          else if (r_llsc_fsm.read() == LLSC_TRT_LOCK) 	          r_alloc_trt_fsm = ALLOC_TRT_LLSC;
          else if (r_xram_rsp_fsm.read() == XRAM_RSP_TRT_COPY)      r_alloc_trt_fsm = ALLOC_TRT_XRAM_RSP;
        }
        break;

    } // end switch alloc_trt_fsm

    ////////////////////////////////////////////////////////////////////////////////////
    //		TGT_CMD to READ FIFO
    ////////////////////////////////////////////////////////////////////////////////////

    if ( cmd_read_fifo_put ) {
      if ( cmd_read_fifo_get ) {
        m_cmd_read_addr_fifo.put_and_get(p_vci_tgt.address.read());
        m_cmd_read_word_fifo.put_and_get((p_vci_tgt.plen.read() == 4));
        m_cmd_read_srcid_fifo.put_and_get(p_vci_tgt.srcid.read());
        m_cmd_read_trdid_fifo.put_and_get(p_vci_tgt.trdid.read());
        m_cmd_read_pktid_fifo.put_and_get(p_vci_tgt.pktid.read());
      } else {
        m_cmd_read_addr_fifo.simple_put(p_vci_tgt.address.read());
        m_cmd_read_word_fifo.simple_put((p_vci_tgt.plen.read() == 4));
        m_cmd_read_srcid_fifo.simple_put(p_vci_tgt.srcid.read());
        m_cmd_read_trdid_fifo.simple_put(p_vci_tgt.trdid.read());
        m_cmd_read_pktid_fifo.simple_put(p_vci_tgt.pktid.read());
      }
    } else {
      if ( cmd_read_fifo_get ) {
        m_cmd_read_addr_fifo.simple_get();
        m_cmd_read_word_fifo.simple_get();
        m_cmd_read_srcid_fifo.simple_get();
        m_cmd_read_trdid_fifo.simple_get();
        m_cmd_read_pktid_fifo.simple_get();
      }
    }
    /////////////////////////////////////////////////////////////////////
    //		TGT_CMD to WRITE FIFO
    /////////////////////////////////////////////////////////////////////

    if ( cmd_write_fifo_put ) {
      if ( cmd_write_fifo_get ) {
        m_cmd_write_addr_fifo.put_and_get(p_vci_tgt.address.read());
        m_cmd_write_eop_fifo.put_and_get(p_vci_tgt.eop.read());
        m_cmd_write_srcid_fifo.put_and_get(p_vci_tgt.srcid.read());
        m_cmd_write_trdid_fifo.put_and_get(p_vci_tgt.trdid.read());
        m_cmd_write_pktid_fifo.put_and_get(p_vci_tgt.pktid.read());
        m_cmd_write_data_fifo.put_and_get(p_vci_tgt.wdata.read());
        m_cmd_write_be_fifo.put_and_get(p_vci_tgt.be.read());
      } else {
        m_cmd_write_addr_fifo.simple_put(p_vci_tgt.address.read());
        m_cmd_write_eop_fifo.simple_put(p_vci_tgt.eop.read());
        m_cmd_write_srcid_fifo.simple_put(p_vci_tgt.srcid.read());
        m_cmd_write_trdid_fifo.simple_put(p_vci_tgt.trdid.read());
        m_cmd_write_pktid_fifo.simple_put(p_vci_tgt.pktid.read());
        m_cmd_write_data_fifo.simple_put(p_vci_tgt.wdata.read());
        m_cmd_write_be_fifo.simple_put(p_vci_tgt.be.read());
      }
    } else {
      if ( cmd_write_fifo_get ) {
        m_cmd_write_addr_fifo.simple_get();
        m_cmd_write_eop_fifo.simple_get();
        m_cmd_write_srcid_fifo.simple_get();
        m_cmd_write_trdid_fifo.simple_get();
        m_cmd_write_pktid_fifo.simple_get();
        m_cmd_write_data_fifo.simple_get();
        m_cmd_write_be_fifo.simple_get();
      }
    }
    ////////////////////////////////////////////////////////////////////////////////////
    //		TGT_CMD to LLSC FIFO
    ////////////////////////////////////////////////////////////////////////////////////

    if ( cmd_llsc_fifo_put ) {
      if ( cmd_llsc_fifo_get ) {
        m_cmd_llsc_addr_fifo.put_and_get(p_vci_tgt.address.read());
        m_cmd_llsc_sc_fifo.put_and_get(p_vci_tgt.cmd.read() == vci_param::CMD_STORE_COND); 
        m_cmd_llsc_srcid_fifo.put_and_get(p_vci_tgt.srcid.read());
        m_cmd_llsc_trdid_fifo.put_and_get(p_vci_tgt.trdid.read());
        m_cmd_llsc_pktid_fifo.put_and_get(p_vci_tgt.pktid.read());
        m_cmd_llsc_wdata_fifo.put_and_get(p_vci_tgt.wdata.read());
      } else {
        m_cmd_llsc_addr_fifo.simple_put(p_vci_tgt.address.read());
        m_cmd_llsc_sc_fifo.simple_put(p_vci_tgt.cmd.read() == vci_param::CMD_STORE_COND); 
        m_cmd_llsc_srcid_fifo.simple_put(p_vci_tgt.srcid.read());
        m_cmd_llsc_trdid_fifo.simple_put(p_vci_tgt.trdid.read());
        m_cmd_llsc_pktid_fifo.simple_put(p_vci_tgt.pktid.read());
        m_cmd_llsc_wdata_fifo.simple_put(p_vci_tgt.wdata.read());
      }
    } else {
      if ( cmd_llsc_fifo_get ) {
        m_cmd_llsc_addr_fifo.simple_get();
        m_cmd_llsc_sc_fifo.simple_get();
        m_cmd_llsc_srcid_fifo.simple_get();
        m_cmd_llsc_trdid_fifo.simple_get();
        m_cmd_llsc_pktid_fifo.simple_get();
        m_cmd_llsc_wdata_fifo.simple_get();
      }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //		TGT_CMD to CLEANUP FIFO
    //	    No need with the CLEANUP Network
    ////////////////////////////////////////////////////////////////////////////////////

    //     if ( cmd_cleanup_fifo_put ) {
    //       if ( cmd_cleanup_fifo_get ) {
    //         m_cmd_cleanup_srcid_fifo.put_and_get(p_vci_tgt.srcid.read());
    //         m_cmd_cleanup_trdid_fifo.put_and_get(p_vci_tgt.trdid.read());
    //         m_cmd_cleanup_pktid_fifo.put_and_get(p_vci_tgt.pktid.read());
    //         m_cmd_cleanup_nline_fifo.put_and_get(p_vci_tgt.wdata.read());
    //       } else {
    //         m_cmd_cleanup_srcid_fifo.simple_put(p_vci_tgt.srcid.read());
    //         m_cmd_cleanup_trdid_fifo.simple_put(p_vci_tgt.trdid.read());
    //         m_cmd_cleanup_pktid_fifo.simple_put(p_vci_tgt.pktid.read());
    //         m_cmd_cleanup_nline_fifo.simple_put(p_vci_tgt.wdata.read());
    //       }
    //     } else {
    //       if ( cmd_cleanup_fifo_get ) {
    //         m_cmd_cleanup_srcid_fifo.simple_get();
    //         m_cmd_cleanup_trdid_fifo.simple_get();
    //         m_cmd_cleanup_pktid_fifo.simple_get();
    //         m_cmd_cleanup_nline_fifo.simple_get();
    //       }
    //     }

    m_cpt_cycles++;

  } // end transition()

  /////////////////////////////
  tmpl(void)::genMoore()
    /////////////////////////////
  {
    ////////////////////////////////////////////////////////////
    // Command signals on the p_vci_ixr port
    ////////////////////////////////////////////////////////////


    p_vci_ixr.be      = 0xF;
    p_vci_ixr.pktid   = 0;
    p_vci_ixr.srcid   = m_srcid_ixr;
    p_vci_ixr.cons    = false;
    p_vci_ixr.wrap    = false;
    p_vci_ixr.contig  = true;
    p_vci_ixr.clen    = 0;
    p_vci_ixr.cfixed  = false;

    if ( r_xram_cmd_fsm.read() == XRAM_CMD_READ_NLINE ) {
      p_vci_ixr.cmd     = vci_param::CMD_READ;
      p_vci_ixr.cmdval  = true;
      p_vci_ixr.address = (r_read_to_xram_cmd_nline.read()*m_words*4);
      p_vci_ixr.plen    = m_words*4;
      p_vci_ixr.wdata   = 0x00000000;
      p_vci_ixr.trdid   = r_read_to_xram_cmd_trdid.read();
      p_vci_ixr.eop     = true;
    } 
    else if ( r_xram_cmd_fsm.read() == XRAM_CMD_LLSC_NLINE ) {
      p_vci_ixr.cmd     = vci_param::CMD_READ;
      p_vci_ixr.cmdval  = true;
      p_vci_ixr.address = (r_llsc_to_xram_cmd_nline.read()*m_words*4);
      p_vci_ixr.plen    = m_words*4;
      p_vci_ixr.wdata   = 0x00000000;
      p_vci_ixr.trdid   = r_llsc_to_xram_cmd_trdid.read();
      p_vci_ixr.eop     = true;
    } 
    else if ( r_xram_cmd_fsm.read() == XRAM_CMD_WRITE_NLINE ) {
      if(r_write_to_xram_cmd_write.read()){
        p_vci_ixr.cmd     = vci_param::CMD_WRITE;
        p_vci_ixr.cmdval  = true;
        p_vci_ixr.address = ((r_write_to_xram_cmd_nline.read()*m_words+r_xram_cmd_cpt.read())*4);
        p_vci_ixr.plen    = m_words*4;
        p_vci_ixr.wdata   = r_write_to_xram_cmd_data[r_xram_cmd_cpt.read()].read();
        p_vci_ixr.trdid   = r_write_to_xram_cmd_trdid.read();
        p_vci_ixr.eop     = (r_xram_cmd_cpt == (m_words-1));
      } else {
        p_vci_ixr.cmd     = vci_param::CMD_READ;
        p_vci_ixr.cmdval  = true;
        p_vci_ixr.address = (r_write_to_xram_cmd_nline.read()*m_words*4);
        p_vci_ixr.plen    = m_words*4;
        p_vci_ixr.wdata   = 0x00000000;
        p_vci_ixr.trdid   = r_write_to_xram_cmd_trdid.read();
        p_vci_ixr.eop     = true;
      }
    } 
    else if ( r_xram_cmd_fsm.read() == XRAM_CMD_XRAM_DATA ) {
      p_vci_ixr.cmd     = vci_param::CMD_WRITE;
      p_vci_ixr.cmdval  = true;
      p_vci_ixr.address = ((r_xram_rsp_to_xram_cmd_nline.read()*m_words+r_xram_cmd_cpt.read())*4);
      p_vci_ixr.plen    = m_words*4;
      p_vci_ixr.wdata   = r_xram_rsp_to_xram_cmd_data[r_xram_cmd_cpt.read()].read();
      p_vci_ixr.trdid   = r_xram_rsp_to_xram_cmd_trdid.read();
      p_vci_ixr.eop     = (r_xram_cmd_cpt == (m_words-1));
    } else {
      p_vci_ixr.cmdval  = false;
      p_vci_ixr.address = 0;
      p_vci_ixr.plen    = 0;
      p_vci_ixr.wdata   = 0;
      p_vci_ixr.trdid   = 0;
      p_vci_ixr.eop	  = false;
    }

    ////////////////////////////////////////////////////
    // Response signals on the p_vci_ixr port
    ////////////////////////////////////////////////////

    if ( ((r_alloc_trt_fsm.read() == ALLOC_TRT_IXR_RSP) &&
          (r_ixr_rsp_fsm.read() == IXR_RSP_TRT_READ)) || 
        (r_ixr_rsp_fsm.read() == IXR_RSP_ACK) )           p_vci_ixr.rspack = true;
    else                                                    p_vci_ixr.rspack = false;

    ////////////////////////////////////////////////////
    // Command signals on the p_vci_tgt port
    ////////////////////////////////////////////////////

    switch ((tgt_cmd_fsm_state_e)r_tgt_cmd_fsm.read()) {
      case TGT_CMD_IDLE:
        p_vci_tgt.cmdack  = false;
        break;
      case TGT_CMD_READ:
        p_vci_tgt.cmdack  = m_cmd_read_addr_fifo.wok();
        break;
      case TGT_CMD_READ_EOP:
        p_vci_tgt.cmdack  = true;
        break;
      case TGT_CMD_WRITE:
        p_vci_tgt.cmdack  = m_cmd_write_addr_fifo.wok();
        break;
      case TGT_CMD_ATOMIC:
        p_vci_tgt.cmdack  = m_cmd_llsc_addr_fifo.wok();
        break;
      default:
        p_vci_tgt.cmdack = false;
        break;
    }

    ////////////////////////////////////////////////////
    // Response signals on the p_vci_tgt port
    ////////////////////////////////////////////////////
    switch ( r_tgt_rsp_fsm.read() ) {

      case TGT_RSP_READ_IDLE:
      case TGT_RSP_WRITE_IDLE:
      case TGT_RSP_LLSC_IDLE:
      case TGT_RSP_XRAM_IDLE:
      case TGT_RSP_INIT_IDLE:
      case TGT_RSP_CLEANUP_IDLE:
      case TGT_RSP_READ_TEST:
      case TGT_RSP_XRAM_TEST:

        p_vci_tgt.rspval  = false;
        p_vci_tgt.rsrcid  = 0;
        p_vci_tgt.rdata   = 0;
        p_vci_tgt.rpktid  = 0;
        p_vci_tgt.rtrdid  = 0;
        p_vci_tgt.rerror  = 0;
        p_vci_tgt.reop    = false;	
        break;
      case TGT_RSP_READ_LINE:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = r_read_to_tgt_rsp_data[r_tgt_rsp_cpt.read()].read();
        p_vci_tgt.rsrcid   = r_read_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_read_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_read_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = (r_tgt_rsp_cpt.read() == (m_words-1));
        break;
      case TGT_RSP_READ_WORD:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = r_read_to_tgt_rsp_data[r_tgt_rsp_cpt.read()].read();
        p_vci_tgt.rsrcid   = r_read_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_read_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_read_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = true;	
        break;
      case TGT_RSP_WRITE:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = 0;
        p_vci_tgt.rsrcid   = r_write_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_write_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_write_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = true;
        break;
      case TGT_RSP_CLEANUP:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = 0;
        p_vci_tgt.rsrcid   = r_cleanup_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_cleanup_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_cleanup_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = true;
        break;
      case TGT_RSP_LLSC:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = r_llsc_to_tgt_rsp_data.read();
        p_vci_tgt.rsrcid   = r_llsc_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_llsc_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_llsc_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = true;
        break;
      case TGT_RSP_XRAM_LINE:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = r_xram_rsp_to_tgt_rsp_data[r_tgt_rsp_cpt.read()].read();
        p_vci_tgt.rsrcid   = r_xram_rsp_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_xram_rsp_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_xram_rsp_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = (r_tgt_rsp_cpt.read() == (m_words-1));
        break;
      case TGT_RSP_XRAM_WORD:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = r_xram_rsp_to_tgt_rsp_data[r_tgt_rsp_cpt.read()].read();
        p_vci_tgt.rsrcid   = r_xram_rsp_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_xram_rsp_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_xram_rsp_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = true;
        break;
      case TGT_RSP_INIT:
        p_vci_tgt.rspval   = true;
        p_vci_tgt.rdata    = 0;
        p_vci_tgt.rsrcid   = r_init_rsp_to_tgt_rsp_srcid.read();
        p_vci_tgt.rtrdid   = r_init_rsp_to_tgt_rsp_trdid.read();
        p_vci_tgt.rpktid   = r_init_rsp_to_tgt_rsp_pktid.read();
        p_vci_tgt.rerror   = 0;
        p_vci_tgt.reop     = true;	
        break;
    } // end switch r_tgt_rsp_fsm

    ///////////////////////////////////////////////////
    // Command signals on the p_vci_ini port
    ///////////////////////////////////////////////////

    p_vci_ini.cmd     = vci_param::CMD_WRITE;
    p_vci_ini.srcid   = m_srcid_ini;
    p_vci_ini.pktid   = 0;
    p_vci_ini.cons    = true;
    p_vci_ini.wrap    = false;
    p_vci_ini.contig  = false;
    p_vci_ini.clen    = 0;
    p_vci_ini.cfixed  = false;

    switch ( r_init_cmd_fsm.read() ) {

      case INIT_CMD_UPDT_IDLE:
      case INIT_CMD_INVAL_IDLE:
      case INIT_CMD_UPDT_SEL:
      case INIT_CMD_INVAL_SEL:
        p_vci_ini.cmdval = false;
        p_vci_ini.address = 0;
        p_vci_ini.wdata   = 0;
        p_vci_ini.be      = 0;
        p_vci_ini.plen    = 0;
        p_vci_ini.trdid   = 0;
        p_vci_ini.eop     = false;
        break;
      case INIT_CMD_INVAL_NLINE:
        p_vci_ini.cmdval  = true;
        if(r_xram_rsp_to_init_cmd_brdcast.read())
          p_vci_ini.address = BROADCAST_ADDR;
        else {
          if(r_init_cmd_inst.read()) {
            p_vci_ini.address = m_coherence_table[r_init_cmd_target.read()]+8;
          } else {
            p_vci_ini.address = m_coherence_table[r_init_cmd_target.read()];
          }
        }
        p_vci_ini.wdata   = r_xram_rsp_to_init_cmd_nline.read();
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = 4;
        p_vci_ini.trdid   = r_xram_rsp_to_init_cmd_trdid.read();
        p_vci_ini.eop     = true;
        break;
      case INIT_CMD_BRDCAST:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = BROADCAST_ADDR;
        p_vci_ini.wdata   = r_write_to_init_cmd_nline.read();
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = 4 ;
        p_vci_ini.eop     = true;
        p_vci_ini.trdid   = r_write_to_init_cmd_trdid.read();
        break;
      case INIT_CMD_UPDT_NLINE:
        p_vci_ini.cmdval  = true;
        if(r_init_cmd_inst.read()){
          p_vci_ini.address = m_coherence_table[r_init_cmd_target.read()] + 8;
        } else {
          p_vci_ini.address = m_coherence_table[r_init_cmd_target.read()] + 4;
        }
        p_vci_ini.wdata   = r_write_to_init_cmd_nline.read();
        p_vci_ini.be      = 0xF;
        if(r_init_cmd_inst.read()){
          p_vci_ini.plen    = 4 ;
          p_vci_ini.eop     = true;
        } else {
          p_vci_ini.plen    = 4 * (r_write_to_init_cmd_count.read() + 2);
          p_vci_ini.eop     = false;
        }
        p_vci_ini.trdid   = r_write_to_init_cmd_trdid.read();
        break;
      case INIT_CMD_UPDT_INDEX:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = m_coherence_table[r_init_cmd_target.read()] + 4;
        p_vci_ini.wdata   = r_write_to_init_cmd_index.read();
        p_vci_ini.be      = 0xF;
        p_vci_ini.plen    = 4 * (r_write_to_init_cmd_count.read() + 2);
        p_vci_ini.trdid   = r_write_to_init_cmd_trdid.read();
        p_vci_ini.eop     = false;
        break;
      case INIT_CMD_UPDT_DATA:
        p_vci_ini.cmdval  = true;
        p_vci_ini.address = m_coherence_table[r_init_cmd_target.read()] + 4;
        p_vci_ini.wdata   = r_write_to_init_cmd_data[r_init_cmd_cpt.read() +
          r_write_to_init_cmd_index.read()].read();
        if(r_write_to_init_cmd_we[r_init_cmd_cpt.read() +
            r_write_to_init_cmd_index.read()].read())  
          p_vci_ini.be      = 0xF;
        else			p_vci_ini.be      = 0x0;
        p_vci_ini.plen    = 4 * (r_write_to_init_cmd_count.read() + 2);
        p_vci_ini.trdid   = r_write_to_init_cmd_trdid.read();
        p_vci_ini.eop     = ( r_init_cmd_cpt.read() == (r_write_to_init_cmd_count.read()-1) );
        break;
    } // end switch r_init_cmd_fsm

    //////////////////////////////////////////////////////
    // Response signals on the p_vci_ini port
    //////////////////////////////////////////////////////

    if ( r_init_rsp_fsm.read() == INIT_RSP_IDLE ) p_vci_ini.rspack  = true;
    else                                          p_vci_ini.rspack  = false;

    //////////////////////////////////////////////////////
    // Response signals on the p_vci_tgt_cleanup port
    //////////////////////////////////////////////////////
    p_vci_tgt_cleanup.rspval = false;
    p_vci_tgt_cleanup.rsrcid = 0;
    p_vci_tgt_cleanup.rdata  = 0;
    p_vci_tgt_cleanup.rpktid = 0;
    p_vci_tgt_cleanup.rtrdid = 0;
    p_vci_tgt_cleanup.rerror = 0;
    p_vci_tgt_cleanup.reop   = false;

    switch(r_cleanup_fsm.read()){
      case CLEANUP_IDLE:
        {
          p_vci_tgt_cleanup.cmdack = true ;
          break;
        }
      case CLEANUP_DIR_LOCK:
        {
          p_vci_tgt_cleanup.cmdack = false ;
          break;
        }
      case CLEANUP_RSP:
        {
          p_vci_tgt_cleanup.rspval = true;
          p_vci_tgt_cleanup.rdata  = 0;
          p_vci_tgt_cleanup.rsrcid = r_cleanup_srcid.read();
          p_vci_tgt_cleanup.rpktid = r_cleanup_pktid.read();
          p_vci_tgt_cleanup.rtrdid = r_cleanup_trdid.read();
          p_vci_tgt_cleanup.rerror = 0;
          p_vci_tgt_cleanup.reop   = 1;
          break;
        }

    }

  } // end genMoore()

}} // end name space
