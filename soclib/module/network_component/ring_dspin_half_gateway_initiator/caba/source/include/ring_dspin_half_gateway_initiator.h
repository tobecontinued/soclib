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
 * Date     : july 2010
 * Copyright: UPMC - LIP6
 */

#ifndef RING_DSPIN_HALF_GATEWAY_INITIATOR_H_
#define RING_DSPIN_HALF_GATEWAY_INITIATOR_H_

#include <systemc.h>
#include "caba_base_module.h"
#include "ring_ports.h"
#include "dspin_interface.h"
#include "generic_fifo.h"
#include "mapping_table.h"

namespace soclib { namespace caba {

template<int cmd_width, int rsp_width>
class RingDspinHalfGatewayInitiator : public soclib::caba::BaseModule
{

protected:
    SC_HAS_PROCESS(RingDspinHalfGatewayInitiator);

public:
    // ports
    sc_core::sc_in<bool> 			p_clk;
    sc_core::sc_in<bool> 			p_resetn;
    soclib::caba::RingIn        		p_ring_in;
    soclib::caba::RingOut       		p_ring_out;
    soclib::caba::DspinInput<cmd_width>  	p_gate_cmd_in;	
    soclib::caba::DspinOutput<rsp_width>   	p_gate_rsp_out;	

    // constructor
    RingDspinHalfGatewayInitiator(sc_module_name			insname,
                                  bool                			alloc_init,
                                  const int           			&rsp_fifo_depth,
                                  const soclib::common::MappingTable 	&mt,
                                  const soclib::common::IntTab 		&ringid,
                            	  bool  				local);
      
private:
    
    enum ring_rsp_fsm_state_e {
	RSP_IDLE,    	// waiting for first flit of a response packet
	LOCAL,      	// next flit of a rsp local packet
	RING,  	    	// next flit of a rsp ring packet
	};

    // cmd token allocation fsm
    enum ring_cmd_fsm_state_e {
	CMD_IDLE,	    
       	DEFAULT,  	
	KEEP,          	    
	};

    // internal registers
    sc_signal<int>      		r_ring_cmd_fsm;    // ring command packet FSM 
    sc_signal<int>			r_ring_rsp_fsm;    // ring response packet FSM
          
    // structural parameters
    bool    				m_alloc_init;
    bool    				m_local;

    // internal fifos 
    GenericFifo<sc_uint<rsp_width> > 	m_rsp_fifo;        // fifo for the local response paquet
	  	
    // locality table
    soclib::common::AddressDecodingTable<uint32_t, bool> m_lt;
       
    // methods
    void transition();
    void genMoore();
    void genMealy_rsp_out();
    void genMealy_rsp_in();
    void genMoore_rsp_fifo_out();
    void genMealy_cmd_out();
    void genMealy_cmd_in();
    void genMealy_cmd_grant();
    void genMealy_rsp_grant();

};

}} // end namespace
		
#endif // RING_DSPIN_HALF_GATEWAY_INITIATOR_H_


