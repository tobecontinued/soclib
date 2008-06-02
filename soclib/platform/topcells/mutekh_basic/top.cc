#include <iostream>
#include <cstdlib>

#include "mapping_table.h"
#include "ppc405.h"
#include "mips.h"
#include "gdbserver.h"
#include "iss_wrapper.h"
#include "vci_xcache.h"
#include "vci_timer.h"
#include "vci_ram.h"
#include "vci_heterogeneous_rom.h"
#include "vci_multi_tty.h"
#include "vci_locks.h"
#include "vci_icu.h"
#include "vci_vgmn.h"

#include "mutekh/.config.h"
#include "soclib_addresses.h"

#define SEGTYPEMASK 0x00300000

int _main(int argc, char *argv[])
{
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;

	if (argc < 2)
	  return 1;

#if defined(CONFIG_CPU_MIPS)
# if defined(CONFIG_CPU_ENDIAN_BIG)
	typedef soclib::common::MipsEbIss Processor;
# elif defined(CONFIG_CPU_ENDIAN_LITTLE)
	typedef soclib::common::MipsElIss Processor;
# else
#  error No endian configuration defined
# endif

#elif defined(CONFIG_CPU_PPC)
	typedef soclib::common::Ppc405Iss Processor;
#else
#  error No supported processor configuration defined
#endif

	// Define our VCI parameters
	typedef soclib::caba::VciParams<4,1,32,1,1,1,8,1,1,1> vci_param;

	// Mapping table

	soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00300000);

	maptab.add(Segment("reset", DSX_SEGMENT_RESET_ADDR, DSX_SEGMENT_RESET_SIZE, IntTab(0), true));
	maptab.add(Segment("excep", DSX_SEGMENT_EXCEP_ADDR, DSX_SEGMENT_EXCEP_SIZE, IntTab(0), true));
	maptab.add(Segment("text" , DSX_SEGMENT_TEXT_ADDR, DSX_SEGMENT_TEXT_SIZE , IntTab(0), true));
  
	maptab.add(Segment("data" , DSX_SEGMENT_DATA_CACHED_ADDR, DSX_SEGMENT_DATA_CACHED_SIZE, IntTab(1), true));
	maptab.add(Segment("udata" , DSX_SEGMENT_DATA_UNCACHED_ADDR, DSX_SEGMENT_DATA_UNCACHED_SIZE, IntTab(1), false));

	maptab.add(Segment("tty"  , DSX_SEGMENT_TTY_ADDR, DSX_SEGMENT_TTY_SIZE, IntTab(2), false));
	maptab.add(Segment("timer", DSX_SEGMENT_TIMER_ADDR, DSX_SEGMENT_TIMER_SIZE, IntTab(3), false));
	maptab.add(Segment("locks", DSX_SEGMENT_SEM_ADDR, DSX_SEGMENT_SEM_SIZE, IntTab(4), false));
	maptab.add(Segment("icu", DSX_SEGMENT_ICU_ADDR, DSX_SEGMENT_ICU_SIZE, IntTab(5), false));

	// Signals

	sc_clock signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");

	soclib::caba::ICacheSignals signal_cpu_icache0("signal_cpu_icache0");
	soclib::caba::DCacheSignals signal_cpu_dcache0("signal_cpu_dcache0");
	sc_signal<bool> signal_cpu0_it0("signal_cpu0_it0"); 
	sc_signal<bool> signal_cpu0_it1("signal_cpu0_it1"); 

#if defined(CONFIG_CPU_MIPS)
	sc_signal<bool> signal_cpu0_it2("signal_cpu0_it2"); 
	sc_signal<bool> signal_cpu0_it3("signal_cpu0_it3"); 
	sc_signal<bool> signal_cpu0_it4("signal_cpu0_it4"); 
	sc_signal<bool> signal_cpu0_it5("signal_cpu0_it5");
#endif
  
	soclib::caba::ICacheSignals 	signal_cpu_icache1("signal_cpu_icache1");
	soclib::caba::DCacheSignals 	signal_cpu_dcache1("signal_cpu_dcache1");
	sc_signal<bool> signal_cpu1_it0("signal_cpu1_it0"); 
	sc_signal<bool> signal_cpu1_it1("signal_cpu1_it1"); 

#if defined(CONFIG_CPU_MIPS)
	sc_signal<bool> signal_cpu1_it2("signal_cpu1_it2"); 
	sc_signal<bool> signal_cpu1_it3("signal_cpu1_it3"); 
	sc_signal<bool> signal_cpu1_it4("signal_cpu1_it4"); 
	sc_signal<bool> signal_cpu1_it5("signal_cpu1_it5");
#endif
  
	soclib::caba::ICacheSignals 	signal_cpu_icache2("signal_cpu_icache2");
	soclib::caba::DCacheSignals 	signal_cpu_dcache2("signal_cpu_dcache2");
	sc_signal<bool> signal_cpu2_it0("signal_cpu2_it0"); 
	sc_signal<bool> signal_cpu2_it1("signal_cpu2_it1"); 

