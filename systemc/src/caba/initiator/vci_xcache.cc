/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include "caba/initiator/vci_xcache.h"

namespace soclib {
namespace caba {

static inline bool xcache_is_write(soclib::caba::DCacheSignals::req_type_e cmd)
{
    switch(cmd) {
    case soclib::caba::DCacheSignals::WW:
    case soclib::caba::DCacheSignals::WH:
    case soclib::caba::DCacheSignals::WB:
        return true;
    default:
        return false;
    }
}

static inline bool xcache_can_burst(
    soclib::caba::DCacheSignals::req_type_e old_cmd, uint32_t old_addr,
    soclib::caba::DCacheSignals::req_type_e new_cmd, uint32_t new_addr)
{
//     return xcache_is_write(old_cmd) && xcache_is_write(new_cmd) &&
//         ((old_addr & 0xfffff000) == (new_addr & 0xfffff000));
    return xcache_is_write(old_cmd) && xcache_is_write(new_cmd) &&
        new_addr == old_addr+4;
}

enum dcache_fsm_state_e {
    DCACHE_INIT,
    DCACHE_IDLE,
    DCACHE_WRITE_UPDT,
    DCACHE_WRITE_REQ,
    DCACHE_MISS_REQ,
    DCACHE_MISS_WAIT,
    DCACHE_MISS_UPDT,
    DCACHE_UNC_REQ,
    DCACHE_UNC_WAIT,
    DCACHE_INVAL,
};

enum icache_fsm_state_e {
    ICACHE_INIT,
    ICACHE_IDLE,
    ICACHE_WAIT,
    ICACHE_UPDT,
};

enum cmd_fsm_state_e {
    CMD_IDLE,
    CMD_DATA_LINE,
    CMD_DATA_WORD,
    CMD_DATA_WRITE,
    CMD_INS_MISS,
};

enum rsp_fsm_state_e {
    RSP_IDLE,
    RSP_INS_MISS,
    RSP_INS_ERWAIT,
    RSP_INS_ERROR,
    RSP_DATA_MISS,
    RSP_DATA_UNC,
    RSP_DATA_BURST,
    RSP_DATA_WRITE,
    RSP_DATA_ERWAIT,
    RSP_DATA_ERROR,
};

enum {
    READ_PKTID,
    WRITE_PKTID,
};

#define tmpl(x)                                                         \
template<                                                               \
    size_t WRITE_BUFFER_DEPTH,                                          \
    size_t ICACHE_LINES,                                                \
    size_t ICACHE_WORDS,                                                \
    size_t DCACHE_LINES,                                                \
    size_t DCACHE_WORDS,                                                \
    typename vci_param>                                                 \
x VciXCache<                                                            \
    WRITE_BUFFER_DEPTH,                                                 \
    ICACHE_LINES,                                                       \
    ICACHE_WORDS,                                                       \
    DCACHE_LINES,                                                       \
    DCACHE_WORDS,                                                       \
    vci_param>

tmpl(/**/)::VciXCache(
    sc_module_name name,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &index )
    : soclib::caba::BaseModule(name),
      m_cacheability_table(mt.getCacheabilityTable()),
    r_dcache_fsm("DCACHE_FSM")
      ,DCACHE_SAVE_ADDR("DCACHE_SAVE_ADDR")
      ,DCACHE_SAVE_DATA("DCACHE_SAVE_DATA")
      ,DCACHE_SAVE_TYPE("DCACHE_SAVE_TYPE")
      ,DCACHE_SAVE_PREV("DCACHE_SAVE_PREV")
      ,r_icache_fsm("ICACHE_FSM")
      ,ICACHE_MISS_ADDR("ICACHE_MISS_ADDR")
      ,ICACHE_REQ("ICACHE_REQ")
      ,r_vci_cmd_fsm("VCI_CMD_FSM")
      ,DCACHE_CMD_ADDR("DCACHE_CMD_ADDR")
      ,DCACHE_CMD_DATA("DCACHE_CMD_DATA")
      ,DCACHE_CMD_TYPE("DCACHE_CMD_TYPE")
      ,DCACHE_MISS_ADDR("DCACHE_MISS_ADDR")
      ,CMD_CPT("CMD_CPT")
      ,r_vci_rsp_fsm("VCI_RSP_FSM")
      ,RSP_CPT("RSP_CPT")
      ,DCACHE_CPT_INIT("DCACHE_CPT_INIT")
      ,ICACHE_CPT_INIT("ICACHE_CPT_INIT")
{
#ifdef NONAME_RENAME
    for (size_t i=0; i<DCACHE_LINES; i++ ) {
        for (size_t j=0; j<DCACHE_WORDS; j++ ) {
            SOCLIB_REG_RENAME_N2(DCACHE_DATA, i, j);
        }
        SOCLIB_REG_RENAME_N(DCACHE_TAG, i);
    }
    for (size_t i=0; i<ICACHE_LINES; i++ ) {
        for (size_t j=0; j<ICACHE_WORDS; j++ ) {
            SOCLIB_REG_RENAME_N2(ICACHE_DATA, i, j);
        }
        SOCLIB_REG_RENAME_N(ICACHE_TAG, i);
    }
    for (size_t i=0; i<ICACHE_WORDS; i++ ) {
        SOCLIB_REG_RENAME_N(ICACHE_MISS_BUF, i);
        SOCLIB_REG_RENAME_N(ICACHE_VAL_BUF, i);
    }
    for (size_t i=0; i<DCACHE_WORDS; i++ ) {
        SOCLIB_REG_RENAME_N(DCACHE_MISS_BUF, i);
        SOCLIB_REG_RENAME_N(DCACHE_VAL_BUF, i);
    }

    m_data_fifo.rename("DATA_FIFO");    // DCACHE data FIFO
    m_addr_fifo.rename("ADDR_FIFO");    // DCACHE address FIFO
    m_type_fifo.rename("TYPE_FIFO");    // DCACHE type FIFO
#endif

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
        << p_dcache.unc
        << p_icache.req
        << p_icache.adr;
        
#if defined(SYSTEMCASS_SPECIFIC)
    p_icache.frz  (p_icache.req);
    p_icache.ins  (p_icache.req);
    p_icache.frz  (p_icache.adr);
    p_icache.ins  (p_icache.adr);
    p_dcache.frz  (p_dcache.type);
    p_dcache.frz  (p_dcache.adr);
    p_dcache.frz  (p_dcache.req);
    p_dcache.frz  (p_dcache.unc);
    p_dcache.rdata(p_dcache.adr);

//    SAVE_HANDLER(save_state);
#endif
  
    m_ident = mt.indexForId(index);
}

tmpl(void)::transition()
{
    if ( ! p_resetn.read() ) {
        r_dcache_fsm = DCACHE_INIT;
        r_icache_fsm = ICACHE_INIT;
        r_vci_cmd_fsm = CMD_IDLE;
        r_vci_rsp_fsm = RSP_IDLE;

        DCACHE_CPT_INIT = DCACHE_LINES - 1;
        ICACHE_CPT_INIT = ICACHE_LINES - 1;
        m_data_fifo.init();
        m_type_fifo.init();
        m_addr_fifo.init();

        ICACHE_REQ = false;

        for(size_t i=0 ; i<DCACHE_WORDS ;i++)
            DCACHE_VAL_BUF[i] = false;
        ICACHE_MISS_ADDR = 0xffffffff;

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

    // icache_hit & icache_address
    const int icache_address  = (int)p_icache.adr.read() & (s_icache_zmask | s_icache_ymask);
    const int icache_y = (icache_address & s_icache_ymask) >> s_icache_yshift;
    const int icache_z = (icache_address >> s_icache_zshift) | 0x80000000;
    const bool icache_hit = (icache_z == (ICACHE_TAG[icache_y]));

    // dcache_read, dcache_write, dcache_inval, dcache_unc

    bool dcache_read  = false; // cached read request
    bool dcache_unc   = false; // uncached request (word or burst)
    bool dcache_inval = false; // line invalidate request
    bool dcache_write = false; // write request

    switch((soclib::caba::DCacheSignals::req_type_e)(int)p_dcache.type.read()) {
    case soclib::caba::DCacheSignals::RW:
        if (m_cacheability_table[p_dcache.adr.read()]) {
            dcache_read  = true;
            dcache_unc   = p_dcache.unc.read();
        } else {
            dcache_unc   = true;
        }
        break;
    case soclib::caba::DCacheSignals::RZ:
        dcache_inval = true;
        dcache_unc   = p_dcache.unc.read();
        break;
    case soclib::caba::DCacheSignals::WW:
    case soclib::caba::DCacheSignals::WH:
    case soclib::caba::DCacheSignals::WB:
        dcache_write = true;
        dcache_unc   = p_dcache.unc.read();
        break;
    default:
        dcache_unc   = p_dcache.unc.read();
        break;

    }

    const int dcache_address  = (int)p_dcache.adr.read();
    const int dcache_x = (dcache_address & s_dcache_xmask) >> s_dcache_xshift;
    const int dcache_y = (dcache_address & s_dcache_ymask) >> s_dcache_yshift;
    const int dcache_z = (dcache_address >> s_dcache_zshift) | 0x80000000;

    const bool dcache_buf_hit = (
        (dcache_address & ~s_dcache_xmask) ==
        (DCACHE_MISS_ADDR & ~s_dcache_xmask)
        ) && DCACHE_VAL_BUF[dcache_x];

    const bool dcache_hit = (dcache_z == DCACHE_TAG[dcache_y]);
    
    const bool dcache_validreq = p_dcache.req.read() && icache_hit;

    bool fifo_put = false;
    bool fifo_get = false;
    int  data_fifo;
    int  addr_fifo;
    int  type_fifo;

    /////////////////////////////////////////////////////////////////////
    // The r_icache_fsm controls the following ressources:
    // - r_icache_fsm
    // - ICACHE_DATA[ICACHE_WORDS,ICACHE_LINES]
    // - ICACHE_TAG[ICACHE_LINES]
    // - ICACHE_MISS_ADDR
    // - ICACHE_REQ set
    // - ICACHE_CPT_INIT
    //
    // The VALID bit for a cache line is the MSB bit in the TAG.
    //
    // In case of MISS, the controller writes a request in the
    // ICACHE_MISS_ADDR register and sets the ICACHE_REQ flip-flop.
    // The ICACHE_REQ flip-flop is reset by the VCI_RSP controller,
    // when the cache line is ready in the ICACHE buffer.
    /////////////////////////////////////////////////////////////////////

    switch((enum icache_fsm_state_e)(int)r_icache_fsm.read()) {

    case ICACHE_INIT:
        ICACHE_TAG[ICACHE_CPT_INIT] = 0;
        ICACHE_CPT_INIT = ICACHE_CPT_INIT - 1;
        if (ICACHE_CPT_INIT == 0)
            r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
        break;
    
    case ICACHE_IDLE:
        if ( p_icache.req.read() ) {
            if ( ! icache_hit ) {
                r_icache_fsm = ICACHE_WAIT;
                ICACHE_MISS_ADDR = icache_address;
                ICACHE_REQ = true;
            }
            m_cpt_icache_dir_read++;
            m_cpt_icache_data_read++;
        }
        break;

    case ICACHE_WAIT:
        if ( p_vci.rspval.read() && p_vci.reop.read() &&
             p_vci.rpktid.read() == READ_PKTID)
            if ( r_vci_rsp_fsm == RSP_INS_ERROR ||
                 p_vci.rerror.read() )
                r_icache_fsm = ICACHE_IDLE;
            else
                r_icache_fsm = ICACHE_UPDT;
        break;

    case ICACHE_UPDT:
    {
        const int y = (ICACHE_MISS_ADDR & s_icache_ymask) >> s_icache_yshift;
        const int z = (ICACHE_MISS_ADDR >> s_icache_zshift) | 0x80000000;
        ICACHE_TAG[y] = z;
        for (size_t i=0 ; i<ICACHE_WORDS ; i++)
            ICACHE_DATA[y][i] = ICACHE_MISS_BUF[i];
        r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
        m_cpt_icache_data_write++;
        break;
    }
    } // end switch r_icache_fsm

    //////////////////////////////////////////////////////////////////////://///////////
    // The r_dcache_fsm controls the following ressources:
    // - r_dcache_fsm
    // - DCACHE_DATA[DCACHE_WORDS,DCACHE_LINES]
    // - DCACHE_TAG[DCACHE_LINES]
    // - DCACHE_SAVE_ADDR
    // - DCACHE_SAVE_TYPE
    // - DCACHE_SAVE_DATA
    // - DCACHE_SAVE_PREV
    // - DCACHE_CPT_INIT
    // - DCACHE_VAL_BUF[DCACHE_WORDS] reset
    // - fifo_put, data_fifo, addr_fifo, type_fifo
    //
    // The VALID bit for a cache line is the MSB bit in the TAG.
    // The request type written in the FIFO is a copy of the processor request.
    // In the IDLE state, the processor request is saved in the DCACHE_SAVE_ADDR,
    // DCACHE_SAVE_DATA, DCACHE_SAVE_TYPE registers.
    // The data word read in the cache is saved in DCACHE_SAVE_PREV.
    //
    // There is five mutually exclusive conditions to exit the IDLE state:
    // - CACHED READ MISS => to the MISS_REQ state (to post the request in the FIFO),
    // then to the MISS_WAIT state (waiting the cache line), then to the MISS_UPDT
    // (to update the cache), and finally to the IDLE state.
    // - UNCACHED READ (simple word or burst) => to the UNC_REQ state
    // (to post the request in the FIFO), then to the UNC_WAIT state,
    // and finally to the IDLE state.
    // - CACHE INVALIDATE HIT => to the INVAL state for one cycle, then IDLE.
    // - WRITE MISS => directly to the WRITE_REQ state (to post the request in the FIFO)
    // Then it depends on the processor request: In order to support VCI write burst,
    // the processor requests are taken into account in the WRITE_REQ state
    // as well as in the IDLE state.
    // - WRITE HIT => to the WRITE_UPDT state (to update the cache), then to
    // the WRITE_REQ state.
    /////////////////////////////////////////////////////://////////////////////////////

    switch ((enum dcache_fsm_state_e)(int)r_dcache_fsm.read()) {

    case DCACHE_INIT:
        DCACHE_TAG[DCACHE_CPT_INIT] = 0;
        DCACHE_CPT_INIT = DCACHE_CPT_INIT - 1;
        if (DCACHE_CPT_INIT == 0)
            r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
        break;

    case DCACHE_IDLE:
        if (dcache_validreq) {
            DCACHE_SAVE_ADDR = (int)p_dcache.adr.read();
            DCACHE_SAVE_DATA = (int)p_dcache.wdata.read();
            DCACHE_SAVE_TYPE = (int)p_dcache.type.read();
            DCACHE_SAVE_PREV = DCACHE_DATA[dcache_y][dcache_x];

            if (dcache_read && ! dcache_hit) {
                // cached read miss     
                for(size_t i=0 ; i<DCACHE_WORDS ; i++)
                    DCACHE_VAL_BUF[i] = false;
                r_dcache_fsm = DCACHE_MISS_REQ;
            } else if (dcache_unc) {
                if (dcache_buf_hit) {
                    // uncached read hit
                    DCACHE_VAL_BUF[dcache_x] = false;
                } else {
                    // uncached read miss
                    for(size_t i=0 ; i<DCACHE_WORDS ; i++)
                        DCACHE_VAL_BUF[i] = false;
                    r_dcache_fsm = DCACHE_UNC_REQ;
                }
            } else if (dcache_write) {
                if (dcache_hit) {
                    // write hit    
                    if (dcache_buf_hit)
                        DCACHE_VAL_BUF[dcache_x] = false;
                    r_dcache_fsm = DCACHE_WRITE_UPDT;
                } else {
                    // write miss
                    if (dcache_buf_hit)
                        DCACHE_VAL_BUF[dcache_x] = false;
                    r_dcache_fsm = DCACHE_WRITE_REQ;
                }
            } else if (dcache_inval && dcache_hit) {
                // line invalidate    
                r_dcache_fsm = DCACHE_INVAL;
            }

            m_cpt_dcache_data_read++;
            m_cpt_dcache_dir_read++;
        }
        break;

    case DCACHE_WRITE_UPDT:
    {
        const int x = (DCACHE_SAVE_ADDR & s_dcache_xmask) >> s_dcache_xshift;
        const int y = (DCACHE_SAVE_ADDR & s_dcache_ymask) >> s_dcache_yshift;
        const int dcache_wmsk = DCACHE_SAVE_TYPE & 0x00000003;
        const int dcache_byte = DCACHE_SAVE_ADDR & 0x00000003;
        if (dcache_wmsk == 0) {
            // write word
            DCACHE_DATA[y][x] = DCACHE_SAVE_DATA;
        } else if (dcache_wmsk == 1) {
            // write half
            if (dcache_byte == 0) {
                DCACHE_DATA[y][x] =
                    (DCACHE_SAVE_PREV         & 0xFFFF0000) |
                    (DCACHE_SAVE_DATA         & 0x0000FFFF) ;
            } else {
                DCACHE_DATA[y][x] =
                    (DCACHE_SAVE_PREV         & 0x0000FFFF) |
                    ((DCACHE_SAVE_DATA << 16) & 0xFFFF0000) ;
            }
        } else {
            // write byte
            if (dcache_byte == 0) {
                DCACHE_DATA[y][x] =
                    (DCACHE_SAVE_PREV         & 0xFFFFFF00) |
                    (DCACHE_SAVE_DATA         & 0x000000FF) ;
            } else if (dcache_byte == 1) {
                DCACHE_DATA[y][x] =
                    (DCACHE_SAVE_PREV         & 0xFFFF00FF) |
                    ((DCACHE_SAVE_DATA << 8 ) & 0x0000FF00) ;
            } else if (dcache_byte == 2) {
                DCACHE_DATA[y][x] =
                    (DCACHE_SAVE_PREV         & 0xFF00FFFF) |
                    ((DCACHE_SAVE_DATA << 16) & 0x00FF0000) ;
            } else if (dcache_byte == 3) {
                DCACHE_DATA[y][x] =
                    (DCACHE_SAVE_PREV         & 0x00FFFFFF) |
                    ((DCACHE_SAVE_DATA << 24) & 0xFF000000) ;
            }
        }
        r_dcache_fsm = DCACHE_WRITE_REQ;
        m_cpt_dcache_data_write++;
        break;
    }

    case DCACHE_WRITE_REQ:
        fifo_put = true;
        data_fifo = DCACHE_SAVE_DATA;
        addr_fifo = DCACHE_SAVE_ADDR;
        type_fifo = DCACHE_SAVE_TYPE;

        if (m_data_fifo.wok()) {
            DCACHE_SAVE_ADDR = (int)p_dcache.adr.read();
            DCACHE_SAVE_DATA = (int)p_dcache.wdata.read();
            DCACHE_SAVE_PREV = DCACHE_DATA[dcache_y][dcache_x];
            DCACHE_SAVE_TYPE = (int)p_dcache.type.read();

            m_cpt_dcache_data_read++;
            m_cpt_dcache_dir_read++;
            m_cpt_fifo_write++;

            if (! dcache_validreq) {
                r_dcache_fsm = DCACHE_IDLE;
                break;
            }
            
            if (dcache_read && ! dcache_hit) {
                // cached read miss     
                for(size_t i=0 ; i<DCACHE_WORDS ; i++)
                    DCACHE_VAL_BUF[i] = false;
                r_dcache_fsm = DCACHE_MISS_REQ;
            } else if (dcache_unc) {
                if (dcache_buf_hit) {
                    // uncached read hit
                    DCACHE_VAL_BUF[dcache_x] = false;
                } else {
                    // uncached read miss
                    for(size_t i=0 ; i<DCACHE_WORDS ; i++)
                        DCACHE_VAL_BUF[i] = false;
                    r_dcache_fsm = DCACHE_UNC_REQ;
                }
            } else if (dcache_write) {
                if (dcache_hit) {
                    // write hit    
                    if (dcache_buf_hit)
                        DCACHE_VAL_BUF[dcache_x] = false;
                    r_dcache_fsm = DCACHE_WRITE_UPDT;
                } else {
                    // write miss
                    if (dcache_buf_hit)
                        DCACHE_VAL_BUF[dcache_x] = false;
                    r_dcache_fsm = DCACHE_WRITE_REQ;
                }
            } else if (dcache_inval && dcache_hit) {
                // line invalidate    
                r_dcache_fsm = DCACHE_INVAL;
            }
            else
                r_dcache_fsm = DCACHE_IDLE;
        }
        break;

    case DCACHE_MISS_REQ:
        fifo_put = true;
        data_fifo = DCACHE_SAVE_DATA;
        addr_fifo = DCACHE_SAVE_ADDR;
        type_fifo = DCACHE_SAVE_TYPE;
        if (m_data_fifo.wok()) {
            r_dcache_fsm = DCACHE_MISS_WAIT;
            m_cpt_fifo_write++;
        }
        break;

    case DCACHE_MISS_WAIT:
        if ( p_vci.rspval.read() && p_vci.reop.read() &&
             p_vci.rpktid.read() == READ_PKTID )
            if ( r_vci_rsp_fsm == RSP_DATA_ERROR ||
                 p_vci.rerror.read() != 0 )
                r_dcache_fsm = DCACHE_IDLE;
            else
                r_dcache_fsm = DCACHE_MISS_UPDT;
        break;

    case DCACHE_MISS_UPDT:
    {
        const int y = (DCACHE_MISS_ADDR & s_dcache_ymask) >> s_dcache_yshift;
        const int z = (DCACHE_MISS_ADDR >> s_dcache_zshift) | 0x80000000;
        DCACHE_TAG[y] = z;
        for (size_t i=0 ; i<DCACHE_WORDS ; i++)
            DCACHE_DATA[y][i] = DCACHE_MISS_BUF[i];
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        break;
    }

    case DCACHE_UNC_REQ:
        fifo_put = true;
        data_fifo = DCACHE_SAVE_DATA;
        addr_fifo = DCACHE_SAVE_ADDR;
        type_fifo = DCACHE_SAVE_TYPE;
        if (m_data_fifo.wok()) {
            r_dcache_fsm = DCACHE_UNC_WAIT;
            m_cpt_fifo_write++;
        }
        break;

    case DCACHE_UNC_WAIT:
        if ( p_vci.rspval.read() &&
             p_vci.reop.read() &&
             p_vci.rpktid.read() == READ_PKTID )
            r_dcache_fsm = DCACHE_IDLE;
        break;

    case DCACHE_INVAL:
    {
        const int y = (DCACHE_SAVE_ADDR & s_dcache_ymask) >> s_dcache_yshift;
        DCACHE_TAG[y] = 0;
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
        break;
    }

    } // end switch r_dcache_fsm

    ////////////////////////////////////////////////////////////////////////////
    // The r_vci_cmd_fsm controls the following ressources:
    // - r_vci_cmd_fsm
    // - DCACHE_CMD_DATA
    // - DCACHE_CMD_ADDR
    // - DCACHE_CMD_TYPE
    // - DCACHE_MISS_ADDR
    // - CMD_CPT
    // - fifo_get
    //
    // This FSM handles requests from both the DCACHE controler
    // (m_data_fifo non empty) and the ICACHE controler (ICACHE_REQ).
    // As the DCACHE miss and the DCACHE uncached burst generate the same VCI request,
    // there is actually 4 request types:
    // - INS_MISS
    // - DATA_LINE
    // - DATA_WORD
    // - DATA_WRITE
    // The DCACHE requests have the highest priority.
    // There is at most one (REQ/RSP) VCI transaction, as both CMD_FSM and RSP_FSM
    // exit simultaneously the IDLE state.
    // A request is consumed from the m_data_fifo each time the CMD and RSP FSMs
    // are in IDLE state, or the CMD FSM is in CMD_DATA_WRITE state, and there is
    // another write request for the same 4K bytes page.
    //////////////////////////////////////////////////////////////////////////////

    switch ((enum cmd_fsm_state_e)(int)r_vci_cmd_fsm.read()) {
    
    case CMD_IDLE:
        CMD_CPT = 0;
        if (r_vci_rsp_fsm == RSP_IDLE) {
            if (ICACHE_REQ.read()) {
                r_vci_cmd_fsm = CMD_INS_MISS;
            } else if (m_data_fifo.rok()) {
                m_cpt_fifo_read++;
                fifo_get = true;
                DCACHE_CMD_DATA   = (int)m_data_fifo.read();
                DCACHE_CMD_ADDR   = (int)m_addr_fifo.read();
                DCACHE_CMD_TYPE   = (int)m_type_fifo.read();
                
                if (((int)m_type_fifo.read()) == soclib::caba::DCacheSignals::RW)   
                {
                    if (m_cacheability_table[p_dcache.adr.read()])
                        r_vci_cmd_fsm = CMD_DATA_LINE;
                    else
                        r_vci_cmd_fsm = CMD_DATA_WORD;
                } else {
                    assert(xcache_is_write(
                               (soclib::caba::DCacheSignals::req_type_e)
                               (int)m_type_fifo.read()));
                    r_vci_cmd_fsm = CMD_DATA_WRITE;
                }
            }
        }
        break;

    case CMD_INS_MISS:
        if ( p_vci.cmdack.read() ) {
            CMD_CPT = CMD_CPT + 1;
            if (CMD_CPT == ICACHE_WORDS - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DATA_WORD:
        DCACHE_MISS_ADDR = DCACHE_CMD_ADDR;
        if ( p_vci.cmdack.read() )
            r_vci_cmd_fsm = CMD_IDLE;
        break;

    case CMD_DATA_LINE:
        DCACHE_MISS_ADDR = DCACHE_CMD_ADDR;
        if ( p_vci.cmdack.read() ) {
            CMD_CPT = CMD_CPT + 1;
            if (CMD_CPT == DCACHE_WORDS - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci.cmdack.read() ) {
            if ( ! m_data_fifo.rok() ||
                 ! xcache_can_burst(
                     (soclib::caba::DCacheSignals::req_type_e)(int)m_type_fifo.read(), (int)m_addr_fifo.read(),
                     (soclib::caba::DCacheSignals::req_type_e)(int)DCACHE_CMD_TYPE, DCACHE_CMD_ADDR ) ) {
                r_vci_cmd_fsm = CMD_IDLE;
            } else {
                fifo_get = true;
                DCACHE_CMD_DATA   = (int)m_data_fifo.read();
                DCACHE_CMD_ADDR   = (int)m_addr_fifo.read();
                DCACHE_CMD_TYPE   = (int)m_type_fifo.read();
            }
        } else {
        }
        break;
    } // end  switch r_vci_cmd_fsm

    //////////////////////////////////////////////////////////////////////////
    //  m_data_fifo, ADR_FIFO and m_type_fifo
    //  These FIFOs are used as a write buffer and contain the requests from
    //  the DCACHE controler to the VCI controler.
    //  They are controlled by the fifo_put signal (defined by r_dcache_fsm)
    //  and the fifo_get signal (defined by r_vci_cmd_fsm)
    //////////////////////////////////////////////////////////////////////////

    if ( fifo_put ) {
        if ( fifo_get ) {
            m_data_fifo.put_and_get((sc_uint<32>)data_fifo);
            m_addr_fifo.put_and_get((sc_uint<32>)addr_fifo);
            m_type_fifo.put_and_get((sc_uint<4>)type_fifo);
        } else {
            m_data_fifo.simple_put((sc_uint<32>)data_fifo);
            m_addr_fifo.simple_put((sc_uint<32>)addr_fifo);
            m_type_fifo.simple_put((sc_uint<4>)type_fifo);
        }
    } else {
        if ( fifo_get ) {
        m_data_fifo.simple_get();
        m_addr_fifo.simple_get();
        m_type_fifo.simple_get();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // The VCI_RSP FSM controls the following ressources:
    // - r_vci_rsp_fsm:
    // - ICACHE_MISS_BUF[ICACHE_WORDS]
    // - DCACHE_MISS_BUF[DCACHE_WORDS]
    // - DCACHE_VAL_BUF[DCACHE_WORDS] set
    // - ICACHE_REQ reset
    // - CPT_RSP
    //
    // This FSM is synchronized with the r_vci_cmd_fsm, as both FSMs exit the
    // IDLE state simultaneously.
    // This FSM analyzes the VCI error code and activates the p_icache.berr and
    // p_dcache.berr signals during one cycle.
    //////////////////////////////////////////////////////////////////////////

    switch ((enum rsp_fsm_state_e)(int)r_vci_rsp_fsm.read()) {

    case RSP_IDLE:
        RSP_CPT = 0;
        if (r_vci_cmd_fsm == CMD_IDLE) {
            if (ICACHE_REQ.read()) {
                r_vci_rsp_fsm = RSP_INS_MISS;
            } else if (m_data_fifo.rok()) {
                if (((int)m_type_fifo.read()) == soclib::caba::DCacheSignals::RW)
                {
                    if (m_cacheability_table[p_dcache.adr.read()])
                        r_vci_rsp_fsm = RSP_DATA_MISS;
                    else
                        r_vci_rsp_fsm = RSP_DATA_UNC;
                } else
                    r_vci_rsp_fsm = RSP_DATA_WRITE;
            }
        }
        break;

    case RSP_INS_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        if (RSP_CPT == ICACHE_WORDS) {
            printf("error in soclib_vci_xcache : \n");
            printf("illegal VCI response packet for instruction miss\n");
            sc_stop();
        }
        RSP_CPT = RSP_CPT + 1;
        ICACHE_MISS_BUF[RSP_CPT] = (int)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == 0 ) {
                r_vci_rsp_fsm = RSP_IDLE;
                ICACHE_REQ = false;
                if (RSP_CPT != ICACHE_WORDS - 1) {
                    printf("error in soclib_vci_xcache : \n");
                    printf("illegal VCI response packet for instruction miss\n");
                    sc_stop();
                }
            } else {
                r_vci_rsp_fsm = RSP_INS_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != 0 )
                r_vci_rsp_fsm = RSP_INS_ERWAIT;
        }
        break;

    case RSP_INS_ERWAIT:
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_INS_ERROR;
        break;

    case RSP_INS_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
        ICACHE_REQ = false;
        break;

    case RSP_DATA_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        if (RSP_CPT == DCACHE_WORDS) {
            printf("error in soclib_vci_xcache : \n");
            printf("illegal VCI response packet for data read miss");
            sc_stop();
        }
        RSP_CPT = RSP_CPT + 1;
        DCACHE_MISS_BUF[RSP_CPT] = (int)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == 0 ) {
                r_vci_rsp_fsm = RSP_IDLE;
                if (RSP_CPT != DCACHE_WORDS - 1) {
                    printf("error in soclib_vci_xcache : \n");
                    printf("illegal VCI response packet for data read miss");
                    sc_stop();
                }
            } else {
                r_vci_rsp_fsm = RSP_DATA_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != 0 )
                r_vci_rsp_fsm = RSP_DATA_ERWAIT;
        }
        break;

    case RSP_DATA_BURST:
        if ( ! p_vci.rspval.read() )
            break;

        if (RSP_CPT == DCACHE_WORDS) {
            printf("error in soclib_vci_xcache : \n");
            printf("illegal VCI response packet for data read burst");
            sc_stop();
        }
        RSP_CPT = RSP_CPT + 1;
        DCACHE_MISS_BUF[RSP_CPT] = (int)p_vci.rdata.read();
        DCACHE_VAL_BUF[RSP_CPT] = true;
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == 0 ) {
                r_vci_rsp_fsm = RSP_IDLE;
                if (RSP_CPT != DCACHE_WORDS - 1) {
                    printf("error in soclib_vci_xcache : \n");
                    printf("illegal VCI response packet for data read burst");
                    sc_stop();
                }
            } else {
                r_vci_rsp_fsm = RSP_DATA_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != 0 )
                r_vci_rsp_fsm = RSP_DATA_ERWAIT;
        }
        break;

    case RSP_DATA_WRITE:
        if ( ! p_vci.rspval.read() )
            break;
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == 0 ) {
                r_vci_rsp_fsm = RSP_IDLE;
            } else {
                r_vci_rsp_fsm = RSP_DATA_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != 0 )
            r_vci_rsp_fsm = RSP_DATA_ERWAIT;
        }
        break;

