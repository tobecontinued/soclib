mipseb_CC_PREFIX=mipsel-unknown-elf-
mipseb_CFLAGS=-mips2 -mno-branch-likely -gstabs+ -DSOCLIB_MIPS_R3000 -EB
mipseb_LDFLAGS=-EB

mipsel_CC_PREFIX=mipsel-unknown-elf-
mipsel_CFLAGS=-mips2 -mno-branch-likely -gstabs+ -DSOCLIB_MIPS_R3000 -EL
mipsel_LDFLAGS=-EL

mips32eb_CC_PREFIX=mipsel-unknown-elf-
mips32eb_CFLAGS=-mips32r2 -gstabs+ -DSOCLIB_MIPS32 -EB
mips32eb_LDFLAGS=-EB

mips32el_CC_PREFIX=mipsel-unknown-elf-
mips32el_CFLAGS=-mips32r2 -gstabs+ -DSOCLIB_MIPS32 -EL
mips32el_LDFLAGS=-EL

powerpc_CC_PREFIX=powerpc-unknown-elf-
powerpc_CFLAGS=-mcpu=405 -mstrict-align -gstabs+

microblaze_CC_PREFIX=mb-
microblaze_CFLAGS=-mno-xl-soft-div -mno-xl-soft-mul -gstabs+
microblaze_LDFLAGS=-nostdlib

nios2_CC_PREFIX=nios2-unknown-elf-
nios2_CFLAGS=-mhw-mul -mhw-div

arm7tdmi_CC_PREFIX = armv5b-softfloat-linux-
arm7tdmi_CFLAGS = -nostdinc -gstabs+
arm7tdmi_LDFLAGS = -nostdlib

arm966_CC_PREFIX = armv5b-softfloat-linux-
arm966_CFLAGS = -nostdinc -gstabs+
arm966_LDFLAGS = -nostdlib

lm32_CC_PREFIX 	= 	lm32-elf-
lm32_CFLAGS 	+=  -gstabs+ -mall-enabled
lm32_LDFLAGS 	+=  -nostdlib 

-include $(HOME)/.soclib/soft_compilers.conf
