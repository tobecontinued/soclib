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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *         Aline Vieira de Mello <Aline.Vieira-de-Mello@lip6.fr>, 2008
 *
 * Maintainers: nipo, alinev
 */
#ifndef SOCLIB_TLMT_FIFO_READER_H
#define SOCLIB_TLMT_FIFO_READER_H

#include <tlmt>
#include "tlmt_base_module.h"
#include "fifo_ports.h"
#include "vci_ports.h"
#include "process_wrapper.h"

namespace soclib { namespace tlmt {

template<typename vci_param>
class FifoReader
  : public soclib::tlmt::BaseModule
{
private:
    soclib::common::ProcessWrapper m_wrapper;
    typename vci_param::data_t     m_write_buffer[64];
    typename vci_param::data_t     m_data;
    int                            m_woffset;
    unsigned int                   m_status;
    uint32_t                       m_depth_fifo;

    tlmt_core::tlmt_thread_context c0;
    fifo_cmd_packet<vci_param>     m_cmd;
    sc_core::sc_event              m_rsp_write;

protected:
    SC_HAS_PROCESS(FifoReader);

public:
    soclib::tlmt::FifoInitiator<vci_param> p_fifo;

    FifoReader( sc_core::sc_module_name name,
                const std::string &bin,
                const std::vector<std::string> &argv,
		uint32_t depth_fifo);

     void writeReponseReceived(int data,
			      const tlmt_core::tlmt_time &time,
			      void *private_data);
private:

    void execLoop();
};

}}

#endif /* SOCLIB_TLMT_FIFO_READER_H */

