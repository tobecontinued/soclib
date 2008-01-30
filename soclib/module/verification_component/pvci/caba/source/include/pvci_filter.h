/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) CEA
 *
 * Authors: Franck Vedrine <franck.vedrine@cea.fr>, 2008
 */

#ifndef SOCLIB_CABA_PVDC_FILTERH
#define SOCLIB_CABA_PVDC_FILTERH

#include "caba_base_module.h"
#include "vci_initiator.h"
#include "vci_target.h"

namespace soclib { namespace caba {

template <typename vci_param>
class PVciFilter : public soclib::caba::BaseModule {
  private:
   std::ostream* plog_file;
   bool fDefaultMode;

  public:
   struct In {
      sc_in<typename vci_param::val_t>     val;
      sc_in<typename vci_param::eop_t>     eop;
      sc_in<typename vci_param::cmd_t>     cmd;
      sc_in<typename vci_param::addr_t>    address;
      sc_in<typename vci_param::be_t>      be;
      sc_in<typename vci_param::data_t>    wdata;

   	sc_out<typename vci_param::data_t>    rdata;
      sc_out<typename vci_param::ack_t>     cmdack;
   	sc_out<typename vci_param::rerror_t>  rerror;


#define __ren(x) x((name+"_in_" #x).c_str())
      In(const std::string &name)
         : __ren(val), __ren(eop), __ren(cmd), __ren(address), __ren(be), __ren(wdata),
#undef __ren
#define __ren(x) x((name+"_out_" #x).c_str())
           __ren(rdata), __ren(cmdack), __ren(rerror) {}
#undef __ren

      void operator()(VciSignals<vci_param> &sig)
         {  val     (sig.cmdval);
            eop     (sig.eop);
            cmd     (sig.cmd);
            address (sig.address);
            be      (sig.be);
            wdata   (sig.wdata);
            rdata   (sig.rdata);
            cmdack  (sig.cmdack);
            rerror  (sig.rerror);
         }

	   void operator()(VciInitiator<vci_param> &ports) // To see : create a VciSignals between the ports
	      {  val     (ports.cmdval);
            eop     (ports.eop);
            cmd     (ports.cmd);
            address (ports.address);
            be      (ports.be);
            wdata   (ports.wdata);
            rdata   (ports.rdata);
            cmdack  (ports.cmdack);
            rerror  (ports.rerror);
         }
   };

   struct Out {
   	sc_in<typename vci_param::data_t>    rdata;
      sc_in<typename vci_param::ack_t>     cmdack;
   	sc_in<typename vci_param::rerror_t>  rerror;

      sc_out<typename vci_param::val_t>    val;
      sc_out<typename vci_param::eop_t>    eop;
      sc_out<typename vci_param::cmd_t>    cmd;
      sc_out<typename vci_param::addr_t>   address;
      sc_out<typename vci_param::be_t>     be;
      sc_out<typename vci_param::data_t>   wdata;

      Out(const std::string &name)
#define __ren(x) x((name+"_in_" #x).c_str())
         : __ren(rdata), __ren(cmdack), __ren(rerror),
#undef __ren
#define __ren(x) x((name+"_out_" #x).c_str())
           __ren(val), __ren(eop), __ren(cmd), __ren(address), __ren(be), __ren(wdata) {}
#undef __ren

      void operator()(VciSignals<vci_param> &sig)
         {  rdata   (sig.rdata);
            cmdack  (sig.cmdack);
            rerror  (sig.rerror);
            
            val     (sig.cmdval);
            eop     (sig.eop);
            cmd     (sig.cmd);
            address (sig.address);
            be      (sig.be);
            wdata   (sig.wdata);
         }

      void operator()(VciTarget<vci_param> &ports)
         {  rdata   (ports.rdata);
            cmdack  (ports.cmdack);
            rerror  (ports.rerror);

            val     (ports.cmdval);
            eop     (ports.eop);
            cmd     (ports.cmd);
            address (ports.address);
            be      (ports.be);
            wdata   (ports.wdata);
         }
   };

   sc_in<bool> p_clk;
   sc_in<bool> resetn;
   In in;
   Out out;

  private:
   void assume(bool fCondition) const
      {  if (!fCondition)
            (plog_file ? *plog_file : (std::ostream&) std::cout) << "Protocol Error!!!\n";
      }
   enum PeripheralHandShakeState { PHSSNone, PHSSValTriggered, PHSSAckVal };
   PeripheralHandShakeState phssHandshakeState;
   void testHandshake() // verified on the waveforms of p.18-20
      {  switch (phssHandshakeState) {
            case PHSSNone:
               if (in.val) {
                  acquireCell();
                  if (out.cmdack)
                     phssHandshakeState = PHSSAckVal;
                  else
                     phssHandshakeState = PHSSValTriggered;
               };
               // else // rule R2 p.16
               //   assume(!out.cmdack);
               break;
            case PHSSValTriggered:
               assume(in.val);
               assume((in.address == addressPrevious) && (in.be == bePrevious) // rule R1 p.16
                  && (in.cmd == cmdPrevious) && (in.wdata == wdataPrevious)
                  && (in.eop == eopPrevious));
               if (out.cmdack)
                  phssHandshakeState = PHSSAckVal;
               break;
            case PHSSAckVal:
               if (!in.val) {
                  if (!out.cmdack)
                     phssHandshakeState = PHSSNone;
                  else
                     phssHandshakeState = PHSSNone; // rule R2 p.16
               }
               else {
                  acquireCell();
                  if (!out.cmdack) // rule R3 p.16
                     // assume((out.rdata == rdataPrevious) && (out.rerror == rerrorPrevious));
                     phssHandshakeState = PHSSValTriggered;
               };
               break;
         };
      }

