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

#include "../include/ring_dspin_half_gateway_target.h"

namespace soclib { namespace caba {

using namespace soclib::common;
using namespace soclib::caba;

#define tmpl(x) template<int cmd_width, int rsp_width> x RingDspinHalfGatewayTarget<cmd_width, rsp_width>

////////////////////////////////////////////////////////////////////////
tmpl(/**/)::RingDspinHalfGatewayTarget(sc_module_name		insname,
               				bool            	alloc_target,
                            		const int       	&cmd_fifo_depth,
                            		const MappingTable 	&mt,
                            		const IntTab 		&ringid,
                            		bool  			local)  
    : soclib::caba::BaseModule(insname),
    m_alloc_target(alloc_target),
    m_local(local),
    m_cmd_fifo("m_cmd_fifo", cmd_fifo_depth),
    m_lt(mt.getLocalityTable(ringid)),
    r_ring_cmd_fsm("r_ring_cmd_fsm"),
    r_ring_rsp_fsm("r_ring_rsp_fsm")
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

    SC_METHOD(genMealy_cmd_out);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.cmd_rok;
    sensitive << p_ring_in.cmd_data;

    SC_METHOD(genMealy_cmd_in);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.cmd_rok;
    sensitive << p_ring_in.cmd_wok;
    sensitive << p_ring_in.cmd_data;

    SC_METHOD(genMoore_cmd_fifo_out);
    dont_initialize();
    sensitive << p_clk.neg();

    SC_METHOD(genMealy_rsp_out);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.rsp_rok;
    sensitive << p_ring_in.rsp_data;
    sensitive << p_gate_rsp_in.write;
    sensitive << p_gate_rsp_in.data;

    SC_METHOD(genMealy_rsp_in);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.rsp_wok;
    sensitive << p_gate_rsp_in.write;

    SC_METHOD(genMealy_rsp_grant);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.rsp_grant;
    sensitive << p_ring_in.rsp_wok;
    sensitive << p_gate_rsp_in.write;
    sensitive << p_gate_rsp_in.data;

