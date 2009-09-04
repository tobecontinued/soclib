/*****************************************************************************
  Filename : top.cc 

  Authors:
  Fabien Colas-Bigey THALES COM - AAL, 2009
  Pierre-Edouard BEAUCAMPS, THALES COM - AAL, 2009

  Copyright (C) THALES COMMUNICATIONS
 
  This code is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.
   
  This code is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.
   
  You should have received a copy of the GNU General Public License along
  with this code (see file COPYING).  If not, see
  <http://www.gnu.org/licenses/>.
  
  This License does not grant permission to use the name of the copyright
  owner, except only as required above for reproducing the content of
  the copyright notice.
*****************************************************************************/

#include <iostream>
#include <cstdlib>

#define CONFIG_GDB_SERVER
//#define CONFIG_DMA
#define CONFIG_VGMN_LATENCY 2
#define VIDEO_TYPE_QCIF	// Video type : CIF(352x288) or QCIF(176x144)
//#define CONFIG_VCACHE
#define CONFIG_XCACHE
#define CONFIG_FRAMEBUFFER
#define CONFIG_RAMDISK

#include "mutekh/.config.h"
#include "mapping_table.h"

#if defined(CONFIG_CPU_MIPS)
#	include "mips32.h"
#elif defined(CONFIG_CPU_PPC)
#	include "ppc405.h"
#elif defined(CONFIG_CPU_ARM)
#	include "arm.h"
#endif

#ifdef CONFIG_SOCLIB_MEMCHECK
#	include "iss_memchecker.h"
#endif

#ifdef CONFIG_GDB_SERVER
#	include "gdbserver.h"
#endif

#ifdef CONFIG_FRAMEBUFFER
#	include "vci_framebuffer.h"
#	warning FRAMEBUFFER added on the platform
#endif

#ifdef CONFIG_DMA
#	include "vci_dma.h"
#	warning DMA added on the platform
#endif

#ifdef CONFIG_DRIVER_TIMER_SOCLIB
#	include "vci_timer.h"
#	warning Timer added on the platform
#endif

#include "loader.h"
#if defined(CONFIG_XCACHE) && !defined(CONFIG_VCACHE)
#	include "vci_xcache_wrapper.h"
#	warning Using Xcache
#elif defined(CONFIG_VCACHE) && !defined(CONFIG_XCACHE)
#	include "vci_vcache_wrapper.h"
#	warning Using Vcache
#else
#	error Processor cache (XCache or VCache) non explicitely defined
#endif

#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_locks.h"
#include "vci_icu.h"
#include "vci_vgmn.h"

#include "soclib_addresses.h"

#define SEGTYPEMASK 0xf0000000

// You may set the SOCLIB_GDB environment variable to
// START_FROZEN before starting the simulator.

