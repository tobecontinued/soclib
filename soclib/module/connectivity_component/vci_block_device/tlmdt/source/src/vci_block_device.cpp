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
 *         Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>, 2010
 */

#include <stdint.h>
#include <iostream>
#include "register.h"
#include "../include/vci_block_device.h"
#include <fcntl.h>
#include "block_device.h"

#define CHUNCK_SIZE (1<<8)

namespace soclib { namespace tlmdt {

#ifdef SOCLIB_MODULE_DEBUG
    namespace {
        const char* SoclibBlockDeviceRegisters_str[] = {
            "BLOCK_DEVICE_BUFFER",
            "BLOCK_DEVICE_LBA",
            "BLOCK_DEVICE_COUNT",
            "BLOCK_DEVICE_OP",
            "BLOCK_DEVICE_STATUS",
            "BLOCK_DEVICE_IRQ_ENABLE",
            "BLOCK_DEVICE_SIZE",
            "BLOCK_DEVICE_BLOCK_SIZE"
        };
        const char* SoclibBlockDeviceOp_str[] = {
            "BLOCK_DEVICE_NOOP",
            "BLOCK_DEVICE_READ",
            "BLOCK_DEVICE_WRITE",
        };
        const char* SoclibBlockDeviceStatus_str[] = {
            "BLOCK_DEVICE_IDLE",
            "BLOCK_DEVICE_BUSY",
            "BLOCK_DEVICE_READ_SUCCESS",
            "BLOCK_DEVICE_WRITE_SUCCESS",
            "BLOCK_DEVICE_READ_ERROR",
            "BLOCK_DEVICE_WRITE_ERROR",
            "BLOCK_DEVICE_ERROR",
        };
    }
#endif

#define tmpl(t) template<typename vci_param> t VciBlockDevice<vci_param>

tmpl (void)::update_time(sc_core::sc_time t){
    if(t > m_pdes_local_time->get()){
        m_pdes_local_time->set(t);
    }
}

tmpl (void)::send_null_message(){
    // set the null message command
    m_null_extension_ptr->set_null_message();
    m_null_extension_ptr->set_src_id(m_id);
    // set the extension to tlm payload
    m_null_payload_ptr->set_extension(m_null_extension_ptr);
    //set the tlm phase
    m_null_phase = tlm::BEGIN_REQ;
    //set the local time to transaction time
    m_null_time = m_pdes_local_time->get();

    m_pdes_local_time->reset_sync();
    
#ifdef SOCLIB_MODULE_DEBUG
    //std::cout << name() << " send NULL MESSAGE time = " << m_null_time.value() << std::endl;
#endif
    
    //send a null message
    p_vci_initiator->nb_transport_fw(*m_null_payload_ptr, m_null_phase, m_null_time);
    //deschedule the initiator thread
    wait(m_rsp_received);
}

tmpl (void)::send_write_message(size_t chunck_size){
    
    m_nbytes = chunck_size;
    m_address = m_buffer+m_chunck_offset;
    
    data_t be = vci_param::be2mask(0xF);;
    for(data_t i=0; i< m_nbytes; i+=vci_param::nbytes){
        utoa(be, m_byte_enable_ptr, i);
    }
        
    // set the values in tlm payload
    m_payload_ptr->set_command(tlm::TLM_IGNORE_COMMAND);
    m_payload_ptr->set_address(m_address & ~3);
    m_payload_ptr->set_byte_enable_ptr(m_byte_enable_ptr);
    m_payload_ptr->set_byte_enable_length(m_nbytes);
    m_payload_ptr->set_data_ptr(m_data+m_chunck_offset);
    m_payload_ptr->set_data_length(m_nbytes);
    // set the values in payload extension
    m_extension_ptr->set_write();
    m_extension_ptr->set_src_id(m_id);
    m_extension_ptr->set_trd_id(0);
    m_extension_ptr->set_pkt_id(0);
    // set the extension to tlm payload
    m_payload_ptr->set_extension(m_extension_ptr);
    //set the tlm phase
    m_phase = tlm::BEGIN_REQ;
    //set the local time to transaction time
    m_time = m_pdes_local_time->get();
    //reset the counter to synchronize
    m_pdes_local_time->reset_sync();
    
    //send a write message
    p_vci_initiator->nb_transport_fw(*m_payload_ptr, m_phase, m_time);
    //deschedule the initiator thread
    wait(m_rsp_received);
}

tmpl (void)::send_read_message(size_t chunck_size){
    m_nbytes = chunck_size;
    m_address = m_buffer+m_chunck_offset;

    data_t be = vci_param::be2mask(0xF);;
    for(data_t i=0; i< m_nbytes; i+=vci_param::nbytes){
        utoa(be, m_byte_enable_ptr, i);
    }

    m_payload_ptr = new tlm::tlm_generic_payload();
    m_extension_ptr = new soclib_payload_extension();
  
    // set the values in tlm payload
    m_payload_ptr->set_command(tlm::TLM_IGNORE_COMMAND);
    m_payload_ptr->set_address(m_address & ~3);
    m_payload_ptr->set_byte_enable_ptr(m_byte_enable_ptr);
    m_payload_ptr->set_byte_enable_length(m_nbytes);
    m_payload_ptr->set_data_ptr(m_data+m_chunck_offset);
    //m_payload_ptr->set_data_ptr(m_data_ptr);
    m_payload_ptr->set_data_length(m_nbytes);
    // set the values in payload extension
    m_extension_ptr->set_read();
    m_extension_ptr->set_src_id(m_id);
    m_extension_ptr->set_trd_id(0);
    m_extension_ptr->set_pkt_id(0);
    // set the extension to tlm payload
    m_payload_ptr->set_extension(m_extension_ptr);
    //set the tlm phase
    m_phase = tlm::BEGIN_REQ;
    //set the local time to transaction time
    m_time = m_pdes_local_time->get();
    //reset the counter to synchronize
    m_pdes_local_time->reset_sync();
#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " send READ message time = " << std::dec << m_time.value() << std::endl;
#endif
    //send a read message
    p_vci_initiator->nb_transport_fw(*m_payload_ptr, m_phase, m_time);
    //deschedule the initiator thread
    wait(m_rsp_received);
}

tmpl(void)::ended(int status){
    
#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " finished current operation ("
        << SoclibBlockDeviceOp_str[m_current_op]
        << ") with the status "
        << SoclibBlockDeviceStatus_str[status]
        << std::endl;
#endif

	if ( m_irq_enabled ){
		m_irq = true;
        m_irq_changed = true;
    }
	m_current_op = m_op = BLOCK_DEVICE_NOOP;
    m_status = status;
}

tmpl(void)::next_req(){
    switch (m_current_op) {
    case BLOCK_DEVICE_READ:
    {
        m_transfer_size = m_count * m_block_size;
        if ( m_count && m_chunck_offset == 0 ) {
            if ( m_lba + m_count > m_device_size ) {
                std::cerr << name() << " warning: trying to read beyond end of device" << std::endl;
                ended(BLOCK_DEVICE_READ_ERROR);
                break;
            }
            m_data = new uint8_t[m_transfer_size];
            lseek(m_fd, m_lba*m_block_size, SEEK_SET);
            int retval = ::read(m_fd, m_data, m_transfer_size);
            if ( retval < 0 ) {
                ended(BLOCK_DEVICE_READ_ERROR);
                break;
            }
        }
        size_t chunck_size = m_transfer_size-m_chunck_offset;
        if ( chunck_size > CHUNCK_SIZE )
            chunck_size = CHUNCK_SIZE;

        send_write_message(chunck_size);
        m_chunck_offset += CHUNCK_SIZE;
		m_status = BLOCK_DEVICE_BUSY;
        read_done();
        break;
    }
    case BLOCK_DEVICE_WRITE:
    {
        m_transfer_size = m_count * m_block_size;
        if ( m_count && m_chunck_offset == 0 ) {
            if ( m_lba + m_count > m_device_size ) {
                std::cerr << name() << " warning: trying to write beyond end of device" << std::endl;
                ended(BLOCK_DEVICE_WRITE_ERROR);
                break;
            }
            m_data = new uint8_t[m_transfer_size];
            lseek(m_fd, m_lba*m_block_size, SEEK_SET);
        }
        size_t chunck_size = m_transfer_size-m_chunck_offset;
        if ( chunck_size > CHUNCK_SIZE )
            chunck_size = CHUNCK_SIZE;

        send_read_message(chunck_size);
        m_chunck_offset += CHUNCK_SIZE;
		m_status = BLOCK_DEVICE_BUSY;
        write_finish();
        break;
    }
    default:
        ended(BLOCK_DEVICE_ERROR);
        break;
    }
}
    
tmpl(void)::write_finish( ){
    
    if ( ! m_payload_ptr->is_response_error() && m_chunck_offset < m_transfer_size ) {
#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " completed transferring a chunck. Do now the next one..."
        << std::endl;
#endif
        next_req();
        return;
    }

	ended(
        ( ! m_payload_ptr->is_response_error() && m_fd >= 0 && ::write( m_fd, (char *)m_data, m_transfer_size ) > 0 )
        ? BLOCK_DEVICE_WRITE_SUCCESS : BLOCK_DEVICE_WRITE_ERROR );
    delete m_data;
    
}

tmpl(void)::read_done( ){
    
    if ( ! m_payload_ptr->is_response_error() && m_chunck_offset < m_transfer_size ) {
#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " completed transferring a chunck. Do now the next one..."
        << std::endl;
#endif
        next_req();
        return;
    }

	ended( m_payload_ptr->is_response_error() ? BLOCK_DEVICE_READ_ERROR : BLOCK_DEVICE_READ_SUCCESS );
    delete m_data;
    
}
    
tmpl(void)::send_interruption(){
    // set the values in irq tlm payload
    data_t nwords= 1;
    data_t nbytes= nwords * vci_param::nbytes;
    data_t byte_enable = vci_param::be2mask(0xF);
    unsigned char data_ptr[nbytes];
    unsigned char byte_enable_ptr[nbytes];
    
    utoa(byte_enable, byte_enable_ptr, 0);
    utoa(m_irq, data_ptr, 0);
  
    // set the values in irq tlm payload
    m_irq_payload.set_byte_enable_ptr(byte_enable_ptr);
    m_irq_payload.set_byte_enable_length(nbytes);
    m_irq_payload.set_data_ptr(data_ptr);
    m_irq_payload.set_data_length(nbytes);
    
    // set the tlm phase
    m_irq_phase = tlm::BEGIN_REQ;
    // set the local time to transaction time
    m_irq_time = m_pdes_local_time->get();

#if SOCLIB_MODULE_DEBUG
    std::cout << "[" << name() << "] Send Interruption " << m_irq << " with time = " <<  m_irq_time.value() << std::endl;
#endif

    // send the transaction
    p_irq->nb_transport_fw(m_irq_payload, m_irq_phase, m_irq_time);
}

tmpl (void)::execLoop (){
    while(true){
        
        m_pdes_local_time->add(UNIT_TIME);

        if ( m_current_op == BLOCK_DEVICE_NOOP &&
             m_op != BLOCK_DEVICE_NOOP ) {
            if ( m_access_latency ) {
                m_access_latency--;
            } else {
#ifdef SOCLIB_MODULE_DEBUG
                std::cout 
                    << name()
                    << " launch an operation "
                    << SoclibBlockDeviceOp_str[m_op]
                    << std::endl;
#endif
                m_current_op = m_op;
                m_op = BLOCK_DEVICE_NOOP;
                m_chunck_offset = 0;
                next_req();
            }
        }        

        // if initiator needs synchronize then it sends a null message
        if (m_pdes_local_time->need_sync()) {
            send_null_message();
        }

        if(m_irq_enabled && m_irq_changed){
            send_interruption();
            m_irq_changed = false;
        }
    }
    sc_core::sc_stop();
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (VCI INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl (tlm::tlm_sync_enum)::nb_transport_bw    
( tlm::tlm_generic_payload           &payload,       // payload
  tlm::tlm_phase                     &phase,         // phase
  sc_core::sc_time                   &time)          // time
{
    update_time(time);

    //this target does not treat the null message
#ifdef SOCLIB_MODULE_DEBUG
    soclib_payload_extension *extension_pointer;
    payload.get_extension(extension_pointer);
    if(extension_pointer->is_null_message()){
        //std::cout << name() << " Receive response NULL MESSAGE time = " << time.value() << std::endl;
    }
    else{
        std::cout << name() << " Receive response time = " << time.value() << std::endl;
    }
#endif

    m_rsp_received.notify (sc_core::SC_ZERO_TIME);

    return tlm::TLM_COMPLETED;
}

// Not implemented for this example but required by interface
tmpl(void)::invalidate_direct_mem_ptr               // invalidate_direct_mem_ptr
( sc_dt::uint64 start_range,                        // start range
  sc_dt::uint64 end_range                           // end range
) 
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_fw_transport_if (VCI TARGET SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl(tlm::tlm_sync_enum)::nb_transport_fw 
( tlm::tlm_generic_payload &payload, // payload
  tlm::tlm_phase           &phase,   // phase
  sc_core::sc_time         &time)    // time
{
    soclib_payload_extension *extension_pointer;
    payload.get_extension(extension_pointer);
    
    //update local time
    if(time > m_pdes_local_time->get())
        m_pdes_local_time->set(time);
    
    //this target does not treat the null message
    if(extension_pointer->is_null_message()){
        return tlm::TLM_COMPLETED;
    }
  
    typename vci_param::data_t data;
    int cell;
    uint32_t nwords = payload.get_data_length() / vci_param::nbytes;
  
    std::list<soclib::common::Segment>::iterator seg;
    size_t segIndex;
    for (segIndex=0,seg = segList.begin();seg != segList.end(); ++segIndex, ++seg ){
        soclib::common::Segment &s = *seg;
    
        if (!s.contains(payload.get_address()))
            continue;
        switch(extension_pointer->get_command()){
        case VCI_READ_COMMAND:
            {
                for(unsigned int i=0; i<nwords; i++){
                    
                    cell = (int)(((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes);
                    
#ifdef SOCLIB_MODULE_DEBUG
                    std::cout 
                        << name()
                        << " read config register "
                        << SoclibBlockDeviceRegisters_str[cell]
                        << std::endl;
#endif
                    
                    switch ((enum SoclibBlockDeviceRegisters)cell) {
                    case BLOCK_DEVICE_SIZE:
                        data = (typename vci_param::fast_data_t)m_device_size;
                        utoa(data, payload.get_data_ptr(),(i * vci_param::nbytes));
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        break;
                    case BLOCK_DEVICE_BLOCK_SIZE:
                        utoa(m_block_size, payload.get_data_ptr(),(i * vci_param::nbytes));
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        break;
                    case BLOCK_DEVICE_STATUS:
                        utoa(m_status, payload.get_data_ptr(),(i * vci_param::nbytes));
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        if (m_status != BLOCK_DEVICE_BUSY) {
                            m_status = BLOCK_DEVICE_IDLE;
                            m_irq = false;
                            m_irq_changed = true;
                        }
                        break;
                    default:
                        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                        break;
                    }
                }
                
                phase = tlm::BEGIN_RESP;
                time = time + (nwords * UNIT_TIME);
                p_vci_target->nb_transport_bw(payload, phase, time);
                return tlm::TLM_COMPLETED;
            }//end case READ
            break;
        case VCI_WRITE_COMMAND:
            {
                for(unsigned int i=0; i<nwords; i++){
                    cell = (int)(((payload.get_address()+(i*vci_param::nbytes)) - s.baseAddress()) / vci_param::nbytes);
                    data = atou(payload.get_data_ptr(), (i * vci_param::nbytes));

#ifdef SOCLIB_MODULE_DEBUG
                    std::cout 
                        << name()
                        << " write config register "
                        << SoclibBlockDeviceRegisters_str[cell]
                        << " with data 0x"
                        << std::hex << data
                        << std::endl;
#endif

                    switch ((enum SoclibBlockDeviceRegisters)cell) {
                    case BLOCK_DEVICE_BUFFER:
                        m_buffer = data;
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        break;
                     case BLOCK_DEVICE_COUNT:
                        m_count = data;
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        break;
                    case BLOCK_DEVICE_LBA:
                        m_lba = data;
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        break;
                    case BLOCK_DEVICE_OP:
                        if ( m_status == BLOCK_DEVICE_BUSY ||
                             m_op != BLOCK_DEVICE_NOOP ) {
                            std::cerr << name() << " warning: receiving a new command while busy, ignored" << std::endl;
                        } else {
                            m_op = data;
                            m_access_latency = m_lfsr % (m_latency+1);
                            m_lfsr = (m_lfsr >> 1) ^ ((-(m_lfsr & 1)) & 0xd0000001);
                        }
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        break;
                      case BLOCK_DEVICE_IRQ_ENABLE:
                        m_irq_enabled = data;
                        payload.set_response_status(tlm::TLM_OK_RESPONSE);
                        break;
                      default:
                        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                        break;
                    }
                } //end for

                phase = tlm::BEGIN_RESP;
                time = time + (nwords * UNIT_TIME);
                p_vci_target->nb_transport_bw(payload, phase, time);
                return tlm::TLM_COMPLETED;
            }//end case WRITE
            break;
	    default:
            break;
        }// end switch
    }//end for segments        
    
    //send error message
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    phase  = tlm::BEGIN_RESP;
    time = time + UNIT_TIME;
        
    p_vci_target->nb_transport_bw(payload, phase, time);
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

/////////////////////////////////////////////////////////////////////////////////////
// Virtual Fuctions  tlm::tlm_bw_transport_if (IRQ INITIATOR SOCKET)
/////////////////////////////////////////////////////////////////////////////////////
tmpl (tlm::tlm_sync_enum)::irq_nb_transport_bw    
( tlm::tlm_generic_payload           &payload,       // payload
  tlm::tlm_phase                     &phase,         // phase
  sc_core::sc_time                   &time)          // time
{
#ifdef SOCLIB_MODULE_DEBUG
    std::cout << name() << " Receive IRQ response time = " << time.value() << std::endl;
#endif

  return tlm::TLM_COMPLETED;
}

tmpl(/**/)::VciBlockDevice(
    sc_module_name name,
    const soclib::common::MappingTable &mt,
    const soclib::common::IntTab &srcid,
    const soclib::common::IntTab &tgtid,
    const std::string &filename,
    const uint32_t block_size, 
    const uint32_t latency)
           : sc_core::sc_module(name)
          , m_block_size(block_size)
          , m_latency(latency)
          , m_id(mt.indexForId(srcid))
          , p_vci_initiator("init") // vci initiator socket
          , p_vci_target("tgt")     // vci target socket
          , p_irq("irq")            // irq initiator socket
{
    // bind vci initiator
    p_vci_initiator(*this);                     
    
    // bind vci target
    p_vci_target(*this);                     
    
    // register irq initiator
    p_irq.register_nb_transport_bw(this, &VciBlockDevice::irq_nb_transport_bw);
    
    //PDES local time
    m_pdes_local_time = new pdes_local_time(100*UNIT_TIME);
    
    //create payload and extension to a normal message
    m_payload_ptr = new tlm::tlm_generic_payload();
    m_extension_ptr = new soclib_payload_extension();
    
    //create payload and extension to a null message
    m_null_payload_ptr = new tlm::tlm_generic_payload();
    m_null_extension_ptr = new soclib_payload_extension();
    
    m_fd = ::open(filename.c_str(), O_RDWR);
    if ( m_fd < 0 ) {
        std::cerr << "Unable to open block device image file " << filename << std::endl;
        m_device_size = 0;
    } else {
        m_device_size = lseek(m_fd, 0, SEEK_END) / m_block_size;
        if ( m_device_size > ((uint64_t)1<<(vci_param::nbytes*8)) ) {
            std::cerr
                << "Warning: block device " << filename
                << " has more blocks than addressable with "
                << (8*vci_param::nbytes) << "." << std::endl;
            m_device_size = ((uint64_t)1<<(vci_param::nbytes*8));
        }
    }
    
    segList=mt.getSegmentList(tgtid);
    m_irq = false;
    m_irq_enabled = false;
    m_irq_changed = false;
    m_op = BLOCK_DEVICE_NOOP;
    m_current_op = BLOCK_DEVICE_NOOP;
    m_access_latency = 0;
    m_status = BLOCK_DEVICE_IDLE;
    m_lfsr = -1;
    
#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name
        << " = Opened " 
        << filename
        << " which has "
        << m_device_size
        << " blocks of "
        << m_block_size
        << " bytes"
        << std::endl;
#endif

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

