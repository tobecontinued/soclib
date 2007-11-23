#ifndef USER_H_
#define USER_H_

#include "soclib/tty.h"
#include "../segmentation.h"

#include "stdint.h"

void lock_lock( uint32_t * );
void lock_unlock( uint32_t * );

#define base(x) (void*)(x##_BASE)

int puts(const char *);

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

static inline int putchar(int x)
{
	soclib_io_write8(
		base(TTY),
		TTY_WRITE,
		x);
	return 0;
}

static inline char getc()
{
	return soclib_io_read8(
		base(TTY),
		TTY_READ);
}

#endif
