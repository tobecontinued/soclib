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
 * Authors  : Franck WAJSB�RT, Abdelmalek SI MERABET 
 * Date     : December 2008
 *
 * Copyright: UPMC - LIP6
 */

#ifndef LOCAL_BRIDGE_TARGET_H_
#define LOCAL_BRIDGE_TARGET_H_

#include <systemc.h>
#include "caba_base_module.h"
#include "ring_ports.h"
#include "generic_fifo.h"
#include "mapping_table.h"
#include "fifo_ports.h"

namespace soclib { namespace caba {

using namespace sc_core;

class LocalBridgeTarget 
	: public soclib::caba::BaseModule
{

    protected:
		SC_HAS_PROCESS(LocalBridgeTarget);

    public:
	// ports
	sc_in<bool> 		               	p_clk;
	sc_in<bool>   		               	p_resetn;
	soclib::caba::RingIn                    p_ring_in;
	soclib::caba::RingOut                   p_ring_out;
	soclib::caba::FifoInput<sc_uint<33> >   p_rsp_fifo;
        soclib::caba::FifoOutput<sc_uint<37> >  p_cmd_fifo;

	// constructor
	LocalBridgeTarget(sc_module_name	insname,
                            bool            alloc_target,
                            const int       &cmd_fifo_depth,
                            const soclib::common::MappingTable &mt,
                            const soclib::common::IntTab &ringid);
    
    private:    	
	enum ring_cmd_fsm_state_e {
		CMD_IDLE,	 // waiting for first flit of a command packet
		GATE,            // next flit of a gateway packet
		RING,  	         // next flit of a ring packet
	};

    	// cmd token allocation fsm
	enum ring_rsp_fsm_state_e {
		RSP_IDLE,	    
     		DEFAULT,  	
		KEEP,  	            
	};

	// structural parameters
	bool          m_alloc_target;
		
	// internal registers
	sc_signal<int>	        r_ring_cmd_fsm;	    // ring command packet FSM 
	sc_signal<int>		r_ring_rsp_fsm;	    // ring response packet FSM
        
	            
        // internal fifos 
	GenericFifo<sc_uint<37> > m_cmd_fifo;     // fifo for the local command packet
	
        // locality table 
	soclib::common::AddressDecodingTable<uint32_t, bool> m_lt;
	      
	// methods
	void transition();
        void genMoore();
        void genMealy_rsp_out();
	void genMealy_rsp_in();
	void genMealy_cmd_out();
	void genMealy_cmd_in();
        void genMoore_cmd_fifo_out();
	void genMealy_cmd_grant();
    	void genMealy_rsp_grant();

};

}} // end namespace
		
#endif // LOCAL_BRIDGE_TARGET_H_


