 /* SOCLIB_LGPL_HEADER_BEGIN
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
 * Copyright: UPMC - LIP6
 */
#ifndef RING_DSPIN_HALF_GATEWAY_INITIATOR_FAST_H
#define RING_DSPIN_HALF_GATEWAY_INITIATOR_FAST_H

#include <systemc>
#include "caba_base_module.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "ring_signals_2.h"
#include "dspin_interface.h"

#define HI_DEBUG
//#define HI_DEBUG_FSM

namespace soclib { namespace caba {

using namespace sc_core;

#ifdef HI_DEBUG_FSM
namespace {

        const char *ring_cmd_fsm_state_str_hi[] = {
                "CMD_IDLE",
                "DEFAULT",
                "KEEP",
        };
        const char *ring_rsp_fsm_state_str_hi[] = {
                "RSP_IDLE",
                "LOCAL",
                "RING",
        };
}
#endif

template<typename vci_param, int ring_cmd_data_size, int ring_rsp_data_size>
class RingDspinHalfGatewayInitiatorFast
{

typedef RingSignals2 ring_signal_t;
//typedef soclib::caba::GateInitiator2<ring_cmd_data_size, ring_rsp_data_size> gate_initiator_t;

typedef soclib::caba::DspinInput<ring_cmd_data_size>   cmd_in_t;
typedef soclib::caba::DspinOutput<ring_rsp_data_size>  rsp_out_t;

private:
        
        enum ring_rsp_fsm_state_e {
            	RSP_IDLE,    // waiting for first flit of a response packet
            	LOCAL,      // next flit of a local rsp packet
            	RING,  	    // next flit of a ring rsp packet
            };
        
        // cmd token allocation fsm
        enum ring_cmd_fsm_state_e {
            	CMD_IDLE,	    
             	DEFAULT,  	
            	KEEP,          	    
            };
        
        // structural parameters
 	std::string         m_name;
        bool                m_alloc_init;
        uint32_t            m_current_cycle;
        uint32_t            max_cycle;      

        
        // internal fifos 
        GenericFifo<uint64_t > m_cmd_fifo;     // fifo for the local command packet
        GenericFifo<uint64_t > m_rsp_fifo;     // fifo for the local response packet
        
        // routing table
        soclib::common::AddressDecodingTable<uint32_t, bool> m_lt;
        bool                m_local;

        // internal registers
        sc_signal<int>	    r_ring_cmd_fsm;    // ring command packet FSM (distributed)
        sc_signal<int>	    r_ring_rsp_fsm;    // ring response packet FSM

