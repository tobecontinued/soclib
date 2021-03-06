/*****************************************************************************

  h264.c --  File which start the core functions
   - Function opening the input file and reading  the sps and pps fields
   - Function decoding a frame which call the functions to decode the
     slice_header and the slice_data
   - Place to start the PTHREADS for Multislice version

  Authors:
  Florian Broekaert, THALES COM - AAL, 2006-04-07
  Fabien Colas-Bigey THALES COM - AAL, 2008
  Pierre-Edouard BEAUCAMPS, THALES COM - AAL, 2009

  Copyright (C) THALES & Martin Fiedler All rights reserved.
 
  This code is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.
   
  This code is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.
   
  You should have received a copy of the GNU General Public License along
  with this code (see file COPYING).  If not, see
  <http://www.gnu.org/licenses/>.
  
  This License does not grant permission to use the name of the copyright
  owner, except only as required above for reproducing the content of
  the copyright notice.
*****************************************************************************/

#include "h264.h"

/****************************************************************************
  Variables and structures
****************************************************************************/
// Internal Variables - MUST be static
static frame *this, *ref;		// Current & reference Frames
static seq_parameter_set sps;		// Header of the sequence
static pic_parameter_set pps;		// Header of the sequence

// Thread and synchroniqation tools declaration
static pthread_t *thread;		// Thread declaration
static mode_pred_info **mpi;		// Structure containing all the arg for the thread
static sem_t ring_sem;			// Semaphore to protect the global variable Ring_buffer
static sem_t filter_sem;		// Semaphore to protect the process of the deblocking filter

static char more_frame;	/* Variable shared by all the function of this file
			   and specially between the decode_frame() function
			   and the thread function slice_process
			   It is a flag testing the presence or not of a frame to
			   be processed */

static char frame_no;			// Variable that count the number of frame in the sequence

static int32_t code_table_init_done = INIT_CODE_TABLES_KO;

// Variables defined in input.c
extern uint8_t* *nal_buf;
extern int32_t* nal_pos;
extern int32_t* nal_bit;

 
/*------------------------------------------------------------------*/
/**
   \fn void sem_wait_nosignal(sem_t *sem)
   
   \brief Correct use of the sem_wait()
   Retrieves the error message because while a thread is waiting for
   a semaphore a signal can interupt it and break the
   synchronisation
   
   @param *sem sem_t semaphore
*/
/*------------------------------------------------------------------*/
inline void sem_wait_nosignal(sem_t *sem)
{
  while (sem_wait(sem)) {
  }
}

/*------------------------------------------------------------------*/
/**
   \fn void *slice_process(void *mp)

   \brief copy nal into the nalu_slice structure
   - Read and fill up the head of the slice
   - Call the decode_slice_data function

   @param mp mode_pred_info structure unique to each thread

   @returns 0 if the sequence is over, however the thread is waiting 
   for another slice to decode
*/
/*------------------------------------------------------------------*/
void *slice_process(void *mp)/* reentrant */
{
  cpu_interrupt_enable();

  /*************************************
     Internal declaration
  *************************************/
  nal_unit nalu_slice;
  slice_header sh_slice;
  /* nal_present == 0 -> nal empty
     nal_present == 1 -> nal with infos*/
  int32_t nal_present;	   
  

  /*************************************
     Main Loop
     Thread only kills himself after
     the last frame of sequence
  *************************************/
  while(1)
    {
      /*************************************
         Copy Nal into buffer
      *************************************/
      // waits for the main thread to launch the treatment
      sem_wait_nosignal(&((mode_pred_info *)mp)->sem_start_th);

      // Ring_buffer concurrency access management
      sem_wait_nosignal(&ring_sem);
      // Detect a NAL in the ring_buf and copy it into a nal_buf
      nal_present = get_next_nal_unit_thread(&nalu_slice,(mode_pred_info *)mp);
      sem_post(&ring_sem);
      
      /*************************************
         Checking end of process
      *************************************/
      if (nal_present == 0)
	{	
	  // Update the flag shared by decode_frame(): there is no more NAL, no more frame to process
	  more_frame = 0;

	  // Managing of the concurrency : Signal the decode_frame that this thread has done its process
	  sem_post(&((mode_pred_info *)mp)->sem_stop_th);
	  pthread_exit(0);
	}
      
      
      /*************************************
         NAL Test
      *************************************/
      // Test if the NAL contains an IDR or an non IDR picture without partitioning RBSP (only supported)
      if (nalu_slice.nal_unit_type == 1 || nalu_slice.nal_unit_type == 5) 
	{

#ifdef DEBUG
	  printk("-- H.264 DEBUG -- Decoding Slice Header...\n");
#endif

	  decode_slice_header(&sh_slice, &sps, &pps, &nalu_slice, ((mode_pred_info *)mp)->ID);
	  
	  if (sh_slice.slice_type != I_SLICE && sh_slice.slice_type != P_SLICE) 
	    {
	      printk("-- H.264 -- Warning: Unsupported slice type (%s), skipping!\n",
		     _str_slice_type(sh_slice.slice_type));

	      // Managing of the concurrency : Signal the decode_frame that this thread has done its process
	      sem_post(&((mode_pred_info *)mp)->sem_stop_th);
	      pthread_exit(0);
	    } 
	  else 
	    {
	      // If this is an I or P slice (only supported)
#ifdef DEBUG
	      printk("\033[1A-- H.264 DEBUG -- Decoding slice data...\033[K\n");
#endif
	      decode_slice_data(&sh_slice, &sps, &pps, &nalu_slice, this, ref, (mode_pred_info *)mp);
	      // After this process is done, Go to the filter process if implemented
	    }
	} 
      else if (nalu_slice.nal_unit_type != 7 && nalu_slice.nal_unit_type != 8)
	{
	  printk("-- H.264 -- Warning: unexpected or unsupported NAL unit type!\n");
	  
	  // Managing of the concurrency : Signal the decode_frame that this thread has done its process
	  sem_post(&((mode_pred_info *)mp)->sem_stop_th);
	  pthread_exit(0);
	}

      /*************************************
         Deblocking filter process
      *************************************/
      //sem_wait_nosignal(&filter_sem);

      filter_slice(&sh_slice,&sps,&pps,this,(mode_pred_info *)mp);
      
      //sem_post(&filter_sem);

      // Managing of the concurrency : Signal the decode_frame that this thread has done its process
      sem_post(&((mode_pred_info *)mp)->sem_stop_th);
      
    }// End While(1) : 1 slice has been treated
  pthread_exit(NULL);
} // End slice_process


