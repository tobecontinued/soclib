/* -*- c++ -*-
  *
  * File : vci_dspin_initiator_wrapper.cpp
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner,
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

#include "../include/vci_dspin_initiator_wrapper.h"

namespace soclib { namespace caba {

#define tmpl(x) template<typename vci_param, size_t dspin_cmd_width, size_t dspin_rsp_width> x VciDspinInitiatorWrapper<vci_param, dspin_cmd_width, dspin_rsp_width>

//////////////////////////////////////////////////////////
tmpl(/**/)::VciDspinInitiatorWrapper(sc_module_name name )
       : soclib::caba::BaseModule(name),
     
        p_clk( "p_clk" ),
        p_resetn( "p_resetn" ),
        p_dspin_cmd( "p_dspin_cmd" ),
        p_dspin_rsp( "p_dspin_rsp" ),
        p_vci( "p_vci" ),

        r_cmd_fsm( "r_cmd_fsm" ),
        r_rsp_fsm( "r_rsp_fsm" ),
        r_rsp_buf( "r_rsp_buf" )
{
	SC_METHOD (transition);
	dont_initialize();
	sensitive << p_clk.pos();

    SC_METHOD (genMealy_vci_cmd);
	dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_dspin_cmd.read;

    SC_METHOD (genMealy_vci_rsp);
	dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_dspin_rsp.data;
    sensitive << p_dspin_rsp.write;

    SC_METHOD (genMealy_dspin_cmd);
	dont_initialize();
    sensitive << p_clk.neg();
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
    sensitive << p_vci.rspack;

    assert( (dspin_cmd_width == 40) && "The DSPIN CMD flit width must have 40 bits");
    assert( (dspin_rsp_width == 33) && "The DSPIN RSP flit width must have 33 bits");
    assert( (vci_param::N    <= 40) && "The VCI ADDRESS field cannot have more than 40 bits");
    assert( (vci_param::B    == 4 ) && "The VCI DATA filds must have 32 bits");
    assert( (vci_param::K    == 8 ) && "The VCI PLEN field cannot have more than 8 bits");
    assert( (vci_param::S    <= 14) && "The VCI SRCID field cannot have more than 14 bits");
    assert( (vci_param::T    <= 4 ) && "The VCI TRDID field cannot have more than 4 bits");
    assert( (vci_param::P    <= 4 ) && "The VCI PKTID field cannot have more than 4 bits");
    assert( (vci_param::E    <= 2 ) && "The VCI RERROR field cannot have more than 2 bits");

}; //  end constructor

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
    // VCI command packet to DSPIN command packet
    /////////////////////////////////////////////////////////////
    // - A N flits VCI write command packet is translated
    //   to a N+2 flits DSPIN command.
    // - A single flit VCI read command packet is translated
    //   to a 2 flits DSPIN command.
    // A DSPIN flit is written on the DSPIN port in all states
    // but a VCI flit is consumed only in the CMD_READ and
    // CMD_WDATA states.
    //////////////////////////////////////////////////////////////

	switch( r_cmd_fsm.read() )
    {
	    case CMD_IDLE:        // transmit first DSPIN CMD flit 
		    if ( p_vci.cmdval.read() and p_dspin_cmd.read.read() )
            {
                if ( (p_vci.cmd.read() == vci_param::CMD_READ) or
                     (p_vci.cmd.read() == vci_param::CMD_LOCKED_READ) ) 
                {
                    r_cmd_fsm = CMD_READ;
                }
                else
                {
                    r_cmd_fsm = CMD_WRITE;
                }
            } 
        break;
        case CMD_READ:        // transmit second DSPIN CMD flit
		    if ( p_vci.cmdval.read() and p_dspin_cmd.read.read() )
            {
                r_cmd_fsm = CMD_IDLE;
            }
        break;
        case CMD_WRITE:       // transmit second DSPIN CMD flit
		    if ( p_vci.cmdval.read() and p_dspin_cmd.read.read() )
            {
                r_cmd_fsm = CMD_WDATA;
            }
        break;
        case CMD_WDATA:     // transfer DSPIN DATA flits for a WRITE 
		    if ( p_vci.cmdval.read() and 
                 p_dspin_cmd.read.read() and
                 p_vci.eop.read() )
            {
                r_cmd_fsm = CMD_IDLE;
            } 
        break;
    }  // end switch CMD

    /////////////////////////////////////////////////////////////////
    // DSPIN response packet to VCI response packet
    /////////////////////////////////////////////////////////////////
    // - A N+1 flits DSPIN response packet is translated
    //   to a N flits VCI response.
    // - A single flit DSPIN response packet is translated
    //   to a single flit VCI response with RDATA = 0.
    // A valid DSPIN flit is always consumed in the CMD_IDLE 
    // state, but no VCI flit is transmitted.
    // The VCI flits are sent in the RSP_READ & RSP_WRITE states.
    /////////////////////////////////////////////////////////////////

    bool is_eop = (p_dspin_rsp.data.read() & 0x8000000000LL);

    switch( r_rsp_fsm.read() )
    {
        case RSP_IDLE:     // try to transmit VCI flit if  WRITE
            if ( p_dspin_rsp.write.read() )  
            {
                r_rsp_buf = p_dspin_rsp.data.read();
                if ( not is_eop )                    r_rsp_fsm = RSP_READ; 
                else if ( not p_vci.rspack.read() )  r_rsp_fsm = RSP_WRITE;
            }
        break;
        case RSP_READ:    // try to transmit a flit VCI for a READ
            if ( p_vci.rspack.read() and 
                 p_dspin_rsp.write.read() and
                 is_eop )              r_rsp_fsm = RSP_IDLE;
        break;
        case RSP_WRITE:    // try to transmit a VCI flit for a WRITE
            if ( p_vci.rspack.read() ) r_rsp_fsm = RSP_IDLE;
        break;
    } // end switch RSP

}  // end transition

