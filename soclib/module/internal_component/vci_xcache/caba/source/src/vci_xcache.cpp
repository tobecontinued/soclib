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
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *         Alain Greiner <alain.greiner@lip6.fr>, 2006
 *
 * Maintainers: nipo
 */

/////////////////////////////////////////////////////////////////////////////
// History
// - 11/17/2007
//   The ICACHE FSM, DCACHE FSM, and the VCI_RSP FSM have been modified
//   in order to handle both synchronous bus errors (read) and 
//   asynchronous bus errors (write)
//   The WRITE_BUFFER_DEPTH template parameter has been suppressed.
//   The "unc" signal has been supressed on the DCACHE interface,
//   and  replaced by a new code RU for the "type" signal.
//   The bool arrays DCACHE_VAL_BUF[] & ICACHE_VAL_BUF[] have been
//   suppressed. A simple flip-flop r_dcache_unc_valid has been created
//   to signal the uncached data availability, stored in r_dcache_miss_buf[0].
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include "arithmetics.h"
#include "alloc_elems.h"
#include "../include/vci_xcache.h"

namespace soclib { 
namespace caba {

#ifndef XCACHE_DEBUG
#define XCACHE_DEBUG 0
#endif

#define LINE_VALID 0x80000000

#define tmpl(x)  template<typename vci_param> x VciXCache<vci_param>

tmpl(inline bool)::is_write(soclib::caba::DCacheSignals::req_type_e cmd)
{
    switch(cmd) {
    case soclib::caba::DCacheSignals::WRITE_WORD:
    case soclib::caba::DCacheSignals::WRITE_HALF:
    case soclib::caba::DCacheSignals::WRITE_BYTE:
        // Dont put LL/SC/SW as they should not burst
        return true;
    default:
        return false;
    }
}

tmpl(inline bool)::can_burst(
        DCacheSignals::req_type_e old_type, addr_t old_addr,
        DCacheSignals::req_type_e new_type, addr_t new_addr )
{
              
    bool res = is_write(old_type) && is_write(new_type) &&
        ((addr_t)(new_addr&~(vci_param::B-1)) == old_addr&~(vci_param::B-1)+4);
//        !((next.addr^old.addr)&~4095);
    return res;
}

using soclib::common::uint32_log2;

tmpl(/**/)::VciXCache(
    sc_module_name name,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &index,
    size_t icache_lines,
    size_t icache_words,
    size_t dcache_lines,
    size_t dcache_words )
    : soclib::caba::BaseModule(name),

      p_clk("clk"),
      p_resetn("resetn"),
      p_icache("icache"),
      p_dcache("dcache"),
      p_vci("vci"),

      m_cacheability_table(mt.getCacheabilityTable()),
      m_ident(mt.indexForId(index)),

      s_dcache_lines(dcache_lines),
      s_dcache_words(dcache_words),
      s_icache_lines(icache_lines),
      s_icache_words(icache_words),

      m_i_x( uint32_log2(s_icache_words), uint32_log2(vci_param::B) ),
      m_i_y( uint32_log2(s_icache_lines), uint32_log2(s_icache_words) + uint32_log2(vci_param::B) ),
      m_i_z( vci_param::N-uint32_log2(s_icache_lines) - uint32_log2(s_icache_words) - uint32_log2(vci_param::B),
             uint32_log2(s_icache_lines) + uint32_log2(s_icache_words) + uint32_log2(vci_param::B) ),
      m_icache_yzmask((~0)<<(uint32_log2(s_icache_words) + uint32_log2(vci_param::B))),

      m_d_x( uint32_log2(s_dcache_words), uint32_log2(vci_param::B) ),
      m_d_y( uint32_log2(s_dcache_lines), uint32_log2(s_dcache_words) + uint32_log2(vci_param::B) ),
      m_d_z( vci_param::N-uint32_log2(s_dcache_lines) - uint32_log2(s_dcache_words) - uint32_log2(vci_param::B),
             uint32_log2(s_dcache_lines) + uint32_log2(s_dcache_words) + uint32_log2(vci_param::B) ),
      m_dcache_yzmask((~0)<<(uint32_log2(s_dcache_words) + uint32_log2(vci_param::B))),

      r_dcache_fsm("r_dcache_fsm"),
      r_dcache_addr_save("r_dcache_addr_save"),
      r_dcache_data_save("r_dcache_data_save"),
      r_dcache_prev_save("r_dcache_prev_save"),
      r_dcache_type_save("r_dcache_type_save"),
      r_dcache_cached_save("r_dcache_cached_save"),

      m_dreq_addr_fifo("m_dreq_addr_fifo", 8),
      m_dreq_data_fifo("m_dreq_data_fifo", 8),
      m_dreq_prev_fifo("m_dreq_prev_fifo", 8),
      m_dreq_type_fifo("m_dreq_type_fifo", 8),
      m_dreq_cached_fifo("m_dreq_cached_fifo", 8),

      r_icache_fsm("r_icache_fsm"),
      r_icache_miss_addr("r_icache_miss_addr"),
      r_icache_req("r_icache_req"),

      r_vci_cmd_fsm("r_vci_cmd_fsm"),
      r_dcache_addr_cmd("r_dcache_addr_cmd"),
      r_dcache_data_cmd("r_dcache_data_cmd"),
      r_dcache_prev_cmd("r_dcache_prev_cmd"),
      r_dcache_type_cmd("r_dcache_type_cmd"),
      r_dcache_cached_cmd("r_dcache_cached_cmd"),
      r_dcache_miss_addr("r_dcache_miss_addr"),
      r_cmd_cpt("r_cmd_cpt"),

