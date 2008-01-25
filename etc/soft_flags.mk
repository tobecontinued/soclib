mipsel_CFLAGS=-mips2 -mno-branch-likely
mipseb_CFLAGS=-mips2 -mno-branch-likely
powerpc_CFLAGS=-mcpu=405 -mstrict-align
microblaze_CFLAGS=-mno-xl-soft-div -mno-xl-soft-mul
microblaze_LDFLAGS=-nostdlib

mipsel_CC_PREFIX=mipsel-unknown-elf-
mipseb_CC_PREFIX=mips-unknown-elf-
powerpc_CC_PREFIX=powerpc-unknown-elf-
microblaze_CC_PREFIX=mb-

-include $(HOME)/.soclib/soft_compilers.conf
