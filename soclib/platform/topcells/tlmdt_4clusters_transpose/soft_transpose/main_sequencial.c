#include "stdio.h"

#define NLINES		128
#define NPIXELS		128
#define NBLOCKS		NLINES*NPIXELS/512
#define NMAX		20
#define FALSE		0
#define TRUE		1

volatile int	buf_in_empty	= TRUE;
volatile int	buf_out_empty 	= TRUE;

unsigned char 	buf_out[NLINES][NPIXELS];
unsigned char 	buf_in[NLINES][NPIXELS];

////////////////////
void load(int image)
{
    int image = 0;

    while(image < NMAX) 
    {
        while ( buf_in_empty == FALSE ) {}	// synchro
        
        tty_printf("\n *** Starting load for image %d *** at cycle %d \n", image, proctime());
        if( ioc_read(image*NBLOCKS, buf_in, NBLOCKS) )
        {
            tty_printf("echec ioc_read\n");
            exit();
        }
        if ( ioc_completed() )
        {
            tty_printf("echec ioc_completed\n");
            exit();
        }

        buf_in_empty = FALSE;

        tty_printf(" *** Completing load for image %d *** at cycle %d \n", image, proctime());
        image++;
    } // end while image      
    exit();
} // end load()

/////////////////////
void transpose(int image)
{
    int dx,dy,g;
    int l,p;
//    int image = 0;

//    while( image < NMAX )
    { 
        while( (buf_in_empty == TRUE) || (buf_out_empty == FALSE) ) {}	// synchro

        tty_printf("\n *** Starting transpose for image %d *** at cycle %d \n", image, proctime());
        for( l=0 ; l<NLINES ; l++)
        {
            for( p=0 ; p<NPIXELS ; p++)
            {
/*
                if ( (l==NLINES-1) || (l==0) || (p==NPIXELS-1) || (p==0) )
                {
	            buf_out[l][p] = 0;
                }
                else
	        { 
                    // derivee en x
	            dx = (int)buf_in[l-1][p-1] 
                       + (int)buf_in[l][p-1]*2
                       + (int)buf_in[l+1][p-1]
	               - (int)buf_in[l-1][p+1]
                       - (int)buf_in[l][p+1]*2
                       - (int)buf_in[l+1][p+1];
	            dy = (int)buf_in[l-1][p-1]
                       + (int)buf_in[l-1][p]*2
                       + (int)buf_in[l-1][p+1]
	               - (int)buf_in[l+1][p-1]
                       - (int)buf_in[l+1][p]*2 
                       - (int)buf_in[l+1][p+1];
	            if ( dy < 0 ) dy = -dy;
                    g = dx + dy;
                    if ( g > 255 ) g = 255;
                    buf_out[l][p] = g;
                }
*/
                    buf_out[l][p] = buf_out[p][l];
                
            } // end for p
        } // end for l

        buf_in_empty = TRUE;
        buf_out_empty = FALSE;

        tty_printf(" *** Completing transpose for image %d *** at cycle %d \n", image, proctime());
//        image++;
    } // end while image
//    exit();
} // end transpose

///////////////////////
void display(int image)
{
//    int	image = 0;

//    while(image < NMAX)
    {
        while( buf_out_empty == TRUE ) {} // synchro

        tty_printf("\n *** starting display for image %d at cycle %d\n", image, proctime());
        if ( fb_sync_write(0, buf_out, NLINES*NPIXELS) )
        {
                tty_printf("echec fb_sync_write\n");
                exit();
        }

        buf_out_empty = TRUE;

        tty_printf(" *** completing display for image %d at cycle %d\n", image, proctime());
//        image++;
    } // end while image
//    exit();
} // end display

///////////
void main()
{
    int image = 0;

    while(image < 5)
    {
        load(image);
        transpose(image);
        display(image);
        image++;
    }
    exit();
} // end main
