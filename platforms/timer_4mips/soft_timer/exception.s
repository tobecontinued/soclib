/* 
 * Authors: Frédéric Pétrot and Denis Hommais
 * $Log$
 * Revision 1.1  2005/01/27 13:42:46  wahid
 * Initial revision
 *
 * Revision 1.1.1.1  2004/12/01 13:30:59  wahid
 * Date04 added to examples
 *
 * Revision 1.2  2003/07/18 16:21:30  fred
 * Now interprets the instruction that causes a DBE in order to print the
 * address responsible for the failure.
 * Cleaned up the code a little (mainly comments :)
 *
 * Revision 1.1  2003/03/10 13:39:07  fred
 * Adding the hardware dependent stuff to smoothly handle multi
 * architecture compilation of this kernel
 * 
 * Revision 1.1.1.1  2002/02/28 12:58:55  disydent
 * Creation of Disydent CVS Tree
 * 
 * Revision 1.1.1.1  2001/11/19 16:55:37  pwet
 * Changing the CVS tree structure of disydent
 * 
 * Revision 1.3  2001/11/14 12:08:45  fred
 * Making the ldscript being generated for correct path accesses
 * Removing the syscall entry of the exception handler for now
 * 
 * Revision 1.2  2001/11/09 14:51:03  fred
 * Remove the reset executable and make the assembler be piped through
 * cpp.
 * The ldscript is not usable. I have to think before going on
 * 
 * $Id: exception.S,v 1.1 2003/03/10 13:39:07 fred Exp
 *  Note   : we do not check BD at any time yet! Beware, ...
 *  Rev 0  : minimal handler to support syscall for system calls
 *           implementation
 *  Rev 1  : added correct exception/interruption detection and messages
 *  Rev 2  : added mult and div interpretation in illegal instruction
 *  Rev 3  : Makes it compliant with multi-thread/multi-processor 
 *           Stack is thread stack now (it must at least for preemptive
 *           scheduling stuffs 
 */
        .rdata
        .align 2
msg  :  .ascii "An exception occurred with the following cause :\n\000"

jmptable:
        .word Int
        .word Uimp
        .word Uimp
        .word Uimp
        .word AdEL
        .word AdES
        .word IBE
        .word DBE
        .word Sys
        .word Bp
        .word RI
        .word CpU
        .word Ovf
        .word Ukn
        .word Ukn

        .text
        .align 2
        .ent     excep
excep:
        /*
         * Here I save the whole C compatible register context, which is a
         * waste of time if only assembly functions are to be used
         * A more optimized way to do things would be to check for critical
         * events (typically external interrupts) and do the saving only
         * when an other events took place
         * Note that this implies to check which registers to save according
         * to the interrupt handling routines, that are usually user defined!
         *
         * Reminder: $26 and $27 are kernel registers that need not be saved
         * This shall be executed in critical section since the stack pointer
         * is not properly set until the subu!
         */
        .set noreorder
        /* Shall not touch $1 until it is saved */
        .set noat
#		mfc0 $26, $14
#		nop
#		nop
#		jr $26
#		rfe
		
		         
         subu   $29, $29, 26*4  /* Load kernel stack addr + stack update */
         sw     $1,   1*4($29)  /*  save all temporaries*/
        .set at
         sw     $2,   2*4($29)  /*  others are saved when calling procedures*/
         sw     $3,   3*4($29)
         sw     $4,   4*4($29)
         sw     $5,   5*4($29)
         sw     $6,   6*4($29)
         sw     $7,   7*4($29)
         sw     $8,   8*4($29)
         sw     $9,   9*4($29)
         sw    $10,  10*4($29)
         sw    $11,  11*4($29)
         sw    $12,  12*4($29)
         sw    $13,  13*4($29)
         sw    $14,  14*4($29)
         sw    $15,  15*4($29)
         sw    $24,  16*4($29)
         sw    $25,  17*4($29)
         mfc0  $26,      $14     /* load EPC into k0 (two cycles befor use)*/
         sw    $29,  18*4($29)
         sw    $30,  19*4($29)
         sw    $31,  20*4($29)
         sw    $26,  21*4($29)   /* and store it to be recallable*/
         mfhi  $26
         sw    $26,  22*4($29)
         mflo  $26
         sw    $26,  23*4($29)

         /* Check the cause of the arrival in this code */

         mfc0  $27, $13           /*  load CAUSE into $27*/
         nop
         andi  $27, $27,  0x3c    /*  get the exception code (ExcCode)*/
         beqz  $27, Int           /*  Test if it is an interrupt. If so*/
         sw     $0,  24*4($29)    /* return address is epc + 0 */
                                  /* it's faster.*/
         la    $26,      jmptable /*  load the base of the jump table*/
                                  /* get the exception code (ExcCode)*/
         addu  $26, $26, $27      /*  load the offset in the table*/
         lw    $27,      ($26)    /*  load the pointed value*/
         li    $26,           4
         j     $27                /*  and jump there*/
         sw    $26,  24*4($29)    /* faulty insn is at epc - 4 */
         nop                      /*  delayed slot too*/

 /*
  * External interupts:
  * The meaning of these interupts is for sure system dependent, so
  * this routine must be adapted to each system particularities
  *
  * Right now we just find out what interrupt lead us here, and print
  * a small message about it.
  */
        .rdata
        .align 2
        .text
