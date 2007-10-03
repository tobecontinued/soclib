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
 *
 * Maintainers: nipo
 */
#ifndef TARGET_FSM_HANDLER_H
#define TARGET_FSM_HANDLER_H

#include <systemc.h>
#include <vector>
#include <list>
#include <cassert>
#include "caba/interface/vci_target.h"
#include "caba/util/generic_fifo.h"
#include "common/mapping_table.h"
#include "caba/util/base_module.h"

namespace soclib {
namespace caba {

using namespace soclib::common;

/** \cond DONT_SHOW
 * [Dont show this ugliness to Doxygen, it may be scared.]
 *
 * Why those big bad casts ?
 *
 * When component register on_read and on_write functions, it passes
 * pointer to member functions (ie functions like
 * ComponentName::on_read(...)). When we use them, we would like
 * pointers on standart functions (ie like func(...)), so we must cast
 * the pointers.
 *
 * The problem is C++ wont allow casing from one to the other
 * directly, event if it may be correct. We have to cast three times
 * in order to get what we want without warning.
 *
 * Hopefully enough SystemC declares SC_CURRENT_USER_MODULE which is a
 * typedef to current component type, this allows us to have some
 * generic defines for casts (defines names are expanded at usage time)
 *
 */

#define __rcast1 bool (SC_CURRENT_USER_MODULE::*)(int, typename vci_param::addr_t, typename vci_param::data_t &)
#define __wcast1 bool (SC_CURRENT_USER_MODULE::*)(int, typename vci_param::addr_t, typename vci_param::data_t, int)

#define __rcast2 bool (*)(SC_CURRENT_USER_MODULE *, int, typename vci_param::addr_t, typename vci_param::data_t &)
#define __wcast2 bool (*)(SC_CURRENT_USER_MODULE *, int, typename vci_param::addr_t, typename vci_param::data_t, int)

#define __rcast3 bool (*)(soclib::caba::BaseModule *, int, typename vci_param::addr_t, typename vci_param::data_t &)
#define __wcast3 bool (*)(soclib::caba::BaseModule *, int, typename vci_param::addr_t, typename vci_param::data_t, int)

#define on_read_write(rf, wf)                                           \
_on_read_write(this,                                                    \
(__rcast3)(__rcast2)(__rcast1)&SC_CURRENT_USER_MODULE::rf,              \
(__wcast3)(__wcast2)(__wcast1)&SC_CURRENT_USER_MODULE::wf )

/** \endcond */

/**
 * \brief Full VCI Target port handler
 *
 * This handles a VCI Target port, calls back owner module when data
 * changes (on read or on write). This also handles multiple-segment
 * targets looking up which segment is targetted by query.
 *
 * \param VCI_TMPL_PARAM_DECL VCI fields parameters
 * \param default_target whether the FSM must handle out-of-segments
 * queries answering a VCI Error packed.
 * \param fifo_depth depth of internal fifo for handling requests. If
 * 2 or more, this allow pipelining of queries. Component latency for
 * answering will be 1+fifo_depth.
 */
template<
    typename vci_param,
    bool default_target,
    size_t fifo_depth>
class VciTargetFsm
{
private:

    enum vci_target_fsm_state_e {
        TARGET_IDLE,
        TARGET_WRITE_RSP,
        TARGET_READ_RSP,
        TARGET_ERROR_RSP,
    };

    VciTarget<vci_param> &p_vci;

    std::vector<soclib::common::Segment> m_segments;

    enum vci_target_fsm_state_e m_state;

    struct rsp_info_s {
        typename vci_param::srcid_t srcid;
        typename vci_param::trdid_t trdid;
        typename vci_param::pktid_t pktid;
        typename vci_param::data_t  rdata;
        typename vci_param::eop_t   eop;
        typename vci_param::rerror_t error;
    };
    typedef struct rsp_info_s rsp_info_t;

    soclib::caba::GenericFifo<rsp_info_t, fifo_depth> m_rsp_info;

    typedef typename vci_param::addr_t addr_t;
    typedef typename vci_param::data_t data_t;

    typedef bool wrapper_read_t(soclib::caba::BaseModule *, int segno, addr_t offset, data_t &data);
    typedef bool wrapper_write_t(soclib::caba::BaseModule *, int segno, addr_t offset, data_t data, int be);

    wrapper_read_t *m_on_read_f;
    wrapper_write_t *m_on_write_f;

    soclib::caba::BaseModule *m_owner;

public:

    /**
     * \brief Constructor
     *
     * Takes a reference to the VCI Target port to handler queries
     * from, and a list of segment to handle.
     *
     * \param _vci VCI Target port reference
     * \param seglist list of target's segments
     */
    VciTargetFsm(
        VciTarget<vci_param> &_vci,
        const std::list<soclib::common::Segment> &seglist );

    /**
     * \brief Callback setting
     *
     * Sets which functions should be called when requests asks for
     * data, or changes data
     * \param owner_module module owning target port
     * \param read_func function to call back when data is read from
     * component
     * \param write_func function to call back when data is written to
     * component
     */
    void _on_read_write(
        soclib::caba::BaseModule *owner_module,
        wrapper_read_t *read_func,
        wrapper_write_t *write_func );

    /**
     * \brief Desctructor
     */
    ~VciTargetFsm();

    /**
     * \brief Resets internal state
     *
     * Should be called on reset of the owning component
     */
    void reset();

    /**
     * \brief Performs internal state machine transition
     *
     * Should be called on transitions of the owning component
     */
    void transition();

    /**
     * \brief Performs moore generation function and drives signals on
     * VCI port
     *
     * Should be called when generating outputs from the owning
     * component
     */
    void genMoore();

    /**
     * \copydoc soclib::caba::BaseModule::trace()
     */
    void trace(sc_trace_file &tf, const std::string base_name, unsigned int what);

    /**
     * \brief Gets a segment's size
     *
     * \param seg wanted segment's number
     * \return segment's size (bytes)
     */
    inline typename vci_param::addr_t getSize(size_t seg) const
    {
        assert(seg < m_segments.size());
        return m_segments[seg].size();
    }

    /**
     * \brief Gets a segment's end address
     *
     * \param seg wanted segment's number
     * \return segment's end address
     */
    inline typename vci_param::addr_t getEnd(size_t seg) const
    {
        assert(seg < m_segments.size());
        return m_segments[seg].baseAddress()+m_segments[seg].size();
    }

    /**
     * \brief Gets a segment's base address
     *
     * \param seg wanted segment's number
     * \return segment's base address
     */
    inline typename vci_param::addr_t getBase(size_t seg) const
    {
        assert(seg < m_segments.size());
        return m_segments[seg].baseAddress();
    }

    /**
     * \brief Gets a segment's name
     *
     * \param seg wanted segment's number
     * \return segment's name
     */
    inline const char * getName(size_t seg) const
    {
        assert(seg < m_segments.size());
        return m_segments[seg].name().c_str();
    }

    /**
     * \brief Get number of handled segments
     *
     * \return number of handled segments
     */
    inline size_t nbSegments() const
    {
        return m_segments.size();
    }
};

}}

#endif /* TARGET_FSM_HANDLER_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

