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
 * Copyright (C) IRISA/INRIA, 2007
 *         Francois Charot <charot@irisa.fr>
 *
 */

#include <iostream>
#include <cstdlib>

#include "common/mapping_table.h"
#include "common/iss/nios2_fast.h"
#include "caba/processor/iss_wrapper.h"
#include "caba/initiator/vci_xcache.h"
#include "caba/target/vci_timer.h"
#include "caba/target/vci_multi_ram.h"
#include "caba/target/vci_multi_tty.h"
#include "caba/target/vci_locks.h"
#include "caba/interconnect/vci_vgmn.h"

#include "segmentation.h"

#define SEGTYPEMASK 0x00300000

int _main(int argc, char *argv[]) {
	using namespace sc_core;
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;

	// Define our VCI parameters
	typedef soclib::caba::VciParams<4,1,32,1,1,1,8,1,1,1> vci_param;

	// Mapping table
	soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00300000);

	maptab.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(0), true));
	maptab.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(0), true));
	maptab.add(Segment("text", TEXT_BASE, TEXT_SIZE, IntTab(0), true));

	maptab.add(Segment("data", DATA_BASE, DATA_SIZE, IntTab(1), true));

	maptab.add(Segment("loc0", LOC0_BASE, LOC0_SIZE, IntTab(1), true));
	maptab.add(Segment("loc1", LOC1_BASE, LOC1_SIZE, IntTab(1), true));
	maptab.add(Segment("loc2", LOC2_BASE, LOC2_SIZE, IntTab(1), true));
	maptab.add(Segment("loc3", LOC3_BASE, LOC3_SIZE, IntTab(1), true));

	maptab.add(Segment("tty", TTY_BASE, TTY_SIZE, IntTab(2), false));
	maptab.add(Segment("timer", TIMER_BASE, TIMER_SIZE, IntTab(3), false));
	maptab.add(Segment("locks", LOCKS_BASE, LOCKS_SIZE, IntTab(4), false));

	// Signals
	sc_clock signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");

	soclib::caba::ICacheSignals signal_nios2_icache0("signal_nios2_icache0");
	soclib::caba::DCacheSignals signal_nios2_dcache0("signal_nios2_dcache0");
	sc_signal<bool> signal_nios2_irq00("signal_nios2_irq0");
	sc_signal<bool> signal_nios2_irq0[32];

	soclib::caba::ICacheSignals signal_nios2_icache1("signal_nios2_icache1");
	soclib::caba::DCacheSignals signal_nios2_dcache1("signal_nios2_dcache1");
	sc_signal<bool> signal_nios2_irq10("signal_nios2_irq1");
	sc_signal<bool> signal_nios2_irq1[32];

	soclib::caba::ICacheSignals signal_nios2_icache2("signal_nios2_icache2");
	soclib::caba::DCacheSignals signal_nios2_dcache2("signal_nios2_dcache2");
	sc_signal<bool> signal_nios2_irq20("signal_nios2_irq2");
	sc_signal<bool> signal_nios2_irq2[32];

	soclib::caba::ICacheSignals signal_nios2_icache3("signal_nios2_icache3");
	soclib::caba::DCacheSignals signal_nios2_dcache3("signal_nios2_dcache3");
	sc_signal<bool> signal_nios2_irq30("signal_nios2_irq3");
	sc_signal<bool> signal_nios2_irq3[32];

	soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");
	soclib::caba::VciSignals<vci_param> signal_vci_m1("signal_vci_m1");
	soclib::caba::VciSignals<vci_param> signal_vci_m2("signal_vci_m2");
	soclib::caba::VciSignals<vci_param> signal_vci_m3("signal_vci_m3");

	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param>
			signal_vci_vcimultiram0("signal_vci_vcimultiram0");
	soclib::caba::VciSignals<vci_param>
			signal_vci_vcitimer("signal_vci_vcitimer");
	soclib::caba::VciSignals<vci_param>
			signal_vci_vcilocks("signal_vci_vcilocks");
	soclib::caba::VciSignals<vci_param>
			signal_vci_vcimultiram1("signal_vci_vcimultiram1");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0");
	sc_signal<bool> signal_tty_irq1("signal_tty_irq1");
	sc_signal<bool> signal_tty_irq2("signal_tty_irq2");
	sc_signal<bool> signal_tty_irq3("signal_tty_irq3");

	// Components
	soclib::caba::VciXCache<vci_param> cache0("cache0", maptab, IntTab(0), 8, 4, 8, 4);
	soclib::caba::VciXCache<vci_param> cache1("cache1", maptab, IntTab(1), 8, 4, 8, 4);
	soclib::caba::VciXCache<vci_param> cache2("cache2", maptab, IntTab(2), 8, 4, 8, 4);
	soclib::caba::VciXCache<vci_param> cache3("cache3", maptab, IntTab(3), 8, 4, 8, 4);

	soclib::caba::IssWrapper<soclib::common::Nios2fIss> nios20("nios2_0", 0);
	soclib::caba::IssWrapper<soclib::common::Nios2fIss> nios21("nios2_1", 1);
	soclib::caba::IssWrapper<soclib::common::Nios2fIss> nios22("nios2_2", 2);
	soclib::caba::IssWrapper<soclib::common::Nios2fIss> nios23("nios2_3", 3);

	soclib::common::ElfLoader loader("soft/bin.soft");
	soclib::caba::VciMultiRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciMultiRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty", IntTab(2), maptab, "vcitty0", "vcitty1", "vcitty2", "vcitty3", NULL);
	soclib::caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(3), maptab, 4);
	soclib::caba::VciLocks<vci_param> vcilocks("vcilocks", IntTab(4), maptab);

	soclib::caba::VciVgmn<vci_param> vgmn("vgmn", maptab, 4, 5, 2, 8);

	//	std::cout << "Before binding ..." << std::endl;

	//	Net-List
	nios20.p_clk(signal_clk);
	nios21.p_clk(signal_clk);
	nios22.p_clk(signal_clk);
	nios23.p_clk(signal_clk);
	cache0.p_clk(signal_clk);
	cache1.p_clk(signal_clk);
	cache2.p_clk(signal_clk);
	cache3.p_clk(signal_clk);
	vcimultiram0.p_clk(signal_clk);
	vcimultiram1.p_clk(signal_clk);
	vcilocks.p_clk(signal_clk);
	vcitimer.p_clk(signal_clk);
	vgmn.p_clk(signal_clk);
	vcitty.p_clk(signal_clk);

	nios20.p_resetn(signal_resetn);
	nios21.p_resetn(signal_resetn);
	nios22.p_resetn(signal_resetn);
	nios23.p_resetn(signal_resetn);
	cache0.p_resetn(signal_resetn);
	cache1.p_resetn(signal_resetn);
	cache2.p_resetn(signal_resetn);
	cache3.p_resetn(signal_resetn);
	vcimultiram0.p_resetn(signal_resetn);
	vcimultiram1.p_resetn(signal_resetn);
	vcilocks.p_resetn(signal_resetn);
	vcitimer.p_resetn(signal_resetn);
	vgmn.p_resetn(signal_resetn);
	vcitty.p_resetn(signal_resetn);

	nios20.p_icache(signal_nios2_icache0);
	nios20.p_dcache(signal_nios2_dcache0);
	//	  for (int i = 0; i<2; i++)
	//	  nios20.p_irq[i] (signal_nios2_irq0[i]);
	for (int i = 1; i<32; i++)
		nios20.p_irq[i](signal_nios2_irq0[i]);
	nios20.p_irq[0](signal_nios2_irq00);

	nios21.p_icache(signal_nios2_icache1);
	nios21.p_dcache(signal_nios2_dcache1);
	//	for (int i = 0; i<2; i++)
	//	  nios21.p_irq[i] (signal_nios2_irq1[i]);
	for (int i = 1; i<32; i++)
		nios21.p_irq[i](signal_nios2_irq1[i]);
	nios21.p_irq[0](signal_nios2_irq10);

	nios22.p_icache(signal_nios2_icache2);
	nios22.p_dcache(signal_nios2_dcache2);
	// 	for (int i = 0; i<2; i++)
	//	  nios22.p_irq[i] (signal_nios2_irq2[i]);
	for (int i = 1; i<32; i++)
		nios22.p_irq[i](signal_nios2_irq2[i]);
	nios22.p_irq[0](signal_nios2_irq20);

	nios23.p_icache(signal_nios2_icache3);
	nios23.p_dcache(signal_nios2_dcache3);
	//	for (int i = 0; i<2; i++)
	//	  nios23.p_irq[i] (signal_nios2_irq3[i]);
	for (int i = 1; i<32; i++)
		nios23.p_irq[i](signal_nios2_irq3[i]);
	nios23.p_irq[0](signal_nios2_irq30);

	cache0.p_icache(signal_nios2_icache0);
	cache0.p_dcache(signal_nios2_dcache0);
	cache0.p_vci(signal_vci_m0);

	cache1.p_icache(signal_nios2_icache1);
	cache1.p_dcache(signal_nios2_dcache1);
	cache1.p_vci(signal_vci_m1);

	cache2.p_icache(signal_nios2_icache2);
	cache2.p_dcache(signal_nios2_dcache2);
	cache2.p_vci(signal_vci_m2);

	cache3.p_icache(signal_nios2_icache3);
	cache3.p_dcache(signal_nios2_dcache3);
	cache3.p_vci(signal_vci_m3);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);

	vcitimer.p_vci(signal_vci_vcitimer);
	vcitimer.p_irq[0](signal_nios2_irq00);
	vcitimer.p_irq[1](signal_nios2_irq10);
	vcitimer.p_irq[2](signal_nios2_irq20);
	vcitimer.p_irq[3](signal_nios2_irq30);

	vcilocks.p_vci(signal_vci_vcilocks);

	vcimultiram1.p_vci(signal_vci_vcimultiram1);

	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_tty_irq0);
	vcitty.p_irq[1](signal_tty_irq1);
	vcitty.p_irq[2](signal_tty_irq2);
	vcitty.p_irq[3](signal_tty_irq3);

	vgmn.p_from_initiator[0](signal_vci_m0);
	vgmn.p_from_initiator[1](signal_vci_m1);
	vgmn.p_from_initiator[2](signal_vci_m2);
	vgmn.p_from_initiator[3](signal_vci_m3);

	vgmn.p_to_target[0](signal_vci_vcimultiram0);
	vgmn.p_to_target[1](signal_vci_vcimultiram1);
	vgmn.p_to_target[2](signal_vci_tty);
	vgmn.p_to_target[3](signal_vci_vcitimer);
	vgmn.p_to_target[4](signal_vci_vcilocks);

	int ncycles;
#ifndef SOCVIEW
	if (argc == 2) {
		ncycles = std::atoi(argv[1]);
	} else {
		std::cerr << std::endl << "The number of simulation cycles must "
			"be defined in the command line" << std::endl;
		exit(1);
	}

	sc_start(0);
	signal_resetn = false;

	sc_start(1);
	signal_resetn = true;

	for (int i = 0; i < ncycles; i++) {
		sc_start(1);

		if ((i % 10000) == 0)
			std::cout << "Time elapsed: "<<i<<" cycles." << std::endl;
	}
	std::cout << "Hit ENTER to end simulation" << std::endl;

	char buf[1];

	std::cin.getline(buf, 1);
	return EXIT_SUCCESS;
#else
	ncycles = 1;
	sc_start(0);
	signal_resetn = false;
	sc_start(1);
	signal_resetn = true;

	debug();
	return EXIT_SUCCESS;
#endif
}

int sc_main(int argc, char *argv[]) {
	try {
		return _main(argc, argv);
	} catch (soclib::exception::Exception &e) {
		std::cout << e << std::endl;
	} catch (...) {
		std::cout << "Unknown exception occurred" << std::endl;
		throw;
	}
	return 1;
}
