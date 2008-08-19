/*
 *  Copyright (c) 2008,
 *  Commissariat a l'Energie Atomique (CEA)
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   - Neither the name of CEA nor the names of its contributors may be used to
 *     endorse or promote products derived from this software without specific 
 *     prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 *  SERVICES;LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 *  SUCH DAMAGE.
 *
 * Authors: Daniel Gracia Perez (daniel.gracia-perez@cea.fr)
 * Based on code written by Nicolas Pouillon for the dma example.
 */

#include <iostream>
#include <cstdlib>

#include "mapping_table.h"
#include "arm966.h"
#include "iss_wrapper.h"
#include "vci_xcache.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_vgmn.h"

#include "segmentation.h"

void vci_trace(sc_core::sc_trace_file *file, soclib::caba::VciSignals<soclib::caba::VciParams<4,1,32,1,1,1,8,1,1,1> > &signal, const char *name) {
	sc_trace(file, signal.rspack, signal.rspack.name());
	sc_trace(file, signal.rspval, signal.rspval.name());
	sc_trace(file, signal.rdata, signal.rdata.name());
	sc_trace(file, signal.rerror, signal.rerror.name());
	sc_trace(file, signal.rsrcid, signal.rsrcid.name());
	sc_trace(file, signal.rtrdid, signal.rtrdid.name());
	sc_trace(file, signal.rpktid, signal.rpktid.name());
	sc_trace(file, signal.cmdack, signal.cmdack.name());
	sc_trace(file, signal.cmdval, signal.cmdval.name());
	sc_trace(file, signal.address, signal.address.name());
	sc_trace(file, signal.be, signal.be.name());
	sc_trace(file, signal.cmd, signal.cmd.name());
	sc_trace(file, signal.contig, signal.contig.name());
	sc_trace(file, signal.wdata, signal.wdata.name());
	sc_trace(file, signal.eop, signal.eop.name());
	sc_trace(file, signal.cons, signal.cons.name());
	sc_trace(file, signal.plen, signal.plen.name());
	sc_trace(file, signal.wrap, signal.wrap.name());
	sc_trace(file, signal.cfixed, signal.cfixed.name());
	sc_trace(file, signal.clen, signal.clen.name());
	sc_trace(file, signal.srcid, signal.srcid.name());
	sc_trace(file, signal.trdid, signal.trdid.name());
	sc_trace(file, signal.pktid, signal.pktid.name());
}

int _main(int argc, char *argv[])
{
	using namespace sc_core;
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;

	// Define our VCI parameters
	typedef soclib::caba::VciParams<4,1,32,1,1,1,8,1,1,1> vci_param;

	// Mapping table

	soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00200000);

	maptab.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(0), false));
	maptab.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), false));
  
	maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(1), false));

	maptab.add(Segment("tty" , TTY_BASE , TTY_SIZE , IntTab(2), false));
