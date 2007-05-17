#include <iostream>
#include <cstdlib>

#include "common/mapping_table.h"
#include "caba/processor/mips.h"
#include "caba/initiator/vci_xcache.h"
#include "caba/target/vci_multi_ram.h"
#include "caba/target/vci_multi_tty.h"
#include "caba/interconnect/vci_pi_initiator_wrapper.h"
#include "caba/interconnect/vci_pi_target_wrapper.h"
#include "caba/interconnect/pibus_bcu.h"

#include "segmentation.h"

#define SEGTYPEMASK 0x00300000

int _main(int argc, char *argv[])
{
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
  	maptab.add(Segment("loc0" , LOC0_BASE , LOC0_SIZE , IntTab(0), true));

	maptab.add(Segment("tty"  , TTY_BASE , TTY_SIZE , IntTab(1), false));

	// Signals
	sc_clock		signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");
   
	soclib::caba::ICacheSignals signal_mips_icache0("signal_mips_icache0");
	soclib::caba::DCacheSignals signal_mips_dcache0("signal_mips_dcache0");
	sc_signal<bool> signal_mips0_it0("signal_mips0_it0"); 
	sc_signal<bool> signal_mips0_it1("signal_mips0_it1"); 
	sc_signal<bool> signal_mips0_it2("signal_mips0_it2"); 
	sc_signal<bool> signal_mips0_it3("signal_mips0_it3"); 
	sc_signal<bool> signal_mips0_it4("signal_mips0_it4"); 
	sc_signal<bool> signal_mips0_it5("signal_mips0_it5");

	soclib::caba::Pibus pibus("pibus");
	sc_signal<bool> req("req");
	sc_signal<bool> gnt("gnt");
	sc_signal<bool> sel0("sel0");
	sc_signal<bool> sel1("sel1");
  
	soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");

	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");

	// Components

	soclib::caba::VciXCache<8,vci_param> cache0("cache0", maptab,0,8,4,8,4);

	soclib::caba::Mips mips0("mips0", 0);

	soclib::common::ElfLoader loader("hello/a.out");
	soclib::caba::VciMultiRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(1), maptab, "vcitty0", NULL);
	
	soclib::caba::PibusBcu bcu("bcu", maptab, 1, 2, 100);
	soclib::caba::VciPiInitiatorWrapper<vci_param> cache_wrapper("cache_wrapper");
	soclib::caba::VciPiTargetWrapper<vci_param> multiram_wrapper("multiram_wrapper");
	soclib::caba::VciPiTargetWrapper<vci_param> tty_wrapper("tty_wrapper");

	//	Net-List
 
	mips0.p_clk(signal_clk);  
	cache0.p_clk(signal_clk);
	vcimultiram0.p_clk(signal_clk);
	vcitty.p_clk(signal_clk);
	cache_wrapper.p_clk(signal_clk);
	multiram_wrapper.p_clk(signal_clk);
	tty_wrapper.p_clk(signal_clk);
	bcu.p_clk(signal_clk);

	mips0.p_resetn(signal_resetn);  
	cache0.p_resetn(signal_resetn);
	vcimultiram0.p_resetn(signal_resetn);
	vcitty.p_resetn(signal_resetn);
	cache_wrapper.p_resetn(signal_resetn);
	multiram_wrapper.p_resetn(signal_resetn);
	tty_wrapper.p_resetn(signal_resetn);
	bcu.p_resetn(signal_resetn);
  
	mips0.p_irq[0](signal_mips0_it0); 
	mips0.p_irq[1](signal_mips0_it1); 
	mips0.p_irq[2](signal_mips0_it2); 
	mips0.p_irq[3](signal_mips0_it3); 
	mips0.p_irq[4](signal_mips0_it4); 
	mips0.p_irq[5](signal_mips0_it5); 
	mips0.p_icache(signal_mips_icache0);
	mips0.p_dcache(signal_mips_dcache0);
  
	cache0.p_icache(signal_mips_icache0);
	cache0.p_dcache(signal_mips_dcache0);
	cache0.p_vci(signal_vci_m0);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);

	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_mips0_it0); 

	cache_wrapper.p_gnt(gnt);
	cache_wrapper.p_req(req);
	cache_wrapper.p_pi(pibus);
	cache_wrapper.p_vci(signal_vci_m0);

	multiram_wrapper.p_sel(sel0);
	multiram_wrapper.p_pi(pibus);
	multiram_wrapper.p_vci(signal_vci_vcimultiram0);

	tty_wrapper.p_sel(sel1);
	tty_wrapper.p_pi(pibus);
	tty_wrapper.p_vci(signal_vci_tty);

	bcu.p_req[0](req);
	bcu.p_gnt[0](gnt);
	bcu.p_sel[0](sel0);
	bcu.p_sel[1](sel1);

	bcu.p_a(pibus.a);
	bcu.p_lock(pibus.lock);
	bcu.p_ack(pibus.ack);
	bcu.p_tout(pibus.tout);

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
	  
		if( 1 || (i % 10000) == 0) 
			std::cout
				<< "Time elapsed: "<<i<<" cycles." << std::endl;
	}
	std::cout << "Hit ENTER to end simulation" << std::endl;

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
		throw;
	}
	return 1;
}
