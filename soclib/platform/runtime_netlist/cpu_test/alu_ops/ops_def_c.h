#include "ops_def_preproc.h"

INLINE data_t func(add)(data_t a, data_t b)
IMPL(
	return imp_add(a, b);
)

INLINE data_t func(sub)(data_t a, data_t b)
IMPL(
	return imp_sub(a, b);
)

INLINE data_t func(and)(data_t a, data_t b)
IMPL(
	return imp_and(a, b);
)

INLINE data_t func(or)(data_t a, data_t b)
IMPL(
	return imp_or(a, b);
)

INLINE data_t func(xor)(data_t a, data_t b)
IMPL(
	return imp_xor(a, b);
)

INLINE data_t func(mul)(data_t a, data_t b)
IMPL(
	return imp_mul(a, b);
)

INLINE data_t func(mod)(data_t a, data_t b)
IMPL(
	return imp_mod(a, b);
)

INLINE data_t func(div)(data_t a, data_t b)
IMPL(
	return imp_div(a, b);
)

INLINE data_t func(sll)(data_t a, size_t s)
IMPL(
	return imp_sll(a, s);
)

INLINE data_t func(srl)(data_t a, size_t s)
IMPL(
	return imp_srl(a, s);
)

INLINE data_t func(mklemask)(size_t n)
IMPL(
	return imp_mklemask(n);
)

INLINE data_t func(sign_ext8)(int8_t a)
IMPL(
	return imp_sign_ext8(a);
)

INLINE data_t func(sign_ext16)(int16_t a)
IMPL(
	return imp_sign_ext16(a);
)

INLINE data_t func(rotl)(data_t a, size_t s)
IMPL(
	return imp_rotl(a, s);
)

INLINE data_t func(rotr)(data_t a, size_t s)
IMPL(
	return imp_rotr(a, s);
)

INLINE data_t func(mkmask)(size_t hi, size_t lo)
IMPL(
	return imp_mkmask(hi, lo);
)

INLINE data_t func(extract)(data_t a, size_t le, size_t size)
IMPL(
	return imp_extract(a, le, size);
)

INLINE data_t func(insert)(data_t a, data_t to_ins, size_t le, size_t size)
IMPL(
	return imp_insert(a, to_ins, le, size);
)

INLINE data_t func(extract_15_8)(data_t a)
IMPL(
	return imp_extract_15_8(a);
)

/* data_t func(insert_15_8)(data_t a, data_t to_ins) */
/* IMPL( */
/* 	return imp_insert_15_8(a, to_ins); */
/* ) */

INLINE data_t func(swap)(data_t a)
IMPL(
	return func(imp_swap)(a);
)

