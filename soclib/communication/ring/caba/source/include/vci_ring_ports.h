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
 * Author   : Franck WAJSBÜRT, Yang GAO 
 * Date     : 28/09/2007
 * Copyright: UPMC - LIP6
 */

#ifndef SOCLIB_CABA_SIGNAL_RING_PORTS_H_
#define SOCLIB_CABA_SIGNAL_RING_PORTS_H_

#include "vci_ring_signals.h"

namespace soclib { namespace caba {

/*
 * Ring Wrappor in port
 */
template <typename vci_param>
class RingINPort
{
public:
	sc_in<sc_uint<3> >     	            ring_neg_cmd;     
	sc_in<typename vci_param::srcid_t>  ring_neg_srcid;   
	sc_in<typename vci_param::srcid_t>  ring_neg_msblsb;
	sc_in<sc_uint<3> >                  ring_data_cmd;    
	sc_in<typename vci_param::eop_t>    ring_data_eop;   
	sc_in<typename vci_param::be_t>     ring_data_be;    
	sc_in<typename vci_param::srcid_t>  ring_data_srcid;
	sc_in<typename vci_param::pktid_t>  ring_data_pktid;
	sc_in<typename vci_param::addr_t>   ring_data_adresse;
	sc_in<typename vci_param::data_t>   ring_data;
	sc_in<typename vci_param::rerror_t> ring_data_error;

    RingINPort(const std::string &name = sc_gen_unique_name("ring_in"))
		: ring_neg_cmd    	((name+"neg_cmd").c_str()),
		  ring_neg_srcid   	((name+"neg_srcid").c_str()),
		  ring_neg_msblsb  	((name+"neg_msblsb").c_str()),
		  ring_data_cmd    	((name+"data_cmd").c_str()),
		  ring_data_eop    	((name+"data_eop").c_str()),
		  ring_data_be  	((name+"data_be").c_str()),
		  ring_data_srcid  	((name+"data_srcid").c_str()),
		  ring_data_pktid  	((name+"data_pktid").c_str()),
		  ring_data_adresse ((name+"data_adresse").c_str()),
		  ring_data  		((name+"data_data").c_str()),
		  ring_data_error   ((name+"data_error").c_str())
	{
	}
    
	void operator () (RingSignals<vci_param> &sig)
	{
		ring_neg_cmd		(sig.ring_neg_cmd);			
		ring_neg_srcid   	(sig.ring_neg_srcid);		
		ring_neg_msblsb  	(sig.ring_neg_msblsb);	
		ring_data_cmd    	(sig.ring_data_cmd);		
		ring_data_eop    	(sig.ring_data_eop);		
		ring_data_be  		(sig.ring_data_be);	
		ring_data_srcid  	(sig.ring_data_srcid);	
		ring_data_pktid  	(sig.ring_data_pktid);			
		ring_data_adresse  	(sig.ring_data_adresse);			
		ring_data  		    (sig.ring_data);		
		ring_data_error   	(sig.ring_data_error);	
	}
};

/*
 * Ring Wrappor out port
 */
template <typename vci_param>
class RingOUTPort
{
public:
	sc_out<sc_uint<3> >     	         ring_neg_cmd;     
	sc_out<typename vci_param::srcid_t>  ring_neg_srcid;   
	sc_out<typename vci_param::srcid_t>  ring_neg_msblsb;
	sc_out<sc_uint<3> >                  ring_data_cmd;    
	sc_out<typename vci_param::eop_t>    ring_data_eop;   
	sc_out<typename vci_param::be_t>     ring_data_be;    
	sc_out<typename vci_param::srcid_t>  ring_data_srcid;
	sc_out<typename vci_param::pktid_t>  ring_data_pktid;
	sc_out<typename vci_param::addr_t>   ring_data_adresse;
	sc_out<typename vci_param::data_t>   ring_data;
	sc_out<typename vci_param::rerror_t> ring_data_error;
	
    RingOUTPort(const std::string &name = sc_gen_unique_name("ring_out"))
		: ring_neg_cmd    	((name+"neg_cmd").c_str()),
		  ring_neg_srcid   	((name+"neg_srcid").c_str()),
		  ring_neg_msblsb  	((name+"neg_msblsb").c_str()),
		  ring_data_cmd    	((name+"data_cmd").c_str()),
		  ring_data_eop    	((name+"data_eop").c_str()),
		  ring_data_be  	((name+"data_be").c_str()),
		  ring_data_srcid  	((name+"data_srcid").c_str()),
		  ring_data_pktid  	((name+"data_pktid").c_str()),
		  ring_data_adresse ((name+"data_adresse").c_str()),
		  ring_data  		((name+"data_data").c_str()),
		  ring_data_error   ((name+"data_error").c_str())
	{
	}
    
	void operator () (RingSignals<vci_param> &sig)
	{
		ring_neg_cmd		(sig.ring_neg_cmd);			
		ring_neg_srcid   	(sig.ring_neg_srcid);		
		ring_neg_msblsb  	(sig.ring_neg_msblsb);
		ring_data_cmd    	(sig.ring_data_cmd);		
		ring_data_eop    	(sig.ring_data_eop);		
		ring_data_be  		(sig.ring_data_be);	
		ring_data_srcid  	(sig.ring_data_srcid);	
		ring_data_pktid  	(sig.ring_data_pktid);			
		ring_data_adresse  	(sig.ring_data_adresse);			
		ring_data  		    (sig.ring_data);		
		ring_data_error   	(sig.ring_data_error);		
	}
};

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

