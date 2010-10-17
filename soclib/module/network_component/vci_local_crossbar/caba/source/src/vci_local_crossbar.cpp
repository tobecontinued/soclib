/*
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
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2008
 *
 * Based on previous works by Alain Greiner, 2005
 *
 * Maintainers: nipo, alain
 */

#include <systemc>
#include <cassert>
#include "vci_buffers.h"
#include "../include/vci_local_crossbar.h"
#include "alloc_elems.h"

//#define CROSSBAR_DEBUG

namespace soclib { namespace caba {

using soclib::common::alloc_elems;
using soclib::common::dealloc_elems;

using namespace sc_core;

namespace _local_crossbar {

///////////////////////////////////////
template<typename pkt_t> class Crossbar
{
    typedef typename pkt_t::routing_table_t routing_table_t;
    typedef typename pkt_t::locality_table_t locality_table_t;
    typedef typename pkt_t::input_port_t input_port_t;
    typedef typename pkt_t::output_port_t output_port_t;

    const size_t 	m_in_size;
    const size_t 	m_out_size;
    bool*		m_allocated;
    size_t*		m_origin;
    const 		routing_table_t m_rt;
    const 		locality_table_t m_lt;
    const size_t 	m_non_local_target;
    bool 		m_broadcast_waiting;

public:
    /////////
    Crossbar(
	size_t in_size, size_t out_size,
	const routing_table_t &rt,
	const locality_table_t &lt,
	const size_t non_local_target )
	: m_in_size(in_size),
	  m_out_size(out_size),
	  m_allocated(new bool[m_out_size]),
	  m_origin(new size_t[m_out_size]),
	  m_rt(rt),
	  m_lt(lt),
       	  m_non_local_target(non_local_target)
	{
        reset();
	}

    ////////////
    void reset()
    {
	for (size_t i=0; i<m_out_size; ++i) 
        {
	m_allocated[i] = false;
	m_origin[i] = 0;
	m_broadcast_waiting = false;
	}
    }

    //////////////////
    void print_trace()
    {
        for( size_t n=0 ; n<m_out_size ; n++)
        {
            if( m_allocated[n] ) std::cout << std::dec 
                                           << " output " 
                                           << n 
                                           << " allocated to input " 
                                           << m_origin[n] 
                                           << " /";
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void transition( input_port_t **input_port, output_port_t **output_port )
    {
        if(!m_broadcast_waiting){
            size_t in = m_in_size -1;
            if(input_port[in]->getVal() && !input_port[in]->iAccepted()){ // second part is here to avoid resending the broadcast once every one has acknowledged
                pkt_t tmp;
                tmp.readFrom(*input_port[in]);
                if(tmp.is_broadcast()){
                    m_broadcast_waiting = true;
#ifdef CROSSBAR_DEBUG
                    std::cout << "Crossbar broadcast received " << std::endl;
#endif
                }
            }
        }
#ifdef CROSSBAR_DEBUG
        if(m_broadcast_waiting){
                    std::cout << "Crossbar, broadcast waiting ! " << std::endl;
        }
#endif
        for( size_t out = 0; out < m_out_size; out++) {
            if (m_allocated[out]) {
                if( output_port[out]->toPeerEnd() )
                    m_allocated[out] = false;
            } else {
                if(m_broadcast_waiting && (out!=(m_out_size-1)) ){
#ifdef CROSSBAR_DEBUG
                    std::cout << "Crossbar, broadcast, allocating output " << std::dec << out << std::endl;
#endif
                    m_allocated[out] = true;
                    m_origin[out] = m_in_size -1;
                }
                for(size_t _in = 0; _in < m_in_size; _in++) {
                    size_t in = (_in + m_origin[out] + 1) % m_in_size;
					if (input_port[in]->getVal()) {
						pkt_t tmp;
						tmp.readFrom(*input_port[in]);

                        if ( ( ! tmp.isLocal(m_lt) && out == m_non_local_target && !tmp.is_broadcast()) ||
                             ( tmp.is_broadcast() && out == m_non_local_target && in != (m_in_size-1) ) ||
                             ( tmp.isLocal(m_lt) && (size_t)tmp.route(m_rt) == out && !tmp.is_broadcast() ) ) {
                            m_allocated[out] = true;
                            m_origin[out] = in;
                            break;
                        }
					}
				}
			}
		}
    }

    ///////////////////////////////////////////////////////////////////////
    void genMealy( input_port_t **input_port, output_port_t **output_port )
    {
        bool ack[m_in_size];
        for( size_t in = 0; in < m_in_size; in++)
            ack[in] = false;
		for( size_t out = 0; out < m_out_size; out++) {
			if (m_allocated[out]) {
				size_t in = m_origin[out];
                // transfer only if it is not a broadcast
                if((in!=m_in_size-1)||!m_broadcast_waiting){
				    pkt_t tmp;
				    tmp.readFrom(*input_port[in]);
				    tmp.writeTo(*output_port[out]);
					ack[in] = output_port[out]->getAck();
                }
			} else {
				output_port[out]->setVal(false);
			}
		}
        // Special treatment when a broadcast is waiting
        if(m_broadcast_waiting){
            bool granted = true;
            for(size_t out = 0; out < m_out_size-1; out++){
                // Are all outputs free ?
                granted = granted
                    && (m_allocated[out]&&(m_origin[out]==m_in_size-1))
                    && output_port[out]->getAck();
            }
            // If all outputs are free, we transfer the packet
            // and acknowledge the broadcast
            ack[m_in_size-1]=granted;
            m_broadcast_waiting = !granted;
            for(size_t out = 0; out < m_out_size-1; out++){
                if(granted){
				    pkt_t tmp;
				    tmp.readFrom(*input_port[m_in_size-1]);
				    tmp.writeTo(*output_port[out]);
                } else {
                    if(m_allocated[out]&&(m_origin[out]==m_in_size-1))
                        output_port[out]->setVal(false);
                }
            }
        }
        // Send the acknowledges
        for( size_t in = 0; in < m_in_size; in++)
			input_port[in]->setAck(ack[in]);
    }
};

}

#define tmpl(x) template<typename vci_param> x VciLocalCrossbar<vci_param>

/////////////////////////
tmpl(void)::print_trace()
{
    std::cout << "LocalCrossbar " << name() << " / command  /";
    m_cmd_crossbar->print_trace();
    std::cout << std::endl;
    std::cout << "LocalCrossbar " << name() << " / response /";
    m_rsp_crossbar->print_trace();
    std::cout << std::endl;
}

////////////////////////
tmpl(void)::transition()
{
    if ( ! p_resetn.read() ) {
        m_cmd_crossbar->reset();
        m_rsp_crossbar->reset();
        return;
    }

    m_cmd_crossbar->transition( m_ports_to_initiator, m_ports_to_target );
    m_rsp_crossbar->transition( m_ports_to_target, m_ports_to_initiator );
}

//////////////////////
tmpl(void)::genMealy()
{
    m_cmd_crossbar->genMealy( m_ports_to_initiator, m_ports_to_target );
    m_rsp_crossbar->genMealy( m_ports_to_target, m_ports_to_initiator );
}

/////////////////////////////
tmpl(/**/)::VciLocalCrossbar(
	sc_core::sc_module_name name,
	const soclib::common::MappingTable &mt,
	const soclib::common::IntTab &srcid,
	const soclib::common::IntTab &tgtid,
	size_t nb_attached_initiat,
	size_t nb_attached_target )
           : BaseModule(name),
           p_clk("clk"),
           p_resetn("resetn"),
	   p_to_target(soclib::common::alloc_elems<VciInitiator<vci_param> >(
                           "to_target", nb_attached_target)),
	   p_to_initiator(soclib::common::alloc_elems<VciTarget<vci_param> >(
                              "to_initiator", nb_attached_initiat)),
           p_target_to_up("target_to_up"),
           p_initiator_to_up("initiator_to_up"),
           m_nb_attached_initiat(nb_attached_initiat),
           m_nb_attached_target(nb_attached_target)
{
    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMealy);
    dont_initialize();
    sensitive << p_clk.neg();

	for ( size_t i=0; i<nb_attached_initiat; ++i )
		sensitive << p_to_initiator[i];
	for ( size_t i=0; i<nb_attached_target; ++i )
		sensitive << p_to_target[i];

    sensitive << p_target_to_up
              << p_initiator_to_up;

	m_cmd_crossbar = new cmd_crossbar_t(
		nb_attached_initiat+1, 
		nb_attached_target+1,
		mt.getRoutingTable(tgtid),
        	mt.getLocalityTable(tgtid),
        	nb_attached_target );

	m_rsp_crossbar = new rsp_crossbar_t(
		nb_attached_target+1, 
		nb_attached_initiat+1,
		mt.getIdMaskingTable(srcid.level()),
        	mt.getIdLocalityTable(srcid),
        	nb_attached_initiat );

    m_ports_to_initiator = new VciTarget<vci_param>*[nb_attached_initiat+1];
    for (size_t i=0; i<nb_attached_initiat; ++i)
        m_ports_to_initiator[i] = &p_to_initiator[i];
    m_ports_to_initiator[nb_attached_initiat] = &p_target_to_up;

    m_ports_to_target = new VciInitiator<vci_param>*[nb_attached_target+1];
    for (size_t i=0; i<nb_attached_target; ++i)
        m_ports_to_target[i] = &p_to_target[i];
    m_ports_to_target[nb_attached_target] = &p_initiator_to_up;
}

///////////////////////////////
tmpl(/**/)::~VciLocalCrossbar()
{
    soclib::common::dealloc_elems(p_to_initiator, m_nb_attached_initiat);
    soclib::common::dealloc_elems(p_to_target, m_nb_attached_target);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
