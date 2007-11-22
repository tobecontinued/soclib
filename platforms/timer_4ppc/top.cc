#include <iostream>
#include <cstdlib>

#include "common/mapping_table.h"
#include "common/iss/ppc405.h"
#include "caba/processor/iss_wrapper.h"
#include "caba/initiator/vci_xcache.h"
#include "caba/target/vci_timer.h"
#include "caba/target/vci_multi_ram.h"
#include "caba/target/vci_multi_tty.h"
#include "caba/interconnect/vci_vgmn.h"

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

	maptab.add(Segment("ppc_boot", PPC_BOOT_BASE, PPC_BOOT_SIZE, IntTab(0), false));
	maptab.add(Segment("ppc_special", PPC_SPECIAL_BASE, PPC_SPECIAL_SIZE, IntTab(0), true));
	maptab.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), true));
  
	maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(1), true));
  
	maptab.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(2), false));
	maptab.add(Segment("timer", TIMER_BASE, TIMER_SIZE, IntTab(3), false));

	// Signals

	sc_clock		signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");
   
	soclib::caba::ICacheSignals signal_ppc405_icache0("signal_ppc405_icache0");
	soclib::caba::DCacheSignals signal_ppc405_dcache0("signal_ppc405_dcache0");
	sc_signal<bool> signal_ppc4050_it0("signal_ppc4050_it0"); 
	sc_signal<bool> signal_ppc4050_it1("signal_ppc4050_it1"); 
  
	soclib::caba::ICacheSignals 	signal_ppc405_icache1("signal_ppc405_icache1");
	soclib::caba::DCacheSignals 	signal_ppc405_dcache1("signal_ppc405_dcache1");
	sc_signal<bool> signal_ppc4051_it0("signal_ppc4051_it0"); 
	sc_signal<bool> signal_ppc4051_it1("signal_ppc4051_it1"); 
  
	soclib::caba::ICacheSignals 	signal_ppc405_icache2("signal_ppc405_icache2");
	soclib::caba::DCacheSignals 	signal_ppc405_dcache2("signal_ppc405_dcache2");
	sc_signal<bool> signal_ppc4052_it0("signal_ppc4052_it0"); 
	sc_signal<bool> signal_ppc4052_it1("signal_ppc4052_it1"); 
  
	soclib::caba::ICacheSignals signal_ppc405_icache3("signal_ppc405_icache3");
	soclib::caba::DCacheSignals signal_ppc405_dcache3("signal_ppc405_dcache3");
	sc_signal<bool> signal_ppc4053_it0("signal_ppc4053_it0"); 
	sc_signal<bool> signal_ppc4053_it1("signal_ppc4053_it1"); 

	soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");
	soclib::caba::VciSignals<vci_param> signal_vci_m1("signal_vci_m1");
	soclib::caba::VciSignals<vci_param> signal_vci_m2("signal_vci_m2");
	soclib::caba::VciSignals<vci_param> signal_vci_m3("signal_vci_m3");

	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
	soclib::caba::VciSignals<vci_param> signal_vci_vcitimer("signal_vci_vcitimer");
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

	soclib::caba::IssWrapper<soclib::common::Ppc405Iss> ppc4050("ppc4050", 0);
	soclib::caba::IssWrapper<soclib::common::Ppc405Iss> ppc4051("ppc4051", 1);
	soclib::caba::IssWrapper<soclib::common::Ppc405Iss> ppc4052("ppc4052", 2);
	soclib::caba::IssWrapper<soclib::common::Ppc405Iss> ppc4053("ppc4053", 3);

	soclib::common::ElfLoader loader("soft/bin.soft");
	soclib::caba::VciMultiRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciMultiRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(2), maptab, "vcitty0", "vcitty1", "vcitty2", "vcitty3", NULL);
	soclib::caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(3), maptab, 4);
	
	soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptab, 4, 4, 2, 8);

	//	Net-List
 
	ppc4050.p_clk(signal_clk);  
	ppc4051.p_clk(signal_clk);  
	ppc4052.p_clk(signal_clk);  
	ppc4053.p_clk(signal_clk);  
	cache0.p_clk(signal_clk);
	cache1.p_clk(signal_clk);
	cache2.p_clk(signal_clk);
	cache3.p_clk(signal_clk);
	vcimultiram0.p_clk(signal_clk);
	vcimultiram1.p_clk(signal_clk);
	vcitimer.p_clk(signal_clk);
  
	ppc4050.p_resetn(signal_resetn);  
	ppc4051.p_resetn(signal_resetn);  
	ppc4052.p_resetn(signal_resetn);  
	ppc4053.p_resetn(signal_resetn);  
	cache0.p_resetn(signal_resetn);
	cache1.p_resetn(signal_resetn);
	cache2.p_resetn(signal_resetn);
	cache3.p_resetn(signal_resetn);
	vcimultiram0.p_resetn(signal_resetn);
	vcimultiram1.p_resetn(signal_resetn);
	vcitimer.p_resetn(signal_resetn);
  
	ppc4050.p_irq[0](signal_ppc4050_it0); 
	ppc4050.p_irq[1](signal_ppc4050_it1); 
	ppc4050.p_icache(signal_ppc405_icache0);
	ppc4050.p_dcache(signal_ppc405_dcache0);
  
	ppc4051.p_irq[0](signal_ppc4051_it0); 
	ppc4051.p_irq[1](signal_ppc4051_it1); 
	ppc4051.p_icache(signal_ppc405_icache1);
	ppc4051.p_dcache(signal_ppc405_dcache1);
  
	ppc4052.p_irq[0](signal_ppc4052_it0); 
	ppc4052.p_irq[1](signal_ppc4052_it1); 
	ppc4052.p_icache(signal_ppc405_icache2);
	ppc4052.p_dcache(signal_ppc405_dcache2);
  
	ppc4053.p_irq[0](signal_ppc4053_it0); 
	ppc4053.p_irq[1](signal_ppc4053_it1); 
	ppc4053.p_icache(signal_ppc405_icache3);
	ppc4053.p_dcache(signal_ppc405_dcache3);
        
	cache0.p_icache(signal_ppc405_icache0);
	cache0.p_dcache(signal_ppc405_dcache0);
	cache0.p_vci(signal_vci_m0);

	cache1.p_icache(signal_ppc405_icache1);
	cache1.p_dcache(signal_ppc405_dcache1);
	cache1.p_vci(signal_vci_m1);

	cache2.p_icache(signal_ppc405_icache2);
	cache2.p_dcache(signal_ppc405_dcache2);
	cache2.p_vci(signal_vci_m2);

	cache3.p_icache(signal_ppc405_icache3);
	cache3.p_dcache(signal_ppc405_dcache3);
	cache3.p_vci(signal_vci_m3);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);

	vcitimer.p_vci(signal_vci_vcitimer);
	vcitimer.p_irq[0](signal_ppc4050_it1); 
	vcitimer.p_irq[1](signal_ppc4051_it1); 
	vcitimer.p_irq[2](signal_ppc4052_it1); 
	vcitimer.p_irq[3](signal_ppc4053_it1); 
  
	vcimultiram1.p_vci(signal_vci_vcimultiram1);

	vcitty.p_clk(signal_clk);
	vcitty.p_resetn(signal_resetn);
	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_tty_irq0); 
	vcitty.p_irq[1](signal_tty_irq1); 
	vcitty.p_irq[2](signal_tty_irq2); 
	vcitty.p_irq[3](signal_tty_irq3); 

	vgmn.p_clk(signal_clk);
	vgmn.p_resetn(signal_resetn);

	vgmn.p_from_initiator[0](signal_vci_m0);
	vgmn.p_from_initiator[1](signal_vci_m1);
	vgmn.p_from_initiator[2](signal_vci_m2);
	vgmn.p_from_initiator[3](signal_vci_m3);

	vgmn.p_to_target[0](signal_vci_vcimultiram0);
	vgmn.p_to_target[1](signal_vci_vcimultiram1);
	vgmn.p_to_target[2](signal_vci_tty);
	vgmn.p_to_target[3](signal_vci_vcitimer);


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
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	} catch (...) {
		std::cout << "Unknown exception occured" << std::endl;
		throw;
	}
	return 1;
}
