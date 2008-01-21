/*
 *
 * SOCLIB_GPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU GPLv2.
 * 
 * SoCLib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 * 
 * SOCLIB_GPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2006-2007
 *
 * Maintainers: nipo
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

#define clock_t sc_clock

#if defined(SYSTEMCASS) || defined(SYSTEMCASS_DIR) || defined(SOCVIEW)
#define RUN_CYCLES(c) sc_start(c)
#else
#define RUN_CYCLES(c) sc_start(sc_time(c, SC_NS))
#endif

#include "top.h"

class conf {
private:
    void usage( const char *progname )
    {
        std::cerr
			<< "Usage: "<<progname<<" [args]" <<std::endl
			<< "Args:"<<std::endl
			<< "\t-c --cycles n\tNumber of cycles to simulate"<<std::endl
			<< "\t-e --endless n\tEndless simulation"<<std::endl
			<< "\t-d --delta n\tPrint stats every n cycles"<<std::endl
			<< "\t-t --tracing\tGenerate a trace file"<<std::endl
			<< "\t-f --trace-file\tFile name for trace file (system_trace.vcd)"<<std::endl
			<< std::endl;
    }

public:
    int cycles;
    int delta;
    bool tracing;
    char *trace_filename;
	bool endless;

    conf( int argc, char **argv)
    {
        int c;

        cycles = 10000000;
        delta = 100000;
        tracing = false;
        trace_filename = "system_trace";
		endless = false;

        while (1) {
            static struct option options[] = {
                { "cycles", 1, 0, 'c' },
                { "delta", 1, 0, 'd' },
                { "tracing", 0, 0, 't' },
                { "trace-file", 1, 0, 'f' },
                { "help", 0, 0, 'h' },
                { "endless", 0, 0, 'e' },
                { 0, 0, 0, 0 }
            };
            c = getopt_long(argc, argv, "c:d:tf:eh", options, NULL);
            if (c==-1)
                break;

            switch (c) {
            case 'c':
                cycles = strtol(optarg, NULL, 0);
                break;
            case 'd':
                delta = strtol(optarg, NULL, 0);
                break;
            case 't':
                tracing = true;
                break;
            case 'f':
                trace_filename = optarg;
                break;
            case 'e':
				endless = true;
                break;
            default:
            case 'h':
                usage(argv[0]);
                exit(0);
            }
        }
    }
};


static sc_trace_file *trace_file;

#ifndef SOCVIEW
static float cur_use()
{
    struct rusage ru;

    getrusage( 0, &ru );
    return (float)ru.ru_utime.tv_sec+
		((float)ru.ru_utime.tv_usec)/1e6;
}

static void clean_exit(int arg)
{
    sc_stop();
}
#endif

void waitexit()
{
    sc_close_vcd_trace_file (trace_file);
}

int sc_main(int argc, char *argv[])
{
    conf *config = new conf(argc, argv);

    clock_t clock;
    sc_signal<bool> resetn;

#ifdef SYSTEMCASS_DIR
    /*
     * Here is a little helper if we use systemcass:
     * Defining environment if not defined
     * can help us having strange results
     * (DSX uses as little environment as possible)
     */
    setenv("SYSTEMCASS", SYSTEMCASS_DIR, 0);
#endif

    top::Top *top_inst = new top::Top(clock, resetn);

    if (config->tracing) {
        std::cout
			<< "Logging session to "
			<< config->trace_filename
			<< std::endl;
        trace_file = sc_create_vcd_trace_file (config->trace_filename);

        sc_trace(trace_file, clock, "clock");
        sc_trace(trace_file, resetn, "/reset");

        top_inst->do_traces(trace_file);
        atexit(waitexit);
    }

#ifndef SOCVIEW
    signal(SIGINT,clean_exit);
#endif

	RUN_CYCLES(0);
    resetn = false;
	RUN_CYCLES(1);
    resetn = true;

#ifdef SOCVIEW
    debug();
#else
    int i = 0;
    float usage_start, prev;
    double last_cycles = 0.;

    usage_start = prev = cur_use();
    while ( config->endless || i < config->cycles ) {
		int n = config->delta;
		float cu;
        double cycles = sc_time_stamp().to_double()/1000;

		if ( config->cycles < n && ! config->endless )
			n = config->cycles;
		RUN_CYCLES(n);
		i += n;
		cu = cur_use();
        if (cycles!=last_cycles)
            std::cout
				<< "Time elapsed: "<<i
				<< " cycles "<<(cycles-last_cycles)/(cu-prev)
				<<" c/s"<<std::endl;

        last_cycles = cycles;
		prev = cu;
    }

	std::cout
		<< "*** "<<(int)last_cycles
		<< " cycles simulated in "
		<< (float)(prev-usage_start)
		<<" seconds, average: "
		<< (int)last_cycles <<" c/s ***"
		<< std::endl;
//(float)last_cycles/(float)(prev-usage_start));
#endif

    if (config->tracing)
		sc_close_vcd_trace_file (trace_file);

    delete top_inst;

	exit(EXIT_SUCCESS);
}
