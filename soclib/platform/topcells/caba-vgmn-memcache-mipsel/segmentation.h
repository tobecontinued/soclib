
/////////////////////////////////////////////////////////////////
//	ADDRESS SPACE SEGMENTATION
//
//	This file must be included in the system.cpp file, 
//	for harware configuration : It is used to build
//	the SOCLIB_SEGMENT_TABLE.
//
//	This file can also be used by the ldscript generator,
//	for embedded software generation.
//	
//	It gives the system integrator the garanty
//	that hardware and software have the same
//	description of the address space segmentation.
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
//	reset, and exception segments
//      base address required by MIPS processor
/////////////////////////////////////////////////////////////////

#define	RESET_BASE	0xBFC00000
#define	RESET_SIZE	0x00010000

#define	EXCEP_BASE	0x80000000
#define	EXCEP_SIZE	0x00010000

/////////////////////////////////////////////////////////////////
//	global data segment (initialised)
/////////////////////////////////////////////////////////////////

#define MC_M_BASE	0x10000000
#define MC_M_SIZE	0x00400000

//////////////////////////////////////////////////////////
//	System devices (seen by the software)
///////////////////////////////////////////////////////////

#define	TTY_BASE	0xC0400000
#define	TTY_SIZE	0x00000100

#define	TIMER_BASE	0xD0400000
#define	TIMER_SIZE	0x00000100

#define	LOCKS_BASE	0xE0400000
#define	LOCKS_SIZE	0x00000100

#define PROC_BASE	0xF0400000
#define PROC_SIZE	0x00000008

#define XRAM_BASE	0xB0400000
#define XRAM_SIZE	0x00000008

#define MC_R_BASE	0x20400000
#define MC_R_SIZE	0x00000008

#define CLEANUP_OFFSET  0x20400000
