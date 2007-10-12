
#define assert(x) \
do {																	\
	if ( !(x) ) {														\
		printf("Assertion `%s' failed !!!\n", #x);		\
	}																	\
} while (0)
/* #define assert(x) \ */
/* do {																	\ */
/* 	if ( !(x) ) {														\ */
/* 		printf("Assertion `%s' failed !!!\n", #x);		\ */
/* 		abort();														\ */
/* 	}																	\ */
/* } while (0) */

void abort();
char *strcpy( char *dst, const char *src );
int printf( const char *fmt, ... );
int strcmp( const char *, const char *);
void *malloc( unsigned long sz );
void *memcpy( void *_dst, void *_src, unsigned long size );