int _main(int argc, char *argv[])
{
  int nb_port_icu_irq = 4;
  int nb_initiator = CONFIG_CPU_MAXCOUNT - 1;	//Each CPU	
  int nb_target = 0;

  // Avoid repeating these everywhere
  using namespace sc_core;
  using soclib::common::IntTab;
  using soclib::common::Segment;

  if (argc < 2)
    return 1;

#if defined(CONFIG_CPU_MIPS)
#	ifdef CONFIG_CPU_ENDIAN_BIG
#	warning Using Processor ISS : MIPS32Eb
  typedef soclib::common::Mips32EbIss ProcessorIss;
#	elif defined(CONFIG_CPU_ENDIAN_LITTLE)
#	warning Using Processor ISS : MIPS32El
  typedef soclib::common::Mips32ElIss ProcessorIss;
#	else
#	 error No endian configuration defined
#	endif
#elif defined(CONFIG_CPU_PPC)
#	warning Using Processor ISS : PPC
  typedef soclib::common::Ppc405Iss ProcessorIss;
#elif defined(CONFIG_CPU_ARM)
#	warning Using Processor ISS : ARM
  typedef soclib::common::ArmIss ProcessorIss;
#else
#	 error No supported processor ISS configuration defined
#endif

#ifdef CONFIG_GDB_SERVER
#	ifdef CONFIG_SOCLIB_MEMCHECK
#	warning Using GDB and memchecker
  typedef soclib::common::GdbServer<soclib::common::IssMemchecker<ProcessorIss> > Processor;
#	else
#	warning Using GDB
  typedef soclib::common::GdbServer<ProcessorIss> Processor;
#	endif
#elif defined(CONFIG_SOCLIB_MEMCHECK)
#	warning Using Memchecker
  typedef soclib::common::IssMemchecker<ProcessorIss> Processor;
#else
#	warning Using raw processor
  typedef ProcessorIss Processor;
#endif

  // Froze the simulation if definition of a second argument
  //#ifdef CONFIG_GDB_SERVER
  //	if (argc==3) Processor::start_frozen();
  //#endif
  //#ifdef CONFIG_SOCLIB_MEMCHECK
  std::string exclusions = "icu,tty,locks";
  //#endif


#if (CONFIG_CPU_MAXCOUNT < 1)
#	error You must specify at least one processor
#elif (CONFIG_CPU_MAXCOUNT == 1)
#       warning Platform with 1 processor
#elif (CONFIG_CPU_MAXCOUNT == 2)
#       warning Platform with 2 processors
#elif (CONFIG_CPU_MAXCOUNT == 3)
#       warning Platform with 3 processors
#elif (CONFIG_CPU_MAXCOUNT == 4)
#       warning Platform with 4 processors
#else
#       warning Platform with more than 4 processors
#endif

  // Define our VCI parameters
  typedef soclib::caba::VciParams<4,9,32,1,1,1,8,1,1,1> vci_param;


  // Mapping table
  soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), SEGTYPEMASK);

  maptab.add(Segment("reset", DSX_SEGMENT_RESET_ADDR, DSX_SEGMENT_RESET_SIZE, IntTab(nb_target), true));
  maptab.add(Segment("excep", DSX_SEGMENT_EXCEP_ADDR, DSX_SEGMENT_EXCEP_SIZE, IntTab(nb_target), true));
  //	maptab.add(Segment("text", DSX_SEGMENT_TEXT_ADDR, DSX_SEGMENT_TEXT_SIZE, IntTab(nb_target), false));
  maptab.add(Segment("text", DSX_SEGMENT_TEXT_ADDR, DSX_SEGMENT_TEXT_SIZE, IntTab(nb_target), true));

  maptab.add(Segment("data", DSX_SEGMENT_DATA_CACHED_ADDR, DSX_SEGMENT_DATA_CACHED_SIZE, IntTab(++nb_target), true));
  maptab.add(Segment("udata", DSX_SEGMENT_DATA_UNCACHED_ADDR, DSX_SEGMENT_DATA_UNCACHED_SIZE, IntTab(nb_target), false));

  maptab.add(Segment("icu", DSX_SEGMENT_ICU_ADDR, DSX_SEGMENT_ICU_SIZE, IntTab(++nb_target), false));
  maptab.add(Segment("tty", DSX_SEGMENT_TTY_ADDR, DSX_SEGMENT_TTY_SIZE, IntTab(++nb_target), false));
  maptab.add(Segment("locks", DSX_SEGMENT_SEM_ADDR, DSX_SEGMENT_SEM_SIZE, IntTab(++nb_target), false));
#ifdef CONFIG_FRAMEBUFFER
  maptab.add(Segment("framebuffer", DSX_SEGMENT_FB_ADDR, DSX_SEGMENT_FB_SIZE, IntTab(++nb_target), false));
  exclusions += ",framebuffer";
#endif
#ifdef CONFIG_RAMDISK
  maptab.add(Segment("ramdisk", DSX_SEGMENT_RAMDISK_ADDR, DSX_SEGMENT_RAMDISK_SIZE, IntTab(++nb_target), false));
  exclusions += ",ramdisk";
#endif
#ifdef CONFIG_DRIVER_TIMER_SOCLIB
  maptab.add(Segment("timer", DSX_SEGMENT_TIMER_ADDR, DSX_SEGMENT_TIMER_SIZE, IntTab(++nb_target), false));
  exclusions += ",timer";
#endif
#ifdef CONFIG_DMA
  maptab.add(Segment("dma", DSX_SEGMENT_DMA_ADDR, DSX_SEGMENT_DMA_SIZE, IntTab(++nb_target), false));
  exclusions += ",dma";
#endif


  // Components
#ifdef CONFIG_RAMDISK
  soclib::common::Loader loader(argv[1], "yuv_test.bin@0x68200000:D");
#else
  soclib::common::Loader loader(argv[1]);
#endif

#ifdef CONFIG_SOCLIB_MEMCHECK
  Processor::init(maptab, loader, exclusions);
#endif

#if defined(CONFIG_XCACHE)
  soclib::caba::VciXcacheWrapper<vci_param, Processor> *cache[CONFIG_CPU_MAXCOUNT];
#elif defined(CONFIG_VCACHE)
  soclib::caba::VciVCacheWrapper<vci_param, Processor> *cache[CONFIG_CPU_MAXCOUNT];
