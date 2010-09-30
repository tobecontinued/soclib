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

#ifndef CENTRALIZED_BUFFER_H
#define CENTRALIZED_BUFFER_H

#include <inttypes.h>
#include <list>
#include <sstream>
#include <tlmdt>	   // TLM-DT headers
#include "soclib_spinlock.h"

namespace soclib { namespace tlmdt {

    struct transaction{
      tlm::tlm_generic_payload *payload;
      tlm::tlm_phase           *phase;
      sc_core::sc_time         *time;
    };
    
class _command;

class centralized_buffer
{

  const size_t    m_slots;
  _command       *m_centralized_buffer;
  size_t          m_free_slots;
  int             m_selected_slot;
  int             m_count_push;
  int             m_count_pop;

  soclib::common::SpinLock m_lock;

public:
  centralized_buffer(size_t max);

  ~centralized_buffer();

  void push(
	    size_t                    from,
	    tlm::tlm_generic_payload &payload,
	    tlm::tlm_phase           &phase,
	    sc_core::sc_time         &time);

  bool can_pop();

  void pop(
	   size_t from);

  void get_selected_transaction(
				size_t                    &from,
				tlm::tlm_generic_payload *&payload,
				tlm::tlm_phase           *&phase,
				sc_core::sc_time         *&time);

  void set_activity(unsigned int index, bool b);

  void set_external_access(unsigned int index, bool b);

  inline void lock(){
      m_lock.spin();
  }

  inline void unlock(){
      m_lock.release();
  }
};

}}

#endif
