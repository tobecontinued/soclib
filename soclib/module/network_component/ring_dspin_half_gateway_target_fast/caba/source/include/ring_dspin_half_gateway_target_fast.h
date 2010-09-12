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
 * Author   : Abdelmalek SI MERABET 
 * Date     : March 2010
 *
 * Copyright: UPMC - LIP6
 */

#ifndef RING_DSPIN_HALF_GATEWAY_TARGET_FAST_H
#define RING_DSPIN_HALF_GATEWAY_TARGET_FAST_H

#include <systemc>
#include "vci_initiator.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "ring_signals_2.h"
#include "dspin_interface.h"

#define HT_DEBUG
//#define HT_DEBUG_FSM

namespace soclib { namespace caba {

using soclib::common::IntTab;

#ifdef HT_DEBUG_FSM
namespace {
        const char *ring_rsp_fsm_state_str_ht[] = {
                "RSP_IDLE",
                "DEFAULT",
                "KEEP",
        };
        const char *ring_cmd_fsm_state_str_ht[] = {
                "CMD_IDLE",
                "BROADCAST_0",
                "BROADCAST_1",
                "LOCAL",
                "RING",
        };
}
#endif

template<typename vci_param, int ring_cmd_data_size, int ring_rsp_data_size>
class RingDspinHalfGatewayTargetFast
{

typedef typename vci_param::fast_addr_t vci_addr_t;
typedef RingSignals2 ring_signal_t; 
//typedef soclib::caba::GateTarget2<ring_cmd_data_size, ring_rsp_data_size> gate_target_t;

typedef soclib::caba::DspinOutput<ring_cmd_data_size >  cmd_out_t;
typedef soclib::caba::DspinInput<ring_rsp_data_size >   rsp_in_t;

private:
        
        enum ring_cmd_fsm_state_e {
        	CMD_IDLE,	 // waiting for first flit of a command packet
                BROADCAST_0,
                BROADCAST_1,
        	LOCAL,  	// next flit of a local cmd packet
        	RING,  	       // next flit of a ring cmd packet
        };
        
        // cmd token allocation fsm
        enum ring_rsp_fsm_state_e {
        	RSP_IDLE,	    
        	DEFAULT,  	
        	KEEP,  	            
        };
        
        // structural parameters
 	std::string   m_name;
        bool          m_alloc_target;
        uint32_t      m_current_cycle;
        uint32_t      max_cycle;   
 
        // internal fifos 
        GenericFifo<uint64_t > m_cmd_fifo;     // fifo for the local command paquet
        GenericFifo<uint64_t > m_rsp_fifo;     // fifo for the local response paquet
        
        // locality table 
	soclib::common::AddressDecodingTable<vci_addr_t, bool> m_lt;
        soclib::common::IntTab m_ringid;

        bool          m_local;

        // internal registers
        sc_signal<int>	        r_ring_cmd_fsm;	    // ring command packet FSM 
        sc_signal<int>		r_ring_rsp_fsm;	    // ring response packet FSM

