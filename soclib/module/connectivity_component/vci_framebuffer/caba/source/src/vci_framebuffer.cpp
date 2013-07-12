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
 * 
 * Maintainers: alain
 */

#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/vci_framebuffer.h"
#include "soclib_endian.h"

namespace soclib {
namespace caba {

using namespace soclib;

#define tmpl(t) template<typename vci_param> t VciFrameBuffer<vci_param>

////////////////////////
tmpl(void)::transition()
{
	if ( not p_resetn ) 
    {
		r_fsm_state  = IDLE;
		return;
	}

    // buffer display
	switch ( m_defered_timeout ) 
    {
	    case 0:   break;
	    case 1:   m_fb_controller.update();
	    default:  --m_defered_timeout;
	}
	
    // VCI FSM
    switch ( r_fsm_state.read() )
    {
        //////////
        case IDLE:   // consume one VCI CMD flit / no response in this state
        {
            if ( not p_vci.cmdval.read() ) break;

            vci_addr_t seg_base;
            vci_addr_t address   = p_vci.address.read();
            vci_be_t   be        = p_vci.be.read();
            bool       seg_error = true;

            std::list<soclib::common::Segment>::iterator seg;
            for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ )
            {
                if ( seg->contains(address) )
                {
                    seg_base = seg->baseAddress();
                    seg_error  = false;
                    break;
                }
            }

            r_seg_base = seg_base;
            r_srcid    = p_vci.srcid.read();
            r_trdid    = p_vci.trdid.read();
            r_pktid    = p_vci.pktid.read();

            if ( not seg_error and (p_vci.cmd.read() == vci_param::CMD_WRITE) )
            {
                if ( (vci_param::B == 4) or (p_vci.be.read() == 0x0F) )
                {
                    size_t     index = (size_t)((address - seg_base)>>2);
                    uint32_t*  tab   = (uint32_t*)m_fb_controller.surface();
                    tab[index]       = p_vci.wdata.read();
                    r_index          = index + 1;
                    if ( not p_vci.eop.read() ) r_fsm_state = WRITE_32;
                    else                        r_fsm_state = WRITE_RSP;
	                m_defered_timeout  = 30;
                }
                else if ( (vci_param::B == 8) and (p_vci.be.read() == 0xFF) )
                {
                    size_t     index = (size_t)((address - seg_base)>>3);
                    uint64_t*  tab   = (uint64_t*)m_fb_controller.surface();
                    tab[index]       = p_vci.wdata.read();
                    r_index          = index + 1;
                    if ( not p_vci.eop.read() ) r_fsm_state = WRITE_64;
                    else                        r_fsm_state = WRITE_RSP;
	                m_defered_timeout  = 30;
                }
                else
                {
                    std::cout << "VCI_FRAMEBUFFER ERROR " << name()
                              << " : VCI BE must be 0XF or 0xFF" << std::endl; 
                    r_fsm_state = ERROR;
                }
            }
            else if ( not seg_error and (p_vci.cmd.read() == vci_param::CMD_READ) )
            {

                if ( (vci_param::B == 4) or (p_vci.be.read() == 0x0F) )
                {
                    r_index = (size_t)((address - r_seg_base.read())>>2);
	                r_flit_count   = p_vci.plen.read()>>2;
                    r_fsm_state    = READ_32;
                }
                else if ( (vci_param::B == 8) and (p_vci.be.read() == 0xFF) )
                {
                    r_index = (size_t)((address - r_seg_base.read())>>8);
                    r_flit_count   = p_vci.plen.read()>>3;
                    r_fsm_state    = READ_64;
                }
                else
                {
                    std::cout << "VCI_FRAMEBUFFER ERROR " << name()
                              << " : VCI BE must be 0XF or 0xFF" << std::endl; 
                    r_fsm_state = ERROR;
                }
            }
            else if ( p_vci.eop.read() )
            {
                std::cout << "VCI_FRAMEBUFFER ERROR " << name()
                          << " : only VCI READ and WRITE command are supported" << std::endl; 
                r_fsm_state = ERROR;
            }
            break;
        }
        /////////////
        case READ_32:  // return a 32 bits data word in case of read
        { 
            if ( p_vci.rspack.read() )
            {
                r_flit_count = r_flit_count.read() - 1;
                r_index      = r_index.read() + 1;
                if ( r_flit_count.read() == 1 )  r_fsm_state = IDLE;
            }
            break;
        }
        /////////////
        case READ_64:  // return a 64 bits data word in case of read
        { 
            if ( p_vci.rspack.read() )
            {
                r_flit_count = r_flit_count.read() - 1;
                r_index      = r_index.read() + 1;
                if ( r_flit_count.read() == 1 )  r_fsm_state = IDLE;
            }
            break;
        }
        //////////////
        case WRITE_32:  // write another 32 bits word in buffer
        {
            vci_addr_t   address = p_vci.address.read();
            vci_be_t     be      = p_vci.be.read();
            size_t       index   = (size_t)((address - r_seg_base.read())>>2);
            if ( (r_index.read() != index) or (be != 0xF) )
            {
                std::cout << "VCI_FRAMEBUFFER ERROR " << name()
                          << " : addresses must be contiguous and BE = 0xF"
                          << " in a 32 bits write burst" << std::endl;
                r_fsm_state = ERROR;
            }
            uint32_t* tab = (uint32_t*)m_fb_controller.surface();
            tab[index] = p_vci.wdata.read();
	        m_defered_timeout  = 30;
            r_index = index + 1;
            if ( p_vci.eop.read() ) r_fsm_state = WRITE_RSP;
            break;
        }
        //////////////
        case WRITE_64:  // write another 64 bits word in buffer
        {
            vci_addr_t   address = p_vci.address.read();
            vci_be_t     be      = p_vci.be.read();
            size_t       index   = (size_t)((address - r_seg_base.read())>>3);
            if ( (r_index.read() != index) or (be != 0xFF) )
            {
                std::cout << "VCI_FRAMEBUFFER ERROR " << name()
                          << " : addresses must be contiguous and BE = 0xFF"
                          << " in a 64 bits write burst" << std::endl;
                r_fsm_state = ERROR;
            }
            uint64_t* tab = (uint64_t*)m_fb_controller.surface();
            tab[index] = p_vci.wdata.read();
	        m_defered_timeout  = 30;
            r_index = index + 1;
            if ( p_vci.eop.read() ) r_fsm_state = WRITE_RSP;
            break;
        }

        ///////////////
        case WRITE_RSP:   // returns single flit write response 
        {
            if ( p_vci.rspack.read() ) r_fsm_state = IDLE;
            break;
        }
    }
}  // end transition()

//////////////////////
tmpl(void)::genMoore()
{
    if ( (r_fsm_state.read() == IDLE)     or
         (r_fsm_state.read() == WRITE_32) or
         (r_fsm_state.read() == WRITE_64) )
    {
        p_vci.cmdack  = true;
        p_vci.rspval  = false;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = 0;
        p_vci.rtrdid  = 0;
        p_vci.rpktid  = 0;
        p_vci.rerror  = vci_param::ERR_NORMAL;
        p_vci.reop    = true;
    }
    else if ( r_fsm_state.read() == WRITE_RSP )
    {
        p_vci.cmdack  = false;
        p_vci.rspval  = true;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = r_srcid.read();
        p_vci.rtrdid  = r_trdid.read();
        p_vci.rpktid  = r_pktid.read();
        p_vci.rerror  = vci_param::ERR_NORMAL;
        p_vci.reop    = true;
    }
    else if ( r_fsm_state.read() == READ_32 )
    {
        uint32_t* tab = (uint32_t*)m_fb_controller.surface();
        
        p_vci.cmdack  = false;
        p_vci.rspval  = true;
        p_vci.rdata   = tab[r_index.read()];
        p_vci.rsrcid  = r_srcid.read();
        p_vci.rtrdid  = r_trdid.read();
        p_vci.rpktid  = r_pktid.read();
        p_vci.rerror  = vci_param::ERR_NORMAL;
        p_vci.reop    = (r_flit_count.read() == 1);
    }
    else if ( r_fsm_state.read() == READ_64 )
    {
        uint64_t* tab = (uint64_t*)m_fb_controller.surface();
        
        p_vci.cmdack  = false;
        p_vci.rspval  = true;
        p_vci.rdata   = tab[r_index.read()];
        p_vci.rsrcid  = r_srcid.read();
        p_vci.rtrdid  = r_trdid.read();
        p_vci.rpktid  = r_pktid.read();
        p_vci.rerror  = vci_param::ERR_NORMAL;
        p_vci.reop    = (r_flit_count.read() == 1);
    }
    else if ( r_fsm_state.read() == ERROR )
    {
        p_vci.cmdack  = false;
        p_vci.rspval  = true;
        p_vci.rdata   = 0;
        p_vci.rsrcid  = r_srcid.read();
        p_vci.rtrdid  = r_trdid.read();
        p_vci.rpktid  = r_pktid.read();
        p_vci.rerror  = vci_param::ERR_GENERAL_DATA_ERROR;
        p_vci.reop    = true;
    }
}

///////////////////////////
tmpl(/**/)::VciFrameBuffer(
    sc_module_name       name,
    const IntTab         &index,
    const MappingTable   &mt,
    unsigned long        width,
    unsigned long        height,
    int                  subsampling )
	: caba::BaseModule(name),
     
