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

#include "common/exception.h"
#include "common/xterm_wrapper.h"

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/select.h>
#include <vector>

namespace soclib { namespace common {

XtermWrapper::XtermWrapper(const std::string &name)
{
    m_fd = getmpt();

    m_pid = fork();
    if ( m_pid < 0 ) {
        throw soclib::exception::RunTimeError("fork() failed");
    }

    if ( m_pid ) {
        // parent

		// Xterm gives out a line with its window ID; we dont care
		// about it and we dont want it to get through to the
		// simulated platform... Let's strip it.
		char buf;
		int r;
		do {
			r = ::read( m_fd, &buf, 1 );
		} while( buf != '\n' && buf != '\r' );

		// Change xterm CR/LF mode
		::write( m_fd, "\x1b[20h", 5 );

		// And change our modes
		struct termios attrs;
		tcgetattr( m_fd, &attrs );
#if defined(cfmakeraw)
		cfmakeraw( &attrs );
#endif
		attrs.c_iflag |= IGNCR;
		tcsetattr( m_fd, TCSANOW, &attrs );

		fcntl( m_fd, F_SETFL, O_NONBLOCK );
		// The last char we want to strip is not always present,
		// so dont block if not here
		r = ::read( m_fd, &buf, 1 );
    } else {
        // child
		const char *pname = ptsname(m_fd);
		const char maxlen = 16;

        grantpt(m_fd);
        unlockpt(m_fd);

		char pname_arg[maxlen];
		int fd = open(pname, O_RDWR);
        if ( fd < 0 )
            goto error;
		snprintf(pname_arg, maxlen, "-S0/%d", fd);

		// Set modes
		struct termios attrs;
		tcgetattr( fd, &attrs );
#if defined(cfmakeraw)
		cfmakeraw( &attrs );
#endif
		attrs.c_oflag |= ONLRET;
		attrs.c_oflag &= ~ONLCR;
		tcsetattr( fd, TCSANOW, &attrs );

        unlink(name.c_str());

        execlp("xterm", "xterm",
               pname_arg,
               "-T", name.c_str(),
               "-bg", "black",
               "-fg", "green",
               "-l", "-lf", name.c_str(),
               "-geometry", "80x25",
               NULL);
        perror("execlp");
        /** \todo Replace this with some advertisement mechanism, and
         * report error back in parent process in a clean way
         */
    error:
        ::kill(getppid(), SIGKILL);
        _exit(2);
    }
}
    
XtermWrapper::~XtermWrapper()
{
    kill(SIGTERM);
    close(m_fd);
}

ssize_t XtermWrapper::read( void *buffer, size_t len )
{
    return ::read( m_fd, buffer, len );
}

ssize_t XtermWrapper::write( const void *buffer, size_t len )
{
    return ::write( m_fd, buffer, len );
}

bool XtermWrapper::poll()
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_fd, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int retval = select(m_fd+1, &rfds, NULL, NULL, &tv);
    
    if (retval == -1) {
        perror("select()");
        return false;
    }
    return retval;
}

void XtermWrapper::kill(int sig)
{
    ::kill(m_pid, sig);
}

int XtermWrapper::getmpt()
{
    int pty;

    /* Linux style */
    pty = open("/dev/ptmx", O_RDWR|O_NOCTTY);
    if ( pty >= 0 )
        return pty;

    /* scan Berkeley-style */
    char name[] = "/dev/ptyp0";
    int tty;
    while (access(name, 0) == 0) {
        if ((pty = open(name, O_RDWR)) >= 0) {
            name[5] = 't';
            if ((tty = open(name, O_RDWR)) >= 0) {
                close(tty);
                return pty;
            }
            name[5] = 'p';
            close(pty);
        }

        /* get next pty name */
        if (name[9] == 'f') {
            name[8]++;
            name[9] = '0';
        } else if (name[9] == '9')
            name[9] = 'a';
        else
            name[9]++;
    }
    errno = ENOENT;
    return -1;
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

