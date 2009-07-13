
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
#define MMU

#define	RESET_BASE	0xBFC00000
#define	RESET_SIZE	0x00010000

#define	EXCEP_BASE	0x80000080
#define	EXCEP_SIZE	0x00010000

#define	TEXT_BASE	0x00400000
#define	TEXT_SIZE	0x00050000
/////////////////////////////////////////////////////////////////
//	global data segment (initialised)
/////////////////////////////////////////////////////////////////

#define MC_M_BASE	0x10000000
#define MC_M_SIZE	0x00100000

/////////////////////////////////////////////////////////////////
//	page table (initialised)
/////////////////////////////////////////////////////////////////
#define PTD_ADDR	0x40400000
#define PTE_ADDR	0x40402000
#define IPTE_ADDR	0x40403000
#define	TAB_SIZE	0x00010000

#define V_TTY_BASE 	0x00800000
#define	V_TIMER_BASE	0x00C00000	// timer virtual address
//////////////////////////////////////////////////////////
//	System devices
///////////////////////////////////////////////////////////

#define	TTY_BASE	0xC0200000
#define	TTY_SIZE	0x00000100

#define	TIMER_BASE	0xD0200000
#define	TIMER_SIZE	0x00000100

#define	LOCKS_BASE	0xE0200000
#define	LOCKS_SIZE	0x00000100

#define PROC0_BASE	0x01200000
#define PROC0_SIZE	0x00000008

#define PROC1_BASE	0x02200000
#define PROC1_SIZE	0x00000008

#define MC_R_BASE	0x20200000
#define MC_R_SIZE	0x00000008
