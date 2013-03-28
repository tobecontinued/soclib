/* -*- c++ -*-
 *
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
 * Authors  : alain.greiner@lip6.fr 
 * Date     : august 2010
 * Copyright: UPMC - LIP6
 */

#ifndef DSPIN_PACKET_GENERATOR_H
#define DSPIN_PACKET_GENERATOR_H

#include <systemc>
#include "caba_base_module.h"
#include "dspin_interface.h"

////////////////////////////////////////////////////////////////////////////
// This component is a synthetic sender/receiver of DSPIN paquets.
// It is intended for functional debug, but not for performance.
// The offered load is defined by : (NB_PACKETS * LENGTH / NB_CYCLES).
// All received packets are consumed, but not analysed.
// The flit width is a template parameter.
////////////////////////////////////////////////////////////////////////////
// It has three constructors parameters :
// - size_t length == number of flits
// - size_t load == LOAD*1000
// - size_t bcp == NB_PACKETS / NB_BROACAST  (optionnal)
////////////////////////////////////////////////////////////////////////////
// The flit width Cannot be less than 22 bits:
// - DSPIN first flit format in case of a non-broadcast packet :
//  |EOP|   X     |   Y     |---------------------------------------|BC |
//  | 1 | x_width | y_width |  flit_width - (x_width + y_width + 2) | 1 |
//
// - DSPIN first flit format in case of a broadcast packet :
//  |EOP|  XMIN   |  XMAX   |  YMIN   |  YMAX   |-------------------|BC |
//  | 1 |   5     |   5     |   5     |   5     | flit_width - 22   | 1 |
////////////////////////////////////////////////////////////////////////////


namespace soclib { namespace caba {

// FSM states
enum
{ 
	STATE_IDLE,
	STATE_SEND,
    STATE_BROADCAST,
};

template<size_t flit_width>
class DspinPacketGenerator
: public soclib::caba::BaseModule
{			

protected:

	SC_HAS_PROCESS(DspinPacketGenerator);

public:

	// ports
	sc_core::sc_in<bool>                   p_clk;
	sc_core::sc_in<bool>                   p_resetn;
	soclib::caba::DspinInput<flit_width>   p_in;
	soclib::caba::DspinOutput<flit_width>  p_out;

	// constructor 
	DspinPacketGenerator( sc_module_name  name,
                          size_t          length,		// packet length
                          size_t          load,			// 1000*load
                          size_t		  bcp = 0xFFFFFFFF );
private:

	//  registers 
	sc_core::sc_signal<int>	    r_fsm;		// FSM state
	sc_core::sc_signal<size_t> 	r_cycles;	// cycles counter
	sc_core::sc_signal<size_t>  r_packets;  // packets counter 	
	sc_core::sc_signal<size_t>  r_length;	// packet length counter
	sc_core::sc_signal<size_t>  r_dest;		// packet destination (x,y)

	// structural variables
	size_t                      m_length;   // number of idle cycles
	size_t                      m_load;	    // packet length
    size_t                      m_bcp;		// broadcast period

	// methods 
	void transition();
	void genMoore();

public:

    void print_trace();

}; // end class DspinPacketGenerator
	
}} // end namespace

#endif // end DSPIN_PACKET_GENERATOR_H
