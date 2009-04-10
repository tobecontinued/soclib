/* -*- mode: c++; coding: utf-8 -*-
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
 * Maintainers: fpecheux, nipo, alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */
 
#include <tlmdt>                               // TLM-DT headers
#include <inttypes.h>

#include "soclib_endian.h"
#include "buffer.h"
#include "generic_cache.h"
#include "mapping_table.h"
#include "mips32.h"

namespace soclib { namespace tlmdt {

template<typename vci_param, typename iss_t>
class VciXcacheWrapper
  : public sc_core::sc_module             // inherit from SC module base clase
{
private:
  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  uint32_t                m_id;
  iss_t                   m_iss;
  uint32_t                m_irq;
  sc_core::sc_time        m_simulation_time;
  pdes_local_time        *m_pdes_local_time;
  pdes_activity_status   *m_pdes_activity_status;
  
  soclib::tlmt::genericCache<vci_param> m_dcache ;
  soclib::tlmt::genericCache<vci_param> m_icache ;
  soclib::common::AddressDecodingTable<typename vci_param::addr_t, bool> m_cacheability_table;
  
  //VCI COMMUNICATION
  unsigned int            m_nbytes;
  unsigned char           m_byte_enable_ptr[MAXIMUM_PACKET_SIZE * vci_param::nbytes];
  unsigned char           m_data_ptr[MAXIMUM_PACKET_SIZE * vci_param::nbytes];
  sc_core::sc_event       m_rsp_received;
  bool                    m_error;

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

  //FIELDS OF AN ACTIVITY STATUS MESSAGE
  tlm::tlm_generic_payload *m_activity_payload_ptr;
  soclib_payload_extension *m_activity_extension_ptr;
  tlm::tlm_phase            m_activity_phase;
  sc_core::sc_time          m_activity_time;
  
protected:
  SC_HAS_PROCESS(VciXcacheWrapper);
  
public:

  tlm_utils::simple_initiator_socket<VciXcacheWrapper, 32, tlm::tlm_base_protocol_types> p_vci_initiator;   // VCI initiator port 
  //std::vector<tlm_utils::simple_target_socket_tagged<VciXcache,32,soclib_irq_types> *> p_irq;  // IRQ target port
  
  VciXcacheWrapper(
		   sc_core::sc_module_name name,
		   int cpuid,
		   const soclib::common::IntTab &index,
		   const soclib::common::MappingTable &mt,
		   size_t icache_lines,
		   size_t icache_words,
		   size_t dcache_lines,
		   size_t dcache_words,
		   sc_core::sc_time time_quantum,          // time quantum
		   sc_core::sc_time simulation_time);
  
private:
  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum my_nb_transport_bw      // Receive rsp from target
  ( tlm::tlm_generic_payload   &payload,     // payload
    tlm::tlm_phase             &phase,       // phase
    sc_core::sc_time           &time);       // time

  
  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_fw_transport_if (IRQ TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  /*
  tlm::tlm_sync_enum my_nb_transport_fw              // receive interruption from initiator
  ( int                                id,           // interruption id
    soclib_irq_types::tlm_payload_type &payload,     // payload
    soclib_irq_types::tlm_phase_type   &phase,       // phase
    sc_core::sc_time                   &time);       // time
  */

  uint32_t xcacheAccess(
			struct iss_t::InstructionRequest ireq,
			struct iss_t::DataRequest dreq,
			struct iss_t::InstructionResponse &irsp,
			struct iss_t::DataResponse &drsp
			);
  
  void xcacheAccessInternal(
			    struct iss_t::InstructionRequest ireq,
			    struct iss_t::DataRequest dreq,
			    struct iss_t::InstructionResponse &irsp,
			    struct iss_t::DataResponse &drsp
			    ) const;
  
  void execLoop();
  
  uint32_t ram_write(
		     enum command command,
		     typename vci_param::addr_t address,
		     typename vci_param::data_t wdata, 
		     int be, 
		     typename vci_param::data_t &rdata, 
		     bool &rerror
		     );

  uint32_t ram_read(
		    enum command command, 
		    typename vci_param::addr_t address,
		    typename vci_param::data_t *rdata, 
		    bool &rerror,
		    size_t size = 1
		    );

  uint32_t fill_cache(
		      soclib::tlmt::genericCache<vci_param> &cache,
		      typename vci_param::addr_t address,
		      bool &error
		      );


  void update_time(sc_core::sc_time t);
  void send_activity();
  void send_null_message();
};

}}

