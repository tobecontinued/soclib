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
#ifndef SOCLIB_CABA_VCI_RING_TARGET_H
#define SOCLIB_CABA_VCI_RING_TARGET_H

#include "vci_initiator.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "ring_signals_2.h"

#define T_DEBUG
#define TR_DEBUG
//#define T_DEBUG_FSM

namespace soclib { namespace caba {
#ifdef T_DEBUG_FSM
namespace {
        const char *vci_cmd_fsm_state_str_t[] = {
                "CMD_FIRST_HEADER",
                "CMD_SECOND_HEADER",
                "WDATA",
        };
        const char *vci_rsp_fsm_state_str_t[] = {
                "RSP_HEADER",
                "RSP_DATA",
        };
        const char *ring_rsp_fsm_state_str_t[] = {
                "RSP_IDLE",
                "DEFAULT",
                "KEEP",
        };
        const char *ring_cmd_fsm_state_str_t[] = {
                "CMD_IDLE",
                "BROADCAST_0",
                "BROADCAST_1",
                "LOCAL",
                "RING",
        };
}
#endif

template<typename vci_param, int ring_cmd_data_size, int ring_rsp_data_size>
class VciRingTarget
{

typedef typename vci_param::fast_addr_t vci_addr_t;
typedef RingSignals2 ring_signal_t; 
typedef soclib::caba::VciInitiator<vci_param> vci_initiator_t;

private:
        enum vci_cmd_fsm_state_e {
        	CMD_FIRST_HEADER,     // first flit for a ring cmd packet (read or write)
                CMD_SECOND_HEADER,   //  second flit for a ring cmd packet
        	WDATA,               //  data flit for a ring cmd write packet
            };
        
        enum vci_rsp_fsm_state_e {
        	RSP_HEADER,     // first flit for a ring rsp packet (read or write)
                DATA,          // next flit for a ring rsp packet
            };
        
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
        int           m_tgtid;
        uint32_t      m_shift;
       
        // internal registers
        sc_core::sc_signal<int>	        r_ring_cmd_fsm;	    // ring command packet FSM 
        sc_core::sc_signal<int>		r_ring_rsp_fsm;	    // ring response packet FSM
        sc_core::sc_signal<int>		r_vci_cmd_fsm;	    // vci command packet FSM
        sc_core::sc_signal<int>		r_vci_rsp_fsm;	    // vci response packet FSM
        
        sc_core::sc_signal<sc_dt::sc_uint<vci_param::S> >      r_srcid;
        sc_core::sc_signal<sc_dt::sc_uint<2> >                 r_cmd;
        sc_core::sc_signal<sc_dt::sc_uint<vci_param::T> >      r_trdid;
        sc_core::sc_signal<sc_dt::sc_uint<vci_param::P> >      r_pktid;
        sc_core::sc_signal<sc_dt::sc_uint<vci_param::K> >      r_plen;
        sc_core::sc_signal<sc_dt::sc_uint<1> >                 r_contig;
        sc_core::sc_signal<sc_dt::sc_uint<1> >                 r_const;
        sc_core::sc_signal<sc_dt::sc_uint<vci_param::N> >      r_addr;
            
        // internal fifos 
        GenericFifo<uint64_t > m_cmd_fifo;     // fifo for the local command paquet
        GenericFifo<uint64_t > m_rsp_fifo;     // fifo for the local response paquet
        