;
  


	// Signals

	sc_clock signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");
   
	soclib::caba::ICacheSignals signal_arm_icache0("signal_arm_icache0");
	soclib::caba::DCacheSignals signal_arm_dcache0("signal_arm_dcache0");


	sc_signal<bool> signal_arm0_it0("signal_arm0_it0"); 
	sc_signal<bool> signal_arm0_it1("signal_arm0_it1"); 

	soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");

	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");

	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram1("signal_vci_vcimultiram1");

	soclib::caba::VciSignals<vci_param> signal_vci_tty0("signal_vci_tty0");

	sc_signal<bool> signal_irq_tty("signal_irq_tty");


	// Components

	soclib::caba::VciXCache<vci_param> cache0("cache0", maptab,IntTab(0),1,1,1,1);

	soclib::caba::IssWrapper<soclib::common::ARM966Iss> arm0("arm0", 0);

	soclib::common::ElfLoader loader("soft/bin.soft");
	soclib::caba::VciRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, loader);

	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty", IntTab(2), maptab, "vcitty0", NULL);
 

	soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptab, 1, 3, 2, 8);

	//	Net-List

	arm0.p_clk(signal_clk);  
	cache0.p_clk(signal_clk);
	vcimultiram0.p_clk(signal_clk);
	vcimultiram1.p_clk(signal_clk);
	vcitty.p_clk(signal_clk);

	arm0.p_resetn(signal_resetn);
	cache0.p_resetn(signal_resetn);
	vcimultiram0.p_resetn(signal_resetn);
	vcimultiram1.p_resetn(signal_resetn);
	vcitty.p_resetn(signal_resetn);

	arm0.p_irq[0](signal_arm0_it0); 
	arm0.p_irq[1](signal_arm0_it1); 
	arm0.p_icache(signal_arm_icache0);
	arm0.p_dcache(signal_arm_dcache0);

	cache0.p_icache(signal_arm_icache0);
	cache0.p_dcache(signal_arm_dcache0);
	cache0.p_vci(signal_vci_m0);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);
  
	vcimultiram1.p_vci(signal_vci_vcimultiram1);
 
	vcitty.p_vci(signal_vci_tty0);
	vcitty.p_irq[0](signal_irq_tty);

	vgmn.p_clk(signal_clk);
	vgmn.p_resetn(signal_resetn);

	vgmn.p_to_initiator[0](signal_vci_m0);

	vgmn.p_to_target[0](signal_vci_vcimultiram0);
	vgmn.p_to_target[1](signal_vci_vcimultiram1);
	vgmn.p_to_target[2](signal_vci_tty0);

	/*sc_trace_file *tf;
	tf = sc_create_vcd_trace_file("trace");
	sc_trace(tf, signal_clk, "signal_clk");
	sc_trace(tf, signal_resetn, "signal_resetn");
   
//	vci_trace(tf, signal_arm_icache0, "signal_arm_icache0");
//	vci_trace(tf, signal_arm_dcache0, "signal_arm_dcache0");
	sc_trace(tf, signal_arm0_it0, "signal_arm0_it0"); 
	sc_trace(tf, signal_arm0_it1, "signal_arm0_it1"); 

//	sc_trace(tf, signal_dma_irq, "signal_dma_irq");

	vci_trace(tf, signal_vci_m0, "signal_vci_m0");

//	vci_trace(tf, signal_vci_tty, "signal_vci_tty");

	vci_trace(tf, signal_vci_vcimultiram0, "signal_vci_vcimultiram0");
//	vci_trace(tf, signal_vci_vcitimer, "signal_vci_vcitimer");
//	vci_trace(tf, signal_vci_vcifb, "signal_vci_vcifb");
	vci_trace(tf, signal_vci_vcimultiram1, "signal_vci_vcimultiram1");

//	vci_trace(tf, signal_vci_dmai, "signal_vci_dmai");
//	vci_trace(tf, signal_vci_dmat, "signal_vci_dmat");

//	sc_trace(tf, signal_tty_irq0, "signal_tty_irq0"); 
*/
	int ncycles;
#ifndef SOCVIEW
	if (argc == 2) {
		ncycles = std::atoi(argv[1]);
	} else {
		std::cerr
			<< std::endl
			<< "The number of simulation cycles must "
			   "be defined in the command line"
			<< std::endl;
		exit(1);
	}

	sc_start(0);
	signal_resetn = false;

	sc_start(1);
	signal_resetn = true;

	for (int i = 0; i < ncycles ; i++) {
		sc_start(1);
	  
		if((i % 10000) == 0) 
			std::cout
				<< "Time elapsed: "<<i<<" cycles." << std::endl;
	}
	std::cout << "Hit ENTER to end simulation" << std::endl;

	char buf[1];

	//sc_close_vcd_trace_file(tf);
	std::cin.getline(buf,2);
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

int sc_main (int argc, char *argv[])
{
	try {
		return _main(argc, argv);
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	} catch (...) {
		std::cout << "Unknown exception occured" << std::endl;
		throw;
	}
	return 1;
}
