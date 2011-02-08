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
    return m_vci_payload.is_response_error();
}

////////////////////////////////////////////
tmpl (void)::send_write(size_t burst_length)
{
    m_pdes_local_time->reset_sync();
    
    m_vci_payload.set_address(m_destination & ~3);
    m_vci_payload.set_byte_enable_length(burst_length);
    m_vci_payload.set_data_length(burst_length);
    m_vci_extension.set_write();
    m_vci_time = m_pdes_local_time->get();

#ifdef SOCLIB_MODULE_DEBUG
std::cout <<  "[" <<name() << "] time = " << m_vci_time 
          << std::hex << " WRITE COMMAND / address = " << m_destination << std::endl;
#endif

    p_vci_initiator->nb_transport_fw(m_vci_payload, 
                                     m_vci_phase,
                                     m_vci_time);
}

///////////////////////////////////////////
tmpl (void)::send_read(size_t burst_length)
{
    m_pdes_local_time->reset_sync();

    m_vci_payload.set_address(m_source & ~3);
    m_vci_payload.set_data_length(burst_length);
    m_vci_payload.set_byte_enable_length(burst_length);
    m_vci_extension.set_read();
    m_vci_time = m_pdes_local_time->get();

#ifdef SOCLIB_MODULE_DEBUG
std::cout <<  "[" <<name() << "] time = " << m_vci_time 
          << std::hex << " READ  COMMAND / address = " << m_source << std::endl;
#endif

    p_vci_initiator->nb_transport_fw(m_vci_payload, 
                                     m_vci_phase, 
                                     m_vci_time);
}

////////////////////////
tmpl (void)::send_null()
{
    m_pdes_local_time->reset_sync();
    
    m_null_time = m_pdes_local_time->get();

    p_vci_initiator->nb_transport_fw(m_null_payload, 
                                     m_null_phase,
                                     m_null_time);
}

///////////////////////////
tmpl(void)::set_interrupt()
{
    *m_irq_data_buf = 0xFF;
    m_irq_time = m_pdes_local_time->get();

#ifdef SOCLIB_MODULE_DEBUG
std::cout << "[" <<name() << "] time = " << m_irq_time
          << " SET INTERRUPT : value = " << atou(m_irq_payload.get_data_ptr(),0) << std::endl;
#endif

    p_irq->nb_transport_fw(m_irq_payload, 
                           m_irq_phase,
                           m_irq_time);
}

