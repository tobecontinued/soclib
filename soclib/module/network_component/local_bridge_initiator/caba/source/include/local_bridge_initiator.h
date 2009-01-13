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
 * Date     : December 2008
 * Copyright: UPMC - LIP6
 */

#ifndef LOCAL_BRIDGE_INITIATOR_H_
#define LOCAL_BRIDGE_INITIATOR_H_

#include <systemc.h>
#include "caba_base_module.h"
#include "ring_ports.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "fifo_ports.h"

namespace soclib { namespace caba {

using namespace sc_core;

class LocalBridgeInitiator 
	: public soclib::caba::BaseModule
{

protected:
	SC_HAS_PROCESS(LocalBridgeInitiator);

public:
    // ports
    sc_in<bool> 	                	p_clk;
    sc_in<bool> 	                	p_resetn;
    soclib::caba::RingIn                        p_ring_in;
    soclib::caba::RingOut                       p_ring_out;
    soclib::caba::FifoOutput<sc_uint<33> >      p_rsp_fifo;
    soclib::caba::FifoInput<sc_uint<37> >       p_cmd_fifo;

    // constructor
    LocalBridgeInitiator(sc_module_name	insname,
                            bool                alloc_init,
                            const int           &rsp_fifo_depth,
                            const soclib::common::MappingTable &mt,
                            const soclib::common::IntTab &ringid);
      
private:
    
    enum ring_rsp_fsm_state_e {
		RSP_IDLE,    // waiting for first flit of a response packet
		GATE,        // next flit of a rsp gateway packet
		RING,  	     // next flit of a rsp ring packet
	};

    // cmd token allocation fsm
    enum ring_cmd_fsm_state_e {
		CMD_IDLE,	    
         	DEFAULT,  	
		KEEP,          	    
	};

    // structural parameters
    bool                m_alloc_init;
             
    // internal registers
    sc_signal<int>      r_ring_cmd_fsm;    // ring command packet FSM (distributed)
    sc_signal<int>	r_ring_rsp_fsm;    // ring response packet FSM
          
    // internal fifos 
    GenericFifo<sc_uint<33> > m_rsp_fifo;     // fifo for the local response paquet

    // routing tables
    soclib::common::AddressDecodingTable<uint32_t, bool> m_lt;
       
    // methods
    void transition();
    void genMealy_rsp_out();
    void genMealy_rsp_in();
    void genMoore_rsp_fifo_out();
    void genMealy_cmd_out();
    void genMealy_cmd_in();
    void genMealy_cmd_grant();
    void genMealy_rsp_grant();

};

}} // end namespace
		
#endif // LOCAL_BRIDGE_INITIATOR_H_


