/* -*- c++ -*-
  *
  * File : vci_dspin_initiator_wrapper.h
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner
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

#include "../include/vci_dspin_target_wrapper.h"

namespace soclib { namespace caba {

#define tmpl(x) template<typename vci_param, size_t dspin_cmd_width, size_t dspin_rsp_width> x VciDspinTargetWrapper<vci_param, dspin_cmd_width, dspin_rsp_widt>

///////////////////////////////////////////////////////
tmpl(/**/)::VciDspinTargetWrapper(sc_module_name name )
	   : soclib::caba::BaseModule(insname),

	   p_clk("clk"),
	   p_resetn("resetn"),
	   p_dspin_cmd("dspin_cmd"),
	   p_dspin_rsp("dspin_rsp"),
	   p_vci("vci"),

	   r_cmd_fsm("r_cmd_fsm"),
	   r_rsp_fsm("r_rsp_fsm"),
{
	SC_METHOD (transition);
	dont_initialize();
	sensitive << p_clk.pos();

    SC_METHOD (genMealy_vci_cmd);
	dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_dspin_cmd.write;
    sensitive << p_dspin_cmd.data;

    SC_METHOD (genMealy_vci_rsp);
	dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_dspin_rsp.read;

    SC_METHOD (genMealy_dspin_cmd);
	dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_vci.cmdack;

    sensitive << p_vci.cmdval;
    sensitive << p_vci.address;
    sensitive << p_vci.wdata;
    sensitive << p_vci.srcid;
    sensitive << p_vci.trdid;
    sensitive << p_vci.pktid;
    sensitive << p_vci.plen;
    sensitive << p_vci.be;
    sensitive << p_vci.cmd;
    sensitive << p_vci.cons;
    sensitive << p_vci.contig;

    SC_METHOD (genMealy_dspin_rsp);
	dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_vci.rspval;
    sensitive << p_vci.rerror;
    sensitive << p_vci.rdata;
    sensitive << p_vci.rsrcid;
    sensitive << p_vci.rtrdid;
    sensitive << p_vci.rpktid;
    sensitive << p_vci.reop;

    assert( (dspin_cmd_width == 40) and "The DSPIN CMD flit width must have 40 bits");
    assert( (dspin_rsp_width == 33) and "The DSPIN RSP flit width must have 33 bits");
    assert( (vci_param::N    <= 40) and "The VCI ADDRESS field cannot have more than 40 bits");
    assert( (vci_param::B    == 4 ) and "The VCI DATA filds must have 32 bits");
    assert( (vci_param::K    == 8 ) and "The VCI PLEN field cannot have more than 8 bits");
    assert( (vci_param::S    <= 14) and "The VCI SRCID field cannot have more than 14 bits");
    assert( (vci_param::T    <= 4 ) and "The VCI TRDID field cannot have more than 4 bits");
    assert( (vci_param::P    <= 4 ) and "The VCI PKTID field cannot have more than 4 bits");
    assert( (vci_param::E    <= 2 ) and "The VCI RERROR field cannot have more than 2 bits");

} //  end constructor