/////////////////////////////
tmpl(void)::reset_interrupt()
{
    *m_irq_data_buf = 0x00;
    m_irq_time = m_pdes_local_time->get();

#ifdef SOCLIB_MODULE_DEBUG
std::cout <<  "[" <<name() << "] time = " << m_irq_time
          << " RESET INTERRUPT : value = " << atou(m_irq_payload.get_data_ptr(),0) << std::endl;
#endif

    p_irq->nb_transport_fw(m_irq_payload, 
                           m_irq_phase,
                           m_irq_time);
}
////////////////////////////////////////////
// thread associated to the initiator
////////////////////////////////////////////
tmpl (void)::execLoop ()
{

    while(true)
    {
        m_pdes_local_time->add(UNIT_TIME);

        switch ( m_state ) {
        case STATE_IDLE:
        {
            if ( !m_stop ) m_state = STATE_READ;
            else if ( m_pdes_local_time->need_sync() )
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
        break;
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
        break;
        case STATE_ERROR_READ:
        case STATE_ERROR_WRITE:
        case STATE_SUCCESS:
        {
            if ( m_stop ) 
            {
                m_state = STATE_IDLE;
                if ( !m_irq_disabled )	reset_interrupt();
            }
            else if ( m_pdes_local_time->need_sync() )
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
    if(time > m_pdes_local_time->get()) m_pdes_local_time->set(time);

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
    bool	one_flit = (payload.get_data_length() == (unsigned int)vci_param::nbytes);
    addr_t	address = payload.get_address();

    soclib_payload_extension* extension_pointer;
    payload.get_extension(extension_pointer);
    
    // If a VCI or null message is received when the DMA is running
    // (stare READ or state WRITE), we do not update local time and
    // we do not write in the software visible registers.

    if ( extension_pointer->is_null_message() )
    { 
        if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) )
            if (time > m_pdes_local_time->get()) m_pdes_local_time->set(time);
        return tlm::TLM_COMPLETED;
    }

    // address checking
    if ( m_segment.contains(payload.get_address()) && one_flit )
    {
        if ( (m_state != STATE_READ) && (m_state != STATE_WRITE) )
        {
            if(time > m_pdes_local_time->get()) m_pdes_local_time->set(time);
        }

        cell = (int)((address - m_segment.baseAddress()) / vci_param::nbytes);

        if (extension_pointer->get_command() == VCI_READ_COMMAND)
        {
                    
#ifdef SOCLIB_MODULE_DEBUG
std::cout <<  "[" <<name() << "] time = " << time 
          << " STATUS REQUEST / " << cell << std::endl;
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
        } // end READ

        if (extension_pointer->get_command() == VCI_WRITE_COMMAND)
        {
            data_t data = atou(payload.get_data_ptr(), 0);

#ifdef SOCLIB_MODULE_DEBUG
std::cout <<  "[" <<name() << "] time = " << time 
          << " CONFIGURATION REQUEST / " << cell << std::endl;
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

    if (is_legal)   payload.set_response_status(tlm::TLM_OK_RESPONSE);
    else            payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);

    phase = tlm::BEGIN_RESP;
    time  = m_pdes_local_time->get();
    p_vci_target->nb_transport_bw(payload,
                                  phase,
                                  time);
    return tlm::TLM_COMPLETED;
}

/////////////////////////////////////////////////////////////////////////////////////
// Interface function used when receiving a response to an IRQ transaction
/////////////////////////////////////////////////////////////////////////////////////
tmpl (tlm::tlm_sync_enum)::irq_nb_transport_bw ( tlm::tlm_generic_payload    &payload,
                                                 tlm::tlm_phase              &phase,
                                                 sc_core::sc_time            &time) 
{
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
          , m_segment(mt.getSegment(tgtid))
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
    m_pdes_local_time = new pdes_local_time(100*UNIT_TIME);
    
    // create and initialize the local buffers for VCI transactions (N bytes)
    m_vci_data_buf = new unsigned char[max_burst_length];
    m_vci_be_buf = new unsigned char[max_burst_length];
    for ( size_t i=0 ; i<max_burst_length ; i++)
    {
        m_vci_data_buf[i] = 0;
        m_vci_be_buf[i] = 0xFF;
    }

    // create the local buffer for IRQ transaction
    m_irq_data_buf = new unsigned char;

    // initialize payload, phase and extension for a VCI read/write transaction
    m_vci_payload.set_command(tlm::TLM_IGNORE_COMMAND);
    m_vci_payload.set_data_ptr(m_vci_data_buf);
    m_vci_payload.set_byte_enable_ptr(m_vci_be_buf);
    m_vci_payload.set_extension(&m_vci_extension);
    m_vci_extension.set_src_id(m_srcid);
    m_vci_extension.set_trd_id(0);
    m_vci_extension.set_pkt_id(0);
    m_vci_phase = tlm::BEGIN_REQ;

    // initialize payload, phase and extension for a null message
    m_null_payload.set_extension(&m_null_extension);
    m_null_extension.set_null_message();
    m_null_phase = tlm::BEGIN_REQ;

    // initialize payload and phase for an irq message
    m_irq_payload.set_data_ptr(m_irq_data_buf);
    m_irq_phase = tlm::BEGIN_REQ;

    // register initialisation 
    m_state = STATE_IDLE;
    m_stop  = true;
    m_irq_disabled = false;
    
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

