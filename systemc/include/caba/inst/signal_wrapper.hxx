// -*- c++ -*-
#ifndef SIGNAL_WRAPPER_HXX
#define SIGNAL_WRAPPER_HXX

#include <string>
#include "signal_wrapper.h"

namespace soclib { namespace caba { namespace inst {

BaseSignal::BaseSignal()
{
}

template <typename sig_t>
Signal<sig_t>::Signal( sig_t &sig )
	: m_sig( sig )
{
}

template <typename sig_t> sig_t&
Signal<sig_t>::get()
{
	return m_sig;
}

BaseSignal& BaseSignal::operator /( BasePort &port )
{
	port.connectTo( *this );
	return *this;
}

}}}

#endif
