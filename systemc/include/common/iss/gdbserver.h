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

#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <systemc.h>
#include "common/iss/iss.h"
#include "common/exception.h"
#include "common/endian.h"
#include "common/register.h"

namespace soclib { namespace common {

struct GdbWatchPoint
{
    static const uint8_t a_read = 1;
    static const uint8_t a_write = 2;
    static const uint8_t a_rw = 3;

    inline GdbWatchPoint(uint32_t address, size_t len, uint8_t type)
        : address_(address),
          len_(len),
          type_(type)
    {
    }

    inline bool match(uint32_t address)
    {
        return address >= address_ && address < address_ + len_;
    }

    inline bool operator== (const GdbWatchPoint &it) const
    {
        return it.address_ == address_ &&
            it.type_ == type_;
    }

    uint32_t address_;
    size_t len_;
    uint8_t type_;
};

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

    inline void nullStep()
    {
        if (state_ != Frozen)
            CpuIss::nullStep();        
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

private:

    static void signal_handler(int sig);

    bool exceptionBypassed( uint32_t cause );

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

    static std::map<uint32_t, bool> break_exec_;
    static std::list<GdbWatchPoint> break_access_;

    static inline void access_break_add(const GdbWatchPoint &it)
    {
        break_access_.push_back(it);
    }

    static inline void access_break_remove(const GdbWatchPoint &it)
    {
        typename std::list<GdbWatchPoint>::iterator i;

        i = std::find(break_access_.begin(), break_access_.end(), it);

        if (i != break_access_.end())
            break_access_.erase(i);
    }

    static inline uint8_t access_break_check(uint32_t address, uint8_t type)
    {
        typename std::list<GdbWatchPoint>::iterator i;

        for (i = break_access_.begin();
             i != break_access_.end(); i++)
            {
                if (i->address_ == address && (i->type_ & type))
                    return i->type_;
            }

        return 0;
    }

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
