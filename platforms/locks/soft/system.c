#include "system.h"
#include "stdio.h"
#include "soclib/timer.h"

int puts(const char *str)
{
	while (*str)
		putchar(*str++);
	return 0;
}

extern uint32_t lock;

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	lock_lock(&lock);
	printf("%s type=0x%x at 0x%x dptr=0x%x\n", __FUNCTION__, type, (uint32_t)execptr, (uint32_t)dataptr);
	lock_unlock(&lock);
	exit(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	printf("%s irq=%d\n", __FUNCTION__, irq);
	exit(1);
}

void interrupt_hw_handler(unsigned int irq)
{
	printf("%s irq=%d\n", __FUNCTION__, irq);
	exit(1);
}

static inline volatile uint32_t ll( uint32_t *addr )
{
	uint32_t ret;
	__asm__ __volatile__("ll %0, 0(%1)":"=r"(ret):"p"(addr));
	return ret;
}

static inline volatile uint32_t sc( uint32_t *addr, uint32_t value )
{
	__asm__ __volatile__("sc %0, 0(%1)":"=r"(value):"p"(addr), "0"(value):"memory");
	return !value;
}

void lock_lock( uint32_t *lock )
{
	uint32_t n = procnum()+1;
	__asm__ __volatile__(
		".set push        \n\t"
		".set noreorder   \n\t"
		".set noat        \n\t"
		"1:               \n\t"
		"ll    $2, 0(%0)  \n\t"
		"beq   $2, %1, 2f \n\t"
		"nop              \n\t"
		"bnez  $2, 1b     \n\t"
		"or    $1, $0, %1 \n\t"
		"sc    $1, 0(%0)  \n\t"
		"beqz  $1, 1b     \n\t"
		"nop              \n\t"
		"2:               \n\t"
		".set pop         \n\t"
		:
		: "p"(lock), "r"(n)
		: "$1", "$2", "memory"
		);
}

void lock_unlock( uint32_t *lock )
{
	*lock = 0;
}
