#include <iostream>
#include <systemc>
#include <tlmt>
#include "vci_ports.h"
#include "mapping_table.h"
#include "iss.h"
#include "mips.h"
#include "vci_xcache_wrapper.h"
#include "vci_vgmn.h"
#include "vci_ram.h"
#include "tty.h"
#include "vci_multi_tty.h"

#include <sys/timeb.h>

int sc_main(int argc, char **argv)
{
  struct timeb initial, final;

  ftime(&initial);

  typedef soclib::tlmt::VciParams<uint32_t,uint32_t,4> vci_param;


  // Configurator instanciateOnStack
  soclib::common::ElfLoader loader("soft/bin.soft");
  soclib::common::MappingTable mapping_table(32, soclib::common::IntTab(8), soclib::common::IntTab(8), 0x00200000);
  
  // Configurator configure
  mapping_table.add(soclib::common::Segment("boot",  0xbfc00000,       2048, soclib::common::IntTab(1), 1));
  mapping_table.add(soclib::common::Segment("cram0", 0x10000000, 0x00100000, soclib::common::IntTab(0), 1));
  mapping_table.add(soclib::common::Segment("cram1", 0x20000000, 0x00100000, soclib::common::IntTab(1), 1));
  mapping_table.add(soclib::common::Segment("excep", 0x80000080,       2048, soclib::common::IntTab(1), 1));
  mapping_table.add(soclib::common::Segment("tty0",  0x90200000,         32, soclib::common::IntTab(2), 0));
  mapping_table.add(soclib::common::Segment("uram0", 0x10200000, 0x00100000, soclib::common::IntTab(0), 0));
  mapping_table.add(soclib::common::Segment("uram1", 0x20200000, 0x00100000, soclib::common::IntTab(1), 0));
  
  /////////////////////////////////////////////////////////////////////////////
  // VCI_VGMN 
  /////////////////////////////////////////////////////////////////////////////
  soclib::tlmt::VciVgmn<vci_param> vgmn(1,3,mapping_table,3);

  /////////////////////////////////////////////////////////////////////////////
  // VCI_XCACHE 
  /////////////////////////////////////////////////////////////////////////////
  soclib::tlmt::VciXcacheWrapper<soclib::common::MipsElIss,vci_param> mips0("mips0", 0, mapping_table, 64, 8, 1, 8);

  mips0.p_vci(vgmn.m_RspArbCmdRout[0]->p_vci);

  /////////////////////////////////////////////////////////////////////////////
  // RAM
  /////////////////////////////////////////////////////////////////////////////
  soclib::tlmt::VciRam<vci_param> ram0("ram0", 0, soclib::common::IntTab(0), mapping_table, loader);

  vgmn.m_CmdArbRspRout[0]->p_vci(ram0.p_vci);

  /////////////////////////////////////////////////////////////////////////////
  // RAM
  /////////////////////////////////////////////////////////////////////////////
  soclib::tlmt::VciRam<vci_param> ram1("ram1", 1, soclib::common::IntTab(1), mapping_table, loader);

  vgmn.m_CmdArbRspRout[1]->p_vci(ram1.p_vci);

  /////////////////////////////////////////////////////////////////////////////
  // TTY
  /////////////////////////////////////////////////////////////////////////////
  soclib::tlmt::VciMultiTty<vci_param> vcitty("tty0",soclib::common::IntTab(2), mapping_table, "TTY0", NULL);

  vgmn.m_CmdArbRspRout[2]->p_vci(vcitty.p_vci);
  (*vcitty.p_irq[0])(mips0.p_irq[0]);
 

  /////////////////////////////////////////////////////////////////////////////
  // START
  /////////////////////////////////////////////////////////////////////////////
  sc_core::sc_start();

  ftime(&final);

  std::cout << "Execution Time = " << (int)((1000.0 * (final.time - initial.time))+ (final.millitm - initial.millitm)) << std::endl;

  return 0;
}

