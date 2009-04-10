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
#include "fifo_reader.h"

namespace soclib { namespace tlmdt {

#define tmpl(x) template<typename vci_param> x FifoReader<vci_param>

tmpl(/**/)::FifoReader(sc_core::sc_module_name name,
                       const std::string &bin,
                       const std::vector<std::string> &argv,
                       uint32_t depth_fifo)
           : sc_module(name),           // module name
           m_wrapper( bin, argv ),
           p_fifo("fifo")
{
  //register callback function
  p_fifo.register_nb_transport_bw(this, &FifoReader::my_nb_transport_bw);
  
  //PDES local time
  //m_pdes_local_time = new pdes_local_time(time_quantum);
  m_pdes_local_time = new pdes_local_time(sc_core::SC_ZERO_TIME);
  
  //PDES activity status
  m_pdes_activity_status = new pdes_activity_status();
  
  m_depth_fifo = (depth_fifo/vci_param::nbytes);
  m_data = 0;
  m_woffset = 0;
  m_status = 0;

  SC_THREAD(execLoop);
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (FIFO INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
/// receive a answer to a write request
tmpl (tlm::tlm_sync_enum)::my_nb_transport_bw         // inbound nb_transport_bw
( tlm::tlm_generic_payload &payload,                  // payload
  tlm::tlm_phase           &phase,                    // phase
  sc_core::sc_time         &time)                     // response timestamp 
{
  //update time
  if(time > m_pdes_local_time->get())
    m_pdes_local_time->set(time);

  m_rsp_write.notify(sc_core::SC_ZERO_TIME);
  return tlm::TLM_COMPLETED;
}

tmpl(void)::execLoop()
{
  tlm::tlm_generic_payload *payload_ptr   = new tlm::tlm_generic_payload();
  tlm::tlm_phase            phase;
  sc_core::sc_time          time;
  unsigned int              nwords = m_depth_fifo;
  unsigned int              nbytes = nwords * vci_param::nbytes;

  while(true){
    for(unsigned int i=0, j=0; i<nwords; i++, j+=vci_param::nbytes){
      m_status = m_wrapper.read(((char*)&m_data)+m_woffset, sizeof(typename vci_param::data_t)-m_woffset);
      if ( m_status <= sizeof(typename vci_param::data_t) ) {
	m_woffset += m_status;
	if ( m_woffset == sizeof(typename vci_param::data_t) ) {
	  // Got it
	  m_woffset = 0;
	  //m_write_buffer[i] = m_data;
	  utoa( m_data, m_write_buffer, j);
	}
      }
    }
    m_pdes_local_time->add(nwords * UNIT_TIME);

    // set the values in tlm payload
    payload_ptr->set_write();
    payload_ptr->set_data_ptr(m_write_buffer);
    payload_ptr->set_data_length(nbytes);
    //set the tlm phase
    phase = tlm::BEGIN_REQ;
    //set the local time to transaction time
    time = m_pdes_local_time->get();
    //send a write message
    p_fifo->nb_transport_fw(*payload_ptr, phase, time);
    wait(m_rsp_write);
  }
}

}}
