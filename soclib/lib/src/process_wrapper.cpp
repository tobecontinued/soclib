/* -*- c++ -*-
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

#include "exception.h"
#include "process_wrapper.h"

#include <cassert>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <sys/select.h>
#include <vector>

namespace soclib { namespace common {

ProcessWrapper::ProcessWrapper(
    const std::string &cmd,
    const std::vector<std::string> &argv )
{
    int host_to_child[2];
    int child_to_host[2];
    
    pipe(&host_to_child[0]);
    pipe(&child_to_host[0]);

    m_pid = fork();
    if ( m_pid < 0 ) {
        throw soclib::exception::RunTimeError("fork() failed");
    }

    if ( m_pid ) {
        // parent
        close(host_to_child[0]);
        close(child_to_host[1]);
        m_fd_to_process = host_to_child[1];
        m_fd_from_process = child_to_host[0];

        struct termios ts;
        tcgetattr(m_fd_from_process, &ts);
        ts.c_lflag &= ~ICANON;
        ts.c_lflag &= ~ECHO;
        tcsetattr(m_fd_from_process, TCSANOW, &ts);

        tcgetattr(m_fd_to_process, &ts);
        ts.c_lflag &= ~ICANON;
        ts.c_lflag &= ~ECHO;
        tcsetattr(m_fd_to_process, TCSANOW, &ts);
   } else {
        // child
        close(host_to_child[1]);
        close(child_to_host[0]);
        const char *c_cmd = cmd.c_str();
        char **c_argv = new char *[argv.size()+1];
    
        for ( size_t n = 0; n<argv.size(); ++n )
            c_argv[n] = strdup(argv[n].c_str());
        c_argv[argv.size()] = NULL;
        close(0);
        close(1);
        dup2(host_to_child[0], 0);
        dup2(child_to_host[1], 1);
        execvp(c_cmd, c_argv);
        perror("execv");
        /** \todo Replace this with some advertisement mechanism, and
         * report error back in parent process in a clean way
         */
        ::kill(getppid(), SIGKILL);
        _exit(2);
    }
}
    
ProcessWrapper::~ProcessWrapper()
{
    kill(SIGTERM);
    close(m_fd_to_process);
    close(m_fd_from_process);
}

ssize_t ProcessWrapper::read( void *buffer, size_t len, bool block )
{
    size_t done = 0;
    while ( done < len ) {
        ssize_t r = ::read( m_fd_from_process, (uint8_t*)buffer+done, len-done );
        if ( !block )
            return r;
        assert(r>0);
        done += r;
    }
    return done;
}

ssize_t ProcessWrapper::write( const void *buffer, size_t len, bool block )
{
    size_t done = 0;
    while ( done < len ) {
        ssize_t r = ::write( m_fd_to_process, (uint8_t*)buffer+done, len-done );
        if ( !block )
            return r;
        assert(r>0);
        done += r;
    }
    return done;
}

bool ProcessWrapper::poll()
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_fd_from_process, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int retval = select(m_fd_from_process+1, &rfds, NULL, NULL, &tv);
    
    if (retval == -1) {
        perror("select()");
        return false;
    }
    return retval;
}

void ProcessWrapper::kill(int sig)
{
    ::kill(m_pid, sig);
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

