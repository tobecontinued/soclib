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
 * Maintainers: fpecheux, alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */
#ifndef __FIFO_READER_H__
#define __FIFO_READER_H__

#include <tlmdt>	   	                // TLM-DT headers
#include "process_wrapper.h"

namespace soclib { namespace tlmdt {

template<typename vci_param>
class FifoReader
  : public sc_core::sc_module
{
private:
  soclib::common::ProcessWrapper m_wrapper;
  typename vci_param::data_t     m_data;
  int                            m_woffset;
  unsigned int                   m_status;
  uint32_t                       m_depth_fifo;
  
  unsigned char                  m_write_buffer[64 * vci_param::nbytes];
  pdes_local_time               *m_pdes_local_time;
  pdes_activity_status          *m_pdes_activity_status;

  sc_core::sc_event              m_rsp_write;

protected:
    SC_HAS_PROCESS(FifoReader);

public:
  tlm_utils::simple_initiator_socket<FifoReader,32,tlm::tlm_base_protocol_types> p_fifo;

  FifoReader( sc_core::sc_module_name name,
	      const std::string &bin,
	      const std::vector<std::string> &argv,
	      uint32_t depth_fifo);

private:

  /////////////////////////////////////////////////////////////////////////////////////
  // Virtual Fuctions  tlm::tlm_bw_transport_if (FIFO TARGET SOCKET)
  /////////////////////////////////////////////////////////////////////////////////////
  
  /// Receive rsp from target
  tlm::tlm_sync_enum my_nb_transport_bw     // for resp messages 
  ( tlm::tlm_generic_payload &payload,      // payload
    tlm::tlm_phase           &phase,        // phase
    sc_core::sc_time         &time);        // time
  
  void execLoop();

};

}}

#endif
