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
 * Maintainers: becoulet nipo
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
#include "iss2.h"
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

    GdbServer(const std::string &name, uint32_t ident);

    uint32_t executeNCycles(
        uint32_t ncycle,
        const struct CpuIss::InstructionResponse &irsp,
        const struct CpuIss::DataResponse &drsp,
        uint32_t irq_bit_field );

    inline void getRequests(
        struct CpuIss::InstructionRequest &ireq,
        struct CpuIss::DataRequest &dreq) const
    {
        GdbServer<CpuIss> *_this = const_cast<GdbServer<CpuIss> *>(this);
        if (state_ == Frozen)
            {
                ireq.valid = false;
                dreq.valid = mem_req_;
                dreq.addr = mem_addr_ & ~3;
                dreq.wdata = mem_data_ << (8 * (mem_addr_ & 3));
                dreq.type = mem_type_;
                if ( mem_type_ == CpuIss::DATA_READ )
                    dreq.be = 0xf;
                else
                    dreq.be = 1 << (mem_addr_ & 3);
                dreq.mode = CpuIss::MODE_HYPER;
            }
        else
            {
                CpuIss::getRequests(ireq, dreq);
            }
        _this->pending_data_request_ = dreq.valid;
        _this->pending_ins_request_ = ireq.valid;
    }

    inline void setWriteBerr()
    {
        if (state_ != Frozen)
            CpuIss::setWriteBerr();
    }

    static inline void start_frozen(bool frozen = true)
    {
        init_state_ = frozen ? MemWait : Running;
    }

    bool debugExceptionBypassed( uint32_t cause );

private:

    uint32_t debug_reg_swap(uint32_t);

    static void signal_handler(int sig);

    static void global_init();
    static int write_packet(const char *data);
    static char *read_packet(char *buffer, size_t size);
    void process_gdb_packet();
    void process_monitor_packet(char *data);
    static void try_accept();
    bool process_mem_access();
    void cleanup();
    void watch_mem_access();
    bool check_break_points();

    bool mem_req_;
    bool pending_data_request_;
    bool mem_rsp_valid_;
    enum CpuIss::DataOperationType mem_type_;
    uint32_t mem_addr_;
    uint32_t mem_data_;
    bool mem_error_;
    bool pending_ins_request_;

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