    case RSP_DATA_UNC:
        if ( ! p_vci.rspval.read() )
            break;
        {
        const int index = (DCACHE_MISS_ADDR & s_dcache_xmask) >> s_dcache_xshift;
        DCACHE_MISS_BUF[index] = (int)p_vci.rdata.read();
        DCACHE_VAL_BUF[index] = true;
        if ( p_vci.reop.read() &&
             p_vci.rerror.read() == 0 ) {
            r_vci_rsp_fsm = RSP_IDLE;
        } else if ( p_vci.reop.read() &&
                    p_vci.rerror.read() != 0 ) {
            r_vci_rsp_fsm = RSP_DATA_ERROR;
        } else if ( ! p_vci.reop.read() ) {
            printf("error in soclib_vci_xcache : \n");
            printf("illegal VCI response packet for data read uncached");
            sc_stop();
        }
        }
        break;

    case RSP_DATA_ERWAIT:
        if ( ! p_vci.rspval.read() )
            break;

        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_DATA_ERROR;
        break;

    case RSP_DATA_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
        break;
    } // end switch r_vci_rsp_fsm
}

tmpl(void)::genMoore()
{

    // p_vci.rspack, p_icache.berr & p_dcache.berr

    p_vci.rspack = true;
    p_icache.berr = (r_vci_rsp_fsm == RSP_INS_ERROR);
    p_dcache.berr = (r_vci_rsp_fsm == RSP_DATA_ERROR);

    // VCI CMD

    switch ((enum cmd_fsm_state_e)(int)r_vci_cmd_fsm.read()) {

    case CMD_IDLE:
        p_vci.cmdval  = false;
        break;

    case CMD_DATA_WORD:
        p_vci.cmdval = true;
        p_vci.address = (sc_uint<32>)(DCACHE_CMD_ADDR & ~0x3);
        p_vci.be = 0xF;
        p_vci.plen = 4;
        p_vci.cmd = VCI_CMD_READ;
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
    {
        uint32_t subcell = ((uint32_t)DCACHE_CMD_ADDR&0x3);

        p_vci.cmdval = true;
        p_vci.address = (uint32_t)DCACHE_CMD_ADDR & ~0x3;
        p_vci.wdata   = (uint32_t)DCACHE_CMD_DATA << subcell*8;

        assert(xcache_is_write((soclib::caba::DCacheSignals::req_type_e)
                               (int)DCACHE_CMD_TYPE));

        switch((soclib::caba::DCacheSignals::req_type_e)(int)DCACHE_CMD_TYPE) {
        case soclib::caba::DCacheSignals::WW:
            p_vci.be      = 0xF;
            break;
        case soclib::caba::DCacheSignals::WH:
            p_vci.be      = 3 << subcell;
            break;
        case soclib::caba::DCacheSignals::WB:
            p_vci.be      = 1 << subcell;
            break;
        default:
            assert(0);
        }
        p_vci.plen   = 0;
        p_vci.cmd    = VCI_CMD_WRITE;
        p_vci.trdid  = 0;
        p_vci.pktid  = WRITE_PKTID;
        p_vci.srcid  = m_ident;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = false;
        p_vci.clen   = 0;
        p_vci.cfixed = false;

        p_vci.eop = ! (
            m_data_fifo.rok() &&
            xcache_can_burst(
                (soclib::caba::DCacheSignals::req_type_e)
                (int)m_type_fifo.read(), m_addr_fifo.read(),
                (soclib::caba::DCacheSignals::req_type_e)
                (int)DCACHE_CMD_TYPE, DCACHE_CMD_ADDR )
            );
        break;
    }

    case CMD_DATA_LINE:
        p_vci.cmdval = true;
        p_vci.address = (sc_uint<32>)((DCACHE_CMD_ADDR & ~s_dcache_xmask) + (CMD_CPT << 2));
        p_vci.be     = 0xF;
        p_vci.plen   = 0;
        p_vci.cmd    = VCI_CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_ident;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = (CMD_CPT == DCACHE_WORDS - 1);
        break;

    case CMD_INS_MISS:
        p_vci.cmdval = true;
        p_vci.address = (sc_uint<32>)((ICACHE_MISS_ADDR & ~s_icache_xmask) + (CMD_CPT << 2));
        p_vci.be     = 0xF;
        p_vci.plen   = 0;
        p_vci.cmd    = VCI_CMD_READ;
        p_vci.trdid  = 0;
        p_vci.pktid  = READ_PKTID;
        p_vci.srcid  = m_ident;
        p_vci.cons   = false;
        p_vci.wrap   = false;
        p_vci.contig = true;
        p_vci.clen   = 0;
        p_vci.cfixed = false;
        p_vci.eop = (CMD_CPT == ICACHE_WORDS - 1);
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
//   or an uncached read miss, or when the m_data_fifo is full.
// - p_dcache.frz is true in all other states.
// The p_dcache.rdata signal is read in the DCACHE_MISS_BUF buffer in case
// of an uncached read (simple word or burst), and is read in the cache
// in all other cases.
//
// ICACHE
// The p_icache.frz signal is activated only if there is a processor
// request, and depends on the ICACHE FSM states:
// In the IDLE state, the p_icache.frz signal depends on the directory comparison,
// and the p_icache.ins value is the cache or buffer content.
// In all others states, the p_icache.frz signal is always true.
// and the p_icache.ins value is 0.
///////////////////////////////////////////////////////////////////////////////////

tmpl(void)::genMealy()
{
    //  p_icache.frz & p_icache.ins
    if ( p_icache.req.read() && r_icache_fsm == ICACHE_IDLE) {
        const int icache_address = (int)p_icache.adr.read();
        const int x = (icache_address & s_icache_xmask) >> s_icache_xshift;
        const int y = (icache_address & s_icache_ymask) >> s_icache_yshift;
        const int z = (icache_address & s_icache_zmask) >> s_icache_zshift;
        if ((int)(z | 0x80000000) == (int)ICACHE_TAG[y]) {
            p_icache.frz = false;
            p_icache.ins = (sc_uint<32>)ICACHE_DATA[y][x];
        } else {
            p_icache.frz = true;
            p_icache.ins = (sc_uint<32>)ICACHE_DATA[y][x];
        }
    } else {
        p_icache.frz = true;
        p_icache.ins = 0;
    }

    // p_dcache.frz & p_dcache.rdata
    //  dcache_miss & dcache_buf_hit
    const int dcache_address = (int)p_dcache.adr.read();
    const int x = (dcache_address & s_dcache_xmask) >> s_dcache_xshift;
    const int y = (dcache_address & s_dcache_ymask) >> s_dcache_yshift;
    const int z = (dcache_address & s_dcache_zmask) >> s_dcache_zshift;
    const bool dcache_hit = ((int)(z | 0x80000000) == (int)DCACHE_TAG[y]);

    const bool dcache_buf_hit = (
        (dcache_address & ~s_dcache_xmask) == (DCACHE_MISS_ADDR & ~s_dcache_xmask)
        && DCACHE_VAL_BUF[x].read()
        );
    
    if ( p_dcache.req.read() ) {
        switch ((enum dcache_fsm_state_e)(int)r_dcache_fsm.read()) {
        case DCACHE_WRITE_REQ:
            if ( ! m_data_fifo.wok() ) {
                p_dcache.frz = true;
                p_dcache.rdata = (sc_uint<32>)DCACHE_DATA[y][x];
                break;
            }
            // Beware of the fallback
        case DCACHE_IDLE:
            if ( p_dcache.type.read() == soclib::caba::DCacheSignals::RW )
            {
                if ( m_cacheability_table[p_dcache.adr.read()] ) {
                    p_dcache.frz = ! dcache_hit;
                    p_dcache.rdata = (sc_uint<32>)DCACHE_DATA[y][x];
                } else {
                    p_dcache.frz = !dcache_buf_hit;
                    p_dcache.rdata = (sc_uint<32>)DCACHE_MISS_BUF[x];
                }
            } else {
                p_dcache.frz = false;
                p_dcache.rdata = (sc_uint<32>)DCACHE_DATA[y][x];
            }
            break;
        default:
            p_dcache.frz = true;
            p_dcache.rdata = (sc_uint<32>)DCACHE_DATA[y][x];
            break;
        } // end switch
    } else {
        switch ((enum dcache_fsm_state_e)(int)r_dcache_fsm.read()) {
        case DCACHE_WRITE_UPDT:
            p_dcache.frz = true;
            break;
    
        default:
            p_dcache.frz = false;
            p_dcache.rdata = (sc_uint<32>)DCACHE_DATA[y][x];
            break;
        }
    } // end if REQ
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