      p_clk("clk"),
      p_resetn("resetn"),
      p_vci("vci"),

      r_fsm_state( "r_fsm_state" ),
      r_flit_count( "r_flit_count" ),
      r_index( "r_index" ),
      r_srcid( "r_srcid" ),
      r_trdid( "r_trdid" ),
      r_pktid( "r_pktid" ),

      m_seglist( mt.getSegmentList(index) ),
      m_fb_controller( (const char *)name, width, height, subsampling )
{
    std::cout << "  - Building VciFramebuffer : " << name << std::endl;

    assert( (m_seglist.empty() == false) and
    "VCI_FRAMEBUFFER error : no segment allocated");

    std::list<soclib::common::Segment>::iterator seg;
    for ( seg = m_seglist.begin() ; seg != m_seglist.end() ; seg++ )
    {
        std::cout << "    => segment " << seg->name()
                  << " / base = " << std::hex << seg->baseAddress()
                  << " / size = " << seg->size() << std::endl; 
    }

    assert( ((vci_param::B == 4) or (vci_param::B == 8)) and
    "VCI_FRAMEBUFFER error : VCI DATA width must be 32 or 64 bits");

	m_defered_timeout = 0;

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();
	
	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();
}

/////////////////////////////
tmpl(/**/)::~VciFrameBuffer()
{
}

/////////////////////////
tmpl(void)::print_trace()
{
    const char* state_str[] = { "IDLE",
                                "READ_32",
                                "READ_64",
                                "WRITE_32",
                                "WRITE_64",
                                "WRITE_RSP",
                                "ERROR", };

    std::cout << "FRAMEBUFFER " << name()
              << " : state = " << state_str[r_fsm_state.read()] 
              << " / index = " << r_index.read()
              << " / count = " << r_flit_count.read() << std::endl; 
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