/*------------------------------------------------------------------*/
/**
   \fn int h264_open(char *filename)

   \brief opens the file and decodes the sps and pps for the sequence,

   @param filename

   @returns 0 if OK, anything else on error
*/
/*------------------------------------------------------------------*/
int32_t h264_open(char *file_in_name, char *file_out_name, int8_t slice_numbers)	/* not reentrant */
{
  /*************************************
     Internal declarations
  *************************************/
  static nal_unit nalu;
  int32_t  slice_compteur = 0;
  char have_sps = 0, have_pps = 0;
  more_frame = 1;  // we suppose there are some frames to read here

  /******************************************************
     Memory Allocation for the read of the sps & pps
  ******************************************************/
  printk("-- H.264 OPEN -- Memory allocation\n");
  nal_buf = (unsigned char **) calloc(1, sizeof(unsigned char *));
  nal_buf[0] = calloc(NAL_BUF_SIZE, sizeof(unsigned char));
  nal_bit = calloc(1, sizeof(int32_t));
  nal_pos = calloc(1, sizeof(int32_t));

  // Allocate memory for CAVLC tables
  if (code_table_init_done != INIT_CODE_TABLES_OK) {
    code_table_init_done = init_code_tables();
  }
  frame_no = 0;
  
  /*************************************
     VFS Initialization
  *************************************/
  printk("-- H.264 OPEN -- Starting file management\n");
  input_vfs_init();
  
  /*************************************
     Files opening
  *************************************/
  if (!input_open(file_in_name)) {	// Error while opening the file
    printk("-- H.264 OPEN -- Error: Cannot open input file!\n");
    return 0;
  }
  else
    printk("-- H.264 OPEN -- End of file management\n");

#if defined(GENERATE_OUTPUT_FILE)
  if (!output_open(file_out_name)) {	// Error while opening the file
    printk("-- H.264 OPEN -- Error: Cannot open output file!\n");
    return  0;
  }
#endif

  /*************************************
     Main loop
  *************************************/
  while (get_next_nal_unit(&nalu)) {
    switch (nalu.nal_unit_type) {
    case 7:			// sequence parameter set
      if (have_sps)
	printk("-- H.264 WARNING -- Duplicate sequence parameter set, skipping!\n");
      else{
	decode_seq_parameter_set(&sps);	// Method filling the sps  structure
#ifdef PRINT_PARAM_SET
	results_screening_sps(&sps);
#endif
	have_sps = 1;
      }
      break;
    case 8:			// picture parameter set
      if (!have_sps)
	printk("-- H.264 WARNING -- Picture parameter set without sequence parameter set, skipping!\n");
      else if (have_pps)
	printk("-- H.264 WARNING -- Duplicate picture parameter set, skipping!\n");
      else {
	decode_pic_parameter_set(&pps);	// Method filling the pps structure
	pps.num_slice_groups = slice_numbers;
#ifdef PRINT_PARAM_SET
	results_screening_pps(&pps);
#endif

	// Free Memory of this sps & pps reading
	free(nal_buf[0]);
	free(nal_buf);
	free(nal_bit);
	free(nal_pos);

	have_pps = 1;
	
	// Test all the unsupported features inside the sps & pps
	if (check_unsupported_features(&sps, &pps)){
	  printk("-- H.264 ERROR -- Unsupported features found in headers!\n");
	  input_close();
	  return 0;
	}
#ifdef DEBUG
	else printk("-- H.264 DEBUG -- All features of the streams should be supported\n");
#endif

	// Memory allocation of major structures for the rest of the decoding process
	// Frames memory allocation
	printk("-- H.264 MAIN -- Allocating Image data spaces...\n");
	this = alloc_frame(sps.PicWidthInSamples,
			   sps.FrameHeightInSamples);
	ref  = alloc_frame(sps.PicWidthInSamples,
			   sps.FrameHeightInSamples);

	// Initialization of the semaphore managing the ring buffer and the deblocking filter shared by all threads
	if (sem_init(&ring_sem,0,1)!=0)
	  printk("-- H.264 ERROR -- Fatal error during ring buffer semaphore initialization\n");
	if (sem_init(&filter_sem,0,1)!=0)
	  printk("-- H.264 ERROR -- Fatal error during deblocking filter semaphore initialization \n");
	
	// Thread Memory Allocation
	printk("-- H.264 MAIN -- Allocating thread memory...\n");
	
	thread = (pthread_t *) calloc(pps.num_slice_groups, sizeof(pthread_t));
	mpi =    (mode_pred_info **) calloc(pps.num_slice_groups, sizeof(mode_pred_info *));
	nal_buf =(unsigned char **) calloc(pps.num_slice_groups, sizeof(unsigned char *));
	nal_bit = calloc(pps.num_slice_groups, sizeof(int32_t));
	nal_pos = calloc(pps.num_slice_groups, sizeof(int32_t));

	/*************************************
           Loop on slices
	*************************************/
	printk("-- H.264 MAIN -- Slice thread (0/%d) launched\n", pps.num_slice_groups);
	for (slice_compteur = 0; slice_compteur < pps.num_slice_groups; slice_compteur++) {
	  nal_buf[slice_compteur] = calloc(NAL_BUF_SIZE, sizeof(unsigned char));
	  /* Modification of the size of the mpi, optimisation to fit with the slice size
	     that's why we divide the height of the frame by the number of slices in the frame */
	  mpi[slice_compteur] = alloc_mode_pred_info(sps.PicWidthInSamples,sps.FrameHeightInSamples,pps.num_slice_groups,slice_compteur);
	  
	  // Initialization of threads semaphores
	  if (sem_init(&(mpi[slice_compteur]->sem_start_th),0,0)!=0)
	    printk("-- H.264 ERROR -- Fatal error during start thread semaphore initialization \n");
	  if (sem_init(&(mpi[slice_compteur]->sem_stop_th),0,0)!=0)
	    printk("-- H.264 ERROR -- Fatal error during stop thread semaphore initialization \n");
	  
	  // Creation of threads and start of their process
	  if(pthread_create(&thread[slice_compteur], NULL, slice_process,mpi[slice_compteur]) < 0){
	    printk("-- H.264 ERROR -- 'pthread_create' error for thread %d\n",slice_compteur);
	    abort();
	  }
	  printk("\033[1A-- H.264 MAIN -- Slice thread (%d/%d) launched\n", slice_compteur + 1, pps.num_slice_groups);
	}
	// Once the pps has been read, the decoding process of the core datas can start
	return (sps.FrameHeightInSamples << 16) | sps.PicWidthInSamples;
      }
      break;
    case 1:
    case 5:			// coded slice of a picture
      printk("-- H.264 WARNING -- Pictures sent before headers!\n");
      break;
    default:			// unsupported NAL unit type
      printk("-- H.264 WARNING -- NAL unit with unsupported type, skipping!\n");
    }
  }

  printk("-- H.264 ERROR -- Unexpected end of file!\n");
  return 0;
}


