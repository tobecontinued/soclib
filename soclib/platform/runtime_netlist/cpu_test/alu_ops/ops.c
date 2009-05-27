
#include "stdint.h"

#undef data_t
#undef func

#define IMPL(...) { __VA_ARGS__ }
#define INLINE

#define data_t uint32_t
#define func(x) x##_32
#include "ops_def_c.h"
#undef data_t
#undef func
#define data_t uint64_t
#define func(x) x##_64
#include "ops_def_c.h"

uint32_t insert_15_8_32(uint32_t a, uint32_t to_ins)
{
#if (__mips > 32) || (__mips == 32 && __mips_isa_rev >= 2)
	// Stupid GCC, you could detect this pattern and optimize it ! :)
	uint32_t ret = a;
	asm("ins %0, %2, 15, 8"
		:"=&r"(ret)
		:"0"(ret), "r"(to_ins) );
	return ret;
#else
	return imp_insert_15_8(a, to_ins);
#endif
}

uint64_t insert_15_8_64(uint64_t a, uint64_t to_ins)
{
	return imp_insert_15_8(a, to_ins);
}

