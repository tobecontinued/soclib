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
 
#include <tlmdt>                // TLM-DT headers
#include <inttypes.h>

#include "soclib_endian.h"
#include "write_buffer.h"
#include "generic_cache.h"
#include "mapping_table.h"
#include "mips32.h"

namespace soclib { namespace tlmdt {

template<typename vci_param, typename iss_t>
class VciXcacheWrapper
  : public sc_core::sc_module                                             // inherit from SC module base clase
  , virtual public tlm::tlm_bw_transport_if<tlm::tlm_base_protocol_types> // inherit from TLM "backward interface"
{
private:
  typedef typename vci_param::addr_t addr_t;
  typedef typename vci_param::data_t data_t;
  typedef typename vci_param::data_t tag_t;
  typedef typename vci_param::data_t be_t;
  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  uint32_t                m_id;
  iss_t                   m_iss;
  uint32_t                m_irq;
  std::map<sc_core::sc_time, std::pair<int, bool> > m_pending_irqs;
  pdes_local_time        *m_pdes_local_time;
  pdes_activity_status   *m_pdes_activity_status;
  
  const size_t            m_icache_ways;
  const size_t            m_icache_words;
  const addr_t            m_icache_yzmask;

  const size_t            m_dcache_ways;
  const size_t            m_dcache_words;
  const addr_t            m_dcache_yzmask;
 
  WriteBuffer<addr_t>     m_wbuf;
  GenericCache<addr_t>    m_icache;
  GenericCache<addr_t>    m_dcache;
  soclib::common::AddressDecodingTable<addr_t, bool> m_cacheability_table;
  
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

  /////////////////////////////////////////////////////////////////////////////////////
  // Fuctions
  /////////////////////////////////////////////////////////////////////////////////////
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
			    );
  
  void execLoop();
  
  uint32_t ram_uncacheable_write(
				 enum command command,
				 addr_t address,
				 data_t wdata, 
				 int be, 
				 data_t &rdata, 
				 bool &rerror
				 );

  uint32_t ram_cacheable_write(
			       enum command command, 
			       data_t &rdata, 
			       bool &rerror
			       );

  uint32_t ram_read(
		    enum command command, 
		    addr_t address,
		    data_t *rdata, 
		    bool &rerror,
		    size_t size = 1
		    );

  uint32_t fill_cache(
		      GenericCache<addr_t> &cache,
		      addr_t address,
		      bool &error
		      );

  void update_time(sc_core::sc_time t);
  void send_activity();
  void send_null_message();
  
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
  // Virtual Fuctions  tlm::tlm_fw_transport_if (IRQ TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum my_nb_transport_fw      // receive interruption from initiator
  ( int                      id,             // interruption id
    tlm::tlm_generic_payload &payload,       // payload
    tlm::tlm_phase           &phase,         // phase
    sc_core::sc_time         &time);         // time

protected:
  SC_HAS_PROCESS(VciXcacheWrapper);
  
public:
  tlm::tlm_initiator_socket<32, tlm::tlm_base_protocol_types> p_vci_initiator;   // VCI initiator port
  std::vector<tlm_utils::simple_target_socket_tagged<VciXcacheWrapper,32,tlm::tlm_base_protocol_types> *> p_irq_target;  // IRQ target port
  
  VciXcacheWrapper(
		   sc_core::sc_module_name name,
		   int cpuid,
		   const soclib::common::IntTab &index,
		   const soclib::common::MappingTable &mt,
		   size_t icache_ways,
		   size_t icache_sets,
		   size_t icache_words,
		   size_t dcache_ways,
		   size_t dcache_sets,
		   size_t dcache_words,
		   sc_core::sc_time time_quantum);
  
};

}}

