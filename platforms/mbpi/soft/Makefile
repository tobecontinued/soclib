# The -mno-xl-soft-mut/div are necessary to avoid calls to library
# functions, as we assume that the MicroBlaze model contains a
# hw mult and div (should probably be made a parameter).
CC     = mb-gcc 
CFLAGS = -O3 -mno-xl-soft-div -mno-xl-soft-mul -I$(SOCLIB)/include -I$(SOCLIB)/testcase/common/
%.o : %.c; $(CC) $(CFLAGS) -c $< -o $@
%.o : %.S; $(CC) $(CFLAGS) -c $< -o $@
%.o : %.s; $(CC) $(CFLAGS) -c $< -o $@

all   : a.out

a.out : boot.o uputs.o main.o
	mb-ld -g -nostdlib -T ldscript -o $@ $^
realclean: clean
	$(RM) a.out
clean:
	$(RM) boot.o uputs.o main.o