Int :   
        .set noat
         mfc0  $4, $13           /*  load CAUSE into $27*/
         mfc0  $26, $12           /*  load STATUS into $26*/
         andi  $4, $4,   0xFF00 /*  mask CAUSE to get the ITs only*/
         and   $4, $4,   $26    /*  keep only the enabled ITs*/
         srl   $4, $4,   8      /*  and put them in the LSBees*/
         beqz  $4,        _endit  /* If none, then what are we doing here? */
         nop

         .extern  SwitchOnIt

         la     $26, SwitchOnIt
         jal    $26
         nop

_endit:
         j                 _return  /*  go back to user*/
         nop
        .set at

 /*
  * Clearly fatal exceptions as far as we're concerned, ...
  */
        .rdata
        .align 2
m2  :   .ascii "Unimplemented TLB Exception!\n\000"
m2k :   .ascii "Reserved Exception!\n\000"
m2o :   .ascii "Integer Arithmetic Overflow!\n\000"

        .text
        .align 2
        .set reorder
Uimp :   la     $4, m2            /*  load the appropriate message,...*/
         j      U                 /* */
Ukn  :   la     $4, m2k           /* */
         j      U                 /* */
Ovf  :   la     $4, m2o           /* */
U    :   la     $2, uputs
         jal    $2                /*  and print it!*/
         j      _exit             /* */
        .set noreorder

 /*
  * Illegal instructions:
  * we interpret mult/div and lwl/lwr here, and exit on an other code
  * Before modifing this code think that NO REGISTER CONTENT should
  * be touched before the end of the register loading routine
  * This explains why some code seems duplicated (several mfc0)
  */
        .rdata
m2i  :  .ascii "Illegal or Reserved Instruction!\n\000"
        .text
        .set noat
RI   :   mfc0  $26,      $14      /*  load EPC into $26*/
         nop                      /* */
         nop                      /* */
         lw    $26,      ($26)    /*  $26 contains the faulty ins*/
         lui   $27,      0xFC00    /*  mask for special*/
         and   $27, $26, $27        /* */
         bne   $27,  $0, Ilg         /*  this is not a special trap*/

/*
 * I do not handle the mult/div trap, as we have added the
 * instructions in our R3000 model
 */
         nop

Ilg:     la     $4, m2i
         la     $2, uputs
         jal    $2                /*  we got an illegal ins, so we quit*/
         nop
         j      _exit             /* */
         nop

 /*
  * Unaligned access:
  * Quite fatal, I'm afraid
  */
        .rdata
        .align 2
m3   :  .ascii "Unaligned/unallowed Access while %s address 0x%x at pc 0x%x!\n\000"
m3l  :  .ascii "reading\000"
m3e  :  .ascii "writing\000"

        .text
        .align 2
        .set reorder
AdEL :   la     $5,   m3l
         j      AdE  
AdES :   la     $5,   m3e
AdE  :   la     $4,   m3
        .set noreorder
         mfc0   $6,   $8          /*  load BAR into thrid arg ($6)*/
         mfc0   $7,  $14          /*  load EPC into fourth arg ($7)*/
         la     $2,  uputs
         jal    $2                /*  print all that stuff*/
         nop
         j               _exit
         nop

 /*
  * Synchronous bus error
  */
        .rdata
        .align 2
m4i:    .ascii "Bus error while loading the instrunction at pc 0x%x!\n\000"
m4d:    .ascii "Bus error while loading the datum at pc 0x%x for addr 0x%x!\n\000"
        .data
        .text

        .align 2
        .set reorder
IBE :    mfc0   $5,   $14         /*  load EPC */
         la     $4,   m4i
         j      BE
        .set noreorder
