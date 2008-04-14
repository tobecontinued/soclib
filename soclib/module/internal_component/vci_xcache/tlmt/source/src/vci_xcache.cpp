/* -*- mode: c++; coding: utf-8 -*-
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
 * Maintainers: fpecheux, nipo
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Nicolas Pouillon <nipo@ssji.net>
 */

#include "../include/vci_xcache.h"

namespace soclib
{
  namespace tlmt
  {

#define tmpl(x) template<typename iss_t, typename vci_param> x VciXcache<iss_t,vci_param>

    tmpl (tlmt_core::tlmt_return &)::rspReceived (soclib::tlmt:: vci_rsp_packet < vci_param > *pkt,
						  const tlmt_core:: tlmt_time & time, void *private_data)
    {
      if (pkt->pktid == vci_param::CMD_WRITE)
	m_write_error = (pkt->error != 0);
      else
	m_write_error = false;
      if (pkt->pktid == vci_param::CMD_READ)
	m_read_error = (pkt->error != 0);
      else
	m_read_error = false;
      m_rsptime = time + tlmt_core::tlmt_time (pkt->nwords);
      m_vci_pending = false;
      m_rsp_received.notify (sc_core::SC_ZERO_TIME);

      return m_return;
    }

    tmpl (tlmt_core::tlmt_return &)::irqReceived (bool v, const tlmt_core::
						  tlmt_time & time, void *private_data)
    {
	    int no = (int)(long)private_data;
      std::cout << name () << " callback_synchro " << no << " " << v << std::endl;
      return m_return;
    }


    tmpl (void)::execLoop ()
    {
      for (;;)
	{

	  c0.update_time (c0.time () + tlmt_core::tlmt_time (1));

	  if (m_iss.isBusy ())
	    m_iss.nullStep ();
	  else
	    {
	      bool ins_asked;
	      uint32_t ins_addr;
	      m_iss.getInstructionRequest (ins_asked, ins_addr);

	      bool data_asked;
	      enum iss_t::DataAccessType data_type;
	      uint32_t data_addr;
	      uint32_t data_wdata;
	      uint32_t data_rdata;
	      bool data_ber;
	      uint32_t ins_rdata;
	      bool ins_ber;

	      m_iss.getDataRequest (data_asked, data_type, data_addr,
				    data_wdata);

	      xcacheAccess (ins_asked, ins_addr,
			    data_asked, data_type, data_addr, data_wdata,
			    data_rdata, data_ber, ins_rdata, ins_ber);

	      if (ins_asked)
		{
		  m_iss.setInstruction (ins_ber, ins_rdata);
		}
	      if (data_asked)
		{
		  m_iss.setDataResponse (data_ber, data_rdata);
		}

	      if (m_write_error)
		{
		  m_iss.setWriteBerr ();
		  m_write_error = false;
		}

	      m_iss.step ();
	      //m_iss.dump();

	      m_counter++;
	      if (m_counter >= m_lookahead)
		{
		  m_counter = 0;
		  wait (sc_core::SC_ZERO_TIME);
		}
	    }
	}
    }

