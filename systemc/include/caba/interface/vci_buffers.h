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
#ifndef SOCLIB_CABA_SIGNAL_VCI_BUFFERS_H_
#define SOCLIB_CABA_SIGNAL_VCI_BUFFERS_H_

#include <inttypes.h>
#include <iostream>
#include <systemc.h>
#include "common/address_masking_table.h"
#include "common/address_decoding_table.h"
#include "caba/interface/vci_param.h"
#include "caba/interface/vci_initiator.h"
#include "caba/interface/vci_target.h"

namespace soclib { namespace caba {

template <typename vci_param>
class VciRspBuffer
{
	typename vci_param::val_t  rspval;
	typename vci_param::data_t    rdata;
	bool                          reop;
	typename vci_param::rerror_t  rerror;
	typename vci_param::srcid_t   rsrcid;
	typename vci_param::trdid_t   rtrdid;
	typename vci_param::pktid_t   rpktid;
public:
    typedef soclib::common::AddressMaskingTable<uint32_t> routing_table_t;

    typedef VciInitiator<vci_param> input_port_t;
    typedef VciTarget<vci_param> output_port_t;

    inline bool val() const
    {
        return rspval;
    }

    inline bool eop() const
    {
        return reop;
    }
    
    inline uint32_t dest() const
    {
        return rsrcid;
    }
    
	inline void writeTo( output_port_t &port ) const
	{
		port.rspval = rspval;
		port.rdata = rdata;
		port.reop = reop;
		port.rerror = rerror;
		port.rsrcid = rsrcid;
		port.rtrdid = rtrdid;
		port.rpktid = rpktid;
	}

	inline void readFrom( const input_port_t &port )
	{
		rspval = port.rspval;
		rdata = port.rdata;
		reop = port.reop;
		rerror = port.rerror;
		rsrcid = port.rsrcid;
		rtrdid = port.rtrdid;
		rpktid = port.rpktid;
	}

    inline int route( const routing_table_t &rt ) const
    {
        return rt[rsrcid];
    }

    friend std::ostream &operator << (std::ostream &o, const VciRspBuffer &b)
    {
        b.print(o);
        return o;
    }

    void print( std::ostream &o ) const
    {
        o << "VciRspBuffer" << std::hex << std::endl
          << " rspval: " << rspval << std::endl
          << " rdata : " << rdata << std::endl
          << " reop  : " << reop << std::endl
          << " rerror: " << rerror << std::endl
          << " rsrcid: " << rsrcid << std::endl
          << " rtrdid: " << rtrdid << std::endl
          << " rpktid: " << rpktid << std::endl;
    }
};

template <typename vci_param>
class VciCmdBuffer
{
	typename vci_param::val_t  cmdval;
	typename vci_param::addr_t    address;
	typename vci_param::be_t      be;
	typename vci_param::cmd_t     cmd;
	typename vci_param::contig_t  contig;
	typename vci_param::data_t    wdata;
	typename vci_param::eop_t     eop_;
	typename vci_param::const_t   cons;
	typename vci_param::plen_t    plen;
	typename vci_param::wrap_t    wrap;
	typename vci_param::cfixed_t  cfixed;
	typename vci_param::clen_t    clen;
	typename vci_param::srcid_t   srcid;
	typename vci_param::trdid_t   trdid;
	typename vci_param::pktid_t   pktid;
public:
    typedef soclib::common::AddressDecodingTable<uint32_t, int> routing_table_t;

    typedef VciInitiator<vci_param> output_port_t;
    typedef VciTarget<vci_param> input_port_t;

    inline bool val() const
    {
        return cmdval;
    }

    inline bool eop() const
    {
        return eop_;
    }
    
    inline uint32_t dest() const
    {
        return address;
    }
    
	inline void readFrom( const input_port_t &port )
	{
		cmdval = port.cmdval;
		address = port.address;
		be = port.be;	
		cmd = port.cmd;	
		contig = port.contig;
		wdata = port.wdata;
		eop_ = port.eop;	
		cons = port.cons;	
		plen = port.plen;	
		wrap = port.wrap;	
		cfixed = port.cfixed;
		clen = port.clen;	
		srcid = port.srcid;
		trdid = port.trdid;
		pktid = port.pktid;
	}

	inline void writeTo( output_port_t &port ) const
	{
		port.cmdval = cmdval;
		port.address = address;
		port.be = be;	
		port.cmd = cmd;	
		port.contig = contig;
		port.wdata = wdata;
		port.eop = eop_;	
		port.cons = cons;	
		port.plen = plen;	
		port.wrap = wrap;	
		port.cfixed = cfixed;
		port.clen = clen;	
		port.srcid = srcid;
		port.trdid = trdid;
		port.pktid = pktid;
	}

    inline int route( const routing_table_t &rt ) const
    {
        return rt[address];
    }

    friend std::ostream &operator << (std::ostream &o, const VciCmdBuffer &b)
    {
        b.print(o);
        return o;
    }

    void print( std::ostream &o ) const
    {
        o << "VciCmdBuffer" << std::hex << std::endl
          << " cmdval : " << cmdval << std::endl
          << " address: " << address << std::endl
          << " be     : " << be << std::endl
          << " cmd    : " << cmd << std::endl
          << " contig : " << contig << std::endl
          << " wdata  : " << wdata << std::endl
          << " eop    : " << eop_ << std::endl
          << " cons   : " << cons << std::endl
          << " plen   : " << plen << std::endl
          << " wrap   : " << wrap << std::endl
          << " cfixed : " << cfixed << std::endl
          << " clen   : " << clen << std::endl
          << " srcid  : " << srcid << std::endl
          << " trdid  : " << trdid << std::endl
          << " pktid  : " << pktid << std::endl;
    }
};

}}

#endif /* SOCLIB_CABA_SIGNAL_VCI_BUFFERS_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

