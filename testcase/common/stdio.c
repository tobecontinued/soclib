#include <stdarg.h>
#include "system.h"

#include "soclib/simhelper.h"

int time(int *ret)
{
	int t;
#if defined(__mips__)
	t = get_cp0(9);
#elif defined(__PPC__)
	t = spr_get(284);
#else
#error No cycle counter
#endif
	if ( ret )
		*ret = t;
	return t;
}

char *strcpy( char *dst, const char *src )
{
	while ( (*dst++ = *src++) )
		;
	return dst;
}

#define SIZE_OF_BUF 16

int printf(const char *fmt, ...)
{
	register char *tmp;
	int val, i, count = 0;
	char buf[SIZE_OF_BUF];
    va_list ap;
    va_start(ap, fmt);

	while (*fmt) {
		while ((*fmt != '%') && (*fmt)) {
			putc(*fmt++);
			count++;
		}
		if (*fmt) {
        again:
			fmt++;
			switch (*fmt) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
			case ' ':
				goto again;
			case '%':
				putc('%');
				count++;
				goto next;
			case 'c':
				putc(va_arg(ap, int));
				count++;
				goto next;
			case 'd':
			case 'i':
				val = va_arg(ap, int);
				if (val < 0) {
					val = -val;
					putc('-');
					count++;
				}
				tmp = buf + SIZE_OF_BUF;
				*--tmp = '\0';
				do {
					*--tmp = val % 10 + '0';
					val /= 10;
				} while (val);
				break;
			case 'u':
				val = va_arg(ap, unsigned int);
				tmp = buf + SIZE_OF_BUF;
				*--tmp = '\0';
				do {
					*--tmp = val % 10 + '0';
					val /= 10;
				} while (val);
				break;
			case 'o':
				val = va_arg(ap, int);
				tmp = buf + SIZE_OF_BUF;
				*--tmp = '\0';
				do {
					*--tmp = (val & 7) + '0';
					val = (unsigned int) val >> 3;
				} while (val);
				break;
			case 's':
				tmp = va_arg(ap, char *);
				if (!tmp)
					tmp = "(null)";
				break;
			case 'p':
			case 'x':
			case 'X':
				val = va_arg(ap, int);
				tmp = buf + SIZE_OF_BUF;
				*--tmp = '\0';
				i = 0;
				do {
					char t = '0'+(val & 0xf);
					if (t > '9')
						t += 'a'-'9'-1;
					*--tmp = t;
					val = ((unsigned int) val) >> 4;
					i++;
				} while (val);
				if (*fmt == 'p') {
					while (i < 8) {
						*--tmp = '0';
						i++;
					}
					*--tmp = 'x';
					*--tmp = '0';
				}
				break;
			default:
				putc(*fmt);
				count++;
				goto next;
			}
			while (*tmp) {
				putc(*tmp++);
				count++;
			}
		next:
			fmt++;
		}
	}
	va_end(ap);
	return count;
}

int strcmp( const char *ref0, const char *ref1 )
{
	while (*ref0) {
		int d = *ref0 - *ref1;
		if ( d )
			return d;
		++ref0; ++ref1;
	}
	return *ref0-*ref1;
}

static void *heap_pointer = 0;
extern void _heap();

static inline char *align(char *ptr)
{
	return (void*)((unsigned long)(ptr+15)&~0xf);
}

void *malloc( unsigned long sz )
{
	char *rp;
	if ( ! heap_pointer )
		heap_pointer = align((void*)_heap);
	rp = heap_pointer;
	heap_pointer = align(rp+sz);
	return rp;
}

void abort()
{
	printf("Aborted\n");
	exit(1);
}

void *memcpy( void *_dst, void *_src, unsigned long size )
{
	unsigned char *dst = _dst;
	unsigned char *src = _src;
	while (size--) {
		*dst++ = *src++;
	}
	return _dst;
}

void exit(int retval)
{
	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_END_WITH_VAL,
		retval);
	while(1)
		;
}

void puts(const char *s)
{
	while (*s)
		putc(*s++);
}
