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
 *         Mohamed Lamine KARAOUI <Mohamed.Karaoui@lip6.fr>, 2012
 */

#include <string.h>
#include <cassert>
#include <sstream>
#include <fstream>
#include <ios>
#include <iostream>
#include <cstdarg>
#include <cassert>
#include <iomanip>


#include "exception.h"
#include "vloader.h"

namespace soclib { namespace common {



VLoader::VLoader( const std::string &filename, const size_t pageSize)
{
    m_reference = filename;

    std::string::size_type at = filename.find('@');
    if ( at != std::string::npos )
    {
        std::string::size_type colon = filename.find(':', at+1);
        if ( colon == std::string::npos )
            throw soclib::exception::RunTimeError(std::string("Bad binary file ") + filename + 
                                                    "The format must look like this: path_to_map_info@0XADD0000:FLAGS");
        std::string fname = filename.substr(0, at);
        std::string address = filename.substr(at+1, colon-at-1);
        std::string flags = filename.substr(colon+1);
        m_filename = fname;
        uintptr_t addr;
        std::istringstream( address ) >> std::hex >> addr;
        m_mapAddr = addr;
        //std::cout << std::hex << "map address: " << addr << std::endl;
    }else
    {
        m_filename = filename;
        m_mapAddr = 0;
    }

    PSeg::setPageSize(pageSize);

    void *map = load_bin(m_filename);
#ifdef VLOADER_DEBUG
    std::cout << "Binary filename = " << m_filename << std::endl;
    print_mapping_info(map);
#endif

    buildMap(map);
    
#ifdef VLOADER_DEBUG
    std::cout << "parsing done" << std::endl ;
#endif

    m_psegh.check();
    
#ifdef VLOADER_DEBUG
    std::cout << m_psegh << std::endl;
#endif
}

void VLoader::print( std::ostream &o ) const
{
    std::cout << m_psegh << std::endl;
}

void VLoader::load( void *buffer, uintptr_t address, size_t length )
{
    //TODO:  A memory init value choosed by the user ?
    memset(buffer, 0, length);

#ifdef VLOADER_DEBUG
    std::cout << "Buffer: "<< buffer << ", address: "<< address << ", length: " << length << std::endl;
#endif

    const PSeg ps = m_psegh.getByAddr(address);
    
    if(ps.length() > length)
        std::cout << std::hex
            << "Warning, loading only " << length
            << " bytes to " << ps.name()
            << ", declared size (in map_info) is: " << ps.length() 
            << std::endl;
    

    std::vector<FileVAddress>::const_iterator it ;
    for(it = ps.m_fileVAddress.begin(); it < ps.m_fileVAddress.end(); it++)
    {
        /* Get a hand of the corresponding loader               */
        const std::string file = (*it).file();
        Loader  loader = m_loaders[file];//prohibit the use of const for function
        
        /* The offset of the appropriate physical address       */
        size_t offset = (*it).lma() - ps.lma();

        /* Load with (*it).m_vma() and the corresponding size  */
#ifdef VLOADER_DEBUG
        std::cout << "Loading: "*it << std::endl;
#endif

        assert(ps.lma() <= (*it).lma());

        std::cout << "Loading at (physical address): 0x" 
	              << std::hex << std::noshowbase 
                  << std::setw (8) << std::setfill('0') 
                  << (*it).lma() << " || the virtual segment: ";
        
        loader.match_load(((char *)buffer + offset), (*it).vma(), length);
        std::cout << "(" << (*it).file() << ")" << std::endl;
    }

}

void* VLoader::load_bin(std::string filename)
{
    
#ifdef VLOADER_DEBUG
    std::cout  << "Trying to load the binary blob from file '" << m_filename << "'" << std::endl;
#endif

    std::ifstream input(m_filename.c_str(), std::ios_base::binary|std::ios_base::in);

    if ( ! input.good() )
        throw soclib::exception::RunTimeError(std::string("Can't open the file: ") + m_filename);

    input.seekg( 0, std::ifstream::end );
    size_t size = input.tellg();
    input.seekg( 0, std::ifstream::beg );

    //m_data = std::malloc(size);
    m_data = new void*[size];
    if ( !m_data )
        throw soclib::exception::RunTimeError("failed malloc... No space");

    input.read( (char*)m_data, size );
    
    return m_data;
}


VLoader::~VLoader()
{
    //std::cout << "Deleted VLoader " << *this << std::endl;
    std::free(m_data);
}


/////////////////////////////////////////////////////////////////////////////
// various mapping_info data structure access functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
mapping_pseg_t* VLoader::get_pseg_base( mapping_header_t* header )
{
    return   (mapping_pseg_t*)    ((char*)header +
                                  MAPPING_HEADER_SIZE +
                                  MAPPING_CLUSTER_SIZE*header->clusters);
}
/////////////////////////////////////////////////////////////////////////////
mapping_vspace_t* VLoader::get_vspace_base( mapping_header_t* header )
{
    return   (mapping_vspace_t*)  ((char*)header +
                                  MAPPING_HEADER_SIZE +
                                  MAPPING_CLUSTER_SIZE*header->clusters +
                                  MAPPING_PSEG_SIZE*header->psegs);
}
/////////////////////////////////////////////////////////////////////////////
mapping_vseg_t* VLoader::get_vseg_base( mapping_header_t* header )
{
    return   (mapping_vseg_t*)    ((char*)header +
                                  MAPPING_HEADER_SIZE +
                                  MAPPING_CLUSTER_SIZE*header->clusters +
                                  MAPPING_PSEG_SIZE*header->psegs +
                                  MAPPING_VSPACE_SIZE*header->vspaces);
}

/////////////////////////////////////////////////////////////////////////////
// print the content of the mapping_info data structure 
////////////////////////////////////////////////////////////////////////
void VLoader::print_mapping_info(void* desc)
{
    mapping_header_t*   header = (mapping_header_t*)desc;  

    mapping_pseg_t*	    pseg    = get_pseg_base( header );;
    mapping_vspace_t*	vspace  = get_vspace_base ( header );;
    mapping_vseg_t*	    vseg    = get_vseg_base ( header );

    // header
    std::cout << std::hex << "mapping_info" << std::endl
              << " + signature = " << header->signature << std::endl
              << " + name = " << header->name << std::endl
              << " + clusters = " << header->clusters << std::endl
              << " + psegs = " << header->psegs << std::endl
             << " + ttys = " << header->ttys  << std::endl
             << " + vspaces = " << header->vspaces  << std::endl
             << " + globals = " << header->globals  << std::endl
             << " + vsegs = " << header->vsegs  << std::endl
             << " + tasks = " << header->tasks  << std::endl
             << " + syspath = " << header->syspath  << std::endl;

    // psegs
    for ( size_t pseg_id = 0 ; pseg_id < header->psegs ; pseg_id++ )
    {
        std::cout << "pseg " << pseg_id << std::endl
         << " + name = " << pseg[pseg_id].name << std::endl 
         << " + base = " << pseg[pseg_id].base << std::endl 
         << " + length = " << pseg[pseg_id].length << std::endl ;
    }

    // globals
    for ( size_t vseg_id = 0 ; vseg_id < header->globals ; vseg_id++ )
    {
        std::cout << "global vseg: " << vseg_id << std::endl
         << " + name = " << vseg[vseg_id].name << std::endl 
         << " + vbase = " << vseg[vseg_id].vbase << std::endl 
         << " + length = " << vseg[vseg_id].length << std::endl 
         << " + mode = " << (size_t)vseg[vseg_id].mode << std::endl 
         << " + ident = " << (bool)vseg[vseg_id].ident << std::endl 
         << " + psegname" << pseg[vseg[vseg_id].psegid].name << std::endl;
    }


    // vspaces
    for ( size_t vspace_id = 0 ; vspace_id < header->vspaces ; vspace_id++ )
    {
        std::cout << "***vspace: " << vspace_id << "***" << std::endl
         << " + name = " <<  vspace[vspace_id].name  << std::endl 
         << " + binpath = " <<  vspace[vspace_id].binpath  << std::endl 
         << " + vsegs = " <<  vspace[vspace_id].vsegs  << std::endl
         << " + tasks = " <<  vspace[vspace_id].tasks  << std::endl
         << " + mwmrs = " <<  vspace[vspace_id].mwmrs  << std::endl
         << " + ttys = " <<  vspace[vspace_id].ttys  << std::endl;

        for ( size_t vseg_id = vspace[vspace_id].vseg_offset ; 
              vseg_id < (vspace[vspace_id].vseg_offset + vspace[vspace_id].vsegs) ; 
              vseg_id++ )
        {
            std::cout << "private vseg: ";
            std::cout <<  vseg_id  << std::endl
             << " + name = " <<  vseg[vseg_id].name  << std::endl
             << " + vbase = " <<  vseg[vseg_id].vbase  << std::endl
             << " + length = " <<  vseg[vseg_id].length  << std::endl
             << " + mode = " <<  (size_t)vseg[vseg_id].mode  << std::endl
             << " + ident = " <<  (bool)vseg[vseg_id].ident  << std::endl
             << " + psegname = " << pseg[vseg[vseg_id].psegid].name  << std::endl << std::endl;
        }

    }
} // end print_mapping_info()

///////////////////////////////////////////////////////////////////////////
void VLoader::pseg_map( mapping_pseg_t* pseg) 
{
    std::string name(pseg->name);
    m_psegh.m_pSegs.push_back(PSeg(name, pseg->base, pseg->length));
}



///////////////////////////////////////////////////////////////////////////
void VLoader::vseg_map( mapping_vseg_t* vseg , char * file) 
{
    FileVAddress     * vs = new FileVAddress;

    
    std::string s(vseg->name);
    vs->m_name = s;
    
    vs->m_length = vseg->length;
    
    // get physical segment pointer
    PSeg *ps = &(m_psegh.get(vseg->psegid));//PSegHandler::get

    vs->m_vma = vseg->vbase;

    //in case we are handling the map_info vseg=> then the correcte file is m_filename
    if(vs->m_vma == m_mapAddr)
    {
        //std::cout << "map address: " << m_mapAddr << std::endl;
        vs->m_file = m_filename;
        m_loaders[m_filename] = *(new Loader(m_reference));
    }else
    {
        std::string f(file);
        vs->m_file = f;
    }
    
    vs->m_ident = vseg->ident;      

    if ( vseg->ident != 0 )            // identity mapping required
        ps->addIdent( *vs );
    else
        ps->add( *vs );

} // end vseg_map()


/////////////////////////////////////////////////////////////////////
void VLoader::buildMap(void* desc)
{
    mapping_header_t*   header = (mapping_header_t*)desc;  

    mapping_vspace_t*   vspace = get_vspace_base( header );     
    mapping_pseg_t*     pseg   = get_pseg_base( header ); 
    mapping_vseg_t*     vseg   = get_vseg_base( header );

    // get the psegs
#ifdef VLOADER_DEBUG
std::cout << "\n******* Storing Pseg information *********\n" << std::endl;
#endif
    for ( size_t pseg_id = 0 ; pseg_id < header->psegs ; pseg_id++ )//TO fix in boot_handler globals
    {
        pseg_map( &pseg[pseg_id]);
    }

    // map global vsegs
#ifdef VLOADER_DEBUG
std::cout << "\n******* mapping global vsegs *********\n" << std::endl;
#endif
    m_loaders[header->syspath] = *(new Loader(header->syspath));
    for ( size_t vseg_id = 0 ; vseg_id < header->globals ; vseg_id++ )
    {
        vseg_map( &vseg[vseg_id] , header->syspath);
    }

    // second loop on virtual spaces to map private vsegs
    for (size_t vspace_id = 0 ; vspace_id < header->vspaces ; vspace_id++ )
    {

#ifdef VLOADER_DEBUG
std::cout << "\n******* mapping all vsegs of " << vspace[vspace_id].name << " *********\n" << std::endl;
#endif
            
        m_loaders[(vspace[vspace_id].binpath)] = *(new Loader(vspace[vspace_id].binpath));
        for ( size_t vseg_id = vspace[vspace_id].vseg_offset ; 
              vseg_id < (vspace[vspace_id].vseg_offset + vspace[vspace_id].vsegs) ; 
              vseg_id++ )
        {
            vseg_map( &vseg[vseg_id], vspace[vspace_id].binpath ); 
        }
    } 

#ifdef VLOADER_DEBUG
    print_mapping_info(desc);
#endif

} // end buildMap()


}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

