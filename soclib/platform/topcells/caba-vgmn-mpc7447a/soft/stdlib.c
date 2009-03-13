#include "../segmentation.h"

#define TTY_WRITE 0
#define TTY_STATUS 1
#define TTY_READ 2

void myputc(char c)
{
	*(char *)(TTY_BASE + TTY_WRITE) = c; 
}

void myputs(char *s)
{
	if(*s)
	{
		do
		{
			myputc(*s);
		} while(*++s);
	}
}

void myputl(long value)
{
	static char buffer[30];
	char *p = buffer + sizeof(buffer) - 1;
	*p = 0;

	if(!value)
	{
		*--p = '0';
	}
	else
	{
		int sign = value < 0;
		if(sign) value = -value;
		do
		{
			*--p = '0' + (value % 10);
			value = value / 10;
		} while(value);
		if(sign) *--p = '-';
	}
	myputs(p);
}
