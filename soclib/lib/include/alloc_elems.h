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
#ifndef SOCLIB_ALLOC_ELEMS_H_
#define SOCLIB_ALLOC_ELEMS_H_

#include <stdint.h>
#include <new>
#include <sstream>

namespace soclib { namespace common {

template<typename elem_t>
elem_t *alloc_elems(const std::string &prefix, size_t n)
{
	elem_t *elem = (elem_t*)malloc(sizeof(elem_t)*n);
	for ( size_t i=0; i<n; ++i ) {
		std::ostringstream o;
		o << prefix << "[" << i << "]";
		new(&elem[i]) elem_t(o.str().c_str());
	}
	return elem;
}

template<typename elem_t>
void dealloc_elems(elem_t *elems, size_t n)
{
	for ( size_t i = 0; i<n; ++i )
		elems[i].~elem_t();
	free(elems);
}

}}

#endif /* SOCLIB_ALLOC_ELEMS_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

