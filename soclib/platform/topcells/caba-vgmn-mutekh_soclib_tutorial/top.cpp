
#include "cpu_type.h"

//#define USE_GDB_SERVER
//#define USE_MEMCHECKER

#include <systemc>
#include <sys/time.h>
#include <iostream>
#include <cstdlib>
#include <cstdarg>

#include "gdbserver.h"
#include "iss_memchecker.h"

#include "mapping_table.h"
#include "vci_rom.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_vgmn.h"
#include "vci_xcache_wrapper.h"
#include "vci_xicu.h"
#include "vci_dma.h"



#if defined(CPU_mips)
#   include "mips32.h"
#   warning Using a Mips32
    typedef soclib::common::Mips32ElIss iss_t;
    const char *default_kernel = "mutekh/kernel-soclib-mips.out";
#elif defined(CPU_ppc)
#   include "ppc405.h"
#   warning Using a PPC405
    typedef soclib::common::Ppc405Iss iss_t;
    const char *default_kernel = "mutekh/kernel-soclib-ppc.out";
#elif defined(CPU_arm)
#   include "arm.h"
#   warning Using an ARM
    typedef soclib::common::ArmIss iss_t;
    const char *default_kernel = "mutekh/kernel-soclib-arm.out";
#endif /* End of CPU switches */



#if defined(USE_GDB_SERVER) && defined(USE_MEMCHECKER)
#   include "gdbserver.h"
#   include "iss_memchecker.h"
    typedef soclib::common::GdbServer<
              soclib::common::IssMemchecker<
                iss_t> > complete_iss_t;
#elif defined(USE_MEMCHECKER)
#   include "iss_memchecker.h"
    typedef soclib::common::IssMemchecker<iss_t> complete_iss_t;
#elif defined(USE_GDB_SERVER)
#   include "gdbserver.h"
    typedef soclib::common::GdbServer<iss_t> complete_iss_t;
#else
    typedef iss_t complete_iss_t;
#endif

