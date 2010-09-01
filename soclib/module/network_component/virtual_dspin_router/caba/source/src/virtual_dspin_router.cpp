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
 * Date     : august 2010
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

    ///////////////////////////////////////////////////////////////////////////
    // Implementation Note :
    // The TDM (Time dependant Multiplexing) introduces a strong coupling
    // between the two FSMs controling the two virtual channels 
    // in the input port. Therefore, it is necessary to have two successive
    // loops one the 2 * 5 input FSMs:
    // - The first loop computes - put[k][i] : input(k,i) wishes to produce
    //                           - get[k][i] : output(k,i) wishes to consume
    // - The second loop uses these values to implement the TDM policy
    //   and compute final_put[k][i], final_data[k][i], in_fifo_read[k][i]
    //   and the next FSM state r_input_fsm[k][i].
    // In this implementation, there is only one comrbinational transfer 
    // per cycle from input(i) to output(j) or from output(j) to input(i).
    ///////////////////////////////////////////////////////////////////////////
    // The four functions xfirst_route(), broadcast_route() 
    // is_eop() and is_broadcast() defined below are used 
    // to decode the DSPIN first flit format:
    // - In case of a non-broadcast packet :
    //  |EOP|   X     |   Y     |---------------------------------------|BC |
    //  | 1 | x_width | y_width |  flit_width - (x_width + y_width + 2) | 1 |
    //
    //  - In case of a broacast 
    //  |EOP|  XMIN   |  XMAX   |  YMIN   |  YMAX   |-------------------|BC |
    //  | 1 |   5     |   5     |   5     |   5     | flit_width - 22   | 1 |
    ///////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////
    tmpl(void)::printTrace(int channel)
    {
        int k = channel%2;
        std::cout << "-- " << name() << " : channel " << k << std::endl;
        for( size_t i=0 ; i<5 ; i++)  // loop on input ports
        {
            std::cout << "input[" << i << "] state = " << r_input_fsm[k][i];
            if( in_fifo[k][i].rok() ) std::cout << " / dtin = " << std::hex << in_fifo[k][i].read();
            std::cout << std::endl;
        }
        for( size_t i=0 ; i<5 ; i++)  // loop on output ports
        {
            std::cout << "output[" << i << "] alloc = " << r_output_alloc[k][i]
                      << " / index = " <<  r_output_index[k][i] << std::endl;
        }
    } 

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
        int xmin = (data >> (flit_width - 6 )) & 0x1F;
        int xmax = (data >> (flit_width - 11)) & 0x1F;
        int ymin = (data >> (flit_width - 16)) & 0x1F;
        int ymax = (data >> (flit_width - 21)) & 0x1F;

        switch(source) {
        case LOCAL :
            if      ( iter == 1 )	sel = REQ_NORTH;
            else if ( iter == 2 )	sel = REQ_SOUTH;
            else if ( iter == 3 )	sel = REQ_EAST;
            else if ( iter == 4 )	sel = REQ_WEST;
        break;
        case NORTH :
            if      ( iter == 1 )	sel = REQ_SOUTH;
            else if ( iter == 2 )	sel = REQ_LOCAL;
            else if ( iter == 3 )	sel = REQ_NOP;
            else if ( iter == 4 )	sel = REQ_NOP;
        break;
        case SOUTH :
            if      ( iter == 1 )	sel = REQ_NORTH;
            else if ( iter == 2 )	sel = REQ_LOCAL;
            else if ( iter == 3 )	sel = REQ_NOP;
            else if ( iter == 4 )	sel = REQ_NOP;
        break;
        case EAST :
            if      ( iter == 1 )	sel = REQ_WEST;
            else if ( iter == 2 )	sel = REQ_NORTH;
            else if ( iter == 3 )	sel = REQ_SOUTH;
            else if ( iter == 4 )	sel = REQ_LOCAL;
        break;
        case WEST :
            if      ( iter == 1 )	sel = REQ_EAST;
            else if ( iter == 2 )	sel = REQ_NORTH;
            else if ( iter == 3 )	sel = REQ_SOUTH;
            else if ( iter == 4 )	sel = REQ_LOCAL;
        break;
        }
        if      ( (sel == REQ_NORTH) && !(m_local_y < ymax) ) 	sel = REQ_NOP;
        else if ( (sel == REQ_SOUTH) && !(m_local_y > ymin) ) 	sel = REQ_NOP;
        else if ( (sel == REQ_EAST ) && !(m_local_x < xmax) ) 	sel = REQ_NOP;
        else if ( (sel == REQ_WEST ) && !(m_local_x > xmin) ) 	sel = REQ_NOP;
        return sel;
    }

    ///////////////////////////////////////////////////
    tmpl(inline bool)::is_eop(sc_uint<flit_width> data)
    {
        return ( ((data>>(flit_width-1)) & 0x1) != 0);
    }

    /////////////////////////////////////////////////////////
    tmpl(inline bool)::is_broadcast(sc_uint<flit_width> data)
    {
        return ( (data & 0x1) != 0);
    }

    ////////////////////////////////////////////////////////////
    tmpl(/**/)::VirtualDspinRouter(	sc_module_name 	insname, 
					int 	x,
					int 	y,
					int	x_width,
					int	y_width,
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

        // maximal width of the x & y fields (to support limited broadcast)
        if ( (x_width > 5) || (y_width > 5) )
        {
            std::cout << "Error in the virtual_dspin_router" << name() << std::endl;
            std::cout << "The x_width & y_width parameters cannot be larger than 5" << std::endl;
            exit(0);
        }

        // minimal width of a flit
        if ( flit_width < 22 )
        {
            std::cout << "Error in the virtual_dspin_router" << name() << std::endl;
            std::cout << "The flit_width cannot be smaller than 22 bits" << std::endl;
            exit(0);
        }

        // ports
        p_in  = alloc_elems<DspinInput<flit_width> >("p_in", 2, 5);
        p_out = alloc_elems<DspinOutput<flit_width> >("p_out", 2, 5);
			
        // input & output fifos 
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
        m_x_width		= x_width;
        m_y_width		= y_width;
        m_x_shift		= flit_width - x_width - 1;
        m_y_shift		= flit_width - x_width - y_width - 1;
        m_x_mask		= (0x1 << x_width) - 1;
        m_y_mask		= (0x1 << y_width) - 1;

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
                    r_input_fsm[k][i]		= INFSM_IDLE;			
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
        bool	 		in_fifo_read[2][5];	// wishes to consume data in in_fifo 
        bool	       		in_fifo_write[2][5];	// writes data in in_fifo
        bool 			put[2][5];		// input port wishes to transmit data
        bool			get[2][5];		// selected output port wishes to consume data

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
	bool            	final_put[2][5];        // per input : data valid 

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

        ///////////////////////////////////////////////////////////////////
        //  The following signals depend on the input FSM & FIFO states
        //  but do not depent on r_tdm :
        //  in_fifo_write[k][i], input_req[k][i], get[k][i], put[k][i]

        for(int i=0; i<5; i++)  // loop on the input ports
        {
            for(int k=0; k<2; k++)  // loop on the channels
            {
                get[k][i] = ( output_get[k][i][0]) ||
                            ( output_get[k][i][1]) ||
                            ( output_get[k][i][2]) ||
                            ( output_get[k][i][3]) ||
                            ( output_get[k][i][4]);

                in_fifo_write[k][i] = p_in[k][i].write.read();

                switch( r_input_fsm[k][i] ) {
                case INFSM_IDLE:
                    put[k][i] = false;
                    if( in_fifo[k][i].rok() )
                    {
                        if( is_broadcast(in_fifo[k][i].read()) )  
                            input_req[k][i] = broadcast_route(1, i, in_fifo[k][i].read());
                        else 
                            input_req[k][i] = xfirst_route(in_fifo[k][i].read());
                    }
                    else    input_req[k][i] = REQ_NOP;
                    break;
                case INFSM_REQ:
                    put[k][i] = true;
                    input_req[k][i] = xfirst_route(in_fifo[k][i].read());
                    break;
                case INFSM_DT:
                    put[k][i] = in_fifo[k][i].rok();
                    input_req[k][i] = REQ_NOP;
                    break;
                case INFSM_REQ_FIRST:
                    put[k][i] = true;
                    input_req[k][i] = broadcast_route(1, i, r_buf[k][i].read() );
                    break;
                case INFSM_REQ_SECOND:
                    put[k][i] = true;
                    input_req[k][i] = broadcast_route(2, i, r_buf[k][i].read() );
                    break;
                case INFSM_REQ_THIRD:
                    put[k][i] = true;
                    input_req[k][i] = broadcast_route(3, i, r_buf[k][i].read() );
                    break;
                case INFSM_REQ_FOURTH:
                    put[k][i] = true;
                    input_req[k][i] = broadcast_route(4, i, r_buf[k][i].read() );
                    break;
                case INFSM_DT_FIRST:	
                case INFSM_DT_SECOND:	
                case INFSM_DT_THIRD:	
                case INFSM_DT_FOURTH:
                    put[k][i] = in_fifo[k][i].rok();
                    input_req[k][i] = REQ_NOP;
                    if( in_fifo[k][i].rok() && !is_eop(in_fifo[k][i].read()) )
                    {
                        std::cout << "Error in the virtual_dspin_router " << name() << std::endl;
                        std::cout << "Broadcast packet longer than 2 flits received on input port["
                                  << k << "][" << i << "]" << std::endl;
                        exit(0);
                    }
                    break;
                } // end switch infsm
            } // end for channels
        } // end for inputs

/*
if((k==0) && (i==4)) std::cout << "rok[" << i << "] = " << in_fifo[k][i].rok() << std::endl << std::hex;
if((k==0) && (i==4)) std::cout << "din[" << i << "] = " << in_fifo[k][i].read() << std::endl;
if((k==0) && (i==4)) std::cout << "req[" << i << "] = " << input_req[k][i] << std::endl;
if((k==0) && (i==4)) std::cout << "put[" << i << "] = " << put[k][i] << std::endl;
if((k==0) && (i==4)) std::cout << "get[" << i << "] = " << get[k][i] << std::endl << std::endl;
*/
        ////////////////////////////////////////////////////////////////
        // Time multiplexing in input ports : 
        // final_put[k][i] & final_data[i], final_fifo_read[k][i],
        // and r_input_fsm[k][i] depend on r_tdm.

        for(size_t i=0; i<5; i++) // loop on input ports
        {
            // Virtual channel 1 has priority when r_tdm is true
            // The r_tdm[i] flip-flop toggle each time the owner uses the physical channel
            r_tdm[i] = (r_tdm[i].read() && !put[1][i]) || (!r_tdm[i].read() && put[0][i]);

            for(size_t k=0 ; k<2 ; k++)
            {
                // A virtual channel [k][i] is tdm_ok if he has the priority (r_tdm[i] value),
                // or if the other channel does not use the physical channel (put[nk][i] == false) 
                bool tdm_ok;
                if( k == 0 ) 	tdm_ok = !r_tdm[i].read() || !put[1][i];
                else        	tdm_ok =  r_tdm[i].read() || !put[0][i];
                
                switch( r_input_fsm[k][i] ) {
                case INFSM_IDLE:	// does not depend on tdm in IDLE state
                    final_put[k][i] = false;
                    if( in_fifo[k][i].rok() )
                    {
                        if( is_broadcast(in_fifo[k][i].read()) )  // broadcast request
                        {
                            in_fifo_read[k][i] = true;
                            r_buf[k][i] = in_fifo[k][i].read();
                            if( input_req[k][i] == REQ_NOP )	r_input_fsm[k][i] = INFSM_REQ_SECOND;
                            else                  		r_input_fsm[k][i] = INFSM_REQ_FIRST; 
                        }
                        else 				// not a broadcast request
                        {
                            in_fifo_read[k][i] = false;
                            r_input_fsm[k][i] = INFSM_REQ;
                        }
                    }
                    break;
                case INFSM_REQ:
                    in_fifo_read[k][i] = get[k][i] && tdm_ok;
                    final_put[k][i] =  tdm_ok;
                    if ( get[k][i] && tdm_ok )
                    {
                        final_data[i] = in_fifo[k][i].read();
	                if( is_eop(in_fifo[k][i].read()) )	r_input_fsm[k][i] = INFSM_IDLE;
                        else 					r_input_fsm[k][i] = INFSM_DT;	
                    }
                    break;
                case INFSM_DT:
                    in_fifo_read[k][i] = get[k][i] && tdm_ok;
                    final_put[k][i] = put[k][i] && tdm_ok;
                    if ( get[k][i] && put[k][i] && tdm_ok )
                    {
                        final_data[i] = in_fifo[k][i].read();
			if( is_eop(in_fifo[k][i].read()) ) r_input_fsm[k][i] = INFSM_IDLE;
                    } 
                    break;
                case INFSM_REQ_FIRST:
                    in_fifo_read[k][i] = false;
                    if( input_req[k][i] == REQ_NOP )		
                    {
                        final_put[k][i] = false;
	                r_input_fsm[k][i] = INFSM_REQ_SECOND;
                    }
                    else
                    {
                        final_put[k][i] = tdm_ok;
                        if( get[k][i] && tdm_ok )	
                        {
                            final_data[i] = r_buf[k][i].read();
                            r_input_fsm[k][i] = INFSM_DT_FIRST;
                        }
                    }
                    break;
                case INFSM_DT_FIRST:		
                    in_fifo_read[k][i] = false;
                    final_put[k][i] = put[k][i] && tdm_ok;
                    if( get[k][i] && put[k][i] && tdm_ok )
                    {
                        final_data[i] = in_fifo[k][i].read();
                        r_input_fsm[k][i] = INFSM_REQ_SECOND;
                    }
                    break;
                case INFSM_REQ_SECOND:
                    in_fifo_read[k][i] = false;
                    if( input_req[k][i] == REQ_NOP )		
                    {
                        final_put[k][i] = false;
                        r_input_fsm[k][i] = INFSM_REQ_THIRD;
                    }
                    else
                    {
                        final_put[k][i] = tdm_ok;
                        if( get[k][i] && tdm_ok )
                        {
                            final_data[i] = r_buf[k][i].read();
                            r_input_fsm[k][i] = INFSM_DT_SECOND;
                        }
                    }
                    break;
                case INFSM_DT_SECOND:	
                    in_fifo_read[k][i] = false;
                    final_put[k][i] = put[k][i] && tdm_ok;
                    if( get[k][i] && put[k][i] && tdm_ok )
                    {
                        final_data[i] = in_fifo[k][i].read();
                        r_input_fsm[k][i] = INFSM_REQ_THIRD;
                    }
                    break;
                case INFSM_REQ_THIRD:
                    in_fifo_read[k][i] = false;
                    if( input_req[k][i] == REQ_NOP )		
                    {
                        final_put[k][i] = false;
                        r_input_fsm[k][i] = INFSM_REQ_FOURTH;
                    }
                    else
                    {
                        final_put[k][i] = tdm_ok;
                        if( get[k][i] && tdm_ok )
                        {
                            final_data[i] = r_buf[k][i].read();
                            r_input_fsm[k][i] = INFSM_DT_THIRD;
                        }
                    }
                    break;
                case INFSM_DT_THIRD:	
                    in_fifo_read[k][i] = false;
                    final_put[k][i] = put[k][i] && tdm_ok;
                    if( get[k][i] && put[k][i] && tdm_ok )
                    {
                        final_data[i] = in_fifo[k][i].read();
                        r_input_fsm[k][i] = INFSM_REQ_FOURTH;
                    }
                    break;
                case INFSM_REQ_FOURTH:
                    if( input_req[k][i] == REQ_NOP )		
                    {
                        in_fifo_read[k][i] = true;
                        final_put[k][i] = false;
                        r_input_fsm[k][i] = INFSM_IDLE;
                    }
                    else
                    {
                        in_fifo_read[k][i] = false;
                        final_put[k][i] = tdm_ok;
                        if( get[k][i] && tdm_ok )
                        {
                            final_data[i] = r_buf[k][i].read();
                            r_input_fsm[k][i] = INFSM_DT_FOURTH;
                        }
                    }
                    break;
                case INFSM_DT_FOURTH:
                    in_fifo_read[k][i] = get[k][i] && tdm_ok;
                    final_put[k][i] = put[k][i] && tdm_ok;
                    if( get[k][i] && put[k][i] && tdm_ok )
                    {
                        final_data[i] = in_fifo[k][i].read();
                        r_input_fsm[k][i] = INFSM_IDLE;
                    }
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
                out_fifo_write[k][j] = (output_get[k][0][j] && final_put[k][0]) ||
                                       (output_get[k][1][j] && final_put[k][1]) ||
                                       (output_get[k][2][j] && final_put[k][2]) ||
                                       (output_get[k][3][j] && final_put[k][3]) ||
                                       (output_get[k][4][j] && final_put[k][4]) ;
                for(int i=0; i<5; i++)  // loop on input ports
                {
                    if( output_get[k][i][j] ) 	output_data[k][j] = final_data[i];
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
                    r_output_alloc[k][j] = false;
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
