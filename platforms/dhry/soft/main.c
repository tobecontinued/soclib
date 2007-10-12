#include "soclib/timer.h"
#include "system.h"
#include "stdio.h"

#include "../segmentation.h"

#include "soclib/simhelper.h"

int dhry(int);

int main(void)
{
	const int npass = 300;
	const int clock_rate = 500;
	const int dmips = 1757;

	int run_before2 = run_cycles();
	int max_cycles = dhry(2*npass);
	int run_after2 = run_cycles();
	int run_before = run_cycles();
	int min_cycles = dhry(npass);
	int run_after = run_cycles();
	int tot_cycles = max_cycles-min_cycles;
	int cycles_per_dhry = tot_cycles/npass;
	int dhry_per_mhz = npass*1000000/tot_cycles;
	int dhry_per_s_at_clock_rate = clock_rate*dhry_per_mhz;
	int error = clock_rate*1000000 - cycles_per_dhry * dhry_per_s_at_clock_rate;
	int max_instr = run_after2 - run_before2;
	int min_instr = run_after - run_before;
	int ins_per_dhry = (run_after2-run_before2-(run_after-run_before))/npass;

	printf("%d passes: %d cycles\n", npass, tot_cycles);
	printf("Cycles/dhrystone: %d\n", cycles_per_dhry);
	printf("Dhrystone/Mhz: %d\n", dhry_per_mhz);
	printf("@%dMhz: %d dhry/s\n", clock_rate, dhry_per_s_at_clock_rate);
	printf("check: %d\n", error );

	printf("\n");
	printf("With 1 DMIPS = %d dhry/s\n", dmips);
	printf("Dhrystone MIPS: %d\n", dhry_per_s_at_clock_rate/dmips);
	printf("\n");
	printf("Dhry/mhz (*1000) = %d\n", 569151/cycles_per_dhry);

	printf("ins/%d: %d\n", 2*npass, run_after2-run_before2);
	printf("ins/%d: %d\n", npass, run_after-run_before);

	printf("\n");
	printf("ins/dhry %d\n", ins_per_dhry);
	printf("cyc/dhry %d\n", cycles_per_dhry);
	printf("cpi (*1000) %d\n", 1000*cycles_per_dhry/ins_per_dhry);
	printf("dhry/MHz (*1000) = %d\n", 569151/cycles_per_dhry);

	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_SC_STOP,
		0);
	while(1)
		;
}
