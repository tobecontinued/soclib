#include "soclib/timer.h"
#include "soclib/simhelper.h"
#include "system.h"

#include "../segmentation.h"

int main(void)
{
	const int cpu = procnum();
	char c[80];
	int i;

	uputs("Hello from here\n");
	
	{
		int fd = open("test", O_RDONLY, 0);
		for ( i=0; i<26; ++i ) {
			char *bla = c+i;
			read(fd, bla, 27);
			(bla)[27] = 0;
			uputs(bla);
			lseek(fd, 0, 0);
		}
		close(fd);
	}

	{
		int fd = open("test2", O_WRONLY|O_CREAT, 0644);
		if ( fd < 0 ) {
			uputs("Failed open\n");
			puti(errno);
		}
		for ( i=0; i<26; ++i ) {
			char *bla = c+i;
			int chr;
			for ( chr=0; chr<26; chr++ )
				(bla)[chr] = 'A'+chr;
			(bla)[26] = '\n';
			write(fd, bla, 27);
		}
		close(fd);
	}	

	for ( i=0; i<255; ++i )
		asm volatile("nop");
	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_SC_STOP,
		0);
	while (1);
	return 0;
}