////////////////////////
tmpl(void)::transition()
{
	if ( p_resetn == false ) 
    {
	    r_cmd_fsm = CMD_IDLE;
	    r_rsp_fsm = RSP_IDLE;
	    return;
	} 

    /////////////////////////////////////////////////////////////
    // VCI response packet to DSPIN response packet.
    /////////////////////////////////////////////////////////////
    // - A single flit VCI response packet with a 0 RDATA value
    //   is translated to a single flit DSPIN response.
    // - All other VCI responses are translated to a multi-flit
    //   DSPIN response.
    // In the RSP_IDLE state, the first DSPIN flit is written,
    // but no VCI flit is consumed. 
    // In RSP_SINGLE state no DSPIN flit is transmitted, but
    // the VCI flit is consumed.
    // In the RSP_MULTI state, a VCI flit is consumed and a
    // DSPIN flit is transmited.
    //////////////////////////////////////////////////////////////

    switch(r_rsp_fsm)
    {
        case RSP_IDLE:        // transmit first DSPIN flit
            if( p_vci.rspval.read() and p_dspin_rsp.read.read() )
            {
                if ( p_vci.reop.read() and 
                     (p_vci.rdata.read() == 0) ) r_rsp_fsm = RSP_SINGLE;
                else                             r_rsp_fsm = RSP_MULTI; 
            }
        break;
        case RSP_SINGLE:     // consume VCI flit in case of SINGLE DSPIN flit
            r_rsp_fsm = RSP_IDLE;
        break;
        case RSP_MULTI:      // write DSPIN data flit
            if( p_vci.rspval.read() and 
                p_dspin_rsp.read.read()() and  
                p_vci.reop.read() ) r_rsp_fsm = RSP_IDLE;
        break;
    } // end switch r_rsp_fsm

    //////////////////////////////////////////////////////////////
    // DSPIN command packet to VCI command packet
    //////////////////////////////////////////////////////////////
    // - A 2 flits DSPIN read command is translated
    //   to a 1 flit VCI read command.
    // - A N+2 flits DSPIN write command is translated
    //   to a N flits VCI write command.
    // The VCI flits are sent in the CMD_READ, CMD_WDATA states.
    // The r_cmd_buf0 et r_cmd_buf1 buffers are used to store
    // the two first DSPIN flits (in case of write).
    //////////////////////////////////////////////////////////////

    switch(r_cmd_fsm)
    {
        case CMD_IDLE:  // save first DSPIN flit (address)
            if( p_dspin_cmd.write.read() )
            {
                r_cmd_buf0    = p_dspin_cmd.data.read();  
                r_cmd_fsm = CMD_RW;
            }
        break;
        case CMD_RW:   // save second DSPIN flit (command parameters)
            if( p_dspin_cmd.write.read() )
            {
                r_cmd_buf1 = p_dspin_cmd.data.read();  // save command parameters
                if ( (p_dspin_cmd.data.read() & 0x8000000000LL) ) r_cmd_fsm = CMD_READ;
                else                                              r_cmd_fsm = CMD_WDATA;
                r_cmd_count = 0;
            }
        break;
        case CMD_READ: // send VCI flit if READ
            if ( p_vci.cmdack.read() ) r_cmd_fsm = CMD_IDLE;
        break;
        case CMD_WDATA: // send one VCI flit if WRITE
            if( p_dspin_cmd.write.read() && p_vci.cmdack.read() )
            {
                if ( (r_cmd_buf1.read() & 0x0000200000LL) == 0 )  // CONST
                    r_cmd_count = r_cmd_count + 1;
                if ( (p_dspin_cmd.data.read() & 0x8000000000LL) ) // EOP
                    r_cmd_fsm = CMD_IDLE;
            }
        break;
    } // end switch r_cmd_fsm

}  // end transition()

////////////////////////////////
tmpl(void)::genMealy_dspin_rsp()
{
    sc_uint<dspin_rsp_width>    dspin_data;

    if ( r_rsp_fsm.read() == RSP_IDLE )
    {
        p_dspin_rsp.write = p_vci.rspval.read();
        dspin_data = (((sc_uint<dspin_rsp_width>)p_vci.rsrcid.read()) << 18) |
                     (((sc_uint<dspin_rsp_width>)p_vci.rerror.read()) << 16) |
                     (((sc_uint<dspin_rsp_width>)p_vci.rtrdid.read()) << 12) |
                     (((sc_uint<dspin_rsp_width>)p_vci.rpktid.read()) << 8);
        if (  p_vci.reop.read() and 
              (p_vci.rdata.read() == 0) ) dspin_data = dspin_data | 0x100000000LL;              
    }
    else if ( r_rsp_fsm.read() == RSP_SINGLE )
    {
        p_dspin_rsp.write = false;
    }
    else //  r_rsp_fsm == RSP_MULTI 
    {
        p_dspin_rsp.write = p_vci.rspval.read();
        dspin_data = (sc_uint<dspin_rsp_width>)p_vci.rdata.read();
        if ( p_vci.reop.read() )
            dspin_data = dspin_data | 0x100000000LL;
    }
    p_dspin_rsp.data = dspin_data;
}

