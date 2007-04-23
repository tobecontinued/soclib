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
#ifndef SOCLIB_CABA_VCI_VGMN_H_
#define SOCLIB_CABA_VCI_VGMN_H_

#include <systemc.h>
#include "caba/util/base_module.h"
#include "caba/interface/vci_initiator.h"
#include "caba/interface/vci_target.h"
#include "common/address_decoding_table.h"
#include "common/address_masking_table.h"
#include "common/mapping_table.h"

namespace soclib { namespace caba {

template<
	typename vci_param,
	size_t NB_INITIAT,
	size_t NB_TARGET,
	size_t MIN_LATENCY,
	size_t FIFO_DEPTH>
class VciVgmn
	: public soclib::caba::BaseModule
{
public:
	sc_in<bool> p_clk;
	sc_in<bool> p_resetn;

	soclib::caba::VciInitiator<vci_param> p_to_target[NB_TARGET];
	soclib::caba::VciTarget<vci_param> p_from_initiator[NB_INITIAT];

private:
	soclib::common::AddressDecodingTable<uint32_t, int> m_target_from_addr;
	soclib::common::AddressMaskingTable<uint32_t> m_initiator_from_srcid;

	// TARGET FSMs
	sc_signal<short int> I_ALLOC_VALUE[NB_TARGET]; // allocation register
	sc_signal<bool> I_ALLOC_STATE[NB_TARGET]; // state of the target output port


	// INITIATOR FSMs
	sc_signal<short int> T_ALLOC_VALUE[NB_INITIAT]; // allocation register
	sc_signal<bool> T_ALLOC_STATE[NB_INITIAT]; // state of the initiator output port

	//  CMD FIFOs

	sc_signal<int>  CMD_FIFO_DATA   [NB_TARGET][NB_INITIAT][FIFO_DEPTH];
	sc_signal<int>  CMD_FIFO_ADR    [NB_TARGET][NB_INITIAT][FIFO_DEPTH];
	sc_signal<int>  CMD_FIFO_CMD    [NB_TARGET][NB_INITIAT][FIFO_DEPTH];
	sc_signal<int>  CMD_FIFO_ID     [NB_TARGET][NB_INITIAT][FIFO_DEPTH];
	sc_signal<int>  CMD_FIFO_PTR    [NB_TARGET][NB_INITIAT];
	sc_signal<int>  CMD_FIFO_PTW    [NB_TARGET][NB_INITIAT];
	sc_signal<int>  CMD_FIFO_STATE  [NB_TARGET][NB_INITIAT];

	//  RSP FIFOs
	sc_signal<int>  RSP_FIFO_DATA   [NB_INITIAT][NB_TARGET][FIFO_DEPTH];
	sc_signal<int>  RSP_FIFO_CMD    [NB_INITIAT][NB_TARGET][FIFO_DEPTH];
	sc_signal<int>  RSP_FIFO_PTR    [NB_INITIAT][NB_TARGET];
	sc_signal<int>  RSP_FIFO_PTW    [NB_INITIAT][NB_TARGET];
	sc_signal<int>  RSP_FIFO_STATE  [NB_INITIAT][NB_TARGET];

	// TARGET DELAY_FIFOs
	sc_signal<int>  I_DELAY_VCIDATA[NB_TARGET][MIN_LATENCY];
	sc_signal<int>  I_DELAY_VCICMD [NB_TARGET][MIN_LATENCY];
	sc_signal<bool> I_DELAY_VALID  [NB_TARGET][MIN_LATENCY];
	sc_signal<short int> I_DELAY_PTR    [NB_TARGET];
	sc_signal<bool>  I_DELAY_ACK    [NB_TARGET];

	//  INITIATOR DELAY_FIFOs
	sc_signal<int>  T_DELAY_VCIDATA[NB_INITIAT][MIN_LATENCY];
	sc_signal<int>  T_DELAY_VCIADR [NB_INITIAT][MIN_LATENCY];
	sc_signal<int>  T_DELAY_VCICMD [NB_INITIAT][MIN_LATENCY];
	sc_signal<int>  T_DELAY_VCIID  [NB_INITIAT][MIN_LATENCY];
	sc_signal<bool> T_DELAY_VALID  [NB_INITIAT][MIN_LATENCY];
	sc_signal<short int> T_DELAY_PTR    [NB_INITIAT];

	sc_signal<int>  GLOBAL_RSP_STATE[NB_INITIAT]; //  total number of words in RSP FIFOs
	sc_signal<int>  GLOBAL_CMD_STATE[NB_TARGET];  //  total number of words in CMD FIFOs

	// TRAFIC COUNTERS
	sc_signal<int>  RSP_COUNTER    [NB_INITIAT][NB_TARGET]; // number of transmitted RSP words
	sc_signal<int>  CMD_COUNTER    [NB_TARGET][NB_INITIAT]; // number of transmitted CMD words

	void transition();
	void genMoore();

protected:
	SC_HAS_PROCESS(VciVgmn);

public:
	VciVgmn( sc_module_name name,
			 const soclib::common::MappingTable &mt);
};

}}

#endif /* SOCLIB_CABA_VCI_VGMN_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
