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

#include "../include/vci_framebuffer.h"

#ifndef VCI_FRAMEBUFFER_DEBUG
#define VCI_FRAMEBUFFER_DEBUG 0
#endif

namespace soclib { namespace tlmdt {

#define tmpl(x) template<typename vci_param> x VciFrameBuffer<vci_param>

tmpl(/**/)::VciFrameBuffer
( sc_core::sc_module_name name,
  const soclib::common::IntTab &index,
  const soclib::common::MappingTable &mt,
  size_t width,
  size_t height,
  size_t subsampling
  )
	   : sc_module(name)
	  , m_index(index)
	  , m_mt(mt)
	  , m_segment(m_mt.getSegment(m_index))
	  , m_framebuffer((const char*)name, width, height, subsampling)
	  , m_surface((uint8_t*)m_framebuffer.surface())
	  , m_update(width-1)
	  , p_vci("socket")
{
  // bind target
  p_vci(*this);                     

  assert( m_segment.size() >= m_framebuffer.m_surface_size &&
	  "Framebuffer segment too short." );
}

tmpl(/**/)::~VciFrameBuffer()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if VCI SOCKET
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw
( tlm::tlm_generic_payload &payload,
  tlm::tlm_phase           &phase,  
  sc_core::sc_time         &time)   
{
  soclib_payload_extension *extension_pointer;
  payload.get_extension(extension_pointer);

  //this target does not treat the null message
  if(extension_pointer->is_null_message()){
    return tlm::TLM_COMPLETED;
  }

  size_t address = payload.get_address() - m_segment.baseAddress();
  size_t nwords = payload.get_data_length() / sizeof(typename vci_param::data_t);
  int index;

  if ( m_segment.contains(payload.get_address())) {
      switch(extension_pointer->get_command()){
      case VCI_READ_COMMAND:
          memcpy(payload.get_data_ptr(),
                 m_surface + address,
                 payload.get_data_length());
	
          payload.set_response_status(tlm::TLM_OK_RESPONSE);
          phase = tlm::BEGIN_RESP;
          time = time + ((( 2 * nwords) - 1) * UNIT_TIME);
	
          p_vci->nb_transport_bw(payload, phase, time);
          return tlm::TLM_COMPLETED;
          break;

      case VCI_WRITE_COMMAND:
	  for ( size_t i=0; i<nwords; ++i ){
	    index = address / vci_param::nbytes;
	    uint32_t *tab = m_framebuffer.surface();
	    unsigned int cur = tab[index];
	    uint32_t mask =  atou(payload.get_byte_enable_ptr(), (i * vci_param::nbytes));
	    uint32_t data =  atou( payload.get_data_ptr(), (i * vci_param::nbytes) );
	    
	    tab[index] = (cur & ~mask) | (data & mask);
	    
#ifdef SOCLIB_MODULE_DEBUG
	    std::cout << "[" << name() << "] WRITE tab["<< index <<"] = " << ( (cur & ~mask) | (data & mask)) << std::endl;
#endif
	  }
	  
	  if((index%m_update)==0)
	    m_framebuffer.update();
		
          payload.set_response_status(tlm::TLM_OK_RESPONSE);
          phase = tlm::BEGIN_RESP;
          time = time + ((( 2 * nwords) - 1) * UNIT_TIME);
	
          p_vci->nb_transport_bw(payload, phase, time);
          return tlm::TLM_COMPLETED;
          break;
      default:
          assert("command does not exist in VciFrameBuffer");
          break;
      }
  }

  //send error message
  payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  
  phase = tlm::BEGIN_RESP;
  time = time + (nwords * UNIT_TIME);
  
#ifdef SOCLIB_MODULE_DEBUG
  std::cout << "[" << name() << "] Address " << std::hex << payload.get_address() << std::dec << " does not match any segment " << std::endl;
#endif
  p_vci->nb_transport_bw(payload, phase, time);
  return tlm::TLM_COMPLETED;
}

// Not implemented for this example but required by interface
tmpl(void)::b_transport
( tlm::tlm_generic_payload &payload,                // payload
  sc_core::sc_time         &_time)                  //time
{
  return;
}

// Not implemented for this example but required by interface
tmpl(bool)::get_direct_mem_ptr
( tlm::tlm_generic_payload &payload,                // address + extensions
  tlm::tlm_dmi             &dmi_data)               // DMI data
{ 
  return false;
}
    
// Not implemented for this example but required by interface
tmpl(unsigned int):: transport_dbg                            
( tlm::tlm_generic_payload &payload)                // debug payload
{
  return false;
}
 
}}

