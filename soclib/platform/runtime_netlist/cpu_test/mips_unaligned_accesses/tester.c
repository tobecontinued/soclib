#include <stdint.h>
#include <stdio.h>

void test( uint32_t addr, uint32_t val )
{
	uint32_t vval;
#if defined MIPSEL
	asm volatile("lwr %0, 0(%1) \n\t"
				 "lwl %0, 3(%1) \n\t"
				 : "=&r" (vval)
				 : "r" (addr)
				 );
#elif defined MIPSEB
	asm volatile("lwl %0, 0(%1) \n\t"
				 "lwr %0, 3(%1) \n\t"
				 : "=&r" (vval)
				 : "r" (addr)
				 );
#endif
	assert( vval == val );
}