   int uReset;
   void setReset(int uResetSource = 8)
      {  if (!out.cmdack && !in.val)
            uReset = 0;
         else
            uReset = uResetSource;
      }
   void testReset() // see p.11
      {  if (!out.cmdack && !in.val)
            uReset = 0;
         else {
            assume(uReset > 1);
            --uReset;
         };
      }

   typename vci_param::addr_t aPreviousAddress;
   int uCells;
   void acquireCell()
      {  if (fDefaultMode) {
            unsigned int uBE = in.be.read(); // see p.21
            if (uBE >= 4) {
               if (uBE >> 2 <= 2)
                  assume(uBE & 3 == 0);
               else // uBE >> 2 == 3
                  assume(uBE & 3 == 0 || uBE & 3 == 3);
            };
         };
         if (uCells == 0)
            aPreviousAddress = in.address;
         else { // see p.13
            aPreviousAddress += vci_param::B; // cell_size()
            assume(aPreviousAddress == in.address);
         };
         ++uCells;
         if (in.eop) {
            uCells = 0;
            aPreviousAddress = 0;
         };
      }
   typename vci_param::val_t      valPrevious;
   typename vci_param::eop_t      eopPrevious;
   typename vci_param::cmd_t      cmdPrevious; // = rd
   typename vci_param::addr_t     addressPrevious;
   typename vci_param::be_t       bePrevious;
   typename vci_param::data_t     wdataPrevious;
   
  	typename vci_param::data_t     rdataPrevious;
  	typename vci_param::rerror_t   rerrorPrevious;

  protected:
   SC_HAS_PROCESS(PVciFilter);

  public:
   PVciFilter(sc_module_name insname)
      :  soclib::caba::BaseModule(insname), plog_file(NULL), fDefaultMode(true),
         in((const char*) insname), out((const char*) insname),
         phssHandshakeState(PHSSNone), uReset(0), aPreviousAddress(0), uCells(0),
         valPrevious(0)
      {  SC_METHOD(transition);
         dont_initialize();
         sensitive << in.val << in.eop << in.cmd << in.address
            << in.be << in.wdata << out.rdata << out.cmdack << out.rerror;
         SC_METHOD(reset);
         dont_initialize();
         sensitive << resetn.pos();
         
         SC_METHOD(filter);
         dont_initialize();
         sensitive << p_clk.pos();
      }
   void reset()
      {  setReset(); }
   void transition()
      {  out.val = in.val;
         out.eop = in.eop;
         out.cmd = in.cmd;
         out.address = in.address;
         out.be = in.be;
         out.wdata = in.wdata;
         in.rdata = out.rdata;
         in.cmdack = out.cmdack;
         in.rerror = out.rerror;
      }
   void filter() 
      {  testHandshake();
         if (uReset > 0) testReset();

         valPrevious = in.val;
         eopPrevious = in.eop;
         cmdPrevious = in.cmd;
         addressPrevious = in.address;
         bePrevious = in.be;
         wdataPrevious = in.wdata;

         rdataPrevious = out.rdata;
         rerrorPrevious = out.rerror;
      }
   void setLogOut(std::ostream& osOut) { plog_file = &osOut; }
   void setDefaultMode() { fDefaultMode = true; }
   void setFreeMode() { fDefaultMode = false; }
};

}} // end of namespace soclib::caba

/*
#include <systemc.h>
#include <fstream>

#include "../include/pvci_filter.h"

// A component with a vci iniator
template<typename vci_param>
class VciFstComponent : public soclib::caba::BaseModule {
  public:
   sc_in<bool> p_clk;
   soclib::caba::VciInitiator<vci_param>   p_vci;
   VciFstComponent(sc_module_name insname);
};

// A component with a vci target
template<typename vci_param>
class VciSndComponent : public soclib::caba::BaseModule {
  public:
   sc_in<bool> p_clk;
   soclib::caba::VciTarget<vci_param>   p_vci;
   VciSndComponent(sc_module_name insname);
};

// Specific size for vci-protocol
typedef soclib::caba::VciParams<4,1,32,1,1,1,8,1,1,1> MyVciParams;

int sc_main(int ac, char *av[]) {
  VciFstComponent<MyVciParams> vciFst("VciFstComponent");
  VciSndComponent<MyVciParams> vciSnd("VciSndComponent");
  sc_clock clk("Clock", 1, 0.5, 0.0);
  soclib::caba::VciSignals<MyVciParams> vciSignals("VciSignals");

  std::ofstream log_file("verif.log");
  soclib::caba::VciSignals<MyVciParams> vciSignalsVerif("VciSignals_verif");
  soclib::caba::PVciFilter<MyVciParams> vciFilter("VciFilter_verif");
  vciFilter.p_clk(clk);
  vciFilter.setLogOut(log_file);
  // vciFilter.activateFilter();
  vciFilter.in(vciSignals);
  vciFilter.out(vciSignalsVerif);
  
  vciFst.p_clk(clk);
  vciFst.p_vci(vciSignals);
  vciSnd.p_clk(clk);
  vciSnd.p_vci(vciSignalsVerif);

  sc_start(clk, -1);
  return 0;
}

*/

#endif // SOCLIB_CABA_PVDC_FILTERH