        uint32_t trace_catch()
        {
                //uint32_t time_stamp=0;
                char *ctime_stamp= getenv("FROM_CYCLE");
                
                //if (ctime_stamp) time_stamp=atoi(ctime_stamp); 	
                return ctime_stamp ? atoi(ctime_stamp) : 99999999;
        
        }
public :

RingDspinHalfGatewayTargetFast(
	const char     *name,
        bool            alloc_target,
        const int       &wrapper_fifo_depth,
        const soclib::common::MappingTable &mt,
        const soclib::common::IntTab &ringid,
        bool  local) 
    :   m_name(name), 
        m_alloc_target(alloc_target),
        m_cmd_fifo("m_cmd_fifo", wrapper_fifo_depth),
        m_rsp_fifo("m_rsp_fifo", wrapper_fifo_depth),
        m_lt(mt.getLocalityTable<typename vci_param::fast_addr_t>(ringid)),
        m_ringid(ringid),
        m_local(local),
       	r_ring_cmd_fsm("r_ring_cmd_fsm"),
	r_ring_rsp_fsm("r_ring_rsp_fsm")
{
} //  end constructor

void reset()
{
        if(m_alloc_target)
        	r_ring_rsp_fsm = DEFAULT;
        else
        	r_ring_rsp_fsm = RSP_IDLE;
        
        r_ring_cmd_fsm = CMD_IDLE;
        m_cmd_fifo.init();
        m_rsp_fifo.init();     
        m_current_cycle  = 0;
        max_cycle = trace_catch(); 
}
////////////////////////////////
//	transition 
////////////////////////////////
//void transition(const gate_target_t &p_gate_target, const ring_signal_t p_ring_in)       
void transition(const cmd_out_t &p_gate_cmd_out, const rsp_in_t &p_gate_rsp_in, const ring_signal_t p_ring_in)
{

//	bool      cmd_fifo_get = false;
	bool      cmd_fifo_put = false;
	uint64_t  cmd_fifo_data = 0;
	
	bool      rsp_fifo_get = false;
	bool      rsp_fifo_put = false;
	uint64_t  rsp_fifo_data = 0;

#ifdef HT_DEBUG_FSM
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name
             << " -- ring cmd = " << ring_cmd_fsm_state_str_ht[r_ring_cmd_fsm]
             << " -- ring rsp = " << ring_rsp_fsm_state_str_ht[r_ring_rsp_fsm] 
             <<< std::endl;
#endif
	
//////////// VCI CMD FSM /////////////////////////

	if (p_gate_rsp_in.write) {
		rsp_fifo_data = (uint64_t) p_gate_rsp_in.data.read();
		rsp_fifo_put =  m_rsp_fifo.wok();
	}

	bool cmd_fifo_get = p_gate_cmd_out.read;
   
//////////// RING RSP FSM (distributed) /////////////////////////
        
	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE:   
#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name
          << " -- ring_rsp_fsm : RSP_IDLE"
          << " -- fifo rok : " <<  m_rsp_fifo.rok()
          << " -- ring rok : " <<  p_ring_in.rsp_w
          << " -- ringin rsp grant : " << p_ring_in.rsp_grant
          << " -- fifo data  : " << std::hex << m_rsp_fifo.read()
          << std::endl;
#endif    
			if ( p_ring_in.rsp_grant && m_rsp_fifo.rok() ) 

				r_ring_rsp_fsm = KEEP;           
                
		break;

		case DEFAULT:  
#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name
          << " --  ring_rsp_fsm : DEFAULT " 
          << " -- fifo_rsp_rok : " << m_rsp_fifo.rok()
          << " -- fifo_rsp_data : " << std::hex << m_rsp_fifo.read()
          << std::endl;
#endif			
                        if ( m_rsp_fifo.rok()) // && p_ring_in.rsp_r ) 
			{

				rsp_fifo_get = p_ring_in.rsp_r; //true;
				r_ring_rsp_fsm = KEEP;
			}   
			else if ( !p_ring_in.rsp_grant )
				r_ring_rsp_fsm = RSP_IDLE;  
		break;

		case KEEP:   
             
			if(m_rsp_fifo.rok() && p_ring_in.rsp_r) 
			{
#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name
          << " -- ring_rsp_fsm : KEEP "
          << " -- fifo_rok : " << m_rsp_fifo.rok()
          << " -- ring_in_wok : " << p_ring_in.rsp_r
          << " -- fifo_out_data : " << std::hex << m_rsp_fifo.read()
          << std::endl;
#endif
				rsp_fifo_get = true;              
				if ((int) ((m_rsp_fifo.read() >> 32 ) & 0x1) == 1)  
				{             
					if ( p_ring_in.rsp_grant )
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
			vci_addr_t rtgtid = (vci_addr_t) ((p_ring_in.cmd_data >> (ring_cmd_data_size-vci_param::N+1)) << 2);
                        int cluster =  (int) ((sc_dt::sc_uint<vci_param::S-4>) (p_ring_in.cmd_data >> 9)); 
                        bool brdcst = ((p_ring_in.cmd_data & 0x1) == 0X1) && (IntTab(cluster) == m_ringid);                       
                        bool loc = !((p_ring_in.cmd_data & 0x1) == 0x1) && !m_lt[rtgtid] && !m_local;
                        bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);
 
#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
              << " -- ring_cmd_fsm -- CMD_IDLE "
              << " - in rok : " << p_ring_in.cmd_w
	      << " - addr : " << std::hex << rtgtid
              << " - brdcst : " << brdcst
	      << " - eop : " << eop
              << " - isloc : " << loc 
              << " - in wok : " << p_ring_in.cmd_r
              << " - fifo wok : " << m_cmd_fifo.wok()
	      << " - ringid  : " << m_ringid
              << " - cluster : " << cluster
              << " - intTab : " << IntTab(cluster)
              << std::endl;
#endif
                      if(p_ring_in.cmd_w && !eop && brdcst && !m_cmd_fifo.wok()) {
                              r_ring_cmd_fsm = BROADCAST_0; 
                      }  
        
                      if(p_ring_in.cmd_w && !eop && brdcst && m_cmd_fifo.wok()) {
                              r_ring_cmd_fsm = BROADCAST_1;
                              cmd_fifo_put  = true;
                      	      cmd_fifo_data = p_ring_in.cmd_data;

                      }  
        
                      if (p_ring_in.cmd_w && !eop && !brdcst && loc) {
                              r_ring_cmd_fsm = LOCAL; 
                              cmd_fifo_put   = m_cmd_fifo.wok();
                              cmd_fifo_data  = p_ring_in.cmd_data;  

                      } 
        
                      if (p_ring_in.cmd_w && !eop && !brdcst && !loc) {
                              r_ring_cmd_fsm = RING;
                      }  

                      if (!p_ring_in.cmd_w || eop) {
                              r_ring_cmd_fsm = CMD_IDLE;
                      }

                }

		break;

                case BROADCAST_0:

#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name  
              << " -- ring_cmd_fsm -- BROADCAST_0 "
              << " -- ringin cmd rok : " << p_ring_in.cmd_w
              << " -- ringin cmd wok : " << p_ring_in.cmd_r 
              << " -- ringin data : " << std::hex << p_ring_in.cmd_data
              << " -- fifo cmd wok : " << m_cmd_fifo.wok()
              << std::endl;
#endif
                	if ( m_cmd_fifo.wok() )
                        { 
				cmd_fifo_data = p_ring_in.cmd_data;
				r_ring_cmd_fsm = BROADCAST_1;

                        } else {
				r_ring_cmd_fsm = BROADCAST_0;
                        }

		break;

                case BROADCAST_1:
                {
#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
              << " -- ring_cmd_fsm -- BROADCAST_1 "
              << " -- ringin cmd rok : " << p_ring_in.cmd_w
              << " -- ringin cmd wok : " << p_ring_in.cmd_r 
              << " -- ringin data : " << std::hex << p_ring_in.cmd_data
              << " -- fifo cmd wok : " << m_cmd_fifo.wok()
              << std::endl;
#endif

                        bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);

                	if ( p_ring_in.cmd_w && m_cmd_fifo.wok() && eop )
                        { 
				cmd_fifo_data = p_ring_in.cmd_data;
				cmd_fifo_put  = 1;
				r_ring_cmd_fsm = CMD_IDLE;

                        }
                        else {        		 
                		r_ring_cmd_fsm = BROADCAST_1;
                        }
                        
                } 
 		break;

