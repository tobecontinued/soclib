#ifndef OPS_H_
#define OPS_H_

#include <stdint.h>
#include <stdio.h>


#define pp_add(a, b) ((a)+(b))
uint32_t add(uint32_t a, uint32_t b);

#define pp_sub(a, b) ((a)-(b))
uint32_t sub(uint32_t a, uint32_t b);

#define pp_and(a, b) ((a)&(b))
uint32_t and(uint32_t a, uint32_t b);

#define pp_or(a, b) ((a)|(b))
uint32_t or(uint32_t a, uint32_t b);

#define pp_xor(a, b) ((a)^(b))
uint32_t xor(uint32_t a, uint32_t b);

#define pp_mul(a, b) ((a)*(b))
uint32_t mul(uint32_t a, uint32_t b);

#define pp_div(a, b) ((a)/(b))
uint32_t div(uint32_t a, uint32_t b);

#define pp_sll(a, s) ((a)<<(s))
uint32_t sll(uint32_t a, size_t s);

#define pp_srl(a, s) ((a)>>(s))
uint32_t srl(uint32_t a, size_t s);

#define pp_mklemask(n) ((n)==32?((uint32_t)-1):((1<<(n))-1))
uint32_t mklemask(size_t n);

#define pp_sign_ext8(a) ((uint32_t)(int32_t)(int8_t)(a))
uint32_t sign_ext8(int8_t a);

#define pp_sign_ext16(a) ((uint32_t)(int32_t)(int16_t)(a))
uint32_t sign_ext16(int16_t a);

#define pp_rotl(a, s) ((a)<<(s)|(a)<<(32-(s)))
uint32_t rotl(uint32_t a, size_t s);

#define pp_rotr(a, s) ((a)<<(s)|(a)>>(32-(s)))
uint32_t rotr(uint32_t a, size_t s);

#define pp_extract(a, le, size) (((a)>>(le))&pp_mklemask(size))
uint32_t extract(uint32_t a, size_t le, size_t size);

#define pp_mkmask(high, low) (pp_mklemask(high)^pp_mklemask(low))
uint32_t mkmask(size_t hi, size_t lo);

#define pp_insert(a, to_ins, le, size) (((pp_mkmask(size, 0)&(to_ins))<<le)|(~pp_mkmask(le+size, le)&a))
uint32_t insert(uint32_t a, uint32_t to_ins, size_t le, size_t size);

#define pp_extract_15_8(a) pp_extract(a, 15, 8)
uint32_t extract_15_8(uint32_t a);

#define pp_insert_15_8(a, ti) pp_insert(a, ti, 15, 8)
uint32_t insert_15_8(uint32_t a, uint32_t to_ins);

#define pp_swap16(a) ((((a)>>8)&0xff)|(((a)&0xff)<<8))
uint16_t swap16(uint16_t a);

#define pp_swap32(a) (((a)<<24)|(((a)<<8)&0xff0000)|(((a)>>8)&0xff00)|((a)>>24))
uint32_t swap32(uint32_t a);

#define pp_swap16_2(a) ((((a)>>8)&0xff0000)|(((a)<<8)&0xff000000)|(((a)>>8)&0xff)|(((a)<<8)&0xff00))
uint32_t swap16_2(uint32_t a);

#define test(func, args) ({												\
		printf("Testing %s%s... ", #func, #args);						\
		uint32_t a = func args;											\
		uint32_t b = pp_##func args;									\
		if ( a != b ) {													\
			printf("failed ! %s%s [%08x] != pp_%s%s [%08x]\n",			\
				   #func, #args, a, #func, #args, b);					\
			abort();													\
		} else {														\
			printf("%08x == %08x\n", a, b);									\
		}																\
		a;																\
	})

#define test_val(func, args, val) do {									\
		uint32_t a = test(func, args);									\
		if ( a != val ) {												\
			printf("test with val failed ! %s%s [%08x] != [%08x]\n",	\
				   #func, #args, a, val);								\
			abort();													\
		}																\
	} while(0)

#define test_4_shift(op, a, n) \
	test(op, (a, n+0));		 \
	test(op, (a, n+1));		 \
	test(op, (a, n+2));		 \
	test(op, (a, n+3))

#define test_16_shift(op, a, n) \
	test_4_shift(op, a, n+0);		 \
	test_4_shift(op, a, n+4);		 \
	test_4_shift(op, a, n+8);		 \
	test_4_shift(op, a, n+12)

#define test_33_shift(op, a) \
	test_16_shift(op, a, 0);		 \
	test_16_shift(op, a, 16);		 \
	test(op, (a, 32))


#endif
