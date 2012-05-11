///////////////////////////////////////////////////////////////////////////////////
// File     : top.cpp for the 'caba_vgsb_almo_mmu' generic architecture
// Date     : 01/04/2012
// Author   : alain greiner
// Copyright (c) UPMC-LIP6
///////////////////////////////////////////////////////////////////////////////////
// Implementation note:
// This architecture supports multi-tasking, multi-processing, and virtual memory.
// It has been designed to run the GIET V2.0. 
// There is two separated RAM for user and kernel, as we use the SoCLib pre-loader.
// The processor is a MIPS32 supporting the SoCLib generic MMU.
// The interconnect is the SoCLib VGSB : flat 32 bits address space.
// An IO controler and a Frame buffer can be optionally activated.
// It uses the NB_PROCS and NB_TTYS parameters, defined in the giet_config.h file.
// - The number of processors cannot be larger than 8, because each processor
//   has a private timer and a private DMA, and the number of IRQs must be < 32.
// - The number of TTYs cannot be larger than 15 for the same reason.
///////////////////////////////////////////////////////////////////////////////////

#include <systemc>
#include <limits>
#include <cstdlib>

#include "soft/giet_config.h"

#include "vci_signals.h"
#include "vci_param.h"
#include "mapping_table.h"
#include "gdbserver.h"

#include "mips32.h"
#include "vci_vgsb.h"
#include "vci_vcache_wrapper.h"
#include "vci_multi_tty.h"
#include "vci_timer.h"
#include "vci_multi_icu.h"
#include "vci_multi_dma.h"
#include "vci_block_device.h"
#include "vci_framebuffer.h"
#include "vci_simple_ram.h"
#include "alloc_elems.h"
#include "vloader.h"

#define PSEG_ROM_BASE    0xBFC00000
#define PSEG_ROM_SIZE    0x00010000    // ROM de boot: 32 Kbytes

#define PSEG_RAU_BASE    0x00000000    
#define PSEG_RAU_SIZE    0x01000000    // RAM user: 16 Mbytes

#define PSEG_RAK_BASE    0x80000000    
#define PSEG_RAK_SIZE    0x00100000    // RAM kernel: 1 Mbytes

#define PSEG_TTY_BASE    0x90000000
#define PSEG_TTY_SIZE    (NB_TTYS) * 16 

#define PSEG_TIM_BASE    0x91000000
#define PSEG_TIM_SIZE    NB_PROCS * 16 

#define PSEG_IOC_BASE    0x92000000
#define PSEG_IOC_SIZE    0x00000020

#define PSEG_DMA_BASE    0x93000000
#define PSEG_DMA_SIZE    NB_PROCS * 32

#define PSEG_FBF_BASE    0x96000000
#define PSEG_FBF_SIZE    128 * 128  

#define PSEG_ICU_BASE    0x9F000000
#define PSEG_ICU_SIZE    NB_PROCS * 32 

#define ROM_TGTID   0
#define RAU_TGTID   1
#define RAK_TGTID   2
#define TIM_TGTID   3
#define FBF_TGTID   4
#define IOC_TGTID   5
#define DMA_TGTID   6
#define TTY_TGTID   7
#define ICU_TGTID   8

#define DMA_SRCID   NB_PROCS
#define IOC_SRCID   NB_PROCS + 1

