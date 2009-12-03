 /*
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
 *         Alain Greiner <alain.greiner@lip6.fr>, 2008
 *
 * Maintainers: alain
 */

/////////////////////////////////////////////////////////////////////////
//  This component is a multi-segments Ram controller.
//
//  Complying with the VCI advanced specification, it supports
//  "compact" VCI packets :
//  - A READ burst command packet at consecutive addresses 
//    (such as MISS requests) can contain one single cell or multiple cells. 
//    In case of single cell packet, the read burst response packet length 
//    is defined by the PLEN field. In case of multiple cell packet,
//    the zero value is supported for the PLEN field, and the response
//    packet has the same lenth as the command packet, but the throughput is
//    two cycles per word.
//  - WRITE burst command packets at consecutive addresses are supported.
//    The zero value is supported for the PLEN field, as this field is unused.
//    Write response packets contain always one single cell.
//  - The LL & SC command packets are supported, but the packet
//    length must contain one single cell.
//  - For all types of command ERROR response packets contain one single cell.
//
//  Implementation note: This component does not contain any FIFO,
//  and is controlled by a single FSM.
//  The VCI command is acknowledged and analysed in the IDLE state.
//  The relevant parameters (address, be, srcid, plen etc.) are stored 
//  in dedicated registers, but no response is sent in this state.
/////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include "vci_simple_ram.h"

namespace soclib {
namespace caba {

using namespace soclib;

#define tmpl(x) template<typename vci_param> x VciSimpleRam<vci_param>

//////////////////////////
tmpl(/**/)::VciSimpleRam(
//////////////////////////
	sc_module_name insname,
	const soclib::common::IntTab index,
	const soclib::common::MappingTable &mt,
        const soclib::common::Loader &loader,
	const uint32_t latency)
	: caba::BaseModule(insname),
      m_loader(loader),
      m_seglist(mt.getSegmentList(index)),
      m_latency(latency),

      r_llsc_buf((size_t)(1<<vci_param::S)),

      r_fsm_state("r_fsm_state"),
      r_flit_count("r_flit_count"),
      r_index("r_index"),
      r_address("r_address"),
      r_wdata("r_wdata"),
      r_be("r_be"),
      r_srcid("r_srcid"),
      r_trdid("r_trdid"),
      r_pktid("r_pktid"),
      r_contig("r_contig"),
      r_eop_cmd("r_eop_cmd"),
      r_valid("r_valid"),
      r_eop_rsp("r_eop_rsp"),
      r_latency_count("r_latency_count"),

      m_nbseg(0),

      p_resetn("resetn"),
      p_clk("clk"),
      p_vci("vci")
{
	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();
	
	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
	
        std::list<soclib::common::Segment>::iterator seg;
        for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ ) { 
            m_nbseg++;
        }
        
	m_ram = new ram_t*[m_nbseg];
        m_seg = new soclib::common::Segment*[m_nbseg];
	
        size_t i = 0;
	size_t word_size = vci_param::B; // B is VCI's cell size
        for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ ) { 
            m_ram[i] = new ram_t[(seg->size()+word_size-1)/word_size];
            m_seg[i] = &(*seg);
            i++;
	}
}

///////////////////////////
tmpl(/**/)::~VciSimpleRam()
///////////////////////////
{
	for (size_t i=0 ; i<m_nbseg ; ++i) delete [] m_ram[i];
	delete [] m_ram;
}

/////////////////////
tmpl(void)::reload()
/////////////////////
{
    for ( size_t i=0 ; i<m_nbseg ; ++i ) {
		m_loader.load(&m_ram[i][0], m_seg[i]->baseAddress(), m_seg[i]->size());
        for ( size_t addr = 0 ; addr < m_seg[i]->size()/vci_param::B ; ++addr )
            //m_ram[i][addr] = (vci_data_t)le_to_machine((unsigned int)m_ram[i][addr]);
            m_ram[i][addr] = le_to_machine(m_ram[i][addr]);
	}
}

////////////////////
tmpl(void)::reset()
////////////////////
{
	for ( size_t i=0 ; i<m_nbseg ; ++i ) {
		std::memset(&m_ram[i][0], 0, m_seg[i]->size());
	}
    m_cpt_read = 0;
    m_cpt_write = 0;
    if (m_latency) {
 	r_fsm_state = FSM_IDLE;
        r_latency_count = m_latency - 1;
    } else {
	r_fsm_state = FSM_CMD_GET;
        r_latency_count = 0;
    }
    r_llsc_buf.clearAll();
}

