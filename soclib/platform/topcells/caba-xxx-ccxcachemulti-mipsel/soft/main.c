#include "system.h"
#include "stdio.h"
#include "stdlib.h"
#include "matrice.h"

#include "../segmentation.h"

#define NPROCS 4
#define SIZE 1000 
#define SORT_TYPE 0 // shellSort

volatile int nprocs=NPROCS;

/*
unsigned int gQSortNum0[200] = {
  251,  24, 113, 170, 215,  60,  83,  30, 115,  48, 169, 210,  49,  92, 101,  58,  21, 184, 225,   6,
  199, 244, 227, 146,  99,  96,  25, 222,  65, 140, 213,  22, 219, 136, 175, 182,  73,  36, 141, 190, 
   83, 112, 137, 114, 175, 188, 187, 102,  53, 168, 193, 154, 167, 172,   3, 242,  67,  64, 144, 137,
  142, 175, 188,  69, 102,  53, 168, 193, 102,  89, 172, 253, 242,  67, 192,   7,  62, 159,  20, 181,
  182, 187, 216, 207,  22, 105, 132, 109, 162, 205,  16, 151,  18, 113, 228,  37,   6,  85,   8, 161,
   58, 135,  76,  35, 174,  35, 224,  39, 158, 127, 180, 149,  86, 155, 200, 239, 118, 119,  28,  77,
  254,  19, 176, 183,  78, 145, 132,   5,  90, 117, 152, 127, 218, 153,  20,  67, 178,   3, 128, 185,
  254,  95, 172, 139, 246, 123, 104,  15,  42, 169,  68, 211,  98, 243,  80,  41, 174,  79,  36,  27,
  186, 149,  86, 155, 200, 239, 118, 119,  28,  77, 254,  19, 176, 183,  78, 145, 132,   5,  90, 117,
  152, 127, 218, 153,  20,  67, 178,   3, 128, 185, 254,  95, 172, 139, 246, 123, 104,  15,  42, 169 };
*/

unsigned int SortArr0[NPROCS*(SIZE+200)];

void SORT(unsigned int *base, unsigned int n, int type);
void insertion_sort(unsigned int *base, unsigned int n); // type 2
void selection_sort(unsigned int *base, unsigned int n); // type 1
void bubble_sort(unsigned int *base, unsigned int n);    // type 3
void shellSortPhase(unsigned int a[],unsigned int length, int gap);
void shellSort(unsigned int *base, unsigned int n);     // type 0

int main()
{
  register int p; 

  int beg_cycle, end_cycle;

  beg_cycle = cpu_cycles();

  p=procnum();

  puts("Hello from processor ");
  putchar(p+'0');
  putchar('\n');

  int i;
  int j;
  unsigned int* SortArray;

  if(p+1 <= nprocs)
  {
        i=1;
  	puts("Memory copy \n");
        SortArray = SortArr0 + p*(SIZE+200);
	memcpy(SortArray, gQSortNum0 + p*SIZE,SIZE*4);
	//memcpy(SortArray, gQSortNum0,SIZE*4);

 	puts("Sort... \n");
        SORT((unsigned int *) (SortArray), (unsigned int) SIZE, SORT_TYPE);

  	for (j = 1; j < SIZE; j++)
    	{
	  if (SortArray[j] < SortArray[j-1])
		{
	  		puts("ucbqsort: failed\n");
			while(1);
		}

    	}

  	puts("ucbqsort: success\n");
        end_cycle = cpu_cycles();
//      printf( "nombre cycles cpu : %i\n", end_cycle-beg_cycle);
        printf( "begin time        : %i\n", beg_cycle);
        printf( "end time          : %i\n", end_cycle);
  }

  
// puts("Display the sorted array : \n");
// for(j = 0; j < SIZE; j++)
// {
//   puti(SortArray[j]);
//   putchar('\n');
// }

//  printf( "------------------------------ \n");
//  printf( "nombre cycles cpu : %i\n", end_cycle-beg_cycle);
//  printf( "------------------------------ \n");


  while(1);
}


