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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 */

#include <stdint.h>
#include <iostream>
#include "register.h"
#include "../include/vci_block_device.h"
#include <fcntl.h>
#include "block_device.h"

#define BURST_SIZE (1<<(vci_param::K-1))

namespace soclib { namespace caba {

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

/////////////////////////////
tmpl(void)::ended(int status)
{
#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " finished current operation ("
        << SoclibBlockDeviceOp_str[r_current_op]
        << ") with the status "
        << SoclibBlockDeviceStatus_str[status]
        << std::endl;
#endif

    if ( r_irq_enabled ) r_irq = true;
    r_current_op = r_request_op = BLOCK_DEVICE_NOOP;
    r_status = status;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;

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
	r_buffer = data;
	return true;
    case BLOCK_DEVICE_COUNT:
        r_count = data;
        return true;
    case BLOCK_DEVICE_LBA:
	r_lba = data;
	return true;
    case BLOCK_DEVICE_OP:
        if ( r_status == BLOCK_DEVICE_BUSY ) {
            std::cerr << name() << " warning: receiving a new command while busy, ignored" << std::endl;
        } else {
            r_request_op = data;
        }
	return true;
    case BLOCK_DEVICE_IRQ_ENABLE:
	r_irq_enabled = data;
	return true;
    default:
        return false;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////
tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    int cell = (int)addr / vci_param::B;

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
        return true;
    case BLOCK_DEVICE_BLOCK_SIZE:
        data = m_block_size;
        return true;
    case BLOCK_DEVICE_STATUS:
        data = r_status;
        if (r_status != BLOCK_DEVICE_BUSY) {
            r_status = BLOCK_DEVICE_IDLE;
            r_irq = false;
        }
        return true;
    default:
        return false;
    }
}
//////////////////////////////////
tmpl(void)::read_done( req_t *req )
{
    uint32_t transfer_size = r_count * m_block_size;
    if ( ! req->failed() && r_burst_offset <  transfer_size ) {

#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " completed transferring a burst. Do now the next one..."
        << std::endl;
#endif
        next_req();
        return;
    }

    ended( req->failed() ? BLOCK_DEVICE_READ_ERROR : BLOCK_DEVICE_READ_SUCCESS );
    delete m_data;
    delete req;
}
//////////////////////////////////////
tmpl(void)::write_finish( req_t *req )
{
    uint32_t transfer_size = r_count * m_block_size;
    if ( ! req->failed() && r_burst_offset < transfer_size ) {

#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " completed transferring a burst. Do now the next one..."
        << std::endl;
#endif
        next_req();
        return;
    }

	ended(
        ( ! req->failed() && ::write( m_fd, (char *)m_data, transfer_size ) > 0 )
        ? BLOCK_DEVICE_WRITE_SUCCESS : BLOCK_DEVICE_WRITE_ERROR );
    delete m_data;
    delete req;
}
//////////////////////
tmpl(void)::next_req()
{
    switch (r_current_op) {
    case BLOCK_DEVICE_READ:
    {
	// copy data from the file to a local buffer when it is a new operation
        uint32_t transfer_size 	= r_count * m_block_size;
        if ( r_burst_offset == 0 ) {
            if ( r_lba + r_count > m_device_size ) {
                std::cerr << name() << " warning: trying to read beyond end of device" << std::endl;
                ended(BLOCK_DEVICE_READ_ERROR);
                break;
            }
            m_data = new uint8_t[transfer_size];
            lseek(m_fd, r_lba*m_block_size, SEEK_SET);
            int retval = ::read(m_fd, m_data, transfer_size);
            if ( retval < 0 ) {
                ended(BLOCK_DEVICE_READ_ERROR);
                break;
            }
        }
	// makes the VCI transaction from local buffer to user buffer
        size_t burst_size 	= transfer_size - r_burst_offset;
        if ( burst_size > BURST_SIZE ) burst_size = BURST_SIZE;
        VciInitSimpleWriteReq<vci_param> *req =
            new VciInitSimpleWriteReq<vci_param>(
                r_buffer + r_burst_offset, m_data + r_burst_offset, burst_size );
        r_burst_offset += BURST_SIZE;
        req->setDone( this, ON_T(read_done) );
        m_vci_init_fsm.doReq( req );
	r_status = BLOCK_DEVICE_BUSY;
        break;
    }
    case BLOCK_DEVICE_WRITE:
    {
	// create a local buffer to store data when it is a new operation
        uint32_t transfer_size = r_count * m_block_size;
        if ( r_burst_offset == 0 ) {
            if ( r_lba + r_count > m_device_size ) {
                std::cerr << name() << " warning: trying to write beyond end of device" << std::endl;
                ended(BLOCK_DEVICE_WRITE_ERROR);
                break;
            }
            m_data = new uint8_t[transfer_size];
            lseek(m_fd, r_lba*m_block_size, SEEK_SET);
        }
	// makes the VCI transaction from user buffer to local buffer
        size_t burst_size = transfer_size - r_burst_offset;
        if ( burst_size > BURST_SIZE ) burst_size = BURST_SIZE;
        VciInitSimpleReadReq<vci_param> *req =
            new VciInitSimpleReadReq<vci_param>(
                m_data + r_burst_offset, r_buffer + r_burst_offset, burst_size );
        r_burst_offset += BURST_SIZE;
        req->setDone( this, ON_T(write_finish) );
        m_vci_init_fsm.doReq( req );
	r_status = BLOCK_DEVICE_BUSY;
        break;
    }
    default:
        ended(BLOCK_DEVICE_ERROR);
        break;
    }
}
////////////////////////
tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_target_fsm.reset();
		m_vci_init_fsm.reset();
		r_irq = false;
		r_irq_enabled = false;
		r_request_op = BLOCK_DEVICE_NOOP;
		r_status = BLOCK_DEVICE_IDLE;
		r_access_latency = m_latency;
		return;
	}

	// block device access time modeling
	if ( r_request_op != BLOCK_DEVICE_NOOP ) {
		if ( r_access_latency == 0 ) {

#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " launch an operation "
        << SoclibBlockDeviceOp_str[r_request_op]
        << std::endl;
#endif
			r_current_op = r_request_op;
			r_access_latency = m_latency;
        		r_request_op = BLOCK_DEVICE_NOOP;
        		r_burst_offset = 0;
        		next_req();
		} else {
			r_access_latency--;	
		}
	} else {
		r_access_latency = m_latency;
	}

	// initiator & target FSMs
	m_vci_target_fsm.transition();
	m_vci_init_fsm.transition();
}
/////////////////////
tmpl(void)::genMoore()
{
	m_vci_target_fsm.genMoore();
	m_vci_init_fsm.genMoore();
	p_irq = r_irq && r_irq_enabled;
}

