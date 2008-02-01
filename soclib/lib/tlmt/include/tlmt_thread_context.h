/* -*- mode: c++; coding: utf-8 -*-
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
 * Maintainers: fpecheux, nipo
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <fancois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 */


#ifndef _TLMT_CORE_TLMT_THREAD_CONTEXT_H_
#define _TLMT_CORE_TLMT_THREAD_CONTEXT_H_

#include "tlmt_time.h"

namespace tlmt_core {

class tlmt_thread_context
{
private:
	tlmt_time m_time;
	bool m_active;
	bool m_sending;

public:  

	tlmt_thread_context()
		: m_time(0),
		  m_active(true),
		  m_sending(false)
	{}

	inline const tlmt_time &time() const;

	inline const bool active() const;

	inline const bool sending() const;

	inline void enable();

	inline void disable();

	inline void start_sending();

	inline void stop_sending();

	inline void add_time( const tlmt_time &offset );

	inline void set_time( const tlmt_time &t );
};

}

#endif