//---- insertion sort : non adapté pour tableaux de grande taille (> 100) --
void insertion_sort(unsigned int *base, unsigned int n) 
{
    /* Spécifications externes : Tri du tableau base par insertion séquentielle */
    int i,p,j;
    int x;

    puts("Insertion Sort\n");

    for (i = 1; i < n; i++) 
    {

        putchar('-'); // added for debug

        /* stockage de la valeur en i */
        x = base[i]; 
 
        /* recherche du plus petit indice p inférieur à i tel que base[p] >= base[i] */
        for(p = 0; base[p] < x; p++);
        /* p pointe une valeur de base supérieure à celle en i */ 
 
        /* décalage avant des valeurs de base entre p et i */         
        for (j = i-1; j >= p; j--) {
            base[j+1] = base[j]; 
        }   
 
        base[p] = x; /* insertion de la valeur stockée à la place vacante */

        putchar('+'); // added for debug

    }
}

//------ simple_sort -------------------------------
void selection_sort(unsigned int *base, unsigned int n)
{
     int i, min, j , x;
     puts("Selection Sort\n");

     for(i = 0 ; i < n - 1 ; i++)
     {

         putchar('-'); // added for debug

         min = i;

         
         for(j = i+1 ; j < n ; j++)
         {
        
            if(base[j] < base[min])
                  min = j;
            
         }

         if(min != i)
         {
             x = base[i];
             base[i] = base[min];
             base[min] = x;
         }

         putchar('+'); // added for debug

     }
}
//-------------------------------
void bubble_sort(unsigned int *base, unsigned int n)
{
        int i   = 0; /* Indice de répétition du tri */
 	int j   = 0; /* Variable de boucle */
 	int tmp = 0; /* Variable de stockage temporaire */
        int en_desordre = 1; /* Booléen marquant l'arrêt du tri si le tableau est ordonné */

        puts("Bubble Sort\n");

	/* Boucle de répétition du tri et le test qui arrête le tri dès que le tableau est ordonné */
	for(i = 0 ; (i < n) && en_desordre; i++)
	{
                putchar('-'); // added for debug

		/* Supposons le tableau ordonné */
		en_desordre = 0;
		/* Vérification des éléments des places j et j-1 */
		for(j = 1 ; j < n - i ; j++)
		{
			/* Si les 2 éléments sont mal triés */
			if(base[j] < base[j-1])
			{
				/* Inversion des 2 éléments */
 				tmp = base[j-1];
 				base[j-1] = base[j];
 				base[j] = tmp;
 
 				/* Le tableau n'est toujours pas trié */
				en_desordre = 1;
 			}
		}

                putchar('+'); // added for debug
	}

}
//------------------------------------------------------
/*
 * Exécute un tri par insertion avec la séparation donnée
 * If gap == 1, on fait un tri ordinaire.
 * If gap >= length, on ne fait rien.
 */
void shellSortPhase(unsigned int a[],unsigned int length, int gap) {
    int i;
 
    puti(gap);
    for (i = gap; i < length; ++i) {
        unsigned int value = a[i];
        int j;
        for (j = i - gap; j >= 0 && a[j] > value; j -= gap) {
	    putchar('+');
            a[j + gap] = a[j];
	    putchar('-');
        }
        a[j + gap] = value;
    }
}
 
void shellSort(unsigned int *base, unsigned int n) {
    /*
     * gaps[] doit approximer une Série géométrique.
     * La sequence suivante est la meilleure connue en terme
     * de nombre moyen de comparaisons. voir:
     * http://www.research.att.com/~njas/sequences/A102549
     */
    static const int gaps[] = {
        1, 4, 10, 23, 57, 132, 301, 701
    };
    int sizeIndex;
 
    puts("Shell Sort\n");
    for (sizeIndex = sizeof(gaps)/sizeof(gaps[0]) - 1;
               sizeIndex >= 0;
               --sizeIndex)
        shellSortPhase(base, n, gaps[sizeIndex]);
}

//-------------------------------------*/
void SORT(unsigned int *base, unsigned int n, int type)
{
  switch(type)
  {
  case 0:
    shellSort(base, n);
    break;
  case 1:
    selection_sort(base, n);
    break;
  case 2:
    insertion_sort(base, n);
    break;
  case 3:
    bubble_sort(base, n);
    break;
  default:
    break;
  }
}