int _main(int argc, char *argv[])
{
    using namespace sc_core;
    using namespace soclib::caba;
    using namespace soclib::common;

    // VCI fields width definition
    //  cell_size   = 4;
    //  plen_size   = 8;
    //  addr_size   = 32;
    //  rerror_size = 1;
    //  clen_size   = 1;
    //  rflag_size  = 1;
    //  srcid_size  = 12;
    //  pktid_size  = 1;
    //  trdid_size  = 4;
    //  wrplen_size = 1;

    typedef VciParams<4,8,32,1,1,1,12,1,4,1> vci_param;

    char    sys_name[256]     = "soft/sys.bin";
    char    hello_name[256]   = "soft/hello.bin";
    char    router_name[256]   = "soft/router.bin";
    char    pgcd_name[256]   = "soft/pgcd.bin";
    char    fifo_name[256]    = "soft/fifo.bin";

    char    map_name[256]     = "soft/map.bin@0xBFC0C000:D";

    size_t  n_cycles        = 1000000000;       // simulated cycles
    size_t  tlb_ways        = 8;                // Itlb & Dtlb parameters
    size_t  tlb_sets        = 8;
    bool    icached         = true;             // Icache activated
    size_t  icache_sets     = 256;              // Icache parameters
    size_t  icache_words    = 4;
    size_t  icache_ways     = 4;
    bool    dcached         = true;             // Dcache activated
    size_t  dcache_sets     = 256;              // Dcache parameters
    size_t  dcache_words    = 4;
    size_t  dcache_ways     = 4;
    size_t  wbuf_nlines     = 4;                // Write Buffer parameters
    size_t  wbuf_nwords     = 4;
    size_t  ram_latency     = 0;                // Ram latency (L2 MISS emulation)
    bool    ioc_ok          = false;            // IOC activated
    char    ioc_filename[256];                  // pathname for the ioc file
    bool    fbf_ok          = false;            // FBF acctivated
    size_t  fbf_size        = 128;              // number of lines = number of pixels
    bool    debug_ok        = false;            // debug activated
    size_t  from_cycle      = 0;                // debug start cycle
    bool    trace_ok        = false;            // cache trace activated
    char    trace_filename[256];
    FILE*   trace_file      = NULL;
    bool    stats_ok        = false;            // statistics activated
    char    stats_filename[256];
    FILE*   stats_file      = NULL;

    std::cout << std::endl << "********************************" << std::endl;
    std::cout << std::endl << "****** almo_generic_mmu   ******" << std::endl;
    std::cout << std::endl << "********************************" << std::endl;


    if (argc > 1)
    {
        for( int n=1 ; n<argc ; n=n+2 )
        {
            if( (strcmp(argv[n],"-NCYCLES") == 0) && (n+1<argc) )
            {
                n_cycles = atoi(argv[n+1]);
            }
            else if( (strcmp(argv[n],"-NICACHE") == 0) && (n+1<argc) )
            {
                icache_sets = atoi(argv[n+1]);
                if(icache_sets == 0)
                {
                    icached = false;
                    icache_sets = 1;
                }
            }
            else if( (strcmp(argv[n],"-NDCACHE") == 0) && (n+1<argc) )
            {
                dcache_sets = atoi(argv[n+1]);
                if(dcache_sets == 0)
                {
                    dcached = false;
                    dcache_sets = 1;
                }
            }
            else if( (strcmp(argv[n],"-TRACE") == 0) && (n+1<argc) )
            {
                trace_ok = true;
                if(n_cycles > 10000) n_cycles = 10000;
                strcpy(trace_filename, argv[n+1]);
                trace_file = fopen(trace_filename,"w+");
            }
            else if( (strcmp(argv[n],"-STATS") == 0) && (n+1<argc) )
            {
                stats_ok = true;
                strcpy(stats_filename, argv[n+1]);
                stats_file = fopen(stats_filename,"w+");
            }
            else if( (strcmp(argv[n],"-DEBUG") == 0) && (n+1<argc) )
            {
                debug_ok = true;
                from_cycle = atoi(argv[n+1]);
            }
            else if( (strcmp(argv[n],"-IOCFILE") == 0) && (n+1<argc) )
            {
                ioc_ok = true;
                strcpy(ioc_filename, argv[n+1]) ;
            }
            else if( (strcmp(argv[n],"-FBFSIZE") == 0) && (n+1<argc) )
            {
                fbf_ok = true;
                fbf_size = atoi(argv[n+1]) ;
            }
            else if( (strcmp(argv[n],"-LATENCY") == 0) && (n+1<argc) )
            {
                ram_latency = atoi(argv[n+1]);
            }
            else if( (strcmp(argv[n],"-MAP") == 0) && (n+1<argc) )
            {
                strcpy(map_name, argv[n+1]);
            }
            else
            {
                std::cout << "   Arguments on the command line are (key,value)" << std::endl;
                std::cout << "   The order is not important." << std::endl;
                std::cout << "   Accepted arguments are :" << std::endl << std::endl;
                std::cout << "   -NCYCLES number_of_simulated_cycles" << std::endl;
                std::cout << "   -NICACHE number_of_sets_for_instruction_cache" << std::endl;
                std::cout << "   -NDCACHE number_of_sets_for_data_cache" << std::endl;
                std::cout << "   -IOCFILE file_name" << std::endl;
                std::cout << "   -FBFSIZE number_of_pixels" << std::endl;
                std::cout << "   -TRACE file_name" << std::endl;
                std::cout << "   -STATS file_name" << std::endl;
                std::cout << "   -DEBUG debug_start_cycle" << std::endl;
                std::cout << "   -LATENCY number_of_cycles" << std::endl;
                exit(0);
            }
        }
    }
    std::cout << std::endl;
    std::cout << "    n_cycles     = " << n_cycles << std::endl;
    std::cout << "    icache_sets  = " << icache_sets << std::endl;
    std::cout << "    icache_words = " << icache_words << std::endl;
    std::cout << "    icache_ways  = " << icache_ways << std::endl;
    std::cout << "    dcache_sets  = " << dcache_sets << std::endl;
    std::cout << "    dcache_words = " << dcache_words << std::endl;
    std::cout << "    dcache_ways  = " << dcache_ways << std::endl;
    std::cout << "    ram_latency  = " << ram_latency << std::endl;
    std::cout << "    map_name  = " << map_name << std::endl;
    if(trace_ok) std::cout << "    trace_file   = " << trace_filename << std::endl;
    if(stats_ok) std::cout << "    stats_file   = " << stats_filename << std::endl;
    if(ioc_ok)   std::cout << "    ioc_file     = " << ioc_filename << std::endl;
    if(fbf_ok)   std::cout << "    frame buffer = " << fbf_size << " * " << fbf_size << std::endl;

    /* parameters checking : limitations are related to ICU inputs */
    if ( NB_PROCS > 8 )
    {
        std::cout << std::endl;
        std::cout << "The number of processors cannot be larger than 8" << std::endl;
        exit(0);
    }
    if ( NB_TTYS > 15 )
    {
        std::cout << std::endl;
        std::cout << "The number of TTYs cannot be larger than 8" << std::endl;
        exit(0);
    }

    //////////////////////////////////////////////////////////////////////////
    // Mapping Table
    //////////////////////////////////////////////////////////////////////////
    MappingTable maptab(32, IntTab(12), IntTab(12), 0xFFF00000);

    maptab.add(Segment("seg_rom", PSEG_ROM_BASE, PSEG_ROM_SIZE, IntTab(ROM_TGTID) , true));
    maptab.add(Segment("seg_rau", PSEG_RAU_BASE, PSEG_RAU_SIZE, IntTab(RAU_TGTID) , true));
    maptab.add(Segment("seg_rak", PSEG_RAK_BASE, PSEG_RAK_SIZE, IntTab(RAK_TGTID) , true));
    maptab.add(Segment("seg_tim", PSEG_TIM_BASE, PSEG_TIM_SIZE, IntTab(TIM_TGTID) , false));
    maptab.add(Segment("seg_dma", PSEG_DMA_BASE, PSEG_DMA_SIZE, IntTab(DMA_TGTID) , false));
    maptab.add(Segment("seg_fbf", PSEG_FBF_BASE, PSEG_FBF_SIZE, IntTab(FBF_TGTID) , false));
    maptab.add(Segment("seg_ioc", PSEG_IOC_BASE, PSEG_IOC_SIZE, IntTab(IOC_TGTID) , false));
    maptab.add(Segment("seg_tty", PSEG_TTY_BASE, PSEG_TTY_SIZE, IntTab(TTY_TGTID) , false));
    maptab.add(Segment("seg_icu", PSEG_ICU_BASE, PSEG_ICU_SIZE, IntTab(ICU_TGTID) , false));

    std::cout << std::endl << maptab << std::endl;

    //////////////////////////////////////////////////////////////////////////
    // Signals
    //////////////////////////////////////////////////////////////////////////
    sc_clock        signal_clk("signal_clk", sc_time( 1, SC_NS ), 0.5 );
    sc_signal<bool> signal_resetn("signal_resetn");

    VciSignals<vci_param> *signal_vci_init_proc =
        alloc_elems<VciSignals<vci_param> >("signal_vci_init_proc", NB_PROCS);

    VciSignals<vci_param> signal_vci_init_dma("signal_vci_init_dma");
    VciSignals<vci_param> signal_vci_init_ioc("signal_vci_init_ioc");

    VciSignals<vci_param> signal_vci_tgt_rom("signal_vci_tgt_rom");
    VciSignals<vci_param> signal_vci_tgt_rau("signal_vci_tgt_rau");
    VciSignals<vci_param> signal_vci_tgt_rak("signal_vci_tgt_rak");
    VciSignals<vci_param> signal_vci_tgt_tim("signal_vci_tgt_tim");
    VciSignals<vci_param> signal_vci_tgt_fbf("signal_vci_tgt_fbf");
    VciSignals<vci_param> signal_vci_tgt_ioc("signal_vci_tgt_ioc");
    VciSignals<vci_param> signal_vci_tgt_dma("signal_vci_tgt_dma");
    VciSignals<vci_param> signal_vci_tgt_icu("signal_vci_tgt_icu");
    VciSignals<vci_param> signal_vci_tgt_tty("signal_vci_tgt_tty");

    sc_signal<bool> signal_false("signal_false");

    sc_signal<bool> *signal_irq_proc =
        alloc_elems<sc_signal<bool> >("signal_irq_proc", NB_PROCS);

    sc_signal<bool> *signal_irq_tim =
        alloc_elems<sc_signal<bool> >("signal_irq_tim", NB_PROCS);

    sc_signal<bool> *signal_irq_dma =
        alloc_elems<sc_signal<bool> >("signal_irq_dma", NB_PROCS);

    sc_signal<bool> *signal_irq_tty =
        alloc_elems<sc_signal<bool> >("signal_irq_tty", NB_TTYS);

    sc_signal<bool> signal_irq_ioc("signal_irq_ioc");

    //////////////////////////////////////////////////////////////////////////
    // VCI Components : (NB_PROCS+2) initiators / (9) targets
    // The IOC & DMA components are both initiator & target.
    // The IOC and FBF components are optionnal.
    //////////////////////////////////////////////////////////////////////////
    // - srcid proc : proc_id
    // - srcid dma  : NB_PROCS   : dma
    // - srcid ioc  : NB_PROCS+1 : ioc
    //////////////////////////////////////////////////////////////////////////
    // The ICU controls at most 32 input IRQs:
    // - IRQ[0]  : ioc   (processor 0)
    // - IRQ[1]  : tty0  (processor 0)
    // - IRQ[2]  : tty1  (processor 0)
    // - IRQ[3]  : tty2  (processor 0)
    // - IRQ[4]  : tty3  (processor 0)
    // - IRQ[5]  : tty4  (processor 0)
    // - IRQ[6]  : tty5  (processor 0)
    // - IRQ[7]  : tty6  (processor 0)
    // - IRQ[8]  : tty7  (processor 0)
    // - IRQ[9]  : tty8  (processor 0)
    //
    // - unused  : IRQ[10] to IRQ]15]
    //
    // - IRQ[16] : tim0  (processor 0)
    // - IRQ[17] : dma0  (processor 0)
    // - IRQ[18] : tim1  (processor 1)
    // - IRQ[19] : dma1  (processor 1)
    // - IRQ[20] : tim2  (processor 2)
    // - IRQ[21] : dma2  (processor 2)
    // - IRQ[22] : tim3  (processor 3)
    // - IRQ[23] : dma3  (processor 3)
    // - IRQ[24] : tim4  (processor 4)
    // - IRQ[25] : dma4  (processor 4)
    // - IRQ[26] : tim5  (processor 5)
    // - IRQ[27] : dma5  (processor 5)
    // - IRQ[28] : tim6  (processor 6)
    // - IRQ[29] : dma6  (processor 6)
    // - IRQ[30] : tim7  (processor 7)
    // - IRQ[31] : dma7  (processor 7)
    ////////////////////////////////////////////////////////////////////////////

    VLoader vloader(map_name);

    // GdbServer<Mips32ElIss>::set_loader(&loader);

    VciVcacheWrapper<vci_param, GdbServer<Mips32ElIss> >* proc[NB_PROCS];
    for( size_t p = 0 ; p < NB_PROCS ; p++ )
    {
        std::ostringstream proc_name;
        proc_name << "proc_" << p;
        proc[p] = new VciVcacheWrapper< vci_param, GdbServer<Mips32ElIss> >(
                proc_name.str().c_str(),
                p,
                maptab,
                IntTab(p),
                tlb_ways, tlb_sets,
                icache_ways, icache_sets, icache_words,
                dcache_ways, dcache_sets, dcache_words,
                wbuf_nlines, wbuf_nwords,
                1000,           // max frozen cycles
                from_cycle,     // debug_start_cycle
                debug_ok );     //  detailed debug activation
    }

std::cout << std::endl << "proc(s) constructed" << std::endl;

    VciSimpleRam<vci_param>* rau;
    rau = new VciSimpleRam<vci_param>("rau",
            IntTab(RAU_TGTID),
            maptab,
            vloader,
            ram_latency);

std::cout << "rau constructed" << std::endl;

    VciSimpleRam<vci_param>* rak;
    rak = new VciSimpleRam<vci_param>("rak",
            IntTab(RAK_TGTID),
            maptab,
            vloader,
            ram_latency);

std::cout << "rak constructed" << std::endl;

    VciSimpleRam<vci_param>* rom;
    rom = new VciSimpleRam<vci_param>("rom",
            IntTab(ROM_TGTID),
            maptab,
            vloader);

std::cout << "rom constructed" << std::endl;

    std::vector<std::string> vect_names;
    for( size_t p = 0 ; p < (NB_TTYS) ; p++ )
    {
        std::ostringstream term_name;
        term_name <<  "term" << p;
        vect_names.push_back(term_name.str().c_str());
    }
    VciMultiTty<vci_param> *tty;
    tty = new VciMultiTty<vci_param>("tty",
            IntTab(TTY_TGTID),
            maptab,
            vect_names);

std::cout << "tty constructed" << std::endl;

    VciMultiIcu<vci_param> *icu;
    icu = new VciMultiIcu<vci_param>("icu",
            IntTab(ICU_TGTID),
            maptab,
            32,
            NB_PROCS);

std::cout << "icu constructed" << std::endl;

    VciTimer<vci_param>* timer;
    timer = new VciTimer<vci_param>("timer",
            IntTab(TIM_TGTID),
            maptab,
            NB_PROCS);

std::cout << "timer constructed" << std::endl;

    VciMultiDma<vci_param>* dma;
    dma = new VciMultiDma<vci_param>("dma",
            maptab,
            IntTab(DMA_SRCID),
            IntTab(DMA_TGTID),
            128,
            NB_PROCS);

std::cout << "dma constructed" << std::endl;

    VciFrameBuffer<vci_param>* fbf;
    if( fbf_ok ) fbf = new VciFrameBuffer<vci_param>("fbf",
            IntTab(FBF_TGTID),
            maptab,
            fbf_size, fbf_size);
    else fbf = NULL;

    VciBlockDevice<vci_param>* ioc;
    if( ioc_ok ) ioc = new VciBlockDevice<vci_param>("ioc",
            maptab,
            IntTab(IOC_SRCID),
            IntTab(IOC_TGTID),
            ioc_filename,
            512,
            200000);
    else ioc = NULL;

    VciVgsb<vci_param>* bus;
    bus = new VciVgsb<vci_param>("bus",
            maptab,
            NB_PROCS+2,
            9);

std::cout << "bus constructed" << std::endl << std::endl;

    //////////////////////////////////////////////////////////////////////////
    // Net-List
    //////////////////////////////////////////////////////////////////////////
    for ( size_t p = 0 ; p < NB_PROCS ; p++)
    {
        proc[p]->p_clk      (signal_clk);
        proc[p]->p_resetn   (signal_resetn);
        proc[p]->p_vci      (signal_vci_init_proc[p]);
        proc[p]->p_irq[0]   (signal_irq_proc[p]);
        proc[p]->p_irq[1]   (signal_false);
        proc[p]->p_irq[2]   (signal_false);
        proc[p]->p_irq[3]   (signal_false);
        proc[p]->p_irq[4]   (signal_false);
        proc[p]->p_irq[5]   (signal_false);
    }

std::cout << "processors connected" << std::endl;

    rau->p_clk      (signal_clk);
    rau->p_resetn   (signal_resetn);
    rau->p_vci      (signal_vci_tgt_rau);

std::cout << "rau connected" << std::endl;

    rak->p_clk      (signal_clk);
    rak->p_resetn   (signal_resetn);
    rak->p_vci      (signal_vci_tgt_rak);

std::cout << "rak connected" << std::endl;

    rom->p_clk      (signal_clk);
    rom->p_resetn   (signal_resetn);
    rom->p_vci      (signal_vci_tgt_rom);

std::cout << "rom connected" << std::endl;

    tty->p_clk      (signal_clk);
    tty->p_resetn   (signal_resetn);
    tty->p_vci      (signal_vci_tgt_tty);
    for (size_t t = 0 ; t < NB_TTYS ; t++)
        tty->p_irq[t] (signal_irq_tty[t]);

std::cout << "tty connected" << std::endl;

    icu->p_clk      (signal_clk);
    icu->p_resetn   (signal_resetn);
    icu->p_vci      (signal_vci_tgt_icu);

    for (size_t p = 0 ; p < NB_PROCS ; p++)
        icu->p_irq_out[p]       (signal_irq_proc[p]);

    for (size_t t = 0 ; t < (NB_TTYS) ; t++)
        icu->p_irq_in[t+1]      (signal_irq_tty[t]);

    for (size_t t = (NB_TTYS) ; t < 15 ; t++)
        icu->p_irq_in[t+1]      (signal_false);

    for (size_t p = 0 ; p < NB_PROCS ; p++)
    {
        icu->p_irq_in[16+2*p]   (signal_irq_tim[p]);
        icu->p_irq_in[17+2*p]   (signal_irq_dma[p]);
    }
    for (size_t p = NB_PROCS ; p < 8 ; p++)
    {
        icu->p_irq_in[16+2*p]   (signal_false);
        icu->p_irq_in[17+2*p]   (signal_false);
    }
    if( ioc_ok ) icu->p_irq_in[0] (signal_irq_ioc);
    else         icu->p_irq_in[0] (signal_false);

std::cout << "icu connected" << std::endl;

    timer->p_clk    (signal_clk);
    timer->p_resetn (signal_resetn);
    timer->p_vci    (signal_vci_tgt_tim);
    for (size_t p = 0 ; p < NB_PROCS ; p++)
        timer->p_irq[p] (signal_irq_tim[p]);

std::cout << "timer connected" << std::endl;

    dma->p_clk          (signal_clk);
    dma->p_resetn       (signal_resetn);
    dma->p_vci_initiator(signal_vci_init_dma);
    dma->p_vci_target   (signal_vci_tgt_dma);
    for (size_t p = 0 ; p < NB_PROCS ; p++)
        dma->p_irq[p] (signal_irq_dma[p]);

std::cout << "dma connected" << std::endl;

    if( fbf_ok )
    {
        fbf->p_clk      (signal_clk);
        fbf->p_resetn   (signal_resetn);
        fbf->p_vci      (signal_vci_tgt_fbf);
    }

    if( ioc_ok )
    {
        ioc->p_clk          (signal_clk);
        ioc->p_resetn       (signal_resetn);
        ioc->p_vci_initiator(signal_vci_init_ioc);
        ioc->p_vci_target   (signal_vci_tgt_ioc);
        ioc->p_irq          (signal_irq_ioc);
    }

    bus->p_clk      (signal_clk);
    bus->p_resetn   (signal_resetn);
    for ( size_t p = 0 ; p < NB_PROCS ; p++)
    {
        bus->p_to_initiator[p]  (signal_vci_init_proc[p]);
    }
    bus->p_to_initiator[DMA_SRCID]  (signal_vci_init_dma);
    bus->p_to_initiator[IOC_SRCID]  (signal_vci_init_ioc);
    bus->p_to_target[ROM_TGTID]     (signal_vci_tgt_rom);
    bus->p_to_target[RAU_TGTID]     (signal_vci_tgt_rau);
    bus->p_to_target[RAK_TGTID]     (signal_vci_tgt_rak);
    bus->p_to_target[TIM_TGTID]     (signal_vci_tgt_tim);
    bus->p_to_target[DMA_TGTID]     (signal_vci_tgt_dma);
    bus->p_to_target[FBF_TGTID]     (signal_vci_tgt_fbf);
    bus->p_to_target[IOC_TGTID]     (signal_vci_tgt_ioc);
    bus->p_to_target[TTY_TGTID]     (signal_vci_tgt_tty);
    bus->p_to_target[ICU_TGTID]     (signal_vci_tgt_icu);

std::cout << "bus connected" << std::endl;

    //////////////////////////////////////////////////////////////////////////
    // simulation
    //////////////////////////////////////////////////////////////////////////

    if( !ioc_ok ) signal_vci_init_ioc.cmdval = false;
    if( !ioc_ok ) signal_vci_tgt_ioc.rspval = false;
    if( !fbf_ok ) signal_vci_tgt_fbf.rspval = false;

    signal_resetn = false;
    sc_start( sc_time( 1, SC_NS ) ) ;

    signal_resetn = true;
    for ( size_t n=1 ; n<n_cycles ; n++ )
    {
        sc_start( sc_time( 1 , SC_NS ) ) ;
/*
        if( stats_ok && (n%10 == 0) )
            proc[0]->file_stats(stats_file);

        if( trace_ok )
            proc[0]->file_trace(trace_file);
*/
        if( debug_ok && (n > from_cycle) )
        {
            std::cout << "***************** cycle " << std::dec << n
                << " ***********************" << std::endl;

            proc[0]->print_trace();
            signal_vci_init_proc[0].print_trace("signal_proc_0");
            proc[1]->print_trace();
            signal_vci_init_proc[1].print_trace("signal_proc_1");
#if 0
            proc[2]->print_trace();
            signal_vci_init_proc[2].print_trace("signal_proc_2");
            proc[3]->print_trace();
            signal_vci_init_proc[3].print_trace("signal_proc_3");
#endif

            bus->print_trace();
            rom->print_trace();
            signal_vci_tgt_rom.print_trace("signal_rom");
            rau->print_trace();
            signal_vci_tgt_rau.print_trace("signal_rau");
            rak->print_trace();
            signal_vci_tgt_rak.print_trace("signal_rak");
            if ( signal_irq_tim[0].read() ) 
            {
                std::cout << "!!! IRQ_TIMER [0] ACTIVATED !!!" << std::endl; 
                std::cout << "    PROC_IRQ [0] = " << signal_irq_proc[0] << std::endl;
            }
            if ( signal_irq_tim[1].read() ) 
            {
                std::cout << "!!! IRQ_TIMER [1] ACTIVATED !!!" << std::endl; 
                std::cout << "    PROC_IRQ [1] = " << signal_irq_proc[1] << std::endl;
            }
            if ( signal_irq_tim[2].read() ) 
            {
                std::cout << "!!! IRQ_TIMER [2] ACTIVATED !!!" << std::endl; 
                std::cout << "    PROC_IRQ [2] = " << signal_irq_proc[2] << std::endl;
            }
        }
    }

    /* forcing xterm to quit too */
    kill(0, SIGINT);

    return 0;
}

void quit(int)
{
    sc_core::sc_stop();
}

int sc_main (int argc, char *argv[])
{
    signal(SIGINT, quit);
    signal(SIGPIPE, quit);

    try {
        return _main(argc, argv);
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception occured" << std::endl;
        throw;
    }
    return 1;
}

// Local Variables:
// tab-width: 4;
// c-basic-offset: 4;
// c-file-offsets:((innamespace . 0)(inline-open . 0));
// indent-tabs-mode: nil;
// End:
//
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