int _main(int argc, char *argv[])
{
	using namespace sc_core;
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;
    const size_t ncpu = 4;
    const size_t xicu_n_irq = 2;

	// Define VCI parameters
	typedef soclib::caba::VciParams<4,8,32,1,1,1,8,4,4,1> vci_param;

    const char *kernel = default_kernel;
    if ( argc > 1 ) {
        kernel = argv[1];
    }

	// Mapping table
	soclib::common::Loader loader(kernel);

	soclib::common::MappingTable maptabp(32, IntTab(8), IntTab(8), 0xf0000000);
	
#if defined(CPU_ppc)
	maptabp.add(Segment("rom",  0xbfc00000, 0x00080000, IntTab(0), true));
	maptabp.add(Segment("boot", 0xffffff80, 0x00000080, IntTab(0), true));
#elif defined(CPU_arm)
	maptabp.add(Segment("rom",  0xbfc00000, 0x00080000, IntTab(0), true));
	maptabp.add(Segment("boot", 0x00000000, 0x00000400, IntTab(0), true));
#elif defined(CPU_mips)
	maptabp.add(Segment("rom",  0xbfc00000, 0x00080000, IntTab(0), true));
#endif

	maptabp.add(Segment("tty",  0xd0200000, 0x00000040, IntTab(1), false));
	maptabp.add(Segment("mem",  0x7F400000, 0x00100000, IntTab(2), false ));
	maptabp.add(Segment("xicu", 0xd2200000, 0x00001000, IntTab(3), false));
	maptabp.add(Segment("dma",  0xd1200000, 0x00000020, IntTab(4), false));

#if defined(USE_GDB_SERVER) && defined(USE_MEMCHECKER)
    soclib::common::GdbServer<
        soclib::common::IssMemchecker<
            iss_t> >::set_loader(loader);
#endif
#if defined(USE_MEMCHECKER)
    soclib::common::IssMemchecker<
        iss_t>::init(maptabp, loader, "tty,xicu,dma");
#elif defined(USE_GDB_SERVER)
    soclib::common::GdbServer<iss_t>::set_loader(loader);
#endif

	// Signals

	sc_clock	signal_clk("clk");
	sc_signal<bool> signal_resetn("resetn");

	sc_signal<bool> signal_proc_it[ncpu][iss_t::n_irq]; 
   
	soclib::caba::VciSignals<vci_param> signal_vci_proc[ncpu];

	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param> signal_vci_xicu("signal_vci_xicu");

	soclib::caba::VciSignals<vci_param> signal_vci_dmai("signal_vci_dmai");
	soclib::caba::VciSignals<vci_param> signal_vci_dmat("signal_vci_dmat");

	soclib::caba::VciSignals<vci_param> signal_vci_ram("vci_tgt_ram");
	soclib::caba::VciSignals<vci_param> signal_vci_rom("vci_tgt_rom");

	sc_signal<bool> signal_xicu_irq[xicu_n_irq];

	soclib::caba::VciXcacheWrapper<vci_param, complete_iss_t > *procs[ncpu];

    for ( size_t i = 0; i<ncpu; ++i ) {
        std::ostringstream o;
        o << "proc" << i;
        procs[i] = new soclib::caba::VciXcacheWrapper<vci_param, complete_iss_t >
            (o.str().c_str(), i, maptabp, IntTab(i),4,64,16, 4,64,16);
    }

	soclib::caba::VciRom<vci_param> rom("rom", IntTab(0), maptabp, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(1), maptabp, "vcitty0", NULL);
	soclib::caba::VciRam<vci_param> ram("ram", IntTab(2), maptabp, loader);
	soclib::caba::VciXicu<vci_param> vciicu("vciicu", maptabp,IntTab(3), ncpu, xicu_n_irq, ncpu, ncpu);
	soclib::caba::VciDma<vci_param> vcidma("vcidma", maptabp, IntTab(ncpu), IntTab(4), (1<<(vci_param::K-1)));
	
	soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptabp, ncpu+1, 5, 2, 8);

    for ( size_t i = 0; i<ncpu; ++i ) {
        for ( size_t irq = 0; irq < iss_t::n_irq; ++irq )
            procs[i]->p_irq[irq](signal_proc_it[i][irq]); 
        procs[i]->p_clk(signal_clk);  
        procs[i]->p_resetn(signal_resetn);  
        procs[i]->p_vci(signal_vci_proc[i]);
    }

	rom.p_clk(signal_clk);
	rom.p_resetn(signal_resetn);
	rom.p_vci(signal_vci_rom);

	ram.p_clk(signal_clk);
	ram.p_resetn(signal_resetn);
	ram.p_vci(signal_vci_ram);

	vciicu.p_clk(signal_clk);
	vciicu.p_resetn(signal_resetn);
	vciicu.p_vci(signal_vci_xicu);
    // Input IRQs
    for ( size_t i = 0; i<xicu_n_irq; ++i )
        vciicu.p_hwi[i](signal_xicu_irq[i]);
    // Output IRQs to processor
    for ( size_t i = 0; i<ncpu; ++i )
        vciicu.p_irq[i](signal_proc_it[i][0]);

	vcidma.p_clk(signal_clk);
	vcidma.p_resetn(signal_resetn);
	vcidma.p_irq(signal_xicu_irq[1]); 
	vcidma.p_vci_target(signal_vci_dmat);
	vcidma.p_vci_initiator(signal_vci_dmai);

	vcitty.p_clk(signal_clk);
	vcitty.p_resetn(signal_resetn);
	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_xicu_irq[0]); 

	vgmn.p_clk(signal_clk);
	vgmn.p_resetn(signal_resetn);

    for ( size_t i = 0; i<ncpu; ++i )
        vgmn.p_to_initiator[i](signal_vci_proc[i]);
	vgmn.p_to_initiator[ncpu](signal_vci_dmai);

	vgmn.p_to_target[0](signal_vci_rom);
	vgmn.p_to_target[1](signal_vci_tty);
	vgmn.p_to_target[2](signal_vci_ram);
	vgmn.p_to_target[3](signal_vci_xicu);
	vgmn.p_to_target[4](signal_vci_dmat);

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
