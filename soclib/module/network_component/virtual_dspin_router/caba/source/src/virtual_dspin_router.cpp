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
 * Authors  : alain.greiner@lip6.fr noe.girand@polytechnique.org
 * Date     : july 2010
 * Copyright: UPMC - LIP6
 */

#include "alloc_elems.h"
#include "virtual_dspin_router.h"
#include <cstdlib>
#include <sstream>

namespace soclib { namespace caba {

using namespace soclib::caba;
using namespace soclib::common;

#define tmpl(x) template<int flit_width> x VirtualDspinRouter<flit_width>

    ///////////////////////////////////////////////////////////////////////
    // The four functions xfirst_route(), broadcast_route() 
    // is_eop() and is_broadcast() defined below are used 
    // to decode the DSPIN first flit format:
    // The EOP, X, and Y fiels are left aligned.
    // The XMIN, XMAX, YMIN, YMAX and BC fields are used
    // in case of broadcast, and are right aligned
    // 
    //  |EOP|   X    |   Y    |---|  XMIN  |  XMAX  |  YMIN  |  YMAX  |BC |
    //  | 1 | x_size | y_size |---| x_size | x_size | y_size | y_size | 2 |
    //
    /////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////
    tmpl(int)::xfirst_route(sc_uint<flit_width> data)
    {
        int xdest = (int)(data >> m_x_shift) & m_x_mask;
        int ydest = (int)(data >> m_y_shift) & m_y_mask;
        return (xdest < m_local_x ? REQ_WEST : 
               (xdest > m_local_x ? REQ_EAST : 
               (ydest < m_local_y ? REQ_SOUTH : 
               (ydest > m_local_y ? REQ_NORTH : REQ_LOCAL))));
    }

    /////////////////////////////////////////////////////////////////////////
    tmpl(int)::broadcast_route(int iter, int source, sc_uint<flit_width> data)
    {
        int sel = REQ_NOP;
        int xmin = (data >> (2 + 2*m_y_size + m_x_size) ) & m_x_mask;
        int xmax = (data >> (2 + 2*m_y_size           ) ) & m_x_mask;
        int ymin = (data >> (2 + m_y_size             ) ) & m_y_mask;
        int ymax = (data >> (2                        ) ) & m_y_mask;

        switch(source) {
        case LOCAL :
            if      ( iter == FIRST  )	sel = REQ_NORTH;
            else if ( iter == SECOND )	sel = REQ_SOUTH;
            else if ( iter == THIRD  )	sel = REQ_EAST;
            else if ( iter == FOURTH )	sel = REQ_WEST;
        break;
        case NORTH :
            if      ( iter == FIRST  )	sel = REQ_SOUTH;
            else if ( iter == SECOND )	sel = REQ_LOCAL;
            else if ( iter == THIRD  )	sel = REQ_NOP;
            else if ( iter == FOURTH )	sel = REQ_NOP;
        break;
        case SOUTH :
            if      ( iter == FIRST  )	sel = REQ_NORTH;
            else if ( iter == SECOND )	sel = REQ_LOCAL;
            else if ( iter == THIRD  )	sel = REQ_NOP;
            else if ( iter == FOURTH )	sel = REQ_NOP;
        break;
        case EAST :
            if      ( iter == FIRST  )	sel = REQ_WEST;
            else if ( iter == SECOND )	sel = REQ_NORTH;
            else if ( iter == THIRD  )	sel = REQ_SOUTH;
            else if ( iter == FOURTH )	sel = REQ_LOCAL;
        break;
        case WEST :
            if      ( iter == FIRST  )	sel = REQ_EAST;
            else if ( iter == SECOND )	sel = REQ_NORTH;
            else if ( iter == THIRD  )	sel = REQ_SOUTH;
            else if ( iter == FOURTH )	sel = REQ_LOCAL;
        break;
        }
        if      ( (sel == REQ_NORTH) && !(m_local_y < ymax) ) 	sel = REQ_NOP;
        else if ( (sel == REQ_SOUTH) && !(m_local_y > ymin) ) 	sel = REQ_NOP;
        else if ( (sel == REQ_EAST ) && !(m_local_x < xmax) ) 	sel = REQ_NOP;
        else if ( (sel == REQ_WEST ) && !(m_local_x > xmin) ) 	sel = REQ_NOP;
        return sel;
    }

    ///////////////////////////////////////////
    tmpl(bool)::is_eop(sc_uint<flit_width> data)
    {
        return ( ((data>>(flit_width-1)) & 0x1) != 0);
    }

    /////////////////////////////////////////////////
    tmpl(bool)::is_broadcast(sc_uint<flit_width> data)
    {
        return ( (data & 0x3) != 0);
    }

