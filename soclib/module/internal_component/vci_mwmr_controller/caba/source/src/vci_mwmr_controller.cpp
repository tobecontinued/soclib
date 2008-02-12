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
 *         Nicolas Pouillon <nipo@ssji.net>, 2008
 */

#include "register.h"
#include "../include/vci_mwmr_controller.h"
#include "mwmr_controller.h"

#define MWMR_CONTROLLER_DEBUG 0

namespace soclib { namespace caba {

namespace Mwmr {
struct fifo_state_s {
	uint32_t state_address;
	uint32_t offset_address;
	uint32_t lock_address;
	uint32_t depth;
	uint32_t base_address;
	uint32_t burst_size;
	bool running;
	enum SoclibMwmrWay way;
	uint32_t timer;
    GenericFifo<uint32_t> *fifo;
};
}

#define tmpl(t) template<typename vci_param> t VciMwmrController<vci_param>

#define check_fifo() if ( ! m_config_fifo ) return false

typedef enum {
	INIT_IDLE,
	INIT_LOCK_TAKE_LL,
	INIT_LOCK_TAKE_LL_W,
	INIT_LOCK_TAKE_SC,
	INIT_LOCK_TAKE_SC_W,
	INIT_STATUS_READ,
	INIT_STATUS_READ_W,
	INIT_OFFSET_READ,
	INIT_OFFSET_READ_W,
	INIT_DECIDE,
	INIT_DATA_WRITE,
	INIT_DATA_WRITE_W,
	INIT_DATA_READ,
	INIT_DATA_READ_W,
	INIT_OFFSET_WRITE,
	INIT_OFFSET_WRITE_W,
	INIT_STATUS_WRITE,
	INIT_STATUS_WRITE_W,
	INIT_LOCK_RELEASE,
	INIT_LOCK_RELEASE_W,
	INIT_DONE,
} InitFsmState;

tmpl(void)::rehashConfigFifo()
{
	fifo_state_t *base =
		(m_config_way == MWMR_TO_COPROC)
		? m_to_coproc_state
		: m_from_coproc_state;
	size_t max_no =
		(m_config_way == MWMR_TO_COPROC)
		? m_n_to_coproc
		: m_n_from_coproc;
	if ( m_config_no < max_no )
		m_config_fifo = &base[m_config_no];
	else
		m_config_fifo = NULL;
}

tmpl(void)::elect()
{
	for ( size_t _i=0; _i<m_n_all; ++_i ) {
		size_t i = (m_current_no+_i)%m_n_all;
		fifo_state_t *st = &m_all_state[i];
		if ( st->timer == 0 && st->running &&
			 (( st->way == MWMR_TO_COPROC )
			  ? (st->fifo->filled_status() + st->burst_size < m_fifo_depth)
			  : (st->fifo->filled_status() >= st->burst_size)
				 ) ) {
			m_current = st;
			m_current_no = i;
			return;
		}
	}
}

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    uint32_t cell = (int)addr / vci_param::B;

	if ( cell < m_n_config ) {
		p_config[cell] = data;
		return true;
	}

#if MWMR_CONTROLLER_DEBUG
    std::cout << name() << " on write cell " << cell << " data: " << std::hex << data << std::endl;
#endif

	switch ((enum SoclibMwmrRegisters)cell) {
	case MWMR_RESET:
		reset();
		return true;
	case MWMR_CONFIG_FIFO_WAY:
		m_config_way = data == MWMR_FROM_COPROC ? MWMR_FROM_COPROC : MWMR_TO_COPROC;
		rehashConfigFifo();
		return true;
	case MWMR_CONFIG_FIFO_NO:
		m_config_no = data;
		rehashConfigFifo();
		return true;
    case MWMR_CONFIG_STATE_ADDR:
		check_fifo();
		m_config_fifo->state_address = data;
		return true;
    case MWMR_CONFIG_OFFSET_ADDR:
		check_fifo();
		m_config_fifo->offset_address = data;
		return true;
    case MWMR_CONFIG_LOCK_ADDR:
		check_fifo();
		m_config_fifo->lock_address = data;
		return true;
    case MWMR_CONFIG_DEPTH:
		check_fifo();
		m_config_fifo->depth = data;
		return true;
    case MWMR_CONFIG_WIDTH:
		check_fifo();
		m_config_fifo->burst_size = data;
		return true;
    case MWMR_CONFIG_BASE_ADDR:
		check_fifo();
		m_config_fifo->base_address = data;
		return true;
    case MWMR_CONFIG_RUNNING:
		check_fifo();
		m_config_fifo->running = !!data;
		return true;
	}
	return false;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    uint32_t cell = (int)addr / vci_param::B;

