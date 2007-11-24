#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/uio.h>
#include <unistd.h>
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

void handle_display( uint8_t *fb, long width, long height, long asked_bpp )
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

	surface = SDL_SetVideoMode(width, height, asked_bpp, SDL_SWSURFACE);
	if ( !surface ) {
		printf("SDL surface init failed: %s\n", SDL_GetError());
		exit(3);
	}

	while (!do_exit) {
		int line, col;
		SDL_Event event;
		if ( feof(stdin) )
			exit(0);
		if ( getchar() < 0 )
			exit(0);
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
            case SDL_QUIT:
                exit(0);
			}
		}
		if(SDL_MUSTLOCK(surface)) {
			if(SDL_LockSurface(surface) < 0)
				return;
		}
		for ( line=0; line<height; line++ ) {
			for ( col=0; col<width; ++col ) {
#ifndef DEBUG
				float yy  = fb[line*width+col]/128.-1.;
				float cb = fb[width*height	   + (line/2)*width + col/2]/128.-1.;
				float cr = fb[width*height*3/2 + (line/2)*width + col/2]/128.-1.;
/*
  cb *= .436;
  cr *= .615;
*/				
				float rb = yy - 0.00004*cb + 1.140*cr;
				float vb = yy - 0.395*cb - 0.581*cr;
				float bb = yy + 2.032*cb - 0.0005*cr;
/*
  if (cr > .615)
  printf("cr = %f\n", cr);
  if (cb > .436)
  printf("cb = %f\n", cb);
*/
				rb = (rb+1.)*128.;
				vb = (vb+1.)*128.;
				bb = (bb+1.)*128.;
				
				uint8_t r = rb<0?0:(rb>255?255:rb);
				uint8_t v = vb<0?0:(vb>255?255:vb);
				uint8_t b = bb<0?0:(bb>255?255:bb);

				uint8_t y  = fb[line*width+col];
#else
				uint8_t y  = fb[line*width+col];
				uint8_t r = 0;
				uint8_t v = y;
				uint8_t b = 0;
#endif

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
					if(__BYTE_ORDER == __BIG_ENDIAN) {
						p[0] = r;
						p[1] = v;
						p[2] = b;
					} else {
						p[0] = b;
						p[1] = v;
						p[2] = r;
					}
					break;
				}
			}
		}
		if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
		SDL_UpdateRect(surface,0,0,width,height);
	}
}

void usage(char *progname)
{
	printf("%s width height bpp mmap_file\n", progname);
}

int main( int argc, char **argv )
{
	long width, height, depth;
	int buffer_fd;
	char *file;
	void *mmap_res;
	struct sigaction sa;
	if ( argc < 5 ) {
		usage(argv[0]);
		exit(1);
	}

	sa.sa_handler = sig_catcher;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (SIGUSR1, &sa, NULL);
	sigaction (SIGTERM, &sa, NULL);

	fcntl( 1, F_SETFL, O_ASYNC );

	width = strtol(argv[1], NULL, 0);
	height = strtol(argv[2], NULL, 0);
	depth = strtol(argv[3], NULL, 0);
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
	
	handle_display( mmap_res, width, height, depth);
	return 0;
}
