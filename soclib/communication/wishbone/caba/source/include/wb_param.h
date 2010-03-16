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
 * Copyright (c) Telecom ParisTech
 *         Tarik Graba <tarik.graba@telecom-paristech.fr>, 2009
 *
 * Maintainers: tarik.graba@telecom-paristech.fr
 */

#ifndef SOCLIB_CABA_WB_PARAP_H_
#define SOCLIB_CABA_WB_PARAP_H_

#include <cassert>
#include <systemc>

namespace soclib { namespace caba {

    using namespace sc_core;

    /* WishBone parameters */
    /* Granularity BYTE */
    template < int data_width, int add_width >
        class WbParams
        {
            //assert (!(data_width%8) && "WB data width should be multiple of 8");
            public:
                // Constants which can be used externally
                static const int DataWidth = data_width;
                static const int AddWidth = add_width;
                // Signal types
                typedef sc_dt::sc_uint<DataWidth>   wb_data_t;
                typedef sc_dt::sc_uint<AddWidth>    wb_add_t;
                typedef sc_dt::sc_uint<DataWidth/8> wb_sel_t;
        };

}}

#endif //SOCLIB_CABA_WB_PARAP_H_

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
