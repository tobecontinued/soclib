#include "system.h"
#include "stdio.h"

#include "soclib/simhelper.h"

#include "segmentation.h"

int main(void)
{
	soclib_io_set(
		base(SIMHELPER),
		SIMHELPER_END_WITH_RETVAL,
		0);
	while(1)
		;
}
