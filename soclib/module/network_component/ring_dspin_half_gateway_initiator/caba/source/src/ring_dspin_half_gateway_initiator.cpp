/* -*- c++ -*-
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
 * Authors  : Franck WAJSBÜRT, Abdelmalek SI MERABET, Alain GREINER
 * Date     : july 2010
 * Copyright: UPMC - LIP6
 */

#include "../include/ring_dspin_half_gateway_initiator.h"

namespace soclib { namespace caba {

using namespace soclib::common;
using namespace soclib::caba;

#define tmpl(x) template<int cmd_width, int rsp_width> x RingDspinHalfGatewayInitiator<cmd_width, rsp_width>

////////////////////////////////////////////////////////////////////////////////
tmpl(/**/)::RingDspinHalfGatewayInitiator(   sc_module_name		insname,
						bool            	alloc_init,
						const int       	&rsp_fifo_depth,
						const MappingTable 	&mt,
						const IntTab 		&ringid,
						bool 			local)
    : soclib::caba::BaseModule(insname),
    r_ring_cmd_fsm("r_ring_cmd_fsm"),
    r_ring_rsp_fsm("r_ring_rsp_fsm"),
    m_alloc_init(alloc_init),
    m_local(local),
    m_rsp_fifo("m_rsp_fifo", rsp_fifo_depth),
    m_lt(mt.getIdLocalityTable(ringid))
{
    if (cmd_width != 37)
    {
        std::cout << "error in ring_dspin_gateway : the CMD flit width must be 37 bits" << std::endl;
        exit(0);
    }
    if (rsp_width != 33)
    {
        std::cout << "error in ring_dspin_gateway : the RSP flit width must be 33 bits" << std::endl;
        exit(0);
    }

    SC_METHOD (transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMealy_rsp_out);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.rsp_rok;
    sensitive << p_ring_in.rsp_data;

    SC_METHOD(genMealy_rsp_in);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.rsp_rok;
    sensitive << p_ring_in.rsp_wok;
    sensitive << p_ring_in.rsp_data;

    SC_METHOD(genMoore_rsp_fifo_out);
    dont_initialize();
    sensitive << p_clk.neg();

    SC_METHOD(genMealy_cmd_out);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.cmd_rok;
    sensitive << p_ring_in.cmd_data;
    sensitive << p_gate_cmd_in.write;
    sensitive << p_gate_cmd_in.data;

    SC_METHOD(genMealy_cmd_in);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.cmd_wok;
    sensitive << p_gate_cmd_in.write;

    SC_METHOD(genMealy_cmd_grant);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.cmd_grant;
    sensitive << p_ring_in.cmd_wok;
    sensitive << p_gate_cmd_in.write;
    sensitive << p_gate_cmd_in.data;

    SC_METHOD(genMealy_rsp_grant);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.rsp_grant;
} //  end constructor

////////////////////////
tmpl(void)::transition()       
{
    if ( p_resetn == false ) 
    { 
        if(m_alloc_init) 	r_ring_cmd_fsm = DEFAULT;
        else 			r_ring_cmd_fsm = CMD_IDLE;
        r_ring_rsp_fsm = RSP_IDLE;
        m_rsp_fifo.init();
        return;
    } // end if reset
   
    bool                       rsp_fifo_put = false;
    sc_uint<rsp_width>         rsp_fifo_data;

    ////// RING CMD FSM //////
    switch( r_ring_cmd_fsm ) {
    case CMD_IDLE:         
        if ( p_ring_in.cmd_grant.read() && p_gate_cmd_in.write.read() )                        
		r_ring_cmd_fsm = DEFAULT; 
    break;
    case DEFAULT:          
        if ( p_gate_cmd_in.write.read() && p_ring_in.cmd_wok.read() ) 
		r_ring_cmd_fsm = KEEP;             
        else if ( !p_ring_in.cmd_grant.read() )
		r_ring_cmd_fsm = CMD_IDLE; 
    break;
    case KEEP:                              
        if(p_gate_cmd_in.write.read() && p_ring_in.cmd_wok.read() ) 
        {
            if (((int) (p_gate_cmd_in.data.read() >> (cmd_width-1)) & 0x1) == 1) 
            {  
                if ( p_ring_in.cmd_grant.read() ) 	r_ring_cmd_fsm = DEFAULT;  
                else   					r_ring_cmd_fsm = CMD_IDLE; 
            }        
        }      
    break;
    } // end switch ring cmd fsm
 
    ///// RING RSP FSM //////
    switch( r_ring_rsp_fsm ) {
    case RSP_IDLE:
    {  
        int rsrcid = (int)  ((p_ring_in.rsp_data.read() >> 20 ) & 0xFF);
        bool loc = (m_lt[rsrcid] && m_local) || (!m_lt[rsrcid] && !m_local);

        if ( p_ring_in.rsp_rok.read() ) 
        {          
            if ( loc && m_rsp_fifo.wok()) 
            {
                r_ring_rsp_fsm = LOCAL;
                rsp_fifo_put  = true;
                rsp_fifo_data = p_ring_in.rsp_data.read();  
            }
            else 
            {
                if(!loc && p_ring_in.rsp_wok.read())  r_ring_rsp_fsm = RING;  
            } 
        } 
    }
    break;
    case LOCAL:
        if ( p_ring_in.rsp_rok.read() && m_rsp_fifo.wok() )         
        {
            rsp_fifo_put  = true;
            rsp_fifo_data = p_ring_in.rsp_data.read();
            int reop = (int) ((p_ring_in.rsp_data.read() >> (rsp_width-1)) & 0x1) ;
            if ( reop == 1 ) r_ring_rsp_fsm = RSP_IDLE;             
        } 
    break;
    case RING:     
        if ( p_ring_in.rsp_rok.read() && p_ring_in.rsp_wok.read())
        {
        int reop = (int) ((p_ring_in.rsp_data.read() >> (rsp_width-1)) & 0x1) ;
        if ( reop == 1 ) r_ring_rsp_fsm = RSP_IDLE; 
        }
    break;
    } // end switch rsp fsm
      
    // local rsp fifo update
    bool rsp_fifo_get = p_gate_rsp_out.read.read(); 
    if      (  rsp_fifo_put &&  rsp_fifo_get ) m_rsp_fifo.put_and_get(rsp_fifo_data);
    else if (  rsp_fifo_put && !rsp_fifo_get ) m_rsp_fifo.simple_put(rsp_fifo_data);
    else if ( !rsp_fifo_put &&  rsp_fifo_get ) m_rsp_fifo.simple_get();
     
}  // end Transition()

////////////////////////////////
tmpl(void)::genMealy_cmd_grant()
{    
    switch( r_ring_cmd_fsm ) {
    case CMD_IDLE:
        p_ring_out.cmd_grant = p_ring_in.cmd_grant.read() && !p_gate_cmd_in.write.read();
    break;
    case DEFAULT:        
        p_ring_out.cmd_grant = !(p_gate_cmd_in.write.read() && p_ring_in.cmd_wok.read());
    break;
    case KEEP:
        int cmd_fifo_eop = (int) ((p_gate_cmd_in.data.read() >> (cmd_width-1)) & 0x1) ;
        p_ring_out.cmd_grant = p_gate_cmd_in.write.read() && p_ring_in.cmd_wok.read() && (cmd_fifo_eop==1);
    break;
    } // end switch
}

////////////////////////////////
tmpl(void)::genMealy_rsp_grant()
{
    p_ring_out.rsp_grant = p_ring_in.rsp_grant.read();
} 

///////////////////////////////////
tmpl(void)::genMoore_rsp_fifo_out()
{
    p_gate_rsp_out.write      = m_rsp_fifo.rok();
    p_gate_rsp_out.data       = m_rsp_fifo.read();
}   

/////////////////////////////
tmpl(void)::genMealy_rsp_in()
{
    switch( r_ring_rsp_fsm ) {
    case RSP_IDLE:
    {
        int rsrcid = (int)  ((p_ring_in.rsp_data.read() >> 20 ) & 0xFF);
        bool loc = (m_lt[rsrcid] && m_local) || (!m_lt[rsrcid] && !m_local);
        p_ring_out.rsp_r = p_ring_in.rsp_rok.read() && 
                           ((loc && m_rsp_fifo.wok()) || (!loc && p_ring_in.rsp_wok.read()));
    }
    break;
    case RING:
        p_ring_out.rsp_r     = p_ring_in.rsp_wok.read();
    break;
    case LOCAL:
        p_ring_out.rsp_r     = m_rsp_fifo.wok();
    break;
    } // end switch
} // end genMealy_rsp_in_r

//////////////////////////////
tmpl(void)::genMealy_rsp_out()
{
    p_ring_out.rsp_w    = p_ring_in.rsp_rok.read();
    p_ring_out.rsp_data = p_ring_in.rsp_data.read();
} // end genMealy_rsp_out_w

/////////////////////////////
tmpl(void)::genMealy_cmd_in()
{
    p_ring_out.cmd_r = p_ring_in.cmd_wok.read(); 

    // J'ai un doute sur la dépendance combinatoire ci-dessous 
    // entre entre p_gate_cmd_in.read et et p_gate_cmd_in.write / A.G.
    if(r_ring_cmd_fsm==DEFAULT  || r_ring_cmd_fsm==KEEP) 
	p_gate_cmd_in.read = p_gate_cmd_in.write.read() && p_ring_in.cmd_wok.read();
    else
	p_gate_cmd_in.read=false;         
} // end genMealy_cmd_in
    
//////////////////////////////
tmpl(void)::genMealy_cmd_out()
{
    if(r_ring_cmd_fsm==CMD_IDLE)
    {
        p_ring_out.cmd_w    = p_ring_in.cmd_rok.read();
        p_ring_out.cmd_data = p_ring_in.cmd_data.read(); 
    } 
    else
    {
        p_ring_out.cmd_w    =  p_gate_cmd_in.write.read();
        p_ring_out.cmd_data =  p_gate_cmd_in.data.read(); 
    }
} // end genMealy_cmd_out

}} // end namespace


