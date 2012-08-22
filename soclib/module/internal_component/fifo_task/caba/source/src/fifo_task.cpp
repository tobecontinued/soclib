/* -*- c++ -*-
 *
 * DSX is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DSX is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DSX; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Nicolas Pouillon <nipo@ssji.net>, 2006
 */

#include "../include/fifo_task.h"

#include "alloc_elems.h"
#include "soclib_endian.h"
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef SOCLIB_MODULE_DEBUG
#define SOCLIB_MODULE_DEBUG 0
#endif

namespace dsx { namespace caba {

#define tmpl(...) __VA_ARGS__ FifoTask

tmpl(std::vector<std::string> )
::stringArray( const char *first, ... )
{
    std::vector<std::string> ret;
    va_list arg;
    va_start(arg, first);
    const char *s = first;
    while(s) {
        ret.push_back(std::string(s));
        s = va_arg(arg, const char *);
    };
    va_end(arg);
    return ret;
}

tmpl(std::vector<int>)
::intArray( const int length, ... )
{
    int i;
    std::vector<int> ret;
    va_list arg;
    va_start(arg, length);

    for (i=0; i<length; ++i) {
        ret.push_back(va_arg(arg, int));
    };
    va_end(arg);
    return ret;
}



tmpl(/**/)::~FifoTask()
{
	if ( m_buffer ) {
		delete [] m_buffer;
		m_buffer = NULL;
	}

	soclib::common::dealloc_elems(p_to_ctrl, m_n_fifo_to_ctrl);
	soclib::common::dealloc_elems(p_from_ctrl, m_n_fifo_from_ctrl);
}

tmpl(/**/)::FifoTask(
	sc_core::sc_module_name insname,
	const std::vector<std::string> &fifos_out,
	const std::vector<int> &fifos_out_width,
	const std::vector<std::string> &fifos_in,
	const std::vector<int> &fifos_in_width)
	: soclib::caba::BaseModule(insname),
	  m_n_fifo_to_ctrl(fifos_out.size()),
	  m_n_fifo_from_ctrl(fifos_in.size()),
	  p_clk("clk"),
	  p_resetn("resetn"),
	  p_to_ctrl(soclib::common::alloc_elems<soclib::caba::FifoOutput<uint32_t> >("to_ctrl", m_n_fifo_to_ctrl)),
	  p_from_ctrl(soclib::common::alloc_elems<soclib::caba::FifoInput<uint32_t> >("from_ctrl", m_n_fifo_from_ctrl)),
	  m_names_to_ctrl(fifos_out),
	  m_names_from_ctrl(fifos_in),
      m_state(SYNTHHOST_NONE),
	  m_buffer(NULL)
{

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();

    SC_CTHREAD(run, p_clk.pos());
    reset_signal_is(p_resetn, false);

    for(uint32_t i= 0; i< fifos_in.size(); i++)
        m_mwmrs.push_back(new srl_mwmr_s( fifos_in_width[i], fifos_in[i].c_str(), i, 1));
    
    for(uint32_t i= 0; i< fifos_out.size(); i++)
        m_mwmrs.push_back(new srl_mwmr_s( fifos_out_width[i], fifos_out[i].c_str(), i, 0));

    srl_log_open();
}


tmpl(void)::genMoore()
{
    assert(m_state != SYNTHHOST_NONE);

	for (size_t i=0; i<m_n_fifo_from_ctrl; ++i)
		p_from_ctrl[i].r = (
			m_state == SYNTHHOST_TRANS_TO_COPROC &&
			m_numeric_param == i );
	for (size_t i=0; i<m_n_fifo_to_ctrl; ++i) {
		if (
			m_state == SYNTHHOST_TRANS_FROM_COPROC &&
			m_numeric_param == i ) {
			p_to_ctrl[i].data = m_recv_buffer;
			p_to_ctrl[i].w = m_recv_buffer_valid;
		} else
			p_to_ctrl[i].w = false;
	}
}

tmpl(void)::run()
{

#if SOCLIB_MODULE_DEBUG
    std::cout << "RUN... " << std::endl;
#endif

	if ( ! p_resetn.read() ) 
    {
#if SOCLIB_MODULE_DEBUG
        std::cout << "RUN...reset! " << std::endl;
#endif
		m_numeric_param = 0;
		m_cycles_left = 0;
		m_state = SYNTHHOST_NONE;
		m_cycle = 0;
	}
   
    task_func(); 

}

//task code
tmpl(void)::task_func()
{
    srl_log_printf( NONE, "fifo_task: No function implemented?");
}


tmpl(void)::_prot_write( void *mem, size_t len, uint32_t id )
{
#if SOCLIB_MODULE_DEBUG
    std::cout << " prot write " << len << std::endl;
#endif
    assert(mem);
    uint32_t* mem_ptr = (uint32_t*) mem;

    m_state = SYNTHHOST_TRANS_FROM_COPROC;//so the genMoore send the appropriate signals
    m_cycles_left = len;
    m_numeric_param = id;
    m_recv_buffer_valid = false;
    m_buffer_ptr = 0;

    do{
        wait();
        m_cycle++;

		if ( p_to_ctrl[m_numeric_param].wok.read() && p_to_ctrl[m_numeric_param].w.read() ) {
			m_buffer_ptr++;
			m_recv_buffer_valid = false;
			--m_cycles_left;
			if ( m_cycles_left == 0 ) {
				m_state = SYNTHHOST_NONE;
			}
		}
		if ( ! m_recv_buffer_valid && m_cycles_left ) {
			m_recv_buffer = machine_to_le(mem_ptr[m_buffer_ptr]);
			m_recv_buffer_valid = true;
		}
    }while(m_state == SYNTHHOST_TRANS_FROM_COPROC);

    m_state = SYNTHHOST_NONE;
#if SOCLIB_MODULE_DEBUG
    std::cout << " prot write done " << len << std::endl;
#endif

}

tmpl(void)::_prot_read( void *mem, size_t len, uint32_t id )
{
#if SOCLIB_MODULE_DEBUG
    std::cout << " prot read " << len << std::endl;
#endif
    assert(mem);
    uint32_t* mem_ptr = (uint32_t*) mem;
		
	m_state         = SYNTHHOST_TRANS_TO_COPROC;
    m_cycles_left   = len;
    m_numeric_param = id;
    m_buffer_ptr    = 0;

    do{
	    wait();//wait next rising edge
        m_cycle++;//we must have a m_cycle incrementation after each wait

        if ( p_from_ctrl[m_numeric_param].rok.read() && p_from_ctrl[m_numeric_param].r.read() ) {
            mem_ptr[m_buffer_ptr] = machine_to_le((uint32_t)p_from_ctrl[m_numeric_param].data.read());
            m_buffer_ptr++;
            --m_cycles_left;

            if ( m_cycles_left == 0 ) 
            {
#if SOCLIB_MODULE_DEBUG
                std::cout << "reception done!!!" << std::endl;
#endif
                m_state = SYNTHHOST_NONE;
            }
        }

    }while(m_state == SYNTHHOST_TRANS_TO_COPROC);

    m_state = SYNTHHOST_NONE;
#if SOCLIB_MODULE_DEBUG
    std::cout << " prot read done " << len << std::endl;
#endif
}


/*************************** MWMR funcs ************************************/
tmpl(srl_mwmr_t)::get_mwmr(const char* name)
{
    std::string s_name(name);
    for(uint32_t i= 0; i< m_mwmrs.size(); i++)
    {
        if(!s_name.compare(m_mwmrs[i]->name))
            return m_mwmrs[i];
    }
#if SOCLIB_MODULE_DEBUG
    std::cout << "mwmr not found " << s_name << std::endl;
#endif
    assert(0);
}

tmpl(void)::io_dump( unsigned char *mem, size_t len )
{
    size_t i;
    for ( i=0; i<len; ++i )
        srl_log_printf(DEBUG, "%02x ", mem[i]);
    srl_log_printf(DEBUG, "\n");
}

tmpl(void)::srl_mwmr_read( srl_mwmr_t fifo, void *mem, size_t len )//len == nitem
{
    srl_assert(fifo->way == 1);
    //srl_assert(len % fifo->width == 0);
    len = len * fifo->width;
    srl_log_printf(DEBUG, "mwmr_read %s %d\n", fifo->name, len);

    _prot_read(mem, len, fifo->id);
    io_dump((uint8_t*)mem, len);
}

tmpl(void)::srl_mwmr_write( srl_mwmr_t fifo, void *mem, size_t len )
{
    srl_assert(fifo->way == 0);
    //srl_assert(len % fifo->width == 0);
    len = len * fifo->width;
    srl_log_printf(DEBUG, "mwmr_write %s %d\n", fifo->name, len);

    io_dump((uint8_t*)mem, len);
    _prot_write((uint8_t*)mem, len, fifo->id);
}

tmpl(ssize_t)::srl_mwmr_try_read( srl_mwmr_t fifo, void *mem, size_t len )
{
    srl_assert(!"Not implemented");
}

tmpl(ssize_t)::srl_mwmr_try_write( srl_mwmr_t fifo, void *mem, size_t len )
{
    srl_assert(!"Not implemented");
}

/***** hardware helpers ******/
tmpl(size_t)::srl_cycle_count()
{
    return m_cycle;
}


tmpl(void)::srl_busy_cycles(uint32_t cycles)
{
	m_state         = SYNTHHOST_RUNNING;
    m_cycles_left   = cycles;
    do{
	    wait();//wait next rising edge
        m_cycle++;
            
        --m_cycles_left;
        if ( m_cycles_left == 0 ) 
        {
#if SOCLIB_MODULE_DEBUG
            std::cout << "reception done!!!" << std::endl;
#endif
            m_state = SYNTHHOST_NONE;
        }
    }while(m_state == SYNTHHOST_TRANS_TO_COPROC);

}

tmpl(void)::srl_abort()
{
	sc_core::sc_stop();
}

}}
