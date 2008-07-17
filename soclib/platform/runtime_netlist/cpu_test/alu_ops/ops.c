#include "ops.h"

uint32_t add(uint32_t a, uint32_t b)
{
	return pp_add(a, b);
}

uint32_t sub(uint32_t a, uint32_t b)
{
	return pp_sub(a, b);
}

uint32_t and(uint32_t a, uint32_t b)
{
	return pp_and(a, b);
}

uint32_t or(uint32_t a, uint32_t b)
{
	return pp_or(a, b);
}

uint32_t xor(uint32_t a, uint32_t b)
{
	return pp_xor(a, b);
}

uint32_t mul(uint32_t a, uint32_t b)
{
	return pp_mul(a, b);
}

uint32_t div(uint32_t a, uint32_t b)
{
	return pp_div(a, b);
}

uint32_t sll(uint32_t a, size_t s)
{
	return pp_sll(a, s);
}

uint32_t srl(uint32_t a, size_t s)
{
	return pp_srl(a, s);
}

uint32_t mklemask(size_t n)
{
	return pp_mklemask(n);
}

uint32_t sign_ext8(int8_t a)
{
	return pp_sign_ext8(a);
}

uint32_t sign_ext16(int16_t a)
{
	return pp_sign_ext16(a);
}

uint32_t rotl(uint32_t a, size_t s)
{
	return pp_rotl(a, s);
}

uint32_t rotr(uint32_t a, size_t s)
{
	return pp_rotr(a, s);
}

uint32_t mkmask(size_t hi, size_t lo)
{
	return pp_mkmask(hi, lo);
}

uint32_t extract(uint32_t a, size_t le, size_t size)
{
	return pp_extract(a, le, size);
}

uint32_t insert(uint32_t a, uint32_t to_ins, size_t le, size_t size)
{
	return pp_insert(a, to_ins, le, size);
}

uint32_t extract_15_8(uint32_t a)
{
	return pp_extract_15_8(a);
}

uint32_t insert_15_8(uint32_t a, uint32_t to_ins)
{
#if (__mips > 32) || (__mips == 32 && __mips_isa_rev >= 2)
	// Stupid GCC, you could detect this pattern and optimize it ! :)
	uint32_t ret;
	asm("ins %0, %2, 15, 8"
		:"=r"(ret)
		:"0"(a), "r"(to_ins) );
	return ret;
#else
	return pp_insert_15_8(a, to_ins);
#endif
}

uint16_t swap16(uint16_t a)
{
	return pp_swap16(a);
}

uint32_t swap32(uint32_t a)
{
	return pp_swap32(a);
}

uint32_t swap16_2(uint32_t a)
{
	return pp_swap16_2(a);
}
