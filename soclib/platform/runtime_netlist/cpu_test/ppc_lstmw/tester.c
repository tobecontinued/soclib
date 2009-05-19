#include <stdint.h>
#include <stdio.h>

void cpy_10_regs( void* dest, void* src )
{
	asm volatile(
		"lmw  22, 0(%1)     \n\t"
		"stmw 22, 0(%0)     \n\t"
		:
		: "r" (dest), "r" (src)
		: "r22", "r23", "r24", "r25", "r26"
		, "r27", "r28", "r29", "r30", "r31"
		);
}

void cpy_10_regs2( void* dest, void* src )
{
	asm volatile(
		"li    22, 40        \n\t"
		"mtxer 22            \n\t"
		"lswx  22, 0, %1     \n\t"
		"stswx 22, 0, %0     \n\t"
		:
		: "r" (dest), "r" (src)
		: "r22", "r23", "r24", "r25", "r26"
		, "r27", "r28", "r29", "r30", "r31"
		);
}
