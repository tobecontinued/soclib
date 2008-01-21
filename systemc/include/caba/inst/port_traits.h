// -*- c++ -*-
#ifndef PORT_TRAITS_H
#define PORT_TRAITS_H

#include <systemc>

namespace soclib { namespace caba { namespace inst {

using namespace sc_core;
using namespace sc_dt;

template<typename port_t> struct traits {};

#define register_signal_for_port_with_t(t_t, port_t, signal_t)			\
template<t_t> struct traits<port_t > {									   \
	typedef signal_t sig_t;											   \
}

#define register_signal_for_port(port_t, signal_t) \
	register_signal_for_port_with_t(,port_t, signal_t)

register_signal_for_port_with_t(typename T, sc_in<T>,sc_signal<T>);
register_signal_for_port_with_t(typename T, sc_out<T>,sc_signal<T>);
register_signal_for_port_with_t(int W, sc_in<sc_uint<W> >,sc_signal<sc_uint<W> >);
register_signal_for_port_with_t(int W, sc_out<sc_uint<W> >,sc_signal<sc_uint<W> >);

}}}

#endif