	if ( cell < m_n_status ) {
		data = p_config[cell].read();
		return true;
	}

#if MWMR_CONTROLLER_DEBUG
    std::cout << name() << " on read cell " << cell << std::endl;
#endif

	switch ((enum SoclibMwmrRegisters)cell) {
	case MWMR_RESET:
		return false;
	case MWMR_CONFIG_FIFO_WAY:
		data = m_config_way;
		return true;
	case MWMR_CONFIG_FIFO_NO:
		data = m_config_no;
		return true;
    case MWMR_CONFIG_STATE_ADDR:
		check_fifo();
		data = m_config_fifo->state_address;
		return true;
    case MWMR_CONFIG_OFFSET_ADDR:
		check_fifo();
		data = m_config_fifo->offset_address;
		return true;
    case MWMR_CONFIG_LOCK_ADDR:
		check_fifo();
		data = m_config_fifo->lock_address;
		return true;
    case MWMR_CONFIG_DEPTH:
		check_fifo();
		data = m_config_fifo->depth;
		return true;
    case MWMR_CONFIG_WIDTH:
		check_fifo();
		data = m_config_fifo->burst_size;
		return true;
    case MWMR_CONFIG_BASE_ADDR:
		check_fifo();
		data = m_config_fifo->base_address;
		return true;
    case MWMR_CONFIG_RUNNING:
		check_fifo();
		data = m_config_fifo->running;
		return true;
	}
	return false;
}

tmpl(void)::reset()
{
	m_current = NULL;
	m_config_way = MWMR_TO_COPROC;
	m_config_no = 0;
	m_config_fifo = NULL;

	for ( size_t i=0; i<m_n_all; ++i ) {
		m_all_state[i].running = false;
		m_all_state[i].timer = 0;
	}
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_target_fsm.reset();
        r_init_fsm = INIT_IDLE;
		reset();
		return;
	}

    bool current_fifo_get = false;
    bool current_fifo_put = false;

	m_vci_target_fsm.transition();

	for ( size_t i=0; i<m_n_all; ++i )
		if ( m_all_state[i].timer )
			m_all_state[i].timer--;