#endif
  std::ostringstream cache_cpu_name;
  for (int i = 0 ; i < CONFIG_CPU_MAXCOUNT ; i++) {
    cache_cpu_name.str("");
    cache_cpu_name << "cache" << i;
#if defined(CONFIG_XCACHE)
    cache[i] = new soclib::caba::VciXcacheWrapper<vci_param, Processor > ((cache_cpu_name.str()).c_str(), i, maptab, IntTab(i), 1, 8, 4, 1, 8, 4);
#elif defined(CONFIG_VCACHE)
    cache[i] = new soclib::caba::VciVCacheWrapper<vci_param, Processor > ((cache_cpu_name.str()).c_str(), i, maptab, IntTab(i), 4, 4, 4, 16, 4, 4, 4, 16, 4, 64, 16, 4, 64, 16);
#endif
  }

  nb_target = 0;
  soclib::caba::VciRam<vci_param> vcimultiram0("vcimultiram0", IntTab(nb_target), maptab, loader);
  soclib::caba::VciRam<vci_param> vcimultiram1("vcimultiram1", IntTab(++nb_target), maptab, loader);
  soclib::caba::VciIcu<vci_param> vciicu("vciicu", IntTab(++nb_target), maptab, nb_port_icu_irq);
  soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(++nb_target), maptab, "vcitty", NULL);
  soclib::caba::VciLocks<vci_param> vcilocks("vcilocks", IntTab(++nb_target), maptab); 

#ifdef CONFIG_FRAMEBUFFER
#	ifdef VIDEO_TYPE_CIF
  soclib::caba::VciFrameBuffer<vci_param> vciframebuffer("vciframebuffer", IntTab(++nb_target), maptab, 352, 288);
#	warning Using CIF video size
#	elif defined(VIDEO_TYPE_QCIF)
  soclib::caba::VciFrameBuffer<vci_param> vciframebuffer("vciframebuffer", IntTab(++nb_target), maptab, 176, 144);
#	warning Using QCIF video size
#	else
#	error No video type (CIF or QCIF) configuration defined
#	endif
#endif
#ifdef CONFIG_RAMDISK
  soclib::caba::VciRam<vci_param> vciramdisk("vciramdisk", IntTab(++nb_target), maptab, loader);
#endif

#ifdef CONFIG_DRIVER_TIMER_SOCLIB
  soclib::caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(++nb_target), maptab, 1);
#endif

#ifdef CONFIG_DMA
  soclib::caba::VciDma<vci_param> vcidma("vcidma", maptab, IntTab(++nb_initiator), IntTab(++nb_target), sizeof(int));
