#include <fcntl.h>
#include <stdlib.h>
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

void sig_catcher( int sig )
{
	if ( sig != SIGUSR1 )
		exit(0);
}

void handle_display( uint32_t *fb, long width, long height, long asked_bpp )
{
	int res, bpp;
	SDL_Surface *surface;

	res = SDL_Init(SDL_INIT_VIDEO);
	if ( res < 0 ) {
		printf("SDL Init failed: %s\n", SDL_GetError());
		exit(3);
	}

	atexit(SDL_Quit);

	surface = SDL_SetVideoMode(width, height, asked_bpp, SDL_SWSURFACE);
	if ( !surface ) {
		printf("SDL surface init failed: %s\n", SDL_GetError());
		exit(3);
	}
	
	bpp = surface->format->BytesPerPixel;
	for (;;) {
		int line, col;
		pause();
		if(SDL_MUSTLOCK(surface)) {
			if(SDL_LockSurface(surface) < 0)
				return;
		}
		for ( line=0; line<height; line++ ) {
			for ( col=0; col<width; ++col ) {
				Uint32 *p = (Uint32*)((Uint8 *)surface->pixels + line*surface->pitch + col);
				void *y = (char*)fb + (line*width+col)*asked_bpp/8;
				void *u = (char*)fb + (line*width/2+width*height+col/2)*asked_bpp/8;
				void *v = (char*)fb + (line*width/2+width*height*3/2+col/2)*asked_bpp/8;
				
				unsigned char r = y - 0.00004*u + 1.140*v;
				unsigned char v = y - 0.395*u - 0.581*v;
				unsigned char b = y + 2.032*u - 0.0005*v;
				*p = (r<<16+v<<8+b);
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

	width = strtol(argv[1], NULL, 0);
	height = strtol(argv[2], NULL, 0);
	depth = strtol(argv[3], NULL, 0);
	file = argv[4];
	
	buffer_fd = open(file, O_RDONLY);
	mmap_res = mmap(0, width*height*depth*2, PROT_READ, MAP_FILE|MAP_SHARED, buffer_fd, 0);
	if ( ! mmap_res ) {
		perror("mmap");
		exit(2);
	}
	
	handle_display( mmap_res, width, height, depth);
	return 0;
}
