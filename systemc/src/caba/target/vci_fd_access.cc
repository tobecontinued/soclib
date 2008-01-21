/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 */

#include "common/register.h"
#include "caba/target/vci_fd_access.h"
#include <fcntl.h>
#include <errno.h>
#define SYSTEMC_SOURCE
#include "soclib/fd_access.h"
#undef SYSTEMC_SOURCE

namespace soclib { namespace caba {

namespace {

inline int soclib_modes_to_os(int soclib_mode)
{
    int ret = 0;
#define do_mode(y, x) if ( soclib_mode & (y) ) ret |= (x)
	do_mode(FD_ACCESS_O_RDONLY, O_RDONLY);
	do_mode(FD_ACCESS_O_WRONLY, O_WRONLY);
	do_mode(FD_ACCESS_O_RDWR, O_RDWR);
	do_mode(FD_ACCESS_O_CREAT, O_CREAT);
	do_mode(FD_ACCESS_O_EXCL, O_EXCL);
	do_mode(FD_ACCESS_O_NOCTTY, O_NOCTTY);
	do_mode(FD_ACCESS_O_TRUNC, O_TRUNC);
	do_mode(FD_ACCESS_O_APPEND, O_APPEND);
	do_mode(FD_ACCESS_O_NONBLOCK, O_NONBLOCK);
#ifdef O_SYNC
	do_mode(FD_ACCESS_O_SYNC, O_SYNC);
#endif
//	do_mode(FD_ACCESS_O_DIRECT, O_DIRECT);
//	do_mode(FD_ACCESS_O_LARGEFILE, O_LARGEFILE);
#ifdef O_DIRECTORY
	do_mode(FD_ACCESS_O_DIRECTORY, O_DIRECTORY);
#endif
	do_mode(FD_ACCESS_O_NOFOLLOW, O_NOFOLLOW);
//	do_mode(FD_ACCESS_O_NOATIME, O_NOATIME);
	do_mode(FD_ACCESS_O_NDELAY, O_NDELAY);
#undef do_mode
    return ret;
}

}

#define tmpl(t) template<typename vci_param> t VciFdAccess<vci_param>

tmpl(void)::ended()
{
	if ( m_irq_enabled )
		r_irq = true;
	m_current_op = m_op = FD_ACCESS_NOOP;
}

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;

	switch ((enum SoclibFdAccessRegisters)cell) {
	case FD_ACCESS_FD:
		m_fd = data;
		return true;
    case FD_ACCESS_BUFFER:
		m_buffer = data;
		return true;
    case FD_ACCESS_SIZE:
		m_size = data;
		return true;
    case FD_ACCESS_HOW:
		m_how = data;
		return true;
    case FD_ACCESS_WHENCE:
		m_whence = data;
		return true;
    case FD_ACCESS_OP:
		m_op = data;
		return true;
    case FD_ACCESS_IRQ_ENABLE:
		m_irq_enabled = data;
		return true;
    case FD_ACCESS_RETVAL:
    case FD_ACCESS_ERRNO:
		return false;
	};
	return false;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    int cell = (int)addr / vci_param::B;

	switch (cell) {
	case FD_ACCESS_FD:
		data = m_fd;
		return true;
    case FD_ACCESS_BUFFER:
		data = m_buffer;
		return true;
    case FD_ACCESS_SIZE:
		data = m_size;
		return true;
    case FD_ACCESS_HOW:
		data = m_how;
		return true;
    case FD_ACCESS_WHENCE:
		data = m_whence;
		return true;
    case FD_ACCESS_OP:
		data = m_current_op;
		return true;
    case FD_ACCESS_IRQ_ENABLE:
		data = r_irq;
		return true;
    case FD_ACCESS_RETVAL:
		data = m_retval;
		return true;
    case FD_ACCESS_ERRNO:
		data = m_errno;
		return true;
	}
	return false;
}

tmpl(void)::read_done( req_t *req )
{
	if ( req->failed() ) {
		m_retval = -1;
		m_errno = EINVAL;
	}
    delete m_data;
	delete req;
	ended();
}

tmpl(void)::write_finish( req_t *req )
{
	if ( req->failed() ) {
		m_retval = -1;
		m_errno = EINVAL;
	} else {
		m_retval = ::write( m_fd, (char *)m_data, m_size );
		m_errno = errno;
	}
    delete m_data;
	delete req;
	ended();
}

tmpl(void)::open_finish( req_t *req )
{
	if ( req->failed() ) {
		m_retval = -1;
		m_errno = EINVAL;
	} else {
        int how = soclib_modes_to_os(m_how);
		m_retval = ::open( (char *)m_data, how, m_whence );
		m_errno = errno;
	}
    delete m_data;
	delete req;
	ended();
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_target_fsm.reset();
		m_vci_init_fsm.reset();
		r_irq = false;
		m_irq_enabled = false;
		return;
	}

	if ( m_current_op == FD_ACCESS_NOOP &&
		 m_op != FD_ACCESS_NOOP ) {
		m_current_op = m_op;
        m_op = FD_ACCESS_NOOP;
		switch ((enum SoclibFdOp)m_current_op) {
        case FD_ACCESS_NOOP:
            assert(0&&"This is most unprobabilistic...");
		case FD_ACCESS_OPEN:
		{
			m_data = new uint8_t[m_size+1];
			VciInitSimpleReadReq<vci_param> *req =
				new VciInitSimpleReadReq<vci_param>(
					m_data, m_buffer, m_size+1 );
			req->setDone( this, ON_T(open_finish) );
			m_vci_init_fsm.doReq( req );
			break;
		}
		case FD_ACCESS_CLOSE:
			m_retval = ::close(m_fd);
			m_errno = errno;
			ended();
			break;
		case FD_ACCESS_READ:
			m_data = new uint8_t[m_size];
			m_retval = ::read(m_fd, m_data, m_size);
			m_errno = errno;
			if ( m_retval > 0 ) {
				VciInitSimpleWriteReq<vci_param> *req =
					new VciInitSimpleWriteReq<vci_param>(
						m_buffer, m_data, m_size );
				req->setDone( this, ON_T(read_done) );
				m_vci_init_fsm.doReq( req );
			} else {
				delete [] m_data;
				ended();
			}
			break;
		case FD_ACCESS_WRITE:
		{
			m_data = new uint8_t[m_size];
			VciInitSimpleReadReq<vci_param> *req =
				new VciInitSimpleReadReq<vci_param>(
					m_data, m_buffer, m_size );
			req->setDone( this, ON_T(write_finish) );
			m_vci_init_fsm.doReq( req );
			break;
		}
		case FD_ACCESS_LSEEK:
			m_retval = ::lseek(m_fd, m_size, m_whence);
			m_errno = errno;
			ended();
			break;
		}
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

tmpl(/**/)::VciFdAccess(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt)
	: caba::BaseModule(name),
	  m_vci_target_fsm(p_vci_target, mt.getSegmentList(index)),
	  m_vci_init_fsm(p_vci_initiator, mt.indexForId(index)),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci_target("vci_target"),
      p_vci_initiator("vci_initiator"),
      p_irq("irq")
{
	m_vci_target_fsm.on_read_write(on_read, on_write);

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();

    portRegister("clk", p_clk);
    portRegister("resetn", p_resetn);
    portRegister("vci_target", p_vci_target);
    portRegister("vci_initiator", p_vci_initiator);
    portRegister("irq", p_irq);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

