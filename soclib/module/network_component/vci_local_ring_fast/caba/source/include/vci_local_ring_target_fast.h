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
 * Authors  : Abdelmalek SI MERABET
 *            Franck     WAJSBURT
 * Date     : Februrary 2011
 * Copyright: UPMC - LIP6
 */
#ifndef SOCLIB_CABA_VCI_ALLOC_RING_TARGET_FAST_H
#define SOCLIB_CABA_VCI_ALLOC_RING_TARGET_FAST_H

#include "vci_initiator.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "ring_signals_fast.h"

//#define T_DEBUG

namespace soclib { namespace caba {

namespace {

const char *ring_rsp_fsm_state_str_t[] = {
                "RSP_IDLE",
                "DEFAULT",
                "SENDING",
                "WAIT_PALLOC_END",
        };

#ifdef T_DEBUG

const char *vci_cmd_fsm_state_str_t[] = {
        "CMD_FIRST_HEADER",
        "CMD_SECOND_HEADER",
        "WDATA",
};
const char *vci_rsp_fsm_state_str_t[] = {
        "RSP_HEADER",
        "RSP_SINGLE_DATA",
        "RSP_MULTI_DATA",
};

const char *ring_cmd_fsm_state_str_t[] = {
        "CMD_IDLE",
        "BROADCAST_0",
        "BROADCAST_1",
        "ALLOC",
        "NALLOC",
        "PALLOC2",
        "PALLOC1",
};
#endif
} // end namespace

template<typename vci_param, int ring_cmd_data_size, int ring_rsp_data_size>
class VciLocalRingTargetFast
{

typedef typename vci_param::fast_addr_t vci_addr_t;
typedef LocalRingSignals ring_signal_t; 
typedef VciInitiator<vci_param> vci_initiator_t;

private:
        enum vci_cmd_fsm_state_e {
        	CMD_FIRST_HEADER,     // first flit for a ring cmd packet (read or write)
                CMD_SECOND_HEADER,   //  second flit for a ring cmd packet
        	WDATA,               //  data flit for a ring cmd write packet
            };
        
        enum vci_rsp_fsm_state_e {
        	RSP_HEADER,     
                RSP_SINGLE_DATA, 
                RSP_MULTI_DATA,      
            };
        
        enum ring_cmd_fsm_state_e {
        	CMD_IDLE,	 // waiting for first flit of a command packet
                BROADCAST_0,
                BROADCAST_1,
        	ALLOC,  	// next flit of a local cmd packet
        	NALLOC,         // next flit of a ring cmd packet
                PALLOC2,        // local target is receiving from init gate (ring is cmd_preempted) while target gate is allocated
                PALLOC1,        // local target is receiving from init gate, target gate has finished receiving (target gate is not allocated)        
        };
        
        // cmd token allocation fsm
        enum ring_rsp_fsm_state_e {
        	RSP_IDLE,	    
        	DEFAULT,  	
        	SENDING,  
	        WAIT_PALLOC_END,            
        };
        
        // structural parameters
	std::string   m_name;
        bool          m_alloc_target;

        sc_core::sc_signal<bool>     r_brdcst_p; // brdcst from init gate (with cmd_preempt) => cmd_palloc = 1
        //sc_core::sc_signal<bool>     r_from_cmd_palloc; // in NALLOC state, when true, this means that previous state was PALLOC2 =>  cmd_palloc = false

        // internal fifos 
        GenericFifo<uint64_t > m_cmd_fifo;     // fifo for the local command paquet
        GenericFifo<uint64_t > m_rsp_fifo;     // fifo for the local response paquet