    tmpl (void)::xcacheAccess (bool & ins_asked,
			       uint32_t & ins_addr,
			       bool & data_asked,
			       enum iss_t::DataAccessType & data_type,
			       uint32_t & data_addr,
			       uint32_t & data_wdata,
			       uint32_t & data_rdata,
			       bool & data_ber,
			       uint32_t & ins_rdata, bool & ins_ber)
    {

      // handling write buffer requests
      if (!m_wbuf.empty ())
	{
	  // The write packet will be sent to the VCI port if
	  // there is no data memory access during the current cycle
	  // or if the current data memory access is not a write operation
	  // of if the write operation corresponds to an address that is not consecutive
	  // to the last address in the write buffer 
	  if (!data_asked ||
	      !((data_type == iss_t::WRITE_WORD) ||
		(data_type == iss_t::WRITE_HALF) ||
		(data_type == iss_t::WRITE_BYTE)) ||
	      m_wbuf.endBurst (data_addr))
	    {			// write burst can be send
	      typename vci_param::addr_t address = m_wbuf.getAddress ();
	      if (m_wbuf.getType () == iss_t::WRITE_WORD)
		m_cmd.be = 0xF;
	      else if (m_wbuf.getType () == iss_t::WRITE_HALF)
		m_cmd.be = 0x3 << address & 0x2;
	      else if (m_wbuf.getType () == iss_t::WRITE_BYTE)
		m_cmd.be = 0x1 << address & 0x3;
	      //m_addresses_tab[0] = address;
	      m_cmd.contig = true;
	      //m_cmd.address = m_addresses_tab;
	      m_cmd.address = address;
	      m_cmd.cmd = vci_param::CMD_WRITE;
	      m_cmd.buf = m_write_buffer;
	      m_cmd.srcid = 0;
	      m_cmd.trdid = 0;
	      m_cmd.pktid = vci_param::CMD_WRITE;
	      uint32_t i;
	      bool notlast = true;
	      for (i = 0; notlast == true; i++)
		{
		  //m_addresses_tab[i] = address+4*i ;
		  m_write_buffer[i] = m_wbuf.popData ();
		  notlast = m_wbuf.notlastWrite ();
		}
	      m_cmd.nwords = i;
	      p_vci.send (&m_cmd, c0.time ());
	      m_vci_pending = true;
	    }			// end if write burst
	}			// end if m_wbuf not empty

      // handling data request

      if (data_asked)
	{
	  switch (data_type)
	    {
	    case iss_t::READ_WORD:
	    case iss_t::READ_HALF:
	    case iss_t::READ_BYTE:
	      if (m_vci_pending)
		{
		  wait (m_rsp_received);
		  c0.update_time (m_rsptime);
		}
	      //if (data_addr) { // rajouter le code pour vérifier que l'adresse est non cachée     
	      if (0)
		{		// rajouter le code pour vérifier que l'adresse est non cachée       
		  m_cmd.cmd = vci_param::CMD_READ;
		  //m_addresses_tab[0] = data_addr & m_dcache.get_yzmask ();	// a revoir
		  //m_cmd.address = m_addresses_tab;
		  m_cmd.address = data_addr & m_dcache.get_yzmask ();
		  if (data_type == iss_t::READ_WORD)
		    m_cmd.be = 0xF;
		  else if (data_type == iss_t::READ_HALF)
		    m_cmd.be = 0x3 << data_addr & 0x2;
		  else if (data_type == iss_t::READ_BYTE)
		    m_cmd.be = 0x1 << data_addr & 0x3;
		  //m_cmd.contig = false;
		  m_cmd.contig = true;
		  m_cmd.buf = m_read_buffer;
		  m_cmd.nwords = 1;
		  m_cmd.srcid = 0;
		  m_cmd.trdid = 0;
		  //m_cmd.pktid = 0;
		  m_cmd.pktid = vci_param::CMD_READ;

		  tlmt_core::tlmt_return ret;
		  ret = p_vci.send (&m_cmd, c0.time ());
		  sc_core::wait (m_rsp_received);
		  data_ber = m_read_error;
		  data_rdata = m_read_buffer[0];
		}
	      else if (m_dcache.miss (data_addr))
		{		// read data miss (blocking)
		  if (m_vci_pending)
		    {
		      wait (m_rsp_received);
		      c0.update_time (m_rsptime);
		    }

		  //m_addresses_tab[0] = data_addr & m_dcache.get_yzmask ();
		  //m_cmd.address = m_addresses_tab;
		  m_cmd.address = data_addr & m_dcache.get_yzmask ();
		  m_cmd.cmd = vci_param::CMD_READ;
		  m_cmd.nwords = m_dcache.get_nwords ();
		  m_cmd.buf = m_read_buffer;
		  m_cmd.be = 0xF;
		  m_cmd.contig = true;
		  m_cmd.srcid = 0;
		  m_cmd.trdid = 0;
		  m_cmd.pktid = vci_param::CMD_READ;
		  p_vci.send (&m_cmd, c0.time ());
		  m_vci_pending = true;
		  wait (m_rsp_received);
		  c0.update_time (m_rsptime);
		  m_dcache.update (data_addr, m_read_buffer);
		  data_ber = m_read_error;
		  uint32_t d = m_dcache.read (data_addr);
		  if (data_type == iss_t::READ_HALF)
		    {
		      d = 0xffff & (d >> (8 * (data_addr & 0x3)));
		      d = d | (d << 16);
		    }
		  else if (data_type == iss_t::READ_BYTE)
		    {
		      d = 0xff & (d >> (8 * (data_addr & 0x3)));
		      d = d | (d << 8) | (d << 16) | (d << 24);
		    }
		  data_rdata = d;
		}
	      else
		{		// read data hit (non blocking)
		  data_ber = false;
		  uint32_t d = m_dcache.read (data_addr);
		  if (data_type == iss_t::READ_HALF)
		    {
		      d = 0xffff & (d >> (8 * (data_addr & 0x3)));
		      d = d | (d << 16);
		    }
		  else if (data_type == iss_t::READ_BYTE)
		    {
		      d = 0xff & (d >> (8 * (data_addr & 0x3)));
		      d = d | (d << 8) | (d << 16) | (d << 24);
		    }
		  data_rdata = d;
		}
	      break;
	    case iss_t::WRITE_WORD:
	    case iss_t::WRITE_HALF:
	    case iss_t::WRITE_BYTE:
	      if (!m_dcache.miss (data_addr))
		{
		  m_dcache.write (data_addr, data_wdata);
		}
	      m_wbuf.push (data_addr, data_type, data_wdata);
	      data_ber = false;
	      data_rdata = 0;

	      break;
	    case iss_t::LINE_INVAL:
	      m_dcache.inval (data_addr);
	      data_ber = false;
	      data_rdata = 0;
	      break;
	    case iss_t::READ_LINKED:	// linked load (blocking)
	      if (m_vci_pending)
		{
		  wait (m_rsp_received);
		  c0.update_time (m_rsptime);
		}
	      /*
	         m_addresses_tab[0]=data_addr;
	         m_cmd.address = m_addresses_tab ;
	         m_cmd.cmd = vci_param::CMD_READ_LOCKED ;
	         m_cmd.nwords = 1 ;
	         m_cmd.buf = m_read_buffer ;
	         m_cmd.be = 0xF ;
	         p_vci.send( &m_cmd, c0.time() ) ;
	         m_vci_pending = true ;
	         wait( m_rsp_received ) ;
	         c0.update_time( m_rsptime ) ;
	         data_ber = m_read_error ;
	         data_rdata = m_read_buffer[0] ;
	       */
	      break;
	    case iss_t::STORE_COND:	// store conditional (blocking)
	      if (m_vci_pending)
		{
		  wait (m_rsp_received);
		  c0.update_time (m_rsptime);
		}
	      /*
	         m_addresses_tab[0]=data_addr;
	         m_cmd.address = m_addresses_tab ;
	         m_cmd.cmd = vci_param::CMD_WRITE_COND ;
	         m_cmd.nwords = 1 ;
	         m_write_buffer[0] = data_wdata ;
	         m_cmd.buf = m_write_buffer ;
	         m_cmd.be = 0xF ;
	         m_cmd.contig = false ;
	         p_vci.send( &m_cmd, c0.time() ) ;
	         m_vci_pending = true ;
	         wait( m_rsp_received ) ;
	         c0.update_time( m_rsptime ) ;
	         data_ber = m_read_error ;
	         data_rdata = m_write_buffer[0] ;
	       */
	      break;
	    }
	}

      // handling the instruction request
      if (ins_asked)
	{
	  if (m_icache.miss (ins_addr))
	    {			// fetch instruction miss
	      if (m_vci_pending)
		{		// waiting the completion of the current VCI transaction
		  wait (m_rsp_received);
		  c0.update_time (m_rsptime);
		}
	      //m_addresses_tab[0] = ins_addr & m_icache.get_yzmask ();
	      //m_cmd.address = m_addresses_tab;
	      m_cmd.address = ins_addr & m_icache.get_yzmask ();
	      m_cmd.cmd = vci_param::CMD_READ;
	      m_cmd.nwords = m_icache.get_nwords ();
	      m_cmd.buf = m_read_buffer_ins;
	      m_cmd.be = 0xF;
	      m_cmd.contig = true;
              m_cmd.srcid = 0;
	      m_cmd.trdid = 0;
	      m_cmd.pktid = vci_param::CMD_READ;
	      p_vci.send (&m_cmd, c0.time ());
	      m_vci_pending = true;
	      wait (m_rsp_received);
	      c0.update_time (m_rsptime);
	      m_icache.update (ins_addr, m_read_buffer_ins);
	      ins_ber = m_read_error;
	      ins_rdata = m_icache.read (ins_addr);
	    }
	  else
	    {			// fetch instruction hit
	      ins_ber = false;
	      ins_rdata = m_icache.read (ins_addr);
	    }
	}			// endif ireq

    }

  tmpl ( /**/)::VciXcache(
	  sc_core::sc_module_name name,
	  int id )
			   : soclib::tlmt::BaseModule (name),
			   m_iss (id), m_wbuf (100), m_vci_pending (false),
			   m_counter (0), m_lookahead (10), m_dcache (16, 8),
			   m_icache (16, 8),
			   p_vci("vci",
					 new tlmt_core::tlmt_callback<VciXcache,vci_rsp_packet<vci_param>*>(
						 this,&VciXcache<iss_t,vci_param>::rspReceived),&c0)
    {
	    p_irq = (tlmt_core::tlmt_in<bool>*)malloc(sizeof(tlmt_core::tlmt_in<bool>)*iss_t::n_irq);
	    for (int32_t i = 0 ; i < iss_t::n_irq ; i++) {
		std::ostringstream o;
		o << "irq[" << i << "]";
	        new(&p_irq[i])tlmt_core::tlmt_in<bool> (o.str(),
		       new tlmt_core::tlmt_callback < VciXcache,
                   bool >(this, &VciXcache < iss_t,
                                     vci_param >::irqReceived, (void*)(long)i));
	    }
      m_iss.reset ();
      SC_THREAD (execLoop);
    }

}}