#endif

  soclib::caba::VciVgmn<vci_param> vgmn("vgmn", maptab, ++nb_initiator, ++nb_target, CONFIG_VGMN_LATENCY, 8);


  // Connection of components to all signals and ports
  nb_target = 0;

  // Clock & Reset Signals
  sc_clock signal_clk("signal_clk");
  sc_signal<bool> signal_resetn("signal_resetn");

  // Bus VGMN
  vgmn.p_clk(signal_clk);
  vgmn.p_resetn(signal_resetn);

  // CPUs
  sc_core::sc_signal<bool> *signal_cpu_irq[CONFIG_CPU_MAXCOUNT][ProcessorIss::n_irq];
  soclib::caba::VciSignals<vci_param> *signal_cpu_vci[CONFIG_CPU_MAXCOUNT];

  std::ostringstream signal_name_cpu_irq;
  std::ostringstream signal_name_cpu_vci;
  for (int i=0 ; i < CONFIG_CPU_MAXCOUNT ; i++) {
    cache[i]->p_clk(signal_clk);
    cache[i]->p_resetn(signal_resetn);

    for (int i_irq=0 ; i_irq < ProcessorIss::n_irq ; i_irq++) {
      signal_name_cpu_irq.str("");
      signal_name_cpu_irq << "signal_cpu" << i << "_it" << i_irq;
      signal_cpu_irq[i][i_irq] = new sc_core::sc_signal<bool> ((signal_name_cpu_irq.str()).c_str());
      cache[i]->p_irq[i_irq](*signal_cpu_irq[i][i_irq]); 
    }

    signal_name_cpu_vci.str("");
    signal_name_cpu_vci << "signal_cpu" << i << "_vci";
    signal_cpu_vci[i] = new soclib::caba::VciSignals<vci_param> ((signal_name_cpu_vci.str()).c_str());
    cache[i]->p_vci(*signal_cpu_vci[i]);
    vgmn.p_to_initiator[i](*signal_cpu_vci[i]);
  }

  // Multiram0 (Reset + Text + Except)
  soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
  vcimultiram0.p_clk(signal_clk);
  vcimultiram0.p_resetn(signal_resetn);
  vcimultiram0.p_vci(signal_vci_vcimultiram0);
  vgmn.p_to_target[nb_target](signal_vci_vcimultiram0);

  // Multiram1 (cached & uncached data)
  soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram1("signal_vci_vcimultiram1");
  vcimultiram1.p_clk(signal_clk);
  vcimultiram1.p_resetn(signal_resetn);
  vcimultiram1.p_vci(signal_vci_vcimultiram1);
  vgmn.p_to_target[++nb_target](signal_vci_vcimultiram1);

  // ICU
  soclib::caba::VciSignals<vci_param> signal_vci_icu("signal_vci_icu");
  sc_core::sc_signal<bool> *signal_icu_irq[nb_port_icu_irq];

  vciicu.p_clk(signal_clk);
  vciicu.p_resetn(signal_resetn);
  vciicu.p_irq(*signal_cpu_irq[0][0]);

  for (int i=0 ; i < nb_port_icu_irq; i++) {
    signal_icu_irq[i] = new sc_core::sc_signal<bool> ("signal_icu_irq" + i);
    vciicu.p_irq_in[i](*signal_icu_irq[i]); 
  }

  vciicu.p_vci(signal_vci_icu);
  vgmn.p_to_target[++nb_target](signal_vci_icu);

  // Terminal TTY
  soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
  vcitty.p_clk(signal_clk);
  vcitty.p_resetn(signal_resetn);
  vcitty.p_irq[0](*signal_icu_irq[1]);

  vcitty.p_vci(signal_vci_tty);
  vgmn.p_to_target[++nb_target](signal_vci_tty);

  // Locks
  soclib::caba::VciSignals<vci_param> signal_vci_vcilocks("signal_vci_vcilocks");
  vcilocks.p_clk(signal_clk);
  vcilocks.p_resetn(signal_resetn);

  vcilocks.p_vci(signal_vci_vcilocks);
  vgmn.p_to_target[++nb_target](signal_vci_vcilocks);

  // Framebuffer
#ifdef CONFIG_FRAMEBUFFER
  soclib::caba::VciSignals<vci_param> signal_vci_framebuffer("signal_vci_framebuffer");

  vciframebuffer.p_clk(signal_clk);
  vciframebuffer.p_resetn(signal_resetn);

  vciframebuffer.p_vci(signal_vci_framebuffer);
  vgmn.p_to_target[++nb_target](signal_vci_framebuffer);
#endif

  // Ramdisk
#ifdef CONFIG_RAMDISK
  soclib::caba::VciSignals<vci_param> signal_vci_ramdisk("signal_vci_ramdisk");

  vciramdisk.p_clk(signal_clk);
  vciramdisk.p_resetn(signal_resetn);

  vciramdisk.p_vci(signal_vci_ramdisk);
  vgmn.p_to_target[++nb_target](signal_vci_ramdisk);
#endif

  // Timer
#ifdef CONFIG_DRIVER_TIMER_SOCLIB
  soclib::caba::VciSignals<vci_param> signal_vci_vcitimer("signal_vci_vcitimer");

  vcitimer.p_clk(signal_clk);
  vcitimer.p_resetn(signal_resetn);
  vcitimer.p_irq[0](*signal_icu_irq[0]);

  vcitimer.p_vci(signal_vci_vcitimer);
  vgmn.p_to_target[++nb_target](signal_vci_vcitimer);
#endif

  // DMA
#ifdef CONFIG_DMA
  soclib::caba::VciSignals<vci_param> signal_vci_dma_target("signal_vci_dma_target");
  soclib::caba::VciSignals<vci_param> signal_vci_dma_initiator("signal_vci_dma_initiator");

  vcidma.p_clk(signal_clk);
  vcidma.p_resetn(signal_resetn);
  vcidma.p_irq(*signal_icu_irq[3]);

  vcidma.p_vci_target(signal_vci_dma_target);		// Target port
  vgmn.p_to_target[++nb_target](signal_vci_dma_target);
  vcidma.p_vci_initiator(signal_vci_dma_initiator);	// Initiator port
  vgmn.p_to_initiator[nb_initiator - 1](signal_vci_dma_initiator);
#endif


#ifdef SYSTEMCASS
  sc_core::sc_start(0);
  signal_resetn = false;
  sc_core::sc_start(1);
  signal_resetn = true;
#else
  sc_core::sc_start(sc_core::sc_time(0, sc_core::SC_NS));
  signal_resetn = false;
  sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_NS));
  signal_resetn = true;
#endif

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
