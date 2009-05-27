#ifndef OPS_H_
#define OPS_H_

#include <stdint.h>
#include <stdio.h>


#define IMPL(...)  { __VA_ARGS__ }
#define INLINE static inline

#define data_t uint32_t
#define func(x) pp_##x##_32
#include "ops_def_c.h"
#undef data_t
#undef func
#define data_t uint64_t
#define func(x) pp_##x##_64
#include "ops_def_c.h"

#undef IMPL
#undef INLINE
#undef func
#undef data_t

#define IMPL(...) ;
#define INLINE 

#define data_t uint32_t
#define func(x) x##_32
#include "ops_def_c.h"
#undef data_t
#undef func
#define data_t uint64_t
#define func(x) x##_64
#include "ops_def_c.h"

#undef INLINE
#define INLINE static inline

INLINE uint32_t pp_insert_15_8_32(uint32_t a, uint32_t to_ins)
{
	return imp_insert_15_8(a, to_ins);
}

INLINE uint64_t pp_insert_15_8_64(uint64_t a, uint64_t to_ins)
{
	return imp_insert_15_8(a, to_ins);
}

uint32_t insert_15_8_32(uint32_t a, uint32_t to_ins);
uint64_t insert_15_8_64(uint64_t a, uint64_t to_ins);

#endif
