/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_SIGNAL_VCI_SIGNALS_H_
#define SOCLIB_CABA_SIGNAL_VCI_SIGNALS_H_

#include <string>
#include <systemc.h>
#include "caba/interface/vci_param.h"

namespace soclib { namespace caba {

/**
 * VCI Initiator port
 */
template <typename vci_param>
class VciSignals
{
public:
	sc_signal<typename vci_param::ack_t>     rspack;
	sc_signal<typename vci_param::val_t>     rspval;
	sc_signal<typename vci_param::data_t>    rdata;
	sc_signal<bool>                          reop;
	sc_signal<typename vci_param::rerror_t>  rerror;
	sc_signal<typename vci_param::srcid_t>   rsrcid;
	sc_signal<typename vci_param::trdid_t >  rtrdid;
	sc_signal<typename vci_param::pktid_t >  rpktid;

	sc_signal<typename vci_param::ack_t>     cmdack;
	sc_signal<typename vci_param::val_t>     cmdval;
	sc_signal<typename vci_param::addr_t>    address;
	sc_signal<typename vci_param::be_t>      be;
	sc_signal<typename vci_param::cmd_t>     cmd;
	sc_signal<typename vci_param::contig_t>  contig;
	sc_signal<typename vci_param::data_t>    wdata;
	sc_signal<typename vci_param::eop_t>     eop;
	sc_signal<typename vci_param::const_t>   cons;
	sc_signal<typename vci_param::plen_t>    plen;
	sc_signal<typename vci_param::wrap_t>    wrap;
	sc_signal<typename vci_param::cfixed_t>  cfixed;
	sc_signal<typename vci_param::clen_t>    clen;
	sc_signal<typename vci_param::srcid_t>   srcid;
	sc_signal<typename vci_param::trdid_t>   trdid;
	sc_signal<typename vci_param::pktid_t>   pktid; 

#define ren(x) x(((std::string)(name_ + "_"#x)).c_str())

    VciSignals(std::string name_ = (std::string)sc_gen_unique_name("vci"))
        : ren(rspack),
          ren(rspval),
          ren(rdata), 
          ren(reop),  
          ren(rerror),
          ren(rsrcid),
          ren(rtrdid),
          ren(rpktid),
          ren(cmdack),
          ren(cmdval),
          ren(address),
          ren(be),    
          ren(cmd),   
          ren(contig),
          ren(wdata), 
          ren(eop),   
          ren(cons),  
          ren(plen),  
          ren(wrap),  
          ren(cfixed),
          ren(clen),  
          ren(srcid), 
          ren(trdid), 
          ren(pktid)
    {
    }
#undef ren
};

}}

#endif /* SOCLIB_CABA_SIGNAL_VCI_SIGNALS_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

