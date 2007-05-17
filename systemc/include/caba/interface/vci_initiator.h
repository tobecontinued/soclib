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
#ifndef SOCLIB_CABA_SIGNAL_VCI_INITIATOR_H_
#define SOCLIB_CABA_SIGNAL_VCI_INITIATOR_H_

#include <systemc.h>
#include "caba/interface/vci_signals.h"
#include "caba/interface/vci_param.h"

namespace soclib { namespace caba {

/**
 * VCI Initiator port
 */
template <typename vci_param>
class VciInitiator
{
public:
	sc_out<typename vci_param::ack_t>     rspack;
	sc_in<typename vci_param::val_t>      rspval;
	sc_in<typename vci_param::data_t>     rdata;
	sc_in<bool>                           reop;
	sc_in<typename vci_param::rerror_t>   rerror;
	sc_in<typename vci_param::srcid_t>    rsrcid;
	sc_in<typename vci_param::trdid_t >   rtrdid;
	sc_in<typename vci_param::pktid_t >   rpktid;

	sc_in<typename vci_param::ack_t>      cmdack;
	sc_out<typename vci_param::val_t>     cmdval;
	sc_out<typename vci_param::addr_t>    address;
	sc_out<typename vci_param::be_t>      be;
	sc_out<typename vci_param::cmd_t>     cmd;
	sc_out<typename vci_param::contig_t>  contig;
	sc_out<typename vci_param::data_t>    wdata;
	sc_out<typename vci_param::eop_t>     eop;
	sc_out<typename vci_param::const_t>   cons;
	sc_out<typename vci_param::plen_t>    plen;
	sc_out<typename vci_param::wrap_t>    wrap;
	sc_out<typename vci_param::cfixed_t>  cfixed;
	sc_out<typename vci_param::clen_t>    clen;
	sc_out<typename vci_param::srcid_t>   srcid;
	sc_out<typename vci_param::trdid_t>   trdid;
	sc_out<typename vci_param::pktid_t>   pktid;

	void operator()(VciSignals<vci_param> &sig)
	{
		cmdack  (sig.cmdack);
		address (sig.address);
		be      (sig.be);
		cfixed  (sig.cfixed);
		clen    (sig.clen);
		cmd     (sig.cmd);
		cmdval  (sig.cmdval);
		cons    (sig.cons);
		contig  (sig.contig);
		eop     (sig.eop);
		pktid   (sig.pktid);
		plen    (sig.plen);
		rdata   (sig.rdata);
		reop    (sig.reop);
		rerror  (sig.rerror);
		rpktid  (sig.rpktid);
		rsrcid  (sig.rsrcid);
		rspack  (sig.rspack);
		rspval  (sig.rspval);
		rtrdid  (sig.rtrdid);
		srcid   (sig.srcid);
		trdid   (sig.trdid);
		wdata   (sig.wdata);
		wrap    (sig.wrap);
	}

	void operator()(VciInitiator<vci_param> &ports)
	{
		cmdack  (ports.cmdack);
		address (ports.address);
		be      (ports.be);
		cfixed  (ports.cfixed);
		clen    (ports.clen);
		cmd     (ports.cmd);
		cmdval  (ports.cmdval);
		cons    (ports.cons);
		contig  (ports.contig);
		eop     (ports.eop);
		pktid   (ports.pktid);
		plen    (ports.plen);
		rdata   (ports.rdata);
		reop    (ports.reop);
		rerror  (ports.rerror);
		rpktid  (ports.rpktid);
		rsrcid  (ports.rsrcid);
		rspack  (ports.rspack);
		rspval  (ports.rspval);
		rtrdid  (ports.rtrdid);
		srcid   (ports.srcid);
		trdid   (ports.trdid);
		wdata   (ports.wdata);
		wrap    (ports.wrap);
	}

    inline bool getAck() const
    {
        return cmdack;
    }

    inline bool getVal() const
    {
        return rspval;
    }

    inline void setAck( bool x )
    {
        rspack = x;
    }

    inline void setVal( bool x )
    {
        cmdval = x;
    }

    inline bool iAccepted() const
    {
        return rspval && rspack;
    }

    inline bool peerAccepted() const
    {
        return cmdval && cmdack;
    }
};

}}

#endif /* SOCLIB_CABA_SIGNAL_VCI_INITIATOR_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

