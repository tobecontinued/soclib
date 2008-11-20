/*
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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#include <systemc>
#include <sys/time.h>
#include <iostream>
#include <cstdlib>
#include <cstdarg>

#include "mapping_table.h"
#include "mips.h"
#include "ississ2.h"
#include "iss_simhelper.h"
#include "vci_simple_ram.h"
#include "vci_multi_tty.h"
#include "vci_vgmn.h"
#include "vci_mem_cache.h"
#include "vci_xram.h"
#include "vci_cc_xcache_wrapper.h"


//#define USE_GDB_SERVER

#ifdef USE_GDB_SERVER
#include "iss/gdbserver.h"
#endif

#include "segmentation.h"

int _main(int argc, char *argv[])
{
	using namespace sc_core;
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;

	// Define our VCI parameters
	typedef soclib::caba::VciParams<4,8,32,1,1,1,8,4,4,1> vci_param;
	typedef soclib::common::IssIss2<soclib::common::IssSimhelper<soclib::common::MipsElIss> > proc_iss;

	// Mapping table

	soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00300000);

	maptab.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(8), true));
	maptab.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(8), true));

	//maptab.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), true));  
	//maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(0), true));

	maptab.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(11), false));

	maptab.add(Segment("ccache0", CC_XCACHE0_BASE, CC_XCACHE0_SIZE, IntTab(0), false, true, IntTab(0)));
/*
	maptab.add(Segment("ccache1", CC_XCACHE1_BASE, CC_XCACHE1_SIZE, IntTab(1), false, true, IntTab(1)));
	maptab.add(Segment("ccache2", CC_XCACHE2_BASE, CC_XCACHE2_SIZE, IntTab(2), false, true, IntTab(2)));
	maptab.add(Segment("ccache3", CC_XCACHE3_BASE, CC_XCACHE3_SIZE, IntTab(3), false, true, IntTab(3)));
	maptab.add(Segment("ccache4", CC_XCACHE4_BASE, CC_XCACHE4_SIZE, IntTab(4), false, true, IntTab(4)));
	maptab.add(Segment("ccache5", CC_XCACHE5_BASE, CC_XCACHE5_SIZE, IntTab(5), false, true, IntTab(5)));
	maptab.add(Segment("ccache6", CC_XCACHE6_BASE, CC_XCACHE6_SIZE, IntTab(6), false, true, IntTab(6)));
	maptab.add(Segment("ccache7", CC_XCACHE7_BASE, CC_XCACHE7_SIZE, IntTab(7), false, true, IntTab(7)));
*/
	maptab.add(Segment("xram", XRAM_BASE, XRAM_SIZE, IntTab(9), false, true, IntTab(8)));
	maptab.add(Segment("memc_reg", MEMC_REG_BASE, MEMC_REG_SIZE, IntTab(10), false, true, IntTab(9)));
	maptab.add(Segment("memc_mem", MEMC_MEM_BASE, MEMC_MEM_SIZE, IntTab(10), true ));

	// Signals

	sc_clock	signal_clk("clk");
	sc_signal<bool> signal_resetn("resetn");
   
	sc_signal<bool> signal_proc0_it0("proc0_it0"); 
	sc_signal<bool> signal_proc0_it1("proc0_it1"); 
	sc_signal<bool> signal_proc0_it2("proc0_it2"); 
	sc_signal<bool> signal_proc0_it3("proc0_it3"); 
	sc_signal<bool> signal_proc0_it4("proc0_it4"); 
	sc_signal<bool> signal_proc0_it5("proc0_it5");
