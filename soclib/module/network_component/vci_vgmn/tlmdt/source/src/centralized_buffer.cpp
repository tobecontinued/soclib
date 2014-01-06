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
#include "centralized_buffer.h"

namespace soclib { namespace tlmdt {

////////////////////////////////////////////////////////////////////////////
//   constructor / destructor for central buffer
////////////////////////////////////////////////////////////////////////////
centralized_buffer::centralized_buffer ( sc_core::sc_module_name   name,
                                         size_t                    nslots )
  : sc_module(name)
  , m_slots(nslots)
  , m_port_array(new init_port_descriptor[nslots])
{
    for(unsigned int i=0; i<nslots; i++)
    {
        std::ostringstream buf_name;
        buf_name << "slot_" << i;
        m_port_array[i].buffer.set_name(buf_name.str());
    }
}

centralized_buffer::~centralized_buffer()
{
  delete [] m_port_array;
}

///////////////////////////////////////////////////////////////
// This function push a transaction (payload, phase, time)
// in the circular buffer associated to initiator (from)
///////////////////////////////////////////////////////////////
bool centralized_buffer::push ( size_t                    from,
                                tlm::tlm_generic_payload  &payload,
                                tlm::tlm_phase            &phase,
                                sc_core::sc_time          &time)
{

#if SOCLIB_MODULE_DEBUG
std::cout << "[" << name() << "] PUSH [" << from <<"] " << std::endl;
#endif

    assert(!(time < m_port_array[from].delta_time) and
    "PUSH transaction in a slot with a time smaller than precedent");

    return m_port_array[from].buffer.push(payload, phase, time);
}

////////////////////////////////////////////////////////////////////////////
// This function implements the PDES time filtering algorithm:
// All active initiators are scanned, to select the earliest date.
// - if there is no transaction for this initiator, (ok=false) is returned,
//   and no transaction is consumed in the central buffer.
// - if there is a transaction, (ok=true) is returned. The selected 
//   initiator index is returned in (from). The transaction parameters 
//   are returned in (payload, phase, time), and the transaction is 
//   removed from the central buffer.
///////////////////////////////////////////////////////////////////////////
bool centralized_buffer::pop ( size_t                    &from,
                               tlm::tlm_generic_payload* &payload,
                               tlm::tlm_phase*           &phase,
                               sc_core::sc_time*         &time )
{
    bool ok = false;
    uint64_t min_time = MAX_TIME;
    int sel_id;				        // currently selected port
    uint64_t time_value;            // date of the currently selected port
  
    for(unsigned int i=0; i<m_slots; i++)
    {
        if(m_port_array[i].active)   // only active ports are competing
        {
            if(m_port_array[i].buffer.is_empty())   // no transaction available
            {
	            time       = &m_port_array[i].delta_time;
	            time_value = (*time).value();

                if(time_value < min_time)
                {
	                sel_id = i;
	                min_time = time_value;
	                ok = false;
	            }
            }
            else                             // transaction available for this port
            {
	            m_port_array[i].buffer.get_front(payload, phase, time);
	            time_value = (*time).value();
	  
	            if(time_value < min_time)
                {
	                sel_id = i;
	                min_time = time_value;
	                ok = true;
	            }
            }
        }
    }

    from = sel_id;

    if ( ok )  // The transaction is removed from central buffer
    {
        m_port_array[sel_id].buffer.pop(payload, phase, time);
    }

    return ok;
} // end pop()
   
/////////////////////////////////////////////////////
circular_buffer centralized_buffer::get_buffer(int i)
{
    return m_port_array[i].buffer;
}

/////////////////////////////////////////////
const size_t centralized_buffer::get_nslots()
{
    return m_slots;
}

///////////////////////////////////////////////////////////////////////
sc_core::sc_time centralized_buffer::get_delta_time(unsigned int index)
{
    return m_port_array[index].delta_time;
}

///////////////////////////////////////////////////////////////////////////////
void centralized_buffer::set_delta_time(unsigned int index, sc_core::sc_time t)
{
    m_port_array[index].delta_time = t;

#if SOCLIB_MODULE_DEBUG
std::cout << "[" << name() << "] DELTA_TIME[" << index <<"] = " << t.value() << std::endl;
#endif

}

////////////////////////////////////////////////////////////////
void centralized_buffer::set_activity(unsigned int index, bool b)
{
    m_port_array[index].active = b;

#if SOCLIB_MODULE_DEBUG
std::cout << "[" << name() << "] ACTIVE[" << index <<"] = " << b << std::endl;
#endif

}

}}
