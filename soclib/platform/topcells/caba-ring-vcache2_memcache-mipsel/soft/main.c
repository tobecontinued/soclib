#include "system.h"
#include "stdio.h"
#include "stdlib.h"


#include "../segmentation.h"

#define NPROCS 4
#define SIZE 100
#define SORT_TYPE 1

volatile int lock=0;
volatile int compteur=NPROCS;
volatile int nprocs=NPROCS;

// 100 elements
unsigned int gQSortNum0[100] = {
  251,  24, 113, 170, 215,  60,  83,  30, 115,  48, 169, 210,  49,  92, 101,  58,  21, 184, 225,   6,
  199, 244, 227, 146,  99,  96,  25, 222,  65, 140, 213,  22, 219, 136, 175, 182,  73,  36, 141, 190, 
   83, 112, 137, 114, 175, 188, 187, 102,  53, 168, 193, 154, 167, 172,   3, 242,  67,  64, 144, 137,
  142, 175, 188,  69, 102,  53, 168, 193, 102,  89, 172, 253, 242,  67, 192,   7,  62, 159,  20, 181,
  182, 187, 216, 207,  22, 105, 132, 109, 162, 205,  16, 151,  18, 113, 228,  37,   6,  85,   8, 161
};

unsigned int SortArr0[2*(SIZE)];

void SORT(int *base, int n, int size, int (*compar) (),int type);
void QSORT(int *base, int n, int size, int (*compar) ());
void simple_sort(int *base, int n, int size, int (*compar) ());
void insertion_sort(int *base, int n, int size, int (*compar) ());
void bubble_sort(int *base, int n, int size, int (*compar) ());
int compare(int *n1, int *n2)
{
  return (*((unsigned int *) n1) - *((unsigned int *) n2));
}

int main()
{
/* -------------------------------------------------- */
	// define the page table entry
        uint32_t * ptd_table;
        ptd_table = (uint32_t *)PTD_ADDR;

        ptd_table[0] = 0x00000000; 	// unused
        //ptd_table[1] = 0x40040203; 	// PTD for instruction 	 
        ptd_table[1] = 0x410100C0; 	// PTD for instruction 	 
        //ptd_table[2] = 0x40040202; 	// PTD for tty
        ptd_table[2] = 0x41010080; 	// PTD for tty
        ptd_table[4] = 0x80000114; 	// PTE for proc
        ptd_table[8] = 0x80000214; 	// PTE for proc
        ptd_table[64] = 0x80001034; 	// PTE for data ram
        ptd_table[512] = 0x8000802E; 	// PTE for exception
        //ptd_table[128] = 0x80002014; 	// PTE for mc_r
        ptd_table[704] = 0x8000B014; 	// PTE for xram
        ptd_table[832] = 0x8000D014; 	// PTE for timer
        ptd_table[896] = 0x8000E014; 	// PTE for lock
        //ptd_table[960] = 0x8000F014; 	// PTE for proc
	
	// tty pte
        uint32_t * pte_table;
        pte_table = (uint32_t *)PTE_ADDR;
        pte_table[0] = 0x83008014;	//tty no global

	// instruction pte
        uint32_t * ipte_table;
        ipte_table = (uint32_t *)IPTE_ADDR;
	ipte_table[0] = 0x8001002C;

	puts("Page table are defined! \n");

        //static inline void set_cp2(uint32_t val, uint32_t reg), file : common/system_mips.h

	// context switch and tlb mode change
	set_cp2(0x04040000, 0x0);	// context switch
	set_cp2(3, 0x1);		// TLB enable
	puts("Context switch(TLB flush) and TLB enable!\n");

	set_cp2(0, 0x2);
	puts("icache flush done!\n");

	set_cp2(0, 0x3);
	puts("dcache flush done!\n");

	// cache inval
	set_cp2(0x004000a4, 0x6);
	set_cp2(0x004000c4, 0x6);
	puts("icache invalidation test good :-)\n");
	set_cp2(0x00800000, 0x7);
	puts("dcache invalidation test good :-) \n");

	// tlb inval
	puts("TLB invalidation test begin: \n");
	set_cp2(0x00400000, 0x4);
	puts("itlb invalidation test good :-) \n");
	set_cp2(0x00800000, 0x5);
	puts("dtlb invalidation test good :-) \n");

/* -------------------------------------------------- */

  register int p; 

  p=procnum();

  puts("Hello from processor ");
  putchar(p+'0');
  putchar('\n');


  int i;
  int j;
  int temp;

  if(p+1 <= nprocs){
	for(i=1 ; i <= 8 ; i++ ){
  	puts("Memory copy \n");
	memcpy(SortArr0+p*(SIZE), gQSortNum0 + ( (4*(p+1)*i) % 32) ,SIZE);
	
  	puts("Sort... \n");
  	SORT((int *) (SortArr0+p*(SIZE)), (int) SIZE, sizeof(unsigned int), compare,SORT_TYPE);
  	for (j = 1; j < SIZE; j++)
    	{
      		if (SortArr0[p*(SIZE)+j] < SortArr0[p*(SIZE) + j - 1])
		{
	  		puts("ucbqsort: failed\n");
			while(1);
		}

    	}
  	puts("ucbqsort: success\n");
	}
  }
  

  if(p+1 <= nprocs){
	lock_lock(&lock);
	compteur--;
	if(!compteur){
  		putchar('\n');
  		puti(temp);
  		putchar('\n');
	}
	lock_unlock(&lock);
  }

  while(1);
}