DBE :    la     $4,   m4d
         /*
          * The instruction at EPC must be interpreted to get the
          * address that caused the fault.
          * The register value must be reloaded from the stack, and
          * since the storage of the registers is quite exotic right
          * now, this will work only for registers 1 to 15!
          */
         mfc0   $5,   $14         /* load EPC */
         lw     $6, 0($5)         /* load faulty insn */
         andi   $8,  $6, 0xFFFF   /* get the imm field of it */
         srl    $9,  $6, 19
         andi   $9,  $9, 0x7c     /* get the rs register index times 4 */
         add    $9, $29, $9       /* compute the stack index of this register */
         lw     $9, 0($9)
         addu   $6, $9, $8        /* get the address as computed */

BE :     la     $2,   uputs
         jal    $2                /*  print all that stuff*/
         nop
         j               _exit
         nop

 /*
  * Syscall: simple and straightforward.
  * Even elegant : nothing's done at all anymore
  */
        .rdata
        .align 2
m5 :    .ascii "Syscall : no actions performed anymore\000"
        .text
        .align 2
Sys : 
         la     $4, m5
         la     $2, uputs
         jalr   $2
         nop
         j       _ret_sys         /* get back to caller */
         nop
#endif

 /*
  *
  * We just indicate we got a breakpoint
  * This shall be filled on a system per system basis
  *
  */
        .rdata
        .align 2
m6 :    .ascii "Breakpoint of code %d!\n\000"

        .text
        .align 2
Bp :     la     $4,   m6
         mfc0   $3, $14           /*  load EPC*/
         nop
         nop
         lw     $5, ($3)          /*  load the 'break' instruction*/
         li     $3,      0xF      /*  filled delayed slot (load a mask)*/
         srl    $5,  $5, 16       /*  shift it*/
         and    $5,  $5, $3       /*  and get the code from the insn*/
         la     $2,  uputs
         jal    $2                /*  print the string*/
         nop
         j               _return
         nop

 /*
  *
  * Kernel/user violation, or coprocessor unusable.
  * that's pretty fatal, ain't it ?
  *
  */
        .rdata
        .align 2
m7 :    .ascii "Unusable Coprocessor %d!\n\000"
m70:    .ascii "Trying to use a kernel instruction in user mode!\n\000"

        .text
        .align 2
CpU :    mfc0   $3, $13           /*  load CAUSE*/
         nop
         li     $2,     0x30000000/* */
         and    $2, $2, $3        /*  mask to get the CE bits */
         bnez   $2,     cun       /*  not zero, so it is a CUN*/
         nop                      /*  cannot put a 'la' here,...*/
        .set reorder
         la     $4,     m70       /*  here, it is a kernel violation*/
         j      C
        .set noreorder
cun:     la     $4,   m7
         srl    $5,   $2,  28     /*  LSByte contains now the copro number*/
C:       la     $2,   uputs
         jal    $2
         nop
         j               _exit
         nop

_exit:   li     $2, 100000
_exit_l: bne    $2, $0,  _exit_l
         addiu  $2, $2, -1
         c0     00
         nop

_return :
         lw     $2,    2*4($29)
         j      tmp
         nop
_ret_sys: /* Special case for syscall : $2 is not restored */
tmp:    .set noat      
         lw     $1,    1*4($29)
        .set at        
         lw     $3,    3*4($29)
         lw     $4,    4*4($29)
         lw     $5,    5*4($29)
         lw     $6,    6*4($29)
         lw     $7,    7*4($29)
         lw     $8,    8*4($29)
         lw     $9,    9*4($29)
         lw    $10,   10*4($29)
         lw    $11,   11*4($29)
         lw    $12,   12*4($29)
         lw    $13,   13*4($29)
         lw    $14,   14*4($29)
         lw    $15,   15*4($29)
         lw    $24,   16*4($29)
         lw    $25,   17*4($29)
         lw    $29,   18*4($29)
         lw    $30,   19*4($29)
         lw    $31,   20*4($29)

         /* Take care of hi and lo also */
         lw    $26,   22*4($29)
         mthi  $26
         lw    $26,   23*4($29)
         mtlo  $26

         lw    $26,   21*4($29)   /* This is the epc of this context */
         lw    $27,   24*4($29)   /* This is the offset: 4 excep, 0 int */
         addu  $29,   $29, 26*4   /* Stack update : fill lw stall */
         addu  $26,   $26, $27    /* Addr for return from excep */
         j     $26                /* Jump back where the exception occured*/
         rfe                      /* And restore the original mode*/
        .set     reorder
        .end  excep
