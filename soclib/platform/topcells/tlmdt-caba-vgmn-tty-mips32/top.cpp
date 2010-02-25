/*
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC / Lip6, 2010
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 *
 * Maintainers: alinevieiramello@hotmail.com
 */

#include <iostream>
#include <cstdlib>
#include <sys/timeb.h>

#include "loader.h"           	
#include "mapping_table.h"
#include "mips32.h"
#include "iss2_simhelper.h"
#include "vci_xcache_wrapper.h"
#include "vci_timer.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_locks.h"
#include "vci_vgmn.h"
#include "vci_blackhole.h"
#include "vci_target_transactor.h"
#include "vci_initiator_transactor.h"

#include "segmentation.h"

#ifndef TOP_DEBUG
#define TOP_DEBUG 0
#endif

#ifndef XCACHE_CABA
#define XCACHE_CABA 1
#endif

#ifndef RAM_CABA
#define RAM_CABA 1
#endif

#ifndef TTY_CABA
#define TTY_CABA 1
#endif

using namespace soclib::common;

std::vector<std::string> getTTYNames(int n)
{
  std::vector<std::string> ret;
  for(int i=0; i<n; i++){
    std::ostringstream tty_name;
    tty_name << "tty" << i;
    ret.push_back(tty_name.str());
  }
  return ret;
}

int _main(int argc, char *argv[])
{
  typedef soclib::tlmdt::VciParams<uint32_t,uint32_t> vci_param_tlmdt;
  typedef soclib::caba::VciParams<4,8,32,1,1,1,8,8,8,1> vci_param_caba;
  typedef soclib::common::Mips32ElIss iss_t;
  typedef soclib::common::Iss2Simhelper<iss_t> sim_helper;

  struct timeb initial, final;

  /////////////////////////////////////////////////////////////////////////////
  // VARIABLES
  /////////////////////////////////////////////////////////////////////////////
  size_t network_latence = 10;
  char * network_latence_env; //env variable that says the global interconnect delay
  network_latence_env = getenv("NETWORK_LATENCE");
  if (network_latence_env!=NULL) {
    network_latence = atoi( network_latence_env );
  }

  int n_initiators = 1;
  char * n_initiators_env; //env variable that says the number of initiators to be used
  n_initiators_env = getenv("N_INITS");
  if (n_initiators_env==NULL) {
    printf("WARNING : You should specify the number of initiators in variable N_INITS. For example, export N_INITS=2\n");
    printf("Using 1 initiator\n");
  }else {
    n_initiators = atoi( n_initiators_env );
  }

  int n_rams = 2;

  /////////////////////////////////////////////////////////////////////////////
  // MAPPING TABLE
  /////////////////////////////////////////////////////////////////////////////
  
  MappingTable maptab(32, IntTab(8), IntTab(8), 0x00300000);
  
  maptab.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(0), true));
  maptab.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(0), true));
  maptab.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), true));
  
  maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(1), true));
  
  maptab.add(Segment("loc0" , LOC0_BASE , LOC0_SIZE , IntTab(1), true));
  maptab.add(Segment("loc1" , LOC1_BASE , LOC1_SIZE , IntTab(1), true));
  maptab.add(Segment("loc2" , LOC2_BASE , LOC2_SIZE , IntTab(1), true));
  maptab.add(Segment("loc3" , LOC3_BASE , LOC3_SIZE , IntTab(1), true));
  
  maptab.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(2), false));
  maptab.add(Segment("timer", TIMER_BASE, TIMER_SIZE, IntTab(3), false));
  maptab.add(Segment("locks", LOCKS_BASE, LOCKS_SIZE, IntTab(4), false));
  
  
  /////////////////////////////////////////////////////////////////////////////
  // LOADER
  /////////////////////////////////////////////////////////////////////////////
  Loader loader("soft/bin.soft");

  /////////////////////////////////////////////////////////////////////////////
  // VCI_VGMN
  /////////////////////////////////////////////////////////////////////////////
  soclib::tlmdt::VciVgmn vgmn("vgmn", maptab, n_initiators, 3, network_latence, 8);

  /////////////////////////////////////////////////////////////////////////////
  // VCI_XCACHE_WRAPPER 
  /////////////////////////////////////////////////////////////////////////////