/*------------------------------------------------------------------*/
/**
   \fn frame *h264_decode_frame(int32_t verbose)

   \brief Function decoding a frame by launching THREAD (multislices)

   @param verbose

   @returns decoded frame
*/
/*------------------------------------------------------------------*/
//frame *h264_decode_frame(int32_t verbose)
frame *h264_decode_frame()
{
  int32_t slice_compteur;
  frame *temp;
  uint32_t cycle_count = 0;

  /*************************************
	  Sync management
  *************************************/
  cycle_count = cpu_cycle_count();
  for (slice_compteur = 0; slice_compteur < pps.num_slice_groups; slice_compteur++)
    sem_post(&mpi[slice_compteur]->sem_start_th);

  ++frame_no;  // Update the frame_number variable

#if (defined(CONFIG_DRIVER_TIMER_SOCLIB) || defined(CONFIG_DRIVER_TIMER_EMU))
  //set_timer_value(0);
  //set_timer_value(2147483647);
#endif
  //printk("-- H.264 -- Frame %d decoding...", frame_no);

  for (slice_compteur = 0; slice_compteur < pps.num_slice_groups; slice_compteur++) {
    sem_wait_nosignal(&mpi[slice_compteur]->sem_stop_th);
  }
  
  cycle_count = cpu_cycle_count() - cycle_count;
  printk("%d\n",cycle_count);
  
  fb_display_frame(this);

#if (defined(CONFIG_DRIVER_TIMER_SOCLIB) || defined(CONFIG_DRIVER_TIMER_EMU))
  //	printk("\033[1A\n-- H.264 -- Frame %d decoded in %d ms\033[K\n", frame_no, get_timer_value() / 200000);
  //printk("\033[1A\n-- H.264 -- Frame %d decoded in %d ms\033[K\n", frame_no, get_timer_value());
  //printk("%d\n",get_timer_value());
#else
  printk("\033[1A\n-- H.264 -- Frame %d decoded\033[K\n", frame_no);
#endif

  if (more_frame == 0) {
    printk("\033[2A\n-- H.264 -- End decoding %d frames\033[K", frame_no - 1);
    return NULL;		// Stop the decoding process
  }
  else {

#if defined(CONFIG_ARCH_EMU)
    output_write(this);   // Write  the frame to the output file

    /*    if (frame_no == 1){
	  FILE * test_file = fopen("test_file.txt","w");
	  int32_t aa;
	  int32_t bb;
	  for (aa=0;aa<stream_height;aa++){
	  for (bb=0;bb<stream_width;bb++)
	  fprintf(test_file,"%d ",L_pixel(this,aa,bb));
	  fprintf(test_file,"\n");
	  }
	  fclose(test_file);
	  } */
#endif

    /*Save the decoded frame to the name ref (ie this frame becomes the reference frame
      for the next frame to be decoded)*/
    temp = this;
    this = ref;
    ref = temp;

    return temp;		// Return the decoded frame
  }
}