/* 
	sc_signal<bool> signal_proc1_it0("proc1_it0"); 
	sc_signal<bool> signal_proc1_it1("proc1_it1"); 
	sc_signal<bool> signal_proc1_it2("proc1_it2"); 
	sc_signal<bool> signal_proc1_it3("proc1_it3"); 
	sc_signal<bool> signal_proc1_it4("proc1_it4"); 
	sc_signal<bool> signal_proc1_it5("proc1_it5");
  
	sc_signal<bool> signal_proc2_it0("proc2_it0"); 
	sc_signal<bool> signal_proc2_it1("proc2_it1"); 
	sc_signal<bool> signal_proc2_it2("proc2_it2"); 
	sc_signal<bool> signal_proc2_it3("proc2_it3"); 
	sc_signal<bool> signal_proc2_it4("proc2_it4"); 
	sc_signal<bool> signal_proc2_it5("proc2_it5");
  
	sc_signal<bool> signal_proc3_it0("proc3_it0"); 
	sc_signal<bool> signal_proc3_it1("proc3_it1"); 
	sc_signal<bool> signal_proc3_it2("proc3_it2"); 
	sc_signal<bool> signal_proc3_it3("proc3_it3"); 
	sc_signal<bool> signal_proc3_it4("proc3_it4"); 
	sc_signal<bool> signal_proc3_it5("proc3_it5");

	sc_signal<bool> signal_proc4_it0("proc4_it0"); 
	sc_signal<bool> signal_proc4_it1("proc4_it1"); 
	sc_signal<bool> signal_proc4_it2("proc4_it2"); 
	sc_signal<bool> signal_proc4_it3("proc4_it3"); 
	sc_signal<bool> signal_proc4_it4("proc4_it4"); 
	sc_signal<bool> signal_proc4_it5("proc4_it5");

	sc_signal<bool> signal_proc5_it0("proc5_it0"); 
	sc_signal<bool> signal_proc5_it1("proc5_it1"); 
	sc_signal<bool> signal_proc5_it2("proc5_it2"); 
	sc_signal<bool> signal_proc5_it3("proc5_it3"); 
	sc_signal<bool> signal_proc5_it4("proc5_it4"); 
	sc_signal<bool> signal_proc5_it5("proc5_it5");

	sc_signal<bool> signal_proc6_it0("proc6_it0"); 
	sc_signal<bool> signal_proc6_it1("proc6_it1"); 
	sc_signal<bool> signal_proc6_it2("proc6_it2"); 
	sc_signal<bool> signal_proc6_it3("proc6_it3"); 
	sc_signal<bool> signal_proc6_it4("proc6_it4"); 
	sc_signal<bool> signal_proc6_it5("proc6_it5");

	sc_signal<bool> signal_proc7_it0("proc7_it0"); 
	sc_signal<bool> signal_proc7_it1("proc7_it1"); 
	sc_signal<bool> signal_proc7_it2("proc7_it2"); 
	sc_signal<bool> signal_proc7_it3("proc7_it3"); 
	sc_signal<bool> signal_proc7_it4("proc7_it4"); 
	sc_signal<bool> signal_proc7_it5("proc7_it5");
*/
	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc0("vci_ini_proc0");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc0("vci_tgt_proc0");
/*
	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc1("vci_ini_proc1");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc1("vci_tgt_proc1");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc2("vci_ini_proc2");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc2("vci_tgt_proc2");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc3("vci_ini_proc3");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc3("vci_tgt_proc3");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc4("vci_ini_proc4");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc4("vci_tgt_proc4");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc5("vci_ini_proc5");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc5("vci_tgt_proc5");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc6("vci_ini_proc6");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc6("vci_tgt_proc6");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc7("vci_ini_proc7");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc7("vci_tgt_proc7");
*/
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_tty("vci_tgt_tty");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_ram("vci_tgt_ram");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_xram("vci_ini_xram");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_xram("vci_tgt_xram");

	soclib::caba::VciSignals<vci_param> signal_vci_ixr_memc("vci_ixr_memc");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_memc("vci_ini_memc");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_memc("vci_tgt_memc");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0"); 
	sc_signal<bool> signal_tty_irq1("signal_tty_irq1"); 
	sc_signal<bool> signal_tty_irq2("signal_tty_irq2"); 
	sc_signal<bool> signal_tty_irq3("signal_tty_irq3");
	sc_signal<bool> signal_tty_irq4("signal_tty_irq4"); 
	sc_signal<bool> signal_tty_irq5("signal_tty_irq5"); 
	sc_signal<bool> signal_tty_irq6("signal_tty_irq6"); 
	sc_signal<bool> signal_tty_irq7("signal_tty_irq7");

	// Components

	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc0("proc0", 0, maptab,IntTab(0),IntTab(0),16,4,16,16,4,16);
/*
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc1("proc1", 1, maptab,IntTab(1),IntTab(1),16,4,16,16,4,16);
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc2("proc2", 2, maptab,IntTab(2),IntTab(2),16,4,16,16,4,16);
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc3("proc3", 3, maptab,IntTab(3),IntTab(3),16,4,16,16,4,16);
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc4("proc4", 4, maptab,IntTab(4),IntTab(4),16,4,16,16,4,16);
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc5("proc5", 5, maptab,IntTab(5),IntTab(5),16,4,16,16,4,16);
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc6("proc6", 6, maptab,IntTab(6),IntTab(6),16,4,16,16,4,16); 
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > 
	proc7("proc7", 7, maptab,IntTab(7),IntTab(7),16,4,16,16,4,16);
*/
	soclib::common::ElfLoader loader("soft/bin.soft");

	soclib::caba::VciSimpleRam<vci_param> 
	ram("vciram", IntTab(8), maptab, loader);

	soclib::caba::VciXRam<vci_param> 
	xram("xram",maptab,IntTab(8),IntTab(9),loader,16,MEMC_MEM_SIZE,2);

	soclib::caba::VciMemCache<vci_param> 
	memc("memc",maptab,IntTab(9),IntTab(10),256,16,16,MEMC_MEM_BASE,IntTab(9));
