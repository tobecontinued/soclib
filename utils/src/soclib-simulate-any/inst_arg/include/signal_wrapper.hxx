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

#ifndef SIGNAL_WRAPPER_HXX
#define SIGNAL_WRAPPER_HXX

#include <string>
#include "signal_wrapper.h"

namespace soclib { namespace common { namespace inst {

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