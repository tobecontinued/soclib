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
 * Copyright (c) UPMC, Lip6, Asim
 *         Alain Greiner <alain.greiner@lip6.fr>, 2011
 */

#include <stdint.h>
#include <iostream>
#include "register.h"
#include "../include/vci_dma.h"
#include <fcntl.h>
#include "dma.h"

namespace soclib { namespace tlmdt {

#define tmpl(t) template<typename vci_param> t VciDma<vci_param>

////////////////////////////
// service functions
////////////////////////////
tmpl (bool)::vci_rsp_error()
{
    return m_rw_payload_ptr->is_response_error();
}

////////////////////////////////////////////
tmpl (void)::send_write(size_t burst_length)
{
    //reset the synchronisation counter 
    m_local_time->reset_sync();
    
    // set the values in tlm payload
    m_rw_payload_ptr->set_address(m_destination & ~3);
    m_rw_payload_ptr->set_byte_enable_length(burst_length);
    m_rw_payload_ptr->set_data_length(burst_length);
    m_rw_extension_ptr->set_write();
    
    //send write message
    p_vci_initiator->nb_transport_fw(*m_rw_payload_ptr, 
                                     tlm::BEGIN_REQ,
                                     m_local_time.get());
}

/////////////////////////////////////////////
tmpl (void)::send_read(uint32_t burst_length)
{
    // reset the synchronization counter
    m_local_time->reset_sync();

    // set the values in tlm payload
    m_rw_payload_ptr->set_address(m_source & ~3);
    m_rw_payload_ptr->set_data_length(burst_length);
    m_rw_payload_ptr->set_byte_enable_length(burst_length);
    m_rw_extension_ptr->set_read();

    //send read message
    p_vci_initiator->nb_transport_fw(*m_rw_payload_ptr, 
                                     tlm::BEGIN_REQ, 
                                     m_local_time.get());
}

////////////////////////
tmpl (void)::send_null()
{
    m_local_time->reset_sync();
    
    p_vci_initiator->nb_transport_fw(*m_rw_payload_ptr, 
                                     tlm::BEGIN_REQ,
                                     m_local_time.get());
}

///////////////////////////
tmpl(void)::set_interrupt()
{
    m_irq_data_buf = 0xFF;

    p_irq->nb_transport_fw(m_irq_payload, 
                           tlm::BEGIN_REQ,
                           m_local_time.get());
}

/////////////////////////////
tmpl(void)::reset_interrupt()
{
    m_irq_data_buf = 0;

    p_irq->nb_transport_fw(m_irq_payload, 
                           tlm::BEGIN_REQ,
                           m_local_time.get());
}
////////////////////////////////////////////
// thread associated to the initiator
////////////////////////////////////////////
tmpl (void)::execLoop ()
{
    while(true)
    {
        switch ( m_state ) {
        case STATE_IDLE:
        {
            if ( !m_stop ) m_state = STATE_READ;
            else if ( m_local_time->need_sync() )
            {
                send_null();
                wait(m_rsp_received);
            }
        } 
        break;
        case STATE_READ:
        {
            uint32_t length;
            if ( m_length < m_max_burst_length )	length = m_length;
            else					length = m_max_burst_length;
            // blocking command
            send_read(length);
            wait(m_rsp_received);
            // response analysis
            if ( !vci_rsp_error() ) 
            {
                m_state = STATE_WRITE;
            }
            else  			
            {
                m_state = STATE_ERROR_READ; 
                if ( !m_irq_disabled )	set_interrupt();
            }
        }
_       break;
        case STATE_WRITE:
        {
            uint32_t length;
            if ( m_length < m_max_burst_length )        length = m_length; 
            else                                        length = m_max_burst_length;
            // blocking command
            send_write(length);
            wait(m_rsp_received);
            // registers update
            m_length            = m_length - length;
            m_source            = m_source + length;
            m_destination       = m_destination + length;
            // response analysis
            if ( m_stop ) 		
            {
                m_state = STATE_IDLE;
            }
            else if ( vci_rsp_error() )
            {
                m_state = STATE_ERROR_WRITE;
                if ( !m_irq_disabled )	set_interrupt();
            }
            else if ( m_length == 0 )
            {
                m_state = STATE_SUCCESS;
                if ( !m_irq_disabled )	set_interrupt();
            }
            else
            {
                m_state = STATE_READ;
            }
        }
_       break;
        case STATE_ERROR_READ:
        case STATE_ERROR_WRITE;
        case STATE_ERROR_SUCCESS;
        {
            if ( m_stop ) 
            {
                m_state = STATE_IDLE;
                if ( !m_irq_disabled )	reset_interrupt();
            }
            else if ( m_local_time->need_sync() )
            {
                send_null();
                wait(m_rsp_received);
            }
        }
        } // end switch
    } // end while
} // end execLoop

////////////////////////////////////////////////////////////////////////////////////////////
// Interface function used when receiving a response to a VCI command on the initiator port
////////////////////////////////////////////////////////////////////////////////////////////
tmpl (tlm::tlm_sync_enum)::nb_transport_bw ( tlm::tlm_generic_payload &payload, 
                                             tlm::tlm_phase           &phase,       
                                             sc_core::sc_time         &time)  
{
    if(time > m_local_time->get()) m_local_time->set(time);

#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Receive VCI response / time = " << time.value() << std::endl;
#endif
  
    m_rsp_received.notify (sc_core::SC_ZERO_TIME);

    return tlm::TLM_COMPLETED;
}

///////////////////////////////////////////////////////////////////////////////
// Interface function used when receiving a VCI command on the target port
///////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw ( tlm::tlm_generic_payload &payload,
                                            tlm::tlm_phase           &phase, 
                                            sc_core::sc_time         &time)   
{
    int 	cell;
    bool	is_legal = false;
    bool	one_flit = (payload.get_data_length() == vci_param::nbytes);
    addr_t	address = payload.get_address();

    soclib_payload_extension* extension_pointer;
    payload.get_extension(extension_pointer);
    
    // If a VCI or null message is received when the DMA is running
    // (stare READ or state WRITE), we do not update local time and
    // we do not write in the software visible registers.

    if ( extension_pointer->is_null_message() )
    { 
        if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) )
            if (time > m_local_time->get()) m_local_time->set(time);
        return tlm::TLM_COMPLETED;
    }

    // address checking
    if ( m_segment.contains(payload.get_address()) && one_flit )
    {
        if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) )
        {
            if(time > m_local_time->get()) m_local_time->set(time);
        }

        cell = (int)((address - m_segment.baseAddress()) / vci_param::nbytes);

        if (extension_pointer->get_command() == VCI_READ_COMMAND)
        {
                    
#ifdef SOCLIB_MODULE_DEBUG
std::cout << name() << " read config register " << cell << std::endl;
#endif

            switch (cell) {
            case DMA_SRC:
                utoa(m_source, payload.get_data_ptr(), 0);
                is_legal = true;
            break;
            case DMA_DST:
                utoa(m_destination, payload.get_data_ptr(), 0);
                is_legal = true;
            break;
            case DMA_LEN:
                utoa(m_state, payload.get_data_ptr(), 0);
                is_legal = true;
            break;
            case DMA_IRQ_DISABLED:
                utoa((int)(m_irq_disabled), payload.get_data_ptr(), 0);
                is_legal = true;
            break;
            } // end switch read
            utoa(data, payload.get_data_ptr(), 0);
        } // end READ

        if (extension_pointer->get_command() == VCI_WRITE_COMMAND)
        {
            data_t data = atou(payload.get_data_ptr(), 0);

#ifdef SOCLIB_MODULE_DEBUG
std::cout << name() << " write config register " << cell << " with data 0x" << std::hex << data << std::endl;
#endif
            switch (cell) {
            case DMA_SRC:
                is_legal = true;
                if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) ) m_source = data;
            break;
            case DMA_DST:
                is_legal = true;
                if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) ) m_destination = data;
            break;
            case DMA_LEN:
                is_legal = true;
                if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) ) m_length = data;
                m_stop = false;
            break;
            case DMA_IRQ_DISABLED:
                is_legal = true;
                if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) ) m_irq_disabled = (data != 0);
            break;
            case DMA_RESET:
                is_legal = true;
                m_stop = true;
            break;
            } // end switch
        } // end WRITE
    } // end if legal address 

    m_cmd_received.notify (sc_core::SC_ZERO_TIME);
    
    if (is_legal)   payload.set_response_status(tlm::TLM_OK_RESPONSE);
    else            payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);

    p_vci_target->nb_transport_bw(payload,
                                  tlm::BEGIN_RSP,
                                  m_local_time.get());
    return tlm::TLM_COMPLETED;
}

