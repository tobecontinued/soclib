
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
#include "vci_mem_cache.h"
#include "vci_cc_xcache_wrapper.h"
#include "vci_local_ring_network.h"
#include "vci_simple_ring_network.h"
#include "virtual_dspin_network.h"

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
	typedef soclib::common::IssIss2<soclib::common::IssSimhelper<soclib::common::MipsElIss> > proc_iss;

	// Mapping table primary network

	soclib::common::MappingTable maptabp(32, IntTab(2,10), IntTab(2,3), 0x00C00000);

	maptabp.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(2,1), true));
	maptabp.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(2,1), true));
	maptabp.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(3,1), false));

	maptabp.add(Segment("mc_r0" , MC0_R_BASE , MC0_R_SIZE , IntTab(0,0), false, true, IntTab(0,0)));
	maptabp.add(Segment("mc_m0" , MC0_M_BASE , MC0_M_SIZE , IntTab(0,0), true ));
	maptabp.add(Segment("mc_r1" , MC1_R_BASE , MC1_R_SIZE , IntTab(1,0), false, true, IntTab(1,0)));
	maptabp.add(Segment("mc_m1" , MC1_M_BASE , MC1_M_SIZE , IntTab(1,0), true ));
	maptabp.add(Segment("mc_r2" , MC2_R_BASE , MC2_R_SIZE , IntTab(2,0), false, true, IntTab(2,0)));
	maptabp.add(Segment("mc_m2" , MC2_M_BASE , MC2_M_SIZE , IntTab(2,0), true ));
	maptabp.add(Segment("mc_r3" , MC3_R_BASE , MC3_R_SIZE , IntTab(3,0), false, true, IntTab(3,0)));
	maptabp.add(Segment("mc_m3" , MC3_M_BASE , MC3_M_SIZE , IntTab(3,0), true ));

	std::cout << maptabp << std::endl;

	// Mapping table coherence network

	soclib::common::MappingTable maptabc(32, IntTab(2,10), IntTab(2,3), 0x00C00000);

	maptabc.add(Segment("proc0" , PROC0_BASE , PROC0_SIZE , IntTab(0,0), false, true, IntTab(0,0)));
	maptabc.add(Segment("proc1" , PROC1_BASE , PROC1_SIZE , IntTab(0,1), false, true, IntTab(0,1)));
	maptabc.add(Segment("proc2" , PROC2_BASE , PROC2_SIZE , IntTab(0,2), false, true, IntTab(0,2)));
	maptabc.add(Segment("proc3" , PROC3_BASE , PROC3_SIZE , IntTab(0,3), false, true, IntTab(0,3)));
	maptabc.add(Segment("proc4" , PROC4_BASE , PROC4_SIZE , IntTab(1,0), false, true, IntTab(1,0)));
	maptabc.add(Segment("proc5" , PROC5_BASE , PROC5_SIZE , IntTab(1,1), false, true, IntTab(1,1)));
	maptabc.add(Segment("proc6" , PROC6_BASE , PROC6_SIZE , IntTab(1,2), false, true, IntTab(1,2)));
	maptabc.add(Segment("proc7" , PROC7_BASE , PROC7_SIZE , IntTab(1,3), false, true, IntTab(1,3)));
	maptabc.add(Segment("proc8" , PROC8_BASE , PROC8_SIZE , IntTab(2,0), false, true, IntTab(2,0)));
	maptabc.add(Segment("proc9" , PROC9_BASE , PROC9_SIZE , IntTab(2,1), false, true, IntTab(2,1)));
	maptabc.add(Segment("proc10" , PROC10_BASE , PROC10_SIZE , IntTab(2,2), false, true, IntTab(2,2)));
	maptabc.add(Segment("proc11" , PROC11_BASE , PROC11_SIZE , IntTab(2,3), false, true, IntTab(2,3)));
	maptabc.add(Segment("proc12" , PROC12_BASE , PROC12_SIZE , IntTab(3,0), false, true, IntTab(3,0)));
	maptabc.add(Segment("proc13" , PROC13_BASE , PROC13_SIZE , IntTab(3,1), false, true, IntTab(3,1)));
	maptabc.add(Segment("proc14" , PROC14_BASE , PROC14_SIZE , IntTab(3,2), false, true, IntTab(3,2)));
	maptabc.add(Segment("proc15" , PROC15_BASE , PROC15_SIZE , IntTab(3,3), false, true, IntTab(3,3)));

	std::cout << maptabc << std::endl;

	// Mapping table external network

	soclib::common::MappingTable maptabx(32, IntTab(8,4), IntTab(4,4), 0x00C00000);
	maptabx.add(Segment("xram0" , MC0_M_BASE , MC0_M_SIZE , IntTab(0,0), false));
	maptabx.add(Segment("xram1" , MC1_M_BASE , MC1_M_SIZE , IntTab(0,0), false));
	maptabx.add(Segment("xram2" , MC2_M_BASE , MC2_M_SIZE , IntTab(0,0), false));
	maptabx.add(Segment("xram3" , MC3_M_BASE , MC3_M_SIZE , IntTab(0,0), false));
	std::cout << maptabx << std::endl;

	// Signals

	sc_clock	signal_clk("clk");
	sc_signal<bool> signal_resetn("resetn");
   
	sc_signal<bool> ** signal_proc_it = soclib::common::alloc_elems<sc_signal<bool> >("signal_proc_it", 16, 6);

	soclib::caba::VciSignals<vci_param> * signal_vci_ini_proc = soclib::common::alloc_elems<soclib::caba::VciSignals<vci_param> >("signal_vci_ini_proc", 16);
	soclib::caba::VciSignals<vci_param> * signal_vci_tgt_proc = soclib::common::alloc_elems<soclib::caba::VciSignals<vci_param> >("signal_vci_tgt_proc", 16);

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_tty("vci_tgt_tty");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_rom("vci_tgt_rom");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_xram("vci_tgt_xram");

	soclib::caba::VciSignals<vci_param> * signal_vci_ixr_memc = soclib::common::alloc_elems<soclib::caba::VciSignals<vci_param> >("signal_vci_ixr_memc", 4);
	soclib::caba::VciSignals<vci_param> * signal_vci_ini_memc = soclib::common::alloc_elems<soclib::caba::VciSignals<vci_param> >("signal_vci_ini_memc", 4);
	soclib::caba::VciSignals<vci_param> * signal_vci_tgt_memc = soclib::common::alloc_elems<soclib::caba::VciSignals<vci_param> >("signal_vci_tgt_memc", 4);

	sc_signal<bool> * signal_tty_irq = soclib::common::alloc_elems<sc_signal<bool> >("signal_tty_irq", 16);

	soclib::caba::GateSignals gate_PN[4][2];
	soclib::caba::GateSignals gate_CN[4][2];



	///////////////////////////////////////////////////////////////
	// Components
	///////////////////////////////////////////////////////////////

	// VCI ports indexation : 3 initiateurs et 5 cibles

        // INIT0 : proc0_ini
        // INIT1 : memc_ini
        // INIT2 : memc_ixr

	// TGT 0 : rom_tgt
	// TGT 1 : tty_tgt
        // TGT 2 : xram_tgt
	// TGT 3 : memc_tgt
	// TGT 4 : proc0_tgt

	soclib::common::Loader loader("soft/bin.soft");

	// 16 MIPS processors (4 processors per cluster)
	soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss> * proc = (soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss > *) malloc(sizeof(soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss> )*16);
	for(int i=0; i<4; i++)
		for(int j=0; j<4; j++)
			new(&proc[4*i+j]) soclib::caba::VciCcXcacheWrapper<vci_param, proc_iss> ("proc" + (4*i+j), 4*i+j, maptabp, maptabc,IntTab(i,j),IntTab(i,j),4,64,16,4,64,16,CLEANUP_OFFSET);

	// ROM which contains boot code
	soclib::caba::VciSimpleRam<vci_param> 
	rom("rom", IntTab(2,1), maptabp, loader);

	// External RAM
	soclib::caba::VciSimpleRam<vci_param> 
	xram("xram",IntTab(0,0),maptabx, loader);

	// Distribuated memory caches (1 memory cache per cluster)
	soclib::caba::VciMemCache<vci_param> * memc = (soclib::caba::VciMemCache<vci_param> *) malloc(sizeof(soclib::caba::VciMemCache<vci_param>)*4);
	for(int i=0; i<4; i++)
		new(&memc[i]) soclib::caba::VciMemCache<vci_param>("memc" + (i),maptabp,maptabc,maptabx,IntTab(i,0),IntTab(i,0),IntTab(i,0),16,256,16);

	// Multi TTY Controller : 1 display per processor	
	soclib::caba::VciMultiTty<vci_param> 
	tty("tty",IntTab(3,1),maptabp,"tty0","tty1","tty2","tty3","tty4","tty5","tty6","tty7","tty8","tty9","tty10","tty11","tty12","tty13","tty14","tty15",NULL);

	// Local ring interconnects : 1 direct ring per cluster
	soclib::caba::VciLocalRingNetwork<vci_param> * clusterPN = (soclib::caba::VciLocalRingNetwork<vci_param> *) malloc(sizeof(soclib::caba::VciLocalRingNetwork<vci_param> )*4);
	for(int i=0; i<4; i++)
		new(&clusterPN[i]) soclib::caba::VciLocalRingNetwork<vci_param>("clusterPN" + i,maptabp, IntTab(i), 2, 18, 4, i/2+1 );

	// Local ring interconnects : 1 coherence ring per cluster
	soclib::caba::VciLocalRingNetwork<vci_param> * clusterCN = (soclib::caba::VciLocalRingNetwork<vci_param> *) malloc(sizeof(soclib::caba::VciLocalRingNetwork<vci_param> )*4);
	for(int i=0; i<4; i++)
		new(&clusterCN[i]) soclib::caba::VciLocalRingNetwork<vci_param>("clusterCN" + i,maptabc, IntTab(i), 2, 2, 1, 4 );

	// External network
	soclib::caba::VciSimpleRingNetwork<vci_param> 
	xring("xring",maptabx, IntTab(), 2, 4, 1);

	// Global interconnect : virtual dspin
	soclib::caba::VirtualDspinNetwork<2, 2, 1, 1, 37, 1, 3, 5, 6, 0, 35, 33, 1, 3, 8, 9, 0, 18, 18, 19, 24, 29, 34> network("network", 2, 2, NULL);

	///////////////////////////////////////////////////////////////
	// Net-list description
	///////////////////////////////////////////////////////////////

	// Virtual Dspin Network
	network.p_clk(signal_clk);
	network.p_resetn(signal_resetn);
	for(int i=0; i<2; i++){
		for(int j=0; j<2; j++){
			// GatePN[][1]
			network.p_out_cmd[0][i][j].data(	gate_PN[j+2*i][1].cmd_data);
			network.p_out_cmd[0][i][j].read(	gate_PN[j+2*i][1].cmd_r_wok);
			network.p_out_cmd[0][i][j].write(	gate_PN[j+2*i][1].cmd_w_rok);
			network.p_in_rsp[0][i][j].data(		gate_PN[j+2*i][1].rsp_data);
			network.p_in_rsp[0][i][j].read(		gate_PN[j+2*i][1].rsp_r_wok);
			network.p_in_rsp[0][i][j].write(	gate_PN[j+2*i][1].rsp_w_rok);
			// GateCN[][1]
			network.p_out_cmd[1][i][j].data(	gate_CN[j+2*i][1].cmd_data);
			network.p_out_cmd[1][i][j].read(	gate_CN[j+2*i][1].cmd_r_wok);
			network.p_out_cmd[1][i][j].write(	gate_CN[j+2*i][1].cmd_w_rok);
			network.p_in_rsp[1][i][j].data(		gate_CN[j+2*i][1].rsp_data);
			network.p_in_rsp[1][i][j].read(		gate_CN[j+2*i][1].rsp_r_wok);
			network.p_in_rsp[1][i][j].write(	gate_CN[j+2*i][1].rsp_w_rok);
			// GatePN[][0]
			network.p_in_cmd[0][i][j].data(		gate_PN[j+2*i][0].cmd_data);
			network.p_in_cmd[0][i][j].read(		gate_PN[j+2*i][0].cmd_r_wok);
			network.p_in_cmd[0][i][j].write(	gate_PN[j+2*i][0].cmd_w_rok);
			network.p_out_rsp[0][i][j].data(	gate_PN[j+2*i][0].rsp_data);
			network.p_out_rsp[0][i][j].read(	gate_PN[j+2*i][0].rsp_r_wok);
			network.p_out_rsp[0][i][j].write(	gate_PN[j+2*i][0].rsp_w_rok);
			// GateCN[][0]
			network.p_in_cmd[1][i][j].data(		gate_CN[j+2*i][0].cmd_data);
			network.p_in_cmd[1][i][j].read(		gate_CN[j+2*i][0].cmd_r_wok);
			network.p_in_cmd[1][i][j].write(	gate_CN[j+2*i][0].cmd_w_rok);
			network.p_out_rsp[1][i][j].data(	gate_CN[j+2*i][0].rsp_data);
			network.p_out_rsp[1][i][j].read(	gate_CN[j+2*i][0].rsp_r_wok);
			network.p_out_rsp[1][i][j].write(	gate_CN[j+2*i][0].rsp_w_rok);
		}
	}
 
	// Processors
	for(int i=0; i<16; i++){
		proc[i].p_clk(signal_clk);  
		proc[i].p_resetn(signal_resetn);  
		proc[i].p_vci_ini(signal_vci_ini_proc[i]);
		proc[i].p_vci_tgt(signal_vci_tgt_proc[i]);
		for(int j=0; j<6; j++)
			proc[i].p_irq[j](signal_proc_it[i][j]); 
	}

	// ROM
	rom.p_clk(signal_clk);
	rom.p_resetn(signal_resetn);
	rom.p_vci(signal_vci_tgt_rom);

	// TTY
	tty.p_clk(signal_clk);
	tty.p_resetn(signal_resetn);
	tty.p_vci(signal_vci_tgt_tty);
	for(int i=0; i<16; i++)
		tty.p_irq[i](signal_tty_irq[i]);

	// Memory caches
	for(int i=0; i<4; i++){
		memc[i].p_clk(signal_clk);
		memc[i].p_resetn(signal_resetn);
		memc[i].p_vci_tgt(signal_vci_tgt_memc[i]);	
		memc[i].p_vci_ini(signal_vci_ini_memc[i]);
		memc[i].p_vci_ixr(signal_vci_ixr_memc[i]);
	}

	// XRAM
	xram.p_clk(signal_clk);
        xram.p_resetn(signal_resetn);
	xram.p_vci(signal_vci_tgt_xram);	

	// XRING
	xring.p_clk(signal_clk);
	xring.p_resetn(signal_resetn);
	for(int i=0; i<4; i++)
		xring.p_to_initiator[i](signal_vci_ixr_memc[i]);
	xring.p_to_target[0](signal_vci_tgt_xram);

	// direct ring
	for(int i=0; i<4; i++){
		clusterPN[i].p_clk(signal_clk);
		clusterPN[i].p_resetn(signal_resetn);
		clusterPN[i].p_to_target[0](signal_vci_tgt_memc[i]);
	        clusterPN[i].p_gate_initiator(gate_PN[i][1]);
		clusterPN[i].p_gate_target(gate_PN[i][0]);
		for(int j=0; j<4; j++)
			clusterPN[i].p_to_initiator[j](signal_vci_ini_proc[i*4+j]);
	}

	clusterPN[2].p_to_target[1](signal_vci_tgt_rom);
	clusterPN[3].p_to_target[1](signal_vci_tgt_tty);

	// coherence ring
	for(int i=0; i<4; i++){
		clusterCN[i].p_clk(signal_clk);
		clusterCN[i].p_resetn(signal_resetn);
		clusterCN[i].p_to_initiator[0](signal_vci_ini_memc[i]);
		clusterCN[i].p_gate_initiator(gate_CN[i][1]);
		clusterCN[i].p_gate_target(gate_CN[i][0]);
		for(int j=0; j<4; j++)
			clusterCN[i].p_to_target[j](signal_vci_tgt_proc[i*4+j]);
	}

	////////////////////////////////////////////////////////

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

	for (int i = 0; i < ncycles ; i+=100000) {
		sc_start(sc_core::sc_time(100000, SC_NS));
		
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