	switch ((InitFsmState)r_init_fsm.read()) {
	case INIT_IDLE:
		if ( m_current )
			r_init_fsm = INIT_LOCK_TAKE_LL;
		else
			elect();
		break;

	case INIT_LOCK_TAKE_LL:
	case INIT_LOCK_TAKE_SC:
	case INIT_STATUS_READ:
	case INIT_OFFSET_READ:
	case INIT_STATUS_WRITE:
	case INIT_OFFSET_WRITE:
	case INIT_LOCK_RELEASE:
		if ( p_vci_initiator.cmdack.read() )
			// Select next state...
			r_init_fsm = r_init_fsm+1;
		break;
		
	case INIT_LOCK_TAKE_LL_W:
		if ( !p_vci_initiator.rspval.read() )
			break;
		r_init_fsm = ( p_vci_initiator.rdata.read() == 0 )
			? INIT_LOCK_TAKE_SC
			: INIT_LOCK_TAKE_LL;
		break;
	case INIT_LOCK_TAKE_SC_W:
		if ( !p_vci_initiator.rspval.read() )
			break;
		r_init_fsm = ( p_vci_initiator.rdata.read() == 0 )
			? INIT_STATUS_READ
			: INIT_DONE;
		break;

	case INIT_STATUS_READ_W:
		if ( !p_vci_initiator.rspval.read() )
			break;
		r_current_status = p_vci_initiator.rdata.read();
		r_init_fsm = INIT_OFFSET_READ;
		break;
	case INIT_OFFSET_READ_W:
		if ( !p_vci_initiator.rspval.read() )
			break;
		r_current_offset = p_vci_initiator.rdata.read();
		r_init_fsm = INIT_DECIDE;
		break;

	case INIT_DECIDE:
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " deciding for " << m_current_no
                  << ", status: " << r_current_status.read()
                  << ", offset: " << r_current_offset.read()
                  << ": " << std::endl;
#endif
		if ( m_current->way == MWMR_FROM_COPROC ) {
			if ( r_current_status + m_current->burst_size <= m_current->depth ) {
				r_cmd_count = m_current->burst_size;
                r_rsp_count = m_current->burst_size;
				r_init_fsm = INIT_DATA_WRITE;
#if MWMR_CONTROLLER_DEBUG
                std::cout << name() << " goint to read from coproc " << m_current->burst_size << " words" << std::endl;
#endif
			}
		} else {
			if ( r_current_status >= m_current->burst_size ) {
				r_cmd_count = m_current->burst_size;
                r_rsp_count = m_current->burst_size;
				r_init_fsm = INIT_DATA_READ;
#if MWMR_CONTROLLER_DEBUG
                std::cout << name() << " goint to put " << m_current->burst_size << " words to coproc" << std::endl;
#endif
			}
		}
		break;
	case INIT_DATA_WRITE:
		if ( p_vci_initiator.cmdack.read() ) {
			if ( r_cmd_count == 1 )
				r_init_fsm = INIT_DATA_WRITE_W;
			r_cmd_count = r_cmd_count-1;
            r_current_status = r_current_status+1;
            r_current_offset = (r_current_offset + 1) % m_current->depth;
            current_fifo_get = true;
		}
		if ( p_vci_initiator.rspval.read() )
			r_rsp_count = r_rsp_count-1;
		break;
	case INIT_DATA_READ:
		if ( p_vci_initiator.cmdack.read() ) {
			if ( r_cmd_count == 1 )
				r_init_fsm = INIT_DATA_READ_W;
			r_cmd_count = r_cmd_count-1;
            r_current_status = r_current_status-1;
            r_current_offset = (r_current_offset + 1) % m_current->depth;
		}
		if ( p_vci_initiator.rspval.read() ) {
            current_fifo_put = true;
			r_rsp_count = r_rsp_count-1;
        }
		break;
	case INIT_DATA_WRITE_W:
		if ( p_vci_initiator.rspval.read() ) {
			if ( r_rsp_count == 1 )
				r_init_fsm = INIT_OFFSET_WRITE;
			r_rsp_count = r_rsp_count-1;
		}
		break;
	case INIT_DATA_READ_W:
		if ( p_vci_initiator.rspval.read() ) {
            current_fifo_put = true;
			if ( r_rsp_count == 1 )
				r_init_fsm = INIT_OFFSET_WRITE;
			r_rsp_count = r_rsp_count-1;
		}
		break;

	case INIT_LOCK_RELEASE_W:
	case INIT_STATUS_WRITE_W:
	case INIT_OFFSET_WRITE_W:
		if ( p_vci_initiator.rspval.read() )
			r_init_fsm = r_init_fsm+1;
		break;
    case INIT_DONE:
        m_current->timer = m_plaps;
        m_current = NULL;
        r_init_fsm = INIT_IDLE;
	}

