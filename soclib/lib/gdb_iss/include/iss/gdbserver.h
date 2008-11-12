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
 * Copyright (c) UPMC, Lip6
 *         Alexandre Becoulet <alexandre.becoulet@lip6.fr>
 *
 * Maintainers: becoulet
 *
 * $Id$
 *
 * History:
 * - 2007-10-22
 *   Alexandre becoulet: Model created
 */

#ifndef _SOCLIB_GDBSERVER_ISS_H_
#define _SOCLIB_GDBSERVER_ISS_H_

#include <stdint.h>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <systemc.h>
#include "interval_set.hh"
#include "iss.h"
#include "exception.h"
#include "soclib_endian.h"
#include "register.h"

namespace soclib { namespace common {

template<typename CpuIss>
class GdbServer
    : public CpuIss
{
public:

    inline void set_tcp_port(uint16_t port)
    {
        port_ = port;
    }

    GdbServer(uint32_t ident);

    void step();

    inline void nullStep( uint32_t i=1 )
    {
        if (state_ != Frozen)
            CpuIss::nullStep(i);        
    }

    inline void getDataRequest(bool &req, enum Iss::DataAccessType &type, uint32_t &address, uint32_t &wdata) const
    {
        if (state_ == Frozen)
            {
                req = mem_req_;
                address = mem_addr_ & ~3;
                wdata = mem_data_;
                type = mem_type_;
            }
        else
            {
                CpuIss::getDataRequest(req, type, address, wdata);
            }
    }

    inline void setWriteBerr()
    {
        if (state_ != Frozen)
            CpuIss::setWriteBerr();
    }

	inline void setDataResponse(bool error, uint32_t rdata)
    {
        if (state_ == Frozen)
            {
                mem_error_ = error;
                mem_data_ = rdata;
            }
        else
            CpuIss::setDataResponse(error, rdata);
    }

    inline void getInstructionRequest(bool &req, uint32_t &address) const
	{
        if (state_ == Frozen) {
            address = 0;
            req = false;
        } else
            CpuIss::getInstructionRequest(req, address);
	}

    static inline void start_frozen(bool frozen = true)
    {
        init_state_ = frozen ? MemWait : Running;
    }

    bool exceptionBypassed( uint32_t cause );

private:

    static void signal_handler(int sig);

    static void global_init();
    static int write_packet(char *data);
    static char *read_packet(char *buffer, size_t size);
    void process_gdb_packet();
    void process_monitor_packet(char *data);
    static void try_accept();
    bool process_mem_access();
    void cleanup();
    void watch_mem_access();
    bool check_break_points();

    bool mem_req_;
    Iss::DataAccessType mem_type_;
    uint32_t mem_addr_;
    uint32_t mem_data_;
    bool mem_error_;

    // number of memory access left to process
    size_t mem_count_;
    // memory dump bytes size
    size_t mem_len_;
    uint8_t *mem_buff_;
    uint8_t *mem_ptr_;
    static unsigned int current_id_;
    static unsigned int step_id_; // can be used to force single step on a specific processor
    bool catch_execeptions_;

#ifdef GDB_PC_TRACE
    uint32_t pc_trace_table[GDB_PC_TRACE];
    unsigned int pc_trace_index;
#endif

    typedef dpp::interval_set<uint32_t, dpp::interval_bound_inclusive<uint32_t> > address_set_t;

    static std::map<uint32_t, bool> break_exec_;
    static address_set_t break_read_access_;
    static address_set_t break_write_access_;

    // listen socket
    static int socket_;
    // accepted connection socket
    static int asocket_;
    // listen port
    static uint16_t port_;

    // CtrlC pressed
    static bool ctrl_c_;

    enum State
        {
            Running,
            StepWait,
            Step,
            MemWait,            // waiting for memory operation end before freeze
            Frozen,
        };

    static State init_state_;
    static State init_state();
    State state_;

    static inline void change_all_states(State s)
    {
        for (unsigned int i = 0; i < list_.size(); i++)
            list_[i]->state_ = s;
    }

    unsigned int id_;
    static std::vector<GdbServer *> list_;
};

}}

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