		case LOCAL:   
                {
                        bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);
#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
              << " -- ring_cmd_fsm -- LOCAL "
              << " -- in cmd rok : " << p_ring_in.cmd_w
              << " -- in cmd wok : " << p_ring_in.cmd_r 
              << " -- in data : " << std::hex << p_ring_in.cmd_data
              << " -- fifo wok : " << m_cmd_fifo.wok()	
	      << " -- eop : " << eop
              << std::endl;
#endif

                 	if ( p_ring_in.cmd_w && m_cmd_fifo.wok() && eop )
                        { 

				cmd_fifo_put  = true;
				cmd_fifo_data = p_ring_in.cmd_data;
			     	r_ring_cmd_fsm = CMD_IDLE;
			     		
                        }
                        
                 	if ( !p_ring_in.cmd_w || !m_cmd_fifo.wok() || !eop )
                        { 

				cmd_fifo_put  = p_ring_in.cmd_w && m_cmd_fifo.wok();
				cmd_fifo_data = p_ring_in.cmd_data;
			     	r_ring_cmd_fsm = LOCAL;
			     		
                        }                        
                } 
		break;

		case RING:   
                { 
 
			bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);
#ifdef HT_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
              << " -- ring_cmd_fsm -- RING "
              << " -- in cmd rok : " << p_ring_in.cmd_w
              << " -- in data : " << std::hex << p_ring_in.cmd_data
              << " -- in wok : " << p_ring_in.cmd_r
	      << " -- eop : " << eop
              << std::endl;
#endif 

			if ( p_ring_in.cmd_w && eop ) {        
        			r_ring_cmd_fsm = CMD_IDLE;
                        }
                        else {
 				r_ring_cmd_fsm = RING;
                        }
                }
		break;
	} // end switch cmd fsm

        m_current_cycle++;

    ////////////////////////
    //  fifos update      //
   ////////////////////////

// local cmd fifo update
	if ( cmd_fifo_put && cmd_fifo_get ) m_cmd_fifo.put_and_get(cmd_fifo_data);
	else if (  cmd_fifo_put && !cmd_fifo_get ) m_cmd_fifo.simple_put(cmd_fifo_data);
	else if ( !cmd_fifo_put && cmd_fifo_get ) m_cmd_fifo.simple_get();
