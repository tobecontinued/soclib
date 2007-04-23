/////////////////////////////////////////////////////////////////
//	ADDRESS SPACE SEGMENTATION
//
//	This file must be included in the system.cpp file, 
//	for harware configuration : It is used to build
//	the SOCLIB_SEGMENT_TABLE.
//
//	This file is also used by the ldscript generator,
//	for embedded software generation.
//	
//	It gives the system integrator the garanty
//	that hardware and software have the same
//	description of the address space segmentation.
//	
//	The segment names cannot be changed.
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
//	text, reset, and exception segments
//	
//	Those three segments are mandatory
//	The base adresses are imposed by the 
//	MIPS processor architecture
/////////////////////////////////////////////////////////////////

#define	TEXT_BASE	0x00400000
#define	TEXT_SIZE	0x00050000

#define	RESET_BASE	0xBFC00000
#define	RESET_SIZE	0x00010000

#define	EXCEP_BASE	0x80000080
#define	EXCEP_SIZE	0x00010000

/////////////////////////////////////////////////////////////////
//	global data segment (initialised)
/////////////////////////////////////////////////////////////////

#define	DATA_BASE	0x10000000
#define	DATA_SIZE	0x00010000

/////////////////////////////////////////////////////////
//	local data segments 
//	
//	The number of local segments
//	must be equal to the number of processors
/////////////////////////////////////////////////////////

#define	LOC0_BASE	0x20000000
#define	LOC0_SIZE	0x00001000

#define	LOC1_BASE	0x21000000
#define	LOC1_SIZE	0x00001000

#define	LOC2_BASE	0x22000000
#define	LOC2_SIZE	0x00001000

#define	LOC3_BASE	0x23000000
#define	LOC3_SIZE	0x00001000

//////////////////////////////////////////////////////////
//	System TTY and private TTYs
//	
//	- The system TTY is used by the printf() function.
//	This segment must always be defined, 
//	as it is used by the Mutek kernel. 
//	- The private TTYs are used by the pprintf() function.
//	The number of TTYs segments must be equal 
//	to the number of processors.
///////////////////////////////////////////////////////////

#define	TTY_BASE	0xC0200000
#define	TTY_SIZE	0x00000258

/////////////////////////////////////////////////////////
//	System Peripherals
//
//	Those segments must be defined for MUTEK,
//	even if the corresponding peripherals
//	are not instanciated in the hardware platform
/////////////////////////////////////////////////////////

#define	TIMER_BASE	0xB0200000
#define	TIMER_SIZE	0x00000100

#define	ITC_BASE	0xB1200000
#define	ITC_SIZE	0x00001000

#define	LOCKS_BASE	0xB2200000
#define	LOCKS_SIZE	0x00001000

#define	DFT_BASE	0xB3200000
#define	DFT_SIZE	0x00001000



