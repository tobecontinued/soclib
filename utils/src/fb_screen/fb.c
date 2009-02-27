/*
 *
 * SOCLIB_GPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU GPLv2.
 * 
 * SoCLib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 * 
 * SOCLIB_GPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/uio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>
#include <SDL.h>


#ifdef __linux__

#  include <endian.h>

#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)

#  include <machine/endian.h>
#  define __BYTE_ORDER BYTE_ORDER
#  define __LITTLE_ENDIAN LITTLE_ENDIAN
#  define __BIG_ENDIAN BIG_ENDIAN

#else

#  ifdef __LITTLE_ENDIAN__
#	 define __BYTE_ORDER __LITTLE_ENDIAN
#  endif

#  if defined(i386) || defined(__i386__)
#	 define __BYTE_ORDER __LITTLE_ENDIAN
#  endif

#  if defined(sun) && defined(unix) && defined(sparc)
#	 define __BYTE_ORDER __BIG_ENDIAN
#  endif

#endif /* os switches */

#ifndef __BYTE_ORDER
# error Need to know endianess
#endif

static int do_exit = 0;

void sig_catcher( int sig )
{
	do_exit = 1;
}

    enum SubsamplingType {
        YUV420 = 420,
        YUV422 = 422,
        RGB = 0,
        RGB_PALETTE_256 = 256,
        BW = 1,
    };


void set_rgb( SDL_Surface *surface, size_t line, size_t col,
			  uint8_t r, uint8_t g, uint8_t b )
{
	uint8_t y = ((uint32_t)r + (uint32_t)g + (uint32_t)b) / 3;
	char *p = (char *)surface->pixels + line*surface->pitch + col*surface->format->BytesPerPixel;
	switch(surface->format->BytesPerPixel) {
	case 1:
		*p = y;
		break;

	case 2:
		*(unsigned short *)p = y<<8;
		break;

	case 3:
	case 4:
		p[1] = g;
		if (__BYTE_ORDER == __BIG_ENDIAN) {
			p[0] = r;
			p[2] = b;
		} else {
			p[0] = b;
			p[2] = r;
		}
		break;
	}

}

void transform_yuv( SDL_Surface *surface, uint8_t *fb,
					size_t width, size_t height, int subsampling )
{
	size_t line, col;
	for ( line=0; line<height; line++ ) {
		for ( col=0; col<width; ++col ) {
			float yy  = fb[line*width+col]/128.-1.;
			float cb, cr, rb, vb, bb;
			uint8_t r, v, b;
			switch (subsampling) {
			case 420:
				cb = fb[width*height     + (line/2)*(width/2) + col/2]/128.-1.;
				cr = fb[width*height*5/4 + (line/2)*(width/2) + col/2]/128.-1.;
				break;
			case 422:
				cb = fb[width*height     + (line/2)*width + col/2]/128.-1.; 
				cr = fb[width*height*3/2 + (line/2)*width + col/2]/128.-1.; 
				break;
			default:
				cb = cr = .5;
				break;
			}
			rb = ((yy - 0.00004*cb + 1.140*cr)+1.)*128.;
			vb = ((yy - 0.395*cb - 0.581*cr)+1.)*128.;
			bb = ((yy + 2.032*cb - 0.0005*cr)+1.)*128.;
				
			r = rb<0?0:(rb>255?255:rb);
			v = vb<0?0:(vb>255?255:vb);
			b = bb<0?0:(bb>255?255:bb);
			set_rgb( surface, line, col, r, v, b );
		}
	}
}

void transform_rgb( SDL_Surface *surface, uint8_t *fb,
					size_t width, size_t height )
{
	size_t line, col;
	for ( line=0; line<height; line++ ) {
		for ( col=0; col<width; ++col ) {
			uint8_t r, v, b;
			r = fb[width*line*3 + col];
			v = fb[width*line*3 + col + 1];
			b = fb[width*line*3 + col + 2];
			set_rgb( surface, line, col, r, v, b );
		}
	}
}

