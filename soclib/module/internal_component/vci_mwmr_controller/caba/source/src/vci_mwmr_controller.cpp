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
 *
 * Based on previous works by Etienne Faure & Alain Greiner, 2005
 *
 * E. Faure: Communications matérielles-logicielles dans les systèmes
 * sur puce orientés télécommunications.  PhD thesis, UPMC, 2007
 */

#include "register.h"
#include "../include/vci_mwmr_controller.h"
#include "mwmr_controller.h"
#include "alloc_elems.h"

#ifndef MWMR_CONTROLLER_DEBUG
#define MWMR_CONTROLLER_DEBUG 0
#endif

namespace soclib { namespace caba {

using soclib::common::alloc_elems;
using soclib::common::dealloc_elems;

namespace Mwmr {
struct fifo_state_s {
	uint32_t status_address;
	uint32_t depth;
	uint32_t buffer_address;
	uint32_t lock_address;
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
	INIT_LOCK_TAKE_RAMLOCK,
	INIT_LOCK_TAKE_RAMLOCK_W,
	INIT_LOCK_TAKE_LL,
    INIT_LOCK_TAKE_LL_W,
	INIT_LOCK_TAKE_SC,
    INIT_LOCK_TAKE_SC_W,
	INIT_STATUS_READ_RPTR,
	INIT_STATUS_READ_WPTR,
	INIT_STATUS_READ_USAGE,
	INIT_DECIDE,
	INIT_DATA_WRITE,
	INIT_DATA_READ,
	INIT_STATUS_WRITE_RPTR,
	INIT_STATUS_WRITE_WPTR,
	INIT_STATUS_WRITE_USAGE,
	INIT_STATUS_WAIT,
	INIT_STATUS_WRITE_LOCK,
	INIT_STATUS_WRITE_RAMLOCK,
	INIT_DONE,
} InitFsmState;

static const char *init_states[] = {
	"INIT_IDLE",
	"INIT_LOCK_TAKE_RAMLOCK",
	"INIT_LOCK_TAKE_RAMLOCK_W",
	"INIT_LOCK_TAKE_LL",
    "INIT_LOCK_TAKE_LL_W",
	"INIT_LOCK_TAKE_SC",
    "INIT_LOCK_TAKE_SC_W",
	"INIT_STATUS_READ_RPTR",
	"INIT_STATUS_READ_WPTR",
	"INIT_STATUS_READ_USAGE",
	"INIT_DECIDE",
	"INIT_DATA_WRITE",
	"INIT_DATA_READ",
	"INIT_STATUS_WRITE_RPTR",
	"INIT_STATUS_WRITE_WPTR",
	"INIT_STATUS_WRITE_USAGE",
	"INIT_STATUS_WAIT",
	"INIT_STATUS_WRITE_LOCK",
	"INIT_STATUS_WRITE_RAMLOCK",
	"INIT_DONE",
};

typedef enum {
    RSP_IDLE,
    RSP_STATUS_READ_RPTR,
    RSP_STATUS_READ_WPTR,
    RSP_STATUS_READ_USAGE,
    RSP_DATA_READ_W,
    RSP_DATA_WRITE_W,
    RSP_STATUS_WAIT,
} RspFsmState;

static const char *rsp_states[] = {
    "RSP_IDLE",
    "RSP_STATUS_READ_RPTR",
    "RSP_STATUS_READ_WPTR",
    "RSP_STATUS_READ_USAGE",
    "RSP_DATA_READ_W",
    "RSP_DATA_WRITE_W",
    "RSP_STATUS_WAIT",
};

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
#if 0
		if ( st->timer == 0 && st->running &&
			 (( st->way == MWMR_TO_COPROC )
			  ? (st->fifo->empty())
			  : (st->fifo->full())
				 ) ) {
			m_current = st;
			m_current_no = i;
			return;
		}
#else
		if ( st->timer == 0 && st->running &&
			 (( st->way == MWMR_TO_COPROC )
			  ? (st->fifo->empty())
			  : (st->fifo->full())
				 ) ) {
			m_current = st;
			m_current_no = i;
			return;
		}
#endif
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
        r_pending_reset = true;
		return true;
	case MWMR_CONFIG_FIFO_WAY:
		m_config_way = data == MWMR_FROM_COPROC ? MWMR_FROM_COPROC : MWMR_TO_COPROC;
		rehashConfigFifo();
		return true;
	case MWMR_CONFIG_FIFO_NO:
		m_config_no = data;
		rehashConfigFifo();
		return true;
    case MWMR_CONFIG_STATUS_ADDR:
		check_fifo();
		m_config_fifo->status_address = data;
		return true;
    case MWMR_CONFIG_DEPTH:
		check_fifo();
		m_config_fifo->depth = data;
		return true;
    case MWMR_CONFIG_BUFFER_ADDR:
		check_fifo();
		m_config_fifo->buffer_address = data;
		return true;
    case MWMR_CONFIG_LOCK_ADDR:
        if ( m_use_llsc )
            assert(!"You must not configure lock address with a LL-SC protocol, looks like a protocol mismatch");
		check_fifo();
		m_config_fifo->lock_address = data;
		return true;
    case MWMR_CONFIG_RUNNING:
		check_fifo();
        if ( m_use_llsc )
            assert(m_config_fifo->lock_address==0 && "You must not configure lock address with a LL-SC protocol, looks like a protocol mismatch");
		m_config_fifo->running = !!data;
		return true;
	}
	return false;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    uint32_t cell = (int)addr / vci_param::B;

