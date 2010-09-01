#include "system.h"
#include "stdio.h"
#include "stdlib.h"
#include "matrice.h"

#include "../segmentation.h"

#define NPROCS 16
#define SIZE 500 //1000
#define SORT_TYPE 1

extern unsigned int __SortArr1;
extern unsigned int __SortArr2;
extern unsigned int __SortArr3;

volatile int compteur=NPROCS;
volatile int nprocs=NPROCS;
volatile int mmu=0;

unsigned int SortArr0[4*(SIZE+200)];
unsigned int *SortArr1 = &__SortArr1;
unsigned int *SortArr2 = &__SortArr2;
unsigned int *SortArr3 = &__SortArr3;

void SORT(int *base, int n, int type);
void insertion_sort(int *base, int n);
void selection_sort(int *base, int n);
void bubble_sort(int *base, int n);
void quick_sort(int *base, int n);

int main()
{

unsigned int *mappArray[4] = {SortArr0, SortArr1, SortArr2, SortArr3};

register int p; 

p=procnum();


  puts("Hello from processor ");
  putchar(p+'0');
  putchar('\n');


  int i;
  int j;

  if(p+1 <= nprocs){
        i=1;
        puts("Memory copy \n");
        memcpy(&mappArray[p/4][(p % 4)*(SIZE+200)], gQSortNum0 + ( (4*(p+1)*i) % 32) ,SIZE);

        puts("Sort... \n");
        SORT(&mappArray[(p/4)][(p % 4)*(SIZE+200)], SIZE, SORT_TYPE);

        for (j = 1; j < SIZE; j++)
        {
          if (mappArray[p/4][(p % 4)*(SIZE+200)+j] < mappArray[p/4][(p%4)*(SIZE+200) + j - 1])
                {
                        puts("ucbqsort: failed\n");
                        while(1);
                }

        }
        puts("ucbqsort: success\n");

  }

  while(1);
}

//---- insertion sort : non adapt� pour tableaux de grande taille (> 100) --
void insertion_sort(int *base, int n) 
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
void selection_sort(int *base, int n)
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
void bubble_sort(int *base, int n)
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
//-------------------------------
void quick_sort(int *base, int n)
{}

//-------------------------------------*/
void SORT(int *base, int n, int type)
{
  switch(type)
  {
  case 0:
    quick_sort(base, n);
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


