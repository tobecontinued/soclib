/*
 * Authors: Denis Hommais and Frédéric Pétrot
 * $Log$
 * Revision 1.1  2005/01/27 13:42:46  wahid
 * Initial revision
 *
 * Revision 1.1.1.1  2004/12/01 12:53:26  wahid
 * Examples added
 * AOC project
 *
 * Revision 1.2  2003/07/01 10:21:55  fred
 * Updating the linker script generator, and adding more examples
 *
 * Revision 1.1  2003/03/10 13:39:08  fred
 * Adding the hardware dependent stuff to smoothly handle multi
 * architecture compilation of this kernel
 *
 * Revision 1.1.1.1  2002/02/28 12:58:55  disydent
 * Creation of Disydent CVS Tree
 *
 * Revision 1.1.1.1  2001/11/19 16:55:38  pwet
 * Changing the CVS tree structure of disydent
 *
 * Revision 1.2  2001/11/09 14:51:05  fred
 * Remove the reset executable and make the assembler be piped through
 * cpp.
 * The ldscript is not usable. I have to think before going on
 *
 * $Id: user.s 8 2005-01-27 13:42:44Z wahid $
 */



/*********************  uputs  evolue : vers une adresse de tty precis ****************************/

.text
.align 2
.globl uputs_adr
.ent uputs_adr
.set noreorder

uputs_adr:
   addiu     $sp, $sp, -16
   sw        $2, ($sp)
   sw	     $3, 4($sp)
   sw        $8, 8($sp)
   sw 	     $31, 12($sp)
   addu      $8, $0, $5 
   lb        $2, ($8)
   addiu     $3, $4, 0
   beq       $2, $0, endputs_adr
   nop
   addiu     $8, $8, 1

loop_adr:
   sb        $2, ($3)
   lb        $2, ($8)
   addiu     $8, $8, 1
   bne       $2, $0, loop_adr
   nop

endputs_adr:
   lw         $2, ($sp)
   lw	      $3, 4($sp)
   lw         $8, 8($sp)
   lw 	      $31, 12($sp)
   jr         $31
   addiu      $sp, $sp, 16

.end uputs_adr

.set reorder





/********************* fonctionqui permet de sauter aveuglement à une adresse  ***********/

.text
.align 2
.globl jump
.ent jump
.set noreorder

jump :
  addu	$31, $4, $0
  jr $31
  nop
.end jump
.set reorder





/* fonction pour avoir la position de la pile */

/********************* fonction pos_pile  ***********/

.text
.align 2
.globl pos_pile
.ent pos_pile
.set noreorder

pos_pile :
  addu	$2, $sp, $0
 jr $31
 nop
.end pos_pile
.set reorder






/********************* fonction get_lo  ***********/
.text
.align 2
.globl get_lo
.ent get_lo
.set noreorder
.set noat
get_lo	:
	mflo	$2
	jr	$31
	nop
.end get_lo
.set reorder







/********************* fonction execute  ***********/
.text
.align 2
.globl execute
.ent execute
.set noreorder
.set noat
execute :
	addiu	$29, $29, -16
	sw	$10, ($29)
	sw	$8, 4($29)
	sw	$7, 8($29)
	sw	$31, 12($29)
	la 	$10, 0x20000000
	addiu	$8, $0, 35
	la	$7, 0xa0000000	
	sb	$8, ($7)
	addiu	$8, $0, 10
	sb	$8, ($7)
	jal	$10
	nop
	lw	$10, ($29)
	lw	$8, 4($29)
	lw	$7, 8($29)
	lw	$31, 12($29)
	jr 	$31
	addiu	$29, $29, 16
.end execute
.set reorder








/********************* fonction mult  ***********/
.text
.align 2
.globl mult
.ent mult
.set noreorder
.set noat
mult :

	addiu	$sp, $sp, -16	
	sw	$4, ($sp)	/* Save du contexte */
	sw	$5, 4($sp)	
	sw	$6, 8($sp)	
	addiu	$2, $0 , 0	
calcul_mult :
	beqz	$5, fin_mult		
	andi    $6, $5, 0x1	
	beqz	$6, decalage_mult	
	srl	$5, $5, 1
	add	$2, $2, $4
decalage_mult :
	j 	calcul_mult
	sll 	$4, $4, 1	
fin_mult	:
	mtlo	$2
	mthi	$0
	lw 	$6, 8($sp)
	lw 	$5, 4($sp)
	lw 	$4, ($sp)
	jr	$31
	addiu	$sp, $sp, 16	
