#include "system.h"

void putchar( const char x )
{
	putc(x);
}

void puts( const char *x )
{
	uputs(x);
}

void uputs(const char *str)
{
	while (*str)
		putc(*str++);
}

void puti(const int i)
{
	if ( i>10 )
		puti(i/10);
	putc(i%10+'0');
}

uint32_t run_cycles()
{
#if defined(__mips__)
	return get_cp0(1);
#elif defined(__PPC__)
	return dcr_get(3);
#else
	return 0;
#endif
}

void interrupt_ex_handler(
	unsigned int type, void *execptr,
	void *dataptr, void *regtable,
	void *stackptr)
{
	printf("Exception at 0x%x: 0x%x\n", execptr, type);
	exit(1);
}

void interrupt_sys_handler(unsigned int irq)
{
	uputs(__FUNCTION__);
	putc('\n');
	exit(1);
}

void interrupt_hw_handler(unsigned int irq)
{
	int i;

/* 	uputs(__FUNCTION__); */
/* 	puti(irq); */
/* 	putc('\n'); */

	for (i=0; i<8;++i) {
		if (irq&1)
			break;
		irq>>=1;
	}

	switch (i) {
	case 0:   /* SwIt 0 */
		break;
	case 1:   /* SwIt 1 */
		break;
	case 2:   /* It 0   */
		break;           
	case 4:   /* It 2   */
		break;           
	case 5:   /* It 3   */
		break;           
	case 6:   /* It 4   */
		break;           
	case 7:   /* It 5   */
		break;
	default:
		break;
	}

	exit(1);
}
