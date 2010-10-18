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
 *         Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>, 2010
 *
 * Maintainers: alinev
 */
#ifndef SOCLIB_VCI_BLOCK_DEVICE_H
#define SOCLIB_VCI_BLOCK_DEVICE_H

#include <stdint.h>
#include <systemc>
#include <tlmdt>
#include "mapping_table.h"

namespace soclib {
namespace tlmdt {

using namespace sc_core;

template<typename vci_param>
class VciBlockDevice
  : public sc_core::sc_module
  , virtual public tlm::tlm_bw_transport_if<tlm::tlm_base_protocol_types> // inherit from TLM "backward interface"
  , virtual public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types> // inherit from TLM "forward interface"
{
private:
	
    typedef typename vci_param::addr_t addr_t;
    typedef typename vci_param::data_t data_t;
    typedef typename vci_param::data_t tag_t;
    typedef typename vci_param::data_t be_t;

    /////////////////////////////////////////////////////////////////////////////////////
    // Member Variables
    /////////////////////////////////////////////////////////////////////////////////////
    const uint32_t m_block_size;
	const uint32_t m_latency;

    uint32_t m_id;
	int      m_fd;
	int      m_op;
	uint32_t m_buffer;
	uint32_t m_count;
	uint64_t m_device_size;
	uint32_t m_lba;
    int      m_status;
    uint32_t m_chunck_offset;
    uint32_t m_transfer_size;
    uint32_t m_access_latency;
    bool     m_irq_changed;
	bool     m_irq_enabled;
	bool     m_irq;

    uint32_t m_lfsr;

	int      m_current_op;

	uint8_t *m_data;

    std::list<soclib::common::Segment>  segList;

    pdes_local_time          *m_pdes_local_time;

    //VCI COMMUNICATION
    unsigned int              m_nbytes;
    addr_t                    m_address;
    unsigned char             m_byte_enable_ptr[MAXIMUM_PACKET_SIZE * vci_param::nbytes];
    unsigned char             m_data_ptr[MAXIMUM_PACKET_SIZE * vci_param::nbytes];
    sc_core::sc_event         m_rsp_received;

    //FIELDS OF A NORMAL MESSAGE
    tlm::tlm_generic_payload *m_payload_ptr;
    soclib_payload_extension *m_extension_ptr;
    tlm::tlm_phase            m_phase;
    sc_core::sc_time          m_time;
    
    //FIELDS OF A NULL MESSAGE
    tlm::tlm_generic_payload *m_null_payload_ptr;
    soclib_payload_extension *m_null_extension_ptr;
    tlm::tlm_phase            m_null_phase;
    sc_core::sc_time          m_null_time;
    
    //FIELDS OF AN IRQ TRANSACTION
    tlm::tlm_generic_payload  m_irq_payload;
    tlm::tlm_phase            m_irq_phase;
    sc_core::sc_time          m_irq_time;

    /////////////////////////////////////////////////////////////////////////////////////
    // Fuctions
    /////////////////////////////////////////////////////////////////////////////////////
    void execLoop();
    void update_time(sc_core::sc_time t);
    void send_interruption();
    void send_null_message();
    void send_write_message(size_t chunck_size);
    void send_read_message(size_t chunck_size);
    void ended(int status);
    void write_finish();
    void read_done();
    void next_req();

    /////////////////////////////////////////////////////////////////////////////////////
    // Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
    /////////////////////////////////////////////////////////////////////////////////////
    tlm::tlm_sync_enum nb_transport_bw         // receive rsp from target
    ( tlm::tlm_generic_payload   &payload,     // payload
      tlm::tlm_phase             &phase,       // phase
      sc_core::sc_time           &time);       // time
    
    void invalidate_direct_mem_ptr             // invalidate_direct_mem_ptr
    ( sc_dt::uint64 start_range,               // start range
      sc_dt::uint64 end_range);                // end range
    
    
    /////////////////////////////////////////////////////////////////////////////////////
    // Virtual Fuctions  tlm::tlm_fw_transport_if  (VCI TARGET SOCKET)
    /////////////////////////////////////////////////////////////////////////////////////
    tlm::tlm_sync_enum nb_transport_fw       // receive command from initiator
    ( tlm::tlm_generic_payload &payload,      // payload
      tlm::tlm_phase           &phase,        // phase
      sc_core::sc_time         &time);        // time
    
    // Not implemented for this example but required by interface
    void b_transport                          // b_transport() - Blocking Transport
    ( tlm::tlm_generic_payload &payload,      // payload
      sc_core::sc_time         &time);        // time
    
    // Not implemented for this example but required by interface
    bool get_direct_mem_ptr
    ( tlm::tlm_generic_payload &payload,      // payload
      tlm::tlm_dmi             &dmi_data);    // DMI data
    
    // Not implemented for this example but required by interface
    unsigned int transport_dbg                            
    ( tlm::tlm_generic_payload &payload);     // payload
    
    /////////////////////////////////////////////////////////////////////////////////////
    // Virtual Fuctions  tlm::tlm_bw_transport_if (IRQ INITIATOR SOCKET)
    /////////////////////////////////////////////////////////////////////////////////////
    tlm::tlm_sync_enum irq_nb_transport_bw    
    ( tlm::tlm_generic_payload           &payload,       // payload
      tlm::tlm_phase                     &phase,         // phase
      sc_core::sc_time                   &time);         // time

protected:
    SC_HAS_PROCESS(VciBlockDevice);
    
public:
    tlm::tlm_initiator_socket<32, tlm::tlm_base_protocol_types> p_vci_initiator; // VCI INITIATOR socket
    tlm::tlm_target_socket<32,tlm::tlm_base_protocol_types> p_vci_target;        // VCI TARGET socket
    tlm_utils::simple_initiator_socket<VciBlockDevice, 32, tlm::tlm_base_protocol_types> p_irq;           // IRQ INITIATOR socket

	VciBlockDevice(
		sc_module_name name,
		const soclib::common::MappingTable &mt,
		const soclib::common::IntTab &srcid,
		const soclib::common::IntTab &tgtid,
        const std::string &filename,
        const uint32_t block_size = 512,
        const uint32_t latency = 0);
};

}}

#endif /* SOCLIB_VCI_BLOCK_DEVICE_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