/////////////////////////////////////////////////////////////////////////////////////
// Interface function used when receiving a response to an IRQ transaction
/////////////////////////////////////////////////////////////////////////////////////
tmpl (tlm::tlm_sync_enum)::irq_nb_transport_bw ( tlm::tlm_generic_payload    &payload,
                                                 tlm::tlm_phase              &phase,
                                                 sc_core::sc_time            &time) 
{
#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Receive IRQ response time = " << time.value() << std::endl;
#endif
  return tlm::TLM_COMPLETED;
}

/////////////////////////////////////////////////////////////
// Not implemented but required by interface
/////////////////////////////////////////////////////////////
tmpl(void)::b_transport ( tlm::tlm_generic_payload &payload, 
                          sc_core::sc_time         &_time)
{
    return;
}

tmpl(bool)::get_direct_mem_ptr ( tlm::tlm_generic_payload &payload, 
                                 tlm::tlm_dmi             &dmi_data) 
{ 
    return false;
}
    
tmpl(unsigned int):: transport_dbg ( tlm::tlm_generic_payload &payload)
{
    return false;
}

tmpl(void)::invalidate_direct_mem_ptr ( sc_dt::uint64 start_range,
                                        sc_dt::uint64 end_range) 
{
    return;
}

////////////////////////////////
// Constructor
////////////////////////////////
tmpl(/**/)::VciDma( sc_module_name name,
                    const soclib::common::MappingTable &mt,
                    const soclib::common::IntTab &srcid,
                    const soclib::common::IntTab &tgtid,
                    const size_t max_burst_length)
           : sc_core::sc_module(name)
          , m_srcid(mt.indexForId(srcid))
          , m_max_burst_length(max_burst_length)
          , p_vci_initiator("p_vci_init") 
          , p_vci_target("p_vci_tgt")    
          , p_irq("p_irq")     
{
    // bind vci initiator port
    p_vci_initiator(*this);                     
    
    // bind vci target port
    p_vci_target(*this);                     
    
    // register irq initiator port
    p_irq.register_nb_transport_bw(this, &VciDma::irq_nb_transport_bw);
    
    // initialize PDES local time
    m_local_time = new pdes_local_time(100*UNIT_TIME);
    
    // create and initialize the local buffers for VCI transactions (N bytes)
    m_vci_data_buf = new char[m_max_burst_length];
    m_vci_be_buf = new char[m_max_burst_length];
    for ( size_t i=0 ; i<m_max_burst_length ; i++)
    {
        m_vci_data_buf = 0;
        m_vci_be_buf = 0xFF;
    }

    // create and initialize payload and extension for a VCI read/write transaction
    m_rw_payload_ptr = new tlm::tlm_generic_payload();
    m_rw_extension_ptr = new soclib_payload_extension();
    m_rw_payload_ptr->set_command(tlm::TLM_IGNORE_COMMAND);
    m_rw_payload_ptr->set_data_ptr(m_vci_data_buf);
    m_rw_payload_ptr->set_byte_enable_ptr(m_vci_be_buf);
    m_rw_extension_ptr->set_src_id(m_srcid);
    m_rw_extension_ptr->set_trd_id(0);
    m_rw_extension_ptr->set_pkt_id(0);
    m_rw_payload_ptr->set_extension(m_rw_extension_ptr);

    // create and initialize payload and extension for a null message
    m_null_payload_ptr = new tlm::tlm_generic_payload();
    m_null_extension_ptr = new soclib_payload_extension();
    m_null_extension_ptr->set_null_message();
    m_null_extension_ptr->set_src_id(m_srcid);
    m_null_payload_ptr->set_extension(m_null_extension_ptr);

    // create and initialize payload for an irq message
    m_irq_payload_ptr = new tlm::tlm_generic_payload();
    m_irq_payload.set_data_ptr(m_irq_data_buf);

    // register initialisation 
    m_state = STATE_IDLE;
    m_irq_enabled = false;
    
    // get the associated segment
    assert ( (mt.getSegmentList.begin() == mt.getSegmentList.end() ) &&
          "error : the VciDma component cannot use more than one segment");
    m_segment = *mt.getSegmentList(tgtid).front();

    SC_THREAD(execLoop);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