	if ( cell < m_n_status ) {
		data = p_status[cell].read();
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
    case MWMR_CONFIG_STATUS_ADDR:
		check_fifo();
		data = m_config_fifo->status_address;
		return true;
    case MWMR_CONFIG_DEPTH:
		check_fifo();
		data = m_config_fifo->depth;
		return true;
    case MWMR_CONFIG_BUFFER_ADDR:
		check_fifo();
		data = m_config_fifo->buffer_address;
		return true;
    case MWMR_CONFIG_LOCK_ADDR:
        if ( m_use_llsc )
            assert(!"You must not read lock address with a LL-SC protocol, looks like a protocol mismatch");
		check_fifo();
		data = m_config_fifo->lock_address;
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
        r_pending_reset = false;
		m_vci_target_fsm.reset();
        r_init_fsm = INIT_IDLE;
        r_rsp_fsm = RSP_IDLE;
		reset();
		return;
	}

    bool current_fifo_get = false;
    bool current_fifo_put = false;

	m_vci_target_fsm.transition();

	for ( size_t i=0; i<m_n_all; ++i )
		if ( m_all_state[i].timer )
			m_all_state[i].timer--;

#if MWMR_CONTROLLER_DEBUG
    if ((InitFsmState)r_init_fsm.read() != INIT_IDLE || r_rsp_fsm.read() != RSP_IDLE) {
        std::cout << name() << " init: " << init_states[r_init_fsm.read()] << " rsp: " << rsp_states[r_rsp_fsm.read()] << std::endl;
    }
#endif