static int (*qcmp) ();
static int qsz;
static int thresh;
static int mthresh;
static void qst(int *, int *);
void QSORT(base, n, size, compar)
     int *base;
     int n;
     int size;
     int (*compar) ();
{
  register int c, *i, *j, *lo, *hi;
  int *min, *max;
  if (n <= 1)
    return;
  qsz = size;
  qcmp = compar;
  thresh = 4;
  mthresh = 6;
  max = base + n;
  if (n >= 4)
    {
      qst(base, max);
      hi = base + thresh;
    } 
  else
    {
      hi = max;
    }

  puts("Quick sort done... \n");

  for (j = lo = base; (lo ++) < hi;)
    if ((*qcmp) (j, lo) > 0)
      j = lo;
  if (j != base)
    {
      for (i = base, hi = base + 1; i < hi;)
	{
	  c = *j;
	  *j++ = *i;
	  *i++ = c;
	}

    }
  for (min = base; (hi = min ++) < max;)
    {
      while ((*qcmp) (hi --, min) > 0)
	;
      if ((hi ++) != min)
	{
	  for (lo = min + 1; --lo >= min;)
	    {
	      c = *lo;
	      for (i = j = lo; (j --) >= hi; i = j)
		*i = *j;
	      *i = c;
	    }

	}
    }

}


void simple_sort(base, n, size, compar)
     int *base;
     int n;
     int size;
     int (*compar) ();
{
  register int c, *i, *j, *lo, p, *k;
  int *max;
  if (n <= 1)
    return;
  qsz = size;
  qcmp = compar;
  max = base + n;
  puts("Selection Sort\n");
  p=procnum();
//  k=4;
  for(j = base; j < max ; j++){
    lo=j;
    c=(*lo);
    for(i = j; i < max ; i++){
	asm("	nop		\n"); //1
      if( c-(*i) > 0){
 	lo=i;
        c=(*lo);
      }
      asm("	nop		\n"); // 1
    }
    *lo = *j;
    *j = c;
  }
}

void insertion_sort(base, n, size, compar)
     int *base;
     int n;
     int size;
     int (*compar) ();
{
  register int c, *i, *j,p;
  int *max;
  if (n <= 1)
    return;
  qsz = size;
  qcmp = compar;
  max = base + n;
  puts("Insertion Sort\n");
  p=procnum();
  for(j = base; j < max-1 ; j++){
    for(i = j; i < max-1 ; i++){
      if( (*i)-(*(i+1)) > 0){
 	c = *i;
	*i = *(i+1);
	*(i+1)=c;
      }
    }
  }
}


void bubble_sort(base, n, size, compar)
     int *base;
     int n;
     int size;
     int (*compar) ();
{
  register int c, *i;
  register int b_swap=1;
  int *max;
  if (n <= 1)
    return;
  qsz = size;
  qcmp = compar;
  max = base + n;
  puts("Bubble Sort\n");
  while(b_swap){
    b_swap=0;
    for(i = base; i < max-1 ; i++){
      if( (*i)-(*(i+1)) > 0){
	b_swap=1;
 	c = *i;
	*i = *(i+1);
	*(i+1)=c;
      }
    }
  }
}

static
void qst(base, max)
     int *base, *max;
{
  register int c, *i, *j, *jj;
  register int ii;
  int *mid, *tmp;
  int lo, hi;
  lo = max - base;
  do
    {
      mid = i = base + 1 * ((lo / qsz) >> 1);
      if (lo >= mthresh)
	{
	  j = ((*qcmp) ((jj = base), i) > 0 ? jj : i);
	  if ((*qcmp) (j, (tmp = max - 1)) > 0)
	    {
	      j = (j == jj ? i : jj);
	      if ((*qcmp) (j, tmp) < 0)
		j = tmp;
	    }

	  if (j != i)
	    {
	      ii = 1;
	      do
		{
		  c = *i;
		  *i++ = *j;
		  *j++ = c;
		}

	      while (--ii);
	    }

	}
      for (i = base, j = max - 1;;)
	{
	  while (i < mid && (*qcmp) (i, mid) <= 0)
            i ++;
	  while (j > mid)
	    {
	      if ((*qcmp) (mid, j) <= 0)
		{
		  j --;
		  continue;
		}

	      tmp = i + 1;
	      if (i == mid)
		{
		  mid = jj = j;
		} 
	      else
		{
		  jj = j;
		  j --;
		}

	      goto swap;
	    }

	  if (i == mid)
	    {
	      break;
	    } 
	  else
	    {
	      jj = mid;
	      tmp = mid = i;
	      j --;
	    }

	swap:
	  ii = 1;
	  do
	    {
	      c = *i;
	      *i++ = *jj;
	      *jj++ = c;
	    }

	  while (--ii);
	  i = tmp;
	}

      i = (j = mid) + 1;
      if ((lo = j - base) <= (hi = max - i))
	{
	  if (lo >= thresh)
            qst(base, j);
	  base = i;
	  lo = hi;
	} 
      else
	{
	  if (hi >= thresh)
            qst(i, max);
	  max = j;
	}

    }
  while (lo >= thresh);
}

void SORT(int *base, int n, int size, int (*compar) (),int type){
  switch(type){
  case 0:
    QSORT(base, n, size, compar);
    break;
  case 1:
    simple_sort(base, n, size, compar);
    break;
  case 2:
    insertion_sort(base, n, size, compar);
    break;
  case 3:
    bubble_sort(base, n, size, compar);
    break;
  default:
    break;
  }

}


