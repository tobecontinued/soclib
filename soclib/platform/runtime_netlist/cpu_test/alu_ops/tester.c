#include <stdint.h>
#include <stdio.h>

uint32_t test_info_32(const char*func, uint32_t expected_result, uint32_t got_result)
{
	printf("Testing %s... ", func);
	if ( expected_result != got_result ) {
		int i;
		printf("expected [%08x] got [%08x]\n",
			   expected_result, got_result);
		for ( i=0; i<10000; i++ )
			asm volatile("nop");
		abort();
	}
	printf(" = %08x OK\n", expected_result);
	return got_result;
}

uint64_t test_info_64(const char*func, uint64_t expected_result, uint64_t got_result)
{
	printf("Testing %s... ", func);
	printf("expected [%08x%08x] got [%08x%08x]",
		   ((uint32_t)(expected_result>>32)), (uint32_t)expected_result,
		   ((uint32_t)(got_result>>32)), (uint32_t)got_result );
	if ( expected_result != got_result ) {
		int i;
		printf("failed\n");
		for ( i=0; i<10000; i++ )
			asm volatile("nop");
		abort();
	}
	printf(" = %08x%08x OK\n", ((uint32_t)(expected_result>>32)), (uint32_t)expected_result);
	return got_result;
}
