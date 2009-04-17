#include <sys/timeb.h>

#include "loader.h"           	
#include "mapping_table.h"
#include "my_initiator.h"
#include "vci_ram.h"
#include "vci_mwmr_controller.h"
#include "coprocessor.h"
#include "vci_vgmn.h"

using namespace soclib::tlmdt;
using namespace soclib::common;

int sc_main (int   argc, char  *argv[])
{
  typedef VciParams<uint32_t,uint32_t> vci_param;

  struct timeb initial, final;
 
  size_t read_depth, write_depth, n_read_channel, n_write_channel, n_config, n_status;
  size_t network_latence = 10;

  uint32_t simulation_time = std::numeric_limits<uint32_t>::max();
  char * simulation_time_env; //env variable that says the simulation time
  simulation_time_env = getenv("SIMULATION_TIME");
  if (simulation_time_env==NULL) {
    printf("WARNING : You can specify the simulation time in variable SIMULATION_TIME. For example, export SIMULATION_TIME=100000\n");
  }else {
    simulation_time = atoi( simulation_time_env );
  }

  std::cout << "SIMULATION PARAMETERS: simulation time = " << simulation_time << " network latence = " << network_latence << std::endl << std::endl;

  /////////////////////////////////////////////////////////////////////////////
  // MAPPING TABLE
  /////////////////////////////////////////////////////////////////////////////
  MappingTable maptab(32, IntTab(20), IntTab(20), 0x00200000);
  maptab.add(Segment("ram0",        0xbfc00000, 0x00010000, IntTab(0), 1));
  maptab.add(Segment("mwmr0",       0x10000000, 0x00010000, IntTab(1), 1));
  maptab.add(Segment("mwmr1",       0x20000000, 0x00010000, IntTab(2), 1));
  //  maptab.add(Segment("mwmr2",       0x30000000, 0x00010000, IntTab(3), 1));
  // maptab.add(Segment("mwmr3",       0x40000000, 0x00010000, IntTab(4), 1));

  /////////////////////////////////////////////////////////////////////////////
  // LOADER
  /////////////////////////////////////////////////////////////////////////////
  Loader loader("soft/bin.soft");

  /////////////////////////////////////////////////////////////////////////////
  // VGMN
  /////////////////////////////////////////////////////////////////////////////
  VciVgmn vgmn_1("vgmn", maptab, IntTab(), 3, 3, network_latence * UNIT_TIME);

  /////////////////////////////////////////////////////////////////////////////
  // INITIATOR
  /////////////////////////////////////////////////////////////////////////////
  my_initiator *initiator = new my_initiator("init", IntTab(0), maptab, 1000 * UNIT_TIME, simulation_time * UNIT_TIME);
  initiator->p_vci_initiator(vgmn_1.m_RspArbCmdRout[0]->p_vci_target);

  /////////////////////////////////////////////////////////////////////////////
  // RAM 0
  /////////////////////////////////////////////////////////////////////////////
  VciRam<vci_param> *ram0 = new VciRam<vci_param>("ram0", IntTab(0), maptab, loader);
  vgmn_1.m_CmdArbRspRout[0]->p_vci_initiator(ram0->p_vci_target);

  /////////////////////////////////////////////////////////////////////////////
  // MWMR AND COPROCESSOR 0
  /////////////////////////////////////////////////////////////////////////////
  read_depth      = 0;
  write_depth     = 48;
  n_read_channel  = 0;
  n_write_channel = 1;
  n_config        = 1;
  n_status        = 1;

  VciMwmrController<vci_param> *mwmr0 = new VciMwmrController<vci_param>("mwmr0", maptab, IntTab(1), IntTab(1), read_depth, write_depth, n_read_channel, n_write_channel, n_config, n_status, simulation_time * UNIT_TIME);

  mwmr0->p_vci_initiator(vgmn_1.m_RspArbCmdRout[1]->p_vci_target);
  vgmn_1.m_CmdArbRspRout[1]->p_vci_initiator(mwmr0->p_vci_target);

  Coprocessor *copro0 = new Coprocessor("copro0", 0, read_depth/4, write_depth/4, n_read_channel, n_write_channel, n_config, n_status);

  for(uint32_t i=0; i<n_read_channel; i++)
    (*mwmr0->p_read_fifo[i])(*copro0->p_read_fifo[i]);

  for(uint32_t i=0; i<n_write_channel; i++)
    (*mwmr0->p_write_fifo[i])(*copro0->p_write_fifo[i]);

  for(uint32_t i=0; i<n_config; i++)
    (*mwmr0->p_config[i])(*copro0->p_config[i]);

  for(uint32_t i=0; i<n_status; i++)
    (*mwmr0->p_status[i])(*copro0->p_status[i]);

  /////////////////////////////////////////////////////////////////////////////
  // MWMR AND COPROCESSOR 1
  /////////////////////////////////////////////////////////////////////////////
  read_depth      = 48;
  write_depth     = 0;
  n_read_channel  = 1;
  n_write_channel = 0;
  n_config        = 1;
  n_status        = 1;

  VciMwmrController<vci_param> *mwmr1 = new VciMwmrController<vci_param>("mwmr1", maptab, IntTab(2), IntTab(2), read_depth, write_depth, n_read_channel, n_write_channel, n_config, n_status, simulation_time * UNIT_TIME);

  mwmr1->p_vci_initiator(vgmn_1.m_RspArbCmdRout[2]->p_vci_target);
  vgmn_1.m_CmdArbRspRout[2]->p_vci_initiator(mwmr1->p_vci_target);

  Coprocessor *copro1 = new Coprocessor("copro1", 1, read_depth/4, write_depth/4, n_read_channel, n_write_channel, n_config, n_status);

  for(uint32_t i=0; i<n_read_channel; i++)
    (*mwmr1->p_read_fifo[i])(*copro1->p_read_fifo[i]);

  for(uint32_t i=0; i<n_write_channel; i++)
    (*mwmr1->p_write_fifo[i])(*copro1->p_write_fifo[i]);


  for(uint32_t i=0; i<n_config; i++)
    (*mwmr1->p_config[i])(*copro1->p_config[i]);

  for(uint32_t i=0; i<n_status; i++)
    (*mwmr1->p_status[i])(*copro1->p_status[i]);

  /////////////////////////////////////////////////////////////////////////////
  // SIMULATION
  /////////////////////////////////////////////////////////////////////////////
  ftime(&initial);

  sc_core::sc_start();  // start the simulation

  ftime(&final);

  std::cout << "Execution Time = " << (int)((1000.0 * (final.time - initial.time))+ (final.millitm - initial.millitm)) << std::endl;

  return 0;
}