        // routing table 
        soclib::common::AddressDecodingTable<vci_addr_t, int> m_rt;
        // locality table
        soclib::common::AddressDecodingTable<vci_addr_t, bool> m_lt_addr;
	soclib::common::AddressDecodingTable<uint32_t, bool>   m_lt_src;
        int           m_tgtid;
        uint32_t      m_shift;

#ifdef T_DEBUG
        uint32_t            m_cpt;
	uint32_t            m_cyc; 
#endif

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
            
public :

VciLocalRingTargetFast(
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
        m_lt_addr(mt.getLocalityTable<typename vci_param::fast_addr_t>(ringid)),
        m_lt_src(mt.getIdLocalityTable(ringid)),
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

#ifdef T_DEBUG 
	m_cpt = 0;
	m_cyc = (uint32_t) atoi(getenv("CYCLES"));
#endif

   
}
void transition(const vci_initiator_t &p_vci, const ring_signal_t p_ring_in, bool &tgt_cmd_val, rsp_str &tgt_rsp)       
{

	bool      cmd_fifo_get = false;
	bool      cmd_fifo_put = false;
	uint64_t  cmd_fifo_data = 0;
	
	bool      rsp_fifo_get = false;
	bool      rsp_fifo_put = false;
	uint64_t  rsp_fifo_data = 0;


//////////// VCI CMD FSM /////////////////////////
	switch ( r_vci_cmd_fsm ) 
	{

		case CMD_FIRST_HEADER:
                        if (m_cmd_fifo.rok() == true)
                        {

#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - vci_cmd_fsm = " << vci_cmd_fsm_state_str_t[r_vci_cmd_fsm]
                          << " - fifo_rok : " << m_cmd_fifo.rok()
                          << " - fifo_data : " << std::hex << m_cmd_fifo.read()
                          << " - vci cmdack : " << p_vci.cmdack.read()
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
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - vci_cmd_fsm = " << vci_cmd_fsm_state_str_t[r_vci_cmd_fsm]
                          << " - fifo_rok : " << m_cmd_fifo.rok()
                          << " - fifo_data : " << std::hex << m_cmd_fifo.read()
                          << " - r_addr : " << r_addr
                          << " - vci cmdack : " << p_vci.cmdack.read()
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
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - vci_cmd_fsm = " << vci_cmd_fsm_state_str_t[r_vci_cmd_fsm]
                          << " - fifo_rok : " << m_cmd_fifo.rok()
                          << " - fifo_data : " << std::hex << m_cmd_fifo.read()
                          << " - r_addr : " << r_addr
                          << " - vci cmdack : " << p_vci.cmdack.read()
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
                {

#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - vci_rsp_fsm = " << vci_rsp_fsm_state_str_t[r_vci_rsp_fsm]
                          << " - vci_rspval : " << p_vci.rspval.read()
                          << " - fifo wok : " << m_rsp_fifo.wok() 
                          << " - rsrcid : " << std::hex << p_vci.rsrcid.read()
                          << " - rdata : " << p_vci.rdata.read()
                          << " - reop : " << p_vci.reop.read()
                          << " - rerror : " << p_vci.rerror.read()
                          << std::endl;
#endif

			if(p_vci.rspval.read() && m_rsp_fifo.wok())
			{
                                rsp_fifo_put = true;

				rsp_fifo_data = (((uint64_t) p_vci.rsrcid.read()) << (ring_rsp_data_size-vci_param::S-1)) |
                                                (((uint64_t) p_vci.rerror.read() & 0x1) << 16) | 
                                                (((uint64_t) p_vci.rpktid.read() & 0xF) << 12) | 
                                                (((uint64_t) p_vci.rtrdid.read() & 0xF) << 8);
                               
                                // one flit for write response                                 
                                if((p_vci.rdata.read() == 0) && p_vci.reop.read()) 
                                {
                                        
                                        rsp_fifo_data = rsp_fifo_data |  (((uint64_t) 0x1) << (ring_rsp_data_size-1)) ;
				        r_vci_rsp_fsm = RSP_SINGLE_DATA;
                                }
                                else
        				r_vci_rsp_fsm = RSP_MULTI_DATA;                      
			}
                }
		break;

		case RSP_SINGLE_DATA: // to avoid Mealy dependency (testing vci_rdata == 0 && vci_reop == 1 in genMoore/RSP_HEADER)

#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - vci_rsp_fsm = " << vci_rsp_fsm_state_str_t[r_vci_rsp_fsm]
                          << " - vci_rspval : " << p_vci.rspval.read()
                          << " - fifo wok : " << m_rsp_fifo.wok() 
                          << " - rsrcid : " << std::hex << p_vci.rsrcid.read()
                          << " - rdata : " << p_vci.rdata.read()
                          << " - reop : " << p_vci.reop.read()
                          << " - rerror : " << p_vci.rerror.read()
                          << std::endl;
#endif
           
			r_vci_rsp_fsm = RSP_HEADER;
		break;

		case RSP_MULTI_DATA:

#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - vci_rsp_fsm = " << vci_rsp_fsm_state_str_t[r_vci_rsp_fsm]
                          << " - vci_rspval : " << p_vci.rspval.read()
                          << " - fifo wok : " << m_rsp_fifo.wok() 
                          << " - rsrcid : " << std::hex << p_vci.rsrcid.read()
                          << " - rdata : " << p_vci.rdata.read()
                          << " - reop : " << p_vci.reop.read()
                          << " - rerror : " << p_vci.rerror.read()
                          << std::endl;
#endif  
          
			if(p_vci.rspval.read() && m_rsp_fifo.wok())
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
   
//////////// RING RSP FSM  /////////////////////////
        
	switch( r_ring_rsp_fsm ) 
	{
		case RSP_IDLE: 
  
#ifdef T_DEBUG 
if(m_cpt > m_cyc)
        std::cout << std::dec << sc_time_stamp() << " - " << m_name
                  << " - ring_rsp_fsm = " << ring_rsp_fsm_state_str_t[r_ring_rsp_fsm]
                  << " - fifo ROK : " << m_rsp_fifo.rok()
                  << " - in grant : " << p_ring_in.rsp_grant  
                  << " - in palloc : " << p_ring_in.rsp_palloc
                  << " - in wok : " << p_ring_in.rsp_r
                  << " - fifo data : " << std::hex << m_rsp_fifo.read()
                  << std::endl;  
#endif 

			if ( p_ring_in.rsp_grant && m_rsp_fifo.rok() ) 

				r_ring_rsp_fsm = SENDING;           
                
		break;

		case DEFAULT: 
                { 

#ifdef T_DEBUG 
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_rsp_fsm = " << ring_rsp_fsm_state_str_t[r_ring_rsp_fsm]
                          << " - in rsp grant : " << p_ring_in.rsp_grant
                          << " - in wok : " <<  p_ring_in.rsp_r
                          << " - fifo rok : " <<  m_rsp_fifo.rok()
                          << " - fifo data  : " <<  std::hex << m_rsp_fifo.read()
                          << std::endl;  
#endif

                        bool eop = ( (int) ((m_rsp_fifo.read() >> (ring_rsp_data_size - 1) ) & 0x1) == 1);

                        if ( m_rsp_fifo.rok() && eop && p_ring_in.rsp_r )  
			{
				rsp_fifo_get = true;
                                if ( p_ring_in.rsp_grant )
				        r_ring_rsp_fsm = DEFAULT;
                                else
				        r_ring_rsp_fsm = RSP_IDLE;
			}  
 
                        if ( m_rsp_fifo.rok() && (!eop || !p_ring_in.rsp_r)) 
			{
				rsp_fifo_get = p_ring_in.rsp_r;
				r_ring_rsp_fsm = SENDING;
			}

                        if ( !m_rsp_fifo.rok() && !p_ring_in.rsp_grant )
				r_ring_rsp_fsm = RSP_IDLE; 

                        if ( !m_rsp_fifo.rok() && p_ring_in.rsp_grant )
				r_ring_rsp_fsm = DEFAULT;
                }
		break;

		case SENDING: 
 
                // stay here while !release + rsp_preempt 
                // release = fifo_rok.ring_in.wok.fifo_data.eop
#ifdef T_DEBUG
if(m_cpt > m_cyc)
        std::cout << std::dec << sc_time_stamp() << " - " << m_name
                  << " - ring_rsp_fsm = " << ring_rsp_fsm_state_str_t[r_ring_rsp_fsm] 
                  << " - fifo ROK : " << m_rsp_fifo.rok()
                  << " - in grant : " << p_ring_in.rsp_grant  
                  << " - in preempt : " << p_ring_in.rsp_preempt
                  << " - in palloc : " << p_ring_in.rsp_palloc
                  << " - in wok : " << p_ring_in.rsp_r
                  << " - fifo data : " << std::hex << m_rsp_fifo.read()
                  << std::endl;
#endif                       
                        if(p_ring_in.rsp_preempt) break;

			if(m_rsp_fifo.rok() && p_ring_in.rsp_r)
			{
				rsp_fifo_get = true;  
                                bool eop = ((int) (m_rsp_fifo.read() >> (ring_rsp_data_size - 1) ) & 0x1) == 1;
                                
				if (eop && !p_ring_in.rsp_palloc && p_ring_in.rsp_grant)
                                { 
						r_ring_rsp_fsm = DEFAULT;  
                                }
                                else if  (eop && !p_ring_in.rsp_palloc && !p_ring_in.rsp_grant)          
                                        {   
						r_ring_rsp_fsm = RSP_IDLE; 
                                        }
                                        else if (eop && p_ring_in.rsp_palloc) 
                                                {
						        r_ring_rsp_fsm = WAIT_PALLOC_END;
				                }
			}   
   
		break;

		case WAIT_PALLOC_END:
                // stay here and keep token till Init Gate last flit
                bool eop = ((int) (p_ring_in.rsp_data >> (ring_rsp_data_size - 1) ) & 0x1) == 1;
#ifdef T_DEBUG
if(m_cpt > m_cyc)
        std::cout << std::dec << sc_time_stamp() << " - " << m_name
                  << " - ring_rsp_fsm = " << ring_rsp_fsm_state_str_t[r_ring_rsp_fsm] 
                  << " - palloc : " << p_ring_in.rsp_palloc
                  << " - in grant : " << p_ring_in.rsp_grant  
                  << " - in preempt : " << p_ring_in.rsp_preempt
                  << " - in rok : " << p_ring_in.rsp_w
                  << " - in wok : " << p_ring_in.rsp_r
                  << " - eop : " << eop
                  << std::endl;
#endif                        
                        if(p_ring_in.rsp_w && p_ring_in.rsp_r && eop) // last flit from Init gate
			{
				if (p_ring_in.rsp_grant)
					r_ring_rsp_fsm = DEFAULT;  
                                else            
					r_ring_rsp_fsm = CMD_IDLE; 
				        
			}   
   
		break;
	} // end switch ring cmd fsm

/////////// RING CMD FSM ////////////////////////
	switch( r_ring_cmd_fsm ) 
	{

		case CMD_IDLE:  
                // condition de broadcast pour local target : (flit(0) = 1).( (all coord = 0) + (srcid not local) )
                // first condition : broadcast comes from local initiator
                // second condition : broadcast comes from external initiator via init gate
		{ // for variable scope

			vci_addr_t rtgtid  = (vci_addr_t) ((p_ring_in.cmd_data >> m_shift) << 2);
                        uint32_t   rsrcid  =  (uint32_t) ((sc_dt::sc_uint<vci_param::S>) (p_ring_in.cmd_data >> 5));
		        bool       islocal = m_lt_addr[rtgtid]  && (m_rt[rtgtid] == m_tgtid); 
                        bool       brdcst  = (p_ring_in.cmd_data & 0x1) == 0x1;
                        bool       c1      = (p_ring_in.cmd_data >> 19) == 0;
                        bool       c2      = m_lt_src[rsrcid];
                        bool       eop     = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1); 

                        r_brdcst_p.write(false);
#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_cmd_fsm = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm]
                          << " - in preempt : " << p_ring_in.cmd_preempt
                          << " - in rok : " << p_ring_in.cmd_w
                          << " - in data : " << std::hex << p_ring_in.cmd_data
                          << " - brdcst : " << brdcst
                          << " - c1 (coord = 0) : " << c1
                          << " - c2 (srcid local) : " << c2
                          << " - rtgtid : " << rtgtid
                          << " - islocal : " << islocal
                          << " - in wok : " << p_ring_in.cmd_r
                          << " - fifo wok : " << m_cmd_fifo.wok()
                          << std::endl;
#endif

                        if(p_ring_in.cmd_w && !eop)
                        {
                                if(brdcst && (c1 || !c2)) 
                                {
                                        if (m_cmd_fifo.wok())
                                        {
                                                r_ring_cmd_fsm = BROADCAST_1;
                                                cmd_fifo_put  = true;
                        	                cmd_fifo_data = p_ring_in.cmd_data;
                                        }
                                        else
                                        {
                                                r_ring_cmd_fsm = BROADCAST_0;
                                        }
                        
                                }

                                if(brdcst && !c1 && c2) 
                                {
                                        r_ring_cmd_fsm = NALLOC;
                                }

                                if (!brdcst && islocal) 
                                {
                                        r_ring_cmd_fsm = ALLOC; 
                                        cmd_fifo_put   = m_cmd_fifo.wok();
                                        cmd_fifo_data  = p_ring_in.cmd_data;  
                                
                                } 
                                
                                if (!brdcst && !islocal) 
                                {
                                        r_ring_cmd_fsm = NALLOC;
                                }  
                        
                        }
                        else  r_ring_cmd_fsm = CMD_IDLE;

                }

		break;

                case BROADCAST_0:
                {
#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_cmd_fsm = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm]
                          << " - in preempt : " << p_ring_in.cmd_preempt
                          << " - in rok : " << p_ring_in.cmd_w
                          << " - in data : " << std::hex << p_ring_in.cmd_data
                          << " - in wok : " << p_ring_in.cmd_r
                          << " - fifo wok : " << m_cmd_fifo.wok()
                          << " - brdcst_p : " << r_brdcst_p
                          << std::endl;
#endif

                        // r_brdcst_p : we come from NALLOC state, ring is preempted
                        if (r_brdcst_p && !p_ring_in.cmd_preempt) break;

                	if (m_cmd_fifo.wok())
                        { 
				cmd_fifo_data = p_ring_in.cmd_data;
				cmd_fifo_put  = true;
				r_ring_cmd_fsm = BROADCAST_1;
                        }
                        
                } 
 		break;

                case BROADCAST_1:
                {
#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_cmd_fsm = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm]
                          << " - in preempt : " << p_ring_in.cmd_preempt
                          << " - in rok : " << p_ring_in.cmd_w
                          << " - in data : " << std::hex << p_ring_in.cmd_data
                          << " - in wok : " << p_ring_in.cmd_r
                          << " - fifo wok : " << m_cmd_fifo.wok()
                          << " - brdcst_p : " << r_brdcst_p
                          << std::endl;
#endif

                        // r_brdcst_p : we come from NALLOC state, ring is preempted
                        if (r_brdcst_p && !p_ring_in.cmd_preempt) break;

                        bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);


                	if (p_ring_in.cmd_w && eop && p_ring_in.cmd_r && m_cmd_fifo.wok())
                        { 
				cmd_fifo_data = p_ring_in.cmd_data;
				cmd_fifo_put  = true;
                                if(r_brdcst_p)  // r_brdcst_p = 1 => cmd_preempt = 1 
				        r_ring_cmd_fsm = NALLOC;
                                else
                                        r_ring_cmd_fsm = CMD_IDLE;
                        }
                        
                } 
 		break;

		case ALLOC:  
#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_cmd_fsm = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm]
                          << " - in preempt : " << p_ring_in.cmd_preempt
                          << " - in rok : " << p_ring_in.cmd_w
                          << " - in data : " << std::hex << p_ring_in.cmd_data
                          << " - in wok : " << p_ring_in.cmd_r
                          << " - fifo wok : " << m_cmd_fifo.wok()
                          << std::endl;
#endif
                {
                        bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);


                 	if ( p_ring_in.cmd_w && m_cmd_fifo.wok() && eop )
                        { 

				cmd_fifo_put  = true;
				cmd_fifo_data = p_ring_in.cmd_data;
			     	r_ring_cmd_fsm = CMD_IDLE;
			     		
                        }
                        
                 	else // !p_ring_in.cmd_w || !m_cmd_fifo.wok() || !eop
                        { 

				cmd_fifo_put  = p_ring_in.cmd_w && m_cmd_fifo.wok();
				cmd_fifo_data = p_ring_in.cmd_data;
			     	r_ring_cmd_fsm = ALLOC;
			     		
                        }                        
                } 
		break;

		case NALLOC:  
                // if preempt (init gate is preempting ring while target gate is allocated), 
                // if first flit of a new packet, is it for this local target ? 
#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_cmd_fsm = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm]
                          << " - preempt : " << p_ring_in.cmd_preempt
                          << " - palloc : " << p_ring_in.cmd_palloc
                          << " - header : " << p_ring_in.cmd_header
                          << " - in rok : " << p_ring_in.cmd_w
                          << " - in data : " << std::hex << p_ring_in.cmd_data
                          << " - in wok : " << p_ring_in.cmd_r
                          << " - fifo wok : " << m_cmd_fifo.wok()
                          << std::endl;
#endif              
                {
			bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);

                        
                        if(p_ring_in.cmd_preempt) 
                        {
                                // cmd_header : sent by init gate 
                                if(p_ring_in.cmd_header)
                                { 
                                        vci_addr_t rtgtid = (vci_addr_t) ((p_ring_in.cmd_data >> m_shift) << 2);
                                        bool islocal = m_lt_addr[rtgtid]  && (m_rt[rtgtid] == m_tgtid);
                                        bool brdcst  = (p_ring_in.cmd_data & 0x1) == 0X1 ;

                                        r_brdcst_p.write(true); // broadcast with cmd_preempt => cmd_palloc = 1
                                        // broadcast from init gate, ring is preempted, Target gate is in ALLOC state 
                                        if(brdcst)  
                                        {
                                                if(!m_cmd_fifo.wok())
                                                        r_ring_cmd_fsm = BROADCAST_0; 
                                                else
                                                {
                                                        r_ring_cmd_fsm = BROADCAST_1;
                                                        cmd_fifo_put  = true;
                      	                                cmd_fifo_data = p_ring_in.cmd_data;
                                                }

                                        }  
                                        
                                        else if (islocal) 
                                                {
                                                        r_ring_cmd_fsm = PALLOC2; 
                                                        cmd_fifo_put   = m_cmd_fifo.wok();
                                                        cmd_fifo_data  = p_ring_in.cmd_data;
                                                }

                                                else // preempt && header && !brdcst && !islocal
                                                        r_ring_cmd_fsm = NALLOC;
                                }
                                else // preempt && !header 
                                {
                                        // palloc=1, means TG is not allocated, thus, if last flit, all targets must be in IDLE state
                                        if(eop && p_ring_in.cmd_r && (p_ring_in.cmd_palloc == 1))        
                                                r_ring_cmd_fsm = CMD_IDLE;
                                        else
                                                r_ring_cmd_fsm = NALLOC;

                                }

                        }

                        else // !preempt
                        {
                                if(p_ring_in.cmd_w && eop && p_ring_in.cmd_r && !p_ring_in.cmd_palloc)
                                        r_ring_cmd_fsm = CMD_IDLE;
                                else 
                                        r_ring_cmd_fsm = NALLOC;

                        }

                }
		break;