/////////////////////////////////////////////////////////////////////////////
tmpl(bool)::write(size_t seg, vci_addr_t addr, vci_data_t wdata, vci_be_t be)
/////////////////////////////////////////////////////////////////////////////
{
    if ( m_seg[seg]->contains(addr) ) {
        size_t index = (size_t)((addr - m_seg[seg]->baseAddress()) / vci_param::B);
	    vci_data_t cur = m_ram[seg][index];
        vci_data_t mask = vci_param::be2mask(be);
        m_ram[seg][index] = (cur & ~mask) | (wdata & mask);
        m_cpt_write++;
        return true;
    } else {
        return false;
    }
}

/////////////////////////////////////////////////////////////////
tmpl(bool)::read(size_t seg, vci_addr_t addr, vci_data_t &rdata )
/////////////////////////////////////////////////////////////////
{
    if ( m_seg[seg]->contains(addr) ) {
        size_t index = (size_t)((addr - m_seg[seg]->baseAddress()) / vci_param::B);
	    rdata = m_ram[seg][index];
        m_cpt_read++;
	    return true;
    } else {
        return false;
    }
}

/////////////////////////
tmpl(void)::transition()
/////////////////////////
{
	if (!p_resetn) {
		reset();
		reload();
		return;
	}

#ifdef SOCLIB_MODULE_DEBUG
std::cout << "************** vci_simple_ram : " << name() << std::endl;
std::cout << "r_fsm_state     = " << r_fsm_state << std::endl;
std::cout << "r_latency_count = " << r_latency_count << std::endl;
#endif

    switch ( r_fsm_state ) {
    case FSM_IDLE: 	// unreachable state if m_latency == 0 
        {
        if ( p_vci.cmdval.read() ) {
            if (r_latency_count.read() == 0) {
                r_fsm_state = FSM_CMD_GET;
                r_latency_count = m_latency - 1;
            } else {
                r_latency_count = r_latency_count.read() - 1;
            }
	}				   
	break;
	}
    case FSM_CMD_GET:
	{
        if ( !p_vci.cmdval.read() ) break;

        vci_addr_t   address = p_vci.address.read();
        bool         reached = false;
        for ( size_t index = 0 ; index<m_nbseg  && !reached ; ++index) {
           if ( m_seg[index]->contains(address) ) {
                reached = true;
                r_index = index;
            }
        } 

        r_address    = address;
        r_be         = p_vci.be.read();
        r_flit_count = p_vci.plen.read()/vci_param::B;
        r_wdata      = p_vci.wdata.read();
        r_srcid      = p_vci.srcid.read();
        r_trdid      = p_vci.trdid.read();
        r_pktid      = p_vci.pktid.read();
        r_eop_cmd    = p_vci.eop.read();
        r_contig     = p_vci.contig.read();
        r_valid      = true;
        r_eop_rsp    = false;

        if ( !reached ) {
            if ( (p_vci.cmd.read() == vci_param::CMD_WRITE) ||
                 (p_vci.cmd.read() == vci_param::CMD_STORE_COND ) ){
                // The response to a WRITE or a SC must have a size of 1 cell
                r_fsm_state = FSM_WRITE_ERROR;
            } else {
                r_fsm_state = FSM_ERROR;
            }
        } else {
            if ( p_vci.cmd.read() == vci_param::CMD_WRITE ) {   // using eop
                r_fsm_state = FSM_WRITE_BURST;
            } else if ( p_vci.cmd.read() == vci_param::CMD_READ ) {
                if ( p_vci.plen.read() == 0 ) { // plen undocumented => using eop
                    r_fsm_state = FSM_READ_WORD;
                } else {                        // plen documented => using r_flit_count
                    r_fsm_state = FSM_READ_BURST;
                }
            } else if ( p_vci.cmd.read() == vci_param::CMD_STORE_COND ) {
                r_fsm_state = FSM_SC;
                assert( p_vci.eop.read() && 
                    "VCI sc packets should be one word");
            } else if ( p_vci.cmd.read() == vci_param::CMD_LOCKED_READ ) {
                r_fsm_state = FSM_LL;
                assert( p_vci.eop.read() && 
                    "VCI ll packets should be one word");
            }
        }
        break;
        }
    case FSM_WRITE_BURST:
        {
        if ( r_valid ) {

            assert( write (r_index, r_address , r_wdata, r_be ) &&
                     "out of bounds access in a write burst" );
        }
        if ( r_valid && r_eop_cmd ) {       // last write in the burst
            if ( p_vci.rspack.read() ) {
                if( m_latency )	r_fsm_state = FSM_IDLE;
                else           	r_fsm_state = FSM_CMD_GET;
            } else {
                                r_fsm_state = FSM_WRITE_BURST_RSP;
            }
        } else {                            // not the last write
            if ( p_vci.cmdval.read() ) {
                vci_addr_t next_address = r_address.read() + (vci_addr_t)vci_param::B;
                assert( (r_contig && (next_address == p_vci.address.read())) ||
                        (!r_contig && (r_address.read() == p_vci.address.read())) );
                r_address   = p_vci.address.read();
                r_be        = p_vci.be.read();
                r_wdata     = p_vci.wdata.read();
                r_eop_cmd   = p_vci.eop.read();
                r_valid     = true;
            } else {
                r_valid     = false;
            } 
        }
        break;
        }
    case FSM_WRITE_BURST_RSP:
        {
        if( p_vci.rspack.read() )
            if( m_latency )	r_fsm_state = FSM_IDLE;
            else           	r_fsm_state = FSM_CMD_GET;
        break;
        }
    case FSM_READ_WORD:
        {
        if ( p_vci.rspack.read() ) {    // VCI response accepted
            if( m_latency )	r_fsm_state = FSM_IDLE;
            else           	r_fsm_state = FSM_CMD_GET;
        }
        break;
        }
    case FSM_READ_BURST: 
        {
        // VCI response
        if ( p_vci.rspack.read() && (r_flit_count > 0) ) {
            r_flit_count = r_flit_count - 1;
            if ( r_contig ) r_address  = r_address.read() + vci_param::B;
            }
        bool rsp_completed = r_eop_rsp || (p_vci.rspack.read() &&  (r_flit_count == 1)) ;
        r_eop_rsp = rsp_completed;

        // VCI command
        bool cmd_completed = r_eop_cmd || (p_vci.cmdval.read() && p_vci.eop.read()); 
        r_eop_cmd = cmd_completed ; 

        if ( cmd_completed && rsp_completed ) {
            if( m_latency )	r_fsm_state = FSM_IDLE;
            else           	r_fsm_state = FSM_CMD_GET;
        }
        break;
        }
    case FSM_ERROR:
        if ( p_vci.rspack.read() ) {
            if (r_flit_count == 1) {	// end of response packet
                if( m_latency )	r_fsm_state = FSM_IDLE;
                else           	r_fsm_state = FSM_CMD_GET;
            }
            r_flit_count = r_flit_count.read() - 1;
        }
        break;
    case FSM_WRITE_ERROR:
        if ( p_vci.rspack.read() ) {
            r_fsm_state = FSM_IDLE;
        }
        break;

    case FSM_LL:
        if ( p_vci.rspack.read() ) {   
            r_llsc_buf.doLoadLinked(r_address.read(), r_srcid.read());
            if( m_latency )	r_fsm_state = FSM_IDLE;
            else           	r_fsm_state = FSM_CMD_GET;
        }
        break;

    case FSM_SC:
        if ( p_vci.rspack.read() ) {    
            if ( r_llsc_buf.isAtomic(r_address.read(), r_srcid.read()) ) {
                r_llsc_buf.accessDone(r_address.read());
                write (r_index, r_address , r_wdata, r_be);
            }
            if( m_latency )	r_fsm_state = FSM_IDLE;
            else           	r_fsm_state = FSM_CMD_GET;
        }
        break;
    } // end switch fsm_state

} // end transition()

