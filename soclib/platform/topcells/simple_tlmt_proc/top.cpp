#include <iostream>
#include <systemc>
#include <tlmt>
#include "vci_ports.h"
#include "mapping_table.h"
#include "vci_vgmn.h"
#include "vci_iss_xcache.h"
#include "vci_simple_target.h"
#include "vci_multi_tty.h"
#include "vci_ram.h"
#include "segmentation.h"
#include "iss/mips.h"

int sc_main(int argc, char **argv)
{
	typedef soclib::tlmt::VciParams<uint32_t,uint32_t> vci_param;

	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;

	// Mapping table

	soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00300000);

	maptab.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(0), true));
	maptab.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(0), true));
	maptab.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), true));

	maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(0), true));

	maptab.add(Segment("loc0" , LOC0_BASE , LOC0_SIZE , IntTab(0), true));

	maptab.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(1), false));

	soclib::tlmt::VciVgmn<vci_param> vgmn(1,2,maptab,10);

	soclib::tlmt::VciIssXcache<soclib::common::MipsElIss,vci_param> i("init",0);
	//soclib::tlmt::VciSimpleTarget<vci_param> t("target");

	soclib::common::ElfLoader loader("soft/bin.soft");
	soclib::tlmt::VciRam<vci_param> ram("ram", IntTab(0), maptab, loader);
	soclib::tlmt::VciMultiTty<vci_param> tty("tty", IntTab(1), maptab, "tty0", NULL);

	std::cout << "ram initialisee" << std::endl;

	//i.p_vci(t.p_vci);
	i.p_vci(vgmn.m_RspArbCmdRout[0]->p_vci);
	vgmn.m_CmdArbRspRout[0]->p_vci(ram.p_vci);
	vgmn.m_CmdArbRspRout[1]->p_vci(tty.p_vci);

	tty.p_out(i.p_irq[2]);

	sc_core::sc_start();
	return 0;
}

