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
 * File : top.cpp
 * Date : 05/05/2007
 * Author :  Francois Charot
 * 
 * Copyright : IRISA
 *
 * This architecture contains:
 *  - 1 VCI Generic Micro Network (1 initiators /3 targets)
 *  - 1 NIOS2 processor with VCI cache (Data/Instruction)
 *  - 1 VCI target VCI RAM
 *  - 1 VCI target TTY display
 *********************************************************************
 */

#include <iostream>
#include <cstdlib>

#include "mapping_table.h"
#include "nios2_fast.h"
#include "iss_wrapper.h"
#include "vci_xcache.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_vgmn.h"

#include "segmentation.h"

#define TRACE_FILE 0
#define WIF_FILE 0

#define SEGTYPEMASK 0x00300000

int SIMULATION_END = 0; 

int _main (int argc, char *argv[])
{

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
	maptab.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), true));

	maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(0), true));

	maptab.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(1), false));

	///////////////////////////////////////////////////////
	//	SIGNALS DECLARATION
	//////////////////////////////////////////////////////

	sc_clock		signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");

	soclib::caba::ICacheSignals signal_nios2_icache("signal_nios2_icache");
	soclib::caba::DCacheSignals signal_nios2_dcache("signal_nios2_dcache");
	sc_signal<bool> signal_nios2_it0("signal_nios2_it0"); 
	sc_signal<bool> signal_nios2_it1("signal_nios2_it1"); 
	sc_signal<bool> signal_nios2_it2("signal_nios2_it2"); 
	sc_signal<bool> signal_nios2_it3("signal_nios2_it3"); 
	sc_signal<bool> signal_nios2_it4("signal_nios2_it4"); 
	sc_signal<bool> signal_nios2_it5("signal_nios2_it5");

	soclib::caba::VciSignals<vci_param>	signal_vci_m0("signal_vci_m0");  
	soclib::caba::VciSignals<vci_param>	signal_vci_t0("signal_vci_t0");
	soclib::caba::VciSignals<vci_param>	signal_vci_t1("signal_vci_t1");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0"); 
	sc_signal<bool>        signal_nios2_irq[32]; 

	/////////////////////////////////////////////////////////
	//	INSTANCIATED  COMPONENTS
	/////////////////////////////////////////////////////////

	soclib::caba::VciXCache<vci_param> cache ("cache", maptab, IntTab(0), 8,4,8,4);

	soclib::caba::IssWrapper<soclib::common::Nios2fIss> nios2("nios2", 0);

	soclib::common::ElfLoader loader("soft/bin.soft");

	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(1), maptab, "vcitty", NULL);

	soclib::caba::VciRam<vci_param> vcimultiram0("vcimultiram", IntTab(0), maptab, loader);

	soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptab,1,2,2,8);

//	std::cout
//	<< "Mapping table is:" << std::endl
//	<< maptab << dec << std::endl;

//	loader.print(std::cout);

	//////////////////////////////////////////////////////////
	//	Net-List
	//////////////////////////////////////////////////////////

	nios2.p_clk         	(signal_clk);  
	cache.p_clk       	(signal_clk);
	vcitty.p_clk        	(signal_clk);
	vcimultiram0.p_clk  	(signal_clk);

	nios2.p_resetn        (signal_resetn);  
	cache.p_resetn       	(signal_resetn);
	vcitty.p_resetn       (signal_resetn);
	vcimultiram0.p_resetn (signal_resetn);

	nios2.p_icache   	(signal_nios2_icache);
	nios2.p_dcache   	(signal_nios2_dcache);

	for (int i = 0; i<32; i++)
		nios2.p_irq[i]      (signal_nios2_irq[i]);

	cache.p_icache        (signal_nios2_icache);
	cache.p_dcache        (signal_nios2_dcache);
	cache.p_vci           (signal_vci_m0);

	vcimultiram0.p_vci      (signal_vci_t0);
	vcitty.p_vci      	(signal_vci_t1);

	vcitty.p_irq[0]     	(signal_tty_irq0); 

	vgmn.p_clk         	 (signal_clk);
	vgmn.p_resetn       	 (signal_resetn);
	vgmn.p_to_initiator[0]     (signal_vci_m0);
	vgmn.p_to_target[0]    (signal_vci_t0);
	vgmn.p_to_target[1]    (signal_vci_t1);

	int ncycles;
	clock_t starttime, endtime;

	//////////////////////////////////////////////////////////
	//	Traces
	//////////////////////////////////////////////////////////