    SC_METHOD(genMealy_cmd_grant);
    dont_initialize();
    sensitive << p_clk.neg();
    sensitive << p_ring_in.cmd_grant;
} //  end constructor

////////////////////////
tmpl(void)::transition()       
{
    if ( p_resetn == false ) 
    { 
        if(m_alloc_target) 	r_ring_rsp_fsm = DEFAULT;
        else 			r_ring_rsp_fsm = RSP_IDLE;
        r_ring_cmd_fsm = CMD_IDLE;
        m_cmd_fifo.init();
        return;
    } 
 
    bool                       cmd_fifo_put = false;
    sc_uint<cmd_width>         cmd_fifo_data;
   
    ////// RING RSP FSM //////
    switch( r_ring_rsp_fsm ) {
    case RSP_IDLE:  
        if ( p_ring_in.rsp_grant.read() && p_gate_rsp_in.write.read() ) 
		r_ring_rsp_fsm = DEFAULT;           
    break;
    case DEFAULT:   
        if (p_gate_rsp_in.write.read() && p_ring_in.rsp_wok.read() ) 
		r_ring_rsp_fsm = KEEP;
        else if ( !p_ring_in.rsp_grant.read() )
		r_ring_rsp_fsm = RSP_IDLE;  
    break;
    case KEEP:     
        if(p_gate_rsp_in.write.read() && p_ring_in.rsp_wok.read()) 
        {
            if ((int) ((p_gate_rsp_in.data.read() >> (rsp_width-1)) & 0x1) == 1)  
            {             
                if ( p_ring_in.rsp_grant.read() ) 	r_ring_rsp_fsm = DEFAULT;  
                else   					r_ring_rsp_fsm = RSP_IDLE;                
            } 
        }           
    break;
    } // end switch ring cmd fsm

    /////// RING CMD FSM //////
    bool brdcst = false;
    switch( r_ring_cmd_fsm ) {
    case CMD_IDLE: 
    {
        uint32_t rtgtid = (uint32_t) p_ring_in.cmd_data.read();
        bool loc = (m_lt[rtgtid] && m_local) || (!m_lt[rtgtid] && !m_local);
        brdcst = (rtgtid & 0x3 ) == 0x3;
        if ( p_ring_in.cmd_rok.read() ) 
        {
            if (((brdcst && p_ring_in.cmd_wok.read()) || loc ) && m_cmd_fifo.wok())
            {
                r_ring_cmd_fsm = LOCAL;
                cmd_fifo_put  = true;
                cmd_fifo_data = p_ring_in.cmd_data.read();                                 
            }                     
            else 
            {
                if(!(brdcst || loc) && p_ring_in.cmd_wok.read()) r_ring_cmd_fsm = RING;                  
            }
        } 
    }
    break;
    case LOCAL:         
   	if ( p_ring_in.cmd_rok.read() && m_cmd_fifo.wok() )         
        {
            cmd_fifo_put  = true;
            cmd_fifo_data = p_ring_in.cmd_data.read();
            int reop = (int) ((p_ring_in.cmd_data.read() >> (cmd_width-1)) & 0x1) ;
            if ( reop == 1 ) r_ring_cmd_fsm = CMD_IDLE; 
        } 
    break;
    case RING:        
        if ( p_ring_in.cmd_rok.read() && p_ring_in.cmd_wok.read())        
        {            
            int reop = (int) ((p_ring_in.cmd_data.read() >> (cmd_width-1)) & 0x1) ;
            if ( reop == 1 ) r_ring_cmd_fsm = CMD_IDLE; 
        }
    break;
    } // end switch cmd fsm 

    // local cmd fifo update
    bool cmd_fifo_get = p_gate_cmd_out.read.read();
    if      (  cmd_fifo_put && cmd_fifo_get  ) m_cmd_fifo.put_and_get(cmd_fifo_data);
    else if (  cmd_fifo_put && !cmd_fifo_get ) m_cmd_fifo.simple_put(cmd_fifo_data);
    else if ( !cmd_fifo_put && cmd_fifo_get  ) m_cmd_fifo.simple_get();
}  // end Transition()
   
////////////////////////////////
tmpl(void)::genMealy_rsp_grant()
{
    switch( r_ring_rsp_fsm ) {
    case RSP_IDLE:
        p_ring_out.rsp_grant = p_ring_in.rsp_grant.read() && !p_gate_rsp_in.write.read();
    break;
    case DEFAULT:
        p_ring_out.rsp_grant = !( p_gate_rsp_in.write.read() && p_ring_in.rsp_wok.read() ); 
    break;
    case KEEP:        
        int rsp_fifo_eop = (int) ((p_gate_rsp_in.data.read() >> (rsp_width-1)) & 0x1);
        p_ring_out.rsp_grant = p_gate_rsp_in.write.read() && p_ring_in.rsp_wok.read() && (rsp_fifo_eop==1);
    break; 
    } // end switch
} 

////////////////////////////////
tmpl(void)::genMealy_cmd_grant()
{
    p_ring_out.cmd_grant = p_ring_in.cmd_grant.read();
} 
    
/////////////////////////////
tmpl(void)::genMealy_cmd_in()
{    
    bool brdcst  = false;
    switch( r_ring_cmd_fsm ) {
    case CMD_IDLE:
    {
        uint32_t rtgtid = (uint32_t) p_ring_in.cmd_data.read();
        bool loc = (m_lt[rtgtid] && m_local) || (!m_lt[rtgtid] && !m_local);
        brdcst = (rtgtid & 0x3) == 0x3;
        p_ring_out.cmd_r = p_ring_in.cmd_rok.read() && 
                           ( (brdcst && m_cmd_fifo.wok() && p_ring_in.cmd_wok.read()) ||
                             (loc && m_cmd_fifo.wok()) || 
                             (!(brdcst || loc) && p_ring_in.cmd_wok.read()) ) ;
    }
    break;
    case RING:
        p_ring_out.cmd_r     = p_ring_in.cmd_wok.read();
    break;
    case LOCAL:
        p_ring_out.cmd_r = m_cmd_fifo.wok() && ((brdcst && p_ring_in.cmd_wok.read()) || !brdcst);
    break;
    } // end switch
} 
  
//////////////////////////////
tmpl(void)::genMealy_cmd_out()
{
    p_ring_out.cmd_w    = p_ring_in.cmd_rok.read();
    p_ring_out.cmd_data = p_ring_in.cmd_data.read();
} 
   
/////////////////////////////
tmpl(void)::genMealy_rsp_in()
{
    p_ring_out.rsp_r = p_ring_in.rsp_wok.read();
    if(r_ring_rsp_fsm==DEFAULT || r_ring_rsp_fsm==KEEP) 
	p_gate_rsp_in.read= p_gate_rsp_in.write.read() && p_ring_in.rsp_wok.read();
    else
	p_gate_rsp_in.read=false; 
} 
  
//////////////////////////////
tmpl(void)::genMealy_rsp_out()
{
    if(r_ring_rsp_fsm==RSP_IDLE)
    {
        p_ring_out.rsp_w    = p_ring_in.rsp_rok.read();
        p_ring_out.rsp_data = p_ring_in.rsp_data.read();
    }
    else
    { 
        p_ring_out.rsp_w    =  p_gate_rsp_in.write.read();
        p_ring_out.rsp_data =  p_gate_rsp_in.data.read();
    }
} 

///////////////////////////////////
tmpl(void)::genMoore_cmd_fifo_out()
{
    p_gate_cmd_out.write    = m_cmd_fifo.rok();
    p_gate_cmd_out.data     = m_cmd_fifo.read();
} 

}} // end namespace


