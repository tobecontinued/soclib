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

#ifndef PORT_WRAPPER_H
#define PORT_WRAPPER_H

#include <string>
#include <iostream>

namespace soclib { namespace caba { namespace inst {
class BasePort;
template <typename port_t, typename sig_t> class Port;
}}}

#include "signal_wrapper.h"

namespace soclib {
namespace caba {
namespace inst {

class BasePort {
	std::string m_port_name;
	std::string m_owner_name;
protected:
	bool m_connected;
	BaseSignal *m_signal;
public:
	BasePort( const std::string & );
	BasePort( const BasePort & );
	virtual ~BasePort();
	virtual void connectTo( BaseSignal& sig ) = 0;
	inline void ownerNameSet( const std::string &name )
	{
		m_owner_name = name;
	}
	const std::string fullName() const
	{
		return m_owner_name + '.' + m_port_name;
	}
	inline void rename( const std::string &name )
	{
		m_port_name = name;
	}
	inline bool isNamed( const std::string &name ) const
	{
		return name == m_port_name;
	}
	virtual BaseSignal &operator /( BasePort &port ) = 0;
	virtual BaseSignal &alone() = 0;
	inline void autoConn()
	{
		if ( m_connected ) return;
		std::cerr << "Warning: autoconnecting " << fullName() << " to nothing" << std::endl;
		alone();
	}
};

template <typename port_t, typename sig_t>
class Port
	: public BasePort
{
	port_t &m_port;
public:
	inline Port( const std::string &name, port_t &port );
	void connectTo( BaseSignal &sig );
	BaseSignal &operator /( BasePort &port );
	BaseSignal &alone();
};

}}}

#endif