	switch ((InitFsmState)r_init_fsm.read()) {
	case INIT_IDLE:
		if ( m_current )
			r_init_fsm = m_use_llsc ? INIT_LOCK_TAKE_LL : INIT_LOCK_TAKE_RAMLOCK;
		else {
            if ( r_pending_reset ) {
                reset();
                break;
            }
			elect();
        }
		break;

	case INIT_STATUS_WRITE_RPTR:
	case INIT_STATUS_READ_RPTR:
	case INIT_LOCK_TAKE_LL:
	case INIT_LOCK_TAKE_RAMLOCK:
	case INIT_LOCK_TAKE_SC:
	case INIT_STATUS_READ_WPTR:
	case INIT_STATUS_READ_USAGE:
	case INIT_STATUS_WRITE_WPTR:
		if ( p_vci_initiator.cmdack.read() )
			// Select next state...
			r_init_fsm = r_init_fsm+1;
		break;

	case INIT_STATUS_WRITE_USAGE:
        if ( p_vci_initiator.cmdack.read() ) {
            if ( m_use_llsc )
                r_init_fsm = INIT_STATUS_WRITE_LOCK;
            else
                r_init_fsm = INIT_STATUS_WAIT;
        }
        break;

	case INIT_STATUS_WAIT:
		if ( p_vci_initiator.cmdack.read() )
			r_init_fsm = INIT_STATUS_WRITE_RAMLOCK;
		break;

	case INIT_STATUS_WRITE_LOCK:
	case INIT_STATUS_WRITE_RAMLOCK:
		if ( p_vci_initiator.cmdack.read() )
			r_init_fsm = INIT_DONE;
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
        r_status_modified = false;
		r_init_fsm = ( p_vci_initiator.rdata.read() == 0 )
			? INIT_STATUS_READ_RPTR
			: INIT_DONE;
		break;
	case INIT_LOCK_TAKE_RAMLOCK_W:
		if ( !p_vci_initiator.rspval.read() )
			break;
        r_status_modified = false;
		r_init_fsm = ( p_vci_initiator.rdata.read() == 0 )
			? INIT_STATUS_READ_RPTR
			: INIT_LOCK_TAKE_RAMLOCK;
		break;

	case INIT_DECIDE:
        if (r_rsp_fsm.read() != RSP_IDLE)
            break;
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " deciding for " << m_current_no
                  << ", status: " << r_current_usage.read()
                  << ", rptr: " << r_current_rptr.read()
                  << ", wptr: " << r_current_wptr.read()
                  << ", fifo full: " << m_current->fifo->full()
                  << ", fifo empty: " << m_current->fifo->empty()
                  << ": ";
#endif
		if ( m_current->way == MWMR_FROM_COPROC ) {
			if ( r_current_usage + m_fifo_from_coproc_depth <= m_current->depth &&
                 m_current->fifo->full() ) {
				r_cmd_count = m_fifo_from_coproc_depth/vci_param::B;
                r_rsp_count = m_fifo_from_coproc_depth/vci_param::B;
				r_init_fsm = INIT_DATA_WRITE;
                r_status_modified = true;
#if MWMR_CONTROLLER_DEBUG
                std::cout << "going to read from coproc " << m_fifo_from_coproc_depth/vci_param::B << " words" << std::endl;
#endif
                break;
			}
		} else {
			if ( r_current_usage >= m_fifo_to_coproc_depth &&
                 m_current->fifo->empty() ) {
				r_cmd_count = m_fifo_to_coproc_depth/vci_param::B;
                r_rsp_count = m_fifo_to_coproc_depth/vci_param::B;
				r_init_fsm = INIT_DATA_READ;
                r_status_modified = true;
#if MWMR_CONTROLLER_DEBUG
                std::cout << "going to put " << m_fifo_to_coproc_depth/vci_param::B << " words to coproc" << std::endl;
#endif
                break;
			}
		}
#if MWMR_CONTROLLER_DEBUG
        std::cout << "going to bail out: no room for transfer" << std::endl;
#endif
        if ( r_status_modified.read() ) {
            r_init_fsm = INIT_STATUS_WRITE_RPTR;
        } else {
            r_init_fsm = m_use_llsc ? INIT_STATUS_WRITE_LOCK : INIT_STATUS_WRITE_RAMLOCK;
        }
		break;
	case INIT_DATA_WRITE:
		if ( p_vci_initiator.cmdack.read() ) {
			if ( r_cmd_count == 1 )
				r_init_fsm = INIT_DECIDE;
			r_cmd_count = r_cmd_count-1;
            r_current_usage = r_current_usage+vci_param::B;
            r_current_wptr = (r_current_wptr + vci_param::B) % m_current->depth;
            current_fifo_get = true;
		}
		break;
	case INIT_DATA_READ:
		if ( p_vci_initiator.cmdack.read() ) {
			if ( r_cmd_count == 1 )
				r_init_fsm = INIT_DECIDE;
			r_cmd_count = r_cmd_count-1;
            r_current_usage = r_current_usage-vci_param::B;
            r_current_rptr = (r_current_rptr + vci_param::B) % m_current->depth;
		}
		break;

    case INIT_DONE:
        if ( r_rsp_fsm.read() != RSP_IDLE )
            break;
        m_current->timer = m_plaps;
        m_current = NULL;
        r_init_fsm = INIT_IDLE;
	}