        // routing table 
        soclib::common::AddressDecodingTable<vci_addr_t, int> m_rt;
        // locality table
        soclib::common::AddressDecodingTable<vci_addr_t, bool> m_lt;

bool trace(int sc_time_stamp)
{
int time_stamp=0;
char *ctime_stamp= getenv("FROM_CYCLE");

if (ctime_stamp) time_stamp=atoi(ctime_stamp); 	

return sc_time_stamp >= time_stamp;

}

public :

VciRingTarget(
	const char     *name,
        bool            alloc_target,
        const int       &wrapper_fifo_depth,
        const soclib::common::MappingTable &mt,
        const soclib::common::IntTab &ringid,
        const int &tgtid)
     :  m_name(name),
        m_alloc_target(alloc_target),
        m_cmd_fifo("m_cmd_fifo", wrapper_fifo_depth),
        m_rsp_fifo("m_rsp_fifo", wrapper_fifo_depth),
        m_rt(mt.getRoutingTable<typename vci_param::fast_addr_t>(ringid)),
        m_lt(mt.getLocalityTable<typename vci_param::fast_addr_t>(ringid)),
        m_tgtid(tgtid),
        m_shift(ring_cmd_data_size-vci_param::N+1),
        r_ring_cmd_fsm("r_ring_cmd_fsm"),
	r_ring_rsp_fsm("r_ring_rsp_fsm"),
	r_vci_cmd_fsm("r_vci_cmd_fsm"),
	r_vci_rsp_fsm("r_vci_rsp_fsm"),
        r_srcid("r_srcid"),
        r_cmd("r_cmd"),
        r_trdid("r_trdid"),
        r_pktid("r_pktid"),
        r_plen("r_plen"),
        r_contig("r_contig"),
        r_const("r_const"),
        r_addr("r_addr")

{} //  end constructor

void reset()
{
        if(m_alloc_target)
        	r_ring_rsp_fsm = DEFAULT;
        else
        	r_ring_rsp_fsm = RSP_IDLE;
        
        r_vci_cmd_fsm = CMD_FIRST_HEADER;
        r_vci_rsp_fsm = RSP_HEADER;
        r_ring_cmd_fsm = CMD_IDLE;
        m_cmd_fifo.init();
        m_rsp_fifo.init();       
}
void transition(const vci_initiator_t &p_vci, const ring_signal_t p_ring_in)       
{

	bool      cmd_fifo_get = false;
	bool      cmd_fifo_put = false;
	uint64_t  cmd_fifo_data = 0;
	
	bool      rsp_fifo_get = false;
	bool      rsp_fifo_put = false;
	uint64_t  rsp_fifo_data = 0;

#ifdef T_DEBUG_FSM
if( trace(sc_time_stamp()))
    std::cout << sc_time_stamp() << " - " << m_name
                                 << " - vci cmd = " << vci_cmd_fsm_state_str_t[r_vci_cmd_fsm]
                                 << " - vci rsp = " << vci_rsp_fsm_state_str_t[r_vci_rsp_fsm]
                                 << " - ring cmd = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm] 
                                 << " - ring rsp = " << ring_rsp_fsm_state_str_t[r_ring_rsp_fsm]
                                 << std::endl;
#endif
	
//////////// VCI CMD FSM /////////////////////////
	switch ( r_vci_cmd_fsm ) 
	{

		case CMD_FIRST_HEADER:
                        if (m_cmd_fifo.rok() == true)
                        {
#ifdef T_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
              << " -- r_vci_cmd_fsm -- CMD_FIRST_HEADER "
              << " -- fifo_rok : " << m_cmd_fifo.rok()
              << " -- fifo_data : " << std::hex << m_cmd_fifo.read()
              << std::endl;
#endif

                                cmd_fifo_get = true; 
				
                                if (m_cmd_fifo.read() & 0x1 == 0x1) // broadcast
                                {
                                        r_addr   = (sc_dt::sc_uint<vci_param::N>) 0x3;     
                                        r_srcid  = (sc_dt::sc_uint<vci_param::S>) (m_cmd_fifo.read() >> 5); 
 					r_cmd    = (sc_dt::sc_uint<2>)  0x2; 
 					r_contig = (sc_dt::sc_uint<1>)  0x1; 
 					r_const  = (sc_dt::sc_uint<1>)  0x0; 
 					r_plen   = (sc_dt::sc_uint<vci_param::K>) 0x04; 
 					r_pktid  = (sc_dt::sc_uint<vci_param::P>) 0x0; 
 					r_trdid  = (sc_dt::sc_uint<vci_param::T>) (m_cmd_fifo.read() >> 1); 

                                        r_vci_cmd_fsm = WDATA; 
                                }
                                else
                                {
                                        r_addr = (sc_dt::sc_uint<vci_param::N>) ((m_cmd_fifo.read() >> m_shift )<< 2);
                                        r_vci_cmd_fsm = CMD_SECOND_HEADER; 
                                }

         
			}  // end if rok
		break;

		case CMD_SECOND_HEADER:        
			if ( m_cmd_fifo.rok() ) 
			{

#ifdef T_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
              << " -- r_vci_cmd_fsm -- CMD_SECOND_HEADER "
              << " -- r_addr : " << std::hex << r_addr 
              << " -- fifo_rok : " << m_cmd_fifo.rok()
              << " -- fifo_data : " << m_cmd_fifo.read()
              << " -- vci cmdack : " << p_vci.cmdack.read()
              << std::endl;
#endif
				if(((int) (m_cmd_fifo.read() >> (ring_cmd_data_size - 1) ) & 0x1) == 1)  // read command
				{

					if (p_vci.cmdack.read())
					{

						cmd_fifo_get = true;
						r_vci_cmd_fsm = CMD_FIRST_HEADER;
					} 
				}
				else  // write command
				{

 					cmd_fifo_get =  true;
 					r_srcid  = (sc_dt::sc_uint<vci_param::S>)  (m_cmd_fifo.read() >> (ring_cmd_data_size-vci_param::S-1)) ; 
 					r_cmd    = (sc_dt::sc_uint<2>)  ((m_cmd_fifo.read() >> 23) & 0x3); 
 					r_contig = (sc_dt::sc_uint<1>)  ((m_cmd_fifo.read() >> 22) & 0x1); 
 					r_const =  (sc_dt::sc_uint<1>)  ((m_cmd_fifo.read() >> 21) & 0x1); 
 					r_plen  =  (sc_dt::sc_uint<vci_param::K>) ((m_cmd_fifo.read() >> 13) & 0xFF); 
 					r_pktid  = (sc_dt::sc_uint<vci_param::P>) ((m_cmd_fifo.read() >> 9) & 0xF); 
 					r_trdid  = (sc_dt::sc_uint<vci_param::T>) ((m_cmd_fifo.read() >> 5) & 0xF); 
 					r_vci_cmd_fsm = WDATA;
				}                                          
			} 
		break;

		case WDATA:
#ifdef T_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
              << " -- r_vci_cmd_fsm -- WDATA "
              << " -- vci_cmdack : " << p_vci.cmdack.read()
              << " -- fifo_rok : " << m_cmd_fifo.rok()
              << " -- fifo_data : " << std::hex << m_cmd_fifo.read()
              << " -- r_plen : " << r_plen.read()
              << std::endl;
#endif
			if ( p_vci.cmdack.read() && m_cmd_fifo.rok() ) 
			{

				cmd_fifo_get = true; 
				sc_dt::sc_uint<1> contig = r_contig;
				if(contig == 0x1)    
					r_addr = r_addr.read() + vci_param::B ;                        
				if(( (m_cmd_fifo.read() >> (ring_cmd_data_size - 1) ) & 0x1) == 1)
					r_vci_cmd_fsm = CMD_FIRST_HEADER;   
				else 
					r_vci_cmd_fsm = WDATA;                                   
			} // end if cmdack
		break;
        
	} // end switch r_vci_cmd_fsm

/////////// VCI RSP FSM /////////////////////////
	switch ( r_vci_rsp_fsm ) 
	{
		case RSP_HEADER:
#ifdef T_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
              << " -- r_vci_rsp_fsm -- RSP_HEADER "
              << " -- fifo_rsp_wok : " << m_rsp_fifo.wok()
              << " -- vci_rspval : " << p_vci.rspval.read()
              << " -- rsrcid : " << std::hex << p_vci.rsrcid.read()
              << " -- rerror : " << p_vci.rerror.read()
              << std::endl;
#endif
			if((p_vci.rspval.read() == true) && (m_rsp_fifo.wok()))
			{

				rsp_fifo_data = (((uint64_t) p_vci.rsrcid.read()) << (ring_rsp_data_size-vci_param::S-1)) |
                                                (((uint64_t) p_vci.rerror.read() & 0x3) << 16) | 
                                                (((uint64_t) p_vci.rpktid.read() & 0xF) << 12) | 
                                                (((uint64_t) p_vci.rtrdid.read() & 0xF) << 8); 
				rsp_fifo_put = true; 
				r_vci_rsp_fsm = DATA;
			}
		break;

		case DATA:
 #ifdef T_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
              << " -- r_vci_rsp_fsm -- RSP_DATA "
              << " -- fifo_rsp_wok : " << m_rsp_fifo.wok()
              << " -- vci_rspval : " << p_vci.rspval.read()
              << " -- rdata : " << std::hex << p_vci.rdata.read()
              << " -- reop : " << p_vci.reop.read()
              << std::endl;
#endif             
			if((p_vci.rspval.read() == true) && (m_rsp_fifo.wok())) 
			{


				rsp_fifo_put = true;
				rsp_fifo_data = (uint64_t) p_vci.rdata.read();  
         
				if (p_vci.reop.read()) 
				{ 
					rsp_fifo_data = rsp_fifo_data |  (((uint64_t) 0x1) << (ring_rsp_data_size-1)) ;
					r_vci_rsp_fsm = RSP_HEADER;
				}           		    
			}  
		break;

	} // end switch r_vci_rsp_fsm
   
//////////// RING RSP FSM (distributed) /////////////////////////
        
	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE:   
#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
std::cout << sc_time_stamp() << " -- " << m_name <<  " -- ring_rsp_fsm : RSP_IDLE"
          << " -- fifo rok : " <<  m_rsp_fifo.rok()
          << " -- in rok : " <<  p_ring_in.rsp_w
          << " -- in wok : " <<  p_ring_in.rsp_r
          << " -- in rsp grant : " << p_ring_in.rsp_grant
          << " -- fifo data  : " <<  std::hex << m_rsp_fifo.read()
          << std::endl;
#endif    
			if ( p_ring_in.rsp_grant && m_rsp_fifo.rok() ) 

				r_ring_rsp_fsm = KEEP;           
                
		break;

		case DEFAULT:  
			
                        if ( m_rsp_fifo.rok())  
			{
#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
std::cout << sc_time_stamp() << " -- " << m_name <<  " -- ring_rsp_fsm : DEFAULT " 
          << " -- fifo data : " << std::hex << m_rsp_fifo.read()
          << std::endl;
#endif
				rsp_fifo_get = p_ring_in.rsp_r; //true;
				r_ring_rsp_fsm = KEEP;
			}   
			else if ( !p_ring_in.rsp_grant )
				r_ring_rsp_fsm = RSP_IDLE;  
		break;

		case KEEP:   
             
			if(m_rsp_fifo.rok() && p_ring_in.rsp_r) 
			{
#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
std::cout << sc_time_stamp() << " -- " << m_name <<  " -- ring_rsp_fsm : KEEP "
          << " -- fifo rok : " << m_rsp_fifo.rok()
          << " -- in wok : " << p_ring_in.rsp_r
          << " -- fifo data : " << std::hex << m_rsp_fifo.read()
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
		{ // for variable scope

			vci_addr_t rtgtid = (vci_addr_t) ((p_ring_in.cmd_data >> m_shift) << 2);
			bool islocal = m_lt[rtgtid]  && (m_rt[rtgtid] == (vci_addr_t) m_tgtid); 
                        bool brdcst  = (p_ring_in.cmd_data & 0x1) == 0X1 ;
                        bool eop     = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1); 

#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
              << " - ring_cmd_fsm -- CMD_IDLE "
              << " - in rok : " << p_ring_in.cmd_w
              << " - isloc : " << islocal
              << " - addr : " << std::hex << rtgtid
              << " - brdcst : " << brdcst
              << " - in wok : " << p_ring_in.cmd_r
              << " - fifo wok : " << m_cmd_fifo.wok()
              << " - eop : " << eop  
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
        
                      if (p_ring_in.cmd_w && !eop && !brdcst && islocal) {
                              r_ring_cmd_fsm = LOCAL; 
                              cmd_fifo_put   = m_cmd_fifo.wok();
                              cmd_fifo_data  = p_ring_in.cmd_data;  

                      } 
        
                      if (p_ring_in.cmd_w && !eop && !brdcst && !islocal) {
                              r_ring_cmd_fsm = RING;
                      }  

                      if (!p_ring_in.cmd_w || eop) {
                              r_ring_cmd_fsm = CMD_IDLE;
                      }

                }

		break;

                case BROADCAST_0:

#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
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
#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
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
#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
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

#ifdef TR_DEBUG
if( trace(sc_time_stamp()))
         std::cout << sc_time_stamp() << " -- " << m_name  
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
void genMoore(vci_initiator_t &p_vci)
///////////////////////////////////////////////////////////////////
{
	if( r_vci_rsp_fsm == RSP_HEADER ) 
		p_vci.rspack = false;
	else
		p_vci.rspack = m_rsp_fifo.wok();

	switch ( r_vci_cmd_fsm ) 
	{
		case CMD_FIRST_HEADER:                        			  
		        p_vci.cmdval = false;

		break;

		case CMD_SECOND_HEADER:         
			if(((int) (m_cmd_fifo.read() >> (ring_cmd_data_size - 1) ) & 0x1) == 1) // eop
			{
				p_vci.cmdval  = m_cmd_fifo.rok(); 
				p_vci.address = (sc_dt::sc_uint<vci_param::N>) r_addr.read();
				p_vci.cmd     = (sc_dt::sc_uint<2>)  ((m_cmd_fifo.read() >> 23) & 0x3);
                                p_vci.be      = (sc_dt::sc_uint<4>)  ((m_cmd_fifo.read() >> 1) & 0xF);
				p_vci.wdata   = 0;
				p_vci.pktid   = (sc_dt::sc_uint<vci_param::P>) ((m_cmd_fifo.read() >> 9) & 0xF);
				p_vci.srcid   = (sc_dt::sc_uint<vci_param::S>)  (m_cmd_fifo.read() >> (ring_cmd_data_size-vci_param::S-1));
				p_vci.trdid   = (sc_dt::sc_uint<vci_param::T>) ((m_cmd_fifo.read() >> 5) & 0xF);
				p_vci.plen    =  (sc_dt::sc_uint<vci_param::K>)  ((m_cmd_fifo.read() >> 13) & 0xFF);
				p_vci.eop     = true;         
				sc_dt::sc_uint<1> cons = (sc_dt::sc_uint<1>)  ((m_cmd_fifo.read() >> 21) & 0x1) ; 
				if (cons == 0x1)
					p_vci.cons = true;
				else
					p_vci.cons = false;        
				sc_dt::sc_uint<1> contig = (sc_dt::sc_uint<1>)  ((m_cmd_fifo.read() >> 22) & 0x1);
				if(contig == 0x1) 
					p_vci.contig = true;
				else
					p_vci.contig = false;          	    
			} 
			else 
				p_vci.cmdval = false;         
		break;
    
		case WDATA:
		{   // for variable scope

			p_vci.cmdval = m_cmd_fifo.rok();
			p_vci.address = (sc_dt::sc_uint<vci_param::N>) r_addr.read();
			p_vci.be = (sc_dt::sc_uint<vci_param::B>)((m_cmd_fifo.read()  >> 32) & 0xF);
			p_vci.cmd = r_cmd;
			p_vci.wdata = (sc_dt::sc_uint<32>)(m_cmd_fifo.read()); 
			p_vci.pktid = r_pktid;
			p_vci.srcid = r_srcid;
			p_vci.trdid = r_trdid;
			p_vci.plen  = r_plen;        
			sc_dt::sc_uint<1> cons = r_const;         
			if (cons == 0x1)
				p_vci.cons = true;
			else
				p_vci.cons = false;        
			sc_dt::sc_uint<1> contig = r_contig;
			if(contig == 0x1)                     
				p_vci.contig = true;           
			else
				p_vci.contig = false;
                        if(((int) (m_cmd_fifo.read() >> (ring_cmd_data_size - 1) ) & 0x1) == 1)
				p_vci.eop = true;
			else    
				p_vci.eop = false; 
		}
		break;
            
	} // end switch fsm
} // end genMoore

///////////////////////////////////////////////////////////////////
void update_ring_signals(ring_signal_t p_ring_in, ring_signal_t &p_ring_out)
///////////////////////////////////////////////////////////////////
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
			vci_addr_t rtgtid = (vci_addr_t) ((p_ring_in.cmd_data >> m_shift) << 2);
		        bool islocal = m_lt[rtgtid]  && (m_rt[rtgtid] == (vci_addr_t) m_tgtid); 
                        bool brdcst  = (p_ring_in.cmd_data & 0x1) == 0X1 ;
                        bool eop     = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1); 

                        if(p_ring_in.cmd_w && !eop && brdcst && !m_cmd_fifo.wok()) {
                                p_ring_out.cmd_r = m_cmd_fifo.wok() && p_ring_in.cmd_r;
                        }  
        
                        if(p_ring_in.cmd_w && !eop && brdcst && m_cmd_fifo.wok()) {
                                p_ring_out.cmd_r = m_cmd_fifo.wok() && p_ring_in.cmd_r;

                        }  
        
                        if (p_ring_in.cmd_w && !eop && !brdcst && islocal) {
                                p_ring_out.cmd_r =  m_cmd_fifo.wok();

                        } 
        
                        if (p_ring_in.cmd_w && !eop && !brdcst && !islocal) {
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
#endif // SOCLIB_CABA_VCI_RING_TARGET_H

