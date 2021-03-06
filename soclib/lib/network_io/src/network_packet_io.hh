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
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_NETWORK_PACKET_IO_H_
#define SOCLIB_NETWORK_PACKET_IO_H_

#include "../include/network_io.h"
#include "static_init_code.h"

namespace soclib { namespace common {

class NetworkPacketIo
{
	uint32_t m_refcount;

protected:
	NetworkPacketIo();

public:
	virtual ~NetworkPacketIo() = 0;
	virtual bool has_packet() const = 0;
	virtual void put_packet(NetworkPacket *packet) = 0;
	virtual NetworkPacket *get_packet() = 0;

	void ref();
	void unref();
};

}}

#endif /* SOCLIB_NETWORK_PACKET_IO_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

