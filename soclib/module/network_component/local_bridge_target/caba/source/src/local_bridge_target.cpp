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
 * Authors  : Franck WAJSBÜRT, Abdelmalek SI MERABET 
 * Date     : december 2008
 *
 * Copyright: UPMC - LIP6
 */
#include "../include/local_bridge_target.h"
#include <iostream>
#include <time.h>

namespace soclib { namespace caba {

///////////////////////////////////////////////
//	constructor
///////////////////////////////////////////////
LocalBridgeTarget::LocalBridgeTarget(sc_module_name	insname,
                            bool            alloc_target,
                            const int       &cmd_fifo_depth,
                            const soclib::common::MappingTable &mt,
                            const soclib::common::IntTab &ringid)
    : soclib::caba::BaseModule(insname),
	r_ring_cmd_fsm("r_ring_cmd_fsm"),
	r_ring_rsp_fsm("r_ring_rsp_fsm"),
	m_alloc_target(alloc_target),
        m_cmd_fifo("m_cmd_fifo", cmd_fifo_depth),
        m_lt(mt.getLocalityTable(ringid))
{

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
sensitive << p_rsp_fifo.rok;
sensitive << p_rsp_fifo.data;

SC_METHOD(genMealy_rsp_in);
dont_initialize();
sensitive << p_clk.neg();
sensitive << p_ring_in.rsp_wok;
sensitive << p_rsp_fifo.rok;

SC_METHOD(genMealy_rsp_grant);
dont_initialize();
sensitive << p_clk.neg();
sensitive << p_ring_in.rsp_grant;
sensitive << p_ring_in.rsp_wok;
sensitive << p_rsp_fifo.rok;
sensitive << p_rsp_fifo.data;

SC_METHOD(genMealy_cmd_grant);
dont_initialize();
sensitive << p_clk.neg();
sensitive << p_ring_in.cmd_grant;

} //  end constructor

////////////////////////////////
//	transition 
////////////////////////////////
void LocalBridgeTarget::transition()       
{

	bool                       cmd_fifo_put = false;
	sc_uint<37>                cmd_fifo_data;
    
	if ( p_resetn == false ) 
	{ 
		if(m_alloc_target)
			r_ring_rsp_fsm = DEFAULT;
		else
			r_ring_rsp_fsm = RSP_IDLE;

		r_ring_cmd_fsm = CMD_IDLE;
		m_cmd_fifo.init();
             
		return;
	} 
    
//////////// RING RSP FSM (distributed) /////////////////////////
        
	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE:   
    
			if ( p_ring_in.rsp_grant.read() && p_rsp_fifo.rok.read()) 
				r_ring_rsp_fsm = DEFAULT;           
		break;

		case DEFAULT:         
      
			if (p_rsp_fifo.rok.read() && p_ring_in.rsp_wok.read()) 
			{
				r_ring_rsp_fsm = KEEP;
			}   
			else if ( !p_ring_in.rsp_grant.read() )
				r_ring_rsp_fsm = RSP_IDLE;  
		break;

		case KEEP:         
       
			if(p_rsp_fifo.rok.read() && p_ring_in.rsp_wok.read()) 
			{
				if ((int) ((p_rsp_fifo.data.read() >> 32 ) & 0x1) == 1)  
				{             
					if ( p_ring_in.rsp_grant.read() )
						r_ring_rsp_fsm = DEFAULT;  
					else   
						r_ring_rsp_fsm = RSP_IDLE;                
				} 
			}           
		break;

} // end switch ring cmd fsm

/////////// RING CMD FSM ////////////////////////
	switch( r_ring_cmd_fsm ) 
	{
		case CMD_IDLE:    
		{
			uint32_t rtgtid = (uint32_t) p_ring_in.cmd_data.read();
			bool Local = m_lt[rtgtid];    

			if ( p_ring_in.cmd_rok.read() ) 
			{
                 
				if ( !Local && m_cmd_fifo.wok()) 
				{
					r_ring_cmd_fsm = GATE;
					cmd_fifo_put  = true;
					cmd_fifo_data = p_ring_in.cmd_data.read();
				}                     
				else 
				{
					if(Local && p_ring_in.cmd_wok.read()) 
						r_ring_cmd_fsm = RING;                  
				}  
			} 
		}  
		break;

		case GATE:         
			if ( p_ring_in.cmd_rok.read() && m_cmd_fifo.wok() )         
			{
				cmd_fifo_put  = true;
				cmd_fifo_data = p_ring_in.cmd_data.read();
				int reop = (int) ((p_ring_in.cmd_data.read() >> 36 ) & 0x1) ;
				if ( reop == 1 ) 
					r_ring_cmd_fsm = CMD_IDLE; 
			} 
		break;

		case RING:        
			if ( p_ring_in.cmd_rok.read() && p_ring_in.cmd_wok.read())        
			{            
				if ((int) ((p_ring_in.cmd_data.read() >> 36 ) & 0x1) == 1 ) 
					r_ring_cmd_fsm = CMD_IDLE; 
			}

		break;

	} // end switch cmd fsm 

