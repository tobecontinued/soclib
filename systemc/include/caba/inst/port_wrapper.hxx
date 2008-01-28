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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

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
	if ( m_signal )
		return *m_signal;
	Signal<sig_t> &sig = Signal<sig_t>::create();
	m_signal = &sig;
	sig / *this;
	return sig;
}

}}}

#endif
