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
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_COMMON_FB_CONTROLLER_H_
#define SOCLIB_COMMON_FB_CONTROLLER_H_

#include <inttypes.h>
#include "process_wrapper.h"

namespace soclib { namespace common {

class FbController
{
	int m_map_fd;
    bool m_headless_mode;
    soclib::common::ProcessWrapper *m_screen_process;

	uint32_t *m_surface;

public:
	const unsigned long m_width, m_height;

    inline uint32_t* surface() const
    {
        return m_surface;
    }

	FbController(
		const std::string &basename,
		unsigned long width,
		unsigned long height);

	~FbController();

	void update();
};

}}

#endif /* SOCLIB_COMMON_FB_CONTROLLER_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

