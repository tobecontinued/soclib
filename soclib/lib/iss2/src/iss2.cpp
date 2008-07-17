/* -*- c++ -*-
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
 *         Nicolas Pouillon <nipo@ssji.net>, 2008
 */

#include <iostream>
#include "iss2.h"

namespace soclib { namespace common {

#define rstr(x) case Iss2::x: return #x

static const char *mode_str(Iss2::ExecMode mode)
{
    switch(mode) {
        rstr(MODE_HYPER);
        rstr(MODE_KERNEL);
        rstr(MODE_USER);
    default: return "invalid";
    }
}

static const char *type_str(Iss2::DataOperationType type)
{
    switch(type) {
        rstr(DATA_READ);
        rstr(DATA_WRITE);
        rstr(DATA_LL);
        rstr(DATA_SC);
        rstr(XTN_WRITE);
        rstr(XTN_READ);
    default: return "invalid";
    }
}

static const char *xtn_str(Iss2::ExternalAccessType type)
{
    switch(type) {
        rstr(XTN_PTPR);
        rstr(XTN_TLB_EN);
        rstr(XTN_ICACHE_FLUSH);
        rstr(XTN_DCACHE_FLUSH);
        rstr(XTN_ITLB_INVAL);
        rstr(XTN_DTLB_INVAL);
        rstr(XTN_ICACHE_INVAL);
        rstr(XTN_DCACHE_INVAL);
        rstr(XTN_ICACHE_PREFETCH);
        rstr(XTN_DCACHE_PREFETCH);
        rstr(XTN_SYNC);
        rstr(XTN_ERR_TYPE);
        rstr(XTN_BAD_VADDR);
    default: return "invalid";
    }
}

void Iss2::InstructionRequest::print( std::ostream &o ) const
{
    o << "<InsReq "
      << " mode " << mode_str(mode)
      << (valid ? "   valid" : " invalid")
      << " @ " << std::hex << std::showbase << addr
      << ">";
}

void Iss2::InstructionResponse::print( std::ostream &o ) const
{
    o << "<InsRsp  " << (valid ? "valid" : "invalid")
      << " " << (error ? "error" : "no error")
      << " ins " << std::hex << std::showbase << instruction
      << ">";
}

void Iss2::DataRequest::print( std::ostream &o ) const
{
    o << "<DataReq"
      << " mode " << mode_str(mode)
      << (valid ? "   valid" : " invalid")
      << " type " << type_str(type);
    if ( type == XTN_READ || type == XTN_WRITE )
        o << " (" << xtn_str((enum ExternalAccessType)addr) << ")";
    else
        o << " @ " << std::hex << std::showbase << addr;
    o << " wdata " << std::hex << std::showbase << wdata
      << " be " << std::hex << (int)be
      << ">";
}

void Iss2::DataResponse::print( std::ostream &o ) const
{
    o << "<DataRsp " << (valid ? "valid" : "invalid")
      << " " << (error ? "error" : "no error")
      << " rdata " << std::hex << std::showbase << rdata
      << ">";
}


}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
