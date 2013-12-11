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
 * Copyright (c) UPMC, Lip6
 *         Sylvain Leroy <sylvain.leroy@lip6.fr>
 *
 * Maintainers: sylvain
 */

#ifndef SOCLIB_CABA_BACKEND_RX_H
#define SOCLIB_CABA_BACKEND_RX_H

#include <inttypes.h>
#include <systemc>
#include <assert.h>

#include <string>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>


namespace soclib {
namespace caba {

////////////////
class NicRxBackend
{
    /*!
     * \brief This function is used to read one packet from the input fill.
     *        It updates the r_buffer and the r_plen variables.
     */
    virtual void read_one_packet() = 0;

public:

    /*!
     * \brief ???
     */
    virtual void reset() = 0;

    /*!
     * \brief ???
     */
    virtual void get( bool* dv, bool* er, uint8_t* dt) = 0;

    /*!
     * \brief This method returns true if the RX chain is to be frozen.
     */
    virtual bool frz() = 0;

    /*!
     * \brief destructor
     */
    // virtual ~NicRxBackend();

}; // end NicRxBackend

} /* caba */
} /* soclib */

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4



