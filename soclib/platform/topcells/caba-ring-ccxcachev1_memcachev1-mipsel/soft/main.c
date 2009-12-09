#include "system.h"
#include "stdio.h"
#include "stdlib.h"
#include "matrice.h"

#include "../segmentation.h"

#define NPROCS 4
#define SIZE 1000 
#define SORT_TYPE 0 //3

//extern int _malek; //_gp;

volatile int nprocs=NPROCS;

unsigned int SortArr0[NPROCS*(SIZE+200)];
//unsigned int SortArr0[4*4*SIZE];

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

//  printf("_malek = %p\n", &_malek);   

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
        printf( "nombre cycles cpu : %i\n", end_cycle-beg_cycle);
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


//---- insertion sort : non adapt� pour tableaux de grande taille (> 100) --
void insertion_sort(unsigned int *base, unsigned int n) 
{
    /* Sp�cifications externes : Tri du tableau base par insertion s�quentielle */
    int i,p,j;
    int x;

    puts("Insertion Sort\n");

    for (i = 1; i < n; i++) 
    {

        putchar('-'); // added for debug

        /* stockage de la valeur en i */
        x = base[i]; 
 
        /* recherche du plus petit indice p inf�rieur � i tel que base[p] >= base[i] */
        for(p = 0; base[p] < x; p++);
        /* p pointe une valeur de base sup�rieure � celle en i */ 
 
        /* d�calage avant des valeurs de base entre p et i */         
        for (j = i-1; j >= p; j--) {
            base[j+1] = base[j]; 
        }   
 
        base[p] = x; /* insertion de la valeur stock�e � la place vacante */

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
        int i   = 0; /* Indice de r�p�tition du tri */
 	int j   = 0; /* Variable de boucle */
 	int tmp = 0; /* Variable de stockage temporaire */
        int en_desordre = 1; /* Bool�en marquant l'arr�t du tri si le tableau est ordonn� */

        puts("Bubble Sort\n");

	/* Boucle de r�p�tition du tri et le test qui arr�te le tri d�s que le tableau est ordonn� */
	for(i = 0 ; (i < n) && en_desordre; i++)
	{
                putchar('-'); // added for debug

		/* Supposons le tableau ordonn� */
		en_desordre = 0;
		/* V�rification des �l�ments des places j et j-1 */
		for(j = 1 ; j < n - i ; j++)
		{
			/* Si les 2 �l�ments sont mal tri�s */
			if(base[j] < base[j-1])
			{
				/* Inversion des 2 �l�ments */
 				tmp = base[j-1];
 				base[j-1] = base[j];
 				base[j] = tmp;
 
 				/* Le tableau n'est toujours pas tri� */
				en_desordre = 1;
 			}
		}

                putchar('+'); // added for debug
	}

}
//------------------------------------------------------
/*
 * Ex�cute un tri par insertion avec la s�paration donn�e
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
     * gaps[] doit approximer une S�rie g�om�trique.
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

