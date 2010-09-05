
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
//	It gives the system integrator the guaranty
//	that hardware and software have the same
//	description of the address space segmentation.
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
//	reset, and exception segments
//      base address required by MIPS processor
/////////////////////////////////////////////////////////////////
#define MMU

#define	RESET_BASE	0xBFC00000
#define	RESET_SIZE	0x00001000      // 4 k

#define	EXCEP_BASE	0x80000080
#define	EXCEP_SIZE	0x00001000      // 4 k

#define	TEXT_BASE	0x00400000
#define	TEXT_SIZE       0x00050000	// 20 k 
///////////////////////////////////////////////////////////////////////////////////
//	global data segment (initialised)
///////////////////////////////////////////////////////////////////////////////////

#define DATA0_BASE	0x10000000
#define DATA0_SIZE	0x00100000     // 256 k

#define DATA1_BASE	0x50000000
#define DATA1_SIZE	0x00100000

#define DATA2_BASE	0x90000000
#define DATA2_SIZE	0x00100000

#define DATA3_BASE	0xE0000000
#define DATA3_SIZE	0x00100000

/////////////////////////////////////////////////////////////////
//	page table (initialised)
/////////////////////////////////////////////////////////////////
#define PTD_ADDR	0x40400000
#define PTE_ADDR	0x40402000
#define IPTE_ADDR	0x40403000
#define	TAB_SIZE	0x00010000      // 64 k

#define V_TTY_BASE 	0x00800000
#define	V_TIMER_BASE	0x00C00000	// timer virtual address
//////////////////////////////////////////////////////////
//	System devices
///////////////////////////////////////////////////////////

#define	TTY_BASE	0xC0200000  
#define	TTY_SIZE	0x00000100  // 256

#define	TIMER_BASE	0xD0200000
#define	TIMER_SIZE	0x00000100

#define	LOCKS_BASE	0xE0200000
#define	LOCKS_SIZE	0x00000100

#define PROC0_BASE      0x01400000
#define PROC0_SIZE      0x0000000A

#define PROC1_BASE      0x02400000
#define PROC1_SIZE      0x0000000A

#define PROC2_BASE      0x03400000
#define PROC2_SIZE      0x0000000A

#define PROC3_BASE      0x04400000
#define PROC3_SIZE      0x0000000A

#define PROC4_BASE      0x43400000
#define PROC4_SIZE      0x0000000A

#define PROC5_BASE      0x44400000
#define PROC5_SIZE      0x0000000A

#define PROC6_BASE      0x45400000
#define PROC6_SIZE      0x0000000A

#define PROC7_BASE      0x46400000
#define PROC7_SIZE      0x0000000A

#define PROC8_BASE      0xA3400000
#define PROC8_SIZE      0x0000000A

#define PROC9_BASE      0xA4400000
#define PROC9_SIZE      0x0000000A

#define PROC10_BASE     0xA5400000
#define PROC10_SIZE     0x0000000A

#define PROC11_BASE     0xA6400000
#define PROC11_SIZE     0x0000000A

#define PROC12_BASE     0xE2400000
#define PROC12_SIZE     0x0000000A

#define PROC13_BASE     0xE3400000
#define PROC13_SIZE     0x0000000A

#define PROC14_BASE     0xE4400000
#define PROC14_SIZE     0x0000000A

#define PROC15_BASE     0xE5400000
#define PROC15_SIZE     0x0000000A