/*
	soclib::caba::VciMultiTty<vci_param> 
	tty("tty",IntTab(11),maptab,"tty0","tty1","tty2","tty3","tty4","tty5","tty6","tty7",NULL);
*/	
	soclib::caba::VciMultiTty<vci_param> 
	tty("tty",IntTab(11),maptab,"tty0",NULL);

	soclib::caba::VciVgmn<vci_param> 
	vgmn("vgmn",maptab, 10, 12, 2, 8);

	// Net-List
 
	proc0.p_clk(signal_clk);  
	proc0.p_resetn(signal_resetn);  
	proc0.p_irq[0](signal_proc0_it0); 
	proc0.p_irq[1](signal_proc0_it1); 
	proc0.p_irq[2](signal_proc0_it2); 
	proc0.p_irq[3](signal_proc0_it3); 
	proc0.p_irq[4](signal_proc0_it4); 
	proc0.p_irq[5](signal_proc0_it5); 
	proc0.p_vci_ini(signal_vci_ini_proc0);
	proc0.p_vci_tgt(signal_vci_tgt_proc0);
 /* 
	proc1.p_clk(signal_clk);  
	proc1.p_resetn(signal_resetn);  
	proc1.p_irq[0](signal_proc1_it0); 
	proc1.p_irq[1](signal_proc1_it1); 
	proc1.p_irq[2](signal_proc1_it2); 
	proc1.p_irq[3](signal_proc1_it3); 
	proc1.p_irq[4](signal_proc1_it4); 
	proc1.p_irq[5](signal_proc1_it5); 
	proc1.p_vci_ini(signal_vci_ini_proc1);
	proc1.p_vci_tgt(signal_vci_tgt_proc1);
  
	proc2.p_clk(signal_clk);  
	proc2.p_resetn(signal_resetn);  
	proc2.p_irq[0](signal_proc2_it0); 
	proc2.p_irq[1](signal_proc2_it1); 
	proc2.p_irq[2](signal_proc2_it2); 
	proc2.p_irq[3](signal_proc2_it3); 
	proc2.p_irq[4](signal_proc2_it4); 
	proc2.p_irq[5](signal_proc2_it5); 
	proc2.p_vci_ini(signal_vci_ini_proc2);
	proc2.p_vci_tgt(signal_vci_tgt_proc2);
  
	proc3.p_clk(signal_clk);  
	proc3.p_resetn(signal_resetn);  
	proc3.p_irq[0](signal_proc3_it0); 
	proc3.p_irq[1](signal_proc3_it1); 
	proc3.p_irq[2](signal_proc3_it2); 
	proc3.p_irq[3](signal_proc3_it3); 
	proc3.p_irq[4](signal_proc3_it4); 
	proc3.p_irq[5](signal_proc3_it5); 
	proc3.p_vci_ini(signal_vci_ini_proc3);
	proc3.p_vci_tgt(signal_vci_tgt_proc3);
        
	proc4.p_clk(signal_clk);  
	proc4.p_resetn(signal_resetn);  
	proc4.p_irq[0](signal_proc4_it0); 
	proc4.p_irq[1](signal_proc4_it1); 
	proc4.p_irq[2](signal_proc4_it2); 
	proc4.p_irq[3](signal_proc4_it3); 
	proc4.p_irq[4](signal_proc4_it4); 
	proc4.p_irq[5](signal_proc4_it5); 
	proc4.p_vci_ini(signal_vci_ini_proc4);
	proc4.p_vci_tgt(signal_vci_tgt_proc4);
  
	proc5.p_clk(signal_clk);  
	proc5.p_resetn(signal_resetn);  
	proc5.p_irq[0](signal_proc5_it0); 
	proc5.p_irq[1](signal_proc5_it1); 
	proc5.p_irq[2](signal_proc5_it2); 
	proc5.p_irq[3](signal_proc5_it3); 
	proc5.p_irq[4](signal_proc5_it4); 
	proc5.p_irq[5](signal_proc5_it5); 
	proc5.p_vci_ini(signal_vci_ini_proc5);
	proc5.p_vci_tgt(signal_vci_tgt_proc5);
  
	proc6.p_clk(signal_clk);  
	proc6.p_resetn(signal_resetn);  
	proc6.p_irq[0](signal_proc6_it0); 
	proc6.p_irq[1](signal_proc6_it1); 
	proc6.p_irq[2](signal_proc6_it2); 
	proc6.p_irq[3](signal_proc6_it3); 
	proc6.p_irq[4](signal_proc6_it4); 
	proc6.p_irq[5](signal_proc6_it5); 
	proc6.p_vci_ini(signal_vci_ini_proc6);
	proc6.p_vci_tgt(signal_vci_tgt_proc6);
  
	proc7.p_clk(signal_clk);  
	proc7.p_resetn(signal_resetn);  
	proc7.p_irq[0](signal_proc7_it0); 
	proc7.p_irq[1](signal_proc7_it1); 
	proc7.p_irq[2](signal_proc7_it2); 
	proc7.p_irq[3](signal_proc7_it3); 
	proc7.p_irq[4](signal_proc7_it4); 
	proc7.p_irq[5](signal_proc7_it5); 
	proc7.p_vci_ini(signal_vci_ini_proc7);
	proc7.p_vci_tgt(signal_vci_tgt_proc7);
*/
	ram.p_clk(signal_clk);
	ram.p_resetn(signal_resetn);
	ram.p_vci(signal_vci_tgt_ram);

	tty.p_clk(signal_clk);
	tty.p_resetn(signal_resetn);
	tty.p_vci(signal_vci_tgt_tty);
	tty.p_irq[0](signal_tty_irq0); 
	tty.p_irq[1](signal_tty_irq1); 
	tty.p_irq[2](signal_tty_irq2); 
	tty.p_irq[3](signal_tty_irq3);
	tty.p_irq[4](signal_tty_irq4); 
	tty.p_irq[5](signal_tty_irq5); 
	tty.p_irq[6](signal_tty_irq6); 
	tty.p_irq[7](signal_tty_irq7); 

	memc.p_clk(signal_clk);
	memc.p_resetn(signal_resetn);
	memc.p_vci_tgt(signal_vci_tgt_memc);	
	memc.p_vci_ini(signal_vci_ini_memc);
	memc.p_vci_ixr(signal_vci_ixr_memc);

	xram.p_clk(signal_clk);
        xram.p_resetn(signal_resetn);
	xram.p_vci_tgt(signal_vci_tgt_xram);	
	xram.p_vci_ini(signal_vci_ini_xram);

	vgmn.p_clk(signal_clk);
	vgmn.p_resetn(signal_resetn);

	vgmn.p_to_initiator[0](signal_vci_ini_xram);
	vgmn.p_to_initiator[1](signal_vci_ini_memc);
	vgmn.p_to_initiator[2](signal_vci_ini_proc0);
