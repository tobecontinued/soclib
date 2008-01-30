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
/* NOTE : The size of the fifo in the target must be 2 at least */ 

#include <iostream>
#include <cstdlib>

#include "mapping_table.h"
#include "mips.h"
#include "iss_wrapper.h"
#include "vci_xcache.h"
#include "vci_timer.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_locks.h"
#include "vci_ring_register.h"
#include "vci_ring_initiator_wrapper.h"
#include "vci_ring_target_wrapper.h"

//#define USE_GDB_SERVER

#ifdef USE_GDB_SERVER
#include "gdbserver.h"
#endif

#include "segmentation.h"

int _main(int argc, char *argv[])
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
  
	maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(1), true));
  
	maptab.add(Segment("loc0" , LOC0_BASE , LOC0_SIZE , IntTab(1), true));
	maptab.add(Segment("loc1" , LOC1_BASE , LOC1_SIZE , IntTab(1), true));
	maptab.add(Segment("loc2" , LOC2_BASE , LOC2_SIZE , IntTab(1), true));
	maptab.add(Segment("loc3" , LOC3_BASE , LOC3_SIZE , IntTab(1), true));
  
	maptab.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(2), false));
	maptab.add(Segment("timer", TIMER_BASE, TIMER_SIZE, IntTab(3), false));
	maptab.add(Segment("locks", LOCKS_BASE, LOCKS_SIZE, IntTab(4), false));

	// Signals

	sc_clock	signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");
   
	soclib::caba::ICacheSignals signal_mips_icache0("signal_mips_icache0");
	soclib::caba::DCacheSignals signal_mips_dcache0("signal_mips_dcache0");
	sc_signal<bool> signal_mips0_it0("signal_mips0_it0"); 
	sc_signal<bool> signal_mips0_it1("signal_mips0_it1"); 
	sc_signal<bool> signal_mips0_it2("signal_mips0_it2"); 
	sc_signal<bool> signal_mips0_it3("signal_mips0_it3"); 
	sc_signal<bool> signal_mips0_it4("signal_mips0_it4"); 
	sc_signal<bool> signal_mips0_it5("signal_mips0_it5");
  
	soclib::caba::ICacheSignals 	signal_mips_icache1("signal_mips_icache1");
	soclib::caba::DCacheSignals 	signal_mips_dcache1("signal_mips_dcache1");
	sc_signal<bool> signal_mips1_it0("signal_mips1_it0"); 
	sc_signal<bool> signal_mips1_it1("signal_mips1_it1"); 
	sc_signal<bool> signal_mips1_it2("signal_mips1_it2"); 
	sc_signal<bool> signal_mips1_it3("signal_mips1_it3"); 
	sc_signal<bool> signal_mips1_it4("signal_mips1_it4"); 
	sc_signal<bool> signal_mips1_it5("signal_mips1_it5");
  
	soclib::caba::ICacheSignals 	signal_mips_icache2("signal_mips_icache2");
	soclib::caba::DCacheSignals 	signal_mips_dcache2("signal_mips_dcache2");
	sc_signal<bool> signal_mips2_it0("signal_mips2_it0"); 
	sc_signal<bool> signal_mips2_it1("signal_mips2_it1"); 
	sc_signal<bool> signal_mips2_it2("signal_mips2_it2"); 
	sc_signal<bool> signal_mips2_it3("signal_mips2_it3"); 
	sc_signal<bool> signal_mips2_it4("signal_mips2_it4"); 
	sc_signal<bool> signal_mips2_it5("signal_mips2_it5");
  
	soclib::caba::ICacheSignals signal_mips_icache3("signal_mips_icache3");
	soclib::caba::DCacheSignals signal_mips_dcache3("signal_mips_dcache3");
	sc_signal<bool> signal_mips3_it0("signal_mips3_it0"); 
	sc_signal<bool> signal_mips3_it1("signal_mips3_it1"); 
	sc_signal<bool> signal_mips3_it2("signal_mips3_it2"); 
	sc_signal<bool> signal_mips3_it3("signal_mips3_it3"); 
	sc_signal<bool> signal_mips3_it4("signal_mips3_it4"); 
	sc_signal<bool> signal_mips3_it5("signal_mips3_it5");

	soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");
	soclib::caba::VciSignals<vci_param> signal_vci_m1("signal_vci_m1");
	soclib::caba::VciSignals<vci_param> signal_vci_m2("signal_vci_m2");
	soclib::caba::VciSignals<vci_param> signal_vci_m3("signal_vci_m3");

	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
	soclib::caba::VciSignals<vci_param> signal_vci_vcitimer("signal_vci_vcitimer");
	soclib::caba::VciSignals<vci_param> signal_vci_vcilocks("signal_vci_vcilocks");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram1("signal_vci_vcimultiram1");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0"); 
	sc_signal<bool> signal_tty_irq1("signal_tty_irq1"); 
	sc_signal<bool> signal_tty_irq2("signal_tty_irq2"); 
	sc_signal<bool> signal_tty_irq3("signal_tty_irq3"); 

	// Components

	soclib::caba::VciXCache<vci_param> cache0("cache0", maptab,IntTab(0),8,4,8,4);
	soclib::caba::VciXCache<vci_param> cache1("cache1", maptab,IntTab(1),8,4,8,4);
	soclib::caba::VciXCache<vci_param> cache2("cache2", maptab,IntTab(2),8,4,8,4);
	soclib::caba::VciXCache<vci_param> cache3("cache3", maptab,IntTab(3),8,4,8,4);

