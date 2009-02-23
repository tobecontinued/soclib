
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
#include "vci_vgmn.h"
#include "vci_mem_cache.h"
#include "vci_cc_vcache_wrapper2.h"


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
	//typedef soclib::common::IssIss2<soclib::common::IssSimhelper<soclib::common::MipsElIss> > proc_iss;
	typedef soclib::common::Iss2Simhelper<soclib::common::Mips32ElIss> proc_iss;
	// Mapping table

	soclib::common::MappingTable maptabp(32, IntTab(8), IntTab(8), 0x00300000);

	maptabp.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(0), true));
	maptabp.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(0), true));
	maptabp.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), true));
	maptabp.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(1), false));
	maptabp.add(Segment("mc_r" , MC_R_BASE , MC_R_SIZE , IntTab(2), false, true, IntTab(0)));
	maptabp.add(Segment("mc_m" , MC_M_BASE , MC_M_SIZE , IntTab(2), true ));
	maptabp.add(Segment("ptba" ,  PTD_ADDR , TAB_SIZE  , IntTab(3), false));

	std::cout << maptabp << std::endl;

        soclib::common::MappingTable maptabc(32, IntTab(8), IntTab(8), 0x00300000);
	maptabc.add(Segment("proc0" , PROC0_BASE , PROC0_SIZE , IntTab(0), false, true, IntTab(0)));
	maptabc.add(Segment("proc1" , PROC1_BASE , PROC1_SIZE , IntTab(1), false, true, IntTab(1)));
	std::cout << maptabc << std::endl;
	
	soclib::common::MappingTable maptabx(32, IntTab(8), IntTab(8), 0x00300000);
	maptabx.add(Segment("xram" , MC_M_BASE , MC_M_SIZE , IntTab(0), false));
	
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

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc0("vci_ini_proc0");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc0("vci_tgt_proc0");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_proc1("vci_ini_proc1");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc1("vci_tgt_proc1");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_tty("vci_tgt_tty");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_rom("vci_tgt_rom");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_xram("vci_tgt_xram");

	soclib::caba::VciSignals<vci_param> signal_vci_ixr_memc("vci_ixr_memc");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_memc("vci_ini_memc");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_memc("vci_tgt_memc");

	soclib::caba::VciSignals<vci_param> signal_vci_vcimultipagt("signal_vci_vcimultipagt");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0"); 
	sc_signal<bool> signal_tty_irq1("signal_tty_irq1"); 

	// Components
	// VCI ports indexation : 3 intiateurs et 5 cibles

        // INIT0 : proc_ini
        // INIT1 : memc_ini
        // INIT2 : memc_ixr

	// TGT 0 : rom_tgt
	// TGT 1 : tty_tgt
        // TGT 2 : xram_tgt
	// TGT 3 : memc_tgt
	// TGT 4 : proc_tgt

	soclib::common::Loader loader("soft/bin.soft");

	soclib::caba::VciCcVCacheWrapper2<vci_param, proc_iss > 
	//proc("proc", 0, maptab,IntTab(0),IntTab(4),4,64,16,4,64,16);
	proc0("proc0", 0, maptabp, maptabc, IntTab(0),IntTab(0),4,4,4,16,4,4,4,16,4,64,16,4,64,16,CLEANUP_OFFSET);

	soclib::caba::VciCcVCacheWrapper2<vci_param, proc_iss > 
	proc1("proc1", 1, maptabp, maptabc, IntTab(1),IntTab(1),4,4,4,16,4,4,4,16,4,64,16,4,64,16,CLEANUP_OFFSET);

	soclib::caba::VciSimpleRam<vci_param> 
	rom("rom", IntTab(0), maptabp, loader);

	soclib::caba::VciSimpleRam<vci_param> 
	xram("xram", IntTab(0), maptabx, loader);

	soclib::caba::VciMemCache<vci_param> 
	memc("memc",maptabp,maptabc,maptabx,IntTab(0),IntTab(0),IntTab(2),16,256,16);
	
	soclib::caba::VciMultiTty<vci_param> 
	tty("tty",IntTab(1),maptabp,"tty0","tty1",NULL);

	soclib::caba::VciSimpleRam<vci_param> 
	vcimultipagt("vcimultipagt", IntTab(3), maptabp, loader);

	soclib::caba::VciVgmn<vci_param> 
	vgmnp("vgmnp",maptabp, 2, 4, 1, 8);

	soclib::caba::VciVgmn<vci_param> 
	vgmnc("vgmnc",maptabc, 1, 2, 1, 8);

	soclib::caba::VciVgmn<vci_param> 
	vgmnx("vgmnx",maptabx, 1, 1, 1, 8);

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
	memc.p_vci_ini(signal_vci_ini_memc);
	memc.p_vci_ixr(signal_vci_ixr_memc);

	xram.p_clk(signal_clk);
        xram.p_resetn(signal_resetn);
	xram.p_vci(signal_vci_tgt_xram);
	
	vcimultipagt.p_clk(signal_clk);
	vcimultipagt.p_resetn(signal_resetn);
	vcimultipagt.p_vci(signal_vci_vcimultipagt);

	vgmnp.p_clk(signal_clk);
	vgmnp.p_resetn(signal_resetn);

	vgmnc.p_clk(signal_clk);
	vgmnc.p_resetn(signal_resetn);

	vgmnx.p_clk(signal_clk);
	vgmnx.p_resetn(signal_resetn);

	vgmnp.p_to_initiator[0](signal_vci_ini_proc0);
	vgmnp.p_to_initiator[1](signal_vci_ini_proc1);
	vgmnc.p_to_initiator[0](signal_vci_ini_memc);
	vgmnx.p_to_initiator[0](signal_vci_ixr_memc);

	vgmnp.p_to_target[0](signal_vci_tgt_rom);
	vgmnp.p_to_target[1](signal_vci_tgt_tty);
	vgmnp.p_to_target[2](signal_vci_tgt_memc);
	vgmnp.p_to_target[3](signal_vci_vcimultipagt);


	vgmnc.p_to_target[0](signal_vci_tgt_proc0);
	vgmnc.p_to_target[1](signal_vci_tgt_proc1);
	vgmnx.p_to_target[0](signal_vci_tgt_xram);


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
