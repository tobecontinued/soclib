.text
.align 2
.globl uputs
.ent uputs
.set noreorder

uputs:
   addu      $8, $0, $4 
   la        $3, 0xc0000000

   addu      $9,$0,$5
   sll       $9,$9,24
   or	     $3,$3,$9

   /*mfc0      $2, $0
   addiu     $2, $2, 48 
   sb        $2, ($3)*/

   lb        $2, ($8)
   beq       $2, $0, endputs
   nop
   addiu     $8, $8, 1

loop:
   sb        $2, ($3)
   lb        $2, ($8)
   addiu     $8, $8, 1
   bne       $2, $0, loop
   nop

endputs:
   addiu      $2, $0, 0xA
   sb         $2, ($3)
   j          $31
   addu       $2, $0, 0
.end uputs

.set reorder

