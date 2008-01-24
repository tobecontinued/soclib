/*
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
 * Author   : Yang GAO 
 * Date     : 28/09/2007
 * Copyright: UPMC - LIP6
 */

#ifndef SOCLIB_CABA_RING_SIGNALS_H_
#define SOCLIB_CABA_RING_SIGNALS_H_

#include "caba/interface/vci_param.h"

namespace soclib { namespace caba {

enum{
    NEG_EMPTY	   = 0,  //empty
    NEG_REQ_READ   = 1,  //read request
    NEG_REQ_WRITE  = 2,  //write request
    NEG_ACK	   = 3   //ack
};

// ANNEAU_DATA STATES
enum{
    DATA_EMPTY     = 0,  //empty
    DATA_REQ_READ  = 1,  //read request
    DATA_REQ_WRITE = 2,  //write request 
    DATA_RES	   = 3   //answer
};

enum{
    RING           = 0,   //request
    LOCAL          = 1    //empty
};

template <typename vci_param>
class RingNegSignals
{
public:
	// signals
	sc_signal<sc_uint<2> >     	   	ring_neg_cmd;        
	sc_signal<typename vci_param::srcid_t>  ring_neg_srcid;   
	sc_signal<typename vci_param::srcid_t>  ring_neg_msblsb;
	
#define ren(x) x(((std::string)(name_ + "_"#x)).c_str())
	RingNegSignals(std::string name_ = (std::string)sc_gen_unique_name("ring_neg_signals"))
	  : ren(ring_neg_cmd),
	  ren(ring_neg_srcid),
	  ren(ring_neg_msblsb)
	  {
	  }
#undef ren
};

template <typename vci_param>
class RingDataSignals
{
public:
	// signals
	sc_signal<sc_uint<2> >              	ring_data_cmd;    
	sc_signal<typename vci_param::eop_t>    ring_data_eop;   
	sc_signal<typename vci_param::be_t>     ring_data_be;    
	sc_signal<typename vci_param::srcid_t>  ring_data_srcid;
	sc_signal<typename vci_param::pktid_t>  ring_data_pktid;
	sc_signal<typename vci_param::addr_t>   ring_data_adresse;
	sc_signal<typename vci_param::data_t>   ring_data;
	sc_signal<typename vci_param::rerror_t> ring_data_error;

#define ren(x) x(((std::string)(name_ + "_"#x)).c_str())
	RingDataSignals(std::string name_ = (std::string)sc_gen_unique_name("ring_data_signals"))
	  : ren(ring_data_cmd),
	  ren(ring_data_eop),
	  ren(ring_data_be),
	  ren(ring_data_srcid),
	  ren(ring_data_pktid),
	  ren(ring_data_adresse),
	  ren(ring_data),
	  ren(ring_data_error)
	  {
	  }
#undef ren
};

}} // end namespace

#endif /* SOCLIB_CABA_RING_SIGNALS_H_ */
