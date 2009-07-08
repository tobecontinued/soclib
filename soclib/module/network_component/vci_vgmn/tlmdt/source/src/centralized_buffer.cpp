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

#include <systemc>
#include <cmath>
#include <climits>
#include <limits>
#include <cstdlib>

#include "centralized_buffer.h"

#ifndef CENTRALIZED_BUFFER_DEBUG
#define CENTRALIZED_BUFFER_DEBUG 0
#endif

namespace soclib { namespace tlmdt {

class _command
{
  friend class centralized_buffer;

  std::list<transaction> fifo;
  bool active;
  bool eligible;

public:
  _command()
  {
    init();
  }

  void init()
  {
    active = true;
    eligible = true;
  }

};

centralized_buffer::centralized_buffer( size_t nslots )
  : m_slots(nslots),
    m_centralized_buffer(new _command[nslots]),
    m_free_slots(nslots)
{
  pthread_spin_init(&m_lock, PTHREAD_PROCESS_PRIVATE);
}

centralized_buffer::~centralized_buffer()
{
  delete [] m_centralized_buffer;
}

void centralized_buffer::push( size_t from,
			     tlm::tlm_generic_payload &payload,
			     tlm::tlm_phase &phase,
			     const sc_core::sc_time &time)
{
 
#if CENTRALIZED_BUFFER_DEBUG
  std::cout << "PUSH pending[" << from <<"] ";
#endif

  //get payload extension
  soclib_payload_extension *extension_pointer;
  payload.get_extension(extension_pointer);

  //if transaction command is active
  //the source is actived and the transaction is not sent
  if(extension_pointer->is_active()){
    set_activity(from, extension_pointer->is_active());
  }
  else{
    //if there is no transaction in the slot then the number of free slots is reduced
    if (m_centralized_buffer[from].fifo.empty()) {
      m_free_slots--;
    }    
    
    transaction trans;
    trans.payload = &payload;
    trans.phase = phase;
    trans.time = time;
    
    m_centralized_buffer[from].fifo.push_back(trans);
    
#if CENTRALIZED_BUFFER_DEBUG
    std::cout << "PUSH m_free_slots=" << m_free_slots << std::endl;
#endif
  }
}