#if XCACHE_CABA
  soclib::caba::VciXcacheWrapper<vci_param_caba, sim_helper>  *xcache[n_initiators];
  soclib::tlmdt::VciInitiatorTransactor<vci_param_caba,vci_param_tlmdt> *init_wrapper[n_initiators];
  soclib::caba::VciSignals<vci_param_caba> *xcache_to_wrapper_p_vci[n_initiators];
  sc_core::sc_signal<bool> *irq[n_initiators][iss_t::n_irq];
  sc_core::sc_signal<bool> *xcache_clock[n_initiators];
  sc_core::sc_signal<bool> *xcache_resetn[n_initiators];
 
  for (int i=0 ; i<n_initiators ; i++) {
    std::ostringstream xcache_name;
    xcache_name << "xcache" << i;
    xcache[i] = new soclib::caba::VciXcacheWrapper<vci_param_caba, sim_helper>((xcache_name.str()).c_str(), i,  maptab, IntTab(i), 1, 32, 8, 1, 32, 8);

    unsigned int wrapper_n_irq = 1;
    std::ostringstream wrapper_name;
    wrapper_name << "init_wrapper" << i;
    init_wrapper[i] = new soclib::tlmdt::VciInitiatorTransactor<vci_param_caba,vci_param_tlmdt>((wrapper_name.str()).c_str(), wrapper_n_irq);
  
    std::ostringstream xcache_wrapper;
    xcache_wrapper << "xcache" << i << "_to_wrapper" << i << "_p_vci";
    xcache_to_wrapper_p_vci[i] = new soclib::caba::VciSignals<vci_param_caba>((xcache_wrapper.str()).c_str());
    
    std::ostringstream xcache_clock_name;
    xcache_clock_name << "xcache_clock" << i;
    xcache_clock[i] = new sc_core::sc_signal<bool>((xcache_clock_name.str()).c_str());

    std::ostringstream xcache_resetn_name;
    xcache_resetn_name << "xcache_resetn" << i;
    xcache_resetn[i] = new sc_core::sc_signal<bool>((xcache_resetn_name.str()).c_str());

    for(unsigned int n=0; n<iss_t::n_irq; n++){
      std::ostringstream irq_name;
      irq_name << "xcache" << i << "_p_irq_" << n << "_";
      irq[i][n] = new sc_core::sc_signal<bool>((irq_name.str()).c_str());
    }
    
    // Component connect
    xcache[i]->p_clk(*xcache_clock[i]);
    xcache[i]->p_resetn(*xcache_resetn[i]);
    xcache[i]->p_vci(*xcache_to_wrapper_p_vci[i]);
    for(unsigned int n=0; n<iss_t::n_irq; n++){
      xcache[i]->p_irq[n](*irq[i][n]);
    }

    init_wrapper[i]->p_clk(*xcache_clock[i]);
    init_wrapper[i]->p_resetn(*xcache_resetn[i]);
    init_wrapper[i]->p_vci_target(*xcache_to_wrapper_p_vci[i]);
    init_wrapper[i]->p_vci_initiator(*vgmn.p_to_initiator[i]);
    for(unsigned int n=0; n<wrapper_n_irq; n++){
      init_wrapper[i]->p_irq_initiator[n](*irq[i][n]);
    }
  }

#if TOP_DEBUG
  printf("Vci_Xcache_Wrapper CABA\n");
#endif
  
#else
  soclib::tlmdt::VciXcacheWrapper<vci_param_tlmdt, sim_helper > *xcache[n_initiators]; 
  soclib::tlmdt::VciBlackhole<tlm::tlm_initiator_socket<> > *fake_initiator[n_initiators];

  for (int i=0 ; i < n_initiators ; i++) {
    std::ostringstream cpu_name;
    cpu_name << "cache" << i;
    xcache[i] = new soclib::tlmdt::VciXcacheWrapper<vci_param_tlmdt, sim_helper >((cpu_name.str()).c_str(), i, IntTab(i), maptab, 1, 4, 8, 1, 4, 8, 500 * UNIT_TIME);
    xcache[i]->p_vci(*vgmn.p_to_initiator[i]);

    std::ostringstream fake_name;
    fake_name << "fake" << i;
    fake_initiator[i] = new soclib::tlmdt::VciBlackhole<tlm::tlm_initiator_socket<> >((fake_name.str()).c_str(), iss_t::n_irq-1);
    
    for(unsigned int irq=0; irq<iss_t::n_irq-1; irq++){
      (*fake_initiator[i]->p_socket[irq])(*xcache[i]->p_irq[irq+1]);
    }
  }
#if TOP_DEBUG
  printf("Vci_Xcache_Wrapper TLMDT\n");
#endif

#endif

  /////////////////////////////////////////////////////////////////////////////
  // VCI_RAM
  /////////////////////////////////////////////////////////////////////////////