///////////////////////
tmpl(void)::genMoore()
///////////////////////
{
    switch ( r_fsm_state ) {
    case FSM_IDLE:
        p_vci.cmdack  = false;
        p_vci.rspval  = false;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = 0;
        p_vci.rtrdid  = 0;
        p_vci.rpktid  = 0;
        p_vci.rerror  = 0;
        p_vci.reop    = false;
        break;
    
    case FSM_CMD_GET:
        p_vci.cmdack  = true;
        p_vci.rspval  = false;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = 0;
        p_vci.rtrdid  = 0;
        p_vci.rpktid  = 0;
        p_vci.rerror  = 0;
        p_vci.reop    = false;
        break;
    
    case FSM_WRITE_BURST:
        if ( r_valid && r_eop_cmd ) {   // last write
            p_vci.cmdack = false;
            p_vci.rspval = true;
            p_vci.rdata  = 0;
            p_vci.rsrcid  = r_srcid.read();
            p_vci.rtrdid  = r_trdid.read();
            p_vci.rpktid  = r_pktid.read();
            p_vci.rerror = vci_param::ERR_NORMAL;
            p_vci.reop   = true;
        } else {                        // not the last write
            p_vci.cmdack = true;
            p_vci.rspval = false;
            p_vci.rdata   = 0;
            p_vci.rsrcid  = 0;
            p_vci.rtrdid  = 0;
            p_vci.rpktid  = 0;
            p_vci.rerror  = 0;
            p_vci.reop    = false;
        }
        break;

    case FSM_WRITE_BURST_RSP:
        p_vci.cmdack  = false;
        p_vci.rspval  = true;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = r_srcid.read();
        p_vci.rtrdid  = r_trdid.read();
        p_vci.rpktid  = r_pktid.read();
        p_vci.rerror  = vci_param::ERR_NORMAL;
        p_vci.reop    = true;
        break;

    case FSM_READ_BURST:
        {
        p_vci.cmdack = !r_eop_cmd;
        if ( r_flit_count > 0 ) {
            vci_data_t   rdata;
            assert( read(r_index, r_address, rdata) &&
                    "out of bounds read access" ); 
            p_vci.rspval = true;
            p_vci.rdata  = rdata;
            p_vci.rsrcid = r_srcid.read();
            p_vci.rtrdid = r_trdid.read();
            p_vci.rpktid = r_pktid.read();
            p_vci.rerror = vci_param::ERR_NORMAL;
            p_vci.reop   = (r_flit_count.read() == 1);
        } else {
            p_vci.rspval = false;
            p_vci.rdata   = 0;
            p_vci.rsrcid  = 0;
            p_vci.rtrdid  = 0;
            p_vci.rpktid  = 0;
            p_vci.rerror  = 0;
            p_vci.reop    = false;
        }
        break;
        }

    case FSM_READ_WORD:
        {
        vci_data_t   rdata;
        assert( read(r_index, r_address, rdata) &&
                "out of bounds read access" ); 
        p_vci.cmdack = false;
        p_vci.rspval = true;
        p_vci.rdata  = rdata;
        p_vci.rsrcid = r_srcid.read();
        p_vci.rtrdid = r_trdid.read();
        p_vci.rpktid = r_pktid.read();
        p_vci.rerror = vci_param::ERR_NORMAL;
        p_vci.reop   = r_eop_cmd;
        break;
        }
            
    case FSM_LL:
        {
        vci_data_t   rdata;
        assert( read(r_index, r_address, rdata) &&
                "out of bounds linked read access" ); 
        p_vci.cmdack = false;
        p_vci.rspval = true;
        p_vci.rdata  = rdata;
        p_vci.rsrcid = r_srcid.read();
        p_vci.rtrdid = r_trdid.read();
        p_vci.rpktid = r_pktid.read();
        p_vci.rerror = vci_param::ERR_NORMAL;
        p_vci.reop   = true;
        break;
        }
            
    case FSM_SC:
        p_vci.cmdack = false;
        p_vci.rspval = true;
        if ( r_llsc_buf.isAtomic(r_address.read(), r_srcid.read()) ) p_vci.rdata = 0;
        else                                           p_vci.rdata = 1;
        p_vci.rsrcid = r_srcid.read();
        p_vci.rtrdid = r_trdid.read();
        p_vci.rpktid = r_pktid.read();
        p_vci.rerror = vci_param::ERR_NORMAL;
        p_vci.reop   = true;
        break;

    case FSM_ERROR:
        p_vci.cmdack  = false;
        p_vci.rspval  = true;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = r_srcid.read();
        p_vci.rtrdid  = r_trdid.read();
        p_vci.rpktid  = r_pktid.read();
        p_vci.rerror  = vci_param::ERR_GENERAL_DATA_ERROR;
        p_vci.reop    = (r_flit_count.read() == 1);
        break;
    case FSM_WRITE_ERROR:
        p_vci.cmdack  = false;
        p_vci.rspval  = true;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = r_srcid.read();
        p_vci.rtrdid  = r_trdid.read();
        p_vci.rpktid  = r_pktid.read();
        p_vci.rerror  = vci_param::ERR_GENERAL_DATA_ERROR;
        p_vci.reop    = true;
        break;
    } // end switch fsm_state
} // end genMoore()

}} 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

