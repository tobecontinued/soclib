#include "../segmentation.h"
#include "soclib/tty.h"
#include "uputs.h"

void uputs(unsigned int addr, const char *str)
{
	int* tty = (int*)addr;
	while (*str) {
		tty[TTY_WRITE] = *str;
		str++;
	}
}


