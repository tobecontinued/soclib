.global _crt0
_crt0:
    /* Setup stack and global pointer */
    mvhi    sp, hi(__stack)
    ori     sp, sp, lo(__stack)

    /* Clean BSS */

    mvhi    r1, hi(__bss_start)
    ori     r1, r1, lo(__bss_start)
    mvhi    r2, hi(__bss_end)
    ori     r2, r2, lo(__bss_end)

clearbss:
    be      r1, r2, gotomain 
    sw      (r1+0), r0
    addi    r1, r1, 4
    bi      clearbss

    /* Jump to main program */
gotomain:
    mv      r1,r0
    mv      r2,r0
    calli   main

    /* If we return from main ...*/   
.global _exit
_exit:
    break

