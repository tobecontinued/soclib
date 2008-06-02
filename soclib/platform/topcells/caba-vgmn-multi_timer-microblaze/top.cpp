#include <iostream>
#include <cstdlib>

#include "mapping_table.h"
#include "microblaze.h"
#include "iss_wrapper.h"
#include "vci_xcache.h"
#include "vci_timer.h"
#include "vci_ram.h"
#include "vci_multi_tty.h"
#include "vci_vgmn.h"

int _main(int argc, char *argv[])
{
   using namespace sc_core;
   // Avoid repeating these everywhere
   using soclib::common::IntTab;
   using soclib::common::Segment;

   // Define our VCI parameters
   typedef soclib::caba::VciParams<4,1,32,1,1,1,8,1,1,1> vci_param;

   // Mapping table
   soclib::common::MappingTable maptab(32, IntTab(12), IntTab(3), 0x00F00000);
   maptab.add(Segment("tout" , 0x00000000, 0x0000FFFF, IntTab(0), true));
   maptab.add(Segment("tty"  , 0x00400000, 0x00000258, IntTab(1), false));
   maptab.add(Segment("timer", 0x00500000, 0x00000100, IntTab(2), false));

   // Signals
   sc_clock    signal_clk("signal_clk");
   sc_signal<bool> signal_resetn("signal_resetn");
   
   soclib::caba::ICacheSignals signal_mb_icache0("signal_mb_icache0");
   soclib::caba::DCacheSignals signal_mb_dcache0("signal_mb_dcache0");
   sc_signal<bool> signal_mb0_it("signal_mb0_it"); 
  
   soclib::caba::ICacheSignals   signal_mb_icache1("signal_mb_icache1");
   soclib::caba::DCacheSignals   signal_mb_dcache1("signal_mb_dcache1");
   sc_signal<bool> signal_mb1_it("signal_mb1_it"); 
  
   soclib::caba::ICacheSignals   signal_mb_icache2("signal_mb_icache2");
   soclib::caba::DCacheSignals   signal_mb_dcache2("signal_mb_dcache2");
   sc_signal<bool> signal_mb2_it("signal_mb2_it"); 
  
   soclib::caba::ICacheSignals signal_mb_icache3("signal_mb_icache3");
   soclib::caba::DCacheSignals signal_mb_dcache3("signal_mb_dcache3");
   sc_signal<bool> signal_mb3_it("signal_mb3_it"); 

   soclib::caba::VciSignals<vci_param> signal_vci_m0("signal_vci_m0");
   soclib::caba::VciSignals<vci_param> signal_vci_m1("signal_vci_m1");
   soclib::caba::VciSignals<vci_param> signal_vci_m2("signal_vci_m2");
   soclib::caba::VciSignals<vci_param> signal_vci_m3("signal_vci_m3");

   soclib::caba::VciSignals<vci_param> signal_vci_tty("signal_vci_tty");
   soclib::caba::VciSignals<vci_param> signal_vci_vcimultiram0("signal_vci_vcimultiram0");
   soclib::caba::VciSignals<vci_param> signal_vci_vcitimer("signal_vci_vcitimer");

   sc_signal<bool> signal_tty_irq0("signal_tty_irq0"); 
   sc_signal<bool> signal_tty_irq1("signal_tty_irq1"); 
   sc_signal<bool> signal_tty_irq2("signal_tty_irq2"); 
   sc_signal<bool> signal_tty_irq3("signal_tty_irq3"); 

   // Components

   soclib::caba::VciXCache<vci_param> cache0("cache0", maptab,IntTab(0),8,4,8,4);
   soclib::caba::VciXCache<vci_param> cache1("cache1", maptab,IntTab(1),8,4,8,4);
   soclib::caba::VciXCache<vci_param> cache2("cache2", maptab,IntTab(2),8,4,8,4);
   soclib::caba::VciXCache<vci_param> cache3("cache3", maptab,IntTab(3),8,4,8,4);

   soclib::caba::IssWrapper<soclib::common::MicroBlazeIss> mb0("mb0", 0);
   soclib::caba::IssWrapper<soclib::common::MicroBlazeIss> mb1("mb1", 1);
   soclib::caba::IssWrapper<soclib::common::MicroBlazeIss> mb2("mb2", 2);
   soclib::caba::IssWrapper<soclib::common::MicroBlazeIss> mb3("mb3", 3);

   soclib::common::ElfLoader loader("soft/bin.soft");
   soclib::caba::VciMultiRam<vci_param> vcimultiram0("vcimultiram0", IntTab(0), maptab, loader);
   soclib::caba::VciMultiTty<vci_param> vcitty("vcitty", IntTab(1), maptab, "vcitty0", "vcitty1", "vcitty2", "vcitty3", NULL);
   soclib::caba::VciTimer<vci_param> vcitimer("vcittimer", IntTab(2), maptab, 4);
   
   soclib::caba::VciVgmn<vci_param> vgmn("vgmn",maptab, 4, 3, 2, 8);

   // Net-List
 
   mb0.p_clk(signal_clk);  
   mb1.p_clk(signal_clk);  
   mb2.p_clk(signal_clk);  
   mb3.p_clk(signal_clk);  
   cache0.p_clk(signal_clk);
   cache1.p_clk(signal_clk);
   cache2.p_clk(signal_clk);
   cache3.p_clk(signal_clk);
   vcimultiram0.p_clk(signal_clk);
   vcitimer.p_clk(signal_clk);
  
   mb0.p_resetn(signal_resetn);  
   mb1.p_resetn(signal_resetn);  
   mb2.p_resetn(signal_resetn);  
   mb3.p_resetn(signal_resetn);  
   cache0.p_resetn(signal_resetn);
   cache1.p_resetn(signal_resetn);
   cache2.p_resetn(signal_resetn);
   cache3.p_resetn(signal_resetn);
   vcimultiram0.p_resetn(signal_resetn);
   vcitimer.p_resetn(signal_resetn);
  
   mb0.p_irq[0](signal_mb0_it); 
   mb0.p_icache(signal_mb_icache0);
   mb0.p_dcache(signal_mb_dcache0);
  
   mb1.p_irq[0](signal_mb1_it); 
   mb1.p_icache(signal_mb_icache1);
   mb1.p_dcache(signal_mb_dcache1);
  
   mb2.p_irq[0](signal_mb2_it); 
   mb2.p_icache(signal_mb_icache2);
   mb2.p_dcache(signal_mb_dcache2);
  
   mb3.p_irq[0](signal_mb3_it); 
   mb3.p_icache(signal_mb_icache3);
   mb3.p_dcache(signal_mb_dcache3);
        
   cache0.p_icache(signal_mb_icache0);
   cache0.p_dcache(signal_mb_dcache0);
   cache0.p_vci(signal_vci_m0);

   cache1.p_icache(signal_mb_icache1);
   cache1.p_dcache(signal_mb_dcache1);
   cache1.p_vci(signal_vci_m1);

   cache2.p_icache(signal_mb_icache2);
   cache2.p_dcache(signal_mb_dcache2);
   cache2.p_vci(signal_vci_m2);

   cache3.p_icache(signal_mb_icache3);
   cache3.p_dcache(signal_mb_dcache3);
   cache3.p_vci(signal_vci_m3);

   vcimultiram0.p_vci(signal_vci_vcimultiram0);

   vcitimer.p_vci(signal_vci_vcitimer);
   vcitimer.p_irq[0](signal_mb0_it); 
   vcitimer.p_irq[1](signal_mb1_it); 
   vcitimer.p_irq[2](signal_mb2_it); 
   vcitimer.p_irq[3](signal_mb3_it); 
  

   vcitty.p_clk(signal_clk);
   vcitty.p_resetn(signal_resetn);
   vcitty.p_vci(signal_vci_tty);
   vcitty.p_irq[0](signal_tty_irq0); 
   vcitty.p_irq[1](signal_tty_irq1); 
   vcitty.p_irq[2](signal_tty_irq2); 
   vcitty.p_irq[3](signal_tty_irq3); 

   vgmn.p_clk(signal_clk);
   vgmn.p_resetn(signal_resetn);

   vgmn.p_to_initiator[0](signal_vci_m0);
   vgmn.p_to_initiator[1](signal_vci_m1);
   vgmn.p_to_initiator[2](signal_vci_m2);
   vgmn.p_to_initiator[3](signal_vci_m3);

   vgmn.p_to_target[0](signal_vci_vcimultiram0);
   vgmn.p_to_target[1](signal_vci_tty);
   vgmn.p_to_target[2](signal_vci_vcitimer);

#ifdef MBTRACE
   sc_trace_file *tf = sc_create_vcd_trace_file("sc_dump");
 
   sc_trace(tf, signal_clk,    "clk");
   sc_trace(tf, signal_resetn, "rst_n");
   sc_trace(tf, signal_vci_m0.address, "vci_m0_addr");
   sc_trace(tf, signal_mb_icache0.req, "icache0_req");
   sc_trace(tf, signal_mb_icache0.adr, "icache0_addr");
#endif

   int ncycles = 1000000;
   sc_start(0);
   signal_resetn = false;

   sc_start(1);
   signal_resetn = true;

   for (int i = 0; i < ncycles ; i++) {
      sc_start(1);
     
      if( (i % 100000) == 0) 
         std::cout
            << "Time elapsed: "<<i<<" cycles." << std::endl;
   }
   std::cout << "Stopping after "<<ncycles<<" cycles." << std::endl;
#ifdef MBTRACE
   sc_close_vcd_trace_file(tf);
#endif
   return EXIT_SUCCESS;
}

int sc_main (int argc, char *argv[])
{
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
