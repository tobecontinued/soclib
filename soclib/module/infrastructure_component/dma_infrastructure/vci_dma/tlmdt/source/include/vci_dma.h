/* -*- c++ -*-
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
 * Copyright (c) UPMC, Lip6, Asim
 *         Alain Greiner  <alain.greiner@lip6.fr> 2011
 *
 * Maintainers: alain
 */

/////////////////////////////////////////////////////////////////////////////
// Implementation Note
// This component is both a VCI initiator and a VCI target, but
// it has only one PDES thread, and one local time.
// The component can be in six states:
// - IDLE        : not running : waiting a VCI command on the target port
// - READ        : running     : performing a VCI read transaction
// - WRITE       : running     : performing a VCI write transaction
// - SUCCESS     : not running : waiting a VCI command on the target port
// - ERROR_READ  : not running : waiting a VCI command on the target port
// - ERROR_WRITE : not running : waiting a VCI command on the target port
//
// When the DMA is in a running, state the local time is not updated by a
// received command on the target port, and the software visible registers 
// are not written. As it makes continuously read/write transactions, 
// there is no need to send null messages.,
// When the DMA is not in a running state, it is frozen) waiting a VCI
// command on the target port, and should not be taken into account
// in the time arbitration.
//////////////////////////////////////////////////////////////////////////////

#ifndef SOCLIB_VCI_DMA_H
#define SOCLIB_VCI_DMA_H

#include <stdint.h>
#include <systemc>
#include <tlmdt>
#include "mapping_table.h"

namespace soclib {
namespace tlmdt {

using namespace sc_core;

template<typename vci_param>
class VciDma
  : public sc_core::sc_module
  , virtual public tlm::tlm_bw_transport_if<tlm::tlm_base_protocol_types> 
  , virtual public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types> 
{
	
    typedef typename vci_param::addr_t addr_t;
    typedef typename vci_param::data_t data_t;

private:

    // Member Variables
    uint32_t 			m_srcid;		// DMA SRCID
    uint32_t 			m_max_burst_length;	// local buffer length (bytes)

    int				m_state;		// DMA state
    data_t 			m_source;		// source buffer pointer
    data_t 			m_destination;		// destination buffer pointer
    data_t 			m_length;		// tranfer length (bytes)
    bool     			m_irq_disabled;		// no IRQ when true
    bool     			m_stop;			// DMA running when false

    unsigned char*		m_vci_data_buf;		// pointer on the local data buffer for VCI (N bytes)
    unsigned char*		m_vci_be_buf;		// pointer on the local be buffer for VCI (N bytes)
    unsigned char*		m_irq_data_buf;		// pointer on the local data buffer for IRQ (1 byte)

    soclib::common::Segment 	m_segment;		// segment associated
    pdes_local_time*		m_pdes_local_time;	// local time
    sc_core::sc_event         	m_rsp_received;		// event to wake-up the DMA

    // VCI READ/WRITE TRANSACTION
    tlm::tlm_generic_payload	m_vci_payload;
    tlm::tlm_phase		m_vci_phase;
    sc_core::sc_time		m_vci_time;
    soclib_payload_extension	m_vci_extension;
    
    // NULL MESSAGE
    tlm::tlm_generic_payload	m_null_payload;
    tlm::tlm_phase		m_null_phase;
    sc_core::sc_time		m_null_time;
    soclib_payload_extension	m_null_extension;
    
    // IRQ TRANSACTION
    tlm::tlm_generic_payload  	m_irq_payload;
    tlm::tlm_phase		m_irq_phase;
    sc_core::sc_time		m_irq_time;

public:

    enum iniit_state_e {
        STATE_IDLE,
        STATE_SUCCESS,
        STATE_ERROR_READ,
        STATE_ERROR_WRITE,
        STATE_READ,
        STATE_WRITE,
    };

    // Functions
    void execLoop();
    bool vci_rsp_error();
    void set_interrupt();
    void reset_interrupt();
    void send_null();
    void send_write(size_t burst_length);
    void send_read(size_t burst_length);

    // Virtual Fuction to receive response on the VCI initiator port
    tlm::tlm_sync_enum 	nb_transport_bw ( tlm::tlm_generic_payload   &payload,
                                          tlm::tlm_phase             &phase,  
                                          sc_core::sc_time           &time);   
    
    // Virtual Fuction to receive response on the IRQ port
    tlm::tlm_sync_enum 	irq_nb_transport_bw ( tlm::tlm_generic_payload  &payload, 
                                              tlm::tlm_phase            &phase,  
                                              sc_core::sc_time          &time);  

    // Virtual Fuction to receive command on the VCI target port
    tlm::tlm_sync_enum 	nb_transport_fw ( tlm::tlm_generic_payload &payload,  
                                          tlm::tlm_phase           &phase,      
                                          sc_core::sc_time         &time);        
    
    // Not implemented 
    void b_transport ( tlm::tlm_generic_payload &payload, 
                       sc_core::sc_time         &time); 
    
    bool get_direct_mem_ptr ( tlm::tlm_generic_payload &payload,  
                              tlm::tlm_dmi             &dmi_data);  
    
    unsigned int transport_dbg ( tlm::tlm_generic_payload &payload); 
    
    void invalidate_direct_mem_ptr ( sc_dt::uint64 start_range,  
                                     sc_dt::uint64 end_range); 
protected:

    SC_HAS_PROCESS(VciDma);
    
public:

    // ports
    tlm::tlm_initiator_socket<32, tlm::tlm_base_protocol_types>                  p_vci_initiator; 
    tlm::tlm_target_socket<32,tlm::tlm_base_protocol_types> 			 p_vci_target;
    tlm_utils::simple_initiator_socket<VciDma, 32, tlm::tlm_base_protocol_types> p_irq; 

    VciDma( sc_module_name name,
            const soclib::common::MappingTable &mt,
            const soclib::common::IntTab &srcid,
            const soclib::common::IntTab &tgtid,
            const size_t max_burst_length );
};

}}

#endif /* SOCLIB_VCI_DMA_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

