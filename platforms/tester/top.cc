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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#include <iostream>
#include <cstdlib>

#include "common/mapping_table.h"
#include "common/iss/mips.h"
#include "common/iss/microblaze.h"
#include "common/iss/ppc405.h"
#include "caba/processor/iss_wrapper.h"
#include "caba/initiator/vci_xcache.h"
#include "caba/target/vci_multi_ram.h"
#include "caba/target/vci_multi_tty.h"
#include "caba/target/vci_simhelper.h"
#include "caba/interconnect/vci_vgmn.h"

#include "segmentation.h"

typedef enum {
	MIPSEL,
	POWERPC,
    MICROBLAZE,
} arch_t;

int _main(int argc, char *argv[])
{
	using namespace sc_core;
	// Avoid repeating these everywhere
	using soclib::common::IntTab;
	using soclib::common::Segment;

	// Define our VCI parameters
	typedef soclib::caba::VciParams<4,1,32,1,1,1,8,1,1,1> vci_param;

	::setenv("SOCLIB_TTY", "TERM", 1);

	if ( argc < 3 )
		throw soclib::exception::RunTimeError("Must pass architecture and binary image on command line !");

	std::string arch_string = argv[1];
	arch_t arch;
	if ( arch_string == "mipsel" )
		arch = MIPSEL;
	else if ( arch_string == "powerpc" )
		arch = POWERPC;
	else if ( arch_string == "microblaze" )
		arch = MICROBLAZE;
	else
		throw soclib::exception::RunTimeError("Incorrect architecture: "+arch_string);

	// Mapping table

	soclib::common::MappingTable maptab(32, IntTab(8), IntTab(8), 0x00300000);

	switch ( arch ) {
	case MIPSEL:
		maptab.add(Segment("reset", RESET_BASE, RESET_SIZE, IntTab(0), true));
		maptab.add(Segment("excep", EXCEP_BASE, EXCEP_SIZE, IntTab(0), true));
		break;
	case POWERPC:
		maptab.add(Segment("ppc_boot", PPC_BOOT_BASE, PPC_BOOT_SIZE, IntTab(0), false));
		maptab.add(Segment("ppc_special" , PPC_SPECIAL_BASE , PPC_SPECIAL_SIZE , IntTab(0), true));
		break;
	case MICROBLAZE:
		maptab.add(Segment("mb_boot", MB_BOOT_BASE, MB_BOOT_SIZE, IntTab(0), false));
		break;
	}

	maptab.add(Segment("text" , TEXT_BASE , TEXT_SIZE , IntTab(0), true));

	maptab.add(Segment("data" , DATA_BASE , DATA_SIZE , IntTab(1), true));

	maptab.add(Segment("loc0" , LOC0_BASE , LOC0_SIZE , IntTab(1), true));

	maptab.add(Segment("unc"  , UNC_BASE  , UNC_SIZE  , IntTab(1), false));

	maptab.add(Segment("tty"  , TTY_BASE  , TTY_SIZE  , IntTab(2), false));

	maptab.add(Segment("simhelper", SIMHELPER_BASE  , SIMHELPER_SIZE  , IntTab(3), false));

	// Signals

	sc_clock		signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");

	soclib::caba::ICacheSignals signal_mips_icache0("signal_mips_icache0");
	soclib::caba::DCacheSignals signal_mips_dcache0("signal_mips_dcache0");
	sc_signal<bool> signal_cpu0_it0("signal_cpu0_it0");
	sc_signal<bool> signal_cpu0_it1("signal_cpu0_it1");
	sc_signal<bool> signal_cpu0_it2("signal_cpu0_it2");
	sc_signal<bool> signal_cpu0_it3("signal_cpu0_it3");
	sc_signal<bool> signal_cpu0_it4("signal_cpu0_it4");
	sc_signal<bool> signal_cpu0_it5("signal_cpu0_it5");

	soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");

	soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
	soclib::caba::VciSignals<vci_param> signal_vci_simhelper("signal_vci_simhelper");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
	soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram1("signal_vci_vcimultiram1");

	sc_signal<bool> signal_tty_irq0("signal_tty_irq0");

	// Components

	soclib::caba::VciXCache<vci_param> cache0("cache0", maptab,IntTab(0),16,8,16,8);

	soclib::caba::IssWrapper<soclib::common::MicroBlazeIss> *mb0;
	soclib::caba::IssWrapper<soclib::common::MipsIss> *mips0;
	soclib::caba::IssWrapper<soclib::common::Ppc405Iss> *ppc0;

	soclib::common::ElfLoader loader(argv[2]);
	soclib::caba::VciMultiRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
	soclib::caba::VciMultiRam<vci_param> vcimultiram1("vcimultiram1", IntTab(1), maptab, loader);
	soclib::caba::VciMultiTty<vci_param> vcitty("vcitty",	IntTab(2), maptab, "vcitty0", NULL);
	soclib::caba::VciSimhelper<vci_param> vcisimhelper("vcisimhelper",	IntTab(3), maptab);

	soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptab, 1, 4, 2, 8);

	//	Net-List

	cache0.p_clk(signal_clk);
	vcimultiram0.p_clk(signal_clk);
	vcimultiram1.p_clk(signal_clk);

	cache0.p_resetn(signal_resetn);
	vcimultiram0.p_resetn(signal_resetn);
	vcimultiram1.p_resetn(signal_resetn);

	switch ( arch ) {
	case MIPSEL:
		mips0 = new soclib::caba::IssWrapper<soclib::common::MipsIss>("mips0", 0);
		mips0->p_clk(signal_clk);
		mips0->p_resetn(signal_resetn);
		mips0->p_irq[0](signal_cpu0_it0);
		mips0->p_irq[1](signal_cpu0_it1);
		mips0->p_irq[2](signal_cpu0_it2);
		mips0->p_irq[3](signal_cpu0_it3);
		mips0->p_irq[4](signal_cpu0_it4);
		mips0->p_irq[5](signal_cpu0_it5);
		mips0->p_icache(signal_mips_icache0);
		mips0->p_dcache(signal_mips_dcache0);
		break;
	case POWERPC:
		ppc0 = new soclib::caba::IssWrapper<soclib::common::Ppc405Iss>("ppc0", 0);
		ppc0->p_clk(signal_clk);
		ppc0->p_resetn(signal_resetn);
		ppc0->p_irq[0](signal_cpu0_it0);
		ppc0->p_irq[1](signal_cpu0_it1);
		ppc0->p_icache(signal_mips_icache0);
		ppc0->p_dcache(signal_mips_dcache0);
		break;
	case MICROBLAZE:
		mb0 = new soclib::caba::IssWrapper<soclib::common::MicroBlazeIss>("mb0", 0);
		mb0->p_clk(signal_clk);
		mb0->p_resetn(signal_resetn);
		mb0->p_irq[0](signal_cpu0_it0);
		mb0->p_icache(signal_mips_icache0);
		mb0->p_dcache(signal_mips_dcache0);
		break;
	}

	cache0.p_icache(signal_mips_icache0);
	cache0.p_dcache(signal_mips_dcache0);
	cache0.p_vci(signal_vci_m0);

	vcimultiram0.p_vci(signal_vci_vcimultiram0);
	vcimultiram1.p_vci(signal_vci_vcimultiram1);

	vcitty.p_clk(signal_clk);
	vcitty.p_resetn(signal_resetn);
	vcitty.p_vci(signal_vci_tty);
	vcitty.p_irq[0](signal_tty_irq0);

	vcisimhelper.p_clk(signal_clk);
	vcisimhelper.p_resetn(signal_resetn);
	vcisimhelper.p_vci(signal_vci_simhelper);

	vgmn.p_clk(signal_clk);
	vgmn.p_resetn(signal_resetn);

	vgmn.p_from_initiator[0](signal_vci_m0);

	vgmn.p_to_target[0](signal_vci_vcimultiram0);
	vgmn.p_to_target[1](signal_vci_vcimultiram1);
	vgmn.p_to_target[2](signal_vci_tty);
	vgmn.p_to_target[3](signal_vci_simhelper);


	sc_start(sc_time(0, SC_FS));
	signal_resetn = false;

	sc_start(sc_time(1, SC_FS));
	signal_resetn = true;

	sc_start();

	return EXIT_SUCCESS;
}

int sc_main (int argc, char *argv[])
{
	int r = EXIT_FAILURE;
	try {
		r = _main(argc, argv);
	} catch (const std::exception &e) {
		std::cout << e.what() << std::endl;
	} catch (...) {
		std::cout << "Unknown exception occured" << std::endl;
	}
	return r;
}
