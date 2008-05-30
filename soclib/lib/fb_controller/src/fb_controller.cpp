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

#include <string>
#include <sstream>

#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>

#include "soclib_endian.h"
#include "exception.h"
#include "fb_controller.h"

namespace soclib {
namespace common {

FbController::FbController(
    const std::string &basename,
    unsigned long width,
    unsigned long height)
	: m_width(width),
      m_height(height)
{
	char name[32];
	m_surface = NULL;

    std::string tmpname = std::string("/tmp/") + basename + ".rawXXXXXX";

	strncpy( name, tmpname.c_str(), sizeof(name));
	m_map_fd = mkstemp(name);
	if ( m_map_fd < 0 ) {
		perror("open");
        throw soclib::exception::RunTimeError("Cant create file");
	}

	lseek(m_map_fd, m_width*m_height*2-1, SEEK_SET);
	write(m_map_fd, "", 1);
	lseek(m_map_fd, 0, SEEK_SET);
	
	m_surface = (uint32_t*)mmap(0, m_width*m_height*2, PROT_WRITE|PROT_READ, MAP_FILE|MAP_SHARED, m_map_fd, 0);
	if ( m_surface == ((uint32_t *)-1) ) {
		perror("mmap");
        throw soclib::exception::RunTimeError("Cant mmap file");
	}
	
	memset(m_surface, 128, m_width*m_height*2);

    char *soclib_fb = std::getenv("SOCLIB_FB");
    m_headless_mode = ( soclib_fb && !strcmp(soclib_fb, "HEADLESS") );
    
	if (m_headless_mode == false) {
        std::vector<std::string> argv;
        std::ostringstream o;

        argv.push_back("soclib-fb");
        o << m_width;
        argv.push_back(o.str());
        o.str("");
        o << m_height;
        argv.push_back(o.str());
        argv.push_back("32");
        argv.push_back(name);

        m_screen_process = new ProcessWrapper("soclib-fb", argv);
	}
}

FbController::~FbController()
{
	if (m_headless_mode == false)
        delete m_screen_process;
}

void FbController::update()
{
	if (m_headless_mode == false)
        m_screen_process->write("", 1);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

