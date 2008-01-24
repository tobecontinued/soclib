/////////////////////////////////////////////////////////////////////
// File     : ring_register.h
// Author   : Yang GAO 
// Date     : 28/09/2007
// Copyright: UPMC - LIP6
// This program is released under the GNU Public License 
//
// This component is a register for generic VCI anneau-ani. 
/////////////////////////////////////////////////////////////////////

#ifndef VCI_RING_REGISTER_H_
#define VCI_RING_REGISTER_H_

#include <systemc.h>
#include "caba/util/base_module.h"
#include "caba/interface/ring_ports.h"

namespace soclib { namespace caba {

template<typename vci_param>
class RingRegister 
	: public soclib::caba::BaseModule
{

protected:
	SC_HAS_PROCESS(RingRegister);

public:
	// ports
	sc_in<bool> 				  p_clk;
	sc_in<bool>   				  p_resetn;
	soclib::caba::RingNegINPort<vci_param> 	  p_rni;
	soclib::caba::RingDataINPort<vci_param>	  p_rdi;
	soclib::caba::RingNegOUTPort<vci_param>	  p_rno;
	soclib::caba::RingDataOUTPort<vci_param>  p_rdo;
	
	// constructor / destructor
	RingRegister(sc_module_name	insname);

private:
	// methods
	void transition();
};

}} // end namespace
		
#endif // VCI_RING_REGISTER_H_