        uint32_t trace_catch()
        {
                //uint32_t time_stamp=0;
                char *ctime_stamp= getenv("FROM_CYCLE");
                
                //if (ctime_stamp) time_stamp=atoi(ctime_stamp); 	
                return ctime_stamp ? atoi(ctime_stamp) : 99999999;
        
        }

public :

RingDspinHalfGatewayInitiatorFast(
	const char     *name,
        bool            alloc_init,
        const int       &wrapper_fifo_depth,
        const soclib::common::MappingTable &mt,
        const soclib::common::IntTab &ringid,
        bool local)
      : m_name(name),
        m_alloc_init(alloc_init),
        m_cmd_fifo("m_cmd_fifo", wrapper_fifo_depth),
        m_rsp_fifo("m_rsp_fifo", wrapper_fifo_depth),
        m_lt(mt.getIdLocalityTable(ringid)),
        m_local(local),
        r_ring_cmd_fsm("r_ring_cmd_fsm"),
        r_ring_rsp_fsm("r_ring_rsp_fsm")
 { } //  end constructor

void reset()
{
	if(m_alloc_init)
		r_ring_cmd_fsm = DEFAULT;
	else
		r_ring_cmd_fsm = CMD_IDLE;

	r_ring_rsp_fsm = RSP_IDLE;
	m_cmd_fifo.init();
	m_rsp_fifo.init();
        m_current_cycle  = 0;
        max_cycle = trace_catch();
}

//void transition(const gate_initiator_t &p_gate_initiator, const ring_signal_t p_ring_in) 
void transition(const cmd_in_t &p_gate_cmd_in, const rsp_out_t &p_gate_rsp_out, const ring_signal_t p_ring_in)      
{

	bool      cmd_fifo_get = false;
	bool      cmd_fifo_put = false;
	uint64_t  cmd_fifo_data = 0;

//	bool      rsp_fifo_get = false;
	bool      rsp_fifo_put = false;
	uint64_t  rsp_fifo_data = 0;

#ifdef HI_DEBUG_FSM
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
                                 << " - ring cmd  = " << ring_cmd_fsm_state_str_hi[r_ring_cmd_fsm] 
                                 << " - ring rsp  = " << ring_rsp_fsm_state_str_hi[r_ring_rsp_fsm] 
                                 << std::endl;
#endif
//////////// VCI CMD FSM /////////////////////////

	if (p_gate_cmd_in.write.read()) {
		cmd_fifo_data = (uint64_t) p_gate_cmd_in.data.read();
		cmd_fifo_put =  m_cmd_fifo.wok();
	}

	bool rsp_fifo_get = p_gate_rsp_out.read.read();

//////////// RING CMD FSM /////////////////////////
	switch( r_ring_cmd_fsm ) 
	{
		case CMD_IDLE:    
#ifdef HI_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
          << " -- r_ring_cmd_fsm : CMD_IDLE "
          << " -- fifo ROK : " << m_cmd_fifo.rok()
          << " -- in grant : " << p_ring_in.cmd_grant
          << " -- fifo _data : " << std::hex << m_cmd_fifo.read()
          << std::endl;
#endif
    
			if ( p_ring_in.cmd_grant && m_cmd_fifo.rok() )  
                        {
// debug above is here
                		r_ring_cmd_fsm = KEEP; 
                        }
		break;

		case DEFAULT: 
#ifdef HI_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
          << " -- r_ring_cmd_fsm : DEFAULT "
          << " -- fifo ROK : " << m_cmd_fifo.rok()
          << " -- in grant : " << p_ring_in.cmd_grant
          << " -- fifo _data : " << std::hex << m_cmd_fifo.read()
          << std::endl;
#endif
        
			if ( m_cmd_fifo.rok() ) 
			{
// debug above is here
				cmd_fifo_get = p_ring_in.cmd_r;  
				r_ring_cmd_fsm = KEEP;             
			}   
			else if ( !p_ring_in.cmd_grant )
				r_ring_cmd_fsm = CMD_IDLE; 
		break;

		case KEEP:   
 #ifdef HI_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name 
          << " -- r_ring_cmd_fsm : KEEP "
          << " -- fifo_rok : " << m_cmd_fifo.rok()
          << " -- in grant : " << p_ring_in.cmd_grant
          << " -- ring_in_wok : " << p_ring_in.cmd_r
          << " -- fifo_out_data : " << std::hex << m_cmd_fifo.read()
          << std::endl;
#endif
                         
			if(m_cmd_fifo.rok() && p_ring_in.cmd_r ) 
			{
// debug above is here
				cmd_fifo_get = true;  
				if (((int) (m_cmd_fifo.read() >> (ring_cmd_data_size - 1) ) & 0x1) == 1)  // 39
				{  
					if ( p_ring_in.cmd_grant )
						r_ring_cmd_fsm = DEFAULT;  
					else   
						r_ring_cmd_fsm = CMD_IDLE; 
				}        
			}      
		break;

	} // end switch ring cmd fsm
 
/////////// RING RSP FSM ////////////////////////
    
	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE:  
		{
			int  rsrcid  = (int)  (p_ring_in.rsp_data >> ring_rsp_data_size-vci_param::S-1);
			bool islocal = (m_lt[rsrcid] && m_local) || (!m_lt[rsrcid] && !m_local);
			bool reop     = ((p_ring_in.rsp_data >> (ring_rsp_data_size - 1)) & 0x1) == 1;

#ifdef HI_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name  
              << " -- ring_rsp_fsm -- RSP_IDLE "
              << " -- islocal : " << islocal
              << " -- eop : " << reop
              << " -- rsrcid : " << std::hex << rsrcid
              << " -- in rok : " << p_ring_in.rsp_w
              << " -- in wok : " << p_ring_in.rsp_r
              << " -- fifo wok : " <<  m_rsp_fifo.wok()         
              << std::endl;
#endif
			if (p_ring_in.rsp_w  &&  !reop && islocal) 
			{   
				r_ring_rsp_fsm = LOCAL;
				rsp_fifo_put  = m_rsp_fifo.wok();
				rsp_fifo_data = p_ring_in.rsp_data;
			}
			if (p_ring_in.rsp_w  &&  !reop && !islocal) 
			{
				r_ring_rsp_fsm = RING;  
			}
			if (!p_ring_in.rsp_w  || reop ) 
			{			
				r_ring_rsp_fsm = RSP_IDLE;
			} 
		}
		break;

		case LOCAL:
		{

			bool reop     = ((p_ring_in.rsp_data >> (ring_rsp_data_size - 1)) & 0x1) == 1;
#ifdef HI_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name  
              << " -- ring_rsp_fsm -- LOCAL "
              << " -- in rok : " << p_ring_in.rsp_w
              << " -- fifo wok : " <<  m_rsp_fifo.wok()   
              << " -- in data : " << std::hex << p_ring_in.rsp_data
	      << " -- eop : " << reop
              << std::endl;
#endif


			if (p_ring_in.rsp_w && m_rsp_fifo.wok() && reop)         
			{

				rsp_fifo_put  = true;
				rsp_fifo_data = p_ring_in.rsp_data;
				r_ring_rsp_fsm = RSP_IDLE;             
			}
			if (!p_ring_in.rsp_w || !m_rsp_fifo.wok() || !reop)         
			{

				rsp_fifo_put  = p_ring_in.rsp_w && m_rsp_fifo.wok();
				rsp_fifo_data = p_ring_in.rsp_data;
				r_ring_rsp_fsm = LOCAL;             
			}
		} 
		break;

		case RING:     
		{
			bool reop     = ((p_ring_in.rsp_data >> (ring_rsp_data_size - 1)) & 0x1) == 1;

#ifdef HI_DEBUG
if( m_current_cycle >= max_cycle )
   std::cout << std::dec << m_current_cycle << " - " << m_name  
              << " -- ring_rsp_fsm -- RING "
              << " -- in rok : " << p_ring_in.rsp_w
              << " -- in wok : " <<  p_ring_in.rsp_r   
              << " -- in data : " << std::hex << p_ring_in.rsp_data
	      << " -- eop : " << reop	
              << std::endl;
#endif


			if (p_ring_in.rsp_w && reop)
			{
				r_ring_rsp_fsm = RSP_IDLE; 
			}
			else
			{
				r_ring_rsp_fsm = RING;
			}
		}
		break;

	} // end switch rsp fsm

