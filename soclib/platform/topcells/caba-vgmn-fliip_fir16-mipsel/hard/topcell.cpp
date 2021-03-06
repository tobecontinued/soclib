
/**
 ** MARKER_BEGIN
 ** Auto-generated file, dont modify directly
 ** your changes will be lost !
 **
 ** Generated by DSX on 2009-05-26 14:47:28.398845
 ** by llhours@glazik
 ** MARKER_END
 **/
#include <systemc>
#include <signal.h>
#include <sys/time.h>
#include <cstdlib>
#include <cstdarg>

// 
#include "base_module.h"
#include "iss2.h"
#include "loader.h"
#include "mapping_table.h"
#include "mips32.h"
#include "simhelper.h"
#include "tty.h"
#include "vci_fir16.h"
#include "vci_framebuffer.h"
#include "vci_multi_tty.h"
#include "vci_param.h"
#include "vci_ram.h"
#include "vci_signals.h"
#include "vci_simhelper.h"
#include "vci_vgmn.h"
#include "vci_xcache_wrapper.h"
// Component getIncludes
// Configurator getIncludes
// Signal getIncludes

bool stop;
void run(sc_core::sc_signal<bool> &resetn)
{
#ifdef SYSTEMCASS
	sc_core::sc_start(0);
	resetn = false;
	sc_core::sc_start(1);
	resetn = true;
#else
	sc_core::sc_start(sc_core::sc_time(0, sc_core::SC_NS));
	resetn = false;
	sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_NS));
	resetn = true;
#endif

#ifdef SOCVIEW
	debug();
#else
	const char *t = getenv("STATS_EVERY");
	if ( t ) {
		int delta = atoi(t);
		while (!stop) {
			struct timezone tz;
			struct timeval end, begin, tp;
			gettimeofday( &begin, &tz );
#ifdef SYSTEMCASS
			sc_core::sc_start(delta);
#else
			sc_core::sc_start(sc_core::sc_time(delta, sc_core::SC_NS));
#endif
			gettimeofday( &end, &tz );
			timersub( &end, &begin, &tp );
			long ms = (tp.tv_sec*1000+tp.tv_usec/1000);
			std::cout << std::dec << delta << " cycles in " << ms << " ms: " << ((double)delta*1000/ms) << " Hz" << std::endl;
		}
	} else {
		sc_core::sc_start();
	}
#endif
}

std::vector<std::string> stringArray(
    const char *first, ... )
{
    std::vector<std::string> ret;
	va_list arg;
	va_start(arg, first);
	const char *s = first;
	while(s) {
		ret.push_back(std::string(s));
		s = va_arg(arg, const char *);
	};
	va_end(arg);
    return ret;
}

std::vector<int> intArray(
    const int length, ... )
{
	int i;
    std::vector<int> ret;
	va_list arg;
	va_start(arg, length);

	for (i=0; i<length; ++i) {
		ret.push_back(va_arg(arg, int));
	};
	va_end(arg);
    return ret;
}