    for ( size_t i = 0; i<m_n_from_coproc; ++i ) {
        fifo_state_t *st = &m_from_coproc_state[i];
        bool coproc_sent_data = p_from_coproc[i].r.read() && p_from_coproc[i].rok.read();
        bool vci_took_data = false;
        if ( st == m_current )
            vci_took_data = current_fifo_get;
        if ( coproc_sent_data ) {
#if MWMR_CONTROLLER_DEBUG
            std::cout << name() << " getting " << std::hex << p_from_coproc[i].data.read() << " from coproc" << std::endl;
#endif
            if ( vci_took_data )
                st->fifo->put_and_get(p_from_coproc[i].data.read());
            else
                st->fifo->simple_put(p_from_coproc[i].data.read());
        } else {
            if ( vci_took_data )
                st->fifo->simple_get();
        }
    }
    for ( size_t i = 0; i<m_n_to_coproc; ++i ) {
        fifo_state_t *st = &m_to_coproc_state[i];
        bool coproc_took_data = p_to_coproc[i].w.read() && p_to_coproc[i].wok.read();
        bool vci_gave_data = false;
        if ( st == m_current )
            vci_gave_data = current_fifo_put;
        if ( vci_gave_data ) {
#if MWMR_CONTROLLER_DEBUG
            std::cout << name() << " getting " << std::hex << p_vci_initiator.rdata.read() << " from vci" << std::endl;
#endif
            if ( coproc_took_data )
                st->fifo->put_and_get(p_vci_initiator.rdata.read());
            else
                st->fifo->simple_put(p_vci_initiator.rdata.read());
        } else {
            if ( coproc_took_data )
                st->fifo->simple_get();
        }
    }
}

tmpl(void)::genMoore()
{
	m_vci_target_fsm.genMoore();

	p_vci_initiator.rspack = true;

	switch ((InitFsmState)r_init_fsm.read()) {
	case INIT_LOCK_TAKE_LL_W:
	case INIT_LOCK_TAKE_SC_W:
	case INIT_STATUS_READ_W:
	case INIT_OFFSET_READ_W:
	case INIT_STATUS_WRITE_W:
	case INIT_OFFSET_WRITE_W:
	case INIT_LOCK_RELEASE_W:
	case INIT_IDLE:
	case INIT_DECIDE:
	case INIT_DATA_READ_W:
	case INIT_DATA_WRITE_W:
    case INIT_DONE:
		p_vci_initiator.cmdval = false;
		break;
	case INIT_LOCK_TAKE_LL:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->lock_address;
		p_vci_initiator.cmd = vci_param::CMD_LOCKED_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_LOCK_TAKE_SC:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.wdata = 1;
		p_vci_initiator.address = m_current->lock_address;
		p_vci_initiator.cmd = vci_param::CMD_STORE_COND;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_STATUS_READ:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->state_address;
		p_vci_initiator.cmd = vci_param::CMD_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_OFFSET_READ:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->offset_address;
		p_vci_initiator.cmd = vci_param::CMD_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_DATA_WRITE:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->base_address + r_current_offset*4;
		p_vci_initiator.wdata = m_current->fifo->read();
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " putting @" << (m_current->base_address + r_current_offset*4) << ": " << m_current->fifo->read() << " on VCI" << std::endl;
#endif
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = (r_cmd_count==1);
		break;
	case INIT_DATA_READ:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->base_address + r_current_offset*4;
		p_vci_initiator.cmd = vci_param::CMD_READ;
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " reading data @" << (m_current->base_address + r_current_offset*4) << std::endl;
#endif
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = (r_cmd_count==1);
		break;
	case INIT_OFFSET_WRITE:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->offset_address;
		p_vci_initiator.wdata = r_current_offset.read();
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_STATUS_WRITE:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->state_address;
		p_vci_initiator.wdata = r_current_status.read();
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_LOCK_RELEASE:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->lock_address;
		p_vci_initiator.wdata = 0;
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	}
	p_vci_initiator.contig = true;
	p_vci_initiator.cons = 1;
	p_vci_initiator.plen = 0;
	p_vci_initiator.wrap = 0;
	p_vci_initiator.cfixed = true;
	p_vci_initiator.clen = 0;
	p_vci_initiator.srcid = m_ident;
	p_vci_initiator.trdid = 0;
	p_vci_initiator.pktid = 0;

    for ( size_t i = 0; i<m_n_from_coproc; ++i )
        p_from_coproc[i].r = m_from_coproc_state[i].fifo->wok();
    for ( size_t i = 0; i<m_n_to_coproc; ++i ) {
        p_to_coproc[i].w = m_to_coproc_state[i].fifo->rok();
        p_to_coproc[i].data = m_to_coproc_state[i].fifo->read();
#if MWMR_CONTROLLER_DEBUG
        if (m_to_coproc_state[i].fifo->rok())
            std::cout << name() << " putting " << std::hex << m_to_coproc_state[i].fifo->read() << " to fifo" << std::endl;
#endif
    }
}

