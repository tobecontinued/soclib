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
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 */

#include <stdint.h>
#include "register.h"
#include "../include/vci_dma_tsar_v2.h"
#include "dma_tsar_v2.h"

namespace soclib { namespace caba {

#define tmpl(t) template<typename vci_param> t VciDmaTsarV2<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    int cell = (int)addr / vci_param::B;

	switch ((enum SoclibDmaRegisters)cell) {
	case DMA_SRC:
		m_src = data;
		return true;
    case DMA_DST:
		m_dst = data;
		return true;
    case DMA_LEN:
		m_len = data;
		return true;
    case DMA_RESET:
        m_must_finish = true;
        m_irq_enabled = false;
		r_irq = false;
        return true;
    case DMA_IRQ_DISABLED:
		m_irq_enabled = !data;
		return true;
	};
	return false;
}

tmpl(void)::ended()
{
	if ( m_irq_enabled )
		r_irq = true;
	m_len = 0;
    m_must_finish = false;
	m_handling = false;
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    int cell = (int)addr / vci_param::B;

	switch ((enum SoclibDmaRegisters)cell) {
	case DMA_SRC:
		data = m_src;
		return true;
    case DMA_DST:
		data = m_dst;
		return true;
	case DMA_LEN:
		data = m_len;
		return true;
    default:
        return false;
	}
	return false;
}

tmpl(void)::read_done( req_t *req )
{
	if ( !req->failed() && !m_must_finish ) {
		ssize_t burst = m_offset_buffer;

        m_partial = false;
        if( (( m_dst+m_offset ) & ~( m_data.size()-1 )) !=
            (( m_dst+m_offset+m_offset_buffer-1 ) & ~( m_data.size()-1 )) ){
            burst = (((m_dst+m_offset) & ~( m_data.size()-1))+m_data.size()) - (m_dst+m_offset);
            m_partial = true;
        }
		VciInitSimpleWriteReq<vci_param> *new_req =
			new VciInitSimpleWriteReq<vci_param>( m_dst+m_offset, &m_data[0], burst );
		new_req->setDone( this, ON_T(write_finish) );
		m_vci_init_fsm.doReq( new_req );
	} else {
		ended();
	}
	delete req;
}

tmpl(void)::write_finish( req_t *req )
{
    ssize_t burst = 0;
    uint32_t offset = 0;
    if( m_partial ){
        burst = m_dst + m_offset + m_offset_buffer - ((m_dst+m_offset+m_offset_buffer) & ~( m_data.size()-1));
        offset = m_offset_buffer - burst ;
    } else {
	    m_offset += m_offset_buffer;
    }
	if ( !req->failed() && !m_must_finish ){
        if(m_partial){
            VciInitSimpleWriteReq<vci_param> *new_req =
                new VciInitSimpleWriteReq<vci_param>( m_dst+m_offset+offset, &m_data[offset], burst );
            new_req->setDone( this, ON_T(write_finish) );
            m_partial = false;
            m_vci_init_fsm.doReq( new_req );
            delete req;
            return;
        }
		next_req();
    }
	else {
		ended();
	}
	delete req;
}

tmpl(void)::next_req()
{
	ssize_t remaining = (ssize_t)m_len-(size_t)m_offset;
	if ( remaining <= 0 ) {
		ended();
		return;
	}

	ssize_t burst = m_data.size();
    m_partial = false;
    if ( ( m_src+m_offset ) & ( m_data.size()-1 ) ){
        burst = m_data.size() - ( (m_src+m_offset) & (m_data.size()-1) );
        m_partial = true;
    }
	if ( remaining < burst ){
		burst = remaining;
        m_partial = true;
    }

    m_offset_buffer = burst;

	VciInitSimpleReadReq<vci_param> *req =
		new VciInitSimpleReadReq<vci_param>(
			&m_data[0], (m_src+m_offset), burst );
	req->setDone( this, ON_T(read_done) );
	m_vci_init_fsm.doReq( req );
}

tmpl(void)::transition()
{
	if (!p_resetn) {
		m_vci_target_fsm.reset();
		m_vci_init_fsm.reset();
		r_irq = false;
		m_irq_enabled = false;
		m_len = 0;
        m_offset_buffer = 0;
		m_handling = false;
        m_must_finish = false;
		return;
	}


	if ( m_len && !m_handling ) {
		m_handling = true;
		m_offset = 0;

		next_req();
	}
    
    if( !m_handling )
        m_must_finish = false;

	m_vci_target_fsm.transition();
	m_vci_init_fsm.transition();
}

tmpl(void)::genMoore()
{
	m_vci_target_fsm.genMoore();
	m_vci_init_fsm.genMoore();

	p_irq = r_irq && m_irq_enabled;
}

tmpl(/**/)::VciDmaTsarV2(
    sc_module_name name,
    const MappingTable &mt,
    const IntTab &srcid,
    const IntTab &tgtid,
	const size_t burst_size )
	: caba::BaseModule(name),
	  m_vci_target_fsm(p_vci_target, mt.getSegmentList(tgtid)),
	  m_vci_init_fsm(p_vci_initiator, mt.indexForId(srcid)),
	  m_len(0),
	  m_data(burst_size, (uint8_t)0),
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci_target("vci_target"),
      p_vci_initiator("vci_initiator"),
      p_irq("irq")
{
	m_vci_target_fsm.on_read_write(on_read, on_write);
    
    assert(burst_size && "Useless DMA with no buffer");
    assert(burst_size < (1<<vci_param::K) && "I will be unable to create requests that big");

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