                case PALLOC2:
#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_cmd_fsm = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm]
                          << " - in preempt : " << p_ring_in.cmd_preempt
                          << " - in rok : " << p_ring_in.cmd_w
                          << " - in data : " << std::hex << p_ring_in.cmd_data
                          << " - in wok : " << p_ring_in.cmd_r
                          << " - fifo wok : " << m_cmd_fifo.wok()
                          << std::endl;
#endif
                {
                         bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);

                        if(p_ring_in.cmd_preempt)      
                        {
                                cmd_fifo_put  = m_cmd_fifo.wok();
                                cmd_fifo_data = p_ring_in.cmd_data;
                                
                                if(eop && m_cmd_fifo.wok())
                                        r_ring_cmd_fsm = NALLOC;
                                else
                                        r_ring_cmd_fsm = PALLOC2;
                                break;
                        }

                        if(p_ring_in.cmd_w && eop && p_ring_in.cmd_r)
                        {
			        r_ring_cmd_fsm = PALLOC1;
                        }
                 	                        
                } 
		break;                       

                case PALLOC1:
#ifdef T_DEBUG
if(m_cpt > m_cyc)
    std::cout << std::dec << sc_time_stamp() << " - " << m_name
                          << " - ring_cmd_fsm = " << ring_cmd_fsm_state_str_t[r_ring_cmd_fsm]
                          << " - in preempt : " << p_ring_in.cmd_preempt
                          << " - in rok : " << p_ring_in.cmd_w
                          << " - in data : " << std::hex << p_ring_in.cmd_data
                          << " - in wok : " << p_ring_in.cmd_r
                          << " - fifo wok : " << m_cmd_fifo.wok()
                          << std::endl;
#endif
                {
                         bool eop = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1);