.end mult
.set reorder






/********************* fonction div  ***********/
.text
.align 2
.globl div
.ent div
.set noreorder
.set noat
div :
	addiu	$sp, $sp, -16	
	sw	$4, ($sp)	/* Save du contexte */
	sw	$5, 4($sp)	
	sw	$6, 8($sp)
	sw	$7, 12($sp)
	subu	$6, $4, $5      /* si a<b, on va a la fin  */
	addiu	$2, $0 , 0      /* initialisations des registres */
	addiu 	$7, $0, 1
	bltz	$6, fin_div    /* je teste ici pour eviter la dependance de $6 */
decalage_init:
	sll	$5, $5, 1       /* on cherche combien de fois 2 puissance n $5  il y a dans $4 */
	subu	$6, $4, $5
	bltz	$6, sortie_init
	nop 
	j	decalage_init
	sll	$7, $7, 1
sortie_init :
	srl	$5, $5, 1
calcul_div :
	subu	$6, $4, $5
	bltz	$6, suite_div
	nop
	subu	$4, $4, $5
	beqz	$4, fin_div 
	add	$2, $2, $7
suite_div :
	srl	$5, $5, 1
	srl 	$7, $7, 1
	bgtz	$7, calcul_div
	nop
fin_div	:
	mtlo	$2
	mthi 	$4
	lw 	$4, ($sp)
	lw 	$5, 4($sp)
	lw 	$6, 8($sp)
	lw	$7, 12($sp)	
	jr	$31
	addiu	$sp, $sp, 16	

.end div
.set reorder





/********************* fonction mult_div - appeler pendant la proc d'exception  ***********/

.rdata
mess1  :  .ascii "fonction mult_div !\n\000"
mess2  :  .ascii "entree dans operande !\n\000"
mess3  :  .ascii "entree dans mult !\n\000"
mess4  :  .ascii "entree dans div !\n\000"
mess5  :  .ascii "multiplication !\n\000"
mess6  :  .ascii "retourne !\n\000"

.text
.align 2
.globl mult_div
.ent mult_div
.set noreorder
.set noat

/* $26 contient l'instruction fautive */

mult_div :
  andi	$4, $26, 0x3F
  li 	$5, 0x3F      /* codop de multiplication */
  beq   $4, $5, goto_mult
  nop
  li    $5, 0x3C
  beq   $4, $5, goto_div
  nop
  jr	$31		/* si ce n'est une exception de mult ou div on repart */
  nop
operande :

  /*  on va supooser que les registres de calculs vont du registres 1 au registre 15  */
  /*  sinon on plante, enfin c plus compliqué */

  srl 	$4, $26, 16     /* recherche de Rt */
  andi	$5, $4, 0x1F    
  sll 	$5, $5, 2      /* multiplie par 4 */
  addu	$5, $29, $5
  lw	$6, ($5)       /* du coup , j'ai Rt dans le registre R6 */
  srl 	$4, $4, 5     /* recherche de Rs */
  andi	$5, $4, 0x1F    
  sll 	$5, $5, 2      /* multiplie par 4 */
  addu	$5, $29, $5
  lw	$7, ($5)       /* du coup , j'ai Rs dans le registre R7 */
  jr	$31
  nop

goto_mult :

  jal 	operande
  nop
  addiu	$4, $7, 0
  addiu $5, $6, 0

  jal	mult
  nop
  sw	$2, 23*4($29)
  sw	$0, 22*4($29)
  j retourne
  nop

goto_div :
  jal 	operande
  nop
  addiu	$4, $7, 0
  addiu $5, $6, 0
  jal	div
  nop
  sw	$2, 23*4($29)
  mfhi	$2
  sw	$2, 22*4($29)
  
  j retourne 
  nop
.end mult_div
.set reorder





/********************* fonction retourne - retour d'exception 2  ***********/

.text
.align 2
.globl retourne
.ent retourne
.set noreorder
.set noat

retourne :
         lw     $1,    1*4($29)
         lw     $2,    2*4($29)
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
/*         mtlo  $26*/

         lw    $26,   21*4($29)   /* This is the epc of this context */
         lw    $27,   24*4($29)   /* This is the offset: 4 excep, 0 int */
         addu  $29,   $29, 26*4   /* Stack update : fill lw stall */
         addu  $26,   $26, $27    /* Addr for return from excep */
         j     $26                /* Jump back where the exception occured*/
         rfe                      /* And restore the original mode*/
.end retourne
.set reorder