int _main(int argc, char **argv)
{
	// 

	// Configurator instanciate
	soclib::common::Loader loader("soft/bin.soft");
	soclib::common::MappingTable mapping_table(32, soclib::common::IntTab(8), soclib::common::IntTab(8), 0x00300000);

	// Configurator configure
	mapping_table.add(soclib::common::Segment("data", 0x10000000, 0x00060000, soclib::common::IntTab(0), 1));
	mapping_table.add(soclib::common::Segment("excep", 0x80000000, 0x00010000, soclib::common::IntTab(0), 1));
	mapping_table.add(soclib::common::Segment("fb0", 0xb2200000, 0x00020000, soclib::common::IntTab(3), 0));
	mapping_table.add(soclib::common::Segment("ip0", 0xb1200000, 256, soclib::common::IntTab(4), 0));
	mapping_table.add(soclib::common::Segment("reset", 0xbfc00000, 512, soclib::common::IntTab(0), 1));
	mapping_table.add(soclib::common::Segment("sim0", 0xb0200000, 256, soclib::common::IntTab(2), 0));
	mapping_table.add(soclib::common::Segment("text", 0x00400000, 0x00050000, soclib::common::IntTab(0), 1));
	mapping_table.add(soclib::common::Segment("tty0", 0xc0200000, 16, soclib::common::IntTab(1), 0));

	// Configurator staticConfigure
	// common:loader has no static config;
	// common:mapping_table has no static config;

	// Component staticConfigure
	// caba:vci_fir16 has no static config;
	// caba:vci_framebuffer has no static config;
	// caba:vci_multi_tty has no static config;
	// caba:vci_param has no static config;
	// caba:vci_ram has no static config;
	// caba:vci_simhelper has no static config;
	// caba:vci_vgmn has no static config;
	// caba:vci_xcache_wrapper has no static config;
	// common:base_module has no static config;
	// common:iss2 has no static config;
	// common:mips32 has no static config;
	// common:mips32el has no static config;
	//;

	// Component instanciate
	soclib::caba::VciFir16<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  ip0("ip0", mapping_table, soclib::common::IntTab(1), soclib::common::IntTab(4), 20);
	soclib::caba::VciFrameBuffer<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  fb0("fb0", soclib::common::IntTab(3), mapping_table, 256, 256, 420);
	soclib::caba::VciMultiTty<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  tty0("tty0", soclib::common::IntTab(1), mapping_table, stringArray("tty0", NULL));
	soclib::caba::VciRam<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  ram0("ram0", soclib::common::IntTab(0), mapping_table, loader);
	soclib::caba::VciSimhelper<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  sim0("sim0", soclib::common::IntTab(2), mapping_table);
	soclib::caba::VciVgmn<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  vgmn0("vgmn0", mapping_table, 2, 5, 2, 8);
	soclib::caba::VciXcacheWrapper<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 > ,soclib::common::Mips32ElIss >  mips0("mips0", 0, mapping_table, soclib::common::IntTab(0), 1, 8, 4, 1, 8, 4);

	// Signal instanciate
	sc_core::sc_clock clock("clock");
	sc_core::sc_signal<bool> ip0_p_irq("ip0_p_irq");
	sc_core::sc_signal<bool> mips0_p_irq_0_("mips0_p_irq_0_");
	sc_core::sc_signal<bool> mips0_p_irq_1_("mips0_p_irq_1_");
	sc_core::sc_signal<bool> mips0_p_irq_2_("mips0_p_irq_2_");
	sc_core::sc_signal<bool> mips0_p_irq_3_("mips0_p_irq_3_");
	sc_core::sc_signal<bool> mips0_p_irq_4_("mips0_p_irq_4_");
	sc_core::sc_signal<bool> mips0_p_irq_5_("mips0_p_irq_5_");
	sc_core::sc_signal<bool> resetn("resetn");
	sc_core::sc_signal<bool> tty0_p_irq_0_("tty0_p_irq_0_");
	soclib::caba::VciSignals<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  fb0_p_vci_to_vgmn0_p_to_target_3_("fb0_p_vci_to_vgmn0_p_to_target_3_");
	soclib::caba::VciSignals<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  ip0_p_vci_initiator_to_vgmn0_p_to_initiator_1_("ip0_p_vci_initiator_to_vgmn0_p_to_initiator_1_");
	soclib::caba::VciSignals<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  ip0_p_vci_target_to_vgmn0_p_to_target_4_("ip0_p_vci_target_to_vgmn0_p_to_target_4_");
	soclib::caba::VciSignals<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  mips0_p_vci_to_vgmn0_p_to_initiator_0_("mips0_p_vci_to_vgmn0_p_to_initiator_0_");
	soclib::caba::VciSignals<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  ram0_p_vci_to_vgmn0_p_to_target_0_("ram0_p_vci_to_vgmn0_p_to_target_0_");
	soclib::caba::VciSignals<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  sim0_p_vci_to_vgmn0_p_to_target_2_("sim0_p_vci_to_vgmn0_p_to_target_2_");
	soclib::caba::VciSignals<soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1 >  >  tty0_p_vci_to_vgmn0_p_to_target_1_("tty0_p_vci_to_vgmn0_p_to_target_1_");

	// Component configure

	// Signal configure

	// Component connect
	fb0.p_clk(clock);
	fb0.p_resetn(resetn);
	fb0.p_vci(fb0_p_vci_to_vgmn0_p_to_target_3_);
	ip0.p_clk(clock);
	ip0.p_irq(ip0_p_irq);
	ip0.p_resetn(resetn);
	ip0.p_vci_initiator(ip0_p_vci_initiator_to_vgmn0_p_to_initiator_1_);
	ip0.p_vci_target(ip0_p_vci_target_to_vgmn0_p_to_target_4_);
	mips0.p_clk(clock);
	mips0.p_irq[0](mips0_p_irq_0_);
	mips0.p_irq[1](mips0_p_irq_1_);
	mips0.p_irq[2](mips0_p_irq_2_);
	mips0.p_irq[3](mips0_p_irq_3_);
	mips0.p_irq[4](mips0_p_irq_4_);
	mips0.p_irq[5](mips0_p_irq_5_);
	mips0.p_resetn(resetn);
	mips0.p_vci(mips0_p_vci_to_vgmn0_p_to_initiator_0_);
	ram0.p_clk(clock);
	ram0.p_resetn(resetn);
	ram0.p_vci(ram0_p_vci_to_vgmn0_p_to_target_0_);
	sim0.p_clk(clock);
	sim0.p_resetn(resetn);
	sim0.p_vci(sim0_p_vci_to_vgmn0_p_to_target_2_);
	tty0.p_clk(clock);
	tty0.p_irq[0](tty0_p_irq_0_);
	tty0.p_resetn(resetn);
	tty0.p_vci(tty0_p_vci_to_vgmn0_p_to_target_1_);
	vgmn0.p_clk(clock);
	vgmn0.p_resetn(resetn);
	vgmn0.p_to_initiator[0](mips0_p_vci_to_vgmn0_p_to_initiator_0_);
	vgmn0.p_to_initiator[1](ip0_p_vci_initiator_to_vgmn0_p_to_initiator_1_);
	vgmn0.p_to_target[0](ram0_p_vci_to_vgmn0_p_to_target_0_);
	vgmn0.p_to_target[1](tty0_p_vci_to_vgmn0_p_to_target_1_);
	vgmn0.p_to_target[2](sim0_p_vci_to_vgmn0_p_to_target_2_);
	vgmn0.p_to_target[3](fb0_p_vci_to_vgmn0_p_to_target_3_);
	vgmn0.p_to_target[4](ip0_p_vci_target_to_vgmn0_p_to_target_4_);

run(resetn);


	return 0;
}

void quit(int)
{
	sc_core::sc_stop();
    stop = true;
}

int sc_main (int argc, char *argv[])
{
	signal(SIGINT, quit);
	atexit(sc_core::sc_stop);
    stop = false;

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
