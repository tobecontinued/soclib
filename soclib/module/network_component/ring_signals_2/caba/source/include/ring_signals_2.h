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
 * Author   : Abdelmalek SI MERABET 
 * Date     : september 2008
 * Copyright: UPMC - LIP6
 */

#ifndef SOCLIB_CABA_RING_SIGNALS_H_
#define SOCLIB_CABA_RING_SIGNALS_H_

namespace soclib { namespace caba {

class RingSignals2
{
public:
	bool       cmd_grant;
        uint64_t   cmd_data;
    	bool       cmd_w;       // in : cmd_rok
    	bool       cmd_r;       // in : cmd_wok
    	bool       rsp_grant;   
    	uint64_t   rsp_data;
    	bool       rsp_w;       // in : rsp_rok
    	bool       rsp_r;       // in : rsp_wok

	RingSignals2()
	{}
/*
	RingSignals2(std::string name = (std::string)sc_gen_unique_name("ring_signals_2_"))
	  : 	cmd_grant	((name+"cmd_grant").c_str()),
		cmd_data        ((name+"cmd_data").c_str()),
		cmd_w    	((name+"cmd_w").c_str()),
		cmd_r    	((name+"cmd_r").c_str()),	
		rsp_grant	((name+"rsp_grant").c_str()),
		rsp_data        ((name+"rsp_data").c_str()),
		rsp_w           ((name+"rsp_w").c_str()),
		rsp_r    	((name+"rsp_r").c_str()) 
	{ }
*/
};

}} // end namespace

#endif /* SOCLIB_CABA_RING_SIGNALS_H_ */