// local rsp fifo update
	if (  rsp_fifo_put &&  rsp_fifo_get ) m_rsp_fifo.put_and_get(rsp_fifo_data);
	else if (  rsp_fifo_put && !rsp_fifo_get ) m_rsp_fifo.simple_put(rsp_fifo_data);
	else if ( !rsp_fifo_put &&  rsp_fifo_get ) m_rsp_fifo.simple_get();
 
}  // end Transition()
  
///////////////////////////////////////////////////////////////////
void genMoore(cmd_out_t &p_gate_cmd_out, rsp_in_t &p_gate_rsp_in)
///////////////////////////////////////////////////////////////////
{
	p_gate_cmd_out.write = m_cmd_fifo.rok();
	p_gate_cmd_out.data  = (sc_uint<ring_cmd_data_size>) m_cmd_fifo.read();

	p_gate_rsp_in.read = m_rsp_fifo.wok();

} // end genMoore

/////////////////////////////////////////////////////////////////////////////
void update_ring_signals(ring_signal_t p_ring_in, ring_signal_t &p_ring_out)
////////////////////////////////////////////////////////////////////////////
{

	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE:
			p_ring_out.rsp_grant = p_ring_in.rsp_grant && !m_rsp_fifo.rok();

	        	p_ring_out.rsp_w    = p_ring_in.rsp_w;
		        p_ring_out.rsp_data = p_ring_in.rsp_data;

                	p_ring_out.rsp_r = p_ring_in.rsp_r;
		break;

		case DEFAULT:
			p_ring_out.rsp_grant = !( m_rsp_fifo.rok()); 

        		p_ring_out.rsp_w    =  m_rsp_fifo.rok();
	        	p_ring_out.rsp_data =  m_rsp_fifo.read(); 

                	p_ring_out.rsp_r = 1;
		break;

		case KEEP:  
			int rsp_fifo_eop = (int) ((m_rsp_fifo.read() >> 32) & 0x1);
			p_ring_out.rsp_grant = m_rsp_fifo.rok() && p_ring_in.rsp_r && (rsp_fifo_eop == 1);

        		p_ring_out.rsp_w    =  m_rsp_fifo.rok();
	        	p_ring_out.rsp_data =  m_rsp_fifo.read();

                	p_ring_out.rsp_r = 1;

		break; 

	} // end switch

	p_ring_out.cmd_w    = p_ring_in.cmd_w;
	p_ring_out.cmd_data = p_ring_in.cmd_data;

	p_ring_out.cmd_grant = p_ring_in.cmd_grant;

	switch( r_ring_cmd_fsm ) 
	{
		case CMD_IDLE:
		{

			vci_addr_t rtgtid = (vci_addr_t) ((p_ring_in.cmd_data >> (ring_cmd_data_size-vci_param::N+1)) << 2);
                        int cluster =  (int) ((sc_dt::sc_uint<vci_param::S-4>) (p_ring_in.cmd_data >> 9));
                        bool brdcst = ((p_ring_in.cmd_data & 0x1) == 0X1) && (IntTab(cluster) == m_ringid);
                        bool loc = !((p_ring_in.cmd_data & 0x1) == 0x1) && !m_lt[rtgtid] && !m_local;
                        bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);

                        if(p_ring_in.cmd_w && !eop && brdcst && !m_cmd_fifo.wok()) {
                                p_ring_out.cmd_r = m_cmd_fifo.wok() && p_ring_in.cmd_r;
                        }  
                        if(p_ring_in.cmd_w && !eop && brdcst && m_cmd_fifo.wok()) {
                                p_ring_out.cmd_r = m_cmd_fifo.wok() && p_ring_in.cmd_r;

                        }  
                        if (p_ring_in.cmd_w && !eop && !brdcst && loc) {
                                p_ring_out.cmd_r =  m_cmd_fifo.wok();

                        } 
                        if (p_ring_in.cmd_w && !eop && !brdcst && !loc) {
                                p_ring_out.cmd_r =  p_ring_in.cmd_r; 
                        }  

                        if (!p_ring_in.cmd_w || eop) {
                                p_ring_out.cmd_r =  p_ring_in.cmd_r; 
                        }
		}
		break;
               case BROADCAST_0:
                        p_ring_out.cmd_r =  m_cmd_fifo.wok() && p_ring_in.cmd_r; 
                break;

                case BROADCAST_1:
                        p_ring_out.cmd_r =  m_cmd_fifo.wok() && p_ring_in.cmd_r; 
                break;

		case LOCAL:

                        p_ring_out.cmd_r =  m_cmd_fifo.wok(); 	
		break;

		case RING:

			p_ring_out.cmd_r = p_ring_in.cmd_r;
		break;

	} // end switch

} // end update_ring_signals 
  
};

}} // end namespace

#endif // RING_DSPIN_HALF_GATEWAY_TARGET_FAST_H
