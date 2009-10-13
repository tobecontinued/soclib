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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007-2009
 *
 * Based on previous works by Laurent Mortiez & Alain Greiner, 2005
 *
 * Maintainers: nipo
 */

#include <systemc>
#include <cassert>
#include <dpp/ref>
#include "../include/vci_vgmn.h"
#include "alloc_elems.h"

#ifdef SOCLIB_MODULE_DEBUG
#define DEBUG_BEGIN do { if (name == "ringc") { do{} while(0)
#define DEBUG_END } } while(0)
// #define DEBUG_BEGIN do { do{} while(0)
// #define DEBUG_END } while(0)
#else
#define DEBUG_BEGIN do { if (0) { do{} while(0)
#define DEBUG_END } } while(0)
#endif

namespace soclib { namespace caba {

using namespace sc_core;

namespace _vgmn {

template<typename data_t>
class DelayLine
{
    typename data_t::ptr *m_line;
    size_t m_size;
    size_t m_ptr;
public:
    DelayLine()
    {}

    inline size_t size() const
    {
        return m_size;
    }

    void init( size_t n_alloc )
    {
        m_line = new typename data_t::ptr[n_alloc];
        m_size = n_alloc;
        m_ptr = 0;
    }

    ~DelayLine()
    {
        delete [] m_line;
    }

    inline typename data_t::ptr shift( const typename data_t::ptr &input )
    {
        typename data_t::ptr tmp = m_line[m_ptr];
        m_line[m_ptr] = input;
        m_ptr = (m_ptr+1)%m_size;
        return tmp;
    }

    inline const typename data_t::ptr &head()
    {
        return m_line[m_ptr];
    }

    void reset()
    {
        for ( size_t i=0; i<m_size; ++i )
            m_line[i].invalidate();
        m_ptr = 0;
    }
};

template<typename data_t>
class AdHocFifo
{
    size_t m_size;
    typename data_t::ptr *m_data;
    size_t m_rptr;
    size_t m_wptr;
    size_t m_usage;

public:
    AdHocFifo()
    {}

    inline size_t size() const
    {
        return m_size;
    }

    void init( size_t fifo_size )
    {
        m_size = fifo_size;
        m_data = new typename data_t::ptr[m_size];
        m_rptr = 0;
        m_wptr = 0;
        m_usage = 0;
    }

    ~AdHocFifo()
    {
        delete [] m_data;
    }

    void reset()
    {
        for ( size_t i=0; i<m_size; ++i )
            m_data[i].invalidate();
        m_rptr = 0;
        m_wptr = 0;
        m_usage = 0;
    }

    inline typename data_t::ptr head() const
    {
        return m_data[m_rptr];
    }

    inline typename data_t::ptr pop()
    {
        typename data_t::ptr tmp = head();
        m_data[m_rptr].invalidate();
        assert(!empty());
        --m_usage;
        m_rptr = (m_rptr+1)%m_size;
        return tmp;
    }

    inline void push( const std::string &name, const typename data_t::ptr data )
    {
        assert(!full());
        ++m_usage;
DEBUG_BEGIN;
        std::cout << name << " pushing data " << *data << " usage: " << m_usage << std::endl;
DEBUG_END;
        m_data[m_wptr] = data;
        m_wptr = (m_wptr+1)%m_size;
    }

    inline bool empty() const
    {
        return m_usage == 0;
    }

    inline bool full() const
    {
        return m_usage == m_size;
    }
};

template<typename _vci_pkt_t>
class OutputPortQueue
{
public:
    typedef _vci_pkt_t vci_pkt_t;
    typedef AdHocFifo<vci_pkt_t> input_fifo_t;
    typedef DelayLine<vci_pkt_t> output_delay_line_t;
    typedef typename vci_pkt_t::output_port_t output_port_t;

private:
    input_fifo_t *m_input_queues;
    output_delay_line_t m_output_delay_line;
    uint32_t m_rand_state;
    size_t m_n_inputs;
    size_t m_current_input;
    bool m_in_transaction;

public:
    OutputPortQueue()
    {}
    
    void init( size_t n_inputs, size_t min_delay, size_t fifo_size )
    {
        m_input_queues = new input_fifo_t[n_inputs];
        for ( size_t i=0; i<n_inputs; ++i )
            m_input_queues[i].init(fifo_size);
        m_output_delay_line.init(min_delay);
        m_n_inputs = n_inputs;
        m_rand_state = 0x55555555;
        m_current_input = 0;
        m_in_transaction = false;
        m_output_delay_line.reset();
    }

    ~OutputPortQueue()
    {
        reset();
        delete [] m_input_queues;
    }

    void reset()
    {
        for ( size_t i=0; i<m_n_inputs; ++i )
            m_input_queues[i].reset();
        m_output_delay_line.reset();
        m_current_input = 0;
        m_in_transaction = false;
        for (size_t i=0; i<42; ++i)
            next_rand();
    }

    inline input_fifo_t *getFifo( size_t n )
    {
        return &m_input_queues[n];
    }
    
