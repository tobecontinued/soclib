asm(
    ".section        .reset,\"ax\"	\n"

    ".globl arm966_reset_entry				\n"
    ".globl main					\n"
    "arm966_reset_entry:					\n"
//    ".set push							\n"
//    ".set noat							\n"

	"   b bootstart                     \n"
	"bootstart:                         \n"
	"   mov sp, #0x05000000		    \n"
	"   b main_asm	                    \n"


	".text					\n"
	"dataaddr:				\n"
	"   .long	maindata		\n"
	"ttyaddr:				\n"
	"   .long	0xC0200000		\n"
	"message:				\n"
	"   .long	0x48656C6C		\n"	//"Hell"
	"   .long	0x6F0A0000		\n"	//"o\n"
	"main_asm:					\n"
	"   ldr r0, dataaddr			\n"	//load the base address of data
	"   ldr r1, [r0,#0]			\n"	//r1 = 3
	"   ldr r2, [r0,#4]			\n"	//r2 = 6
	"   add r1, r1, r2			\n"	//just do a add to test
	"   str r1, [r0,#8]			\n"	//store the result of the add at (base address of data + 8)
	"   ldr r3, message			\n"	//load the first part of the message ("Hell")
	"   ldr r4, ttyaddr			\n"	//load the TTY address
	"   str r3, [r4]			\n"	//store the message in the TTY to print the first part of the message
	);
asm volatile("" ::: "memory");
asm(
	"   ldr r3, message + 4			\n"	//load the last part of the message
	"   str r3, [r4]			\n"	//store the message in the TTy to print the last part of th emessage
   );
asm volatile("" ::: "memory");
asm(
	"	b main_asm				\n"
	"   b main				\n"



	".data					\n"
	"maindata:				\n"
	"   .int	3			\n"
	"   .int	6			\n"
	"   .int	0			\n"

   );


