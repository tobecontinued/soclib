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
 * Authors  : alain.greiner@lip6.fr 
 * Date     : july 2010
 * Copyright: UPMC - LIP6
 */

#include "alloc_elems.h"
#include "dspin_packet_generator.h"
#include <cstdlib>
#include <sstream>

namespace soclib { namespace caba {

using namespace soclib::caba;
using namespace soclib::common;

#define tmpl(x) template<size_t flit_width> x DspinPacketGenerator<flit_width>


    ///////////////////////////////////////////////////////
    //              constructor
    ///////////////////////////////////////////////////////
    tmpl(/**/)::DspinPacketGenerator( sc_module_name name, 
                                      size_t 	     length,  // packet length
                                      size_t	     load,    // load * 1000
                                      size_t	     bcp )    // broadcast period
    : BaseModule(name),

    p_clk( "clk" ),
    p_resetn( "resetn" ),
    p_in( "p_in" ),
    p_out( "p_out" ),

    r_fsm( "r_fsm" ),
    r_cycles( "r_cycles" ),
    r_packets( "r_packets" ),
    r_length( "r_length" ), 
 
    m_length( length ),
    m_load( load ),
    m_bcp( bcp )

    {
        assert( (load >= 1) and (load <= 1000) and
        "DSPIN PACKET GENERATOR ERROR: The load should be between 1 and 1000" );

        assert( (flit_width >= 22) and
        "DSPIN PACKET GENERATOR ERROR: The flit width cannot be less than 22 bits");

        assert( (length > 0 ) and
        "DSPIN PACKET GENERATOR ERROR: The packet length cannot be 0");

        SC_METHOD (transition);
        dont_initialize();
        sensitive << p_clk.pos();

        SC_METHOD (genMoore);
        dont_initialize();
        sensitive  << p_clk.neg();

    } //  end constructor

    ////////////////////////
    tmpl(void)::transition()
    {
        if ( not p_resetn.read() )
        {
            r_fsm     = STATE_IDLE;
            r_cycles  = 1;
            r_packets = 1;
            srandom(1968);
            return;
        }

        r_cycles = r_cycles.read() + 1;

        switch( r_fsm.read() ) 
        {
            case STATE_IDLE:
                if ( ( (r_packets.read() * m_length) * 1000) / r_cycles.read() < m_load )
                {
                    if ( r_packets.read() % m_bcp )  // no broadcast
                    {
                        r_length = m_length;
                        r_fsm    = STATE_SEND;
                        r_dest   = random() & 0x3FF;
                    }
                    else							// broadcast
                    {
                        r_length = 2;
                        r_fsm    = STATE_BROADCAST;
                    }
                }
            break;
            case STATE_SEND:
                if( p_out.read ) 
                {
                    r_length = r_length.read() - 1;
                    if( r_length.read() == 1 )  
                    {
                        r_fsm     = STATE_IDLE;
                        r_packets = r_packets.read() + 1;
                    }
                }
            break;
            case STATE_BROADCAST:
                if( p_out.read ) 
                {
                    r_length = r_length.read() - 1;
                    if( r_length.read() == 1 )  
                    {
                        r_fsm     = STATE_IDLE;
                        r_packets = r_packets.read() + 1;
                    }
                }
            break;
        }
    } // end transition

    //////////////////////
    tmpl(void)::genMoore()
    {
        // p_out
        sc_uint<flit_width>	data;
        bool                write;

        if ( r_fsm.read() == STATE_IDLE )
        {
            write = false;
        }
        else if ( r_fsm.read() == STATE_SEND )
        {
            write = true;
            if ( r_length.read() == m_length )  // first flit
            {
                data = ((sc_uint<flit_width>)r_dest.read()) << (flit_width - 11);
            }
            else
            {
                data = 0;
            }
            if( r_length.read() == 1 ) 
                data |= sc_uint<flit_width>(1) << (flit_width - 1);
        }
        else  // STATE_BROADCAST
        {
            write = true;
            if ( r_length.read() == 2 )  // first flit
            {
                data = sc_uint<flit_width>(0x07C1F) << (flit_width - 21) | 
                       sc_uint<flit_width>(1);	 
            }
            else // r_length == 1
            {
                data = sc_uint<flit_width>(1)<<(flit_width-1);
            }
        }
        p_out.data  = data;
        p_out.write = write;

        // p_in
        p_in.read = true;
    } // end genMoore

    /////////////////////////
    tmpl(void)::print_trace()
    {
        const char* str_state[] = { "IDLE", "SEND", "BROADCAST" };

        std::cout << "DSPIN_GENERATOR " << name() << " : state = " 
                  << str_state[r_fsm.read()] << std::endl;
    } // end printTrace

}} // end namespaces 