    ////////////////////////
    //  fifos update      //
   ////////////////////////
	bool cmd_fifo_get = p_cmd_fifo.wok.read();
// local cmd fifo update
	if ( cmd_fifo_put && cmd_fifo_get ) m_cmd_fifo.put_and_get(cmd_fifo_data);
	else if (  cmd_fifo_put && !cmd_fifo_get ) m_cmd_fifo.simple_put(cmd_fifo_data);
	else if ( !cmd_fifo_put && cmd_fifo_get ) m_cmd_fifo.simple_get();

}  // end Transition()

///////////////////////////////////////////////////////////////////
void LocalBridgeTarget::genMealy_rsp_grant()
///////////////////////////////////////////////////////////////////
{
	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE:
			p_ring_out.rsp_grant = p_ring_in.rsp_grant.read() && !p_rsp_fifo.rok.read();
		break;

		case DEFAULT:
			p_ring_out.rsp_grant = !( p_rsp_fifo.rok.read()&& p_ring_in.rsp_wok.read() ); 
		break;

		case KEEP:        
			int rsp_fifo_eop = (int) ((p_rsp_fifo.data.read() >> 32) & 0x1);
			p_ring_out.rsp_grant = p_rsp_fifo.rok.read() && p_ring_in.rsp_wok.read() && (rsp_fifo_eop==1);
		break; 

	} // end switch
} // end genMealy_rsp_grant

///////////////////////////////////////////////////////////////////
void LocalBridgeTarget::genMealy_cmd_grant()
///////////////////////////////////////////////////////////////////
{
	p_ring_out.cmd_grant = p_ring_in.cmd_grant.read();
} // end genMealy_cmd_grant
    
///////////////////////////////////////////////////////////////////
void LocalBridgeTarget::genMealy_cmd_in()
///////////////////////////////////////////////////////////////////
{     
	switch( r_ring_cmd_fsm ) 
	{

		case CMD_IDLE:
		{
			uint32_t rtgtid = (uint32_t) p_ring_in.cmd_data.read();
			bool Local = m_lt[rtgtid]; 

			p_ring_out.cmd_r = p_ring_in.cmd_rok.read() && ((!Local && m_cmd_fifo.wok()) || (Local && p_ring_in.cmd_wok.read())) ;
		}
		break;

		case RING:
			p_ring_out.cmd_r     = p_ring_in.cmd_wok.read();
		break;

		case GATE:
			p_ring_out.cmd_r     = m_cmd_fifo.wok();
		break;

	} // end switch
} // end genMealy_cmd_in_r
  
///////////////////////////////////////////////////////////////////
void LocalBridgeTarget::genMealy_cmd_out()
///////////////////////////////////////////////////////////////////
{
     
	p_ring_out.cmd_w    = p_ring_in.cmd_rok.read();
	p_ring_out.cmd_data = p_ring_in.cmd_data.read();
    
} // end genMealy_cmd_out_w

   
///////////////////////////////////////////////////////////////////
void LocalBridgeTarget::genMealy_rsp_in()
//////////////////////////////////////////////////////////////////
{
 
	p_ring_out.rsp_r = p_ring_in.rsp_wok.read();
    
	if(r_ring_rsp_fsm==DEFAULT  || r_ring_rsp_fsm==KEEP) 
		p_rsp_fifo.r= p_rsp_fifo.rok.read() && p_ring_in.rsp_wok.read();
	else
		p_rsp_fifo.r=false; 
} // end genMealy_rsp_in
  
///////////////////////////////////////////////////////////////////
void LocalBridgeTarget::genMealy_rsp_out()
///////////////////////////////////////////////////////////////////
{
  
	if(r_ring_rsp_fsm==RSP_IDLE)
	{
		p_ring_out.rsp_w    = p_ring_in.rsp_rok.read();
		p_ring_out.rsp_data = p_ring_in.rsp_data.read();
	}
	else
	{    
		p_ring_out.rsp_w    =  p_rsp_fifo.rok.read();
		p_ring_out.rsp_data =  p_rsp_fifo.data.read();
	}
                                     
} // end genMealy_rsp_out
///////////////////////////////////////////////////////////////////
void LocalBridgeTarget::genMoore_cmd_fifo_out()
///////////////////////////////////////////////////////////////////
{
	p_cmd_fifo.w    = m_cmd_fifo.rok();
	p_cmd_fifo.data = m_cmd_fifo.read();

} 
}} // end namespace