    uint32_t next_rand()
    {
        m_rand_state = (
            (
                (!!(m_rand_state & (1<<31))) ^
                (!!(m_rand_state & (1<<21))) ^
                (!!(m_rand_state & (1<<1))) ^
                (!!(m_rand_state & (1<<0)))
                ) << 31)
            | ( (m_rand_state >> 1) & 0x7fffffff );
        return m_rand_state;
    }

    void transition( const std::string &name, const output_port_t &port )
    {
        if (port.iProposed() && ! port.peerAccepted())
            return;

        typename vci_pkt_t::ptr pkt(NULL);
        if ( m_in_transaction ) {
            if ( !m_input_queues[m_current_input].empty() )
                pkt = m_input_queues[m_current_input].pop();
        } else {
            size_t offset = next_rand() % m_n_inputs;
            for ( size_t _i = m_current_input+offset;
                  _i != m_current_input+m_n_inputs+offset;
                  ++_i ) {
                size_t i = _i%m_n_inputs;
                if ( ! m_input_queues[i].empty() ) {
                    m_current_input = i;
                    pkt = m_input_queues[m_current_input].pop();
                    break;
                }
            }
        }

DEBUG_BEGIN;
        if ( pkt.valid() )
            std::cout << name << " popped packet " << *pkt << std::endl;
DEBUG_END;

        m_output_delay_line.shift(pkt);
        if ( pkt.valid() )
            m_in_transaction = !pkt->eop();
    }

    void genMoore( const std::string &name, output_port_t &port )
    {
        typename vci_pkt_t::ptr pkt = m_output_delay_line.head();

        if ( pkt.valid() ) {
DEBUG_BEGIN;
            std::cout << name << " packet on VCI " << *pkt << std::endl;
DEBUG_END;
            pkt->writeTo(port);
        } else
            port.setVal(false);
    }
};

template<typename output_queue_t>
class InputRouter
{
    typedef typename output_queue_t::input_fifo_t dest_fifo_t;
    typedef typename output_queue_t::vci_pkt_t vci_pkt_t;
    typedef typename output_queue_t::vci_pkt_t::input_port_t input_port_t;
    typedef typename output_queue_t::vci_pkt_t::routing_table_t routing_table_t;

    routing_table_t m_routing_table;
    dest_fifo_t **m_output_fifos;
    size_t m_n_outputs;
    dest_fifo_t *m_dest;
    typename vci_pkt_t::ptr m_waiting_packet;
    uint32_t m_broadcast_waiting;

public:
    InputRouter()
    {}

    ~InputRouter()
    {
        reset();
        delete [] m_output_fifos;
    }

    void init( size_t n_outputs,
               dest_fifo_t **dest_fifos,
               const routing_table_t &rt )
    {
        m_routing_table = rt;
        m_n_outputs = n_outputs;
        m_dest = NULL;
        m_output_fifos = new dest_fifo_t*[m_n_outputs];
        for ( size_t i=0; i<m_n_outputs; ++i )
            m_output_fifos[i] = dest_fifos[i];
        m_broadcast_waiting = 0;
    }

    void reset()
    {
        m_waiting_packet.invalidate();
        m_broadcast_waiting = 0;
        m_dest = NULL;
    }

    void transition( const std::string &name, const input_port_t &port )
    {
        if ( m_waiting_packet.valid() ) {
            if ( m_broadcast_waiting ) {
                for ( size_t i=0; i<m_n_outputs; ++i ) {
                    if ( ! ((m_broadcast_waiting>>i) & 1) )
                        continue;

                    dest_fifo_t *dest = m_output_fifos[i];
                    if ( ! dest->full() ) {
                        dest->push( name, m_waiting_packet );
                        m_broadcast_waiting &= ~(1<<i);
                    }
                }
                if ( m_broadcast_waiting == 0 ) {
                    m_waiting_packet.invalidate();
                }
            } else {
                assert(m_dest != NULL);
                if ( ! m_dest->full() ) {
                    m_dest->push( name, m_waiting_packet );
                    if ( m_waiting_packet->eop() )
                        m_dest = NULL;
                    m_waiting_packet.invalidate();
                }
            }
        }

        if ( port.iAccepted() ) {
            assert( ! m_waiting_packet.valid() );
            m_waiting_packet = typename vci_pkt_t::ptr(new vci_pkt_t(), true);
            m_waiting_packet->readFrom( port );
DEBUG_BEGIN;
            std::cout << name << " accepting " << *m_waiting_packet << std::endl;
DEBUG_END;
            if ( m_dest == NULL ) {
                if ( m_waiting_packet->is_broadcast() ) {
                    m_broadcast_waiting = (1<<m_n_outputs)-1;
                    assert( m_waiting_packet->eop() );
                } else {
                    m_dest = m_output_fifos[m_waiting_packet->route( m_routing_table )];
                }
DEBUG_BEGIN;
                if ( m_broadcast_waiting )
                    std::cout << name << " broadcasted" << std::endl;
                else
                    std::cout << name << " routed to port " << m_waiting_packet->route( m_routing_table ) << std::endl;
DEBUG_END;
            }
        } else {
            // No packet locally waiting nor on port, no need to
            // go further
            return;
        }
    }

