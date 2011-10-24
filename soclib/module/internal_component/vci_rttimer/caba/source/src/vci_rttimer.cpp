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
 * Copyright (c) Telecom ParisTech
 *         Alexandre Becoulet <alexandre.becoulet@telecom-paristech.fr>, 2011
 *
 */

#include <string.h>

#include "rttimer.h"
#include "alloc_elems.h"
#include "../include/vci_rttimer.h"

namespace soclib { namespace caba {

using namespace soclib;

#define tmpl(t) template<typename vci_param> t VciRtTimer<vci_param>

tmpl(bool)::on_write(int seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be)
{
    if (addr >= SOCLIB_RTTIMER_DLN1)
      {
	unsigned int i = (addr - SOCLIB_RTTIMER_DLN1) / 16;
	uint32_t m = 1 << i;

	if (i > m_deadlines)
	  return false;

	switch ((addr - SOCLIB_RTTIMER_DLN1) % 16)
	  {
	  case 0: // SOCLIB_RTTIMER_DLN1
	    r_dln[i] = ((uint64_t)r_rtc_tmp.read() << 32) | data;

	    if (r_ctrl.read() & SOCLIB_RTTIMER_CTRL_IEW)
	      r_ie = r_ie.read() | m;
	    return true;

	  case 4: // SOCLIB_RTTIMER_DLN1S set
	    r_cmd[i] = r_cmd_dln_set;
	    r_data = data;
	    return true;

	  case 8: // SOCLIB_RTTIMER_DLN1A add
	    r_cmd[i] = r_cmd_dln_add;	
	    r_data = data;
	    return true;

	  case 12: // SOCLIB_RTTIMER_DLN1P period
	    r_dlnp[i] = data;
	    return true;

	  default:
	    return false;
	  }
      }
    else
      {
	switch (addr)
	  {
	  case SOCLIB_RTTIMER_SCCNT:
	    r_sccnt = data;
	    return true;

	  case SOCLIB_RTTIMER_SCRLD:
	    r_scrld = data;
	    return true;
	
	  case SOCLIB_RTTIMER_CTRL:
	    r_ctrl = data;
	    return true;

	  case SOCLIB_RTTIMER_RTCTMP:
	    r_rtc_tmp = data;
	    return true;

	  case SOCLIB_RTTIMER_PE:
	    r_pe = data;
	    return true;

	  case SOCLIB_RTTIMER_IE:
	    r_ie = data;
	    return true;

	  case SOCLIB_RTTIMER_IP:
	    r_ip = r_ip.read() & ~data;
	    return true;

	  case SOCLIB_RTTIMER_COPY: {
	    unsigned int i;

	    while ((i = ffs(data)) && (i + 1) < m_deadlines)
	      r_dln[i-1] = r_dln[i].read();

	    if (r_ctrl.read() & SOCLIB_RTTIMER_CTRL_IEC)
	      r_ie = r_ie.read() | data;

	    return true;
	  }

	  default:
	    return false;
	  }
      }
}

tmpl(bool)::on_read(int seg, typename vci_param::addr_t addr, typename vci_param::data_t &data)
{
    if (addr >= SOCLIB_RTTIMER_DLN1)
      {
	unsigned int i = (addr - SOCLIB_RTTIMER_DLN1) / 16;
	//	uint32_t m = 1 << i;

	if (i > m_deadlines)
	  return false;

	switch ((addr - SOCLIB_RTTIMER_DLN1) % 16)
	  {
	  case 0: // SOCLIB_RTTIMER_DLN1
	    data = (uint32_t)r_dln[i].read();
	    r_rtc_tmp = (uint32_t)(r_dln[i].read() >> 32);
	    return true;

	  case 12: // SOCLIB_RTTIMER_DLN1P period
	    data = r_dlnp[i].read();
	    return true;

	  default:
	    return false;
	  }
      }
    else
      {
	switch (addr)
	  {
	  case SOCLIB_RTTIMER_SCCNT:
	    data = r_sccnt.read();
	    return true;
	
	  case SOCLIB_RTTIMER_SCRLD:
	    data = r_scrld.read();
	    return true;

	  case SOCLIB_RTTIMER_CFG:
	    data = m_deadlines | (m_separate_irqs << 8);
	    return true;

	  case SOCLIB_RTTIMER_CTRL:
	    data = r_ctrl.read();
	    return true;

	  case SOCLIB_RTTIMER_RTCL:
	    data = (uint32_t)r_rtc.read();
	    r_rtc_tmp = (uint32_t)(r_rtc.read() >> 32);
	    return true;

	  case SOCLIB_RTTIMER_RTCH:
	    data = (uint32_t)(r_rtc.read() >> 32);
	    r_rtc_tmp = (uint32_t)r_rtc.read();
	    return true;

	  case SOCLIB_RTTIMER_RTCTMP:
	    data = r_rtc_tmp.read();
	    return true;

	  case SOCLIB_RTTIMER_PE:
	    data = r_pe.read();
	    return true;

	  case SOCLIB_RTTIMER_IE:
	    data = r_ie.read();
	    return true;

	  case SOCLIB_RTTIMER_IP:
	    data = r_ip.read();
	    return true;

	  default:
	    return false;
	  }
      }
}

tmpl(void)::transition()
{
  if (!p_resetn)
    {
      m_vci_fsm.reset();
      r_sccnt = 0;
      r_scrld = 0;
      r_ctrl = 0;
      r_rtc = 0;
      r_pe = 0;
      r_ie = 0;
      r_ip = 0;
      r_rtc_tmp = 0;
      r_data = 0;

      for (size_t i = 0 ; i < m_deadlines ; i++)
	{
	  r_cmd[i] = r_cmd_compare;
	  r_dln[i] = 0;
	  r_dlnp[i] = 0;
	}

      return;
    }

  // test chip enabled
  if (r_ctrl.read() & SOCLIB_RTTIMER_CTRL_CE)
    {
      if (r_sccnt.read() > 0)
	// update prescaler
	r_sccnt = r_sccnt.read() - 1;
      else
	{
	  // reset prescaler & update main counter
	  r_sccnt = r_scrld.read();
	  r_rtc = r_rtc.read() + 1;
	}
    }

  for (size_t i = 0 ; i < m_deadlines ; i++)
    {
      uint32_t m = 1 << i;

      switch (r_cmd[i].read())
	{
	case r_cmd_compare:

	  if (r_dln[i].read() >= r_rtc.read()) // deadline reached
	    {
	      // interrupt pending if irq enabled
	      if (r_ie.read() & m)
		r_ip = r_ip.read() | m;

	      if (r_pe.read() & m)
		// add period
		r_cmd[i] = r_cmd_dln_period;
	      else
		// or disable deadline irq
		r_ie = r_ie.read() & ~m;
	    }
	  break;

	case r_cmd_dln_set:
	  r_dln[i] = r_rtc.read() + (uint32_t)r_data.read();
	  r_cmd[i] = r_cmd_compare;

	  if (r_ctrl.read() & SOCLIB_RTTIMER_CTRL_IES)
	    r_ie = r_ie.read() | m;
	  break;

	case r_cmd_dln_add:
	  r_dln[i] = r_dln[i].read() + (int32_t)r_data.read();
	  r_cmd[i] = r_cmd_compare;

	  if (r_ctrl.read() & SOCLIB_RTTIMER_CTRL_IEA)
	    r_ie = r_ie.read() | m;
	  break;

	case r_cmd_dln_period:
	  r_dln[i] = r_dln[i].read() + r_dlnp[i].read();
	  r_cmd[i] = r_cmd_compare;
	  break;

	}
    }

  m_vci_fsm.transition();
}

//////////////////////
tmpl(void)::genMoore()
{
  m_vci_fsm.genMoore();

  if (m_separate_irqs)
    {
      for (size_t i = 0 ; i < m_deadlines ; i++)
	p_irq[i] = (r_ip.read() << i) != 0;
    }
  else
    {
      p_irq[0] = r_ip.read() != 0;
    }
}

tmpl(/**/)::VciRtTimer(sc_module_name name, const IntTab &index,
		     const MappingTable &mt, size_t deadlines = 7, bool separate_irqs = 0)
  : caba::BaseModule(name),
    m_vci_fsm(p_vci, mt.getSegmentList(index)),

