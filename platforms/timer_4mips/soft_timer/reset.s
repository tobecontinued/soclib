  .extern     _stack
  .extern     _gp

  .text
  .align 2

   li         $26,0x0000FF15
   mtc0       $26,$12

   la         $2,     _edata
   la         $3,     _end
   li         $4,     0

   /* set bss to zero */
loop:
   beq        $2, $3, endloop
   nop
   sw         $4, 0($2)
   addiu      $2, $2, 4
   j          loop
endloop:
   mfc0       $2,$0
   beq        $2,$0,toMain0
   nop
   ori        $3,$0,1
   beq        $2,$3,toMain1
   nop
   ori        $3,$0,2
   beq        $2,$3,toMain2
   nop
   ori        $3,$0,3
   beq        $2,$3,toMain3
   nop 
   j          endloop
   nop
toMain0: 
   la         $sp,    _stack - 4
   la         $28,    _gp
   la         $27,    main0
   j          $27
   nop
toMain1: 
   la         $sp,    _stack - 256 - 4
   la         $28,    _gp
   la         $27,    main1
   j          $27
   nop
toMain2: 
   la         $sp,    _stack - 512 - 4
   la         $28,    _gp
   la         $27,    main2
   j          $27
   nop
toMain3: 
   la         $sp,    _stack - 768 - 4
   la         $28,    _gp
   la         $27,    main3
   j          $27
   nop




