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
 * Maintainers: alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2010
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */
#ifndef SOCLIB_TLMDT_VCI_FRAMEBUFFER_H
#define SOCLIB_TLMDT_VCI_FRAMEBUFFER_H

#include <tlmdt>
#include "mapping_table.h"
#include "soclib_endian.h"
#include "fb_controller.h"

namespace soclib { namespace tlmdt {

template<typename vci_param>
class VciFrameBuffer 
  : public sc_core::sc_module                                             // inherit from SC module base class
  , virtual public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types> // inherit from TLM "forward interface"
{
private:
  /////////////////////////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////////////////////////
  soclib::common::IntTab         m_index;
  soclib::common::MappingTable   m_mt;
  soclib::common::Segment        m_segment;
  soclib::common::FbController   m_framebuffer;
  uint8_t                        *m_surface;
  size_t                         m_update;


  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  tlm::tlm_sync_enum nb_transport_fw        // receive command from initiator
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

protected:
  SC_HAS_PROCESS(VciFrameBuffer);
public:
  tlm::tlm_target_socket<32,tlm::tlm_base_protocol_types> p_vci;   // VCI TARGET socket

  VciFrameBuffer(sc_core::sc_module_name name,
		 const soclib::common::IntTab &index,
		 const soclib::common::MappingTable &mt,
		 size_t width,
		 size_t height,
		 size_t subsampling = 420);
  
  ~VciFrameBuffer();

};

}}

#endif
