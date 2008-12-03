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
#define SYSTEMC_SOURCE
#include "block_device.h"
#undef SYSTEMC_SOURCE

#define CHUNCK_SIZE (1<<(vci_param::K-1))

namespace soclib { namespace caba {

#define tmpl(t) template<typename vci_param> t VciBlockDevice<vci_param>

tmpl(void)::ended(int status)
{
	if ( m_irq_enabled )
		r_irq = true;
	m_current_op = m_op = BLOCK_DEVICE_NOOP;
    m_status = status;
}

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;

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
            if ( m_lba + m_count > m_device_size ) {
                m_status = BLOCK_DEVICE_ERROR;
                std::cerr << name() << " warning: trying to access beyond end of device" << std::endl;
            } else
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

	switch ((enum SoclibBlockDeviceRegisters)cell) {
    case BLOCK_DEVICE_IRQ_ENABLE:
		data = r_irq;
		return true;
    case BLOCK_DEVICE_SIZE:
        data = (typename vci_param::fast_data_t)m_device_size;
        return true;
    case BLOCK_DEVICE_BLOCK_SIZE:
        data = m_block_size;
        return true;
    case BLOCK_DEVICE_STATUS:
        data = m_status;
        switch ((enum SoclibBlockDeviceStatus)m_status) {
        case BLOCK_DEVICE_SUCCESS:
        case BLOCK_DEVICE_ERROR:
            m_status = BLOCK_DEVICE_IDLE;
            break;
        default:
            break;
        }
        return true;
    default:
        return false;
	}
}

tmpl(void)::read_done( req_t *req )
{
    if ( ! req->failed() && m_chunck_offset < m_transfer_size ) {
        next_req();
        return;
    }

	ended( req->failed() ? BLOCK_DEVICE_ERROR : BLOCK_DEVICE_SUCCESS );
    delete m_data;
	delete req;
}

tmpl(void)::write_finish( req_t *req )
{
	ended(
        ( ! req->failed() && ::write( m_fd, (char *)m_data, m_count ) > 0 )
        ? BLOCK_DEVICE_SUCCESS : BLOCK_DEVICE_ERROR );
    delete m_data;
	delete req;
}

tmpl(void)::next_req()
{
    switch ((enum SoclibBlockDeviceOp)m_current_op) {
    case BLOCK_DEVICE_NOOP:
		m_status = BLOCK_DEVICE_ERROR;
    case BLOCK_DEVICE_READ:
    {
        m_transfer_size = m_count * m_block_size;
        if ( m_chunck_offset == 0 ) {
            m_data = new uint8_t[m_transfer_size];
            lseek(m_fd, m_lba*m_block_size, SEEK_SET);
            int retval = ::read(m_fd, m_data, m_transfer_size);
            if ( retval < 0 ) {
                ended(BLOCK_DEVICE_ERROR);
                break;
            }
        }
        size_t chunck_size = m_transfer_size-m_chunck_offset;
        if ( chunck_size > CHUNCK_SIZE )
            chunck_size = CHUNCK_SIZE;
        VciInitSimpleWriteReq<vci_param> *req =
            new VciInitSimpleWriteReq<vci_param>(
                m_buffer+m_chunck_offset, m_data+m_chunck_offset, chunck_size );
        m_chunck_offset += CHUNCK_SIZE;
        req->setDone( this, ON_T(read_done) );
        m_vci_init_fsm.doReq( req );
		m_status = BLOCK_DEVICE_BUSY;
        break;
    }
    case BLOCK_DEVICE_WRITE:
    {
        m_transfer_size = m_count * m_block_size;
        if ( m_chunck_offset == 0 ) {
            m_data = new uint8_t[m_transfer_size];
            lseek(m_fd, m_lba*m_block_size, SEEK_SET);
        }
        size_t chunck_size = m_transfer_size-m_chunck_offset;
        if ( chunck_size > CHUNCK_SIZE )
            chunck_size = CHUNCK_SIZE;
        VciInitSimpleReadReq<vci_param> *req =
            new VciInitSimpleReadReq<vci_param>(
                m_data+m_chunck_offset, m_buffer+m_chunck_offset, chunck_size );
        m_chunck_offset += CHUNCK_SIZE;
        req->setDone( this, ON_T(write_finish) );
        m_vci_init_fsm.doReq( req );
		m_status = BLOCK_DEVICE_BUSY;
        break;
    }
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

tmpl(/**/)::VciBlockDevice(
    sc_module_name name,
    const MappingTable &mt,
    const IntTab &srcid,
    const IntTab &tgtid,
    const std::string &filename,
    const uint32_t block_size )
	: caba::BaseModule(name),
	  m_vci_target_fsm(p_vci_target, mt.getSegmentList(tgtid)),
	  m_vci_init_fsm(p_vci_initiator, mt.indexForId(srcid)),
      m_block_size(block_size),
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

