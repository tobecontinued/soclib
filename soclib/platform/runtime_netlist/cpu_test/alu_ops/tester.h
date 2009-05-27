
uint32_t test_info_32(const char*func, uint32_t expected_result, uint32_t got_result);
uint64_t test_info_64(const char*func, uint64_t expected_result, uint64_t got_result);

#define test_32(ff, args)												\
	test_info_32(#ff "_32" #args,										\
				 pp_##ff##_32 args,										\
				 ff##_32 args)


#define test_64(ff, args)												\
	test_info_64(#ff "_64" #args,										\
				 pp_##ff##_64 args,										\
				 ff##_64 args)


#define test_val_32(ff, args, val)								\
	assert( test_32(ff, args) == val )

#define test_val_64(ff, args, val)								\
	assert( test_64(ff, args) == val )

#define test_4_shift(op, a, n) \
	test(op, (a, ((n)+0)));		 \
	test(op, (a, ((n)+1)));		 \
	test(op, (a, ((n)+2)));		 \
	test(op, (a, ((n)+3)))

#define test_16_shift(op, a, n) \
	test_4_shift(op, a, ((n)+0));		 \
	test_4_shift(op, a, ((n)+4));		 \
	test_4_shift(op, a, ((n)+8));		 \
	test_4_shift(op, a, ((n)+12))

#define test_32_shift(op, a, n)			 \
	test_16_shift(op, a, ((n)+0));		 \
	test_16_shift(op, a, ((n)+16));		 \

#define test_64_shift(op, a, n)			 \
	test_32_shift(op, a, ((n)+0));		 \
	test_32_shift(op, a, ((n)+32));		 \

#define test_33_shift(op, a)		 \
	test_32_shift(op, a, 0);		 \
	test(op, (a, 32))

#define test_65_shift(op, a)		 \
	test_64_shift(op, a, 0);		 \
	test(op, (a, 64))