    m_deadlines(deadlines),
    m_separate_irqs(separate_irqs),

    r_sccnt("sccnt"),
    r_scrld("scrld"),
    r_ctrl( "ctrl"),
    r_rtc("ctrl"),
    r_pe("pe"),
    r_ie("ie"),
    r_ip("ip"),
    r_rtc_tmp("rtc_tmp"),

    r_dln(soclib::common::alloc_elems<sc_signal<uint64_t> >("dln", deadlines)),
    r_dlnp(soclib::common::alloc_elems<sc_signal<uint32_t> >("dlnp", deadlines)),

    r_data("data"),
    r_cmd(soclib::common::alloc_elems<sc_signal<int> >("cmd", deadlines)),

    p_clk("clk"),
    p_resetn("resetn"),
    p_vci("vci"),
    p_irq(soclib::common::alloc_elems<sc_out<bool> >("irq", separate_irqs ? deadlines : 1))
{
  m_vci_fsm.on_read_write(on_read, on_write);

  SC_METHOD(transition);
  dont_initialize();
  sensitive << p_clk.pos();

  SC_METHOD(genMoore);
  dont_initialize();
  sensitive << p_clk.neg();
}

///////////////////////
tmpl(/**/)::~VciRtTimer()
{
  soclib::common::dealloc_elems(r_dln, m_deadlines);
  soclib::common::dealloc_elems(r_dlnp, m_deadlines);
  soclib::common::dealloc_elems(r_cmd, m_deadlines);
  soclib::common::dealloc_elems(p_irq, m_separate_irqs ? m_deadlines : 1);
}

}}

