.text
.align 2

.globl procnum 
.ent procnum 
procnum:
		mfc0	$2, $15
		andi	$2, $2, 0x3ff
		j		$31
		nop
.end procnum 