//////////////////////////////
tmpl(void)::genMealy_vci_cmd()
{
    if ( (r_cmd_fsm.read() == CMD_IDLE) or
         (r_cmd_fsm.read() == CMD_WRITE) ) p_vci.cmdack = false;
    else                                   p_vci.cmdack = p_dspin_cmd.read.read();
}
////////////////////////////////
tmpl(void)::genMealy_dspin_cmd()
{
    sc_uint<dspin_cmd_width> dspin_data;

    if      ( r_cmd_fsm.read() == CMD_IDLE )
    {
        sc_uint<dspin_cmd_width> address = (sc_uint<dspin_cmd_width>)p_vci.address.read();
        address = (address >> 2) << (dspin_cmd_width - vci_param::N + 1);
    }
    else if ( (r_cmd_fsm.read() == CMD_READ) or
              (r_cmd_fsm.read() == CMD_WRITE) )
    {
        sc_uint<dspin_cmd_width> be      = (sc_uint<dspin_cmd_width>)p_vci.be.read();
        sc_uint<dspin_cmd_width> srcid   = (sc_uint<dspin_cmd_width>)p_vci.srcid.read();
        sc_uint<dspin_cmd_width> pktid   = (sc_uint<dspin_cmd_width>)p_vci.pktid.read();
        sc_uint<dspin_cmd_width> trdid   = (sc_uint<dspin_cmd_width>)p_vci.trdid.read();
        sc_uint<dspin_cmd_width> cmd     = (sc_uint<dspin_cmd_width>)p_vci.cmd.read();
        sc_uint<dspin_cmd_width> plen    = (sc_uint<dspin_cmd_width>)p_vci.plen.read();
        dspin_data = ((be    << 1 ) & 0x000000001ELL) |
                     ((pktid << 5 ) & 0x00000001E0LL) |
                     ((trdid << 9 ) & 0x0000001E00LL) |
                     ((plen  << 13) & 0x00001FE000LL) |
                     ((cmd   << 23) & 0x0001800000LL) |
                     ((srcid << 25) & 0x7FFE000000LL) ;
        if ( p_vci.contig.read() )   dspin_data = dspin_data | 0x0000400000LL ;
        if ( p_vci.cons.read()   )   dspin_data = dspin_data | 0x0000200000LL ;
        if ( r_cmd_fsm == CMD_READ ) dspin_data = dspin_data | 0x8000000000LL ;
    }
    else  // r_cmd_fsm == CMD_WDATA
    {
        sc_uint<dspin_cmd_width> wdata = (sc_uint<dspin_cmd_width>)p_vci.wdata.read();
        sc_uint<dspin_cmd_width> be    = (sc_uint<dspin_cmd_width>)p_vci.be.read();
        dspin_data =  (wdata      & 0x00FFFFFFFFLL) |
                      ((be << 32) & 0x0F00000000LL) ;
        if ( p_vci.eop.read() ) dspin_data = dspin_data | 0x8000000000LL;
    }
    p_dspin_cmd.write = p_vci.cmdval.read();
    p_dspin_cmd.data  = dspin_data;
}
////////////////////////////////
tmpl(void)::genMealy_dspin_rsp()
{
    if      ( r_rsp_fsm.read() == RSP_IDLE )   p_dspin_rsp.read = true;
    else if ( r_rsp_fsm.read() == RSP_READ )   p_dspin_rsp.read = p_vci.rspack.read();
    else                                       p_dspin_rsp.read = false;
}
//////////////////////////////
tmpl(void)::genMealy_vci_rsp()
{
    bool dspin_eop = (p_dspin_rsp.data.read() & 0x100000000LL);

    if ( r_rsp_fsm.read() == RSP_IDLE )
    {
        p_vci.rspval = p_dspin_rsp.write.read() and dspin_eop;
        p_vci.rdata  = 0;
        p_vci.rsrcid = (sc_uint<vci_param::S>)((p_dspin_rsp.data.read() & 0x0FFFC0000LL) >> 18);
        p_vci.rpktid = (sc_uint<vci_param::T>)((p_dspin_rsp.data.read() & 0x000000F00LL) >> 8);
        p_vci.rtrdid = (sc_uint<vci_param::P>)((p_dspin_rsp.data.read() & 0x00000F000LL) >> 12);
        p_vci.rerror = (sc_uint<vci_param::E>)((p_dspin_rsp.data.read() & 0x000030000LL) >> 16);
        p_vci.reop   = dspin_eop;
    }
    else if ( r_rsp_fsm == RSP_READ )
    {
        p_vci.rspval = p_dspin_rsp.write.read();
        p_vci.rdata  = (sc_uint<8*vci_param::B>)(p_dspin_rsp.data.read() & 0x0FFFFFFFFLL);
        p_vci.rsrcid = (sc_uint<vci_param::S>)((r_rsp_buf.read()         & 0x0FFFC0000LL) >> 18);
        p_vci.rpktid = (sc_uint<vci_param::T>)((r_rsp_buf.read()         & 0x000000F00LL) >> 8);
        p_vci.rtrdid = (sc_uint<vci_param::P>)((r_rsp_buf.read()         & 0x00000F000LL) >> 12);
        p_vci.rerror = (sc_uint<vci_param::E>)((r_rsp_buf.read()         & 0x000030000LL) >> 16);
        p_vci.reop   = dspin_eop;
    }
    else //  r_rsp_fsm == RSP_WRITE
    {
        p_vci.rspval = true;
        p_vci.rdata  = 0;
        p_vci.rsrcid = (sc_uint<vci_param::S>)((r_rsp_buf.read() & 0x0FFFC0000LL) >> 18);
        p_vci.rpktid = (sc_uint<vci_param::T>)((r_rsp_buf.read() & 0x000000F00LL) >> 8);
        p_vci.rtrdid = (sc_uint<vci_param::P>)((r_rsp_buf.read() & 0x00000F000LL) >> 12);
        p_vci.rerror = (sc_uint<vci_param::E>)((r_rsp_buf.read() & 0x000030000LL) >> 16);
        p_vci.reop   = true;
    }
}

}} // end namespace

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
