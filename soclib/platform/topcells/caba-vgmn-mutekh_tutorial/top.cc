
#include <iostream>
#include <cstdlib>
#include <stdexcept>

// If you want a GDB server, uncomment the #define below.
//
// Then you may set the environment variable SOCLIB_GDB=START_FROZEN
// before starting the simulator.

#define CONFIG_GDB_SERVER

//#include "mutekh/.config.h"

//# include "iss_memchecker.h"
# include "gdbserver.h"

# include "mips32.h"
# include "ppc405.h"
# include "arm.h"

#include "mapping_table.h"

#include "vci_xcache_wrapper.h"
#include "vci_timer.h"
#include "vci_ram.h"
#include "vci_heterogeneous_rom.h"
#include "vci_multi_tty.h"
#include "vci_locks.h"
#include "vci_icu.h"
#include "vci_vgmn.h"

using namespace soclib;
using common::IntTab;
using common::Segment;

static common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00c00000);

// Define our VCI parameters
typedef caba::VciParams<4,9,32,1,1,1,8,1,1,1> vci_param;

struct CpuEntry;


//**********************************************************************
//               Processor entry and connection code
//**********************************************************************

#define CPU_CONNECT(n) void (n)(CpuEntry *e, sc_core::sc_clock &clk, \
				sc_core::sc_signal<bool> &rstn, caba::VciSignals<vci_param> &m)

struct CpuEntry {
  caba::BaseModule *cpu;
  common::Loader *loader;
  sc_core::sc_signal<bool> *irq_sig;
  size_t irq_sig_count;
  CPU_CONNECT(*connect);
};

template <class Iss>
CPU_CONNECT(cpu_connect)
{
  caba::VciXcacheWrapper<vci_param, Iss> *cpu = static_cast<caba::VciXcacheWrapper<vci_param, Iss> *>(e->cpu);

  cpu->p_clk(clk);
  cpu->p_resetn(rstn);

  e->irq_sig_count = Iss::n_irq;
  e->irq_sig = new sc_core::sc_signal<bool>[Iss::n_irq];

  for ( int irq = 0; irq < Iss::n_irq; ++irq )
    cpu->p_irq[irq](e->irq_sig[irq]); 

  cpu->p_vci(m);
}


//**********************************************************************
//                     Processor creation code
//**********************************************************************

#if defined(CONFIG_GDB_SERVER)
# if defined(CONFIG_SOCLIB_MEMCHECK)
#  warning Using GDB and memchecker
#  define ISS_NEST(T) common::GdbServer<soclib::common::IssMemchecker<T> >
# else
#  warning Using GDB
#  define ISS_NEST(T) common::GdbServer<T>
# endif
#elif defined(CONFIG_SOCLIB_MEMCHECK)
# warning Using Memchecker
# define ISS_NEST(T) common::IssMemchecker<T>
#else
# warning Using raw processor
# define ISS_NEST(T) T
#endif

template <class Iss>
void newCpu(CpuEntry *e, const std::string &type, int id)
{
  std::ostringstream o;
  o << type << "_" << id;

  e->cpu = new caba::VciXcacheWrapper<vci_param, ISS_NEST(Iss)>(o.str().c_str(), id, maptab, IntTab(id),1, 8, 4, 1, 8, 4);
  e->connect = cpu_connect<ISS_NEST(Iss)>;
}

struct CpuEntry * newCpuEntry(const std::string &type, int id)
{
  CpuEntry *e = new CpuEntry;

  e->cpu = 0;

  switch (type[0])
    {
    case 'm':
      if (type == "mips32el" || type == "mips32")
	newCpu<common::Mips32ElIss>(e, type, id);
      else if (type == "mips32eb")
	newCpu<common::Mips32EbIss>(e, type, id);
      break;

    case 'a':
      if (type == "arm")
	newCpu<common::ArmIss>(e, type, id);
      break;

    case 'p':
      if (type == "ppc405" || type == "ppc")
	newCpu<common::Ppc405Iss>(e, type, id);
      break;
    }

  if (!e->cpu)
    throw std::runtime_error(type + ": wrong processor type");

  return e;
}


//**********************************************************************
//                     Args parsing and netlist
//**********************************************************************

int _main(int argc, char **argv)
{
	// Avoid repeating these everywhere
	std::vector<CpuEntry*> cpus;

	if (argc < 2)
	  {
	    std::cerr << std::endl << "usage: " << *argv << " kernel-binary-file:cpu-type[:count] ... " << std::endl;
	    exit(0);
	  }
	argc--;
	argv++;

	for (int i = 0; i < argc; i++)
	  {
	    char *arg = argv[i];

	    const char *kernel_p = strsep(&arg, ":");
	    if (!kernel_p)
	      throw std::runtime_error("missing kernel file name");

	    const char *type_p = strsep(&arg, ":");
	    if (!type_p)
	      throw std::runtime_error("missing cpu type name");

	    const char *count_p = strsep(&arg, ":");
	    int count = count_p ? atoi(count_p) : 1;

	    common::Loader *loader = new common::Loader(kernel_p);

	    for (int j = 0; j < count; j++)
	      {
		int id = cpus.size();
		CpuEntry *e = newCpuEntry(type_p, id);
		e->loader = loader;
		cpus.push_back(e);
	      }
	  }

	// Mapping table

	maptab.add(Segment("resetarm",  0x00000000, 0x0400, IntTab(0), true));
	maptab.add(Segment("resetmips", 0xbfc00000, 0x2000, IntTab(0), true));
	maptab.add(Segment("resetppc",  0xffffff80, 0x0080, IntTab(0), true));

        maptab.add(Segment("text" , 0x60100000, 0x00100000, IntTab(0), true));
        maptab.add(Segment("data" , 0x61100000, 0x00100000, IntTab(1), true));
        maptab.add(Segment("udata", 0x62600000, 0x00100000, IntTab(1), false));

	maptab.add(Segment("tty"  , 0x90600000, 0x00000010, IntTab(2), false));
	maptab.add(Segment("timer", 0x01620000, 0x00000100, IntTab(3), false));
	maptab.add(Segment("icu",   0x20600000, 0x00000020, IntTab(4), false));

	// Signals

	sc_core::sc_clock signal_clk("signal_clk");
	sc_core::sc_signal<bool> signal_resetn("signal_resetn");

	caba::VciSignals<vci_param> signal_vci_m[cpus.size()];

	caba::VciSignals<vci_param> signal_vci_icu("signal_vci_icu");
	caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
	caba::VciSignals<vci_param> signal_vci_vcitimer("signal_vci_vcitimer");
	caba::VciSignals<vci_param> signal_vci_vcimultiram1("signal_vci_vcimultiram1");

	sc_core::sc_signal<bool> signal_icu_irq[2];

	// Components

#if defined(CONFIG_SOCLIB_MEMCHECK)
	Processor::init(maptab, loader, "tty,timer,icu");
#endif

	caba::VciRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, *cpus[0]->loader);
	caba::VciRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, *cpus[0]->loader);

	caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(2), maptab, "vcitty", NULL);
	caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(3), maptab, 1);
	caba::VciIcu<vci_param> vciicu("vciicu", IntTab(4), maptab, 2);

	caba::VciVgmn<vci_param> vgmn("vgmn",maptab, cpus.size(), 5, 2, 8);

	//	Net-List
	for ( size_t i = 0; i < cpus.size(); ++i ) {
	  cpus[i]->connect(cpus[i], signal_clk, signal_resetn, signal_vci_m[i]);
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
	vciicu.p_irq(cpus[0]->irq_sig[0]);

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
