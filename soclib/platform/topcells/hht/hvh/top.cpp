
#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include "hht_initiator_from_text.h"
#include "vci_cori_config_initiator.h"
#include "vci_ciro_config_initiator.h"
#include "vci_hht_cori_bridge.h"
#include "vci_hht_ciro_bridge.h"
#include "hht_testelg_target.h"
int _main(int argc, char *argv[])
{
	using namespace sc_core;
	
	// Define VCI & HHT parameters
	typedef soclib::caba::VciParams<4,7,40,2,1,1,14,1,1,1> vci_param;
	typedef soclib::caba::HhtParam<64,32> hht_param;
	
	// Signals
	printf("Initializing signals\n");
	sc_clock		signal_clk("signal_clk");
	sc_signal<bool> signal_resetn("signal_resetn");
	// Signal connecting ciro to cori
	soclib::caba::VciSignals<vci_param> s_vci_io("s_vci_io");
	// Signals connecting the config initiators
	soclib::caba::VciSignals<vci_param> s_vci_cori_config("s_vci_cori_config");
	soclib::caba::VciSignals<vci_param> s_vci_ciro_config("s_vci_ciro_config");
	// Signals connecting ciro to initiator
	soclib::caba::FifoSignals<hht_param::ctrl_t> s_ctrlPCI("s_ctrlPCI");
	soclib::caba::FifoSignals<hht_param::ctrl_t> s_ctrlNPCI("s_ctrlNPCI");
	soclib::caba::FifoSignals<hht_param::data_t> s_dataPCI("s_dataPCI");
	soclib::caba::FifoSignals<hht_param::data_t> s_dataNPCI("s_dataNPCI");
	soclib::caba::FifoSignals<hht_param::ctrl_t> s_ctrlRO("s_ctrlRO");
	soclib::caba::FifoSignals<hht_param::data_t> s_dataRO("s_dataRO");
	// Signals connecting cori to target
	soclib::caba::FifoSignals<hht_param::ctrl_t> s_ctrlPCO("s_ctrlPCO");
	soclib::caba::FifoSignals<hht_param::ctrl_t> s_ctrlNPCO("s_ctrlNPCO");
	soclib::caba::FifoSignals<hht_param::data_t> s_dataPCO("s_dataPCO");
	soclib::caba::FifoSignals<hht_param::data_t> s_dataNPCO("s_dataNPCO");
	soclib::caba::FifoSignals<hht_param::ctrl_t> s_ctrlRI("s_ctrlRI");
	soclib::caba::FifoSignals<hht_param::data_t> s_dataRI("s_dataRI");
	
	
	// Components
	printf("Initializing components\n");
	soclib::caba::HhtInitiatorFromText<hht_param> 		hht_init("hht_init");
	soclib::caba::VciCoriConfigInitiator<vci_param> 	vci_cori_config("vci_cori_config");
	soclib::caba::VciCiroConfigInitiator<vci_param> 	vci_ciro_config("vci_ciro_config");
	soclib::caba::VciHhtCoriBridge<vci_param, hht_param> 	cori_bridge("cori_bridge");
	soclib::caba::VciHhtCiroBridge<vci_param, hht_param> 	ciro_bridge("ciro_bridge");
	soclib::caba::HhtTestelgTarget<hht_param> 					hht_targ("hht_targ");
	
	//	Net-List
	printf("Initializing net-list\n");
	// All common signals
	hht_init.p_clk(signal_clk);
	hht_init.p_resetn(signal_resetn);
	hht_targ.p_clk(signal_clk);
	hht_targ.p_resetn(signal_resetn);
	vci_cori_config.p_clk(signal_clk);
	vci_cori_config.p_resetn(signal_resetn);
	vci_ciro_config.p_clk(signal_clk);
	vci_ciro_config.p_resetn(signal_resetn);
	cori_bridge.p_clk(signal_clk);
	cori_bridge.p_resetn(signal_resetn);
	ciro_bridge.p_clk(signal_clk);
	ciro_bridge.p_resetn(signal_resetn);
	
	// Linking components together
	vci_cori_config.p_vci(s_vci_cori_config);
	vci_ciro_config.p_vci(s_vci_ciro_config);
	
	cori_bridge.p_vci_io(s_vci_io);
	cori_bridge.p_vci_config(s_vci_cori_config);
	ciro_bridge.p_vci_io(s_vci_io);
	ciro_bridge.p_vci_config(s_vci_ciro_config);
	
	cori_bridge.p_ctrlPCO(s_ctrlPCO);
	cori_bridge.p_ctrlNPCO(s_ctrlNPCO);
	cori_bridge.p_dataPCO(s_dataPCO);
	cori_bridge.p_dataNPCO(s_dataNPCO);
	cori_bridge.p_ctrlRI(s_ctrlRI);
	cori_bridge.p_dataRI(s_dataRI);
	hht_targ.p_ctrlPCO(s_ctrlPCO);
	hht_targ.p_ctrlNPCO(s_ctrlNPCO);
	hht_targ.p_dataPCO(s_dataPCO);
	hht_targ.p_dataNPCO(s_dataNPCO);
	hht_targ.p_ctrlRI(s_ctrlRI);
	hht_targ.p_dataRI(s_dataRI);
	
	ciro_bridge.p_ctrlPCI(s_ctrlPCI);
	ciro_bridge.p_ctrlNPCI(s_ctrlNPCI);
	ciro_bridge.p_dataPCI(s_dataPCI);
	ciro_bridge.p_dataNPCI(s_dataNPCI);
	ciro_bridge.p_ctrlRO(s_ctrlRO);
	ciro_bridge.p_dataRO(s_dataRO);
	hht_init.p_ctrlPCI(s_ctrlPCI);
	hht_init.p_ctrlNPCI(s_ctrlNPCI);
	hht_init.p_dataPCI(s_dataPCI);
	hht_init.p_dataNPCI(s_dataNPCI);
	hht_init.p_ctrlRO(s_ctrlRO);
	hht_init.p_dataRO(s_dataRO);
	
	// Simulation
	int ncycles;
	if (argc == 2) {
		ncycles = std::atoi(argv[1]);
	} else {
		std::cerr
			<< std::endl << "The number of simulation cycles must be defined in the command line" << std::endl;
		exit(1);
	}

	printf("Starting simulation\n");
	sc_start(sc_core::sc_time(0, SC_NS));
	signal_resetn = false;
	sc_start(sc_core::sc_time(1, SC_NS));
	signal_resetn = true;
	
	if (hht_init.start_send("/root/Desktop/Recherche/SystemC/hht/platforms/hvh/hht_cmd.txt",false))
		printf("Opening HHT commands file\n");
	else
		printf("Error: HHT commands file missing\n");
		
	for (int i = 0; i < ncycles ; i+=1)
	{
		sc_start(sc_core::sc_time(1, SC_NS));
	}		
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