    /*
bool centralized_buffer::pop_inf( size_t &from,
				tlm::tlm_generic_payload *&payload,
				tlm::tlm_phase &phase,
				sc_core::sc_time &time,
				const sc_core::sc_time &max_time)
{
#if CENTRALIZED_BUFFER_DEBUG
  std::cout << "POP INF" << std::endl;
#endif

  int min_i = -1;
  sc_core::sc_time min_time = max_time;

  for ( size_t i = 1; i<m_slots; ++i ) {
    struct _command &cmd = m_centralized_buffer[i];
    if ( cmd.payload && cmd.time < min_time ) {
      min_time = cmd.time;
      min_i = i;
    }
  }

  if ( min_i == -1 )
    return false;

  from = min_i;
  _command &min_cmd = m_centralized_buffer[min_i];
  payload = min_cmd.payload;
  phase = min_cmd.phase;
  time = min_cmd.time;

  if ( time < max_time ) {
    min_cmd.clear();

    m_free_slots++;
    return true;
  }
  return false;
}
    */

void centralized_buffer::pop( size_t &from,
			    tlm::tlm_generic_payload *&payload,
			    tlm::tlm_phase &phase,
			    sc_core::sc_time &time)
{
#if CENTRALIZED_BUFFER_DEBUG
  std::cout << "POP ";
#endif

  size_t min_i = 0;
  sc_core::sc_time min_time = MAX_TIME;
  transaction trans;
  soclib_payload_extension *extension_ptr;
  bool get_next;

  for ( size_t i = 0; i<m_slots; ++i ) {
    //optimization
    //if the current transaction is a null message and the fifo has more packets then it gets next transaction
    do{
      get_next = false;
      trans =  m_centralized_buffer[i].fifo.front();
      
      if(!m_centralized_buffer[i].fifo.empty()){
      	trans.payload->get_extension(extension_ptr);
	if(extension_ptr->is_null_message()){
	  if(m_centralized_buffer[i].fifo.size()>1){
	    m_centralized_buffer[i].fifo.pop_front();
	    get_next = true;
	  }
	}
      }
    }while(get_next);

    if ( m_centralized_buffer[i].active && m_centralized_buffer[i].eligible && trans.time < min_time ) {
      min_time = trans.time;
      min_i = i;
    }
  }

  from = min_i;
#if CENTRALIZED_BUFFER_DEBUG
  std::cout << "select " << from << std::endl;
#endif

  transaction min_trans =  m_centralized_buffer[min_i].fifo.front();
  payload = min_trans.payload;
  phase = min_trans.phase;
  time = min_trans.time;
  m_centralized_buffer[min_i].fifo.pop_front();
  if (m_centralized_buffer[min_i].fifo.empty())
    m_free_slots++;
  
#if CENTRALIZED_BUFFER_DEBUG
  soclib_payload_extension *ext;
  payload->get_extension(ext);
  std::cout << "srcid =  " << ext->get_src_id() << " command = " << ext->get_command() << std::endl;
#endif
}

void centralized_buffer::pop( size_t from)
{
#if CENTRALIZED_BUFFER_DEBUG
  std::cout << "POP selected " << from << std::endl;
#endif

  m_centralized_buffer[from].fifo.pop_front();
  if (m_centralized_buffer[from].fifo.empty())
    m_free_slots++;
}

void centralized_buffer::get_selected_transaction( size_t &from,
						   tlm::tlm_generic_payload *&payload,
						   tlm::tlm_phase &phase,
						   sc_core::sc_time &time)
{
  size_t min_i = 0;
  sc_core::sc_time min_time = MAX_TIME;

  transaction trans;
  soclib_payload_extension *extension_ptr;
  bool get_next;

  for ( size_t i = 0; i<m_slots; ++i ) {
    
    //optimization
    //if the current transaction is a null message and the fifo has more packets then it gets next transaction
    do{
      get_next = false;
      trans =  m_centralized_buffer[i].fifo.front();
      
      if(!m_centralized_buffer[i].fifo.empty()){
      	trans.payload->get_extension(extension_ptr);
	if(extension_ptr->is_null_message()){
	  if(m_centralized_buffer[i].fifo.size()>1){
	    m_centralized_buffer[i].fifo.pop_front();
	    get_next = true;
	  }
	}
      }
    }while(get_next);

    if ( m_centralized_buffer[i].active && m_centralized_buffer[i].eligible && trans.time < min_time ) {
      min_time = trans.time;
      min_i = i;
    }
  }

  from = min_i;
#if CENTRALIZED_BUFFER_DEBUG
  std::cout << "select " << from << std::endl;
#endif


  transaction min_trans =  m_centralized_buffer[min_i].fifo.front();
  payload = min_trans.payload;
  phase = min_trans.phase;
  time = min_trans.time;

#if CENTRALIZED_BUFFER_DEBUG
  soclib_payload_extension *ext;
  payload->get_extension(ext);
  std::cout << "GET SELECTED TRANSACTION srcid =  " << ext->get_src_id() << " command = " << ext->get_command() << std::endl;
#endif
}

bool centralized_buffer::can_pop()
{

  if(m_free_slots == 0){
    for ( size_t i = 0; i<m_slots; ++i ) {
      if(m_centralized_buffer[i].active)
	return true;
    }
  }
  return false;
}

void centralized_buffer::set_activity(unsigned int index, bool b)
{
  m_centralized_buffer[index].active = b;
  if(m_centralized_buffer[index].fifo.empty()){
    if(b)
      m_free_slots++;
    else
      m_free_slots--;
  }

#if CENTRALIZED_BUFFER_DEBUG
  std::cout << "ACTIVE[" << index <<"] = " << b << " m_free_slots=" << m_free_slots << std::endl;
#endif
}

void centralized_buffer::set_external_access(unsigned int index, bool external_access)
{
  m_centralized_buffer[index].eligible = !external_access;
 
  if(external_access)
    m_free_slots--;
  else
    m_free_slots++;
}

}}
