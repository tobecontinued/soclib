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
 * Authors  : Noé Girand noe.girand@polytechnique.org
 * Date     : june 2009
 * Copyright: UPMC - LIP6
 */

#ifndef VIRTUAL_DSPIN_NETWORK_H_
#define VIRTUAL_DSPIN_NETWORK_H_

#include <systemc>
#include "caba_base_module.h"
#include "virtual_dspin_router.h"
#include "virtual_dspin_network_interface.h"

namespace soclib 
{ 
	namespace caba 
	{
		using namespace sc_core;

		template<	int io_mask_size, 		// Size of IO checking
				int io_number_size, 		// Size of IO index
				int x_adressing_size, 		// Size of first coordinate adressing
				int y_adressing_size, 		// Size of second coordinate adressing

				int cmd_data_size, 		// Size of command flits
				int cmd_io_mask_offset, 	// Emplacement of IO checking in command paquets
				int cmd_io_number_offset, 	// Emplacement of IO index in IO table in command paquets
				int cmd_x_adressing_offset, 	// Emplacement of target x in first flit in command paquets
				int cmd_y_adressing_offset, 	// Emplacement of target y in first flit in command paquets
				int cmd_eop_offset, 		// Emplacement of eop checking in command paquets
				int cmd_broadcast_offset,	// Emplacement of broadcast checking in command paquets

				int rsp_data_size, 		// Size of response flits
				int rsp_io_mask_offset, 	// Emplacement of IO checking in response paquets
				int rsp_io_number_offset, 	// Emplacement of IO index in IO table in response paquets
				int rsp_x_adressing_offset, 	// Emplacement of target x in first flit in response paquets
				int rsp_y_adressing_offset,  	// Emplacement of target y in first flit in response paquets
				int rsp_eop_offset,  		// Emplacement of eop checking in response paquets

				int in_fifo_size, 
				int out_fifo_size
		>
		class VirtualDspinNetwork: public soclib::caba::BaseModule
		{
			

		protected:
			SC_HAS_PROCESS(VirtualDspinNetwork);

		public:

			// ports
			sc_in<bool>	p_clk;
			sc_in<bool>	p_resetn;

			VirtualDspinNetworkPort<cmd_data_size, rsp_data_size> *** ports;

			// constructor 
			VirtualDspinNetwork(sc_module_name  insname, int	size_x, int size_y, bool broadcast0, bool broadcast1, bool io0, bool io1, clusterCoordinates<x_adressing_size, y_adressing_size> * aIO_table);

			// destructor 
			~VirtualDspinNetwork();

		private:

			VirtualDspinRouter<cmd_data_size, cmd_io_mask_offset, io_mask_size, cmd_io_number_offset, io_number_size, 
						cmd_x_adressing_offset, x_adressing_size, cmd_y_adressing_offset, y_adressing_size, cmd_eop_offset, cmd_broadcast_offset, in_fifo_size, out_fifo_size> ** cmdVirtualDspinRouters;
			VirtualDspinRouter<rsp_data_size, rsp_io_mask_offset, io_mask_size, rsp_io_number_offset, io_number_size, 
						rsp_x_adressing_offset, x_adressing_size, rsp_y_adressing_offset, y_adressing_size, rsp_eop_offset, 0, in_fifo_size, out_fifo_size> ** rspVirtualDspinRouters;

			DspinSignals<cmd_data_size> *** vWiresCmd;
			DspinSignals<cmd_data_size> *** hWiresCmd;
			DspinSignals<rsp_data_size> *** vWiresRsp;
			DspinSignals<rsp_data_size> *** hWiresRsp;

			int m_size_x;
			int m_size_y;

		};
	}
} // end namespace

#endif // VIRTUAL_DSPIN_NETWORK_H_
