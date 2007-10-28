#ifndef USER_H_
#define USER_H_

#include "stdint.h"
#include "soclib/tty.h"
#include "soclib/fd_access.h"
#include "../segmentation.h"

#define base(x) (void*)(x##_BASE)

void uputs(const char *);

#ifdef __mips__

#define get_cp0(x)									\
({unsigned int __cp0_x;								\
__asm__("mfc0 %0, $"#x:"=r"(__cp0_x));	\
__cp0_x;})

static inline int procnum()
{
    return (get_cp0(15)&0x3ff);
}

#endif

static inline void putc(const char x)
{
	soclib_io_write8(
		base(TTY),
		procnum()*TTY_SPAN+TTY_WRITE,
		x);
}

static inline char getc()
{
	return soclib_io_read8(
		base(TTY),
		procnum()*TTY_SPAN+TTY_READ);
}

#endif

extern int errno;

int open( const char *path, const int how, const int mode );
int close( const int fd );
int read( const int fd, const void *buffer, const size_t len );
int write( const int fd, const void *buffer, const size_t len );
int lseek( const int fd, const off_t offset, int whence );
