
#include <iostream>
#include <cstdlib>

#define CONFIG_GDB_SERVER

#include "mutekh/.config.h"
#include "mapping_table.h"

#if defined(CONFIG_SOCLIB_MEMCHECK)
# include "iss_memchecker.h"
#endif

#if defined(CONFIG_GDB_SERVER)
# include "gdbserver.h"
#endif

#include "vci_xcache_wrapper.h"
#include "vci_timer.h"
#include "vci_ram.h"
#include "vci_heterogeneous_rom.h"
#include "vci_multi_tty.h"
#include "vci_locks.h"
#include "vci_icu.h"
#include "vci_vgmn.h"

#include "soclib_addresses.h"

#define SEGTYPEMASK 0x00300000

// You may set the SOCLIB_GDB environment variable to
// START_FROZEN before starting the simulator.

#if defined(CONFIG_CPU_MIPS)
#include "mips32.h"
# if defined(CONFIG_CPU_ENDIAN_BIG)
# warning Using MIPS32Eb
	typedef soclib::common::Mips32EbIss ProcessorIss;
# elif defined(CONFIG_CPU_ENDIAN_LITTLE)
# warning Using MIPS32El
	typedef soclib::common::Mips32ElIss ProcessorIss;
# else
#  error No endian configuration defined
# endif

#elif defined(CONFIG_CPU_PPC)
#include "ppc405.h"
# warning Using PPC
	typedef soclib::common::Ppc405Iss ProcessorIss;

#elif defined(CONFIG_CPU_ARM)
#include "arm.h"
# warning Using ARM
	typedef soclib::common::ArmIss ProcessorIss;
#else
#  error No supported processor configuration defined
#endif

#if defined(CONFIG_GDB_SERVER)
# if defined(CONFIG_SOCLIB_MEMCHECK)
#  warning Using GDB and memchecker
	typedef soclib::common::GdbServer<soclib::common::IssMemchecker<ProcessorIss> > Processor;
# else
#  warning Using GDB
	typedef soclib::common::GdbServer<ProcessorIss> Processor;
# endif
#elif defined(CONFIG_SOCLIB_MEMCHECK)
# warning Using Memchecker
	typedef soclib::common::IssMemchecker<ProcessorIss> Processor;
#else
# warning Using raw processor
	typedef ProcessorIss Processor;
#endif
	
int _main(int argc, char *argv[])
{
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;
	size_t cpu_count = 4;

	if (argc < 2)
	  return 1;
	
	// Define our VCI parameters
	typedef soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1> vci_param;

	// Mapping table

	soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00c00000);

	maptab.add(Segment("reset", DSX_SEGMENT_RESET_ADDR, DSX_SEGMENT_RESET_SIZE, IntTab(0), true));
	maptab.add(Segment("excep", DSX_SEGMENT_EXCEP_ADDR, DSX_SEGMENT_EXCEP_SIZE, IntTab(0), true));
	maptab.add(Segment("text" , DSX_SEGMENT_TEXT_ADDR, DSX_SEGMENT_TEXT_SIZE , IntTab(0), true));
  
	maptab.add(Segment("data" , DSX_SEGMENT_DATA_CACHED_ADDR, DSX_SEGMENT_DATA_CACHED_SIZE, IntTab(1), true));
	maptab.add(Segment("udata" , DSX_SEGMENT_DATA_UNCACHED_ADDR, DSX_SEGMENT_DATA_UNCACHED_SIZE, IntTab(1), false));

	maptab.add(Segment("tty"  , DSX_SEGMENT_TTY_ADDR, DSX_SEGMENT_TTY_SIZE, IntTab(2), false));
	maptab.add(Segment("timer", DSX_SEGMENT_TIMER_ADDR, DSX_SEGMENT_TIMER_SIZE, IntTab(3), false));
	maptab.add(Segment("icu", DSX_SEGMENT_ICU_ADDR, DSX_SEGMENT_ICU_SIZE, IntTab(4), false));

	// Signals

	sc_core::sc_clock signal_clk("signal_clk");
	sc_core::sc_signal<bool> signal_resetn("signal_resetn");

	sc_core::sc_signal<bool> signal_cpu_it[cpu_count][Processor::n_irq]; 

	soclib::caba::VciSignals<vci_param> signal_vci_m[cpu_count];

	soclib::caba::VciSignals<vci_param> signal_vci_icu("signal_vci_icu");
	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
	soclib::caba::VciSignals<vci_param> signal_vci_vcitimer("signal_vci_vcitimer");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram1("signal_vci_vcimultiram1");

	sc_core::sc_signal<bool> signal_icu_irq[2];

	// Components

	soclib::common::Loader loader(argv[1]);

#if defined(CONFIG_SOCLIB_MEMCHECK)
	Processor::init(maptab, loader, "tty,timer,icu");
#endif

	soclib::caba::VciXcacheWrapper<vci_param, Processor> *cpu[cpu_count];

	for ( size_t i=0; i<cpu_count; ++i ) {
	  std::ostringstream o;
	  o << "cpu" << i;
	  cpu[i] = new soclib::caba::VciXcacheWrapper<vci_param, Processor>(
               o.str().c_str(), i, maptab,IntTab(i),1, 8, 4, 1, 8, 4);
	}

	soclib::caba::VciRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(2), maptab, "vcitty", NULL);
	soclib::caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(3), maptab, 1);
	soclib::caba::VciIcu<vci_param> vciicu("vciicu", IntTab(4), maptab, 2);

	soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptab, cpu_count, 5, 2, 8);

	//	Net-List
	for ( size_t i=0; i<cpu_count; ++i ) {
	  cpu[i]->p_clk(signal_clk);
	  cpu[i]->p_resetn(signal_resetn);
	  for ( size_t irq=0; irq<Processor::n_irq; ++irq )
	    cpu[i]->p_irq[irq](signal_cpu_it[i][irq]); 
	  cpu[i]->p_vci(signal_vci_m[i]);
	  vgmn.p_to_initiator[i](signal_vci_m[i]);
	}
	vcimultiram0.p_clk(signal_clk);
	vcimultiram1.p_clk(signal_clk);
	vcitimer.p_clk(signal_clk);
	vciicu.p_clk(signal_clk);
  
	vcimultiram0.p_resetn(signal_resetn);
	vcimultiram1.p_resetn(signal_resetn);
	vcitimer.p_resetn(signal_resetn);
	vciicu.p_resetn(signal_resetn);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);

	vciicu.p_vci(signal_vci_icu);
	vciicu.p_irq_in[0](signal_icu_irq[0]);
	vciicu.p_irq_in[1](signal_icu_irq[1]);
	vciicu.p_irq(signal_cpu_it[0][0]);

	vcitimer.p_vci(signal_vci_vcitimer);
	vcitimer.p_irq[0](signal_icu_irq[0]);

	vcimultiram1.p_vci(signal_vci_vcimultiram1);

	vcitty.p_clk(signal_clk);
	vcitty.p_resetn(signal_resetn);
	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_icu_irq[1]);

	vgmn.p_clk(signal_clk);
	vgmn.p_resetn(signal_resetn);

	vgmn.p_to_target[0](signal_vci_vcimultiram0);
	vgmn.p_to_target[1](signal_vci_vcimultiram1);
	vgmn.p_to_target[2](signal_vci_tty);
	vgmn.p_to_target[3](signal_vci_vcitimer);
	vgmn.p_to_target[4](signal_vci_icu);

	sc_core::sc_start(sc_core::sc_time(0, sc_core::SC_NS));
	signal_resetn = false;
	sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_NS));
	signal_resetn = true;
	sc_core::sc_start();

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
