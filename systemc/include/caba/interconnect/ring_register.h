/*
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
 * Author   : Yang GAO 
 * Date     : 28/09/2007
 * Copyright: UPMC - LIP6
 */

#ifndef VCI_RING_REGISTER_H_
#define VCI_RING_REGISTER_H_

#include <systemc.h>
#include "caba/util/base_module.h"
#include "caba/interface/ring_ports.h"

namespace soclib { namespace caba {

template<typename vci_param>
class RingRegister 
	: public soclib::caba::BaseModule
{

protected:
	SC_HAS_PROCESS(RingRegister);

public:
	// ports
	sc_in<bool> 				  p_clk;
	sc_in<bool>   				  p_resetn;
	soclib::caba::RingNegINPort<vci_param> 	  p_rni;
	soclib::caba::RingDataINPort<vci_param>	  p_rdi;
	soclib::caba::RingNegOUTPort<vci_param>	  p_rno;
	soclib::caba::RingDataOUTPort<vci_param>  p_rdo;
	
	// constructor / destructor
	RingRegister(sc_module_name	insname);

private:
	// methods
	void transition();
};

}} // end namespace
		
#endif // VCI_RING_REGISTER_H_