    ////////////////////////////////////////////////////////////
    tmpl(/**/)::VirtualDspinRouter(	sc_module_name 	insname, 
					int 	x,
					int 	y,
					int	x_size,
					int	y_size,
                                       	int	in_fifo_depth,
                                       	int	out_fifo_depth)
    : BaseModule(insname),
      p_clk("clk"),
      p_resetn("resetn")
    {
        SC_METHOD (transition);
        dont_initialize();
        sensitive << p_clk.pos();

        SC_METHOD (genMoore);
        dont_initialize();
        sensitive  << p_clk.neg();

        // The minimal width of a DSPIN flit is 33 bits
        if ( flit_width < 33 )
        {
            std::cout << "Error in the virtual_dspin_router" << name() << std::endl;
            std::cout << "The flit_width parameter cannot be smaller than 33" << std::endl;
            exit(0);
        }

        // ports are inconditionnaly constructed
        p_in  = alloc_elems<DspinInput<flit_width> >("p_in", 2, 5);
        p_out = alloc_elems<DspinOutput<flit_width> >("p_out", 2, 5);
			
        // input & output fifos are unconditionnally constructed 
        std::ostringstream str;
        in_fifo = (GenericFifo<sc_uint<flit_width> >**)malloc(sizeof(GenericFifo<sc_uint<flit_width> >*)*2);
        out_fifo = (GenericFifo<sc_uint<flit_width> >**)malloc(sizeof(GenericFifo<sc_uint<flit_width> >*)*2);
        for(int k=0; k<2; k++)
        {
            in_fifo[k] = (GenericFifo<sc_uint<flit_width> >*) 
                         malloc(sizeof(GenericFifo<sc_uint<flit_width> >)*5);
            out_fifo[k] = (GenericFifo<sc_uint<flit_width> >*) 
                         malloc(sizeof(GenericFifo<sc_uint<flit_width> >)*5);
            for(int i=0; i<5; i++) 
            {
                str << "in_fifo_" << name() << "_" << k << i;
                new(&in_fifo[k][i]) GenericFifo<sc_uint<flit_width> >(str.str(), in_fifo_depth) ;
                str << "out_fifo_" << name() << "_" << k << i;
                new(&out_fifo[k][i]) GenericFifo<sc_uint<flit_width> >(str.str(), out_fifo_depth) ;
            }
        }

        m_local_x 		= x;
        m_local_y 		= y;
        m_x_size		= x_size;
        m_y_size		= y_size;
        m_x_shift		= flit_width - x_size;
        m_y_shift		= flit_width - x_size - y_size;
        m_x_mask		= (0x1 << x_size) - 1;
        m_y_mask		= (0x1 << y_size) - 1;

    } //  end constructor

    /////////////////////////////////
    tmpl(/**/)::~VirtualDspinRouter()
    {
    }

