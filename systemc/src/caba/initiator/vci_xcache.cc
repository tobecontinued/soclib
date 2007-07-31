/////////////////////////////////////////////////////////////////////////////
// File     : vci_xcache.cc
// Date     : 17/07/2007
// Copyright: UPMC/LIP6
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
    soclib::caba::DCacheSignals::req_type_e old_cmd, int old_addr,
    soclib::caba::DCacheSignals::req_type_e new_cmd, int new_addr)
{
    return xcache_is_write(old_cmd) && xcache_is_write(new_cmd) &&
        (new_addr == old_addr+4);
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
    DCACHE_ERROR,
};

enum icache_fsm_state_e {
    ICACHE_INIT,
    ICACHE_IDLE,
    ICACHE_WAIT,
    ICACHE_UPDT,
    ICACHE_ERROR,
};

enum cmd_fsm_state_e {
    CMD_IDLE,
    CMD_DATA_MISS,
    CMD_DATA_UNC,
    CMD_DATA_WRITE,
    CMD_INS_MISS,
};

enum rsp_fsm_state_e {
    RSP_IDLE,
    RSP_INS_MISS,
    RSP_INS_ERROR_WAIT,
    RSP_INS_ERROR,
    RSP_INS_OK,
    RSP_DATA_MISS,
    RSP_DATA_UNC,
    RSP_DATA_WRITE,
    RSP_DATA_READ_ERROR_WAIT,
    RSP_DATA_READ_ERROR,
    RSP_DATA_READ_OK,
    RSP_DATA_WRITE_ERROR,
    RSP_DATA_WRITE_ERROR_WAIT,
};

enum {
    READ_PKTID,
    WRITE_PKTID,
};

#define tmpl(x)  template<typename vci_param> x VciXCache<vci_param>

tmpl(/**/)::VciXCache(
    sc_module_name name,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &index,
    int icache_lines,
    int icache_words,
    int dcache_lines,
    int dcache_words )
    : soclib::caba::BaseModule(name),

      m_cacheability_table(mt.getCacheabilityTable()),
      m_ident(mt.indexForId(index)),

      s_dcache_lines(dcache_lines),
      s_dcache_words(dcache_words),
      s_icache_lines(icache_lines),
      s_icache_words(icache_words),

      s_icache_xshift(2),
      s_icache_yshift((int)log2(s_icache_words) + s_icache_xshift),
      s_icache_zshift((int)log2(s_icache_lines) + s_icache_yshift),
      s_icache_xmask(((1<<(int)log2(s_icache_words))-1) << s_icache_xshift),
      s_icache_ymask(((1<<(int)log2(s_icache_lines))-1) << s_icache_yshift),
      s_icache_zmask((~0x0) << s_icache_zshift),

      s_dcache_xshift(2),
      s_dcache_yshift((int)log2(s_dcache_words) + s_dcache_xshift),
      s_dcache_zshift((int)log2(s_dcache_lines) + s_dcache_yshift),
      s_dcache_xmask(((1<<(int)log2(s_dcache_words))-1) << s_dcache_xshift),
      s_dcache_ymask(((1<<(int)log2(s_dcache_lines))-1) << s_dcache_yshift),
      s_dcache_zmask((~0x0) << s_dcache_zshift),

      r_dcache_fsm("DCACHE_FSM"),
      r_dcache_save_addr("r_dcache_save_addr"),
      r_dcache_save_data("r_dcache_save_data"),
      r_dcache_save_type("r_dcache_save_type"),
      r_dcache_save_prev("r_dcache_save_prev"),

      r_icache_fsm("ICACHE_FSM"),
      r_icache_miss_addr("r_icache_miss_addr"),
      r_icache_req("r_icache_req"),

      r_vci_cmd_fsm("VCI_CMD_FSM"),
      r_dcache_cmd_addr("r_dcache_cmd_addr"),
      r_dcache_cmd_data("r_dcache_cmd_data"),
      r_dcache_cmd_type("r_dcache_cmd_type"),
      r_dcache_miss_addr("r_dcache_miss_addr"),
      r_cmd_cpt("r_cmd_cpt"),

      r_vci_rsp_fsm("VCI_RSP_FSM"),
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

    r_dcache_data = new sc_signal<int>*[dcache_lines];
    for ( int i=0; i<dcache_lines; ++i )
        r_dcache_data[i] = new sc_signal<int>[dcache_words];

    r_dcache_tag = new sc_signal<int>[dcache_lines];

    r_icache_data = new sc_signal<int>*[icache_lines];
    for ( int i=0; i<icache_lines; ++i )
        r_icache_data[i] = new sc_signal<int>[icache_words];

    r_icache_tag = new sc_signal<int>[icache_lines];

    r_icache_miss_buf = new sc_signal<int>[icache_words];    
    r_dcache_miss_buf = new sc_signal<int>[dcache_words];    