    switch ((RspFsmState)r_rsp_fsm.read()) {
    case RSP_IDLE:
        switch ((InitFsmState)r_init_fsm.read()) {
        case INIT_STATUS_READ_RPTR:
            r_rsp_count = 3;
            r_rsp_fsm = RSP_STATUS_READ_RPTR;
            break;
        case INIT_DATA_WRITE:
            r_rsp_fsm = RSP_DATA_WRITE_W;
            break;
        case INIT_DATA_READ:
            r_rsp_fsm = RSP_DATA_READ_W;
            break;
        case INIT_STATUS_WRITE_RPTR:
            r_rsp_count = m_use_llsc ? 4 : 3;
            r_rsp_fsm = RSP_STATUS_WAIT;
            break;
        case INIT_STATUS_WRITE_RAMLOCK:
        case INIT_STATUS_WRITE_LOCK:
            r_rsp_count = 1;
            r_rsp_fsm = RSP_STATUS_WAIT;
            break;
        default:
            break;
        }
        break;

	case RSP_DATA_WRITE_W:
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " waiting for write " << r_rsp_count.read() << " words to go" << std::endl;
#endif
		if ( p_vci_initiator.rspval.read() ) {
			if ( r_rsp_count == 1 )
				r_rsp_fsm = RSP_IDLE;
			r_rsp_count = r_rsp_count-1;
		}
		break;
	case RSP_DATA_READ_W:
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " waiting for read " << r_rsp_count.read() << " words to go" << std::endl;
#endif
		if ( p_vci_initiator.rspval.read() ) {
            current_fifo_put = true;
			if ( r_rsp_count == 1 )
				r_rsp_fsm = RSP_IDLE;
			r_rsp_count = r_rsp_count-1;
		}
		break;
    case RSP_STATUS_READ_RPTR:
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " waiting for rptr" << std::endl;
#endif
        r_current_rptr = p_vci_initiator.rdata.read();
		if ( p_vci_initiator.rspval.read() )
            r_rsp_fsm = RSP_STATUS_READ_WPTR;
        break;
    case RSP_STATUS_READ_WPTR:
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " waiting for wptr" << std::endl;
#endif
        r_current_wptr = p_vci_initiator.rdata.read();
		if ( p_vci_initiator.rspval.read() )
            r_rsp_fsm = RSP_STATUS_READ_USAGE;
        break;
    case RSP_STATUS_READ_USAGE:
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " waiting for usage" << std::endl;
#endif
        r_current_usage = p_vci_initiator.rdata.read();
		if ( p_vci_initiator.rspval.read() )
            r_rsp_fsm = RSP_IDLE;
        break;
    case RSP_STATUS_WAIT:
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " waiting for status write " << r_rsp_count.read() << " words to go" << std::endl;
#endif
		if ( p_vci_initiator.rspval.read() ) {
			if ( r_rsp_count == 1 )
				r_rsp_fsm = RSP_IDLE;
			r_rsp_count = r_rsp_count-1;
		}
		break;
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
#if MWMR_CONTROLLER_DEBUG
        if (coproc_took_data)
            std::cout << name() << " put " << std::hex << p_to_coproc[i].data.read() << " to fifo" << std::endl;
#endif
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
	case INIT_LOCK_TAKE_RAMLOCK_W:
	case INIT_LOCK_TAKE_LL_W:
	case INIT_STATUS_WAIT:
	case INIT_LOCK_TAKE_SC_W:
	case INIT_IDLE:
	case INIT_DECIDE:
    case INIT_DONE:
		p_vci_initiator.cmdval = false;
		break;
	case INIT_LOCK_TAKE_RAMLOCK:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->lock_address;
		p_vci_initiator.cmd = vci_param::CMD_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_LOCK_TAKE_LL:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address+3*vci_param::B;
		p_vci_initiator.cmd = vci_param::CMD_LOCKED_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_LOCK_TAKE_SC:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.wdata = 1;
		p_vci_initiator.address = m_current->status_address+3*vci_param::B;
		p_vci_initiator.cmd = vci_param::CMD_STORE_COND;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_STATUS_READ_RPTR:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address;
		p_vci_initiator.cmd = vci_param::CMD_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = false;
		break;
	case INIT_STATUS_READ_WPTR:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address+vci_param::B;
		p_vci_initiator.cmd = vci_param::CMD_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = false;
		break;
	case INIT_STATUS_READ_USAGE:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address+vci_param::B*2;
		p_vci_initiator.cmd = vci_param::CMD_READ;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_DATA_WRITE:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->buffer_address + r_current_wptr;
		p_vci_initiator.wdata = m_current->fifo->read();
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " putting @" << (m_current->buffer_address + r_current_wptr) << ": " << m_current->fifo->read() << " on VCI" << std::endl;
#endif
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = (r_cmd_count==1);
		break;
	case INIT_DATA_READ:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->buffer_address + r_current_rptr;
		p_vci_initiator.cmd = vci_param::CMD_READ;
#if MWMR_CONTROLLER_DEBUG
        std::cout << name() << " reading data @" << (m_current->buffer_address + r_current_rptr) << std::endl;
#endif
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = (r_cmd_count==1);
		break;
	case INIT_STATUS_WRITE_RPTR:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address;
		p_vci_initiator.wdata = r_current_rptr.read();
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = false;
		break;
	case INIT_STATUS_WRITE_WPTR:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address+vci_param::B;
		p_vci_initiator.wdata = r_current_wptr.read();
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = false;
		break;
	case INIT_STATUS_WRITE_USAGE:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address+vci_param::B*2;
		p_vci_initiator.wdata = r_current_usage.read();
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = ! m_use_llsc;
		break;
	case INIT_STATUS_WRITE_LOCK:
		p_vci_initiator.cmdval = true;
		p_vci_initiator.address = m_current->status_address+vci_param::B*3;
		p_vci_initiator.wdata = 0;
		p_vci_initiator.cmd = vci_param::CMD_WRITE;
		p_vci_initiator.be = 0xf;
		p_vci_initiator.eop = true;
		break;
	case INIT_STATUS_WRITE_RAMLOCK:
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
    }
}

tmpl(/**/)::VciMwmrController(
    sc_module_name name,
    const MappingTable &mt,
    const IntTab &srcid,
    const IntTab &tgtid,
	const size_t plaps,
	const size_t fifo_to_coproc_depth,
	const size_t fifo_from_coproc_depth,
	const size_t n_to_coproc,
	const size_t n_from_coproc,
	const size_t n_config,
	const size_t n_status,
    const bool use_llsc )
		   : caba::BaseModule(name),
		   m_vci_target_fsm(p_vci_target, mt.getSegmentList(tgtid), 1),
           m_ident(mt.indexForId(srcid)),
           m_fifo_to_coproc_depth(fifo_to_coproc_depth),
           m_fifo_from_coproc_depth(fifo_from_coproc_depth),
           m_plaps(plaps),
           m_n_to_coproc(n_to_coproc),
           m_n_from_coproc(n_from_coproc),
           m_n_all(n_to_coproc+n_from_coproc),
           m_n_config(n_config),
           m_n_status(n_status),
           m_use_llsc(use_llsc),
           m_all_state(new fifo_state_t[m_n_all]),
           m_to_coproc_state(m_all_state),
           m_from_coproc_state(&m_all_state[m_n_to_coproc]),
           r_config(alloc_elems<sc_signal<uint32_t> >("r_config", n_config)),
           r_init_fsm("init_fsm"),
           r_rsp_fsm("rsp_fsm"),
           r_cmd_count("cmd_count"),
           r_rsp_count("rsp_count"),
           r_current_rptr("current_rptr"),
           r_current_wptr("current_wptr"),
           r_current_usage("current_usage"),
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

    memset(m_all_state, 0, sizeof(*m_all_state)*m_n_all);

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
        m_from_coproc_state[i].fifo = new GenericFifo<uint32_t>(o.str(), m_fifo_from_coproc_depth);
    }
    for ( size_t i = 0; i<m_n_to_coproc; ++i ) {
        std::ostringstream o;
        o << "fifo_to_coproc[" << i << "]";
        m_to_coproc_state[i].way = MWMR_TO_COPROC;
        m_to_coproc_state[i].fifo = new GenericFifo<uint32_t>(o.str(), m_fifo_to_coproc_depth);
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