void transform_rgb_palette_256(
	SDL_Surface *surface, uint8_t *fb,
	size_t width, size_t height )
{
	uint8_t *palette = fb + width*height;
	size_t line, col;
	for ( line=0; line<height; line++ ) {
		for ( col=0; col<width; ++col ) {
			uint8_t idx, r, v, b;
			idx = fb[width*line + col];
			r = palette[idx*3];
			v = palette[idx*3+1];
			b = palette[idx*3+2];
			set_rgb( surface, line, col, r, v, b );
		}
	}
}

void transform_bw(
	SDL_Surface *surface, uint8_t *fb,
	size_t width, size_t height )
{
	size_t line, col;
	for ( line=0; line<height; line++ ) {
		for ( col=0; col<width; col += 8 ) {
			size_t i;
			uint8_t x = fb[(width*line + col)/8];
			for ( i=0; i<8; ++i ) {
				uint8_t v = (x&1) ? 0xff : 0;
				x >>= 1;
				set_rgb( surface, line, col+i, v,v,v );
			}
		}
	}
}


void handle_display( uint8_t *fb, long width, long height, long subsampling )
{
	int res;
	SDL_Surface *surface;

	res = SDL_Init(SDL_INIT_VIDEO);
	if ( res < 0 ) {
		printf("SDL Init failed: %s\n", SDL_GetError());
		exit(3);
	}

	atexit(SDL_Quit);

//#define DEBUG 1
#ifdef DEBUG
	height *= 2;
#endif

	surface = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
	if ( !surface ) {
		printf("SDL surface init failed: %s\n", SDL_GetError());
		exit(3);
	}

	while (!do_exit) {
		SDL_Event event;
		uint8_t data[10];
		int ret;
		struct pollfd pfd;
		pfd.fd = 0;
		pfd.events = POLLIN | POLLHUP | POLLERR;
		ret = poll( &pfd, 1, 1000 );

		while (read( 0, data, 1 ) > 0)
			;

		ret = read( 0, data, 1 );
		if ( ret <= 0 && errno != EAGAIN )
			break;

		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
            case SDL_QUIT:
                exit(0);
			}
		}

//		fprintf(stderr, "Updating... ");
		if (SDL_MUSTLOCK(surface)) {
			if(SDL_LockSurface(surface) < 0)
				return;
		}
		switch (subsampling) {
		case YUV420:
		case YUV422:
			transform_yuv( surface, fb, width, height, subsampling );
			break;
		case RGB:
			transform_rgb( surface, fb, width, height );
			break;
		case RGB_PALETTE_256:
			transform_rgb_palette_256( surface, fb, width, height );
			break;
		case BW:
			transform_bw( surface, fb, width, height );
			break;
		}
		if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
//		fprintf(stderr, "done\n");
		SDL_UpdateRect(surface,0,0,width,height);
	}
}

void usage(char *progname)
{
	printf("%s width height subsampling mmap_file\n", progname);
}

int main( int argc, char **argv )
{
	long width, height, subsampling;
	int buffer_fd;
	char *file;
	void *mmap_res;
	struct sigaction sa;
	if ( argc < 5 ) {
		usage(argv[0]);
		exit(1);
	}

	fcntl( 0, F_SETFL, O_NONBLOCK );

	sa.sa_handler = sig_catcher;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (SIGUSR1, &sa, NULL);
	sigaction (SIGTERM, &sa, NULL);
	sigaction (SIGPIPE, &sa, NULL);

	fcntl( 1, F_SETFL, O_ASYNC );

	width = strtol(argv[1], NULL, 0);
	height = strtol(argv[2], NULL, 0);
	subsampling = strtol(argv[3], NULL, 0);
	file = argv[4];
	
	buffer_fd = open(file, O_RDONLY);
	if ( buffer_fd < 0 ) {
		perror(file);
		exit(2);
	}
	mmap_res = mmap(0, width*height*2, PROT_READ, MAP_FILE|MAP_SHARED, buffer_fd, 0);
	if ( mmap_res == (void*)-1 ) {
		perror("mmap");
		exit(2);
	}

	printf("Frame buffer: %ld %ld %ld %s\n", width, height, subsampling, file);
	
	handle_display( mmap_res, width, height, subsampling);
	return 0;
}
