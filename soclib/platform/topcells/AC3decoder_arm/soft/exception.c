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
	"   .long	0x6F0A6F00		\n"	//"o\n"
	"messageaddr:           \n"
	"   .long   message     \n"
	"main_asm:					\n"
	"   ldr r0, dataaddr			\n"	//load the base address of data
	"   ldr r1, [r0,#0]			\n"	//r1 = 3
	"   ldr r2, [r0,#4]			\n"	//r2 = 6
	"   add r1, r1, r2			\n"	//just do a add to test
	"   str r1, [r0,#8]			\n"	//store the result of the add at (base address of data + 8)
	"   eor r5, r5, r5          \n"
	"   ldr r4, ttyaddr			\n"	//load the TTY address
	"loop:                      \n"
	"   ldr r6, messageaddr \n"
	"   add r6, r6, r5      \n"
	"   ldrb r3, [r6]       \n"
	"   strb r3, [r4]       \n"
	"   add r5, r5, #1      \n"
	"   teq r5, #6          \n"
	"   bne loop            \n"
	"	b main_asm			\n"
	"   b main				\n"



	".data					\n"
	"maindata:				\n"
	"   .int	3			\n"
	"   .int	6			\n"
	"   .int	0			\n"

   );