#ifdef USE_GDB_SERVER
	// uncomment this line if you want processors frozen at boot
	// soclib::common::GdbServer<soclib::common::MipsElIss>::start_frozen();

	soclib::caba::IssWrapper<soclib::common::GdbServer<soclib::common::MipsElIss> > mips0("mips0", 0);
	soclib::caba::IssWrapper<soclib::common::GdbServer<soclib::common::MipsElIss> > mips1("mips1", 1);
	soclib::caba::IssWrapper<soclib::common::GdbServer<soclib::common::MipsElIss> > mips2("mips2", 2);
	soclib::caba::IssWrapper<soclib::common::GdbServer<soclib::common::MipsElIss> > mips3("mips3", 3);
#else
	soclib::caba::IssWrapper<soclib::common::MipsElIss> mips0("mips0", 0);
	soclib::caba::IssWrapper<soclib::common::MipsElIss> mips1("mips1", 1);
	soclib::caba::IssWrapper<soclib::common::MipsElIss> mips2("mips2", 2);
	soclib::caba::IssWrapper<soclib::common::MipsElIss> mips3("mips3", 3);
#endif

	soclib::common::ElfLoader loader("soft/bin.soft");
	soclib::caba::VciMultiRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciMultiRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(2), maptab, "vcitty0", "vcitty1", "vcitty2", "vcitty3", NULL);
	soclib::caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(3), maptab, 4);
	soclib::caba::VciLocks<vci_param> vcilocks("vcilocks", IntTab(4), maptab); 

	// Ring wrappers	
	soclib::caba::VciRingInitiatorWrapper<vci_param> cache0_wrapper("cache0_wrapper");
	soclib::caba::VciRingInitiatorWrapper<vci_param> cache1_wrapper("cache1_wrapper");
	soclib::caba::VciRingInitiatorWrapper<vci_param> cache2_wrapper("cache2_wrapper");
	soclib::caba::VciRingInitiatorWrapper<vci_param> cache3_wrapper("cache3_wrapper");
	soclib::caba::VciRingTargetWrapper<vci_param> multiram0_wrapper("multiram0_wrapper",3,0xbf,0x00,0x80);
	soclib::caba::VciRingTargetWrapper<vci_param> multiram1_wrapper("multiram1_wrapper",5,0x10,0x20,0x21,0x22,0x23);
	soclib::caba::VciRingTargetWrapper<vci_param> tty_wrapper("tty_wrapper",1,0xc0);
	soclib::caba::VciRingTargetWrapper<vci_param> timer_wrapper("timer_wrapper",1,0xb0);
	soclib::caba::VciRingTargetWrapper<vci_param> locks_wrapper("locks_wrapper",1,0xb2);

	soclib::caba::RingRegister<vci_param> ringregister("ringregister");
	
	// Ring interne signals
	soclib::caba::RingSignals<vci_param> signal_ani[10];

	//	Net-List
	mips0.p_clk(signal_clk);  
	mips1.p_clk(signal_clk);  
	mips2.p_clk(signal_clk);  
	mips3.p_clk(signal_clk);  
	cache0.p_clk(signal_clk);
	cache1.p_clk(signal_clk);
	cache2.p_clk(signal_clk);
	cache3.p_clk(signal_clk);
	vcimultiram0.p_clk(signal_clk);
	vcimultiram1.p_clk(signal_clk);
	vcilocks.p_clk(signal_clk);
	vcitimer.p_clk(signal_clk);

	// wrapper clock signals
	cache0_wrapper.p_clk(signal_clk);
	cache1_wrapper.p_clk(signal_clk);
	cache2_wrapper.p_clk(signal_clk);
	cache3_wrapper.p_clk(signal_clk);
	multiram0_wrapper.p_clk(signal_clk);
	multiram1_wrapper.p_clk(signal_clk);
	tty_wrapper.p_clk(signal_clk);
	timer_wrapper.p_clk(signal_clk);
	locks_wrapper.p_clk(signal_clk);
	ringregister.p_clk(signal_clk);
  
	mips0.p_resetn(signal_resetn);  
	mips1.p_resetn(signal_resetn);  
	mips2.p_resetn(signal_resetn);  
	mips3.p_resetn(signal_resetn);  
	cache0.p_resetn(signal_resetn);
	cache1.p_resetn(signal_resetn);
	cache2.p_resetn(signal_resetn);
	cache3.p_resetn(signal_resetn);
	vcimultiram0.p_resetn(signal_resetn);
	vcimultiram1.p_resetn(signal_resetn);
	vcilocks.p_resetn(signal_resetn);
	vcitimer.p_resetn(signal_resetn);
 
	// wrapper resetn signals
	cache0_wrapper.p_resetn(signal_resetn);
	cache1_wrapper.p_resetn(signal_resetn);
	cache2_wrapper.p_resetn(signal_resetn);
	cache3_wrapper.p_resetn(signal_resetn);
	multiram0_wrapper.p_resetn(signal_resetn);
	multiram1_wrapper.p_resetn(signal_resetn);
	tty_wrapper.p_resetn(signal_resetn);
	timer_wrapper.p_resetn(signal_resetn);
	locks_wrapper.p_resetn(signal_resetn);
	ringregister.p_resetn(signal_resetn);
 
	mips0.p_irq[0](signal_mips0_it0); 
	mips0.p_irq[1](signal_mips0_it1); 
	mips0.p_irq[2](signal_mips0_it2); 
	mips0.p_irq[3](signal_mips0_it3); 
	mips0.p_irq[4](signal_mips0_it4); 
	mips0.p_irq[5](signal_mips0_it5); 
	mips0.p_icache(signal_mips_icache0);
	mips0.p_dcache(signal_mips_dcache0);
  
	mips1.p_irq[0](signal_mips1_it0); 
	mips1.p_irq[1](signal_mips1_it1); 
	mips1.p_irq[2](signal_mips1_it2); 
	mips1.p_irq[3](signal_mips1_it3); 
	mips1.p_irq[4](signal_mips1_it4); 
	mips1.p_irq[5](signal_mips1_it5); 
	mips1.p_icache(signal_mips_icache1);
	mips1.p_dcache(signal_mips_dcache1);
  
	mips2.p_irq[0](signal_mips2_it0); 
	mips2.p_irq[1](signal_mips2_it1); 
	mips2.p_irq[2](signal_mips2_it2); 
	mips2.p_irq[3](signal_mips2_it3); 
	mips2.p_irq[4](signal_mips2_it4); 
	mips2.p_irq[5](signal_mips2_it5); 
	mips2.p_icache(signal_mips_icache2);
	mips2.p_dcache(signal_mips_dcache2);
  
	mips3.p_irq[0](signal_mips3_it0); 
	mips3.p_irq[1](signal_mips3_it1); 
	mips3.p_irq[2](signal_mips3_it2); 
	mips3.p_irq[3](signal_mips3_it3); 
	mips3.p_irq[4](signal_mips3_it4); 
	mips3.p_irq[5](signal_mips3_it5); 
	mips3.p_icache(signal_mips_icache3);
	mips3.p_dcache(signal_mips_dcache3);
        
	cache0.p_icache(signal_mips_icache0);
	cache0.p_dcache(signal_mips_dcache0);
	cache0.p_vci(signal_vci_m0);

	cache1.p_icache(signal_mips_icache1);
	cache1.p_dcache(signal_mips_dcache1);
	cache1.p_vci(signal_vci_m1);

	cache2.p_icache(signal_mips_icache2);
	cache2.p_dcache(signal_mips_dcache2);
	cache2.p_vci(signal_vci_m2);

	cache3.p_icache(signal_mips_icache3);
	cache3.p_dcache(signal_mips_dcache3);
	cache3.p_vci(signal_vci_m3);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);

	vcitimer.p_vci(signal_vci_vcitimer);
	vcitimer.p_irq[0](signal_mips0_it0); 
	vcitimer.p_irq[1](signal_mips1_it0); 
	vcitimer.p_irq[2](signal_mips2_it0); 
	vcitimer.p_irq[3](signal_mips3_it0); 
  
	vcilocks.p_vci(signal_vci_vcilocks);
  
	vcimultiram1.p_vci(signal_vci_vcimultiram1);

	vcitty.p_clk(signal_clk);
	vcitty.p_resetn(signal_resetn);
	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_tty_irq0); 
	vcitty.p_irq[1](signal_tty_irq1); 
	vcitty.p_irq[2](signal_tty_irq2); 
	vcitty.p_irq[3](signal_tty_irq3); 


	// wrapper connection
	cache0_wrapper.p_ri(signal_ani[9]);
	cache0_wrapper.p_ro(signal_ani[0]);
	cache0_wrapper.p_vci(signal_vci_m0);

	cache1_wrapper.p_ri(signal_ani[0]);
	cache1_wrapper.p_ro(signal_ani[1]);
	cache1_wrapper.p_vci(signal_vci_m1);

	cache2_wrapper.p_ri(signal_ani[1]);
	cache2_wrapper.p_ro(signal_ani[2]);
	cache2_wrapper.p_vci(signal_vci_m2);

	cache3_wrapper.p_ri(signal_ani[2]);
	cache3_wrapper.p_ro(signal_ani[3]);
	cache3_wrapper.p_vci(signal_vci_m3);

	multiram0_wrapper.p_ri(signal_ani[3]);
	multiram0_wrapper.p_ro(signal_ani[4]);
	multiram0_wrapper.p_vci(signal_vci_vcimultiram0);

	multiram1_wrapper.p_ri(signal_ani[4]);
	multiram1_wrapper.p_ro(signal_ani[5]);
	multiram1_wrapper.p_vci(signal_vci_vcimultiram1);

	tty_wrapper.p_ri(signal_ani[5]);
	tty_wrapper.p_ro(signal_ani[6]);
	tty_wrapper.p_vci(signal_vci_tty);

	timer_wrapper.p_ri(signal_ani[6]);
	timer_wrapper.p_ro(signal_ani[7]);
	timer_wrapper.p_vci(signal_vci_vcitimer);

	locks_wrapper.p_ri(signal_ani[7]);
	locks_wrapper.p_ro(signal_ani[8]);
	locks_wrapper.p_vci(signal_vci_vcilocks);

	ringregister.p_ri(signal_ani[8]);
	ringregister.p_ro(signal_ani[9]);

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

	for (int i = 1; i < ncycles ; i++) {
		sc_start(1);
		if((i % 10000) == 0) 
			std::cout
				<< "Time elapsed: "<<i<<" cycles." << std::endl;
	}
	std::cout << "Hit ENTER to end simulation" << std::endl;

	char buf[1];

	std::cin.getline(buf,1);
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
		throw;
	}
	return 1;
}
