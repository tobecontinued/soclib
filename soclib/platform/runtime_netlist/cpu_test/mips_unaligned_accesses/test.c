#include <stdint.h>
#include <stdio.h>

void test( uint32_t addr, uint32_t val )
{
	uint32_t vval;
	asm volatile("lwr %0, -1(%1) \n\t"
				 "lwl %0, 2(%1) \n\t"
				 : "=&r" (vval)
				 : "r" (addr)
				 );
	assert( vval == val );
}