         m_current_cycle++;     

    ////////////////////////
    //  fifos update      //
   ////////////////////////

// local cmd fifo update
	if (  cmd_fifo_put &&  cmd_fifo_get ) m_cmd_fifo.put_and_get(cmd_fifo_data);
	else if (  cmd_fifo_put && !cmd_fifo_get ) m_cmd_fifo.simple_put(cmd_fifo_data);
	else if ( !cmd_fifo_put &&  cmd_fifo_get ) m_cmd_fifo.simple_get();
	
// local rsp fifo update
	if (  rsp_fifo_put &&  rsp_fifo_get ) m_rsp_fifo.put_and_get(rsp_fifo_data);
	else if (  rsp_fifo_put && !rsp_fifo_get ) m_rsp_fifo.simple_put(rsp_fifo_data);
	else if ( !rsp_fifo_put &&  rsp_fifo_get ) m_rsp_fifo.simple_get();
     
}  // end Transition()

///////////////////////////////////////////////////////////////////
void genMoore(cmd_in_t &p_gate_cmd_in, rsp_out_t &p_gate_rsp_out)
///////////////////////////////////////////////////////////////////
{
	p_gate_rsp_out.write = m_rsp_fifo.rok();
	p_gate_rsp_out.data  = (sc_uint<ring_rsp_data_size>) m_rsp_fifo.read();

	p_gate_cmd_in.read= m_cmd_fifo.wok();

} // end genMoore

///////////////////////////////////////////////////////////////////
void update_ring_signals(ring_signal_t p_ring_in, ring_signal_t &p_ring_out)
///////////////////////////////////////////////////////////////////
{    
	switch( r_ring_cmd_fsm ) 
	{
		case CMD_IDLE:
			p_ring_out.cmd_grant = p_ring_in.cmd_grant && !m_cmd_fifo.rok();

                     	p_ring_out.cmd_r     = p_ring_in.cmd_r;

        		p_ring_out.cmd_w     = p_ring_in.cmd_w;
	        	p_ring_out.cmd_data  = p_ring_in.cmd_data;
		break;
	
		case DEFAULT:        
			p_ring_out.cmd_grant = !( m_cmd_fifo.rok());  

                   	p_ring_out.cmd_r    = 1;

	        	p_ring_out.cmd_w    =  m_cmd_fifo.rok();
		        p_ring_out.cmd_data =  m_cmd_fifo.read();
		break;
	
		case KEEP:  
			int cmd_fifo_eop = (int) ((m_cmd_fifo.read() >> (ring_cmd_data_size - 1)) & 0x1) ; //39
			p_ring_out.cmd_grant = m_cmd_fifo.rok() && p_ring_in.cmd_r && (cmd_fifo_eop == 1);

                   	p_ring_out.cmd_r    = 1;	

        		p_ring_out.cmd_w    =  m_cmd_fifo.rok();
	        	p_ring_out.cmd_data =  m_cmd_fifo.read();
		break;
	
	} // end switch

	p_ring_out.rsp_grant = p_ring_in.rsp_grant;

	p_ring_out.rsp_w    = p_ring_in.rsp_w;
	p_ring_out.rsp_data = p_ring_in.rsp_data;

	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE:	
		{
			int  rsrcid  = (int)  (p_ring_in.rsp_data >> ring_rsp_data_size-vci_param::S-1);
			bool islocal = (m_lt[rsrcid] && m_local) || (!m_lt[rsrcid] && !m_local);
			bool reop    = ((p_ring_in.rsp_data >> (ring_rsp_data_size - 1)) & 0x1) == 1;

			if(p_ring_in.rsp_w && !reop && islocal) {
				p_ring_out.rsp_r = m_rsp_fifo.wok();
			}
			if(p_ring_in.rsp_w && !reop && !islocal) {
				p_ring_out.rsp_r = p_ring_in.rsp_r;
			}
			if(!p_ring_in.rsp_w || reop)  {
				p_ring_out.rsp_r = p_ring_in.rsp_r;
			}
 
		}
		break;
	
		case LOCAL:
			p_ring_out.rsp_r = m_rsp_fifo.wok();
		break;
	
		case RING:
			p_ring_out.rsp_r = p_ring_in.rsp_r;
		break;    
	} // end switch


} // end update_ring_signals

};

}} // end namespace

#endif // RING_DSPIN_HALF_GATEWAY_INITIATOR_FAST_H