    void genMoore( const std::string &name, input_port_t &port )
    {
        bool can_take = !m_waiting_packet.valid();

        // If we know where we go, we may pipeline
        // If we are in a broadcast, m_dest is NULL and no pipelining
        // occurs.
        if (m_dest != NULL)
            can_take |= !m_dest->full();

        port.setAck(can_take);
    }
};

template<typename router_t,typename queue_t>
class MicroNetwork
{
    typedef typename queue_t::vci_pkt_t::input_port_t input_port_t;
    typedef typename queue_t::vci_pkt_t::output_port_t output_port_t;
    typedef typename queue_t::vci_pkt_t::routing_table_t routing_table_t;
    typedef typename queue_t::input_fifo_t mid_fifo_t;

    size_t m_in_size;
    size_t m_out_size;
    queue_t *m_queue;
    router_t *m_router;

public:
    MicroNetwork(
        size_t in_size, size_t out_size,
        size_t min_latency, size_t fifo_size,
        const routing_table_t &rt )
    {
        mid_fifo_t *fifos[out_size];

        m_in_size = in_size;
        m_out_size = out_size;
        m_router = new router_t[m_in_size];
        m_queue = new queue_t[m_out_size];
        for ( size_t i=0; i<m_out_size; ++i )
            m_queue[i].init(m_in_size, min_latency, fifo_size);
        for ( size_t i=0; i<m_in_size; ++i ) {
            for ( size_t j=0; j<m_out_size; ++j )
                fifos[j] = m_queue[j].getFifo(i);
            m_router[i].init(m_out_size, &fifos[0], rt);
        }
    }

    ~MicroNetwork()
    {
        delete [] m_router;
        delete [] m_queue;
    }

    void reset()
    {
        for ( size_t i=0; i<m_in_size; ++i )
            m_router[i].reset();
        for ( size_t i=0; i<m_out_size; ++i )
            m_queue[i].reset();
    }

    void transition( const std::string &name, const input_port_t *input_port, const output_port_t *output_port )
    {
        for ( size_t i=0; i<m_in_size; ++i )
            m_router[i].transition( name, input_port[i] );
        for ( size_t i=0; i<m_out_size; ++i )
            m_queue[i].transition( name, output_port[i] );
    }

    void genMoore( const std::string &name, input_port_t *input_port, output_port_t *output_port )
    {
        for ( size_t i=0; i<m_in_size; ++i )
            m_router[i].genMoore( name, input_port[i] );
        for ( size_t i=0; i<m_out_size; ++i )
            m_queue[i].genMoore( name, output_port[i] );
    }
};

}


#define tmpl(x) template<typename vci_param> x VciVgmn<vci_param>

tmpl(void)::transition()
{
    if ( ! p_resetn.read() ) {
        m_cmd_mn->reset();
        m_rsp_mn->reset();
        return;
    }

    m_cmd_mn->transition( name(), p_to_initiator, p_to_target );
    m_rsp_mn->transition( name(), p_to_target, p_to_initiator );
}

tmpl(void)::genMoore()
{
    m_cmd_mn->genMoore( name(), p_to_initiator, p_to_target );
    m_rsp_mn->genMoore( name(), p_to_target, p_to_initiator );
}

tmpl(/**/)::VciVgmn(
    sc_module_name name,
    const soclib::common::MappingTable &mt,
    size_t nb_attached_initiat,
    size_t nb_attached_target,
    size_t min_latency,
    size_t fifo_depth )
           : soclib::caba::BaseModule(name),
           m_nb_initiat(nb_attached_initiat),
           m_nb_target(nb_attached_target)
{
    assert(m_nb_target <= 31);
    assert(m_nb_initiat <= 31);

    p_to_initiator = soclib::common::alloc_elems<soclib::caba::VciTarget<vci_param> >("to_initiator", nb_attached_initiat);
    p_to_target = soclib::common::alloc_elems<soclib::caba::VciInitiator<vci_param> >("to_target", nb_attached_target);
    m_cmd_mn = new _vgmn::MicroNetwork<cmd_router_t,cmd_queue_t>(
        nb_attached_initiat, nb_attached_target,
        min_latency, fifo_depth,
        mt.getRoutingTable(soclib::common::IntTab(), 0)
        );
    m_rsp_mn = new _vgmn::MicroNetwork<rsp_router_t,rsp_queue_t>(
        nb_attached_target, nb_attached_initiat,
        min_latency, fifo_depth,
        mt.getIdMaskingTable(0)
        );

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();
}

tmpl(/**/)::~VciVgmn()
{
    delete m_rsp_mn;
    delete m_cmd_mn;
    soclib::common::dealloc_elems(p_to_initiator, m_nb_initiat);
    soclib::common::dealloc_elems(p_to_target, m_nb_target);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