void h264_rewind()
{
  input_rewind();
  frame_no = 0;
}

void h264_close()
{
  int32_t slice_compteur;
  int32_t err;
  void *ret;

  printk("\n-- H.264 CLOSE -- Closing threads...\n");
  for (slice_compteur = 0; slice_compteur < pps.num_slice_groups; slice_compteur++)
    {
      if((err=pthread_join(thread[slice_compteur], &ret)))
	{
	  printk("-- H.264 ERROR -- 'pthread_join' error for thread %d\n", slice_compteur);
	  abort();
	}
    }

  // Free Semaphore for Ring_buffer & deblocking filter
  printk("-- H.264 CLOSE -- Free semaphores...\n");
  if(sem_destroy(&ring_sem)!=0)
    printk("-- H.264 ERROR -- Fatal error during destroying the 'ring_sem' semaphore\n");
  if(sem_destroy(&filter_sem)!=0)
    printk("-- H.264 ERROR -- Fatal error during destroying the 'filter_sem' semaphore\n");

  for (slice_compteur = 0; slice_compteur < pps.num_slice_groups; slice_compteur++) {
    free(nal_buf[slice_compteur]);
    free_mode_pred_info(mpi[slice_compteur]);
  }

  free(nal_pos);
  free(nal_bit);
  free(nal_buf);
  free(thread);
  free(mpi);
  code_table_init_done = free_code_tables(); // Reset the flag code_table to KO after freeing the tables

  // Free Memory : frames, threads structures...
  printk("-- H.264 CLOSE -- Free frames memory...\n");
  free_frame(this);
  free_frame(ref);

  printk("-- H.264 CLOSE -- Close files...\n");
  input_close();printk("aa\n");
#if defined(GENERATE_OUTPUT_FILE)
  output_close();
#endif
  printk("bb\n");
}
