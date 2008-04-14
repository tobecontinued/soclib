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
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 */

#include "vci_param.h"
#include "../include/vci_multi_tty.h"
#include "tty.h"

#include <stdarg.h>


namespace soclib { namespace tlmt {

#define tmpl(x) template<typename vci_param> x VciMultiTty<vci_param>

tmpl(tlmt_core::tlmt_return&)::callback(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
	const tlmt_core::tlmt_time &time,
	void *private_data)
{
	// std::cout << name() << " callback" << std::endl;

	// First, find the right segment using the first address of the packet
	
        std::list<soclib::common::Segment>::iterator seg;
        bool reached = false;
        size_t seg_index;
        for (seg_index=0,seg = m_segments.begin();
                 seg != m_segments.end() && !reached; ++seg_index, ++seg ) {
                soclib::common::Segment &s = *seg;
                //if (!s.contains(pkt->address[0]))
                if (!s.contains(pkt->address))
                        continue;
                reached=true;
                switch(pkt->cmd)
                {
                case vci_param::CMD_READ:
                        return callback_read(seg_index,s,pkt,time,private_data);
                case vci_param::CMD_WRITE:
                        return callback_write(seg_index,s,pkt,time,private_data);
		case vci_param::CMD_LOCKED_READ:
		case vci_param::CMD_STORE_COND:
                	return m_return;
			break;
                }
                return m_return;
        }
        if (!reached) {
                std::cout << "Address does not match any segment" << std::endl ;
        }

	return m_return;
}

tmpl(tlmt_core::tlmt_return&)::callback_read(size_t seg_index,
	 soclib::common::Segment &s,
	 soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
	 const tlmt_core::tlmt_time &time,
	 void *private_data)
{
	// std::cout << "callback_read" << std::endl;
	return m_return;
}

tmpl(tlmt_core::tlmt_return&)::callback_write(size_t seg_index,
	  soclib::common::Segment &s,
	  soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
	  const tlmt_core::tlmt_time &time,
	  void *private_data)
{

    //int cell = (pkt->address[0]-s.baseAddress()) / 4;
    int cell = (pkt->address-s.baseAddress()) / 4;
    int reg = cell % TTY_SPAN;
    int term_no = cell / TTY_SPAN;
    char _data = (char)pkt->buf[0];

    switch (reg) {
    case TTY_WRITE:

        if ( _data == '\a' ) {
            char tmp[32];
            size_t ret = snprintf(tmp, sizeof(tmp), "[%10ld] ", r_counter);

            for ( size_t i=0; i<ret; ++i )
                m_term[term_no]->putc( tmp[i] );
        } else
	{
            m_term[term_no]->putc( _data );
	}
	break;
    default:
	break;
    }

	// std::cout << "callback_write" << std::endl;
        //rsp.cmd=pkt->cmd;
        rsp.nwords=pkt->nwords;
        rsp.srcid=pkt->srcid;
        rsp.pktid=pkt->pktid;
        rsp.trdid=pkt->trdid;

        p_vci.send(&rsp, time+tlmt_core::tlmt_time(pkt->nwords+5));
        m_return.set_time(time+tlmt_core::tlmt_time(pkt->nwords+5));

	p_out.send(true, time+tlmt_core::tlmt_time(pkt->nwords+5));
	return m_return;
}

tmpl(/**/)::VciMultiTty(
	sc_core::sc_module_name name,
	const soclib::common::IntTab &index,
	const soclib::common::MappingTable &mt,
	const char *first_name,
	...)
		   : soclib::tlmt::BaseModule(name),
		   m_index(index),
		   m_mt(mt),
		   p_vci("vci", new tlmt_core::tlmt_callback<VciMultiTty,soclib::tlmt::vci_cmd_packet<vci_param> *>(
				   this, &VciMultiTty<vci_param>::callback)),
		   p_out("out")
{
    m_segments = m_mt.getSegmentList(m_index);

    va_list va_tty;

    va_start (va_tty, first_name);
    std::vector<std::string> args;
    const char *cur_tty = first_name;
    while (cur_tty) {
        args.push_back(cur_tty);

        cur_tty = va_arg( va_tty, char * );
    }
    va_end( va_tty );

    for ( std::vector<std::string>::const_iterator i = args.begin();
          i != args.end();
          ++i )
        m_term.push_back(soclib::common::allocateTty(*i));

	r_counter=0;
}


}}