//////////////////////////////
tmpl(void)::genMealy_vci_rsp()
{
    if ( r_rsp_fsm.read() == RSP_IDLE ) p_vci.rspack = false;
    else                                p_vci.rspack = p_dspin_rsp.read();
}

//////////////////////////////
tmpl(void)::genMealy_vci_cmd()
{
    sc_uint<vci_param::N> address;
    if ( vci_param::N == 40 ) address = (r_cmd_buf0.read() << 1);
    else                      address = (r_cmd_buf0.read() >> (39 - vci_param::N) );

    if ( (r_cmd_fsm.read() == CMD_IDLE) or (r_cmd_fsm.read() == CMD_RW) )
    {
        p_pci.cmdval = false;
    }
    else if ( r_cmd_fsm.read() == CMD_READ )  // READ command
    {
        p_vci.cmdval = true;
        p_vci.address = address;
        p_vci.cmd     = (sc_uint<2>)((r_cmd_buf1.read()            & 0x0001800000LL) >> 23);
        p_vci.wdata   = 0;
        p_vci.be      = (sc_uint<vci_param::B>)((r_cmd_buf1.read() & 0x000000001ELL) >> 1);
        p_vci.srcid   = (sc_uint<vci_param::S>)((r_cmd_buf1.read() & 0x7FFE000000LL) >> 25);
        p_vci.pktid   = (sc_uint<vci_param::P>)((r_cmd_buf1.read() & 0x00000001E0LL) >> 5);
        p_vci.trdid   = (sc_uint<vci_param::T>)((r_cmd_buf1.read() & 0x0000001E00LL) >> 9);
        p_vci.plen    = (sc_uint<vci_param::K>)((r_cmd_buf1.read() & 0x00001FE000LL) >> 13);
        p_vci.contig  = ((r_cmd_buf1.read() & 0x0000400000LL) != 0);
        p_vci.cons    = ((r_cmd_buf1.read() & 0x0000200000LL) != 0);
        p_vci.eop     = true;
    }
    else // r_cmd_fsm == CMD_WDATA : WRITE conmmand
    {
        p_vci.cmdval  = p_dspin_cmd.write.read();
        p_vci.address = address + (r_cmd_count.read()*vci_param::B);
        p_vci.cmd     = (sc_uint<2>)((r_cmd_buf1.read()             & 0x0001800000LL) >> 23);
        p_vci.wdata   = (sc_uint<8*vci_param::B>)(r_fifo_cmd.read() & 0x00FFFFFFFFLL);
        p_vci.be      = (sc_uint<vci_param::B>)((r_fifo_cmd.read()  & 0x0F00000000LL) >> 32);
        p_vci.srcid   = (sc_uint<vci_param::S>)((r_cmd_buf1.read()  & 0x7FFE000000LL) >> 25);
        p_vci.pktid   = (sc_uint<vci_param::P>)((r_cmd_buf1.read()  & 0x00000001E0LL) >> 5);
        p_vci.trdid   = (sc_uint<vci_param::T>)((r_cmd_buf1.read()  & 0x0000001E00LL) >> 9);
        p_vci.plen    = (sc_uint<vci_param::K>)((r_cmd_buf1.read()  & 0x00001FE000LL) >> 13);
        p_vci.contig  = ((r_cmd_buf1.read() & 0x0000400000LL) != 0);
        p_vci.cons    = ((r_cmd_buf1.read() & 0x0000200000LL) != 0);
        p_vci.eop     = ((r_fifo_cmd.read() & 0x8000000000LL) == 0x8000000000LL);

    }
}

////////////////////////////////
tmpl(void)::genMealy_dspin_cmd()
{
    if ( (r_cmd_fsm.read() == CMD_IDLE) or 
              (r_cmd_fsm.read() == CMD_RW) )  p_dspin_cmd.read = true;
    else if ( r_cmd_fsm.read() == CMD_READ )  p_dspin_cmd.read = false;
    else                                      p_dspin_cmd.read = p_vci.cmdack.read();
}

}} // end namespace

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
