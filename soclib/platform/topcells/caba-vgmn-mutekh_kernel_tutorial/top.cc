
#include <iostream>
#include <cstdlib>
#include <stdexcept>

// If you want a GDB server, uncomment the #define below.
//
// Then you may set the environment variable SOCLIB_GDB
// to different values before starting the simulator.
// see https://www.soclib.fr/trac/dev/wiki/Tools/GdbServer

#define CONFIG_GDB_SERVER
//#define CONFIG_SOCLIB_MEMCHECK

# include "iss_memchecker.h"
# include "gdbserver.h"

# include "mips32.h"
# include "ppc405.h"
# include "arm.h"

#include "mapping_table.h"

#include "vci_xcache_wrapper.h"
#include "vci_ram.h"
#include "vci_heterogeneous_rom.h"
#include "vci_multi_tty.h"
#include "vci_locks.h"
#include "vci_xicu.h"
#include "vci_vgmn.h"

using namespace soclib;
using common::IntTab;
using common::Segment;

static common::MappingTable maptab(32, IntTab(8), IntTab(8), 0xfff00000);

// Define our VCI parameters
typedef caba::VciParams<4,9,32,1,1,1,8,1,1,1> vci_param;

struct CpuEntry;


#if defined(CONFIG_GDB_SERVER)
# if defined(CONFIG_SOCLIB_MEMCHECK)
#  warning Using GDB and memchecker
#  define ISS_NEST(T) common::GdbServer<common::IssMemchecker<T> >
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


//**********************************************************************
//               Processor entry and connection code
//**********************************************************************

#define CPU_CONNECT(n) void (n)(CpuEntry *e, sc_core::sc_clock &clk, \
				sc_core::sc_signal<bool> &rstn, caba::VciSignals<vci_param> &m)

#define INIT_TOOLS(n) void (n)(const common::Loader &ldr)

#define NEW_CPU(n) caba::BaseModule * (n)(CpuEntry *e)

struct CpuEntry {
  caba::BaseModule *cpu;
  common::Loader *text_ldr;
  sc_core::sc_signal<bool> *irq_sig;
  size_t irq_sig_count;
  std::string name;
  int id;
  CPU_CONNECT(*connect);
  INIT_TOOLS(*init_tools);
  NEW_CPU(*new_cpu);
};

template <class Iss_>
CPU_CONNECT(cpu_connect)
{
  typedef ISS_NEST(Iss_) Iss;
  caba::VciXcacheWrapper<vci_param, Iss> *cpu = static_cast<caba::VciXcacheWrapper<vci_param, Iss> *>(e->cpu);

  cpu->p_clk(clk);
  cpu->p_resetn(rstn);

  e->irq_sig_count = Iss::n_irq;
  e->irq_sig = new sc_core::sc_signal<bool>[Iss::n_irq];

  for ( size_t irq = 0; irq < Iss::n_irq; ++irq )
    cpu->p_irq[irq](e->irq_sig[irq]); 

  cpu->p_vci(m);
}

template <class Iss>
INIT_TOOLS(initialize_tools)
{
#if defined(CONFIG_GDB_SERVER)
  ISS_NEST(Iss)::set_loader(ldr);
#elif defined(CONFIG_SOCLIB_MEMCHECK)
  common::IssMemchecker<Iss>::init(maptab, ldr, "tty,xicu");
#endif
}

template <class Iss>
NEW_CPU(new_cpu)
{
  return new caba::VciXcacheWrapper<vci_param, ISS_NEST(Iss)>(e->name.c_str(), e->id, maptab, IntTab(e->id), 1, 8, 4, 1, 8, 4);
}

//**********************************************************************
//                     Processor creation code
//**********************************************************************

template <class Iss>
CpuEntry * newCpuEntry_(CpuEntry *e)
{
  e->new_cpu = new_cpu<Iss>;
  e->connect = cpu_connect<Iss>;
  e->init_tools = initialize_tools<Iss>;
  return e;
}

struct CpuEntry * newCpuEntry(const std::string &type, int id, common::Loader *ldr)
{
  CpuEntry *e = new CpuEntry;

  std::ostringstream o;
  o << type << "_" << id;

  e->cpu = 0;
  e->text_ldr = ldr;
  e->name = o.str();
  e->id = id;

  switch (type[0])
    {
    case 'm':
      if (type == "mips32el")
	return newCpuEntry_<common::Mips32ElIss>(e);

      else if (type == "mips32eb")
	return newCpuEntry_<common::Mips32EbIss>(e);

    case 'a':
      if (type == "arm")
	return newCpuEntry_<common::ArmIss>(e);

    case 'p':
      if (type == "ppc405")
	return newCpuEntry_<common::Ppc405Iss>(e);
    }

  throw std::runtime_error(type + ": wrong processor type");
}

//**********************************************************************
//                     Args parsing and netlist
//**********************************************************************