                 	if ( p_ring_in.cmd_preempt && m_cmd_fifo.wok() && eop )
                        { 

				cmd_fifo_put  = true;
				cmd_fifo_data = p_ring_in.cmd_data;
                                r_ring_cmd_fsm = CMD_IDLE;
			     		
                        }
                        
                 	else // !p_ring_in.cmd_w || !m_cmd_fifo.wok() || !eop 
                        { 

				cmd_fifo_put  = p_ring_in.cmd_preempt && m_cmd_fifo.wok();
				cmd_fifo_data = p_ring_in.cmd_data;
			     	r_ring_cmd_fsm = PALLOC1;
			     		
                        }                        
                } 
		break;

	} // end switch cmd fsm 

#ifdef T_DEBUG
	m_cpt +=1;
#endif

    ////////////////////////
    //  fifos update      //
   ////////////////////////
//-- to keep trace on ring traffic : a valid command is received by the target
        tgt_rsp.rspval  = rsp_fifo_get;
        tgt_rsp.flit    = m_rsp_fifo.read();
        tgt_rsp.state   = ring_rsp_fsm_state_str_t[r_ring_rsp_fsm];

	tgt_cmd_val = cmd_fifo_put;
	//tgt_rsp_val = rsp_fifo_get;
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
	if(r_vci_rsp_fsm == RSP_HEADER)
        { 
	        p_vci.rspack = false;
        }
	else if (r_vci_rsp_fsm == RSP_SINGLE_DATA)
             {   
	        p_vci.rspack = true;
             }
             else // multi_data
             {   
	        p_vci.rspack = m_rsp_fifo.wok();
             }

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
			p_ring_out.rsp_grant = !m_rsp_fifo.rok() && p_ring_in.rsp_grant;

                        p_ring_out.rsp_palloc  = p_ring_in.rsp_palloc;
                        p_ring_out.rsp_preempt = p_ring_in.rsp_preempt;

	        	p_ring_out.rsp_w    = p_ring_in.rsp_w;
		        p_ring_out.rsp_data = p_ring_in.rsp_data;

		break;

		case DEFAULT:
                {
                        bool eop = ( (int) ((m_rsp_fifo.read() >> (ring_rsp_data_size - 1) ) & 0x1) == 1);
			p_ring_out.rsp_grant = (!m_rsp_fifo.rok() || (eop && p_ring_in.rsp_r)) ; 

                        p_ring_out.rsp_palloc  = 0;                   
                        p_ring_out.rsp_preempt = 0;                     

        		p_ring_out.rsp_w    =  m_rsp_fifo.rok();
	        	p_ring_out.rsp_data =  m_rsp_fifo.read(); 

                }
		break;

		case SENDING: 
                { 
                        bool eop = ( (int) ((m_rsp_fifo.read() >> (ring_rsp_data_size - 1) ) & 0x1) == 1);

                        p_ring_out.rsp_palloc  = p_ring_in.rsp_palloc;
                        p_ring_out.rsp_preempt = p_ring_in.rsp_preempt;

                        // if preempt : Init Gate is sending to Local Target, ring preempted
                        if (p_ring_in.rsp_preempt) 
                        {
                		p_ring_out.rsp_w     = p_ring_in.rsp_w;
	                	p_ring_out.rsp_data  = p_ring_in.rsp_data;
                                p_ring_out.rsp_grant = 0;
                        }
                        else
                        {
	        	        p_ring_out.rsp_w     = m_rsp_fifo.rok();
		                p_ring_out.rsp_data  = m_rsp_fifo.read();
                                p_ring_out.rsp_grant = m_rsp_fifo.rok() && p_ring_in.rsp_r && eop && !p_ring_in.rsp_palloc;
                        }
                }
		break; 

               case WAIT_PALLOC_END: 
                {
                        bool eop = ((int) (p_ring_in.rsp_data >> (ring_rsp_data_size - 1) ) & 0x1) == 1;

                        p_ring_out.rsp_grant = p_ring_in.rsp_w && p_ring_in.rsp_r && eop; 

                        p_ring_out.rsp_palloc  = p_ring_in.rsp_palloc;
                        p_ring_out.rsp_preempt = p_ring_in.rsp_preempt; 

               		p_ring_out.rsp_w       = p_ring_in.rsp_w; //p_ring_in.rsp_preempt; 
                	p_ring_out.rsp_data    = p_ring_in.rsp_data;
                }        
		break;
	} // end switch

        p_ring_out.rsp_header  = p_ring_in.rsp_header;
        p_ring_out.rsp_r       = p_ring_in.rsp_r;

	p_ring_out.cmd_w       = p_ring_in.cmd_w;
	p_ring_out.cmd_data    = p_ring_in.cmd_data;

	p_ring_out.cmd_grant   = p_ring_in.cmd_grant;

        p_ring_out.cmd_preempt = p_ring_in.cmd_preempt;
        p_ring_out.cmd_header  = p_ring_in.cmd_header;
        p_ring_out.cmd_palloc  = p_ring_in.cmd_palloc;

	switch( r_ring_cmd_fsm ) 
	{
		case CMD_IDLE:
                // condition de broadcast pour local target : (flit(0) = 1).( (all coord = 0) + (srcid not local) )
                // first condition : broadcast comes from local initiator
                // second condition : broadcast comes from external initiator via init gate
		{
			vci_addr_t rtgtid  = (vci_addr_t) ((p_ring_in.cmd_data >> m_shift) << 2);
                        uint32_t   rsrcid  =  (uint32_t) ((sc_dt::sc_uint<vci_param::S>) (p_ring_in.cmd_data >> 5));
		        bool       islocal = m_lt_addr[rtgtid]  && (m_rt[rtgtid] == m_tgtid); 
                        bool       brdcst  = (p_ring_in.cmd_data & 0x1) == 0X1;
                        bool       c1      = (p_ring_in.cmd_data >> 19) == 0;
                        bool       c2      = m_lt_src[rsrcid];
                        bool       eop     = ( (int) ((p_ring_in.cmd_data >> (ring_cmd_data_size - 1) ) & 0x1) == 1); 
                        

                        if(p_ring_in.cmd_w && !eop && brdcst && (c1 || !c2)) 
                        {
                                p_ring_out.cmd_r = m_cmd_fifo.wok() && (m_alloc_target || p_ring_in.cmd_r);
                        }  
        
                        else if (p_ring_in.cmd_w && !eop && !brdcst && islocal) 
                                {
                                        p_ring_out.cmd_r =  m_cmd_fifo.wok();

                                } 
                                else
                                {
                                        p_ring_out.cmd_r =  p_ring_in.cmd_r; 
                                }  

		}
		break;

                case BROADCAST_0:

                        if(r_brdcst_p && !p_ring_in.cmd_preempt)
                                p_ring_out.cmd_r =  p_ring_in.cmd_r; 
                        else
                                p_ring_out.cmd_r =  m_cmd_fifo.wok() && (m_alloc_target || p_ring_in.cmd_r);
                break;

                case BROADCAST_1:

                        if(r_brdcst_p && !p_ring_in.cmd_preempt)
                                p_ring_out.cmd_r =  p_ring_in.cmd_r; 
                        else
                                p_ring_out.cmd_r =  m_cmd_fifo.wok() && (m_alloc_target || p_ring_in.cmd_r); 
                break;

		case ALLOC:
                        p_ring_out.cmd_r =  m_cmd_fifo.wok(); 	
		break;

		case NALLOC:
                {
                        vci_addr_t rtgtid = (vci_addr_t) ((p_ring_in.cmd_data >> m_shift) << 2);
                        bool islocal = m_lt_addr[rtgtid]  && (m_rt[rtgtid] == m_tgtid);
                        bool brdcst  = (p_ring_in.cmd_data & 0x1) == 0X1 ;

                        if(p_ring_in.cmd_preempt && p_ring_in.cmd_header && brdcst )
                                p_ring_out.cmd_r  = m_cmd_fifo.wok() && (m_alloc_target || p_ring_in.cmd_r);
                        else if (p_ring_in.cmd_preempt && p_ring_in.cmd_header && islocal)
                                        p_ring_out.cmd_r  = m_cmd_fifo.wok();
                             else 
                                        p_ring_out.cmd_r  =  p_ring_in.cmd_r; 

                }
		break; 

		case PALLOC2:
                        if(p_ring_in.cmd_preempt)
                                p_ring_out.cmd_r  = m_cmd_fifo.wok(); 	
                        else 
                                p_ring_out.cmd_r  = p_ring_in.cmd_r;
		break;

		case PALLOC1:
                        p_ring_out.cmd_r  = m_cmd_fifo.wok(); 	
                break;

	} // end switch

} // end update_ring_signals 
  
};

}} // end namespace
#endif // SOCLIB_CABA_VCI_ALLOC_RING_TARGET_FAST_H
