
#include <systemc>
#include <sys/time.h>
#include <iostream>
#include <cstdlib>
#include <cstdarg>

#include "mapping_table.h"
#include "mips32.h"
#include "iss2_simhelper.h"
#include "vci_simple_ram.h"
#include "vci_multi_tty.h"
#include "vci_simple_ring_network.h"
#include "vci_mem_cache2.h"
#include "vci_cc_vcache_wrapper2_ring.h"

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

	// Define VCI parameters
	typedef soclib::caba::VciParams<4,8,32,1,1,1,8,4,4,1> vci_param;
	typedef soclib::common::Iss2Simhelper<soclib::common::Mips32ElIss> proc_iss;
	// Mapping table

	soclib::common::MappingTable maptabp(32, IntTab(8), IntTab(8), 0x00300000);

	maptabp.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(2), true));
	maptabp.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(2), true));
	maptabp.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(2), true));
	maptabp.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(1), false));
	maptabp.add(Segment("mc_r" , MC_R_BASE , MC_R_SIZE , IntTab(2), false, true, IntTab(0)));
	maptabp.add(Segment("mc_m" , MC_M_BASE , MC_M_SIZE , IntTab(2), true ));
	maptabp.add(Segment("ptba" , PTD_ADDR , TAB_SIZE   , IntTab(2), true));
 
	std::cout << maptabp << std::endl;

        soclib::common::MappingTable maptabc(32, IntTab(8), IntTab(8), 0x00300000);
	maptabc.add(Segment("c_proc0" , C_PROC0_BASE , C_PROC0_SIZE , IntTab(0), false, true, IntTab(0)));
	maptabc.add(Segment("c_proc1" , C_PROC1_BASE , C_PROC1_SIZE , IntTab(1), false, true, IntTab(1)));
	maptabc.add(Segment("mc_r"    , MC_R_BASE , MC_R_SIZE , IntTab(2), false, false));
	maptabc.add(Segment("mc_m"  , MC_M_BASE , MC_M_SIZE  , IntTab(2), false, false));
	maptabc.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(2), false, false));
	maptabc.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(2), false, false));
	maptabc.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(2), false, false));
	maptabc.add(Segment("ptba" , PTD_ADDR  , TAB_SIZE  , IntTab(2), false, false));

	std::cout << maptabc << std::endl;
	
	soclib::common::MappingTable maptabx(32, IntTab(8), IntTab(8), 0x00300000);
	maptabx.add(Segment("xram" , MC_M_BASE , MC_M_SIZE , IntTab(0), false));
	maptabx.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(0), false));
	maptabx.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(0), false));
	maptabx.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), false));
	maptabx.add(Segment("ptba" , PTD_ADDR  , TAB_SIZE  , IntTab(0), false));
	
	std::cout << maptabx << std::endl;

	// Signals
	sc_clock	signal_clk("clk");
	sc_signal<bool> signal_resetn("resetn");
   
	sc_signal<bool> signal_proc0_it0("proc0_it0"); 
	sc_signal<bool> signal_proc0_it1("proc0_it1"); 
	sc_signal<bool> signal_proc0_it2("proc0_it2"); 
	sc_signal<bool> signal_proc0_it3("proc0_it3"); 
	sc_signal<bool> signal_proc0_it4("proc0_it4"); 
	sc_signal<bool> signal_proc0_it5("proc0_it5");

	sc_signal<bool> signal_proc1_it0("proc1_it0"); 
	sc_signal<bool> signal_proc1_it1("proc1_it1"); 
	sc_signal<bool> signal_proc1_it2("proc1_it2"); 
	sc_signal<bool> signal_proc1_it3("proc1_it3"); 
	sc_signal<bool> signal_proc1_it4("proc1_it4"); 
	sc_signal<bool> signal_proc1_it5("proc1_it5");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc0("vci_ini_rw_proc0");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc0("vci_ini_c_proc0");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc0("vci_tgt_proc0");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc1("vci_ini_rw_proc1");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc1("vci_ini_c_proc1");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc1("vci_tgt_proc1");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_tty("vci_tgt_tty");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_rom("vci_tgt_rom");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_xram("vci_tgt_xram");

	soclib::caba::VciSignals<vci_param> signal_vci_ixr_memc("vci_ixr_memc");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_memc("vci_ini_memc");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_memc("vci_tgt_memc");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_cleanup_memc("vci_tgt_cleanup_memc");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0"); 
	sc_signal<bool> signal_tty_irq1("signal_tty_irq1"); 

	soclib::common::Loader loader("soft/bin.soft");

        //                                  init_rw   init_c   tgt
	soclib::caba::VciCcVCacheWrapper2Ring<vci_param, proc_iss > 
	proc0("proc0", 0, maptabp, maptabc, IntTab(0),IntTab(0),IntTab(0),4,4,4,16,4,4,4,16,4,64,16,4,64,16);

	soclib::caba::VciCcVCacheWrapper2Ring<vci_param, proc_iss > 
	proc1("proc1", 1, maptabp, maptabc, IntTab(1),IntTab(1),IntTab(1),4,4,4,16,4,4,4,16,4,64,16,4,64,16);

	soclib::caba::VciSimpleRam<vci_param> 
	rom("rom", IntTab(0), maptabp, loader);

	soclib::caba::VciSimpleRam<vci_param> 
	xram("xram", IntTab(0), maptabx, loader);

        //                                  x_init    c_init    p_tgt     c_tgt
	soclib::caba::VciMemCache2<vci_param> 
	memc("memc",maptabp,maptabc,maptabx,IntTab(0),IntTab(2),IntTab(2), IntTab(2),16,256,16);
	
	soclib::caba::VciMultiTty<vci_param> 
	tty("tty",IntTab(1),maptabp,"tty0","tty1",NULL);

	soclib::caba::VciSimpleRingNetwork<vci_param> 
	ringp("ringp",maptabp, IntTab(), 2, 2, 3);

	soclib::caba::VciSimpleRingNetwork<vci_param> 
	ringc("ringc",maptabc, IntTab(), 2, 3, 3);

	soclib::caba::VciSimpleRingNetwork<vci_param> 
	ringx("ringx",maptabx, IntTab(), 2, 1, 1);

	// Net-List
 
	proc0.p_clk(signal_clk);  
	proc0.p_resetn(signal_resetn);  
	proc0.p_irq[0](signal_proc0_it0); 
	proc0.p_irq[1](signal_proc0_it1); 
	proc0.p_irq[2](signal_proc0_it2); 
	proc0.p_irq[3](signal_proc0_it3); 
	proc0.p_irq[4](signal_proc0_it4); 
	proc0.p_irq[5](signal_proc0_it5); 
	proc0.p_vci_ini_rw(signal_vci_ini_rw_proc0);
	proc0.p_vci_ini_c(signal_vci_ini_c_proc0);
	proc0.p_vci_tgt(signal_vci_tgt_proc0);

	proc1.p_clk(signal_clk);  
	proc1.p_resetn(signal_resetn);  
	proc1.p_irq[0](signal_proc1_it0); 
	proc1.p_irq[1](signal_proc1_it1); 
	proc1.p_irq[2](signal_proc1_it2); 
	proc1.p_irq[3](signal_proc1_it3); 
	proc1.p_irq[4](signal_proc1_it4); 
	proc1.p_irq[5](signal_proc1_it5); 
	proc1.p_vci_ini_rw(signal_vci_ini_rw_proc1);
	proc1.p_vci_ini_c(signal_vci_ini_c_proc1);
	proc1.p_vci_tgt(signal_vci_tgt_proc1);

	rom.p_clk(signal_clk);
	rom.p_resetn(signal_resetn);
	rom.p_vci(signal_vci_tgt_rom);

	tty.p_clk(signal_clk);
	tty.p_resetn(signal_resetn);
	tty.p_vci(signal_vci_tgt_tty);
	tty.p_irq[0](signal_tty_irq0); 
	tty.p_irq[1](signal_tty_irq1); 

	memc.p_clk(signal_clk);
	memc.p_resetn(signal_resetn);
	memc.p_vci_tgt(signal_vci_tgt_memc);
	memc.p_vci_tgt_cleanup(signal_vci_tgt_cleanup_memc);
	memc.p_vci_ini(signal_vci_ini_memc);
	memc.p_vci_ixr(signal_vci_ixr_memc);

	xram.p_clk(signal_clk);
        xram.p_resetn(signal_resetn);
	xram.p_vci(signal_vci_tgt_xram);
	
	ringp.p_clk(signal_clk);
	ringp.p_resetn(signal_resetn);

	ringc.p_clk(signal_clk);
	ringc.p_resetn(signal_resetn);

	ringx.p_clk(signal_clk);
	ringx.p_resetn(signal_resetn);

	ringp.p_to_initiator[0](signal_vci_ini_rw_proc0);
	ringp.p_to_initiator[1](signal_vci_ini_rw_proc1);

	ringc.p_to_initiator[2](signal_vci_ini_memc);
	ringc.p_to_initiator[0](signal_vci_ini_c_proc0);
	ringc.p_to_initiator[1](signal_vci_ini_c_proc1);

	ringx.p_to_initiator[0](signal_vci_ixr_memc);

	ringp.p_to_target[0](signal_vci_tgt_rom);
	ringp.p_to_target[1](signal_vci_tgt_tty);
	ringp.p_to_target[2](signal_vci_tgt_memc);

	ringc.p_to_target[0](signal_vci_tgt_proc0);
	ringc.p_to_target[1](signal_vci_tgt_proc1);
	ringc.p_to_target[2](signal_vci_tgt_cleanup_memc);

	ringx.p_to_target[0](signal_vci_tgt_xram);

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

	for (int i = 0; i < ncycles ; i+=1000) {
		sc_start(sc_core::sc_time(1000, SC_NS));
		//proc.print_stats();
		//memc.print_stats();
	}

        std::cout << "Hit ENTER to end simulation" << std::endl;
        char buf[1];

	std::cin.getline(buf,1);

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
