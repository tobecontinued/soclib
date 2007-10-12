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

#define FAST_STRCPY 1

#if FAST_STRCPY
char *strcpy( char *dst, const char *src )
{
	while ( (*dst++ = *src++) )
		;
	return dst;
}
#else
# define MKW(x) (x|x<<8|x<<16|x<<24)
# define STRALIGN(x) (((unsigned long)x&3)?4-((unsigned long)x&3):0)
#define UNALIGNED(x,y) (((unsigned long)x & (sizeof (unsigned long)-1)) ^ ((unsigned long)y & (sizeof (unsigned long)-1)))

char *
strcpy (char *s1, const char *s2)
{
    char           *res = s1;
    int             tmp;
    unsigned long   l;

    if (UNALIGNED(s1, s2)) {
	while ((*s1++ = *s2++));
	return (res);
    }
    if ((tmp = STRALIGN(s1))) {
	while (tmp-- && (*s1++ = *s2++));
	if (tmp != -1) return (res);
    }

    while (1) {
	l = *(const unsigned long *) s2;
	if (((l - MKW(0x1)) & ~l) & MKW(0x80)) {
	    unsigned char c;
	    while ((*s1++ = (l & 0xff))) l>>=8;
	    return (res);
	}
	*(unsigned long *) s1 = l;
	s2 += sizeof(unsigned long);
	s1 += sizeof(unsigned long);
    }
}
#endif

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

#define FAST_STRCMP 1

#if FAST_STRCMP
int strcmp_b( const char *ref0, const char *ref1 )
#else
int strcmp( const char *ref0, const char *ref1 )
#endif
{
	while (*ref0) {
		int d = *ref0 - *ref1;
		if ( d )
			return d;
		++ref0; ++ref1;
	}
	return *ref0-*ref1;
}

#if FAST_STRCMP
int strcmp( const char *ref0, const char *ref1 )
{
	const uint32_t *iref0 = (uint32_t *)ref0;
	const uint32_t *iref1 = (uint32_t *)ref1;

	if ( ! ((uint32_t)iref0 & 3) && ! ((uint32_t)iref1 & 3) )
		if (*iref0 == *iref1)
			while (*++iref0 == *++iref1);
	return strcmp_b(((char*)iref0)-1, ((char*)iref1)-1);
}
#endif

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
	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_EXCEPT_WITH_VAL,
		1);
}

void *memcpy( void *_dst, void *_src, unsigned long size )
{
	uint32_t *dst = _dst;
	uint32_t *src = _src;
	if ( ! ((uint32_t)dst & 3) && ! ((uint32_t)src & 3) )
		while (size > 3) {
			*dst++ = *src++;
			size -= 4;
		}

	unsigned char *cdst = (char*)dst;
	unsigned char *csrc = (char*)src;

	while (size--) {
		*cdst++ = *csrc++;
	}
	return _dst;
}
