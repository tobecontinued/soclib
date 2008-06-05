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

#include "inst.h"

namespace soclib { namespace common {

namespace inst {

BasePort::BasePort( const std::string &name )
	: m_port_name(name),
      m_connected(false),
      m_signal(NULL)
{
}

BasePort::BasePort( const BasePort &port )
	: m_port_name(port.m_port_name),
	  m_owner_name(port.m_owner_name),
      m_connected(false), m_signal(NULL)
{
}

BasePort::~BasePort()
{}

template <> void
Port<sc_core::sc_in<bool>, sc_core::sc_signal<bool> >::connectTo( BaseSignal &_sig )
{
	typedef Signal<sc_core::sc_clock> ck_t;
	typedef Signal<sc_core::sc_signal<bool> > wrapped_signal_t;

	if ( ck_t *sig = dynamic_cast<ck_t*>(&_sig) ) {
		std::cout << "Connecting clock " << fullName() << std::endl;
		m_port(sig->get());
        m_connected = true;
	} else if ( wrapped_signal_t *sig = dynamic_cast<wrapped_signal_t*>(&_sig) ) {
		std::cout << "Connecting " << fullName() << std::endl;
		m_port(sig->get());
        m_connected = true;
	} else
		throw soclib::exception::RunTimeError("Cant connect "+fullName());
}

}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

