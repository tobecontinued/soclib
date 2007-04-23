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
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */
#ifndef SOCLIB_CABA_FIFO_WRITER_H
#define SOCLIB_CABA_FIFO_WRITER_H

#include <systemc.h>
#include "caba/util/base_module.h"
#include "caba/interface/fifo_ports.h"
#include "common/process_wrapper.h"

namespace soclib {
namespace caba {

template <typename word_t>
class FifoWriter
    : public soclib::caba::BaseModule
{
public:
    sc_in<bool> p_clk;
    sc_in<bool> p_resetn;
    FifoInput<word_t> p_fifo;

private:
    word_t m_buffer;
    bool m_usage;
    int m_woffset;
    soclib::common::ProcessWrapper m_wrapper;

protected:
    SC_HAS_PROCESS(FifoWriter);

public:
    FifoWriter( sc_module_name insname,
                const std::string &bin,
                const std::vector<std::string> &argv );

private:
    void transition();
    void genMoore();
};

}}

#endif /* SOCLIB_CABA_FIFO_WRITER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
