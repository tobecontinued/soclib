/* -*- c++ -*-
  *
  * File : dspin_local_crossbar.cpp
  * Copyright (c) UPMC, Lip6
  * Authors : Alain Greiner
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
  */

#include "../include/dspin_local_crossbar.h"

namespace soclib { namespace caba {

#define tmpl(x) template<size_t flit_width> x DspinLocalCrossbar<flit_width>

    //////////////////////////////////////////////////////////
    //                  constructor
    //////////////////////////////////////////////////////////
    tmpl(/**/)::DspinLocalCossbar( sc_module_name       name, 
                                   const MappingTable   &mt
                                   const size_t         x,
                                   const size_t         y,
                                   const size_t         x_width,
                                   const size_t         y_width,
                                   const size_t         l_width,
                                   const size_t         nb_local_inputs,
                                   const size_t         nb_local_outputs,
                                   const size_t         in_fifo_depth,
                                   const size_t         out_fifo_depth,
                                   const bool           use_routing_table,
                                   const bool           broadcast_supported )
	: soclib::caba::BaseModule(name),

      p_clk("p_clk"),
      p_resetn("p_resetn"),
      p_global_in("p_global_in"),
      p_global_out("p_global_out"),
      p_local_in(soclib::common::alloc_elems<DspinInput<flit_width>
          ("p_local_in", nb_local_inputs)),
      p_local_out(soclib::common::alloc_elems<DspinOutput<flit_width>
          ("p_local_out", nb_local_outputs)),

      m_local_x( x ),
      m_local_y( y ),
      m_x_width( x_width ),
      m_x_shift( flit_width - x_width ),
      m_x_mask( (0x1 << x_width) - 1 ),
      m_y_width( y_width ),
      m_y_shift( flit_width - x_width - y_width ),
      m_y_mask( (0x1 << y_width) - 1 ),
      m_l_width( l_width ),
      m_l_shift( flit_width - x_width - y_width - l_width ),
      m_l_mask( (0x1 << l_width) - 1 ),
      m_local_inputs( nb_local_inputs ),
      m_local_outputs( nb_local_outputs ),
      m_routing_table( mt.getRoutingTable(IntTab((x << x_width) + y)) ),
      m_use_routing_table( use_routing_table ),
      m_broadcast_supported( broadcast_supported ),

      r_alloc_out(soclib::common::alloc_elems<sc_signal<bool> >
          ("r_alloc_out", m_local_outputs + 1)),
	  r_index_out(soclib::common::alloc_elems<sc_signal<size_t> >
          ("r_index_out", m_local_outputs + 1)),
	  r_buf_in(soclib::common::alloc_elems<sc_signal<<sc_uint<flit_size> > >
          ("r_buf_in",  m_local_inputs + 1)),
	  r_fsm_in(soclib::common::alloc_elems<sc_signal<int> >
          ("r_fsm_in",  m_local_inputs + 1)),
	  r_index_in(soclib::common::alloc_elems<sc_signal<size_t> >
          ("r_index_in",  m_local_inputs + 1))
    {
	    SC_METHOD (transition);
	    dont_initialize();
	    sensitive << p_clk.pos();

   	    SC_METHOD (genMoore);
	    dont_initialize();
	    sensitive  << p_clk.neg();

        // construct FIFOs
	    r_fifo_in  = (soclib::caba::GenericFifo<sc_uint<flit_width> >*)
	    malloc(sizeof(soclib::caba::GenericFifo<sc_uint<flit_width> >)*(m_local_inputs+1));
	    
        r_fifo_out = (soclib::caba::GenericFifo<sc_uint<flit_width> >*)
	    malloc(sizeof(soclib::caba::GenericFifo<sc_uint<flit_width> >)*(m_local_outputs+1));

	    for( int i = 0 ; i <= m_local_inputs ; i++ )
        {
		    std::ostringstream str;
		    str << "r_in_fifo_" << i;
	        new(&r_fifo_in[i])  
                soclib::caba::GenericFifo<sc_uint<flit_width> >(str.str(), in_fifo_depth);
        }

	    for( int j = 0 ; j <= m_local_outputs ; j++ )
        {
		    str << "r_out_fifo_" << j;
	        new(&r_fifo_out[j]) 
                soclib::caba::GenericFifo<sc_uint<flit_width> >(str.str(), out_fifo_depth);
	    }

	} //  end constructor