#ifdef NONAME_RENAME
    for (int i=0; i<s_dcache_lines; i++ ) {
        for (int j=0; j<s_dcache_words; j++ ) {
            SOCLIB_REG_RENAME_N2(r_dcache_data, i, j);
        }
        SOCLIB_REG_RENAME_N(r_dcache_tag, i);
    }
    for (int i=0; i<s_icache_lines; i++ ) {
        for (int j=0; j<s_icache_words; j++ ) {
            SOCLIB_REG_RENAME_N2(r_icache_data, i, j);
        }
        SOCLIB_REG_RENAME_N(r_icache_tag, i);
    }
    for (int i=0; i<s_icache_words; i++ ) {
        SOCLIB_REG_RENAME_N(r_icache_miss_buf, i);
    }
    for (int i=0; i<s_dcache_words; i++ ) {
        SOCLIB_REG_RENAME_N(r_dcache_miss_buf, i);
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
    p_dcache.rdata(p_dcache.adr);
#endif
}

//----------------------------
tmpl(void)::transition()
//----------------------------
{
    if ( ! p_resetn.read() ) {
        r_dcache_fsm = DCACHE_INIT;
        r_icache_fsm = ICACHE_INIT;
        r_vci_cmd_fsm = CMD_IDLE;
        r_vci_rsp_fsm = RSP_IDLE;

        r_dcache_cpt_init = s_dcache_lines - 1;
        r_icache_cpt_init = s_icache_lines - 1;
        m_data_fifo.init();
        m_type_fifo.init();
        m_addr_fifo.init();

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

    // icache_address & icache_hit
    const int icache_address = (int)p_icache.adr.read();
    const int icache_y = (icache_address & s_icache_ymask) >> s_icache_yshift;
    const int icache_z = (icache_address >> s_icache_zshift) | 0x80000000;
    const bool icache_hit = (icache_z == (r_icache_tag[icache_y]));

    // dcache_read, dcache_write, dcache_inval, dcache_unc
    // the dcache_req_type contains the data request, possibly
    // modified to take into account the cacheability table.

    int  dcache_req_type = (int)p_dcache.type.read(); 

    bool dcache_read  ; // cached read request
    bool dcache_unc   ; // uncached request 
    bool dcache_inval ; // line invalidate request
    bool dcache_write ; // write request

    switch(dcache_req_type) {
    case soclib::caba::DCacheSignals::RW:
        if (m_cacheability_table[p_dcache.adr.read()]) {
            dcache_read  = true;
            dcache_unc   = false;
            dcache_inval = false;
            dcache_write = false;
        } else {
            dcache_read  = false;
            dcache_unc   = true;
            dcache_inval = false;
            dcache_write = false;
            dcache_req_type  = soclib::caba::DCacheSignals::RU;
        }
        break;
    case soclib::caba::DCacheSignals::RU:
        dcache_read  = false;
        dcache_unc   = true;
        dcache_inval = false;
        dcache_write = false;
        break;
    case soclib::caba::DCacheSignals::RZ:
        dcache_read  = false;
        dcache_unc   = false;
        dcache_inval = true;
        dcache_write = false;
        break;
    case soclib::caba::DCacheSignals::WW:
    case soclib::caba::DCacheSignals::WH:
    case soclib::caba::DCacheSignals::WB:
        dcache_read  = false;
        dcache_unc   = false;
        dcache_inval = false;
        dcache_write = true;
        break;
    default:
        dcache_read  = false;
        dcache_unc   = false;
        dcache_inval = false;
        dcache_write = false;
        break;
    }

    // dcache_address, dcache_unc_hit, dcache_hit, dcache_validreq 
    const int dcache_address  = (int)p_dcache.adr.read();
    const int dcache_x = (dcache_address & s_dcache_xmask) >> s_dcache_xshift;
    const int dcache_y = (dcache_address & s_dcache_ymask) >> s_dcache_yshift;
    const int dcache_z = (dcache_address >> s_dcache_zshift) | 0x80000000;

    const bool dcache_unc_hit = r_dcache_unc_valid && (dcache_address == r_dcache_miss_addr); 

    const bool dcache_hit = (dcache_z == r_dcache_tag[dcache_y]);
    
    const bool dcache_validreq = p_dcache.req.read() && icache_hit;

    bool    fifo_put = false;
    bool    fifo_get = false;
    int     data_fifo = 0;
    int     addr_fifo = 0;
    int     type_fifo = 0;

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

    switch((enum icache_fsm_state_e)(int)r_icache_fsm.read()) {

    case ICACHE_INIT:
        r_icache_tag[r_icache_cpt_init] = 0;
        r_icache_cpt_init = r_icache_cpt_init - 1;
        if (r_icache_cpt_init == 0)
            r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
        break;
    
    case ICACHE_IDLE:
        if ( p_icache.req.read() ) {
            assert((int)p_icache.type.read() == soclib::caba::ICacheSignals::RI &&
                "Only cached instruction fetch are supported");
            if ( ! icache_hit ) {
                r_icache_fsm = ICACHE_WAIT;
                r_icache_miss_addr = icache_address & (s_icache_zmask | s_icache_ymask);
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
        r_icache_fsm = ICACHE_IDLE;
        break;

    case ICACHE_UPDT:
    {
        r_icache_tag[icache_y] = icache_z;
        for (int i=0 ; i<s_icache_words ; i++)
            r_icache_data[icache_y][i] = r_icache_miss_buf[i];
        r_icache_fsm = ICACHE_IDLE;
        m_cpt_icache_dir_write++;
        m_cpt_icache_data_write++;
        break;
    }
    
    } // end switch r_icache_fsm

    //////////////////////////////////////////////////////////////////////://///////////
    // The DCACHE FSM controls the following ressources:
    // - r_dcache_fsm
    // - r_dcache_data[s_dcache_words,s_dcache_lines]
    // - r_dcache_tag[s_dcache_lines]
    // - r_dcache_save_addr
    // - r_dcache_save_type
    // - r_dcache_save_data
    // - r_dcache_save_prev
    // - r_dcache_cpt_init
    // - r_dcache_unc_valid reset
    // - fifo_put, data_fifo, addr_fifo, type_fifo
    //
    // The VALID bit for a cache line is the MSB bit in the TAG.
    // In the IDLE state, the processor request is saved in the r_dcache_save_addr,
    // r_dcache_save_data, r_dcache_save_type registers.
    // The data  read in the cache is saved in r_dcache_save_prev.
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

    switch ((enum dcache_fsm_state_e)(int)r_dcache_fsm.read()) {

    case DCACHE_INIT:
        r_dcache_tag[r_dcache_cpt_init] = 0;
        r_dcache_cpt_init = r_dcache_cpt_init - 1;
        if (r_dcache_cpt_init == 0)
            r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
        break;

    case DCACHE_IDLE:
        if (dcache_validreq) {
            r_dcache_save_addr = (int)p_dcache.adr.read();
            r_dcache_save_data = (int)p_dcache.wdata.read();
            r_dcache_save_type = dcache_req_type;
            r_dcache_save_prev = r_dcache_data[dcache_y][dcache_x];

            if (dcache_read && ! dcache_hit) {      // cached read miss     
                r_dcache_fsm = DCACHE_MISS_REQ;
            } else if (dcache_unc) {
                if (dcache_unc_hit) {               // uncached read hit
                    r_dcache_unc_valid = false;
                } else {                            // uncached read miss
                    r_dcache_fsm = DCACHE_UNC_REQ;
                }
            } else if (dcache_write) {
                if (dcache_hit) {                   // write hit    
                    r_dcache_fsm = DCACHE_WRITE_UPDT;
                } else {                            // write miss
                    r_dcache_fsm = DCACHE_WRITE_REQ;
                }
            } else if (dcache_inval && dcache_hit) { // line invalidate    
                r_dcache_fsm = DCACHE_INVAL;
            }
            m_cpt_dcache_data_read++;
            m_cpt_dcache_dir_read++;
        }
        break;

    case DCACHE_WRITE_UPDT:
    {
        const int x    = (r_dcache_save_addr & s_dcache_xmask) >> s_dcache_xshift;
        const int y    = (r_dcache_save_addr & s_dcache_ymask) >> s_dcache_yshift;
        const int type = r_dcache_save_type;
        const int byte = r_dcache_save_addr & 0x00000003;
        switch(type) {
            case soclib::caba::DCacheSignals::WW:       // write word
                r_dcache_data[y][x] = r_dcache_save_data;
            break;
            case soclib::caba::DCacheSignals::WH:       // write half
                if (byte == 0) {
                    r_dcache_data[y][x] =
                    (r_dcache_save_prev         & 0xFFFF0000) |
                    (r_dcache_save_data         & 0x0000FFFF) ;
                } else {
                    r_dcache_data[y][x] =
                    (r_dcache_save_prev         & 0x0000FFFF) |
                    ((r_dcache_save_data << 16) & 0xFFFF0000) ;
                }
            break;
            case soclib::caba::DCacheSignals::WB:       // write byte
                if (byte == 0) {
                    r_dcache_data[y][x] =
                    (r_dcache_save_prev         & 0xFFFFFF00) |
                    (r_dcache_save_data         & 0x000000FF) ;
                } else if (byte == 1) {
                    r_dcache_data[y][x] =
                    (r_dcache_save_prev         & 0xFFFF00FF) |
                    ((r_dcache_save_data << 8 ) & 0x0000FF00) ;
                } else if (byte == 2) {
                    r_dcache_data[y][x] =
                    (r_dcache_save_prev         & 0xFF00FFFF) |
                    ((r_dcache_save_data << 16) & 0x00FF0000) ;
                } else {
                    r_dcache_data[y][x] =
                    (r_dcache_save_prev         & 0x00FFFFFF) |
                    ((r_dcache_save_data << 24) & 0xFF000000) ;
                }
            break;
        } // end switch 
        m_cpt_dcache_data_write++;
        r_dcache_fsm = DCACHE_WRITE_REQ;
        break;
    }

    case DCACHE_WRITE_REQ:
        fifo_put = true;
        data_fifo = r_dcache_save_data;
        addr_fifo = r_dcache_save_addr;
        type_fifo = r_dcache_save_type;

        if (m_data_fifo.wok()) {
            r_dcache_save_addr = (int)p_dcache.adr.read();
            r_dcache_save_data = (int)p_dcache.wdata.read();
            r_dcache_save_type = dcache_req_type;
            r_dcache_save_prev = r_dcache_data[dcache_y][dcache_x];

            m_cpt_dcache_data_read++;
            m_cpt_dcache_dir_read++;
            m_cpt_fifo_write++;

            if (! dcache_validreq) {
                r_dcache_fsm = DCACHE_IDLE;
                break;
            }
            
            if (dcache_read && ! dcache_hit) {          // cached read miss     
                r_dcache_fsm = DCACHE_MISS_REQ;
            } else if (dcache_unc) {
                if (dcache_unc_hit) {                   // uncached read hit
                    r_dcache_unc_valid = false;
                } else {                                // uncached read miss
                    r_dcache_fsm = DCACHE_UNC_REQ;
                }
            } else if (dcache_write) {
                if (dcache_hit) {                       // write hit    
                    r_dcache_fsm = DCACHE_WRITE_UPDT;
                } else {                                // write miss
                    r_dcache_fsm = DCACHE_WRITE_REQ;
                }
            } else if (dcache_inval && dcache_hit) {    // line invalidate    
                r_dcache_fsm = DCACHE_INVAL;
            } else {                                    // cached read hit
                r_dcache_fsm = DCACHE_IDLE;
            }
        }
        break;

    case DCACHE_MISS_REQ:
        fifo_put = true;
        data_fifo = r_dcache_save_data;
        addr_fifo = r_dcache_save_addr;
        type_fifo = r_dcache_save_type;
        if (m_data_fifo.wok()) {
            r_dcache_fsm = DCACHE_MISS_WAIT;
            m_cpt_fifo_write++;
            }
        break;

    case DCACHE_MISS_WAIT:
        if ( r_vci_rsp_fsm == RSP_DATA_READ_ERROR )  r_dcache_fsm = DCACHE_ERROR;
        if ( r_vci_rsp_fsm == RSP_DATA_READ_OK )     r_dcache_fsm = DCACHE_MISS_UPDT;
        break;

    case DCACHE_MISS_UPDT:
        {
        const int y = (r_dcache_miss_addr & s_dcache_ymask) >> s_dcache_yshift;
        const int z = (r_dcache_miss_addr >> s_dcache_zshift) | 0x80000000;
        r_dcache_tag[y] = z;
        for (int i=0 ; i<s_dcache_words ; i++)
            r_dcache_data[y][i] = r_dcache_miss_buf[i];
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_data_write++;
        m_cpt_dcache_dir_write++;
        break;
        }

    case DCACHE_UNC_REQ:
        fifo_put = true;
        data_fifo = r_dcache_save_data;
        addr_fifo = r_dcache_save_addr;
        type_fifo = r_dcache_save_type;
        if (m_data_fifo.wok()) {
            r_dcache_fsm = DCACHE_UNC_WAIT;
            m_cpt_fifo_write++;
        }
        break;

    case DCACHE_UNC_WAIT:
        if ( r_vci_rsp_fsm == RSP_DATA_READ_ERROR )  r_dcache_fsm = DCACHE_ERROR;
        if ( r_vci_rsp_fsm == RSP_DATA_READ_OK )     r_dcache_fsm = DCACHE_IDLE;
        break;

    case DCACHE_ERROR:
        r_dcache_fsm = DCACHE_IDLE;
        break;
        
    case DCACHE_INVAL:
        {
        const int y = (r_dcache_save_addr & s_dcache_ymask) >> s_dcache_yshift;
        r_dcache_tag[y] = 0;
        r_dcache_fsm = DCACHE_IDLE;
        m_cpt_dcache_dir_write++;
        }
        break;

    } // end switch r_dcache_fsm

    ////////////////////////////////////////////////////////////////////////////
    // The VCI_CMD FSM controls the following ressources:
    // - r_vci_cmd_fsm
    // - r_dcache_cmd_data
    // - r_dcache_cmd_addr
    // - r_dcache_cmd_type
    // - r_dcache_miss_addr
    // - r_cmd_cpt
    // - fifo_get
    //
    // This FSM handles requests from both the DCACHE controler
    // (m_data_fifo non empty) and the ICACHE controler (r_icache_req).
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
    // A request is consumed from the m_data_fifo each time the CMD and RSP FSMs
    // are in IDLE state, or the CMD FSM is in CMD_DATA_WRITE state, and there is
    // another write request at address + 4.
    //////////////////////////////////////////////////////////////////////////////

    switch ((enum cmd_fsm_state_e)(int)r_vci_cmd_fsm.read()) {
    
    case CMD_IDLE:
        r_cmd_cpt = 0;
        if (r_vci_rsp_fsm == RSP_IDLE) {
            if (r_icache_req.read()) {
                r_vci_cmd_fsm = CMD_INS_MISS;
            } else if (m_data_fifo.rok()) {
                m_cpt_fifo_read++;
                fifo_get = true;
                r_dcache_cmd_data   = m_data_fifo.read();
                r_dcache_cmd_addr   = m_addr_fifo.read();
                r_dcache_cmd_type   = (int)m_type_fifo.read();
                
                if (((int)m_type_fifo.read()) == soclib::caba::DCacheSignals::RW)   {
                        r_vci_cmd_fsm = CMD_DATA_MISS;
                } else if (((int)m_type_fifo.read()) == soclib::caba::DCacheSignals::RU)   {
                        r_vci_cmd_fsm = CMD_DATA_UNC;
                } else {
                        r_vci_cmd_fsm = CMD_DATA_WRITE;
                }
            }
        }
        break;

    case CMD_INS_MISS:
        if ( p_vci.cmdack.read() ) {
            r_cmd_cpt = r_cmd_cpt + 1;
            if (r_cmd_cpt == (int)s_icache_words - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DATA_UNC:
        r_dcache_miss_addr = r_dcache_cmd_addr;
        if ( p_vci.cmdack.read() )
            r_vci_cmd_fsm = CMD_IDLE;
        break;

    case CMD_DATA_MISS:
        r_dcache_miss_addr = r_dcache_cmd_addr;
        if ( p_vci.cmdack.read() ) {
            r_cmd_cpt = r_cmd_cpt + 1;
            if (r_cmd_cpt == (int)s_dcache_words - 1)
                r_vci_cmd_fsm = CMD_IDLE;
        }
        break;

    case CMD_DATA_WRITE:
        if ( p_vci.cmdack.read() ) {
            if ( ! m_data_fifo.rok() ||
                 ! xcache_can_burst(
                        (soclib::caba::DCacheSignals::req_type_e)(int)m_type_fifo.read(), 
                        (int)m_addr_fifo.read(),
                        (soclib::caba::DCacheSignals::req_type_e)(int)r_dcache_cmd_type, 
                        r_dcache_cmd_addr ) ) {
                r_vci_cmd_fsm = CMD_IDLE;
            } else {
                fifo_get = true;
                r_dcache_cmd_data   = (int)m_data_fifo.read();
                r_dcache_cmd_addr   = (int)m_addr_fifo.read();
                r_dcache_cmd_type   = (int)m_type_fifo.read();
            }
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

    switch ((enum rsp_fsm_state_e)(int)r_vci_rsp_fsm.read()) {

    case RSP_IDLE:
        r_rsp_cpt = 0;
        if (r_vci_cmd_fsm == CMD_IDLE) {
            if (r_icache_req.read()) {
                r_vci_rsp_fsm = RSP_INS_MISS;
            } else if (m_data_fifo.rok()) {
                if (((int)m_type_fifo.read()) == soclib::caba::DCacheSignals::RW) {
                        r_vci_rsp_fsm = RSP_DATA_MISS;
                } else if (((int)m_type_fifo.read()) == soclib::caba::DCacheSignals::RU) {
                        r_vci_rsp_fsm = RSP_DATA_UNC;
                } else
                        r_vci_rsp_fsm = RSP_DATA_WRITE;
            }
        }
        break;

    case RSP_INS_MISS:
        if ( ! p_vci.rspval.read() )
            break;

        if (r_rsp_cpt == (int)s_icache_words) {
            printf("error in soclib_vci_xcache : \n");
            printf("illegal VCI response packet for instruction miss\n");
            sc_stop();
        }
        r_rsp_cpt = r_rsp_cpt + 1;
        r_icache_miss_buf[r_rsp_cpt] = (int)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == 0 ) {
                r_vci_rsp_fsm = RSP_INS_OK;
                if (r_rsp_cpt != (int)s_icache_words - 1) {
                    printf("error in soclib_vci_xcache : \n");
                    printf("illegal VCI response packet for instruction miss\n");
                    sc_stop();
                }
            } else {
                r_vci_rsp_fsm = RSP_INS_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != 0 )
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

        if (r_rsp_cpt == (int)s_dcache_words) {
            printf("error in soclib_vci_xcache : \n");
            printf("illegal VCI response packet for data read miss");
            sc_stop();
        }
        r_rsp_cpt = r_rsp_cpt + 1;
        r_dcache_miss_buf[r_rsp_cpt] = (int)p_vci.rdata.read();
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == 0 ) {
                r_vci_rsp_fsm = RSP_DATA_READ_OK;
                if (r_rsp_cpt != (int)s_dcache_words - 1) {
                    printf("error in soclib_vci_xcache : \n");
                    printf("illegal VCI response packet for data read miss");
                    sc_stop();
                }
            } else {
                r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != 0 )
                r_vci_rsp_fsm = RSP_DATA_READ_ERROR_WAIT;
        }
        break;

    case RSP_DATA_WRITE:
        if ( ! p_vci.rspval.read() )
            break;
        if ( p_vci.reop.read() ) {
            if ( p_vci.rerror.read() == 0 ) {
                r_vci_rsp_fsm = RSP_IDLE;
            } else {
                r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
            }
        } else {
            if ( p_vci.rerror.read() != 0 )
            r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR_WAIT;
        }
        break;

    case RSP_DATA_UNC:
        if ( ! p_vci.rspval.read() )
            break;
        {
        if ( p_vci.reop.read() && p_vci.rerror.read() == 0 ) {
            r_dcache_miss_buf[0] = (int)p_vci.rdata.read();
            r_dcache_unc_valid = true;
            r_vci_rsp_fsm = RSP_DATA_READ_OK;
        } else if ( p_vci.reop.read() && p_vci.rerror.read() != 0 ) {
            r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
        } else if ( ! p_vci.reop.read() ) {
            printf("error in soclib_vci_xcache : \n");
            printf("illegal VCI response packet for data read uncached");
            sc_stop();
        }
        }
        break;

    case RSP_DATA_READ_OK:
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_READ_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;
        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_DATA_READ_ERROR;
        break;

    case RSP_DATA_READ_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
        break;

    case RSP_DATA_WRITE_ERROR_WAIT:
        if ( ! p_vci.rspval.read() )
            break;
        if ( p_vci.reop.read() )
            r_vci_rsp_fsm = RSP_DATA_WRITE_ERROR;
        break;

    case RSP_DATA_WRITE_ERROR:
        r_vci_rsp_fsm = RSP_IDLE;
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
    p_dcache.berr = (r_dcache_fsm == DCACHE_ERROR) || 
                    (r_vci_rsp_fsm == RSP_DATA_WRITE_ERROR);

    // VCI CMD

    switch ((enum cmd_fsm_state_e)(int)r_vci_cmd_fsm.read()) {

    case CMD_IDLE:
        p_vci.cmdval  = false;
        break;

    case CMD_DATA_UNC:
        p_vci.cmdval = true;
        p_vci.address = r_dcache_cmd_addr & ~0x3;
        p_vci.be = 0xF;
        p_vci.plen = 0;
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
        int subcell = r_dcache_cmd_addr & 0x3;

        p_vci.cmdval = true;
        p_vci.address = r_dcache_cmd_addr & ~0x3;
        p_vci.wdata   = r_dcache_cmd_data << subcell*8;

        switch((soclib::caba::DCacheSignals::req_type_e)(int)r_dcache_cmd_type) {
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
                (int)r_dcache_cmd_type, r_dcache_cmd_addr )
            );
        break;
    }

    case CMD_DATA_MISS:
        p_vci.cmdval = true;
        p_vci.address = (r_dcache_cmd_addr & ~s_dcache_xmask) + (r_cmd_cpt << 2);
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
        p_vci.eop = (r_cmd_cpt == (int)s_dcache_words - 1);
        break;

    case CMD_INS_MISS:
        p_vci.cmdval = true;
        p_vci.address = (r_icache_miss_addr & ~s_icache_xmask) + (r_cmd_cpt << 2);
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
        p_vci.eop = (r_cmd_cpt == (int)s_icache_words - 1);
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
        if (r_icache_fsm == ICACHE_IDLE) {
            const int icache_address = (int)p_icache.adr.read();
            const int x = (icache_address & s_icache_xmask) >> s_icache_xshift;
            const int y = (icache_address & s_icache_ymask) >> s_icache_yshift;
            const int z = (icache_address & s_icache_zmask) >> s_icache_zshift;
            p_icache.frz = ((int)(z | 0x80000000) != (int)r_icache_tag[y]);
            p_icache.ins = (uint32_t)r_icache_data[y][x];
        } else if (r_icache_fsm == ICACHE_ERROR) {
            p_icache.frz = false;
            p_icache.ins = 0;
        } else {
            p_icache.frz = true;
            p_icache.ins = 0;
        }
    } else {
        p_icache.frz = false;
        p_icache.ins = 0;
    }

    ////////// p_dcache.frz & p_dcache.rdata

    if ( p_dcache.req.read() ) {

        //  dcache_hit & dcache_unc_hit
        const int dcache_address = (int)p_dcache.adr.read();
        const int x = (dcache_address & s_dcache_xmask) >> s_dcache_xshift;
        const int y = (dcache_address & s_dcache_ymask) >> s_dcache_yshift;
        const int z = (dcache_address & s_dcache_zmask) >> s_dcache_zshift;
        const bool dcache_hit = ((int)(z | 0x80000000) == (int)r_dcache_tag[y]);
        const bool dcache_unc_hit = r_dcache_unc_valid;

        // Sanity check
        if (dcache_unc_hit)
            assert(dcache_address == r_dcache_miss_addr &&
                    "CPU changed requested address on DCACHE during uncached read");

        // req_uncached & req_cached
        const bool req_uncached =
            ((p_dcache.type.read() == soclib::caba::DCacheSignals::RU) ||
            ((p_dcache.type.read() == soclib::caba::DCacheSignals::RW) &&
            (! m_cacheability_table[dcache_address] )));
        const bool req_cached = 
            ((p_dcache.type.read() == soclib::caba::DCacheSignals::RW) &&
            ( m_cacheability_table[dcache_address] ));
    
        switch ((enum dcache_fsm_state_e)(int)r_dcache_fsm.read()) {
        // if the write buffer is not full, we must have the same behaviour
        // in the DCACHE_WRITE_REQ state as in the IDLE state...
        case DCACHE_WRITE_REQ:
            if ( ! m_data_fifo.wok() ) {       // write buffer full
                p_dcache.frz = true;
                p_dcache.rdata = (uint32_t)r_dcache_data[y][x];
                break;
            }
        case DCACHE_IDLE:
            if (req_cached) {                   // read cached request
                p_dcache.frz = ! dcache_hit;
                p_dcache.rdata = (uint32_t)r_dcache_data[y][x];
            } else if(req_uncached) {           // read uncached request
                p_dcache.frz = ! dcache_unc_hit;
                p_dcache.rdata = (uint32_t)r_dcache_miss_buf[0];
            } else {                            // write request
                p_dcache.frz = false;
                p_dcache.rdata = (uint32_t)r_dcache_data[y][x];
            }
            break;
        default:                    // frz in all other states
            p_dcache.frz = true;
            p_dcache.rdata = (uint32_t)r_dcache_data[y][x];
            break;
        } // end switch
    } else {
        p_dcache.frz = false;
        p_dcache.rdata = 0;
    } // end if data request

} // end genMealy

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

