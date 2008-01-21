#include <iostream>
#include <cstdlib>

#include "common/exception.h"
#include "common/topcell/topcell.h"

int sc_main (int argc, char *argv[])
{
	try {
		if ( argc < 2 )
			throw soclib::exception::RunTimeError("Usage: simulator netlist");
		soclib::common::TopCell tc(argv[1]);
		if ( argc >= 3 )
			tc.run(strtoll(argv[2], 0, 0));
		else
			tc.run();
		return 0;
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	} catch (...) {
		std::cout << "Unknown exception occured" << std::endl;
		throw;
	}
	return 1;
}