    ////////////////////////////////////////////////////////////////////////////
    tmpl(size_t)::route( sc_uint<flit_width> data,     // first flit
                         size_t              index )   // input port index 
    {
        // extract address from first flit        
        sc_uint<flit_width> address = data << 1;

        size_t x_dest = (size_t)(address >> m_x_shift) & m_x_mask;
        size_t y_dest = (size_t)(address >> m_y_shift) & m_y_mask;
        size_t l_dest = (size_t)(address >> m_l_shift) & m_l_mask;
        
        if ( index < m_local_inputs )      // local input port
        {
            if ( (x_dest == m_local_x) and (y_dest == m_local_y) )  // local dest
            {
                if ( m_use_routing_table )  return m_routing_table[address];
                else                        return l_dest;
            }
            else                                                    // global dest
            {
                return m_local_outputs;
            }
        }
        else                                // global input port
        {
            assert( (x_dest == m_local_x) and (y_dest == m_local_y) and
            "ERROR in DSPIN_LOCAL_CROSSBAR : illegal packet received on global input");

            if ( m_use_routing_table ) return m_routing_table[address];
            else                       return l_dest;
        }
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

    /////////////////////////
    tmpl(void)::print_trace()
    {
        std::cout << "DSPIN_LOCAL_CROSSBAR " << name() << std::hex; 
        for( size_t out=0 ; out<m_local_outputs ; out++)  // loop on output ports
        {
            if ( r_output_alloc[out].read() )
            {
                size_t in = r_output_index[out];
                std::cout << " / in[" << in << "] -> out[" << out << "]";
            }   
        }
        std::cout << std::endl;
    }

    /////////////////////////
    tmpl(void)::transition()
    {
        // Long wires connecting input and output ports
        size_t              req_in[m_local_inputs+1];   // input ports  -> output ports
        size_t              get_out[m_local_outputs+1]; // output ports -> input ports
        bool                put_in[m_local_inputs+1];   // input ports  -> output ports
        sc_uint<flit_width> data_in[m_local_inputs+1];  // input ports  -> output ports

        // control signals for the input fifos
	    bool                fifo_in_write[m_local_inputs+1];
	    bool                fifo_in_read[m_local_inputs+1];	
	    sc_uint<flit_width> fifo_in_data[m_local_inputs+1];

        // control signals for the output fifos
	    bool                fifo_out_write[m_local_outputs+1];
	    bool                fifo_out_read[m_local_outputs+1];
	    sc_uint<flit_width> fifo_out_data[m_local_outputs+1];

	    // reset 
	    if ( p_resetn.read() == false ) 
        {
	        for(size_t j = 0 ; j <= m_local_outputs ; j++) 
            {
		        r_alloc_out[j] = false;
		        r_index_out[j] = 0;
		        r_fifo_out[i].init();
            }
	        for(size_t i = 0 ; i <= m_local_inputs ; i++) 
            {
		        r_index_in[i]  = 0;
		        r_fsm_in[i]    = INFSM_IDLE;
		        r_fifo_in[i].init();
	        }
            return;
        }

	    // fifo_in control signals
	    for(size_t i = 0 ; i < m_local_inputs ; i++) 
        {
		    fifo_in_read[i]  = false;   // default value
		    fifo_in_write[i] = p_local_in[i].write.read();
		    fifo_in_wdata[i] = p_local_in[i].data.read();
	    }
        fifo_in_read[m_local_inputs]  = false; // default value
        fifo_in_write[m_local_inputs] = p_global_in.write.read();
        fifo_in_wdata[m_local_inputs] = p_global_in.wdata.read();

	    // fifo_out control signals
	    for(size_t j = 0 ; j < m_local_outputs ; j++) 
        {
		    fifo_out_read[j]  = p_local_out[j].read.read();
		    fifo_out_write[j] = false;  // default value
		    fifo_out_wdata[j] = 0;      // default value
	    }
        fifo_out_read[m_local_outputs]  = p_global_out.read.read();
        fifo_out_write[m_local_outputs] = false;  // default value    
        fifo_out_wdata[m_local_outputs] = 0;      // default value

        // loop on the input ports (including global input port, 
        // with the convention index[global] = m_local_inputs)
        // The port state is defined by r_fsm_in[i], r_index_in[i] 
        // The req_in[i] computation uses the route() function.
        // Both put_in[i] and req_in[i] depend on the input port state.

        for ( size_t i = 0 ; i <= m_local_inputs ; i++ )
        {
            switch ( r_fsm_in[i].read() )
            {
                case INFSM_IDLE:    // no output port allocated
                {
                    put_in[i] = false;
                    if ( r_fifo_in[i].rok() ) // packet available in input fifo
                    {
                        if ( is_broadcast(r_fifo_in[i].read()) and 
                             m_broadcast_supported )   // broadcast required
                        {
                            fifo_in_read[i] = true;
                            r_buf_in[i]     = r_fifo_in[i].read();
                            if ( i == m_local_inputs ) // global input port
                            {
                                req_in[i]     = m_local_outputs - 1;
                            }
                            else                       // local input port
                            {
                                req_in[i]     = m_local_outputs;
                            }
                            r_index_in[i] = req_in[i];
                            r_fsm_in[i]   = INFSM_REQ_BC;
                        }
                        else                           // unicast routing
                        {
                            req_in[i]     = route( r_fifo_in[i].read(), i );
                            r_index_in[i] = req_in[i];
                            r_fsm_in[i]   = INFSM_REQ;
                        }
                    }
                    else
                    {
                        req_in[i]     = 0xFFFFFFFF;  // no request
                    }
                    break;
                }
                case INFSM_REQ:   // waiting output port allocation
                {
                    data_in[i] = r_fifo_in[i].read();
                    put_in[i]  = r_fifo_in[i].rok();
                    req_in[i]  = r_index_in[i];
                    if ( get_out[r_index_in[i].read()].read() ) // first flit transfered
                    {
                        r_fsm_in[i]   = INFSM_ALLOC
                    }
                    break;
                }
                case INFSM_ALLOC:  // output port allocated
                {
                    data_in[i] = r_fifo_in[i].read();
                    put_in[i]  = r_fifo_in[i].rok();
                    req_in[i]  = 0xFFFFFFFF;                // no request
                    if ( is_eop( r_fifo_in[i].read() ) and
                         r_fifo_in[i].rok() and 
                         get_out[r_index_in[i].read()] )        // last flit transfered
                    {
                        r_fsm_in[i] = INFSM_IDLE;
                    }
                    break;
                }
                case INFSM_REQ_BC:  // waiting output port allocation
                {
                    data_in[i] = r_buf_in[i].read();
                    put_in[i]  = true;
                    req_in[i]  = r_index_in[i];
                    if ( get_out[r_index_in[i].read()].read() ) // first flit transfered
                    {
                        r_fsm_in[i] = INFSM_ALLOC_BC;
                        r_index_in[i] = r_index_in[i].read() - 1;
                    }
                    break;
                }
                case INFSM_ALLOC_BC:  // waiting output port allocation
                {
                    data_in[i] = r_fifo_in[i].read();
                    put_in[i]  = r_fifo_in[i].rok();
                    req_in[i]  = 0xFFFFFFFF;                // no request
                    if ( is_eop( r_fifo_in[i].read() ) and
                         r_fifo_in[i].rok() and 
                         get_out[r_index_in[i].read()] )  // last flit transfered
                    {
                        if ( r_index_in[i].read() == 0 ) r_fsm_in[i] = INFSM_IDLE;
                        else                             r_fsm_in[i] = INFSM_REQ_BC;
                    }
                    break;
                }
            } // end switch
        } // end for input ports
                                   
        // loop on the output ports (including global output port, 
        // with the convention index[global] = m_local_outputs)
	    // The r_alloc_out[j] and r_index_out[j] computation
        // implements the round-robin allocation policy.
        // These two registers implement a 10 states FSM.
        // The get_out[j] computation combines fifo_out_wok[j] and r_alloc_out[j]
	    for( size_t j = 0 ; j <= m_local_outputs ; j++ ) 
        {
		    if( not r_alloc_out[j].read() )  // not allocated: possible new allocation
            {
		        for( size_t k = r_index_out[j].read() + 1 ; 
                     k <= (r_index_out[j].read() + m_local_inputs + 1) ; 
                     k++ ) 
                { 
			        i = k % (m_local_inputs + 1);
			        if( req[i] == j ) 
                    {
			            r_alloc_out[j] = true;
			            r_index_out[j] = i;
                    }
			        break;
		        } // end loop on input ports

                // get_out[j] computation
                get_out[j] = 0xFFFFFFFF;    // no allocation
		    } 
            else                            // allocated: possible desallocation
            {
		        if ( is_eop(fifo_in[r_index_out[j]].read()) and
                     r_fifo_out[j].wok() and r_fifo_in[i].rok() ) 
                {
			        r_alloc_out[j] = false;
                }
                
                // get_out[j] computation
                if ( fifo_out[j].wok() ) get_out[j] = r_index_out[j].read();
                else                     get_out[j] = 0xFFFFFFFF;  
		    }
		} // end loop on output ports

        // loop on input ports :
	    // fifo_in_read[i] computation
	    for( size_t i = 0 ; i <= m_local_inputs ; i++ ) 
        {
		    if ( r_fsm_in[i].read() != INFSM_IDLE ) 
            {
                fifo_in_read[i] = (get_out[r_index_in[i].read()] == i);
            }
	    }  // end loop on input ports

        // loop on the output ports :
        // The fifo_out_write[j] and fifo_out_wdata[j] computation
        // implements the output port mux
	    for( size_t j = 0 ; j <= m_local_outputs ; j++ ) 
        {
		    if( r_alloc_out[j] )  // output port allocated
            {
		        fifo_out_write[j] = put_in[r_index_out[j]].read();
		        fifo_out_wdata[j] = data_in[r_index_out[j]].read();
            }
        }  // end loop on the output ports

	    //  input FIFOs update
	    for(size_t i = 0 ; i <= m_local_inputs ; i++) 
        {
		    r_fifo_in[i].update(fifo_in_read[i],
                                fifo_in_write[i],
                                fifo_in_wdata[i]);
        }

	    //  output FIFOs update
	    for(size_t j = 0 ; j <= m_local_outputs ; j++)
        { 
		    r_fifo_out[j].update(fifo_out_read[j],
                                 fifo_out_write[j],
                                 fifo_out_wdata[j]);
	    }
    } // end transition

    ///////////////////////
    tmpl(void)::genMoore()
    {
        for(size_t i = 0 ; i < m_local_inputs ; i++) 
        { 
	        p_local_in[i].read = fifo_in[i].wok();
        }
        p_global_in.read = fifo_in[m_local_inputs].wok();

        for(size_t j = 0 ; j < m_local_outputs ; j++) 
        { 
	        p_local_out[j].write = fifo_out[j].rok();
	        p_local_out[j].data  = fifo_out[j].read();
        }
        p_global_out.write = fifo_out[m_local_outputs].rok();
        p_global_out.data  = fifo_out[m_local_outputs].read();

    } // end genMoore

}} // end namespace

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
