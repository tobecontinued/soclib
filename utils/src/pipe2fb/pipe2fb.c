/*
 * Wrapper for Xramdac, which will enable getting data from stdin.
 * Useful for connection with Fifo components.
 * 
 * This file is part of Kolagen, development environment for static
 * SoC applications.
 * 
 * This file is distributed under the terms of the GNU General Public
 * Lisence.
 * 
 * Copyright (c) 2005, Nicolas Pouillon, <nipo@ssji.net>
 *     Laboratoire d'informatique de Paris 6 / ASIM, France
 * 
 *  $Id: pipe2ramdac.c,v 1.1 2005/10/17 21:16:47 nipo Exp $
 * 
 */

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#include "fb_controller.h"

static int must_quit = 0;

void quit(int signal)
{
    must_quit = 1;
}

int main(int argc, char **argv)
{
    int width = 0, height = 0, nw = 0, nh = 0;
    int i, flags;
    int getsize_each_time = 1, do_get_size = 1;
	unsigned char *frame_buffer;

    struct sigaction sa;
    
    sigaction( SIGQUIT, NULL, &sa );
    sa.sa_handler = quit;
    sigaction( SIGQUIT, &sa, NULL );
    sigaction( SIGINT, &sa, NULL );
    sigaction( SIGCHLD, &sa, NULL );

    if ( argc < 3 ) {
		exit(1);
    }
	
	width = atoi(argv[1]);
	height = atoi(argv[2]);
    
    frame_buffer = fb_init(width, height);

    while ( ! must_quit ) {
		if ( read( 0, frame_buffer, width*height ) <= 0 )
			must_quit = 1;
		fb_update();
	}
	fb_cleanup();
	return 0;
}
