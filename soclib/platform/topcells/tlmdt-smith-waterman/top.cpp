#include <iostream>
#include <systemc>
#include <sys/time.h>
#include <cstdlib>
#include <cstdarg>
#include <sys/timeb.h>

#include "loader.h"           	
#include "mapping_table.h"
#include "vci_vgmn.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "iss_simhelper.h"
#include "vci_xcache.h"
#include "vci_blackhole.h"

using namespace  soclib::tlmdt;
using namespace  soclib::common;

int sc_main (int   argc, char  *argv[])
{
  typedef VciParams<uint32_t,uint32_t> vci_param;

  struct timeb initial, final;
 
  int n_initiators       = 1;
  size_t dcache_size     = 1024;
  size_t icache_size     = 1024;
  size_t network_latence = 11;

  char * ncpu_env; //env variable that says the number of MIPS processors to be used
  ncpu_env = getenv("N_INITS");
  if (ncpu_env==NULL) {
    printf("WARNING : You have to specify the number of processors to be used in the simulation in the N_INITS variable\n");
  }else {
    n_initiators = atoi( ncpu_env );
  }

  uint32_t simulation_time = std::numeric_limits<uint32_t>::max();
  char * simulation_time_env; //env variable that says the simulation time
  simulation_time_env = getenv("SIMULATION_TIME");

  if (simulation_time_env==NULL) {
    printf("WARNING : You can specify the simulation time in variable SIMULATION_TIME. For example, export SIMULATION_TIME=100000\n");
  }else {
    simulation_time = atoi( simulation_time_env );
  }

  std::cout << "SIMULATION PARAMETERS: number of initiators = " << n_initiators << " simulation time = " << simulation_time << " icache size = " << icache_size << " dcache size = " << dcache_size << " network latence = " << network_latence << std::endl << std::endl;

  /////////////////////////////////////////////////////////////////////////////
  // MAPPING TABLE
  /////////////////////////////////////////////////////////////////////////////
  MappingTable maptab(32, IntTab(8), IntTab(8), 0x00200000);
  maptab.add(Segment("boot",  0xbfc00000,       2048, IntTab(1), 1));
  maptab.add(Segment("cram0", 0x10000000, 0x00100000, IntTab(0), 1));
  maptab.add(Segment("cram1", 0x40000000, 0x00100000, IntTab(1), 1));
  maptab.add(Segment("excep", 0x80000080,       2048, IntTab(1), 1));
  maptab.add(Segment("tty",   0x90200000,         32, IntTab(2), 0));
  maptab.add(Segment("uram0", 0x30200000, 0x00100000, IntTab(0), 0));
  maptab.add(Segment("uram1", 0x60200000, 0x00100000, IntTab(1), 0));

  /////////////////////////////////////////////////////////////////////////////
  // LOADER
  /////////////////////////////////////////////////////////////////////////////
  std::ostringstream soft;
  soft << "soft/bin" << n_initiators << "proc.soft";
  std::cout << "soft:" << soft.str() << std::endl;
  Loader loader(soft.str());

  /////////////////////////////////////////////////////////////////////////////
  // VGMN
  /////////////////////////////////////////////////////////////////////////////
  VciVgmn vgmn_1("vgmn", maptab, IntTab(), n_initiators, 3, network_latence * UNIT_TIME);
 
  /////////////////////////////////////////////////////////////////////////////
  // XCACHE
  /////////////////////////////////////////////////////////////////////////////
  VciXcache<vci_param, IssSimhelper<MipsElIss> > *xcache[n_initiators];
  VciBlackhole<tlm::tlm_initiator_socket<> > *fake_initiator[n_initiators];
  for (int i=0; i<n_initiators; i++) {
    std::ostringstream name;
    name << "xcache" << i;
    xcache[i] = new VciXcache<vci_param, IssSimhelper<MipsElIss> >((name.str()).c_str(), i, IntTab(i), maptab, icache_size, 8, dcache_size, 8,  1000 * UNIT_TIME, simulation_time * UNIT_TIME);
    xcache[i]->p_vci_initiator(vgmn_1.m_RspArbCmdRout[i]->p_vci_target);

    std::ostringstream fake_name;
    fake_name << "fake" << i;
    fake_initiator[i] = new VciBlackhole<tlm::tlm_initiator_socket<> >((fake_name.str()).c_str(), soclib::common::MipsElIss::n_irq);
    
    for(int irq=0; irq<soclib::common::MipsElIss::n_irq; irq++){
      (*fake_initiator[i]->p_socket[irq])(*xcache[i]->p_irq_target[irq]);
    }
    
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // TARGET - RAM
  /////////////////////////////////////////////////////////////////////////////
  int n_rams = 2;
  VciRam<vci_param> *ram[n_rams];
  for (int i=0 ; i<n_rams; i++) {
    std::ostringstream name;
    name << "ram" << i;
    ram[i] = new VciRam<vci_param>((name.str()).c_str(), IntTab(i), maptab, loader);
    vgmn_1.m_CmdArbRspRout[i]->p_vci_initiator(ram[i]->p_vci_target);
  }
  

  /////////////////////////////////////////////////////////////////////////////
  // TARGET - TTY
  /////////////////////////////////////////////////////////////////////////////
  VciMultiTty<vci_param> vcitty("tty0", IntTab(n_rams), maptab, "TTY0", NULL);
  vgmn_1.m_CmdArbRspRout[n_rams]->p_vci_initiator(vcitty.p_vci_target);

  VciBlackhole<tlm_utils::simple_target_socket_tagged<VciBlackholeBase, 32, tlm::tlm_base_protocol_types> > *fake_target_tagged;
  
  fake_target_tagged = new VciBlackhole<tlm_utils::simple_target_socket_tagged<VciBlackholeBase, 32, tlm::tlm_base_protocol_types> >("fake_target_tagged", 1);
  
  (*vcitty.p_irq_initiator[0])(*fake_target_tagged->p_socket[0]);

  /////////////////////////////////////////////////////////////////////////////
  // SIMULATION
  /////////////////////////////////////////////////////////////////////////////
  ftime(&initial);

  sc_core::sc_start();  // start the simulation

  ftime(&final);

  std::cout << "Execution Time = " << (int)((1000.0 * (final.time - initial.time))+ (final.millitm - initial.millitm)) << std::endl;

  for (int i=0 ; i<n_rams; i++) {
    ram[i]->print_stats();
  }

  vcitty.print_stats();

  return 0;
}
