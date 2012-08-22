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
 */

#ifndef FIFO_TASK_H_
#define FIFO_TASK_H_

#include <systemc>
#include <stdarg.h>
#include <vector>

#include "caba_base_module.h"

#include "fifo_ports.h"

#include "srl_log.h"
#include "srl_hw_helpers.h"
#include "srl_private_types.h"

namespace dsx { namespace caba {


class FifoTask
	: public soclib::caba::BaseModule
{
	const size_t m_n_fifo_to_ctrl;
	const size_t m_n_fifo_from_ctrl;

public:
    sc_core::sc_in<bool> p_clk;
    sc_core::sc_in<bool> p_resetn;

	soclib::caba::FifoOutput<uint32_t> *p_to_ctrl;
	soclib::caba::FifoInput<uint32_t> *p_from_ctrl;


private:
	size_t m_cycle;             //check maximum ?

    std::vector<srl_mwmr_t> m_mwmrs;

	const std::vector<std::string> m_names_to_ctrl;
	const std::vector<std::string> m_names_from_ctrl;

	enum {
		SYNTHHOST_NONE,         //This state should never be run!
		SYNTHHOST_RUNNING,
		SYNTHHOST_TRANS_TO_COPROC,
		SYNTHHOST_TRANS_FROM_COPROC,
	} m_state;


	size_t m_cycles_left;
	size_t m_cycles_left_last;
	size_t m_numeric_param;

	uint32_t *m_buffer;
	uint8_t *m_data;
	size_t m_buffer_ptr;
    uint32_t m_recv_buffer;
    bool m_recv_buffer_valid;

protected:
    SC_HAS_PROCESS(FifoTask);

public:
    ~FifoTask();

    FifoTask(sc_core::sc_module_name insname,
				  const std::vector<std::string> &fifos_out,
				  const std::vector<int> &fifos_out_width,
				  const std::vector<std::string> &fifos_in,
				  const std::vector<int> &fifos_in_width
            );

    void run();     //has the role of a the transition function
    void genMoore();

    static std::vector<int> intArray( const int length, ... );
    static std::vector<std::string> stringArray( const char *first, ... );


    
    void _prot_read( void *mem, size_t len, uint32_t id );
    void _prot_write( void *mem, size_t len, uint32_t id );

public:
    virtual void task_func();

    /*** All the following function must be in this class ***/

    /**** MWMR ****/
    #define SRL_GET_MWMR(name) get_mwmr( #name)
    srl_mwmr_t get_mwmr(const char* name);
    void io_dump( unsigned char *mem, size_t len );

    ssize_t srl_mwmr_try_write( srl_mwmr_t fifo, void *mem, size_t len );
    ssize_t srl_mwmr_try_read( srl_mwmr_t fifo, void *mem, size_t len );
    void srl_mwmr_write( srl_mwmr_t fifo, void *mem, size_t len );
    void srl_mwmr_read( srl_mwmr_t fifo, void *mem, size_t len );

    /*** hw_helpers ***/
    size_t srl_cycle_count();
    void srl_busy_cycles(uint32_t);
    void srl_abort();

};

}}

//TODO: endianness, use soclib endiannes?
#endif /* FIFO_TASK_H_ */