/*
	vgmn.p_to_initiator[3](signal_vci_ini_proc1);
	vgmn.p_to_initiator[4](signal_vci_ini_proc2);
	vgmn.p_to_initiator[5](signal_vci_ini_proc3);
	vgmn.p_to_initiator[6](signal_vci_ini_proc4);
	vgmn.p_to_initiator[7](signal_vci_ini_proc5);
	vgmn.p_to_initiator[8](signal_vci_ini_proc6);
	vgmn.p_to_initiator[9](signal_vci_ini_proc7);
*/
	vgmn.p_to_target[0](signal_vci_tgt_xram);
	vgmn.p_to_target[1](signal_vci_tgt_memc);
	vgmn.p_to_target[2](signal_vci_tgt_ram);
	vgmn.p_to_target[3](signal_vci_tgt_tty);
	vgmn.p_to_target[4](signal_vci_tgt_proc0);
/*
	vgmn.p_to_target[5](signal_vci_tgt_proc1);
	vgmn.p_to_target[6](signal_vci_tgt_proc2);
	vgmn.p_to_target[7](signal_vci_tgt_proc3);
	vgmn.p_to_target[8](signal_vci_tgt_proc4);	
	vgmn.p_to_target[9](signal_vci_tgt_proc5);
	vgmn.p_to_target[10](signal_vci_tgt_proc6);
	vgmn.p_to_target[11](signal_vci_tgt_proc7);
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

	sc_start(sc_core::sc_time(0, SC_NS));
	signal_resetn = false;

	sc_start(sc_core::sc_time(1, SC_NS));
	signal_resetn = true;

	for (int i = 0; i < ncycles ; i+=1000000) {
		sc_start(sc_core::sc_time(1000000, SC_NS));
		std::cout << "Time elapsed: "<<i<<" cycles." << std::endl;
		proc0.print_stats();
/*
		proc1.print_stats();
		proc2.print_stats();
		proc3.print_stats();
		proc4.print_stats();
		proc5.print_stats();
		proc6.print_stats();
		proc7.print_stats();
*/
		memc.print_stats();
	}
	return EXIT_SUCCESS;
#else
	ncycles = 1;
	sc_start(sc_core::sc_time(0, SC_NS));
	signal_resetn = false;
	sc_start(sc_core::sc_time(1, SC_NS));
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
