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

#include <systemc>
#include "initiator_vci_transactor_buffer.h"

namespace soclib { namespace tlmdt {

#define tmpl(x) template<typename vci_param_caba,typename vci_param_tlmdt> x initiator_vci_transactor_buffer<vci_param_caba,vci_param_tlmdt>

tmpl(/**/)::initiator_vci_transactor_buffer()
{
  init();
}

tmpl(/**/)::~initiator_vci_transactor_buffer()
{
}

tmpl(void)::init()
{
  int n_entries = (int)pow(2,vci_param_caba::T);
  m_buffer = new transaction_index_struct[n_entries];
  for(int i=0; i<n_entries; i++){
    m_buffer[i].status = EMPTY;
    //create payload and extension of a transaction
    m_buffer[i].payload = new tlm::tlm_generic_payload();
    unsigned char *data = new unsigned char[100];
    m_buffer[i].payload->set_data_ptr(data);
    unsigned char *byte_enable = new unsigned char[100];
    m_buffer[i].payload->set_byte_enable_ptr(byte_enable);
    soclib_payload_extension *extension = new soclib_payload_extension();
    m_buffer[i].payload->set_extension(extension);
    m_buffer[i].phase = tlm::UNINITIALIZED_PHASE;
    m_buffer[i].time = sc_core::SC_ZERO_TIME;
  }
}

tmpl(void)::set_status(int idx, int status){
  m_buffer[idx].status = status;
}

tmpl(void)::set_command(int idx, tlm::tlm_command cmd){
  m_buffer[idx].payload->set_command(cmd);
}

tmpl(void)::set_address(int idx, sc_dt::uint64 add){
  m_buffer[idx].payload->set_address(add);
}

tmpl(void)::set_ext_command(int idx, enum command cmd){
  soclib_payload_extension *extension;
  m_buffer[idx].payload->get_extension(extension);
  extension->set_command(cmd);
}

tmpl(void)::set_src_id(int idx, unsigned int src){
  soclib_payload_extension *extension;
  m_buffer[idx].payload->get_extension(extension);
  extension->set_src_id(src);
}

tmpl(void)::set_trd_id(int idx, unsigned int trd){
  soclib_payload_extension *extension;
  m_buffer[idx].payload->get_extension(extension);
  extension->set_trd_id(trd);
}

tmpl(void)::set_pkt_id(int idx, unsigned int pkt){
  soclib_payload_extension *extension;
  m_buffer[idx].payload->get_extension(extension);
  extension->set_pkt_id(pkt);
}

tmpl(void)::set_data(int idx, int idx_data, typename vci_param_caba::data_t int_data){
  unsigned char *data = m_buffer[idx].payload->get_data_ptr();
  utoa(int_data, data, idx_data * vci_param_tlmdt::nbytes);
}

tmpl(void)::set_data_length(int idx, unsigned int length){
  m_buffer[idx].payload->set_data_length(length);
}

tmpl(void)::set_byte_enable(int idx, int idx_be, typename vci_param_caba::data_t int_be){
  unsigned char *be = m_buffer[idx].payload->get_byte_enable_ptr();
  utoa(int_be, be, idx_be * vci_param_tlmdt::nbytes);
}

tmpl(void)::set_byte_enable_length(int idx, unsigned int length){
  m_buffer[idx].payload->set_byte_enable_length(length);
}

tmpl(void)::set_phase(int idx, tlm::tlm_phase phase){
  m_buffer[idx].phase = phase;
}

tmpl(void)::set_time(int idx, sc_core::sc_time time){
  m_buffer[idx].time = time;
}

tmpl(int)::set_response(tlm::tlm_generic_payload &payload){
  int idx;
  soclib_payload_extension *extension;
  unsigned char *rsp_data = payload.get_data_ptr();
  unsigned int length = payload.get_data_length();
  payload.get_extension(extension);
  idx = extension->get_trd_id();

  //set response to buffer
  m_buffer[idx].status = COMPLETED;
  m_buffer[idx].payload->set_response_status(payload.get_response_status());

  unsigned char *data = m_buffer[idx].payload->get_data_ptr();
  for(unsigned int i=0; i<length; i++)
    data[i] = rsp_data[i];
      //m_buffer[idx].payload->set_data_ptr(payload.get_data_ptr());

  return idx;
}

tmpl(unsigned int)::get_src_id(int idx){
  soclib_payload_extension *extension;
  m_buffer[idx].payload->get_extension(extension);
  return extension->get_src_id();
}

tmpl(unsigned int)::get_pkt_id(int idx){
  soclib_payload_extension *extension;
  m_buffer[idx].payload->get_extension(extension);
  return extension->get_pkt_id();
}

tmpl(unsigned int)::get_trd_id(int idx){
  soclib_payload_extension *extension;
  m_buffer[idx].payload->get_extension(extension);
  return extension->get_trd_id();
}  

tmpl(bool)::get_response_status(int idx){
  if(m_buffer[idx].payload->get_response_status()==tlm::TLM_OK_RESPONSE)
    return false;

  return true;
}

tmpl(sc_dt::uint64)::get_address(int idx){
  return m_buffer[idx].payload->get_address();
}

tmpl(typename vci_param_caba::data_t)::get_data(int idx, int idx_data){
  return atou(m_buffer[idx].payload->get_data_ptr(), idx_data*vci_param_tlmdt::nbytes);
}

tmpl(tlm::tlm_generic_payload*)::get_payload(int idx){
  return m_buffer[idx].payload;
}

tmpl(tlm::tlm_phase*)::get_phase(int idx){
  return &m_buffer[idx].phase;
}

tmpl(sc_core::sc_time*)::get_time(int idx){
  return &m_buffer[idx].time;
}

tmpl(unsigned int)::get_time_value(int idx){
  return m_buffer[idx].time.value();
}

}}