#if TRACE_FILE 
	sc_trace_file *my_trace_file;
	my_trace_file = sc_create_vcd_trace_file ("system_trace");
#endif

#if WIF_FILE 
	sc_trace_file *my_trace_file;
	my_trace_file = sc_create_wif_trace_file ("system_trace");
#endif

#if TRACE_FILE || WIF_FILE
	sc_trace(my_trace_file, signal_clk, "CLK");
	sc_trace(my_trace_file, signal_resetn, "RESETN");

	//   sc_trace(my_trace_file, signal_nios2_icache.req, "ICACHE.REQ");
	//   sc_trace(my_trace_file, signal_nios2_icache.type, "ICACHE.TYPE");
	//   sc_trace(my_trace_file, signal_nios2_icache.adr, "ICACHE.ADR");
	//   sc_trace(my_trace_file, signal_nios2_icache.frz, "ICACHE.FRZ");
	//   sc_trace(my_trace_file, signal_nios2_icache.ins, "ICACHE.INS");
	//   sc_trace(my_trace_file, signal_nios2_icache.berr, "ICACHE.BERR");

	sc_trace(my_trace_file, signal_nios2_dcache.req, "DCACHE.REQ");
	sc_trace(my_trace_file, signal_nios2_dcache.type, "DCACHE.TYPE");
	sc_trace(my_trace_file, signal_nios2_dcache.adr, "DCACHE.ADR");
	sc_trace(my_trace_file, signal_nios2_dcache.frz, "DCACHE.FRZ");
	sc_trace(my_trace_file, signal_nios2_dcache.wdata, "DCACHE.WDATA");
	sc_trace(my_trace_file, signal_nios2_dcache.rdata, "DCACHE.RDATA");
	sc_trace(my_trace_file, signal_nios2_dcache.berr, "DCACHE.BERR");

	sc_trace(my_trace_file, signal_vci_m0.rspack, "INITIA.RSPACK");
	sc_trace(my_trace_file, signal_vci_m0.rspval, "INITIA.RSPVAL");
	sc_trace(my_trace_file, signal_vci_m0.rdata, "INITIA.RDATA");
	sc_trace(my_trace_file, signal_vci_m0.reop, "INITIA.REOP");
	sc_trace(my_trace_file, signal_vci_m0.rerror, "INITIA.RERROR");
	sc_trace(my_trace_file, signal_vci_m0.rsrcid, "INITIA.RSRCID");
	sc_trace(my_trace_file, signal_vci_m0.rtrdid, "INITIA.RTRDID");
	sc_trace(my_trace_file, signal_vci_m0.rpktid, "INITIA.RPKTID");

	sc_trace(my_trace_file, signal_vci_m0.cmdack, "INITIA.CMDACK");
	sc_trace(my_trace_file, signal_vci_m0.cmdval, "INITIA.CMDVAL");
	sc_trace(my_trace_file, signal_vci_m0.address, "INITIA.ADDRESS");
	sc_trace(my_trace_file, signal_vci_m0.be, "INITIA.BE");
	sc_trace(my_trace_file, signal_vci_m0.cmd, "INITIA.CMD");
	sc_trace(my_trace_file, signal_vci_m0.contig, "INITIA.CONTIG");
	sc_trace(my_trace_file, signal_vci_m0.wdata, "INITIA.WDATA");
	sc_trace(my_trace_file, signal_vci_m0.eop, "INITIA.EOP");
	sc_trace(my_trace_file, signal_vci_m0.cons, "INITIA.CONS");
	sc_trace(my_trace_file, signal_vci_m0.plen, "INITIA.PLEN");
	sc_trace(my_trace_file, signal_vci_m0.wrap, "INITIA.WRAP");
	sc_trace(my_trace_file, signal_vci_m0.cfixed, "INITIA.CFIXED");
	sc_trace(my_trace_file, signal_vci_m0.clen, "INITIA.CLEN");
	sc_trace(my_trace_file, signal_vci_m0.srcid, "INITIA.SRCID");
	sc_trace(my_trace_file, signal_vci_m0.trdid, "INITIA.TRDID");
	sc_trace(my_trace_file, signal_vci_m0.pktid, "INITIA.PKTID");

	sc_trace(my_trace_file, signal_vci_t0.rspack, "TARGET.RSPACK");
	sc_trace(my_trace_file, signal_vci_t0.rspval, "TARGET.RSPVAL");
	sc_trace(my_trace_file, signal_vci_t0.rdata, "TARGET.RDATA");
	sc_trace(my_trace_file, signal_vci_t0.reop, "TARGET.REOP");
	sc_trace(my_trace_file, signal_vci_t0.rerror, "TARGET.RERROR");
	sc_trace(my_trace_file, signal_vci_t0.rsrcid, "TARGET.RSRCID");
	sc_trace(my_trace_file, signal_vci_t0.rtrdid, "TARGET.RTRDID");
	sc_trace(my_trace_file, signal_vci_t0.rpktid, "TARGET.RPKTID");

	sc_trace(my_trace_file, signal_vci_t0.cmdack, "TARGET.CMDACK");
	sc_trace(my_trace_file, signal_vci_t0.cmdval, "TARGET.CMDVAL");
	sc_trace(my_trace_file, signal_vci_t0.address, "TARGET.ADDRESS");
	sc_trace(my_trace_file, signal_vci_t0.be, "TARGET.BE");
	sc_trace(my_trace_file, signal_vci_t0.cmd, "TARGET.CMD");
	sc_trace(my_trace_file, signal_vci_t0.contig, "TARGET.CONTIG");
	sc_trace(my_trace_file, signal_vci_t0.wdata, "TARGET.WDATA");
	sc_trace(my_trace_file, signal_vci_t0.eop, "TARGET.EOP");
	sc_trace(my_trace_file, signal_vci_t0.cons, "TARGET.CONS");
	sc_trace(my_trace_file, signal_vci_t0.plen, "TARGET.PLEN");
	sc_trace(my_trace_file, signal_vci_t0.wrap, "TARGET.WRAP");
	sc_trace(my_trace_file, signal_vci_t0.cfixed, "TARGET.CFIXED");
	sc_trace(my_trace_file, signal_vci_t0.clen, "TARGET.CLEN");
	sc_trace(my_trace_file, signal_vci_t0.srcid, "TARGET.SRCID");
	sc_trace(my_trace_file, signal_vci_t0.trdid, "TARGET.TRDID");
	sc_trace(my_trace_file, signal_vci_t0.pktid, "TARGET.PKTID");