      r_vci_rsp_fsm("r_vci_rsp_fsm"),
      r_dcache_unc_valid("r_dcache_unc_valid"),
      r_rsp_cpt("r_rsp_cpt"),

      r_dcache_cpt_init("r_dcache_cpt_init"),
      r_icache_cpt_init("r_icache_cpt_init")
{
    assert(IS_POW_OF_2(icache_lines));
    assert(IS_POW_OF_2(dcache_lines));
    assert(IS_POW_OF_2(icache_words));
    assert(IS_POW_OF_2(dcache_words));
    assert(icache_words);
    assert(dcache_words);
    assert(icache_lines);
    assert(dcache_lines);
    assert(icache_words <= 16);
    assert(dcache_words <= 16);
    assert(icache_lines <= 1024);
    assert(dcache_lines <= 1024);

    r_dcache_data = new sc_signal<data_t>*[dcache_lines];
    for ( size_t i=0; i<dcache_lines; ++i ) {
        std::ostringstream o;
        o << "dcache_data[" << i << "]";
        r_dcache_data[i] = soclib::common::alloc_elems<sc_signal<data_t> >(o.str(), dcache_words);
    }

    r_dcache_tag = soclib::common::alloc_elems<sc_signal<tag_t> >("dcache_tag", dcache_lines);

    r_icache_data = new sc_signal<data_t>*[icache_lines];
    for ( size_t i=0; i<icache_lines; ++i ) {
        std::ostringstream o;
        o << "icache_data[" << i << "]";
        r_icache_data[i] = soclib::common::alloc_elems<sc_signal<data_t> >(o.str(), icache_words);
    }

    r_icache_tag = soclib::common::alloc_elems<sc_signal<tag_t> >("icache_tag", icache_lines);

    r_icache_miss_buf = soclib::common::alloc_elems<sc_signal<data_t> >("icache_miss_buff", icache_words);;
    r_dcache_miss_buf = soclib::common::alloc_elems<sc_signal<data_t> >("dcache_miss_buff", dcache_words);

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();
  
    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();

    SC_METHOD (genMealy);
    dont_initialize();
    sensitive
        << p_clk.neg()
        << p_dcache.type
        << p_dcache.adr
        << p_dcache.req
        << p_icache.req
        << p_icache.adr;

#if 0 && defined(SYSTEMCASS_SPECIFIC)
    p_icache.frz  (p_icache.req);
    p_icache.ins  (p_icache.req);
    p_icache.frz  (p_icache.adr);
    p_icache.ins  (p_icache.adr);
    p_dcache.frz  (p_dcache.type);
    p_dcache.frz  (p_dcache.adr);
    p_dcache.frz  (p_dcache.req);
    p_dcache.rdata(p_dcache.adr);
#endif

    portRegister( "clk", p_clk );
    portRegister( "resetn", p_resetn );
    portRegister( "icache", p_icache );
    portRegister( "dcache", p_dcache );
    portRegister( "vci", p_vci );
}

tmpl(void)::transition()
{
    if ( ! p_resetn.read() ) {
        r_dcache_fsm = DCACHE_INIT;
        r_icache_fsm = ICACHE_INIT;
        r_vci_cmd_fsm = CMD_IDLE;
        r_vci_rsp_fsm = RSP_IDLE;

        r_dcache_cpt_init = s_dcache_lines - 1;
        r_icache_cpt_init = s_icache_lines - 1;
        m_dreq_addr_fifo.init();
        m_dreq_data_fifo.init();
        m_dreq_prev_fifo.init();
        m_dreq_type_fifo.init();
        m_dreq_cached_fifo.init();

        r_icache_req = false;

        r_dcache_unc_valid = false;

        m_cpt_dcache_data_read  = 0;
        m_cpt_dcache_data_write = 0;
        m_cpt_dcache_dir_read  = 0;
        m_cpt_dcache_dir_write = 0;
        m_cpt_icache_data_read  = 0;
        m_cpt_icache_data_write = 0;
        m_cpt_icache_dir_read  = 0;
        m_cpt_icache_dir_write = 0;
        m_cpt_fifo_read  = 0;
        m_cpt_fifo_write = 0;

        return;
    }

    const bool icache_req = p_icache.req.read();
    const addr_t icache_address = p_icache.adr.read();

    const int icache_y = m_i_y[icache_address];
    const tag_t icache_z = m_i_z[icache_address] | LINE_VALID;
    const bool icache_hit = (icache_z == r_icache_tag[icache_y]);

    const DCacheSignals::req_type_e dcache_type =
        (DCacheSignals::req_type_e)(int)p_dcache.type.read();
    const bool dcache_req = p_dcache.req.read();
    const addr_t dcache_address  = p_dcache.adr.read();

    const int dcache_x = m_d_x[dcache_address];
    const int dcache_y = m_d_y[dcache_address];
    const tag_t dcache_z = m_d_z[dcache_address] | LINE_VALID;
    const bool dcache_unc_hit = (r_dcache_unc_valid.read() && dcache_address == r_dcache_miss_addr); 
    const bool dcache_hit = (dcache_z == r_dcache_tag[dcache_y]);
    
    bool    fifo_put_dcache_save = false;
    bool    fifo_get = false;
#if XCACHE_DEBUG
    std::cout
        << name()
        << " dcache: " << r_dcache_fsm
        << " icache: " << r_icache_fsm
        << " cmd: " << r_vci_cmd_fsm
        << " rsp: " << r_vci_rsp_fsm
        << std::endl;
#endif

    /////////////////////////////////////////////////////////////////////
    // The ICACHE FSM controls the following ressources:
    // - r_icache_fsm
    // - r_icache_data[s_icache_words,s_icache_lines]
    // - r_icache_tag[s_icache_lines]
    // - r_icache_miss_addr
    // - r_icache_req set
    // - r_icache_cpt_init
    //
    // The VALID bit for a cache line is the MSB bit in the TAG.
    //
    // Only cached read (RI) requests are supported.
    // Invalidate (RZ) and Uncached read (RU) are not supported.
    //
    // In case of MISS, the controller writes a request in the
    // r_icache_miss_addr register and sets the r_icache_req flip-flop.
    // The r_icache_req flip-flop is reset by the VCI_RSP controller,
    // when the cache line is ready in the ICACHE buffer.
    //
    // Error handling : Instruction Bus Errors are synchronous events.
    // The p_icache.berr and p_icache.frz signals are fully controled 
    // by the ICACHE FSM.
    // If a bus error is detected by the vci_rsp_fsm, the ICACHE FSM
    // goes to the ICACHE_ERROR state, ans set the signals:
    // - p_icache.berr = true
    // - p_icache.frz = false
    ///////////////////////////////////////////////////////////////////////

    switch((icache_fsm_state_e)r_icache_fsm.read()) {

    case ICACHE_INIT:
        r_icache_tag[r_icache_cpt_init] = 0;
        r_icache_cpt_init = r_icache_cpt_init - 1;
        if (r_icache_cpt_init == 0)
            r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
        break;
    
    case ICACHE_IDLE:
        if ( icache_req ) {
            if ( ! icache_hit ) {
                r_icache_fsm = ICACHE_WAIT;
                r_icache_miss_addr = icache_address & m_icache_yzmask;
                r_icache_req = true;
            }
            m_cpt_icache_dir_read++;
            m_cpt_icache_data_read++;
        }
        break;

    case ICACHE_WAIT:
        if (r_vci_rsp_fsm.read() == RSP_INS_OK)
            r_icache_fsm = ICACHE_UPDT;
        if (r_vci_rsp_fsm.read() == RSP_INS_ERROR)
            r_icache_fsm = ICACHE_ERROR;
        break;

    case ICACHE_ERROR:
#if XCACHE_DEBUG
        std::cout << name() << " ins berr &" << r_icache_miss_addr.read() << std::endl;
#endif
        r_icache_fsm = ICACHE_IDLE;
        break;

    case ICACHE_UPDT:
        r_icache_tag[icache_y] = icache_z;
        for (size_t i=0 ; i<s_icache_words ; i++)
            r_icache_data[icache_y][i] = r_icache_miss_buf[i];
        r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
        m_cpt_icache_data_write++;
        break;
    
    } // end switch r_icache_fsm

    //////////////////////////////////////////////////////////////////////://///////////
    // The DCACHE FSM controls the following ressources:
    // - r_dcache_fsm
    // - r_dcache_data[s_dcache_words,s_dcache_lines]
    // - r_dcache_tag[s_dcache_lines]
    // - r_dcache_save
    // - r_dcache_cpt_init
    // - r_dcache_unc_valid reset
    // - fifo_put_dcache_save, dreq
    //
    // The VALID bit for a cache line is the MSB bit in the TAG.
    // In the IDLE state, the processor request is saved in r_dcache_save.
    // The data  read in the cache is saved in r_dcache_save.prev.
    // The request type takes into account the cacheability_table.
    //
    // There is five mutually exclusive conditions to exit the IDLE state:
    // - CACHED READ MISS => to the MISS_REQ state (to post the request in the FIFO),
    // then to the MISS_WAIT state (waiting the cache line), then to the MISS_UPDT
    // (to update the cache), and finally to the IDLE state.
    // - UNCACHED READ  => to the UNC_REQ state (to post the request in the FIFO), 
    // then to the UNC_WAIT state, and finally to the IDLE state.
    // - CACHE INVALIDATE HIT => to the INVAL state for one cycle, then IDLE.
    // - WRITE MISS => directly to the WRITE_REQ state (to post the request in the FIFO)
    // Then it depends on the processor request: In order to support VCI write burst,
    // the processor requests are taken into account in the WRITE_REQ state
    // as well as in the IDLE state.
    // - WRITE HIT => to the WRITE_UPDT state (to update the cache), then to
    // the WRITE_REQ state.
    //
    // Error handling :  Read Data Bus Errors are synchronous events (processor frozen).
    // Write Data Bus Errors are asynchronous events (processor is not frozen).
    // The p_dcache.berr signal is  controled by both the DCACHE FSM and the VCI_RSP FSM:
    // If a Read Bus Error is detected, the DCACHE FSM goes to the DCACHE_ERROR state, 
    // and set the signals:
    // - p_dcache.berr = true
    // - p_dcache.frz = false
    // If a Write Bus Error is detected, the VCI_RSP FSM  set the signal:
    // - p_dcache.berr = true
    ///////////////////////////////////////////////////////////////////////////////////

    switch ((dcache_fsm_state_e)r_dcache_fsm.read()) {

    case DCACHE_INIT:
        r_dcache_tag[r_dcache_cpt_init] = 0;
        r_dcache_cpt_init = r_dcache_cpt_init - 1;
        if (r_dcache_cpt_init == 0)
            r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
        break;

    case DCACHE_WRITE_REQ:
        fifo_put_dcache_save = true;

        if (! m_dreq_addr_fifo.wok())
            break;
            
        if (! dcache_req) {
                r_dcache_fsm = DCACHE_IDLE;
                break;
        }
        m_cpt_fifo_write++;
    case DCACHE_IDLE:
    {
        addr_t dr_addr;
        data_t dr_data;
        data_t dr_prev;
        DCacheSignals::req_type_e dr_type;
        bool dr_cached;
        if (! dcache_req)
                break;

        m_cpt_dcache_data_read++;
        m_cpt_dcache_dir_read++;
        
        dr_addr = dcache_address;
        dr_data = p_dcache.wdata.read();
        dr_type = dcache_type;
        dr_prev = r_dcache_data[dcache_y][dcache_x];
        dr_cached = m_cacheability_table[dcache_address];

        switch(dcache_type) {
        case DCacheSignals::READ_WORD:
        case DCacheSignals::READ_HALF:
        case DCacheSignals::READ_BYTE:
            if ( dr_cached ) {
                if ( ! dcache_hit )
                    r_dcache_fsm = DCACHE_MISS_REQ;
                else
                    r_dcache_fsm = DCACHE_IDLE;
                break;
            }
        case DCacheSignals::READ_LINKED:
        case DCacheSignals::STORE_COND:
            if ( dcache_unc_hit ) {
#if XCACHE_DEBUG
                std::cout << name() << " Unc hit" << std::endl;
#endif
                r_dcache_unc_valid = false;
            } else {
#if XCACHE_DEBUG
                std::cout << name() << " Unc miss" << std::endl;
#endif
                r_dcache_fsm = DCACHE_UNC_REQ;
            }
            break;
        case DCacheSignals::LINE_INVAL:
            if ( dcache_hit )
                r_dcache_fsm = DCACHE_INVAL;
            else
                r_dcache_fsm = DCACHE_IDLE;
            break;
        case DCacheSignals::WRITE_WORD:
        case DCacheSignals::WRITE_HALF:
        case DCacheSignals::WRITE_BYTE:
            if (dcache_hit)
                r_dcache_fsm = DCACHE_WRITE_UPDT;
            else
                r_dcache_fsm = DCACHE_WRITE_REQ;
            break;
        }

        r_dcache_addr_save = dr_addr;
        r_dcache_data_save = dr_data;
        r_dcache_prev_save = dr_prev;
        r_dcache_type_save = dr_type;
        r_dcache_cached_save = dr_cached;
        break;
    }

    case DCACHE_WRITE_UPDT:
    {
        addr_t req_addr = r_dcache_addr_save.read();
        data_t req_data = r_dcache_data_save.read();
        data_t req_prev = r_dcache_prev_save.read();
        DCacheSignals::req_type_e req_type = 
            (DCacheSignals::req_type_e)r_dcache_type_save.read();
        const int x = m_d_x[req_addr];
        const int y = m_d_y[req_addr];
        const int byte = req_addr & 0x3;
        switch(req_type) {
        case DCacheSignals::WRITE_WORD:
            r_dcache_data[y][x] = req_data;
            break;
        case DCacheSignals::WRITE_HALF:
        {
            data_t mask = 0xffff << (byte*8);
            data_t new_data = req_data << (byte*8);
            r_dcache_data[y][x] =
                (req_prev & ~mask) |
                (new_data &  mask) ;
            break;
        }
        case DCacheSignals::WRITE_BYTE:
        {
            data_t mask = 0xff << (byte*8);
            data_t new_data = req_data << (byte*8);
            r_dcache_data[y][x] =
                (req_prev & ~mask) |
                (new_data &  mask);
            break;
        }
        default:
            assert(!"There should be nothing but write requests in DCACHE_WRITE_UPDT");
        } // end switch 
        m_cpt_dcache_data_write++;
        r_dcache_fsm = DCACHE_WRITE_REQ;
        break;
    }

    case DCACHE_MISS_REQ:
        fifo_put_dcache_save = true;
        if (m_dreq_addr_fifo.wok()) {
            r_dcache_fsm = DCACHE_MISS_WAIT;
            m_cpt_fifo_write++;
        }
        break;

    case DCACHE_MISS_WAIT:
        if (r_vci_rsp_fsm == RSP_DATA_READ_ERROR)
            r_dcache_fsm = DCACHE_ERROR;
        if (r_vci_rsp_fsm == RSP_DATA_MISS_OK)
            r_dcache_fsm = DCACHE_MISS_UPDT;
        break;

    case DCACHE_MISS_UPDT:
    {
        const int y = m_d_y[r_dcache_miss_addr];
        const tag_t z = m_d_z[r_dcache_miss_addr] | LINE_VALID;
        r_dcache_tag[y] = z;
        for (size_t i=0 ; i<s_dcache_words ; i++)
            r_dcache_data[y][i] = r_dcache_miss_buf[i];
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        break;
    }

    case DCACHE_UNC_REQ:
        fifo_put_dcache_save = true;
        if (m_dreq_addr_fifo.wok()) {
            r_dcache_fsm = DCACHE_UNC_WAIT;
            m_cpt_fifo_write++;
        }
        break;

    case DCACHE_UNC_WAIT:
        if (r_vci_rsp_fsm == RSP_DATA_READ_ERROR)
            r_dcache_fsm = DCACHE_ERROR;
        if (r_vci_rsp_fsm == RSP_DATA_UNC_OK)
            r_dcache_fsm = DCACHE_IDLE;
        break;

    case DCACHE_ERROR:
#if XCACHE_DEBUG
        std::cout << name() << " data berr " << std::endl;
#endif
        r_dcache_fsm = DCACHE_IDLE;
        break;
        
    case DCACHE_INVAL:
        r_dcache_tag[m_d_y[r_dcache_addr_save.read()]] = 0;
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
        break;

    } // end switch r_dcache_fsm

    ////////////////////////////////////////////////////////////////////////////
    // The VCI_CMD FSM controls the following ressources:
    // - r_vci_cmd_fsm
    // - r_dcache_cmd
    // - r_dcache_miss_addr
    // - r_cmd_cpt
    // - fifo_get
    //
    // This FSM handles requests from both the DCACHE controler
    // (m_dreq_*_fifo non empty) and the ICACHE controler (r_icache_req).
    // There is  4 VCI transaction types :
    // - INS_MISS
    // - DATA_MISS
    // - DATA_UNC 
    // - DATA_WRITE
    // The ICACHE requests have the highest priority.
    // There is at most one (CMD/RSP) VCI transaction, as both CMD_FSM and RSP_FSM
    // exit simultaneously the IDLE state.
    // In case of successive write at consecutive addressses, this FSM buids
    // write burst of variable lengths:
    // A request is consumed from the m_dreq_*_fifo each time the CMD and RSP FSMs
    // are in IDLE state, or the CMD FSM is in CMD_DATA_WRITE state, and there is
    // another write request at address + 4.
    //////////////////////////////////////////////////////////////////////////////

    switch ((cmd_fsm_state_e)r_vci_cmd_fsm.read()) {
    
    case CMD_IDLE:
        if (r_vci_rsp_fsm != RSP_IDLE)
            break;

        r_cmd_cpt = 0;
        if (r_icache_req.read()) {
            r_vci_cmd_fsm = CMD_INS_MISS;
        } else if (m_dreq_addr_fifo.rok()) {
            addr_t req_addr = m_dreq_addr_fifo.read();
            data_t req_data = m_dreq_data_fifo.read();
            data_t req_prev = m_dreq_prev_fifo.read();
            DCacheSignals::req_type_e req_type = 
                (DCacheSignals::req_type_e)m_dreq_type_fifo.read();
            bool req_cached = m_dreq_cached_fifo.read();
            r_dcache_addr_cmd = req_addr;
            r_dcache_data_cmd = req_data;
            r_dcache_prev_cmd = req_prev;
            r_dcache_type_cmd = req_type;
            r_dcache_cached_cmd = req_cached;
            m_cpt_fifo_read++;
            fifo_get = true;
                
            switch(req_type) {
            case DCacheSignals::READ_WORD:
            case DCacheSignals::READ_HALF:
            case DCacheSignals::READ_BYTE:
                if ( req_cached ) {
                    r_vci_cmd_fsm = CMD_DATA_MISS;
                    break;
                }
            case DCacheSignals::STORE_COND:
            case DCacheSignals::READ_LINKED:
                r_vci_cmd_fsm = CMD_DATA_UNC;
                break;
            case DCacheSignals::WRITE_WORD:
            case DCacheSignals::WRITE_HALF:
            case DCacheSignals::WRITE_BYTE:
                r_vci_cmd_fsm = CMD_DATA_WRITE;
                break;
            case DCacheSignals::LINE_INVAL:
                assert(0&&"This should not happen");
            }
        }
        break;

    case CMD_INS_MISS:
        if ( p_vci.cmdack.read() ) {
            r_cmd_cpt = r_cmd_cpt + 1;
            if (r_cmd_cpt == s_icache_words - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DATA_UNC:
        r_dcache_miss_addr = r_dcache_addr_cmd.read();
        if ( p_vci.cmdack.read() )
            r_vci_cmd_fsm = CMD_IDLE;
        break;

    case CMD_DATA_MISS:
        r_dcache_miss_addr = r_dcache_addr_cmd.read();
        if ( p_vci.cmdack.read() ) {
            r_cmd_cpt = r_cmd_cpt + 1;
            if (r_cmd_cpt == s_dcache_words - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DATA_WRITE:
        if ( ! p_vci.cmdack.read() )
            break;

        if ( m_dreq_addr_fifo.rok() &&
             can_burst(
                 (DCacheSignals::req_type_e)
                 m_dreq_type_fifo.read(),
                 m_dreq_addr_fifo.read(),
                 (DCacheSignals::req_type_e)
                 r_dcache_type_cmd.read(),
                 r_dcache_addr_cmd.read() ) ) {
            fifo_get = true;
            r_dcache_addr_cmd = m_dreq_addr_fifo.read();
            r_dcache_data_cmd = m_dreq_data_fifo.read();
            r_dcache_prev_cmd = m_dreq_prev_fifo.read();
            r_dcache_type_cmd = m_dreq_type_fifo.read();
            r_dcache_cached_cmd = m_dreq_cached_fifo.read();
        } else {
            r_vci_cmd_fsm = CMD_IDLE;
        }
        break;
    } // end  switch r_vci_cmd_fsm

    //////////////////////////////////////////////////////////////////////////
    //  m_dreq_*_fifo
    //  This FIFO is used as a write buffer and contains the requests from
    //  the DCACHE controler to the VCI controler.
    //  It is controlled by the fifo_put_dcache_save signal (defined by r_dcache_fsm)
    //  and the fifo_get signal (defined by r_vci_cmd_fsm)
    //////////////////////////////////////////////////////////////////////////

    if ( fifo_put_dcache_save ) {
        if ( fifo_get ) {
            m_dreq_addr_fifo.put_and_get(r_dcache_addr_save);
            m_dreq_data_fifo.put_and_get(r_dcache_data_save);
            m_dreq_prev_fifo.put_and_get(r_dcache_prev_save);
            m_dreq_type_fifo.put_and_get(r_dcache_type_save);
            m_dreq_cached_fifo.put_and_get(r_dcache_cached_save);
        } else {
            m_dreq_addr_fifo.simple_put(r_dcache_addr_save);
            m_dreq_data_fifo.simple_put(r_dcache_data_save);
            m_dreq_prev_fifo.simple_put(r_dcache_prev_save);
            m_dreq_type_fifo.simple_put(r_dcache_type_save);
            m_dreq_cached_fifo.simple_put(r_dcache_cached_save);
        }
    } else {
        if ( fifo_get ) {
            m_dreq_addr_fifo.simple_get();
            m_dreq_data_fifo.simple_get();
            m_dreq_prev_fifo.simple_get();
            m_dreq_type_fifo.simple_get();
            m_dreq_cached_fifo.simple_get();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // The VCI_RSP FSM controls the following ressources:
    // - r_vci_rsp_fsm:
    // - r_icache_miss_buf[s_icache_words]
    // - r_dcache_miss_buf[s_dcache_words]
    // - r_dcache_unc_valid set
    // - r_icache_req reset
    // - CPT_RSP
    //
    // This FSM is synchronized with the VCI_CMD FSM, as both FSMs exit the
    // IDLE state simultaneously.
    //
    // Error handling:
    // This FSM analyzes the VCI error code and activates directly the
    // p_dcache.berr signal during one cycle, in case of Write Bus Error.
    // In case of Read Bus Error, the VCI_RSP FSM goes to the RSP_INS_ERROR
    // or RSP_DATA_READ_ERROR state, and the error is signaled by the 
    // ICACHE or DCACHE FSM.
    //////////////////////////////////////////////////////////////////////////

    switch ((rsp_fsm_state_e)r_vci_rsp_fsm.read()) {

    case RSP_IDLE:
        assert( ! p_vci.rspval.read() && "Unexpected response" );

        if (r_vci_cmd_fsm != CMD_IDLE)
            break;

        r_rsp_cpt = 0;
        if (r_icache_req.read()) {
            r_vci_rsp_fsm = RSP_INS_MISS;
        } else if (m_dreq_addr_fifo.rok()) {
            DCacheSignals::req_type_e req_type =
                (DCacheSignals::req_type_e)m_dreq_type_fifo.read();
            bool req_cached = m_dreq_cached_fifo.read();

            switch (req_type) {
            case DCacheSignals::READ_WORD:
            case DCacheSignals::READ_HALF:
            case DCacheSignals::READ_BYTE:
                if ( req_cached ) {
                    r_vci_rsp_fsm = RSP_DATA_MISS;
                    break;
                }
            case DCacheSignals::READ_LINKED:
            case DCacheSignals::STORE_COND:
                r_vci_rsp_fsm = RSP_DATA_UNC;
                break;
            case DCacheSignals::WRITE_WORD:
            case DCacheSignals::WRITE_HALF:
            case DCacheSignals::WRITE_BYTE:
                r_vci_rsp_fsm = RSP_DATA_WRITE;
                break;
            case DCacheSignals::LINE_INVAL:
                assert(0&&"This should not happen");
                break;
            }
        }
        break;

    case RSP_INS_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        assert(r_rsp_cpt != s_icache_words && 
               "illegal VCI response packet for instruction miss");
        r_rsp_cpt = r_rsp_cpt + 1;
        r_icache_miss_buf[r_rsp_cpt] = (data_t)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == vci_param::ERR_NORMAL ) {
                r_vci_rsp_fsm = RSP_INS_OK;
                assert(r_rsp_cpt == s_icache_words - 1 &&
                       "illegal VCI response packet for instruction miss");
            } else {
                r_vci_rsp_fsm = RSP_INS_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_INS_ERROR_WAIT;
        }
        break;

    case RSP_INS_OK:
        r_vci_rsp_fsm = RSP_IDLE;
        r_icache_req = false;
        break;

    case RSP_INS_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;
        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_INS_ERROR;
        break;

    case RSP_INS_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
        r_icache_req = false;
        break;

    case RSP_DATA_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        assert(r_rsp_cpt != s_dcache_words &&
               "illegal VCI response packet for data read miss");

        r_rsp_cpt = r_rsp_cpt + 1;
        r_dcache_miss_buf[r_rsp_cpt] = (data_t)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == vci_param::ERR_NORMAL ) {
                r_vci_rsp_fsm = RSP_DATA_MISS_OK;
                assert(r_rsp_cpt == s_dcache_words - 1 &&
                       "illegal VCI response packet for data read miss");
            } else {
                r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_DATA_READ_ERROR_WAIT;
        }
        break;

    case RSP_DATA_WRITE:
        if ( ! p_vci.rspval.read() )
            break;

#if XCACHE_DEBUG
        std::cout << name() << " RSP_DATA_WRITE err:" << p_vci.rerror.read() << std::endl;
#endif
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_IDLE;
            else
                r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
        } else {
            if ( p_vci.rerror.read() != vci_param::ERR_NORMAL )
                r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR_WAIT;
        }
        break;

    case RSP_DATA_UNC:
        if ( ! p_vci.rspval.read() )
            break;

        assert(p_vci.reop.read() &&
               "illegal VCI response packet for data read uncached");

        if ( p_vci.rerror.read() == vci_param::ERR_NORMAL ) {
            r_dcache_miss_buf[0] = (data_t)p_vci.rdata.read();
            r_dcache_unc_valid = true;
            r_vci_rsp_fsm = RSP_DATA_UNC_OK;
        } else {
            r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
        }
        break;

    case RSP_DATA_WRITE_ERROR:
#if XCACHE_DEBUG
        std::cout << name() << " data WRITE berr" << std::endl;
#endif
    case RSP_DATA_UNC_OK:
    case RSP_DATA_MISS_OK:
    case RSP_DATA_READ_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_READ_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
        break;

    case RSP_DATA_WRITE_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
        break;
    } // end switch r_vci_rsp_fsm
}

//////////////////////////////////////////////////////////////////////////////////
//   genMoore method
//
//   The Moore signals are  p_vci ,  p_icache.berr & p_dcache.berr
//////////////////////////////////////////////////////////////////////////////////
tmpl(void)::genMoore()
{
    // p_vci.rspack, p_icache.berr & p_dcache.berr

    p_vci.rspack = true;
    p_icache.berr = (r_icache_fsm == ICACHE_ERROR);
    p_dcache.berr = (r_dcache_fsm == DCACHE_ERROR || 
                    r_vci_rsp_fsm == RSP_DATA_WRITE_ERROR);

    // VCI CMD

    addr_t req_addr = r_dcache_addr_cmd.read();
    data_t req_data = r_dcache_data_cmd.read();
    DCacheSignals::req_type_e req_type =
        (DCacheSignals::req_type_e)r_dcache_type_cmd.read();
    const int subcell = req_addr & 0x3;

    switch ((cmd_fsm_state_e)r_vci_cmd_fsm.read()) {

    case CMD_IDLE:
        p_vci.cmdval  = false;
        break;

    case CMD_DATA_UNC:
        p_vci.cmdval = true;
        p_vci.address = req_addr & ~0x3;
        switch(req_type) {
        case DCacheSignals::READ_WORD:
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_READ;
            break;
        case DCacheSignals::READ_HALF:
            p_vci.be  = 3 << subcell;
            p_vci.cmd = vci_param::CMD_READ;
            break;
        case DCacheSignals::READ_BYTE:
            p_vci.be  = 1 << subcell;
            p_vci.cmd = vci_param::CMD_READ;
            break;
        case DCacheSignals::READ_LINKED:
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_LOCKED_READ;
            break;
        case DCacheSignals::STORE_COND:
            p_vci.wdata = req_data;
            p_vci.be  = 0xF;
            p_vci.cmd = vci_param::CMD_STORE_COND;
            break;
        default:
            assert(0);
        }
        p_vci.plen = 0;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_ident;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = false;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop    = true;
        break;

    case CMD_DATA_WRITE:
        p_vci.cmdval = true;
        p_vci.address = req_addr & ~0x3;
        p_vci.wdata   = req_data << subcell*8;

        switch(req_type) {
        case DCacheSignals::WRITE_WORD:
            p_vci.be      = 0xF;
            break;
        case DCacheSignals::WRITE_HALF:
            p_vci.be      = 3 << subcell;
            break;
        case DCacheSignals::WRITE_BYTE:
            p_vci.be      = 1 << subcell;
            break;
        default:
            assert(0);
        }
        p_vci.plen   = 0;
        p_vci.cmd    = vci_param::CMD_WRITE;
        p_vci.trdid  = 0;
        p_vci.pktid  = WRITE_PKTID;
        p_vci.srcid  = m_ident;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = false;
        p_vci.clen   = 0;
        p_vci.cfixed = false;

        p_vci.eop = ! (
            m_dreq_addr_fifo.rok() &&
            can_burst(
                (DCacheSignals::req_type_e)
                m_dreq_type_fifo.read(),
                m_dreq_addr_fifo.read(),
                req_type, req_addr )
            );
        break;

    case CMD_DATA_MISS:
        p_vci.cmdval = true;
        p_vci.address = (req_addr & m_dcache_yzmask) + (r_cmd_cpt << 2);
        p_vci.be     = 0xF;
        p_vci.plen   = 0;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_ident;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = (r_cmd_cpt == s_dcache_words - 1);
        break;

    case CMD_INS_MISS:
        p_vci.cmdval = true;
        p_vci.address = (r_icache_miss_addr & m_icache_yzmask) + (r_cmd_cpt << 2);
        p_vci.be     = 0xF;
        p_vci.plen   = 0;
        p_vci.cmd    = vci_param::CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_ident;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = (r_cmd_cpt == s_icache_words - 1);
        break;

    } // end switch r_vci_cmd_fsm
}

//////////////////////////////////////////////////////////////////////////////////
//   genMealy method
//
// The Mealy signals are p_icache.ins, p_icache.frz, p_dcache.frz, p_dcache.rdata
//
// DCACHE
// The processor requests are taken into account only in the DCACHE_IDLE
// and DCACHE_WRITE_REQ states.
// The p_dcache.frz signal is activated only if there is a processor
// request, and depends on the DCACHE FSM states:
// - In the IDLE state, p_dcache.frz is true when there is a cached read miss,
//   or an uncached read miss.
// - In the WRITE_REQ state, p_dcache.frz is true when there is a cached read miss,
//   or an uncached read miss, or when the m_dreq_*_fifo is full.
// - p_dcache.frz is true in all other states.
// The p_dcache.rdata signal is read in r_dcache_miss_buf[0] in case
// of an uncached read, and is read in the cache in all other cases.
//
// ICACHE
// The p_icache.frz signal is activated only if there is a processor
// request. It depends on the ICACHE FSM states:
// - In the IDLE state, the p_icache.frz signal depends on the directory comparison,
// - In the ERROR state, the p_icache.frz signal is always false
// - In all others states, the p_icache.frz signal is true
///////////////////////////////////////////////////////////////////////////////////

tmpl(void)::genMealy()
{
    /////////  p_icache.frz & p_icache.ins

    if ( p_icache.req.read() ) {
        switch((icache_fsm_state_e)r_icache_fsm.read()) {
        case ICACHE_IDLE:
        {
            const addr_t icache_address = (addr_t)p_icache.adr.read();
            const int x = m_i_x[icache_address];
            const int y = m_i_y[icache_address];
            const tag_t z = m_i_z[icache_address];
            p_icache.frz = (z | LINE_VALID) != r_icache_tag[y];
            p_icache.ins = (data_t)r_icache_data[y][x];
            break;
        }
        case ICACHE_ERROR:
            p_icache.frz = false;
            p_icache.ins = 0;
            break;
        default:
            p_icache.frz = true;
            p_icache.ins = 0;
            break;
        }
    } else {
        p_icache.frz = false;
        p_icache.ins = 0;
    }

    ////////// p_dcache.frz & p_dcache.rdata

    if ( p_dcache.req.read() ) {

        //  dcache_hit & dcache_unc_hit
        const addr_t dcache_address = p_dcache.adr.read();
        const addr_t dcache_subcell = dcache_address & 0x3;
        const int x = m_d_x[dcache_address];
        const int y = m_d_y[dcache_address];
        const tag_t z = m_d_z[dcache_address];
        const bool dcache_hit = (z | LINE_VALID) == r_dcache_tag[y];
        const bool dcache_unc_hit = (r_dcache_unc_valid &&
                                     dcache_address == r_dcache_miss_addr);

        // Sanity check
        if (r_dcache_unc_valid.read())
            if (dcache_address != r_dcache_miss_addr) {
                std::cout << name() << "Assertion failed: "
                          << "CPU changed requested address on DCACHE during uncached read"
                          << " old addr: " << std::hex << (uint32_t)r_dcache_miss_addr
                          << " new addr: " << (uint32_t)dcache_address
                          << std::endl;
                abort();
            }

        // if the write buffer is not full, we must have the same behaviour
        // in the DCACHE_WRITE_REQ state as in the IDLE state.
        // Freeze CPU in all other states.
        switch ((dcache_fsm_state_e)r_dcache_fsm.read()) {
        case DCACHE_ERROR:
            p_dcache.frz = false;
            p_dcache.rdata = 0;
            break;
        case DCACHE_WRITE_REQ:
            if ( ! m_dreq_addr_fifo.wok() ) {
                // write buffer full
                p_dcache.frz = true;
                p_dcache.rdata = 0;
                break;
            }
        case DCACHE_IDLE:
            switch((DCacheSignals::req_type_e)(int)p_dcache.type.read()) {
            case DCacheSignals::READ_HALF: {
                uint32_t d;
                if (m_cacheability_table[dcache_address]) {
                    p_dcache.frz = ! dcache_hit;
                    d = (data_t)r_dcache_data[y][x];
                } else {
                    d = (data_t)r_dcache_miss_buf[0];
                    p_dcache.frz = ! dcache_unc_hit;
                }
                d = 0xffff&(d>>(8*dcache_subcell));
                p_dcache.rdata = d | (d<<16);
                break;
            }
            case DCacheSignals::READ_BYTE: {
                uint32_t d;
                if (m_cacheability_table[dcache_address]) {
                    p_dcache.frz = ! dcache_hit;
                    d = (data_t)r_dcache_data[y][x];
                } else {
                    d = (data_t)r_dcache_miss_buf[0];
                    p_dcache.frz = ! dcache_unc_hit;
                }
                d = 0xff&(d>>(8*dcache_subcell));
                p_dcache.rdata = d | (d<<8) | (d<<16) | (d<<24);
                break;
            }
            case DCacheSignals::READ_WORD:
                if (m_cacheability_table[dcache_address]) {
                    p_dcache.frz = ! dcache_hit;
                    p_dcache.rdata = (data_t)r_dcache_data[y][x];
                    break;
                } // else uncacheable, fallback to uncached read case
            case DCacheSignals::READ_LINKED:
            case DCacheSignals::STORE_COND:
                p_dcache.frz = ! dcache_unc_hit;
                p_dcache.rdata = (data_t)r_dcache_miss_buf[0];
                break;
            case DCacheSignals::WRITE_WORD:
            case DCacheSignals::WRITE_HALF:
            case DCacheSignals::WRITE_BYTE:
            case DCacheSignals::LINE_INVAL:
                p_dcache.frz = false;
                p_dcache.rdata = 0xffffffff;
                break;
            }
            break;

        default:
            p_dcache.frz = true;
            p_dcache.rdata = 0;
            break;
        }
    } else {
        p_dcache.frz = false;
        p_dcache.rdata = 0;
    }

}

tmpl(XCacheInfo)::getCacheInfo() const
{
    struct XCacheInfo info;
    info.icache.line_bytes = 4*s_icache_words;
    info.icache.associativity = 1;
    info.icache.n_lines = s_icache_lines;
    info.dcache.line_bytes = 4*s_dcache_words;
    info.dcache.associativity = 1;
    info.dcache.n_lines = s_dcache_lines;
    return info;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