    ////////////////////////
    tmpl(void)::transition()
    {
        if(!p_resetn.read())
        {
            for(int i=0; i<5; i++) // both input & output ports
            {
                r_tdm[i]	= false;
                for(int k=0 ; k<2 ; k++) 
                {
                    r_input_fsm[k][i]		= INFSM_REQ_FIRST;			
                    r_output_index[k][i] 	= 0;
                    r_output_alloc[k][i]  	= false;
                    in_fifo[k][i].init();
                    out_fifo[k][i].init();
                }
            }
            return;
        }

        // internal variables used in each input port module
        // they will not be implemented as inter-module signals in the RTL
        bool	 		in_fifo_read[2][5];	// consume data in input_fifo 
        bool	       		in_fifo_write[2][5];	// write data in input_fifo
        sc_uint<flit_width> 	input_data[2][5]; 	// for each channel, data to be transmitted
        bool 			put[2][5];		// input port wishes to transmit data
        bool			get[2][5];		// the selected oupput port consume data

        // internal variables used in each output port module
        // they will not be implemented as inter-module signals in the RTL
	bool	        	out_fifo_write[2][5]; 	// write data in output_fifo 
	bool	        	out_fifo_read[2][5]; 	// consume data in output_fifo 
	sc_uint<flit_width>	output_data[2][5];   	// data to be written into fifo

        // signals between input port modules & output port modules
        // They must be implemented as inter-modules signals in the RTL
        bool            	output_get[2][5][5];    // output j consume data from input i in channel k 
        int	        	input_req[2][5];        // requested output port (NOP means no request)
	sc_uint<flit_width>	final_data[5];          // per input : data value 
	bool            	final_write[2][5];      // per input : data valid 

        ///////////////////////////////////////////////////////////////
        // output_get[k][i][j] : depends only on the output port states

        for(int k=0; k<2; k++)  // loop on channels
        {
            for(int i=0; i<5; i++)  // loop on inputs	
            {
                for(int j=0; j<5; j++)  // loop on outputs
                {
                    output_get[k][i][j] = (r_output_index[k][j].read() == i) && 
                                          r_output_alloc[k][j] && 
                                          out_fifo[k][j].wok();
                }
            }
        }

        ///////////////////////////////////////////////////////////
        // input_data[k][i], put[k][i] : input port local variables
        // input_req[k][i], final_write[k][i], final_data[i]

        for(int i=0; i<5; i++) // loop on input ports
        {
            // input_data[k][i] & put[k][i]
            for(int k=0; k<2; k++) // loop on channels
            {
                if( (r_input_fsm[k][i].read() == INFSM_REQ_SECOND) ||
                    (r_input_fsm[k][i].read() == INFSM_REQ_THIRD ) ||
                    (r_input_fsm[k][i].read() == INFSM_REQ_FOURTH) )
                {
                    input_data[k][i] = r_buf[k][i].read();
                    put[k][i] = true;
                }
                else
                {
                    input_data[k][i] = in_fifo[k][i].read();
                    put[k][i] = in_fifo[k][i].rok();
                }
            } // end loop channels 

            // Time multiplexing : final_write[k][i] & final_data[i]
            bool tdm = r_tdm[i].read();		// channel 1 has priority when tdm is true
            final_write[0][i] = put[0][i]&&(!tdm||!put[1][i]);
            final_write[1][i] = put[1][i]&&( tdm||!put[0][i]);
            if(final_write[1][i]) 		final_data[i] = input_data[1][i];
            else                        	final_data[i] = input_data[0][i];

            // input_req[k][i] : depends only on the input_fsm & input_fifo states
            for(int k=0; k<2; k++) // loop on channels
            {
                switch( r_input_fsm[k][i] ) {
                case INFSM_REQ_FIRST :
                    if( in_fifo[k][i].rok() )
                    {
                        if( is_broadcast(input_data[k][i]) )  // broadcast request
                        {
                            input_req[k][i] = broadcast_route(FIRST, i, input_data[k][i]);
                        }
                        else			             // not a broadcast request
                        {
                            input_req[k][i] = xfirst_route(input_data[k][i]);
                        }
                    }
                break;
                case INFSM_REQ_SECOND :
                    input_req[k][i] = broadcast_route(SECOND, i, input_data[k][i]);
                break;
                case INFSM_REQ_THIRD :
                    input_req[k][i] = broadcast_route(THIRD, i, input_data[k][i]);
                break;
                case INFSM_REQ_FOURTH :
                    input_req[k][i] = broadcast_route(FOURTH, i, input_data[k][i]);
                break;
                default :
                    input_req[k][i] = REQ_NOP;
                break;
                } // end switch r_input_fsm
            } // enf for channels
        } // end for input ports

        ///////////////////////////////////
        // input ports registers and fifos
        // r_tdm, r_input_fsm, r_buf, 
        // in_fifo_write, in_fifo_read

        for(int i=0; i<5; i++)  // loop on the input ports
        {
            r_tdm[i] = (r_tdm[i].read() && !put[1][i]) || (!r_tdm[i].read() && put[0][i]);

            for(int k=0; k<2; k++)  // loop on the channels
            {
                get[k][i] = ( output_get[k][i][0]) ||
                            ( output_get[k][i][1]) ||
                            ( output_get[k][i][2]) ||
                            ( output_get[k][i][3]) ||
                            ( output_get[k][i][4]);

                in_fifo_write[k][i] = p_in[k][i].write.read();

                switch( r_input_fsm[k][i] ) {
                case INFSM_REQ_FIRST:
                    if( in_fifo[k][i].rok() )
                    {
                        if( is_broadcast(input_data[k][i]) )  // broadcast request
                        {
                            in_fifo_read[k][i] = true;				
                            r_buf[k][i] = in_fifo[k][i].read();
                            if( input_req[k][i] == REQ_NOP )	r_input_fsm[k][i] = INFSM_REQ_SECOND;
                            else if( get[k][i] )  		r_input_fsm[k][i] = INFSM_DT_FIRST; 
                        }
                        else 				// not a broadcast request
                        {
                            in_fifo_read[k][i] = get[k][i];				
                            if( get[k][i] ) r_input_fsm[k][i] = INFSM_DTN;
                        }
                    } // end if rok()
                break;
                case INFSM_DTN:
                    in_fifo_read[k][i] = get[k][i];
	            if( get[k][i] && is_eop(input_data[k][i]) ) r_input_fsm[k][i] = INFSM_REQ_FIRST;
                break;
                case INFSM_DT_FIRST:		
                    in_fifo_read[k][i] = false;
                    if( get[k][i] )			r_input_fsm[k][i] = INFSM_REQ_SECOND;
                break;
                case INFSM_REQ_SECOND:
                    in_fifo_read[k][i] = false;
                    if( input_req[k][i] == REQ_NOP )	r_input_fsm[k][i] = INFSM_REQ_THIRD;
                    else if( get[k][i] ) 		r_input_fsm[k][i] = INFSM_DT_SECOND;
                break;
                case INFSM_DT_SECOND:	
                    in_fifo_read[k][i] = false;
                    if( get[k][i] )			r_input_fsm[k][i] = INFSM_REQ_THIRD;
                break;
                case INFSM_REQ_THIRD:
                    in_fifo_read[k][i] = false;
                    if( input_req[k][i] == REQ_NOP )	r_input_fsm[k][i] = INFSM_REQ_FOURTH;
                    if( get[k][i] ) 			r_input_fsm[k][i] = INFSM_DT_THIRD;
                break;
                case INFSM_DT_THIRD:	
                    in_fifo_read[k][i] = false;
                    if( get[k][i] )			r_input_fsm[k][i] = INFSM_REQ_FOURTH;
                break;
                case INFSM_REQ_FOURTH:
                    in_fifo_read[k][i] = false;
                    if( input_req[k][i] == REQ_NOP )	r_input_fsm[k][i] = INFSM_REQ_FIRST;
                    if( get[k][i] ) 			r_input_fsm[k][i] = INFSM_DT_FOURTH;
                break;
                case INFSM_DT_FOURTH:
                    if( get[k][i] )			r_input_fsm[k][i] = INFSM_REQ_FIRST;
                break;
                } // end switch infsm
            } // end for channels
        } // end for inputs

        ////////////////////////////////////////////////
        // output ports registers and fifos
        // r_output_index , r_output_alloc
        // out_fifo_read, out_fifo_write, output_data

        for(int j=0; j<5; j++) // loop on output ports
        {
            for(int k=0; k<2; k++) // loop on channels
            {
                // out_fifo_read[k][i], out_fifo_write[k][i], output_data[k][i]
                out_fifo_read[k][j] = p_out[k][j].read;
                for(int i=0; i<5; i++)  // loop on input ports
                {
                    if( output_get[k][i][j] && final_write[k][i] )
                    {
                        output_data[k][j] = final_data[i];
                        out_fifo_write[k][j] = true;
                    }
                    else
                    {
                        out_fifo_write[k][j] = false;
                    }
                }
                // r_output_alloc[k][j] & r_output_index[k][j]
                int index = r_output_index[k][j];
                if( !r_output_alloc[k][j] ) 		// allocation
                { 
                    for(int n = index+1; n < index+6; n++) // loop on input ports
                    { 
                        int x = n % 5;
                        if( input_req[k][x] == j ) 
                        {
                            r_output_index[k][j] = x;
                            r_output_alloc[k][j] = true;
                            break;
                        }
                    }
                }
                else if( is_eop(output_data[k][j]) && 
                         out_fifo_write[k][j] && 
                         out_fifo[k][j].wok() ) 	// de-allocation
                { 
                    r_output_alloc[k][index] = false;
                }
            } // end for channels
        } // end for outputs

        ////////////////////////
        //  Updating fifos
        for(int k=0; k<2; k++)
        {			
            for(int i=0; i<5; i++) 
            {
                {
                    if     ( (in_fifo_write[k][i]) && (in_fifo_read[k][i]) )
                        in_fifo[k][i].put_and_get(p_in[k][i].data.read());
                    else if( (in_fifo_write[k][i]) && (!in_fifo_read[k][i]) )
                        in_fifo[k][i].simple_put(p_in[k][i].data.read());
                    else if( (!in_fifo_write[k][i]) && (in_fifo_read[k][i]) )
                        in_fifo[k][i].simple_get();

                    if     ( (out_fifo_write[k][i]) && (out_fifo_read[k][i]) )
                        out_fifo[k][i].put_and_get(output_data[k][i]);
                    else if( (out_fifo_write[k][i]) && (!out_fifo_read[k][i]) )
                        out_fifo[k][i].simple_put(output_data[k][i]);
                    else if( (!out_fifo_write[k][i]) && (out_fifo_read[k][i]) )
                        out_fifo[k][i].simple_get();
                }
            } 
        } 
    } // end transition

    //////////////////////
    tmpl(void)::genMoore()
    {
        for(int k=0; k<2; k++)
        {
            for(int i=0; i<5; i++)
            { 
                {
                    p_in[k][i].read   = in_fifo[k][i].wok();
                    p_out[k][i].write = out_fifo[k][i].rok();
                    p_out[k][i].data  = out_fifo[k][i].read();
                }
            }
        }
    } // end genMoore

}} // end namespaces 