template<typename elem_t>
elem_t *alloc_elems(const std::string &prefix, size_t n)
{
	elem_t *elem = (elem_t*)malloc(sizeof(elem_t)*n);
	for ( size_t i=0; i<n; ++i ) {
		std::ostringstream o;
		o << prefix << "[" << i << "]";
		new(&elem[i]) elem_t(o.str().c_str());
	}
	return elem;
}

template<typename elem_t>
void dealloc_elems(elem_t *elems, size_t n)
{
	for ( size_t i = 0; i<n; ++i )
		elems[i].~elem_t();
	free(elems);
}

tmpl(/**/)::VciMwmrController(
    sc_module_name name,
    const IntTab &index,
    const MappingTable &mt,
	const size_t plaps,
	const size_t fifo_depth,
	const size_t n_to_coproc,
	const size_t n_from_coproc,
	const size_t n_config,
	const size_t n_status )
		   : caba::BaseModule(name),
		   m_vci_target_fsm(p_vci_target, mt.getSegmentList(index), 1),
           m_ident(mt.indexForId(index)),
           m_fifo_depth(fifo_depth),
           m_plaps(plaps),
           m_n_to_coproc(n_to_coproc),
           m_n_from_coproc(n_from_coproc),
           m_n_all(n_to_coproc+n_from_coproc),
           m_n_config(n_config),
           m_n_status(n_status),
           m_all_state(new fifo_state_t[m_n_all]),
           m_to_coproc_state(m_all_state),
           m_from_coproc_state(&m_all_state[m_n_to_coproc]),
           r_config(alloc_elems<sc_signal<uint32_t> >("r_config", n_config)),
           r_init_fsm("init_fsm"),
           r_cmd_count("cmd_count"),
           r_rsp_count("rsp_count"),
           r_current_offset("current_offset"),
           r_current_status("current_status"),
		   p_clk("clk"),
		   p_resetn("resetn"),
		   p_vci_target("vci_target"),
		   p_vci_initiator("vci_initiator"),
		   p_from_coproc(alloc_elems<FifoInput<uint32_t> >("from_coproc", n_from_coproc)),
		   p_to_coproc(alloc_elems<FifoOutput<uint32_t> >("to_coproc", n_to_coproc)),
		   p_config(alloc_elems<sc_out<uint32_t> >("config", n_config)),
		   p_status(alloc_elems<sc_in<uint32_t> >("status", n_status))
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
	portRegisterN("from_coproc", p_from_coproc, n_from_coproc);
	portRegisterN("to_coproc", p_to_coproc, n_to_coproc);
	portRegisterN("config", p_config, n_config);
	portRegisterN("status", p_status, n_status);

    for ( size_t i = 0; i<m_n_from_coproc; ++i ) {
        std::ostringstream o;
        o << "fifo_from_coproc[" << i << "]";
        m_from_coproc_state[i].way = MWMR_FROM_COPROC;
        m_from_coproc_state[i].fifo = new GenericFifo<uint32_t>(o.str(), m_fifo_depth);
    }
    for ( size_t i = 0; i<m_n_to_coproc; ++i ) {
        std::ostringstream o;
        o << "fifo_to_coproc[" << i << "]";
        m_to_coproc_state[i].way = MWMR_TO_COPROC;
        m_to_coproc_state[i].fifo = new GenericFifo<uint32_t>(o.str(), m_fifo_depth);
    }
    reset();
}

tmpl(/**/)::~VciMwmrController()
{
    for ( size_t i = 0; i<m_n_all; ++i )
        delete m_all_state[i].fifo;
    dealloc_elems(p_from_coproc, m_n_from_coproc); 
    dealloc_elems(p_to_coproc, m_n_to_coproc); 
    dealloc_elems(p_config, m_n_config);
    dealloc_elems(p_status, m_n_status);
    dealloc_elems(r_config, m_n_config);
    delete [] m_all_state;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