#endif

	// Starting execution timing
	starttime = clock ();

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

		//   std::cout 
		//      << std::endl << "ncycles " << i<< std::endl;

		if((i % 10000) == 0)  
			std::cout
			<< "Time elapsed: "<<i<<" cycles." << std::endl;
	}

	endtime = clock ();

	double simtime = (1.0 * (endtime - starttime) / CLOCKS_PER_SEC);
	//  double simcycles = sc_time_stamp().to_seconds();
	std::cout << "**" << std::endl;
	//  std::cout << "** simulation time (in seconds) " << simtime  << std::endl;
	std::cout << "** simulation time (in seconds) " << simtime  << std::endl;
	std::cout << "** simulated cycles : " << ncycles-1 << "  (" << ((ncycles-1) / simtime) << " c/s)" << std::endl;
	std::cout << "**" << std::endl;


	std::cout << "Hit ENTER to end simulation" << std::endl;

#if TRACE_FILE
	sc_close_vcd_trace_file (my_trace_file);
#endif
#if WIF_FILE
	sc_close_wif_trace_file (my_trace_file);
#endif

	char buf[1];

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
	} catch (soclib::exception::Exception &e) {
		std::cout << e << std::endl;
	} catch (...) {
		std::cout << "Unknown exception occurred" << std::endl;
	}
	return 1;
}