#if defined(CONFIG_CPU_MIPS)
	sc_signal<bool> signal_cpu2_it2("signal_cpu2_it2"); 
	sc_signal<bool> signal_cpu2_it3("signal_cpu2_it3"); 
	sc_signal<bool> signal_cpu2_it4("signal_cpu2_it4"); 
	sc_signal<bool> signal_cpu2_it5("signal_cpu2_it5");
#endif
  
	soclib::caba::ICacheSignals signal_cpu_icache3("signal_cpu_icache3");
	soclib::caba::DCacheSignals signal_cpu_dcache3("signal_cpu_dcache3");
	sc_signal<bool> signal_cpu3_it0("signal_cpu3_it0"); 
	sc_signal<bool> signal_cpu3_it1("signal_cpu3_it1"); 

#if defined(CONFIG_CPU_MIPS)
	sc_signal<bool> signal_cpu3_it2("signal_cpu3_it2"); 
	sc_signal<bool> signal_cpu3_it3("signal_cpu3_it3"); 
	sc_signal<bool> signal_cpu3_it4("signal_cpu3_it4"); 
	sc_signal<bool> signal_cpu3_it5("signal_cpu3_it5");
#endif

	soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");
	soclib::caba::VciSignals<vci_param> signal_vci_m1("signal_vci_m1");
	soclib::caba::VciSignals<vci_param> signal_vci_m2("signal_vci_m2");
	soclib::caba::VciSignals<vci_param> signal_vci_m3("signal_vci_m3");

	soclib::caba::VciSignals<vci_param> signal_vci_icu("signal_vci_icu");
	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
	soclib::caba::VciSignals<vci_param> signal_vci_vcitimer("signal_vci_vcitimer");
	soclib::caba::VciSignals<vci_param> signal_vci_vcilocks("signal_vci_vcilocks");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram1("signal_vci_vcimultiram1");

	sc_signal<bool> signal_icu_irq0("signal_icu_irq0");
	sc_signal<bool> signal_icu_irq1("signal_icu_irq1");
	sc_signal<bool> signal_icu_irq2("signal_icu_irq2");
	sc_signal<bool> signal_icu_irq3("signal_icu_irq3");

	// Components

	soclib::caba::VciXCache<vci_param> cache0("cache0", maptab,IntTab(0),8,4,8,4);
	soclib::caba::VciXCache<vci_param> cache1("cache1", maptab,IntTab(1),8,4,8,4);
	soclib::caba::VciXCache<vci_param> cache2("cache2", maptab,IntTab(2),8,4,8,4);
	soclib::caba::VciXCache<vci_param> cache3("cache3", maptab,IntTab(3),8,4,8,4);

#if defined(CONFIG_GDB_START_FROZEN)
	soclib::common::GdbServer<Processor>::start_frozen();
#endif

	soclib::caba::IssWrapper<soclib::common::GdbServer<Processor> > cpu0("cpu0", 0);
	soclib::caba::IssWrapper<soclib::common::GdbServer<Processor> > cpu1("cpu1", 1);
	soclib::caba::IssWrapper<soclib::common::GdbServer<Processor> > cpu2("cpu2", 2);
	soclib::caba::IssWrapper<soclib::common::GdbServer<Processor> > cpu3("cpu3", 3);

	soclib::common::ElfLoader loader(argv[1]);
	soclib::caba::VciMultiRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciMultiRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(2), maptab, "vcitty", NULL);
	soclib::caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(3), maptab, 1);
	soclib::caba::VciLocks<vci_param> vcilocks("vcilocks", IntTab(4), maptab); 
	soclib::caba::VciIcu<vci_param> vciicu("vciicu", IntTab(5), maptab, 4);

	soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptab, 4, 6, 2, 8);

	//	Net-List

	cpu0.p_clk(signal_clk);  
	cpu1.p_clk(signal_clk);  
	cpu2.p_clk(signal_clk);  
	cpu3.p_clk(signal_clk);  
	cache0.p_clk(signal_clk);
	cache1.p_clk(signal_clk);
	cache2.p_clk(signal_clk);
	cache3.p_clk(signal_clk);
	vcimultiram0.p_clk(signal_clk);
	vcimultiram1.p_clk(signal_clk);
	vcilocks.p_clk(signal_clk);
	vcitimer.p_clk(signal_clk);
	vciicu.p_clk(signal_clk);
  
	cpu0.p_resetn(signal_resetn);  
	cpu1.p_resetn(signal_resetn);  
	cpu2.p_resetn(signal_resetn);  
	cpu3.p_resetn(signal_resetn);  
	cache0.p_resetn(signal_resetn);
	cache1.p_resetn(signal_resetn);
	cache2.p_resetn(signal_resetn);
	cache3.p_resetn(signal_resetn);
	vcimultiram0.p_resetn(signal_resetn);
	vcimultiram1.p_resetn(signal_resetn);
	vcilocks.p_resetn(signal_resetn);
	vcitimer.p_resetn(signal_resetn);
	vciicu.p_resetn(signal_resetn);
  
	cpu0.p_irq[0](signal_cpu0_it0); 
	cpu0.p_irq[1](signal_cpu0_it1); 

