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
 */

#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/endian.h"
#include "common/fb_controller.h"

namespace soclib {
namespace common {

using namespace soclib;

uint32_t FbController::pixelGet(int x, int y)
{
	char *p = (char *)m_surface + (y*m_width+x)*m_bpp/8;

	switch(m_bpp/8) {
	case 1:
		return *p;

	case 2:
		return *(unsigned short *)p;

	case 3:
 		if(__BYTE_ORDER == __BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
 		else
 			return p[0] | p[1] << 8 | p[2] << 16;

	case 4:
		return *(uint32_t *)p;

	default:
		return 0;
	}
}

void FbController::pixelPut(int x, int y, uint32_t pixel)
{
	char *p = (char *)m_surface + (y*m_width+x)*m_bpp/8;

	switch(m_bpp/8) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(unsigned short *)p = pixel;
		break;

	case 3:
		if(__BYTE_ORDER == __BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(uint32_t *)p = pixel;
		break;
	}
}

FbController::VciFrameBuffer(
    const std::string &basename,
    unsigned long width,
    unsigned long height)
{
	char name[32];
	m_surface = NULL;

	m_width = width;
	m_height = height;

    std::string tmpname = std::string("/tmp/") + basename + ".rawXXXXXX";

	strncpy( name, tmpname.c_str(), sizeof(name));
	m_map_fd = mkstemp(name);
	if ( m_map_fd < 0 ) {
		perror("open");
        throw soclib::exception::RunTimeError("Cant create file");
	}

	lseek(m_map_fd, m_width*m_height*m_bpp/8-1, SEEK_SET);
	write(m_map_fd, "", 1);
	lseek(m_map_fd, 0, SEEK_SET);
	
	m_surface = (uint32_t*)mmap(0, m_width*m_height*m_bpp/8, PROT_WRITE|PROT_READ, MAP_FILE|MAP_SHARED, m_map_fd, 0);
	if ( m_surface == ((uint32_t *)-1) ) {
		perror("mmap");
        throw soclib::exception::RunTimeError("Cant mmap file");
	}
	
	memset(m_surface, 0, m_width*m_height*m_bpp/8);
	
	{
        std::vector<std::string> argv;
        std::ostringstream o;

        argv.push_back("soclib-fb");
        o << m_width;
        argv.push_back(o.str());
        o.str("");
        o << m_height;
        argv.push_back(o.str());
        o.str("");
        o << m_bpp;
        argv.push_back(o.str());
        o.str("");
        argv.push_back(tmpname);

        m_screen_process = new common::ProcessWrapper("soclib-fb", argv);
	}
}

FbController::~VciFrameBuffer()
{
    delete m_screen_process;
}

void FbController::update()
{
	m_screen_process->kill(SIGUSR1);
}


// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