#if RAM_CABA

  soclib::caba::VciRam<vci_param_caba> *ram[n_rams];
  soclib::tlmdt::VciTargetTransactor<vci_param_caba,vci_param_tlmdt> *ram_wrapper[n_rams];
  soclib::caba::VciSignals<vci_param_caba> *wrapper_to_ram_p_vci[n_rams];
  sc_core::sc_signal<bool> *ram_clock[n_rams];
  sc_core::sc_signal<bool> *ram_resetn[n_rams];

  for (int i=0 ; i < n_rams ; i++) {
    std::ostringstream ram_name;
    ram_name << "ram" << i;
    ram[i] = new soclib::caba::VciRam<vci_param_caba>((ram_name.str()).c_str(), IntTab(i), maptab, loader);

    std::ostringstream ram_wrapper_name;
    ram_wrapper_name << "ram_wrapper" << i;
    ram_wrapper[i] = new soclib::tlmdt::VciTargetTransactor<vci_param_caba,vci_param_tlmdt>((ram_wrapper_name.str()).c_str());

    std::ostringstream wrapper_to_ram_p_vci_name;
    wrapper_to_ram_p_vci_name << "wrapper_to_ram_p_vci" << i;
    wrapper_to_ram_p_vci[i] = new soclib::caba::VciSignals<vci_param_caba>((wrapper_to_ram_p_vci_name.str()).c_str());

    std::ostringstream ram_clock_name;
    ram_clock_name << "ram_clock" << i;
    ram_clock[i] = new sc_core::sc_signal<bool>((ram_clock_name.str()).c_str());

    std::ostringstream ram_resetn_name;
    ram_resetn_name << "ram_resetn" << i;
    ram_resetn[i] = new sc_core::sc_signal<bool>((ram_resetn_name.str()).c_str());


    ram[i]->p_clk(*ram_clock[i]);
    ram[i]->p_resetn(*ram_resetn[i]);
    ram[i]->p_vci(*wrapper_to_ram_p_vci[i]);

    ram_wrapper[i]->p_clk(*ram_clock[i]);
    ram_wrapper[i]->p_resetn(*ram_resetn[i]);
    ram_wrapper[i]->p_vci_initiator(*wrapper_to_ram_p_vci[i]);
    ram_wrapper[i]->p_vci_target(*vgmn.p_to_target[i]);

  }

#if TOP_DEBUG
  printf("Vci_Ram CABA\n");
#endif

#else
  soclib::tlmdt::VciRam<vci_param_tlmdt> *ram[n_rams];

  for (int i=0 ; i < n_rams ; i++) {
    std::ostringstream ram_name;
    ram_name << "ram" << i;
    ram[i] = new soclib::tlmdt::VciRam<vci_param_tlmdt>((ram_name.str()).c_str(), IntTab(i), maptab, loader);
   (*vgmn.p_to_target[i])(ram[i]->p_vci);
  }
#endif

  /////////////////////////////////////////////////////////////////////////////
  // VCI_TTY
  /////////////////////////////////////////////////////////////////////////////
#if TTY_CABA
  soclib::caba::VciMultiTty<vci_param_caba> vcitty("tty", IntTab(n_rams), maptab, getTTYNames(n_initiators));
  soclib::tlmdt::VciTargetTransactor<vci_param_caba,vci_param_tlmdt> tty_wrapper("tty_wrapper", n_initiators);
  soclib::caba::VciSignals<vci_param_caba> wrapper_to_tty_p_vci;
  sc_core::sc_signal<bool> *tty_irq[n_initiators];
  sc_core::sc_signal<bool> tty_clock("tty_clock");
  sc_core::sc_signal<bool> tty_resetn("tty_resetn");

  for (int i=0; i<n_initiators; i++) {
    std::ostringstream tty_irq_name;
    tty_irq_name << "tty_p_irq_" << i;
    tty_irq[i] = new sc_core::sc_signal<bool>((tty_irq_name.str()).c_str());
  }

  tty_wrapper.p_clk(tty_clock);
  tty_wrapper.p_resetn(tty_resetn);
  tty_wrapper.p_vci_initiator(wrapper_to_tty_p_vci);
  tty_wrapper.p_vci_target(*vgmn.p_to_target[n_rams]);
  for (int i=0; i<n_initiators; i++) {
    tty_wrapper.p_irq_target[i](*tty_irq[i]);

#if XCACHE_CABA
    (*tty_wrapper.p_irq_initiator[i])(*init_wrapper[i]->p_irq_target[0]);
#else
    (*tty_wrapper.p_irq_initiator[i])(*xcache[i]->p_irq[0]);
#endif

  }

  vcitty.p_clk(tty_clock);
  vcitty.p_resetn(tty_resetn);
  vcitty.p_vci(wrapper_to_tty_p_vci);
  for (int i=0; i<n_initiators; i++) {
    vcitty.p_irq[i](*tty_irq[i]);
  }

#if TOP_DEBUG
  printf("Vci_Tty CABA\n");
#endif

#else
  soclib::tlmdt::VciMultiTty<vci_param_tlmdt> vcitty("tty", IntTab(n_rams), maptab, getTTYNames(n_initiators));
  (*vgmn.p_to_target[n_rams])(vcitty.p_vci);


  for(int i=0; i<n_initiators; i++){
#if XCACHE_CABA
    (*vcitty.p_irq[i])(*init_wrapper[i]->p_irq_target[i]);
#else
    (*vcitty.p_irq[i])(*xcache[i]->p_irq[0]);
#endif
  }

#if TOP_DEBUG
  printf("Vci_Tty TLMDT\n");
#endif

#endif

  /////////////////////////////////////////////////////////////////////////////
  // START
  /////////////////////////////////////////////////////////////////////////////
  ftime(&initial);
  sc_core::sc_start();
  ftime(&final);
  
  std::cout << "Execution Time = " << (int)((1000.0 * (final.time - initial.time))+ (final.millitm - initial.millitm)) << std::endl << std::endl;
  
  return 0;
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