/////// Constructor ///////
tmpl(/**/)::VciBlockDevice(
    sc_module_name name,
    const MappingTable &mt,
    const IntTab &srcid,
    const IntTab &tgtid,
    const std::string &filename,
    const uint32_t block_size, 
    const uint32_t latency)
	: caba::BaseModule(name),
	  m_vci_target_fsm(p_vci_target, mt.getSegmentList(tgtid)),
	  m_vci_init_fsm(p_vci_initiator, mt.indexForId(srcid)),
      m_block_size(block_size),
      m_latency(latency),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci_target("vci_target"),
      p_vci_initiator("vci_initiator"),
      p_irq("irq")
{
    m_vci_target_fsm.on_read_write(on_read, on_write);

    m_fd = ::open(filename.c_str(), O_RDWR);
    if ( m_fd < 0 ) {
        throw soclib::exception::RunTimeError(
            std::string("Unable to open file ")+filename);
    }
    m_device_size = lseek(m_fd, 0, SEEK_END) / m_block_size;
    if ( m_device_size > ((uint64_t)1<<(vci_param::B*8)) ) {
        std::cerr
            << "Warning: block device " << filename
            << " has more blocks than addressable with "
            << (8*vci_param::B) << "." << std::endl;
        m_device_size = ((uint64_t)1<<(vci_param::B*8));
    }
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

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

