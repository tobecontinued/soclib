mipsel_CFLAGS=-mips32 -mno-branch-likely -gstabs+
mipseb_CFLAGS=-mips32 -mno-branch-likely -gstabs+
powerpc_CFLAGS=-mcpu=405 -mstrict-align -gstabs+
microblaze_CFLAGS=-mno-xl-soft-div -mno-xl-soft-mul -gstabs+
microblaze_LDFLAGS=-nostdlib
nios2_CFLAGS=-mhw-mul -mhw-div

mipsel_CC_PREFIX=mipsel-unknown-elf-
mipseb_CC_PREFIX=mips-unknown-elf-
powerpc_CC_PREFIX=powerpc-unknown-elf-
microblaze_CC_PREFIX=mb-
nios2_CC_PREFIX=nios2-unknown-elf-

-include $(HOME)/.soclib/soft_compilers.conf
