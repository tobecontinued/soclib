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
 *         Eric Guthmuller  <eric.guthmuller@polytechnique.edu>, 2009
 */

#include <stdint.h>
#include <iostream>
#include "register.h"
#include "../include/vci_block_device_tsar_v2.h"
#include <fcntl.h>
#include "block_device_tsar_v2.h"

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

#define tmpl(t) template<typename vci_param> t VciBlockDeviceTsarV2<vci_param>

tmpl(void)::ended(int status)
{
#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " finished current operation ("
        << SoclibBlockDeviceOp_str[m_current_op]
        << ") with the status "
        << SoclibBlockDeviceStatus_str[status]
        << std::endl;
#endif

	if ( m_irq_enabled )
		r_irq = true;
	m_current_op = m_op = BLOCK_DEVICE_NOOP;
    m_status = status;
}

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;

#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " write config register "
        << SoclibBlockDeviceRegisters_str[cell]
        << " with data 0x"
        << std::hex << data.read()
        << std::endl;
#endif

	switch ((enum SoclibBlockDeviceRegisters)cell) {
    case BLOCK_DEVICE_BUFFER:
		m_buffer = data;
		return true;
    case BLOCK_DEVICE_COUNT:
        m_count = data;
        return true;
    case BLOCK_DEVICE_LBA:
		m_lba = data;
		return true;
    case BLOCK_DEVICE_OP:
        if ( m_status == BLOCK_DEVICE_BUSY ) {
            std::cerr << name() << " warning: receiving a new command while busy, ignored" << std::endl;
        } else {
            m_op = data;
        }
		return true;
    case BLOCK_DEVICE_IRQ_ENABLE:
		m_irq_enabled = data;
		return true;
    default:
        return false;
	}
}

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
        data = m_status;
        if (m_status != BLOCK_DEVICE_BUSY) {
            m_status = BLOCK_DEVICE_IDLE;
            r_irq = false;
        }
        return true;
    default:
        return false;
	}
}

tmpl(void)::read_done( req_t *req )
{
    if ( ! req->failed() && m_chunck_offset < m_transfer_size ) {
#ifdef SOCLIB_MODULE_DEBUG
    std::cout 
        << name()
        << " completed transferring a chunck. Do now the next one..."
        << std::endl;
#endif
        next_req();
        return;
    }

	ended( req->failed() ? BLOCK_DEVICE_READ_ERROR : BLOCK_DEVICE_READ_SUCCESS );
    delete m_data;
	delete req;
}

tmpl(void)::write_finish( req_t *req )
{
    if ( ! req->failed() && m_chunck_offset < m_transfer_size ) {
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
        ( ! req->failed() && ::write( m_fd, (char *)m_data, m_transfer_size ) > 0 )
        ? BLOCK_DEVICE_WRITE_SUCCESS : BLOCK_DEVICE_WRITE_ERROR );
    delete m_data;
	delete req;
}

tmpl(void)::next_req()
{
    switch (m_current_op) {
    case BLOCK_DEVICE_READ:
    {
        m_transfer_size = m_count * m_block_size;
        if ( m_chunck_offset == 0 ) {
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
        if( m_chunck_offset == 0 )
            chunck_size = ((m_buffer & ~(m_burst_size-1)) + m_burst_size) - m_buffer;
        if ( chunck_size > m_burst_size )
            chunck_size = m_burst_size;
        VciInitSimpleWriteReq<vci_param> *req =
            new VciInitSimpleWriteReq<vci_param>(
                m_buffer+m_chunck_offset, m_data+m_chunck_offset, chunck_size );
        m_chunck_offset += chunck_size;
        req->setDone( this, ON_T(read_done) );
        m_vci_init_fsm.doReq( req );
		m_status = BLOCK_DEVICE_BUSY;
        break;
    }
    case BLOCK_DEVICE_WRITE:
    {
        m_transfer_size = m_count * m_block_size;
        if ( m_chunck_offset == 0 ) {
            if ( m_lba + m_count > m_device_size ) {
                std::cerr << name() << " warning: trying to write beyond end of device" << std::endl;
                ended(BLOCK_DEVICE_WRITE_ERROR);
                break;
            }
            m_data = new uint8_t[m_transfer_size];
            lseek(m_fd, m_lba*m_block_size, SEEK_SET);
        }
        size_t chunck_size = m_transfer_size-m_chunck_offset;
        if ( (( m_buffer + m_chunck_offset ) & ( m_burst_size-1 )) )
            chunck_size = m_burst_size - (( m_buffer + m_chunck_offset ) & ( m_burst_size-1 ));
        if ( chunck_size > m_burst_size )
            chunck_size = m_burst_size;
        VciInitSimpleReadReq<vci_param> *req =
            new VciInitSimpleReadReq<vci_param>(
                m_data+m_chunck_offset, m_buffer+m_chunck_offset, chunck_size );
        m_chunck_offset += chunck_size;
        req->setDone( this, ON_T(write_finish) );
        m_vci_init_fsm.doReq( req );
		m_status = BLOCK_DEVICE_BUSY;
        break;
    }
    default:
        ended(BLOCK_DEVICE_ERROR);
        break;
    }
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_target_fsm.reset();
		m_vci_init_fsm.reset();
		r_irq = false;
		m_irq_enabled = false;
		m_status = BLOCK_DEVICE_IDLE;
		return;
	}

	if ( m_current_op == BLOCK_DEVICE_NOOP &&
		 m_op != BLOCK_DEVICE_NOOP ) {
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

	m_vci_target_fsm.transition();
	m_vci_init_fsm.transition();
}

tmpl(void)::genMoore()
{
	m_vci_target_fsm.genMoore();
	m_vci_init_fsm.genMoore();

	p_irq = r_irq && m_irq_enabled;
}

tmpl(/**/)::VciBlockDeviceTsarV2(
    sc_module_name name,
    const MappingTable &mt,
    const IntTab &srcid,
    const IntTab &tgtid,
    const std::string &filename,
    const uint32_t block_size,
    const uint32_t burst_size)
	: caba::BaseModule(name),
	  m_vci_target_fsm(p_vci_target, mt.getSegmentList(tgtid)),
	  m_vci_init_fsm(p_vci_initiator, mt.indexForId(srcid)),
      m_block_size(block_size),
      m_burst_size(burst_size),
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