#if defined(CONFIG_CPU_MIPS)
	cpu0.p_irq[2](signal_cpu0_it2); 
	cpu0.p_irq[3](signal_cpu0_it3); 
	cpu0.p_irq[4](signal_cpu0_it4); 
	cpu0.p_irq[5](signal_cpu0_it5); 
#endif
	cpu0.p_icache(signal_cpu_icache0);
	cpu0.p_dcache(signal_cpu_dcache0);
  
	cpu1.p_irq[0](signal_cpu1_it0); 
	cpu1.p_irq[1](signal_cpu1_it1); 

#if defined(CONFIG_CPU_MIPS)
	cpu1.p_irq[2](signal_cpu1_it2); 
	cpu1.p_irq[3](signal_cpu1_it3); 
	cpu1.p_irq[4](signal_cpu1_it4); 
	cpu1.p_irq[5](signal_cpu1_it5); 
#endif
	cpu1.p_icache(signal_cpu_icache1);
	cpu1.p_dcache(signal_cpu_dcache1);
  
	cpu2.p_irq[0](signal_cpu2_it0); 
	cpu2.p_irq[1](signal_cpu2_it1); 

#if defined(CONFIG_CPU_MIPS)
	cpu2.p_irq[2](signal_cpu2_it2); 
	cpu2.p_irq[3](signal_cpu2_it3); 
	cpu2.p_irq[4](signal_cpu2_it4); 
	cpu2.p_irq[5](signal_cpu2_it5); 
#endif
	cpu2.p_icache(signal_cpu_icache2);
	cpu2.p_dcache(signal_cpu_dcache2);
  
	cpu3.p_irq[0](signal_cpu3_it0); 
	cpu3.p_irq[1](signal_cpu3_it1); 

#if defined(CONFIG_CPU_MIPS)
	cpu3.p_irq[2](signal_cpu3_it2); 
	cpu3.p_irq[3](signal_cpu3_it3); 
	cpu3.p_irq[4](signal_cpu3_it4); 
	cpu3.p_irq[5](signal_cpu3_it5); 
#endif
	cpu3.p_icache(signal_cpu_icache3);
	cpu3.p_dcache(signal_cpu_dcache3);
        
	cache0.p_icache(signal_cpu_icache0);
	cache0.p_dcache(signal_cpu_dcache0);
	cache0.p_vci(signal_vci_m0);

	cache1.p_icache(signal_cpu_icache1);
	cache1.p_dcache(signal_cpu_dcache1);
	cache1.p_vci(signal_vci_m1);

	cache2.p_icache(signal_cpu_icache2);
	cache2.p_dcache(signal_cpu_dcache2);
	cache2.p_vci(signal_vci_m2);

	cache3.p_icache(signal_cpu_icache3);
	cache3.p_dcache(signal_cpu_dcache3);
	cache3.p_vci(signal_vci_m3);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);

	vciicu.p_vci(signal_vci_icu);
	vciicu.p_irq_in[0](signal_icu_irq0);
	vciicu.p_irq_in[1](signal_icu_irq1);
	vciicu.p_irq_in[2](signal_icu_irq2);
	vciicu.p_irq_in[3](signal_icu_irq3);
	vciicu.p_irq(signal_cpu0_it0);

	vcitimer.p_vci(signal_vci_vcitimer);
	vcitimer.p_irq[0](signal_icu_irq0);

	vcilocks.p_vci(signal_vci_vcilocks);
  
	vcimultiram1.p_vci(signal_vci_vcimultiram1);

	vcitty.p_clk(signal_clk);
	vcitty.p_resetn(signal_resetn);
	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_icu_irq1);

	vgmn.p_clk(signal_clk);
	vgmn.p_resetn(signal_resetn);

	vgmn.p_to_initiator[0](signal_vci_m0);
	vgmn.p_to_initiator[1](signal_vci_m1);
	vgmn.p_to_initiator[2](signal_vci_m2);
	vgmn.p_to_initiator[3](signal_vci_m3);

	vgmn.p_to_target[0](signal_vci_vcimultiram0);
	vgmn.p_to_target[1](signal_vci_vcimultiram1);
	vgmn.p_to_target[2](signal_vci_tty);
	vgmn.p_to_target[3](signal_vci_vcitimer);
	vgmn.p_to_target[4](signal_vci_vcilocks);
	vgmn.p_to_target[5](signal_vci_icu);

	sc_start(sc_core::sc_time(0, SC_NS));
	signal_resetn = false;
	sc_start(sc_core::sc_time(1, SC_NS));
	signal_resetn = true;
	sc_start();

	return EXIT_SUCCESS;
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
