
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
#include "vci_mem_cache_v4.h"
#include "vci_cc_xcache_wrapper_v1.h"
#include "vci_local_ring_fast.h"
#include "vci_vgmn.h"
#include "virtual_dspin_router.h"

#ifdef USE_GDB_SERVER
#include "iss/gdbserver.h"
#endif

#include "segmentation.h"

#define X_MAX           2
#define Y_MAX           2
#define N_PROCS         16
#define N_CLUSTERS      X_MAX*Y_MAX
// Router ports index
#define NORTH		0
#define SOUTH		1
#define EAST		2
#define WEST		3
#define LOCAL		4

// DSPIN flit widths
#define cmd_width	40
#define rsp_width	33

// VCI address format
#define adr_width	32
#define x_width		1 //4
#define y_width		1 //4
#define l_width		4

//  cluster index (from coordinates)
#define cluster(x,y)	(y + Y_MAX*x)
int _main(int argc, char *argv[])
{
	using namespace sc_core;
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;
	using namespace soclib::caba;

	// Define VCI parameters
	typedef soclib::caba::VciParams<4,8,32,2,1,1,8,4,4,1> vci_param;
        typedef soclib::common::Iss2Simhelper<soclib::common::Mips32ElIss> proc_iss;
	
	// Mapping table

	soclib::common::MappingTable maptabp(32, IntTab(2,10), IntTab(2,6), 0x00F00000);

	maptabp.add(Segment("text"   , TEXT_BASE   , TEXT_SIZE   , IntTab(0,0), true));
	maptabp.add(Segment("data0"  , DATA0_BASE  , DATA0_SIZE  , IntTab(0,0), true));
	maptabp.add(Segment("data1"  , DATA1_BASE  , DATA1_SIZE  , IntTab(1,0), true));
	maptabp.add(Segment("data2"  , DATA2_BASE  , DATA2_SIZE  , IntTab(2,0), true));
	maptabp.add(Segment("data3"  , DATA3_BASE  , DATA3_SIZE  , IntTab(3,0), true));
	maptabp.add(Segment("reset"  , RESET_BASE  , RESET_SIZE  , IntTab(2,0), true));
	maptabp.add(Segment("excep"  , EXCEP_BASE  , EXCEP_SIZE  , IntTab(2,0), true));
	maptabp.add(Segment("tty"    , TTY_BASE    , TTY_SIZE    , IntTab(3,1), false));

	std::cout << maptabp << std::endl;

	soclib::common::MappingTable maptabc(32, IntTab(2,10), IntTab(2,6), 0x00F00000);

	maptabc.add(Segment("proc0"  , PROC0_BASE  , PROC0_SIZE  , IntTab(0,0), false, true, IntTab(0,0)));
	maptabc.add(Segment("proc1"  , PROC1_BASE  , PROC1_SIZE  , IntTab(0,1), false, true, IntTab(0,1)));
	maptabc.add(Segment("proc2"  , PROC2_BASE  , PROC2_SIZE  , IntTab(0,2), false, true, IntTab(0,2)));
	maptabc.add(Segment("proc3"  , PROC3_BASE  , PROC3_SIZE  , IntTab(0,3), false, true, IntTab(0,3)));
	maptabc.add(Segment("proc4"  , PROC4_BASE  , PROC4_SIZE  , IntTab(1,0), false, true, IntTab(1,0)));
	maptabc.add(Segment("proc5"  , PROC5_BASE  , PROC5_SIZE  , IntTab(1,1), false, true, IntTab(1,1)));
	maptabc.add(Segment("proc6"  , PROC6_BASE  , PROC6_SIZE  , IntTab(1,2), false, true, IntTab(1,2)));
	maptabc.add(Segment("proc7"  , PROC7_BASE  , PROC7_SIZE  , IntTab(1,3), false, true, IntTab(1,3)));
	maptabc.add(Segment("proc8"  , PROC8_BASE  , PROC8_SIZE  , IntTab(2,0), false, true, IntTab(2,0)));
	maptabc.add(Segment("proc9"  , PROC9_BASE  , PROC9_SIZE  , IntTab(2,1), false, true, IntTab(2,1)));
	maptabc.add(Segment("proc10" , PROC10_BASE , PROC10_SIZE , IntTab(2,2), false, true, IntTab(2,2)));
	maptabc.add(Segment("proc11" , PROC11_BASE , PROC11_SIZE , IntTab(2,3), false, true, IntTab(2,3)));
	maptabc.add(Segment("proc12" , PROC12_BASE , PROC12_SIZE , IntTab(3,0), false, true, IntTab(3,0)));
	maptabc.add(Segment("proc13" , PROC13_BASE , PROC13_SIZE , IntTab(3,1), false, true, IntTab(3,1)));
	maptabc.add(Segment("proc14" , PROC14_BASE , PROC14_SIZE , IntTab(3,2), false, true, IntTab(3,2)));
	maptabc.add(Segment("proc15" , PROC15_BASE , PROC15_SIZE , IntTab(3,3), false, true, IntTab(3,3)));
        maptabc.add(Segment("reset"  , RESET_BASE  , RESET_SIZE  , IntTab(2,4), false, false));
	maptabc.add(Segment("excep"  , EXCEP_BASE  , EXCEP_SIZE  , IntTab(2,4), false, false));
        maptabc.add(Segment("text"   , TEXT_BASE   , TEXT_SIZE   , IntTab(0,4), false, false));
	maptabc.add(Segment("data0"  , DATA0_BASE  , DATA0_SIZE  , IntTab(0,4), false,false));
	maptabc.add(Segment("data1"  , DATA1_BASE  , DATA1_SIZE  , IntTab(1,4), false,false));
	maptabc.add(Segment("data2"  , DATA2_BASE  , DATA2_SIZE  , IntTab(2,4), false,false));
	maptabc.add(Segment("data3"  , DATA3_BASE  , DATA3_SIZE  , IntTab(3,4), false,false));

	std::cout << maptabc << std::endl;

	soclib::common::MappingTable maptabx(32, IntTab(8), IntTab(8), 0x00F00000);
       	maptabx.add(Segment("reset" , RESET_BASE , RESET_SIZE , IntTab(0), false));
	maptabx.add(Segment("excep" , EXCEP_BASE , EXCEP_SIZE , IntTab(0), false));
	maptabx.add(Segment("text"  , TEXT_BASE  , TEXT_SIZE  , IntTab(0), false));
	maptabx.add(Segment("data0"  , DATA0_BASE  , DATA0_SIZE  , IntTab(0), false));
	maptabx.add(Segment("data1"  , DATA1_BASE  , DATA1_SIZE  , IntTab(0), false));
	maptabx.add(Segment("data2"  , DATA2_BASE  , DATA2_SIZE  , IntTab(0), false));
	maptabx.add(Segment("data3"  , DATA3_BASE  , DATA3_SIZE  , IntTab(0), false));

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

	sc_signal<bool> signal_proc8_it0("proc8_it0"); 
	sc_signal<bool> signal_proc8_it1("proc8_it1"); 
	sc_signal<bool> signal_proc8_it2("proc8_it2"); 
	sc_signal<bool> signal_proc8_it3("proc8_it3"); 
	sc_signal<bool> signal_proc8_it4("proc8_it4"); 
	sc_signal<bool> signal_proc8_it5("proc8_it5");

	sc_signal<bool> signal_proc9_it0("proc9_it0"); 
	sc_signal<bool> signal_proc9_it1("proc9_it1"); 
	sc_signal<bool> signal_proc9_it2("proc9_it2"); 
	sc_signal<bool> signal_proc9_it3("proc9_it3"); 
	sc_signal<bool> signal_proc9_it4("proc9_it4"); 
	sc_signal<bool> signal_proc9_it5("proc9_it5");

	sc_signal<bool> signal_proc10_it0("proc10_it0"); 
	sc_signal<bool> signal_proc10_it1("proc10_it1"); 
	sc_signal<bool> signal_proc10_it2("proc10_it2"); 
	sc_signal<bool> signal_proc10_it3("proc10_it3"); 
	sc_signal<bool> signal_proc10_it4("proc10_it4"); 
	sc_signal<bool> signal_proc10_it5("proc10_it5");

	sc_signal<bool> signal_proc11_it0("proc11_it0"); 
	sc_signal<bool> signal_proc11_it1("proc11_it1"); 
	sc_signal<bool> signal_proc11_it2("proc11_it2"); 
	sc_signal<bool> signal_proc11_it3("proc11_it3"); 
	sc_signal<bool> signal_proc11_it4("proc11_it4"); 
	sc_signal<bool> signal_proc11_it5("proc11_it5");

	sc_signal<bool> signal_proc12_it0("proc12_it0"); 
	sc_signal<bool> signal_proc12_it1("proc12_it1"); 
	sc_signal<bool> signal_proc12_it2("proc12_it2"); 
	sc_signal<bool> signal_proc12_it3("proc12_it3"); 
	sc_signal<bool> signal_proc12_it4("proc12_it4"); 
	sc_signal<bool> signal_proc12_it5("proc12_it5");

	sc_signal<bool> signal_proc13_it0("proc13_it0"); 
	sc_signal<bool> signal_proc13_it1("proc13_it1"); 
	sc_signal<bool> signal_proc13_it2("proc13_it2"); 
	sc_signal<bool> signal_proc13_it3("proc13_it3"); 
	sc_signal<bool> signal_proc13_it4("proc13_it4"); 
	sc_signal<bool> signal_proc13_it5("proc13_it5");

	sc_signal<bool> signal_proc14_it0("proc14_it0"); 
	sc_signal<bool> signal_proc14_it1("proc14_it1"); 
	sc_signal<bool> signal_proc14_it2("proc14_it2"); 
	sc_signal<bool> signal_proc14_it3("proc14_it3"); 
	sc_signal<bool> signal_proc14_it4("proc14_it4"); 
	sc_signal<bool> signal_proc14_it5("proc14_it5");

	sc_signal<bool> signal_proc15_it0("proc15_it0"); 
	sc_signal<bool> signal_proc15_it1("proc15_it1"); 
	sc_signal<bool> signal_proc15_it2("proc15_it2"); 
	sc_signal<bool> signal_proc15_it3("proc15_it3"); 
	sc_signal<bool> signal_proc15_it4("proc15_it4"); 
	sc_signal<bool> signal_proc15_it5("proc15_it5");

//--- Dspin router 
	DspinSignals<cmd_width>*** signal_dspin_false_cmd_in = alloc_elems<DspinSignals<cmd_width> >("signal_dspin_false_cmd_in", (Y_MAX*X_MAX), 2, 2);
	DspinSignals<cmd_width>*** signal_dspin_false_cmd_out = alloc_elems<DspinSignals<cmd_width> >("signal_dspin_false_cmd_out", (Y_MAX*X_MAX), 2, 2);
	DspinSignals<rsp_width>*** signal_dspin_false_rsp_in = alloc_elems<DspinSignals<rsp_width> >("signal_dspin_false_rsp_in", (Y_MAX*X_MAX), 2, 2);
	DspinSignals<rsp_width>*** signal_dspin_false_rsp_out = alloc_elems<DspinSignals<rsp_width> >("signal_dspin_false_rsp_out", (Y_MAX*X_MAX), 2, 2);


	// DSPIN signals between local ring & global interconnects
	DspinSignals<cmd_width>** signal_dspin_cmd_l2g_d =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_cmd_l2g_d", X_MAX, Y_MAX);
	DspinSignals<cmd_width>** signal_dspin_cmd_g2l_d =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_cmd_g2l_d", X_MAX, Y_MAX);
	
	DspinSignals<cmd_width>** signal_dspin_cmd_l2g_c =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_cmd_l2g_c", X_MAX, Y_MAX);
	DspinSignals<cmd_width>** signal_dspin_cmd_g2l_c =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_cmd_g2l_c", X_MAX, Y_MAX);
	
	DspinSignals<rsp_width>** signal_dspin_rsp_l2g_d =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_rsp_l2g_d", X_MAX, Y_MAX);
	DspinSignals<rsp_width>** signal_dspin_rsp_g2l_d =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_rsp_g2l_d", X_MAX, Y_MAX);
	
	DspinSignals<rsp_width>** signal_dspin_rsp_l2g_c =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_rsp_l2g_c", X_MAX, Y_MAX);
	DspinSignals<rsp_width>** signal_dspin_rsp_g2l_c =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_rsp_g2l_c", X_MAX, Y_MAX);
	
	// Horizontal inter-clusters DSPIN signals
	DspinSignals<cmd_width>*** signal_dspin_h_cmd_inc =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_h_cmd_inc", X_MAX-1, Y_MAX, 2);
	DspinSignals<cmd_width>*** signal_dspin_h_cmd_dec =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_h_cmd_dec", X_MAX-1, Y_MAX, 2);
	DspinSignals<rsp_width>*** signal_dspin_h_rsp_inc =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_h_rsp_inc", X_MAX-1, Y_MAX, 2);
	DspinSignals<rsp_width>*** signal_dspin_h_rsp_dec =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_h_rsp_dec", X_MAX-1, Y_MAX, 2);
	
	// Vertical inter-clusters DSPIN signals
	DspinSignals<cmd_width>*** signal_dspin_v_cmd_inc =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_v_cmd_inc", X_MAX, Y_MAX-1, 2);
	DspinSignals<cmd_width>*** signal_dspin_v_cmd_dec =
	    alloc_elems<DspinSignals<cmd_width> >("signal_dspin_v_cmd_dec", X_MAX, Y_MAX-1, 2);
	DspinSignals<rsp_width>*** signal_dspin_v_rsp_inc =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_v_rsp_inc", X_MAX, Y_MAX-1, 2);
	DspinSignals<rsp_width>*** signal_dspin_v_rsp_dec =
	    alloc_elems<DspinSignals<rsp_width> >("signal_dspin_v_rsp_dec", X_MAX, Y_MAX-1, 2);

        soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc0("vci_ini_rw_proc0");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc0("vci_ini_c_proc0");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc0("vci_tgt_proc0");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc1("vci_ini_rw_proc1");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc1("vci_ini_c_proc1");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc1("vci_tgt_proc1");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc2("vci_ini_rw_proc2");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc2("vci_ini_c_proc2");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc2("vci_tgt_proc2");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc3("vci_ini_rw_proc3");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc3("vci_ini_c_proc3");

        soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc3("vci_tgt_proc3");

        soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc4("vci_ini_rw_proc4");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc4("vci_ini_c_proc4");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc4("vci_tgt_proc4");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc5("vci_ini_rw_proc5");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc5("vci_ini_c_proc5");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc5("vci_tgt_proc5");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc6("vci_ini_rw_proc6");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc6("vci_ini_c_proc6");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc6("vci_tgt_proc6");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc7("vci_ini_rw_proc7");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc7("vci_ini_c_proc7");

        soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc7("vci_tgt_proc7");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc8("vci_ini_rw_proc8");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc8("vci_ini_c_proc8");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc8("vci_tgt_proc8");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc9("vci_ini_rw_proc9");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc9("vci_ini_c_proc9");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc9("vci_tgt_proc9");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc10("vci_ini_rw_proc10");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc10("vci_ini_c_proc10");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc10("vci_tgt_proc10");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc11("vci_ini_rw_proc11");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc11("vci_ini_c_proc11");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc11("vci_tgt_proc11");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc12("vci_ini_rw_proc12");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc12("vci_ini_c_proc12");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc12("vci_tgt_proc12");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc13("vci_ini_rw_proc13");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc13("vci_ini_c_proc13");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc13("vci_tgt_proc13");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc14("vci_ini_rw_proc14");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc14("vci_ini_c_proc14");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc14("vci_tgt_proc14");

	soclib::caba::VciSignals<vci_param> signal_vci_ini_rw_proc15("vci_ini_rw_proc15");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_c_proc15("vci_ini_c_proc15");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_proc15("vci_tgt_proc15");

	soclib::caba::VciSignals<vci_param> signal_vci_tgt_tty("vci_tgt_tty");


	soclib::caba::VciSignals<vci_param> signal_vci_tgt_xram("vci_tgt_xram");

	soclib::caba::VciSignals<vci_param> signal_vci_ixr_memc0("vci_ixr_memc0");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_memc0("vci_ini_memc0");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_memc0("vci_tgt_memc0");
        soclib::caba::VciSignals<vci_param> signal_vci_tgt_cleanup_memc0("vci_tgt_cleanup_memc0");

	soclib::caba::VciSignals<vci_param> signal_vci_ixr_memc1("vci_ixr_memc1");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_memc1("vci_ini_memc1");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_memc1("vci_tgt_memc1");
        soclib::caba::VciSignals<vci_param> signal_vci_tgt_cleanup_memc1("vci_tgt_cleanup_memc1");

	soclib::caba::VciSignals<vci_param> signal_vci_ixr_memc2("vci_ixr_memc2");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_memc2("vci_ini_memc2");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_memc2("vci_tgt_memc2");
        soclib::caba::VciSignals<vci_param> signal_vci_tgt_cleanup_memc2("vci_tgt_cleanup_memc2");

	soclib::caba::VciSignals<vci_param> signal_vci_ixr_memc3("vci_ixr_memc3");
	soclib::caba::VciSignals<vci_param> signal_vci_ini_memc3("vci_ini_memc3");
	soclib::caba::VciSignals<vci_param> signal_vci_tgt_memc3("vci_tgt_memc3");
        soclib::caba::VciSignals<vci_param> signal_vci_tgt_cleanup_memc3("vci_tgt_cleanup_memc3");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0"); 
	sc_signal<bool> signal_tty_irq1("signal_tty_irq1"); 
	sc_signal<bool> signal_tty_irq2("signal_tty_irq2"); 
	sc_signal<bool> signal_tty_irq3("signal_tty_irq3"); 
	sc_signal<bool> signal_tty_irq4("signal_tty_irq4"); 
	sc_signal<bool> signal_tty_irq5("signal_tty_irq5"); 
	sc_signal<bool> signal_tty_irq6("signal_tty_irq6"); 
	sc_signal<bool> signal_tty_irq7("signal_tty_irq7"); 
	sc_signal<bool> signal_tty_irq8("signal_tty_irq8"); 
	sc_signal<bool> signal_tty_irq9("signal_tty_irq9"); 
	sc_signal<bool> signal_tty_irq10("signal_tty_irq10"); 
	sc_signal<bool> signal_tty_irq11("signal_tty_irq11"); 
	sc_signal<bool> signal_tty_irq12("signal_tty_irq12"); 
	sc_signal<bool> signal_tty_irq13("signal_tty_irq13"); 
	sc_signal<bool> signal_tty_irq14("signal_tty_irq14"); 
	sc_signal<bool> signal_tty_irq15("signal_tty_irq15"); 

	soclib::common::Loader loader("soft/bin.soft");

	//                                  init_rw   init_c   tgt
        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc0("proc0", 0, maptabp, maptabc, IntTab(0,0),IntTab(0,0),IntTab(0,0),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc1("proc1", 1, maptabp, maptabc, IntTab(0,1),IntTab(0,1),IntTab(0,1),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc2("proc2", 2, maptabp, maptabc, IntTab(0,2),IntTab(0,2),IntTab(0,2),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc3("proc3", 3, maptabp, maptabc, IntTab(0,3),IntTab(0,3),IntTab(0,3),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc4("proc4", 4, maptabp, maptabc, IntTab(1,0),IntTab(1,0),IntTab(1,0),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc5("proc5", 5, maptabp, maptabc, IntTab(1,1),IntTab(1,1),IntTab(1,1),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc6("proc6", 6, maptabp, maptabc, IntTab(1,2),IntTab(1,2),IntTab(1,2),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc7("proc7", 7, maptabp, maptabc, IntTab(1,3),IntTab(1,3),IntTab(1,3),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc8("proc8", 8, maptabp, maptabc, IntTab(2,0),IntTab(2,0),IntTab(2,0),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc9("proc9", 9, maptabp, maptabc, IntTab(2,1),IntTab(2,1),IntTab(2,1),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc10("proc10", 10, maptabp, maptabc, IntTab(2,2),IntTab(2,2),IntTab(2,2),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc11("proc11", 11, maptabp, maptabc, IntTab(2,3),IntTab(2,3),IntTab(2,3),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc12("proc12", 12, maptabp, maptabc, IntTab(3,0),IntTab(3,0),IntTab(3,0),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc13("proc13", 13, maptabp, maptabc, IntTab(3,1),IntTab(3,1),IntTab(3,1),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc14("proc14", 14, maptabp, maptabc, IntTab(3,2),IntTab(3,2),IntTab(3,2),4,64,16,4,64,16);

        soclib::caba::VciCcXCacheWrapperV1<vci_param, proc_iss > 
        proc15("proc15", 15, maptabp, maptabc, IntTab(3,3),IntTab(3,3),IntTab(3,3),4,64,16,4,64,16);


	soclib::caba::VciSimpleRam<vci_param> 
	xram("xram",IntTab(0),maptabx, loader);

        //                                  x_init    c_init    p_tgt     c_tgt
	soclib::caba::VciMemCacheV4<vci_param> 
//	memc0("memc0",maptabp,maptabc,maptabx,IntTab(0),IntTab(0,4),IntTab(0,0), IntTab(0,4), 16,256,16);
        memc0("memc0",maptabp,maptabc,maptabx,IntTab(0),IntTab(0,4),IntTab(0,0), IntTab(0,4), 16,256,16);

	soclib::caba::VciMemCacheV4<vci_param> 
	memc1("memc1",maptabp,maptabc,maptabx,IntTab(1),IntTab(1,4),IntTab(1,0), IntTab(1,4), 16,256,16);

	soclib::caba::VciMemCacheV4<vci_param> 
	memc2("memc2",maptabp,maptabc,maptabx,IntTab(2),IntTab(2,4),IntTab(2,0), IntTab(2,4), 16,256,16);

	soclib::caba::VciMemCacheV4<vci_param> 
	memc3("memc3",maptabp,maptabc,maptabx,IntTab(3),IntTab(3,4),IntTab(3,0), IntTab(3,4), 16,256,16);
	
	soclib::caba::VciMultiTty<vci_param> 
	tty("tty",IntTab(3,1),maptabp,"tty0","tty1","tty2","tty3","tty4","tty5","tty6","tty7","tty8","tty9","tty10","tty11","tty12","tty13","tty14","tty15",NULL);

    
	soclib::caba::VciLocalRingFast<vci_param, 40, 33> 
	clusterPN0("clusterPN0",maptabp, IntTab(0), 2, 18, 4, 1 );

	soclib::caba::VciLocalRingFast<vci_param, 40, 33>
	clusterPN1("clusterPN1",maptabp, IntTab(1), 2, 18, 4, 1 );

	soclib::caba::VciLocalRingFast<vci_param, 40, 33>
	clusterPN2("clusterPN2",maptabp, IntTab(2), 2, 18, 4, 1 );

	soclib::caba::VciLocalRingFast<vci_param, 40, 33>
	clusterPN3("clusterPN3",maptabp, IntTab(3), 2, 18, 4, 2 );

	soclib::caba::VciLocalRingFast<vci_param, 40, 33> 
	clusterCN0("clusterCN0",maptabc, IntTab(0), 2, 2, 5, 5 );

	soclib::caba::VciLocalRingFast<vci_param, 40, 33> 
	clusterCN1("clusterCN1",maptabc, IntTab(1), 2, 2, 5, 5 );

	soclib::caba::VciLocalRingFast<vci_param, 40, 33> 
	clusterCN2("clusterCN2",maptabc, IntTab(2), 2, 2, 5, 5 );

	soclib::caba::VciLocalRingFast<vci_param, 40, 33> 
	clusterCN3("clusterCN3",maptabc, IntTab(3), 2, 2, 5, 5 );

        //      name, mapping t, nb_init, nb_tgt, min_latency, fifo_depth
	soclib::caba::VciVgmn<vci_param> 
	xring("xring",maptabx, 4, 1, 2, 2);

	// Distributed Global Interconnect : one cmd router & one rsp router per cluster
	VirtualDspinRouter<cmd_width>* cmdrouter = (VirtualDspinRouter<cmd_width>*)
	    malloc(sizeof(VirtualDspinRouter<cmd_width>) * X_MAX * Y_MAX);
	VirtualDspinRouter<rsp_width>* rsprouter = (VirtualDspinRouter<rsp_width>*)
	    malloc(sizeof(VirtualDspinRouter<rsp_width>) * X_MAX * Y_MAX);

	for ( size_t x = 0 ; x < X_MAX ; x++ )
	{
	    for ( size_t y = 0 ; y < Y_MAX ; y++ )
	    {
	
	        std::ostringstream   str_cmd;
		std::ostringstream   str_rsp;

	        str_cmd << "cmdrouter_" << x << "_" << y;
	        str_rsp << "rsprouter_" << x << "_" << y;

	        new(&cmdrouter[cluster(x,y)]) VirtualDspinRouter<cmd_width>(str_cmd.str().c_str(),
	                                                 x,y,		// coordinate in the mesh
	                                                 x_width, y_width,	// x & y fields width
	                                                 4,4);		// input & output fifo depths
	        new(&rsprouter[cluster(x,y)]) VirtualDspinRouter<rsp_width>(str_rsp.str().c_str(),
	                                                 x,y,		// coordinates in mesh
	                                                 x_width, y_width,	// x & y fields width
	                                                 4,4);		// input & output fifo depths
	    }
	}

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

	proc2.p_clk(signal_clk);  
	proc2.p_resetn(signal_resetn);  
	proc2.p_irq[0](signal_proc2_it0); 
	proc2.p_irq[1](signal_proc2_it1); 
	proc2.p_irq[2](signal_proc2_it2); 
	proc2.p_irq[3](signal_proc2_it3); 
	proc2.p_irq[4](signal_proc2_it4); 
	proc2.p_irq[5](signal_proc2_it5); 
	proc2.p_vci_ini_rw(signal_vci_ini_rw_proc2);
	proc2.p_vci_ini_c(signal_vci_ini_c_proc2);
	proc2.p_vci_tgt(signal_vci_tgt_proc2);

	proc3.p_clk(signal_clk);  
	proc3.p_resetn(signal_resetn);  
	proc3.p_irq[0](signal_proc3_it0); 
	proc3.p_irq[1](signal_proc3_it1); 
	proc3.p_irq[2](signal_proc3_it2); 
	proc3.p_irq[3](signal_proc3_it3); 
	proc3.p_irq[4](signal_proc3_it4); 
	proc3.p_irq[5](signal_proc3_it5); 
	proc3.p_vci_ini_rw(signal_vci_ini_rw_proc3);
	proc3.p_vci_ini_c(signal_vci_ini_c_proc3);
	proc3.p_vci_tgt(signal_vci_tgt_proc3);

	proc4.p_clk(signal_clk);  
	proc4.p_resetn(signal_resetn);  
	proc4.p_irq[0](signal_proc4_it0); 
	proc4.p_irq[1](signal_proc4_it1); 
	proc4.p_irq[2](signal_proc4_it2); 
	proc4.p_irq[3](signal_proc4_it3); 
	proc4.p_irq[4](signal_proc4_it4); 
	proc4.p_irq[5](signal_proc4_it5); 
	proc4.p_vci_ini_rw(signal_vci_ini_rw_proc4);
	proc4.p_vci_ini_c(signal_vci_ini_c_proc4);
	proc4.p_vci_tgt(signal_vci_tgt_proc4);

	proc5.p_clk(signal_clk);  
	proc5.p_resetn(signal_resetn);  
	proc5.p_irq[0](signal_proc5_it0); 
	proc5.p_irq[1](signal_proc5_it1); 
	proc5.p_irq[2](signal_proc5_it2); 
	proc5.p_irq[3](signal_proc5_it3); 
	proc5.p_irq[4](signal_proc5_it4); 
	proc5.p_irq[5](signal_proc5_it5); 
	proc5.p_vci_ini_rw(signal_vci_ini_rw_proc5);
	proc5.p_vci_ini_c(signal_vci_ini_c_proc5);
	proc5.p_vci_tgt(signal_vci_tgt_proc5);

	proc6.p_clk(signal_clk);  
	proc6.p_resetn(signal_resetn);  
	proc6.p_irq[0](signal_proc6_it0); 
	proc6.p_irq[1](signal_proc6_it1); 
	proc6.p_irq[2](signal_proc6_it2); 
	proc6.p_irq[3](signal_proc6_it3); 
	proc6.p_irq[4](signal_proc6_it4); 
	proc6.p_irq[5](signal_proc6_it5); 
	proc6.p_vci_ini_rw(signal_vci_ini_rw_proc6);
	proc6.p_vci_ini_c(signal_vci_ini_c_proc6);
	proc6.p_vci_tgt(signal_vci_tgt_proc6);

	proc7.p_clk(signal_clk);  
	proc7.p_resetn(signal_resetn);  
	proc7.p_irq[0](signal_proc7_it0); 
	proc7.p_irq[1](signal_proc7_it1); 
	proc7.p_irq[2](signal_proc7_it2); 
	proc7.p_irq[3](signal_proc7_it3); 
	proc7.p_irq[4](signal_proc7_it4); 
	proc7.p_irq[5](signal_proc7_it5); 
	proc7.p_vci_ini_rw(signal_vci_ini_rw_proc7);
	proc7.p_vci_ini_c(signal_vci_ini_c_proc7);
	proc7.p_vci_tgt(signal_vci_tgt_proc7);

	proc8.p_clk(signal_clk);  
	proc8.p_resetn(signal_resetn);  
	proc8.p_irq[0](signal_proc8_it0); 
	proc8.p_irq[1](signal_proc8_it1); 
	proc8.p_irq[2](signal_proc8_it2); 
	proc8.p_irq[3](signal_proc8_it3); 
	proc8.p_irq[4](signal_proc8_it4); 
	proc8.p_irq[5](signal_proc8_it5); 
	proc8.p_vci_ini_rw(signal_vci_ini_rw_proc8);
	proc8.p_vci_ini_c(signal_vci_ini_c_proc8);
	proc8.p_vci_tgt(signal_vci_tgt_proc8);

	proc9.p_clk(signal_clk);  
	proc9.p_resetn(signal_resetn);  
	proc9.p_irq[0](signal_proc9_it0); 
	proc9.p_irq[1](signal_proc9_it1); 
	proc9.p_irq[2](signal_proc9_it2); 
	proc9.p_irq[3](signal_proc9_it3); 
	proc9.p_irq[4](signal_proc9_it4); 
	proc9.p_irq[5](signal_proc9_it5); 
	proc9.p_vci_ini_rw(signal_vci_ini_rw_proc9);
	proc9.p_vci_ini_c(signal_vci_ini_c_proc9);
	proc9.p_vci_tgt(signal_vci_tgt_proc9);

	proc10.p_clk(signal_clk);  
	proc10.p_resetn(signal_resetn);  
	proc10.p_irq[0](signal_proc10_it0); 
	proc10.p_irq[1](signal_proc10_it1); 
	proc10.p_irq[2](signal_proc10_it2); 
	proc10.p_irq[3](signal_proc10_it3); 
	proc10.p_irq[4](signal_proc10_it4); 
	proc10.p_irq[5](signal_proc10_it5); 
	proc10.p_vci_ini_rw(signal_vci_ini_rw_proc10);
	proc10.p_vci_ini_c(signal_vci_ini_c_proc10);
	proc10.p_vci_tgt(signal_vci_tgt_proc10);

	proc11.p_clk(signal_clk);  
	proc11.p_resetn(signal_resetn);  
	proc11.p_irq[0](signal_proc11_it0); 
	proc11.p_irq[1](signal_proc11_it1); 
	proc11.p_irq[2](signal_proc11_it2); 
	proc11.p_irq[3](signal_proc11_it3); 
	proc11.p_irq[4](signal_proc11_it4); 
	proc11.p_irq[5](signal_proc11_it5); 
	proc11.p_vci_ini_rw(signal_vci_ini_rw_proc11);
	proc11.p_vci_ini_c(signal_vci_ini_c_proc11);
	proc11.p_vci_tgt(signal_vci_tgt_proc11);

	proc12.p_clk(signal_clk);  
	proc12.p_resetn(signal_resetn);  
	proc12.p_irq[0](signal_proc12_it0); 
	proc12.p_irq[1](signal_proc12_it1); 
	proc12.p_irq[2](signal_proc12_it2); 
	proc12.p_irq[3](signal_proc12_it3); 
	proc12.p_irq[4](signal_proc12_it4); 
	proc12.p_irq[5](signal_proc12_it5); 
	proc12.p_vci_ini_rw(signal_vci_ini_rw_proc12);
	proc12.p_vci_ini_c(signal_vci_ini_c_proc12);
	proc12.p_vci_tgt(signal_vci_tgt_proc12);

	proc13.p_clk(signal_clk);  
	proc13.p_resetn(signal_resetn);  
	proc13.p_irq[0](signal_proc13_it0); 
	proc13.p_irq[1](signal_proc13_it1); 
	proc13.p_irq[2](signal_proc13_it2); 
	proc13.p_irq[3](signal_proc13_it3); 
	proc13.p_irq[4](signal_proc13_it4); 
	proc13.p_irq[5](signal_proc13_it5); 
	proc13.p_vci_ini_rw(signal_vci_ini_rw_proc13);
	proc13.p_vci_ini_c(signal_vci_ini_c_proc13);
	proc13.p_vci_tgt(signal_vci_tgt_proc13);

	proc14.p_clk(signal_clk);  
	proc14.p_resetn(signal_resetn);  
	proc14.p_irq[0](signal_proc14_it0); 
	proc14.p_irq[1](signal_proc14_it1); 
	proc14.p_irq[2](signal_proc14_it2); 
	proc14.p_irq[3](signal_proc14_it3); 
	proc14.p_irq[4](signal_proc14_it4); 
	proc14.p_irq[5](signal_proc14_it5); 
	proc14.p_vci_ini_rw(signal_vci_ini_rw_proc14);
	proc14.p_vci_ini_c(signal_vci_ini_c_proc14);
	proc14.p_vci_tgt(signal_vci_tgt_proc14);

	proc15.p_clk(signal_clk);  
	proc15.p_resetn(signal_resetn);  
	proc15.p_irq[0](signal_proc15_it0); 
	proc15.p_irq[1](signal_proc15_it1); 
	proc15.p_irq[2](signal_proc15_it2); 
	proc15.p_irq[3](signal_proc15_it3); 
	proc15.p_irq[4](signal_proc15_it4); 
	proc15.p_irq[5](signal_proc15_it5); 
	proc15.p_vci_ini_rw(signal_vci_ini_rw_proc15);
	proc15.p_vci_ini_c(signal_vci_ini_c_proc15);
	proc15.p_vci_tgt(signal_vci_tgt_proc15);

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
	tty.p_irq[8](signal_tty_irq8);
	tty.p_irq[9](signal_tty_irq9);
	tty.p_irq[10](signal_tty_irq10);
	tty.p_irq[11](signal_tty_irq11); 
	tty.p_irq[12](signal_tty_irq12);
	tty.p_irq[13](signal_tty_irq13);
	tty.p_irq[14](signal_tty_irq14);
	tty.p_irq[15](signal_tty_irq15);  

	memc0.p_clk(signal_clk);
	memc0.p_resetn(signal_resetn);
	memc0.p_vci_tgt(signal_vci_tgt_memc0);
	memc0.p_vci_tgt_cleanup(signal_vci_tgt_cleanup_memc0);
	memc0.p_vci_ini(signal_vci_ini_memc0);
	memc0.p_vci_ixr(signal_vci_ixr_memc0);

	memc1.p_clk(signal_clk);
	memc1.p_resetn(signal_resetn);
	memc1.p_vci_tgt(signal_vci_tgt_memc1);
	memc1.p_vci_tgt_cleanup(signal_vci_tgt_cleanup_memc1);
	memc1.p_vci_ini(signal_vci_ini_memc1);
	memc1.p_vci_ixr(signal_vci_ixr_memc1);

	memc2.p_clk(signal_clk);
	memc2.p_resetn(signal_resetn);
	memc2.p_vci_tgt(signal_vci_tgt_memc2);
	memc2.p_vci_tgt_cleanup(signal_vci_tgt_cleanup_memc2);
	memc2.p_vci_ini(signal_vci_ini_memc2);
	memc2.p_vci_ixr(signal_vci_ixr_memc2);

	memc3.p_clk(signal_clk);
	memc3.p_resetn(signal_resetn);
	memc3.p_vci_tgt(signal_vci_tgt_memc3);
	memc3.p_vci_tgt_cleanup(signal_vci_tgt_cleanup_memc3);
	memc3.p_vci_ini(signal_vci_ini_memc3);
	memc3.p_vci_ixr(signal_vci_ixr_memc3);


	xram.p_clk(signal_clk);
        xram.p_resetn(signal_resetn);
	xram.p_vci(signal_vci_tgt_xram);	

	///////////////////////////////////////////////////////
	// Réseau vers la XRAM
	///////////////////////////////////////////////////////

	xring.p_clk(signal_clk);
	xring.p_resetn(signal_resetn);

	xring.p_to_initiator[0](signal_vci_ixr_memc0);
	xring.p_to_initiator[1](signal_vci_ixr_memc1);
	xring.p_to_initiator[2](signal_vci_ixr_memc2);
	xring.p_to_initiator[3](signal_vci_ixr_memc3);

	xring.p_to_target[0](signal_vci_tgt_xram);

	///////////////////////////////////////////////////////
	// Routers connections
	///////////////////////////////////////////////////////

	for ( size_t x = 0 ; x < X_MAX ; x++ )
	{
	    for ( size_t y = 0 ; y < Y_MAX ; y++ )
	    {
	        // cmd DSPIN router
	        cmdrouter[cluster(x,y)].p_clk			(signal_clk);
	        cmdrouter[cluster(x,y)].p_resetn		(signal_resetn);
	        cmdrouter[cluster(x,y)].p_out[0][LOCAL]		(signal_dspin_cmd_g2l_d[x][y]);
	        cmdrouter[cluster(x,y)].p_out[1][LOCAL]		(signal_dspin_cmd_g2l_c[x][y]);
	        cmdrouter[cluster(x,y)].p_in[0][LOCAL]		(signal_dspin_cmd_l2g_d[x][y]);
	        cmdrouter[cluster(x,y)].p_in[1][LOCAL]		(signal_dspin_cmd_l2g_c[x][y]);
	
	        // rsp DSPIN router
	        rsprouter[cluster(x,y)].p_clk			(signal_clk);
	        rsprouter[cluster(x,y)].p_resetn		(signal_resetn);
	        rsprouter[cluster(x,y)].p_out[0][LOCAL]		(signal_dspin_rsp_g2l_d[x][y]);
	        rsprouter[cluster(x,y)].p_out[1][LOCAL]		(signal_dspin_rsp_g2l_c[x][y]);
	        rsprouter[cluster(x,y)].p_in[0][LOCAL]		(signal_dspin_rsp_l2g_d[x][y]);
	        rsprouter[cluster(x,y)].p_in[1][LOCAL]		(signal_dspin_rsp_l2g_c[x][y]);
	    }
	}

	// Inter Clusters horizontal connections
	for ( size_t x = 0 ; x < (X_MAX-1) ; x++ )
	{
	    for ( size_t y = 0 ; y < Y_MAX ; y++ )
	    {
	        for ( size_t k = 0 ; k < 2 ; k++ )
	        {
	            cmdrouter[cluster(x,y)].p_out[k][EAST]		(signal_dspin_h_cmd_inc[x][y][k]);		
	            cmdrouter[cluster((x+1),y)].p_in[k][WEST]	(signal_dspin_h_cmd_inc[x][y][k]);
	
	            cmdrouter[cluster(x,y)].p_in[k][EAST]		(signal_dspin_h_cmd_dec[x][y][k]);		
	            cmdrouter[cluster((x+1),y)].p_out[k][WEST]	(signal_dspin_h_cmd_dec[x][y][k]);
	
	            rsprouter[cluster(x,y)].p_out[k][EAST]		(signal_dspin_h_rsp_inc[x][y][k]);		
	            rsprouter[cluster((x+1),y)].p_in[k][WEST]	(signal_dspin_h_rsp_inc[x][y][k]);
	
	            rsprouter[cluster(x,y)].p_in[k][EAST]		(signal_dspin_h_rsp_dec[x][y][k]);		
	            rsprouter[cluster((x+1),y)].p_out[k][WEST]	(signal_dspin_h_rsp_dec[x][y][k]);
	       } 
	    }
	}
	
//	std::cout << "returned after inter cluster are horizontally connected" << std::endl;
	
	// East & West boundary clusters connections
	
	for ( size_t y = 0 ; y < Y_MAX ; y++ )
	{
	    for ( size_t k = 0 ; k < 2 ; k++ )
	    {
	            cmdrouter[cluster(0,y)].p_in[k][WEST]           (signal_dspin_false_cmd_in[cluster(0,y)][k][0]);
	            cmdrouter[cluster(0,y)].p_out[k][WEST]          (signal_dspin_false_cmd_out[cluster(0,y)][k][0]);
	            rsprouter[cluster(0,y)].p_in[k][WEST]           (signal_dspin_false_rsp_in[cluster(0,y)][k][0]);
	            rsprouter[cluster(0,y)].p_out[k][WEST]          (signal_dspin_false_rsp_out[cluster(0,y)][k][0]);
	
	            cmdrouter[cluster((X_MAX-1),y)].p_in[k][EAST]     (signal_dspin_false_cmd_in[cluster((X_MAX-1),y)][k][0]);
	            cmdrouter[cluster((X_MAX-1),y)].p_out[k][EAST]    (signal_dspin_false_cmd_out[cluster((X_MAX-1),y)][k][0]);
	            rsprouter[cluster((X_MAX-1),y)].p_in[k][EAST]     (signal_dspin_false_rsp_in[cluster((X_MAX-1),y)][k][0]);
	            rsprouter[cluster((X_MAX-1),y)].p_out[k][EAST]    (signal_dspin_false_rsp_out[cluster((X_MAX-1),y)][k][0]);
	    }
	 }
	
//	std::cout << "returned after boundary clusters are horizontally connected" << std::endl;
	
	// Inter Clusters vertical connections
	for ( size_t y = 0 ; y < (Y_MAX-1) ; y++ )
	{
	    for ( size_t x = 0 ; x < X_MAX ; x++ )
	    {
	        for ( size_t k = 0 ; k < 2 ; k++ )
	        {
	            cmdrouter[cluster(x,y)].p_out[k][NORTH]		(signal_dspin_v_cmd_inc[x][y][k]);		
	            cmdrouter[cluster(x,(y+1))].p_in[k][SOUTH]	(signal_dspin_v_cmd_inc[x][y][k]);
	
	            cmdrouter[cluster(x,y)].p_in[k][NORTH]		(signal_dspin_v_cmd_dec[x][y][k]);		
	            cmdrouter[cluster(x,(y+1))].p_out[k][SOUTH]	(signal_dspin_v_cmd_dec[x][y][k]);
	
	            rsprouter[cluster(x,y)].p_out[k][NORTH]		(signal_dspin_v_rsp_inc[x][y][k]);		
	            rsprouter[cluster(x,(y+1))].p_in[k][SOUTH]	(signal_dspin_v_rsp_inc[x][y][k]);
	
	            rsprouter[cluster(x,y)].p_in[k][NORTH]		(signal_dspin_v_rsp_dec[x][y][k]);		
	            rsprouter[cluster(x,(y+1))].p_out[k][SOUTH]	(signal_dspin_v_rsp_dec[x][y][k]);
	        }
	    }
	}
	
	// North & South boundary clusters connections
	
	for ( size_t x = 0 ; x < X_MAX ; x++ )
	{
	    for ( size_t k = 0 ; k < 2 ; k++ )
	    {
	            cmdrouter[cluster(x,0)].p_in[k][SOUTH]          (signal_dspin_false_cmd_in[cluster(x,0)][k][1]);
	            cmdrouter[cluster(x,0)].p_out[k][SOUTH]         (signal_dspin_false_cmd_out[cluster(x,0)][k][1]);
	            rsprouter[cluster(x,0)].p_in[k][SOUTH]          (signal_dspin_false_rsp_in[cluster(x,0)][k][1]);
	            rsprouter[cluster(x,0)].p_out[k][SOUTH]         (signal_dspin_false_rsp_out[cluster(x,0)][k][1]);
	
	            cmdrouter[cluster(x,(Y_MAX-1))].p_out[k][NORTH]   (signal_dspin_false_cmd_out[cluster(x,(Y_MAX-1))][k][1]);
         	    cmdrouter[cluster(x,(Y_MAX-1))].p_in[k][NORTH]    (signal_dspin_false_cmd_in[cluster(x,(Y_MAX-1))][k][1]);
	            rsprouter[cluster(x,(Y_MAX-1))].p_out[k][NORTH]   (signal_dspin_false_rsp_out[cluster(x,(Y_MAX-1))][k][1]);
	            rsprouter[cluster(x,(Y_MAX-1))].p_in[k][NORTH]    (signal_dspin_false_rsp_in[cluster(x,(Y_MAX-1))][k][1]);
	    }
	}
	
//	std::cout << "returned after boundary clusters are vertically connected" << std::endl;

	///////////////////////////////////////////////////////
	// Réseau des commandes primaires
	///////////////////////////////////////////////////////

	clusterPN0.p_clk(signal_clk);
	clusterPN0.p_resetn(signal_resetn);

	clusterPN0.p_to_initiator[0](signal_vci_ini_rw_proc0);
	clusterPN0.p_to_initiator[1](signal_vci_ini_rw_proc1);
	clusterPN0.p_to_initiator[2](signal_vci_ini_rw_proc2);
	clusterPN0.p_to_initiator[3](signal_vci_ini_rw_proc3);

	clusterPN0.p_to_target[0](signal_vci_tgt_memc0);

        clusterPN0.p_gate_cmd_out(signal_dspin_cmd_l2g_d[0][0]);
	clusterPN0.p_gate_rsp_in(signal_dspin_rsp_g2l_d[0][0]);
        clusterPN0.p_gate_cmd_in(signal_dspin_cmd_g2l_d[0][0]);
	clusterPN0.p_gate_rsp_out(signal_dspin_rsp_l2g_d[0][0]);

	clusterPN1.p_clk(signal_clk);
	clusterPN1.p_resetn(signal_resetn);

	clusterPN1.p_to_initiator[0](signal_vci_ini_rw_proc4);
	clusterPN1.p_to_initiator[1](signal_vci_ini_rw_proc5);
	clusterPN1.p_to_initiator[2](signal_vci_ini_rw_proc6);
	clusterPN1.p_to_initiator[3](signal_vci_ini_rw_proc7);

	clusterPN1.p_to_target[0](signal_vci_tgt_memc1);

        clusterPN1.p_gate_cmd_out(signal_dspin_cmd_l2g_d[0][1]);
	clusterPN1.p_gate_rsp_in(signal_dspin_rsp_g2l_d[0][1]);
        clusterPN1.p_gate_cmd_in(signal_dspin_cmd_g2l_d[0][1]);
	clusterPN1.p_gate_rsp_out(signal_dspin_rsp_l2g_d[0][1]);

	clusterPN2.p_clk(signal_clk);
	clusterPN2.p_resetn(signal_resetn);

	clusterPN2.p_to_initiator[0](signal_vci_ini_rw_proc8);
	clusterPN2.p_to_initiator[1](signal_vci_ini_rw_proc9);
	clusterPN2.p_to_initiator[2](signal_vci_ini_rw_proc10);
	clusterPN2.p_to_initiator[3](signal_vci_ini_rw_proc11);
	
	clusterPN2.p_to_target[0](signal_vci_tgt_memc2);

        clusterPN2.p_gate_cmd_out(signal_dspin_cmd_l2g_d[1][0]);
	clusterPN2.p_gate_rsp_in(signal_dspin_rsp_g2l_d[1][0]);
        clusterPN2.p_gate_cmd_in(signal_dspin_cmd_g2l_d[1][0]);
	clusterPN2.p_gate_rsp_out(signal_dspin_rsp_l2g_d[1][0]);

	clusterPN3.p_clk(signal_clk);
	clusterPN3.p_resetn(signal_resetn);

	clusterPN3.p_to_initiator[0](signal_vci_ini_rw_proc12);
	clusterPN3.p_to_initiator[1](signal_vci_ini_rw_proc13);
	clusterPN3.p_to_initiator[2](signal_vci_ini_rw_proc14);
	clusterPN3.p_to_initiator[3](signal_vci_ini_rw_proc15);

	clusterPN3.p_to_target[0](signal_vci_tgt_memc3);
	clusterPN3.p_to_target[1](signal_vci_tgt_tty);

        clusterPN3.p_gate_cmd_out(signal_dspin_cmd_l2g_d[1][1]);
	clusterPN3.p_gate_rsp_in(signal_dspin_rsp_g2l_d[1][1]);
        clusterPN3.p_gate_cmd_in(signal_dspin_cmd_g2l_d[1][1]);
	clusterPN3.p_gate_rsp_out(signal_dspin_rsp_l2g_d[1][1]);

//	std::cout << "returned after clusterPN3 connected" << std::endl;
	
	///////////////////////////////////////////////////////
	// Réseau des commandes de cohérence
	///////////////////////////////////////////////////////
	clusterCN0.p_clk(signal_clk);
	clusterCN0.p_resetn(signal_resetn);

	clusterCN0.p_to_initiator[4](signal_vci_ini_memc0);
	clusterCN0.p_to_initiator[0](signal_vci_ini_c_proc0);
	clusterCN0.p_to_initiator[1](signal_vci_ini_c_proc1);
	clusterCN0.p_to_initiator[2](signal_vci_ini_c_proc2);
	clusterCN0.p_to_initiator[3](signal_vci_ini_c_proc3);

	clusterCN0.p_to_target[0](signal_vci_tgt_proc0);
	clusterCN0.p_to_target[1](signal_vci_tgt_proc1);
	clusterCN0.p_to_target[2](signal_vci_tgt_proc2);
	clusterCN0.p_to_target[3](signal_vci_tgt_proc3);
	clusterCN0.p_to_target[4](signal_vci_tgt_cleanup_memc0);

        clusterCN0.p_gate_cmd_out(signal_dspin_cmd_l2g_c[0][0]);
	clusterCN0.p_gate_rsp_in(signal_dspin_rsp_g2l_c[0][0]);
        clusterCN0.p_gate_cmd_in(signal_dspin_cmd_g2l_c[0][0]);
	clusterCN0.p_gate_rsp_out(signal_dspin_rsp_l2g_c[0][0]);

	clusterCN1.p_clk(signal_clk);
	clusterCN1.p_resetn(signal_resetn);

        clusterCN1.p_to_initiator[4](signal_vci_ini_memc1);
	clusterCN1.p_to_initiator[0](signal_vci_ini_c_proc4);
	clusterCN1.p_to_initiator[1](signal_vci_ini_c_proc5);
	clusterCN1.p_to_initiator[2](signal_vci_ini_c_proc6);
	clusterCN1.p_to_initiator[3](signal_vci_ini_c_proc7);

	clusterCN1.p_to_target[0](signal_vci_tgt_proc4);
	clusterCN1.p_to_target[1](signal_vci_tgt_proc5);
	clusterCN1.p_to_target[2](signal_vci_tgt_proc6);
	clusterCN1.p_to_target[3](signal_vci_tgt_proc7);
	clusterCN1.p_to_target[4](signal_vci_tgt_cleanup_memc1);

        clusterCN1.p_gate_cmd_out(signal_dspin_cmd_l2g_c[0][1]);
	clusterCN1.p_gate_rsp_in(signal_dspin_rsp_g2l_c[0][1]);
        clusterCN1.p_gate_cmd_in(signal_dspin_cmd_g2l_c[0][1]);
	clusterCN1.p_gate_rsp_out(signal_dspin_rsp_l2g_c[0][1]);

	clusterCN2.p_clk(signal_clk);
	clusterCN2.p_resetn(signal_resetn);

	clusterCN2.p_to_initiator[4](signal_vci_ini_memc2);
	clusterCN2.p_to_initiator[0](signal_vci_ini_c_proc8);
	clusterCN2.p_to_initiator[1](signal_vci_ini_c_proc9);
	clusterCN2.p_to_initiator[2](signal_vci_ini_c_proc10);
	clusterCN2.p_to_initiator[3](signal_vci_ini_c_proc11);

	clusterCN2.p_to_target[0](signal_vci_tgt_proc8);
	clusterCN2.p_to_target[1](signal_vci_tgt_proc9);
	clusterCN2.p_to_target[2](signal_vci_tgt_proc10);
	clusterCN2.p_to_target[3](signal_vci_tgt_proc11);
        clusterCN2.p_to_target[4](signal_vci_tgt_cleanup_memc2);

        clusterCN2.p_gate_cmd_out(signal_dspin_cmd_l2g_c[1][0]);
	clusterCN2.p_gate_rsp_in(signal_dspin_rsp_g2l_c[1][0]);
        clusterCN2.p_gate_cmd_in(signal_dspin_cmd_g2l_c[1][0]);
	clusterCN2.p_gate_rsp_out(signal_dspin_rsp_l2g_c[1][0]);

	clusterCN3.p_clk(signal_clk);
	clusterCN3.p_resetn(signal_resetn);

	clusterCN3.p_to_initiator[4](signal_vci_ini_memc3);
	clusterCN3.p_to_initiator[0](signal_vci_ini_c_proc12);
	clusterCN3.p_to_initiator[1](signal_vci_ini_c_proc13);
	clusterCN3.p_to_initiator[2](signal_vci_ini_c_proc14);
	clusterCN3.p_to_initiator[3](signal_vci_ini_c_proc15);

	clusterCN3.p_to_target[0](signal_vci_tgt_proc12);
	clusterCN3.p_to_target[1](signal_vci_tgt_proc13);
	clusterCN3.p_to_target[2](signal_vci_tgt_proc14);
	clusterCN3.p_to_target[3](signal_vci_tgt_proc15);
	clusterCN3.p_to_target[4](signal_vci_tgt_cleanup_memc3);

        clusterCN3.p_gate_cmd_out(signal_dspin_cmd_l2g_c[1][1]);
	clusterCN3.p_gate_rsp_in(signal_dspin_rsp_g2l_c[1][1]);
        clusterCN3.p_gate_cmd_in(signal_dspin_cmd_g2l_c[1][1]);
	clusterCN3.p_gate_rsp_out(signal_dspin_rsp_l2g_c[1][1]);


	////////////////////////////////////////////////////////
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

	for(size_t t = 0; t < X_MAX*Y_MAX; t++)
	{
	    for (size_t k = 0; k < 2; k++)
	    {
	        for(size_t a = 0; a < 2; a ++)
	        {
			signal_dspin_false_cmd_in[t][k][a].write = false;
			signal_dspin_false_cmd_in[t][k][a].read = true;
			signal_dspin_false_cmd_out[t][k][a].write = false;
			signal_dspin_false_cmd_out[t][k][a].read = true;
	
			signal_dspin_false_rsp_in[t][k][a].write = false;
			signal_dspin_false_rsp_in[t][k][a].read = true;
			signal_dspin_false_rsp_out[t][k][a].write = false;
			signal_dspin_false_rsp_out[t][k][a].read = true;
	       }
	    }
	}


	sc_start(sc_core::sc_time(1, SC_NS));
	signal_resetn = true;

	for (int i = 0; i < ncycles ; i+=1) {
		sc_start(sc_core::sc_time(1, SC_NS));
	//std::cout << " +++++++++ cycle : " << i << std::endl;
        //rsprouter[3].printTrace(0); 		
        //rsprouter[2].printTrace(0); 		
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

