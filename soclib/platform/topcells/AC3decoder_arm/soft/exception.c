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
	"   b main	                    \n"
);
/*
	".text					\n"
	"dataaddr:				\n"
	"   .long	maindata		\n"
	"ttyaddr:				\n"
	"   .long	0xC0200000		\n"
	"message:				\n"
	"   .long	0x48000000		\n"	//"H"
	"   .long	0x65000000		\n"	//"e"
	"   .long	0x6C000000		\n"	//"l"
	"   .long	0x6F000000		\n"	//"o"
	"   .long	0x0A000000		\n"	//"\n"
	"main_asm:					\n"
	"   ldr r0, dataaddr			\n"	//load the base address of data
	"   ldr r1, [r0,#0]			\n"	//r1 = 3
	"   ldr r2, [r0,#4]			\n"	//r2 = 6
	"   add r1, r1, r2			\n"	//just do a add to test
	"   str r1, [r0,#8]			\n"	//store the result of the add at (base address of data + 8)
	"   ldr r3, message			\n"	//load the first part of the message 'H'
	"   ldr r4, ttyaddr			\n"	//load the TTY address
	"   str r3, [r4]			\n"	//store the message in the TTY to print the first part of the message
	);

asm(
	"   ldr r3, message + 4			\n"	//load the 'e'
	"   str r3, [r4]			\n"	//store the message in the TTY
   );

asm(
	"   ldr r3, message + 8			\n"	//load the 'l'
	"   str r3, [r4]			\n"	//store the message in the TTY
	"   str r3, [r4]			\n"	//store the message in the TTY
   );


asm(
	"   ldr r3, message + 12		\n"	//load the 'o'
	"   str r3, [r4]			\n"	//store the message in the TTY
   );


asm(
	"   ldr r3, message + 16		\n"	//load the '\n'
	"   str r3, [r4]			\n"	//store the message in the TTY
   );


asm(
	"	b main_asm				\n"
	"   b main				\n"



	".data					\n"
	"maindata:				\n"
	"   .int	3			\n"
	"   .int	6			\n"
	"   .int	0			\n"

   );
*/

