// -*- c++ -*-
#ifndef PORT_WRAPPER_HXX
#define PORT_WRAPPER_HXX

#include <systemc>
#include <string>
#include "common/exception.h"
#include "signal_wrapper.h"

namespace soclib { namespace caba { namespace inst {

template <typename port_t, typename sig_t>
Port<port_t, sig_t>::Port( const std::string &name, port_t &port )
	: BasePort(name),
	  m_port( port )
{
}

template <> void
Port<sc_core::sc_in<bool>, sc_core::sc_signal<bool> >::connectTo( BaseSignal &_sig );

template <typename port_t, typename sig_t> void
Port<port_t, sig_t>::connectTo( BaseSignal &_sig )
{
	typedef Signal<sig_t> wrapped_signal_t;

	if ( wrapped_signal_t *sig = dynamic_cast<wrapped_signal_t*>(&_sig) ) {
//		std::cout << "Connecting " << fullName() << std::endl;
		m_port(sig->get());
		m_connected = true;
	} else
		throw soclib::exception::RunTimeError("Cant connect "+fullName());
}

template <typename port_t, typename sig_t> BaseSignal&
Port<port_t, sig_t>::operator /( BasePort &port )
{
	return alone() / port;
}

template <typename port_t, typename sig_t> BaseSignal&
Port<port_t, sig_t>::alone()
{
	Signal<sig_t> &sig = Signal<sig_t>::create();
	sig / *this;
	return sig;
}

}}}

#endif
