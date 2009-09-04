/*****************************************************************************

  main.c -- real main
  - Open the file
  - Launch the decoding process

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

/****************************************************************************
  Include section
 ****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "h264.h"


/****************************************************************************
  Global variables and structures
 ****************************************************************************/
static pthread_t main_thread;

//char *file_in_name = "FOREMAN.264";
//char *file_in_name = "FOREMAN4.264";
//char *file_in_name = "STEFAN2.264";
//char *file_in_name = "STEFAN4.264";
//char *file_in_name = "CONT4.264";
char *file_in_name = "FOREMAN.264";
//char *file_in_name = "README";
int8_t slice_numbers = 3;
char *file_out_name = "TEST.YUV";
/*
typedef struct __decoder_args{
	char * file_in_name;
	char * file_out_name;
	int8_t slice_numbers;
} decoder_args;
decoder_args prg_args;
*/

/****************************************************************************
  Sub Main functions
 ****************************************************************************/
void * main_process(void *arg);
void   do_bench(bool_t mode);


/****************************************************************************
  Main for both host and target implementations
 ****************************************************************************/
int32_t main(int32_t argc, char *argv[])
{
	pthread_create(&main_thread, NULL, main_process, NULL);

	return 0;
}


/****************************************************************************
  Where the program really starts
 ****************************************************************************/
void * main_process(void *arg)
{
//	cpu_interrupt_enable();

// reads the headers of the .264 sequence
	if (!h264_open(file_in_name,file_out_name,slice_numbers))
		printf("-- H.264 ERROR -- Error while opening H.264 stream\n");

	do_bench(1);
	h264_close();

	return 0;
}


/****************************************************************************
 * This function recursively calls decode_frame. It is used
 * to decode a whole sequence.
 * mode = 0 --> no perf evaluation
 * mode = 1 --> perf evaluation
 ****************************************************************************/
void do_bench(bool_t mode)
{
	int32_t frames = 0;
	void *frameptr = NULL;

#if (defined(CONFIG_DRIVER_TIMER_SOCLIB) || defined(CONFIG_DRIVER_TIMER_EMU))
//	printf("-- H.264 INIT -- Initalization finished in %d ms\n", get_timer_value() / 200000);
	printf("-- H.264 INIT -- Initalization finished in %d ms\n", get_timer_value());
#endif

	if (mode==0)
	{
		do {
//			frameptr = h264_decode_frame(1);
			frameptr = h264_decode_frame();
		}
		while(frameptr != NULL);
	}

	else
	{
		do {
//			frameptr = h264_decode_frame(1);
			frameptr = h264_decode_frame();
			frames++;
		}
		while(frameptr != NULL);
	}
}
