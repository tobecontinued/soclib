#include "stdlib.h"

long fibo(unsigned int n)
{
	return (n == 1 || n == 2) ? 1 : fibo(n - 1) + fibo(n - 2);
}

int main()
{
	unsigned int n;

	myputs("Hello world!\n");
	for(n = 1; n < 50; n++)
	{
		myputs("Fibo(");
		myputl(n);
		myputs(")=");
		myputl(fibo(n));
		myputs("\n");
	}
	myputs("Bye!\n");
	return 0;
}