int _main(int argc, char **argv)
{
  // Avoid repeating these everywhere
  std::vector<CpuEntry*> cpus;
  common::Loader data_ldr;

  // Mapping table

  maptab.add(Segment("resetarm",  0x00000000, 0x0400, IntTab(1), true));
  maptab.add(Segment("resetmips", 0xbfc00000, 0x2000, IntTab(1), true));
  maptab.add(Segment("resetppc",  0xffffff80, 0x0080, IntTab(1), true));

  maptab.add(Segment("text" ,     0x60100000, 0x00100000, IntTab(0), true));
  maptab.add(Segment("rodata" ,   0x61100000, 0x01000000, IntTab(1), true));
  maptab.add(Segment("data",      0x71600000, 0x00100000, IntTab(2), false));

  maptab.add(Segment("tty"  ,     0x90600000, 0x00000010, IntTab(3), false));
  maptab.add(Segment("xicu",      0x20600000, 0x00001000, IntTab(4), false));

  if ( (argc < 2) || ((argc % 2) == 0) )
    {
      std::cerr << std::endl << "usage: " << *argv << " < cpu-type[:count] kernel-binary-file > ... " << std::endl;
      exit(0);
    }
  argc--;
  argv++;

  bool heterogeneous = (argc > 2);

  for (int i = 0; i < (argc - 1); i += 2)
    {
      char *cpu_p = argv[i];
      const char *kernel_p = argv[i+1];

      const char *arch_str = strsep(&cpu_p, ":");
      int count = cpu_p ? atoi(cpu_p) : 1;

      common::Loader *text_ldr;

      if (heterogeneous) {
	text_ldr = new common::Loader(std::string(kernel_p) + ";.text");
	data_ldr.load_file(std::string(kernel_p) + ";.rodata;.boot;.excep");
	if (i == 0)
	  data_ldr.load_file(std::string(kernel_p) + ";.data;.cpudata;.contextdata");

      } else {
	text_ldr = new common::Loader(std::string(kernel_p));
	data_ldr.load_file(std::string(kernel_p));
      }

      common::Loader tools_ldr(kernel_p);

      for (int j = 0; j < count; j++) {
	int id = cpus.size();
	CpuEntry *e = newCpuEntry(arch_str, id, text_ldr);

	if (j == 0)
	  e->init_tools(tools_ldr);

	e->cpu = e->new_cpu(e);
	cpus.push_back(e);
      }
    }

  const size_t xicu_n_irq = 1;

  caba::VciHeterogeneousRom<vci_param> vcihetrom("vcihetrom",    IntTab(0), maptab);
  for ( size_t i = 0; i < cpus.size(); ++i )
    vcihetrom.add_srcid(*cpus[i]->text_ldr, IntTab(i));

  caba::VciRam<vci_param> vcirom                ("vcirom",       IntTab(1), maptab, data_ldr);
  caba::VciRam<vci_param> vcimultiram           ("vcimultiram", IntTab(2), maptab);

  caba::VciMultiTty<vci_param> vcitty           ("vcitty",       IntTab(3), maptab, "vcitty", NULL);
  caba::VciXicu<vci_param> vcixicu                ("vcixicu",       maptab, IntTab(4), cpus.size(), xicu_n_irq, cpus.size(), cpus.size());

  caba::VciVgmn<vci_param> vgmn("vgmn",maptab, cpus.size(), 5, 2, 8);

  // Signals

  sc_core::sc_clock signal_clk("signal_clk");
  sc_core::sc_signal<bool> signal_resetn("signal_resetn");

  caba::VciSignals<vci_param> signal_vci_m[cpus.size()];

  caba::VciSignals<vci_param> signal_vci_xicu("signal_vci_xicu");
  caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
  caba::VciSignals<vci_param> signal_vci_vcihetrom("signal_vci_vcihetrom");
  caba::VciSignals<vci_param> signal_vci_vcirom("signal_vci_vcirom");
  caba::VciSignals<vci_param> signal_vci_vcimultiram("signal_vci_vcimultiram");

  sc_core::sc_signal<bool> signal_xicu_irq[xicu_n_irq];

  //	Net-List
  for ( size_t i = 0; i < cpus.size(); ++i ) {
    cpus[i]->connect(cpus[i], signal_clk, signal_resetn, signal_vci_m[i]);
    vgmn.p_to_initiator[i](signal_vci_m[i]);
    vcixicu.p_irq[i](cpus[i]->irq_sig[0]);
  }

  vcimultiram.p_clk(signal_clk);
  vcihetrom.p_clk(signal_clk);
  vcirom.p_clk(signal_clk);
  vcixicu.p_clk(signal_clk);

  vcimultiram.p_resetn(signal_resetn);
  vcihetrom.p_resetn(signal_resetn);
  vcirom.p_resetn(signal_resetn);
  vcixicu.p_resetn(signal_resetn);

  vcihetrom.p_vci(signal_vci_vcihetrom);
  vcirom.p_vci(signal_vci_vcirom);
  vcimultiram.p_vci(signal_vci_vcimultiram);

  vcixicu.p_vci(signal_vci_xicu);

  for ( size_t i = 0; i < xicu_n_irq; ++i )
    vcixicu.p_hwi[i](signal_xicu_irq[i]);

  vcitty.p_clk(signal_clk);
  vcitty.p_resetn(signal_resetn);
  vcitty.p_vci(signal_vci_tty);
  vcitty.p_irq[0](signal_xicu_irq[0]);

  vgmn.p_clk(signal_clk);
  vgmn.p_resetn(signal_resetn);

  vgmn.p_to_target[0](signal_vci_vcihetrom);
  vgmn.p_to_target[1](signal_vci_vcirom);
  vgmn.p_to_target[2](signal_vci_vcimultiram);
  vgmn.p_to_target[3](signal_vci_tty);
  vgmn.p_to_target[4](signal_vci_xicu);

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
