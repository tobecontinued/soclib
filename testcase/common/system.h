#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "soclib/tty.h"
#include "segmentation.h"

#define base(x) (void*)(x##_BASE)

#if __mips__

#define get_cp0(x)									\
({unsigned int __cp0_x;								\
__asm__("mfc0 %0, $"#x:"=r"(__cp0_x));	\
__cp0_x;})

static inline int procnum()
{
    return (get_cp0(15)&0x3ff);
}

#elif __PPC__

#define dcr_get(x)					\
({unsigned int __val;				\
__asm__("mfdcr %0, "#x:"=r"(__val));\
__val;})

#define spr_get(x)					\
({unsigned int __val;				\
__asm__("mfspr %0, "#x:"=r"(__val));\
__val;})

static inline int procnum()
{
    return dcr_get(0);
}

#elif __MICROBLAZE__


#define fsl_get(x)				  \
({unsigned int __val;			  \
__asm__("get %0, RFSL"#x:"=r"(__val));\
__val;})

static inline uint32_t fsl_getd(uint32_t no)
{
    uint32_t val;
    __asm__("getd %0, %1"
            : "=r"(val)
            : "r"(no)
        );
    return val;
}

static inline int procnum()
{
    return fsl_get(0);
}

#endif /* platform switch */

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

void exit(int retval);

#endif /* SYSTEM_H_ */
