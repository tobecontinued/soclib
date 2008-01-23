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

#ifndef SOCLIB_CABA_PVDC_ASSERTH
#define SOCLIB_CABA_PVDC_ASSERTH

#include "caba/util/base_module.h"
#include "caba/interface/vci_initiator.h"
#include "caba/interface/vci_target.h"

namespace soclib { namespace caba {

template <typename vci_param>
class PVciAssert : public soclib::caba::BaseModule {
  private:
   std::ostream* plog_file;
   bool fDefaultMode;

  public:
   VciSignals<vci_param>& observedSignals;
   sc_in<bool> p_clk;
   sc_in<bool> resetn;
   
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
               if (observedSignals.cmdval) {
                  acquireCell();
                  if (observedSignals.cmdack)
                     phssHandshakeState = PHSSAckVal;
                  else
                     phssHandshakeState = PHSSValTriggered;
               };
               // else // rule R2 p.16
               //   assume(!observedSignals.cmdack);
               break;
            case PHSSValTriggered:
               assume(observedSignals.cmdval);
               assume((observedSignals.address == addressPrevious) && (observedSignals.be == bePrevious) // rule R1 p.16
                  && (observedSignals.cmd == cmdPrevious) && (observedSignals.wdata == wdataPrevious)
                  && (observedSignals.eop == eopPrevious));
               if (observedSignals.cmdack)
                  phssHandshakeState = PHSSAckVal;
               break;
            case PHSSAckVal:
               if (!observedSignals.cmdval) {
                  if (!observedSignals.cmdack)
                     phssHandshakeState = PHSSNone;
                  else
                     phssHandshakeState = PHSSNone; // rule R2 p.16
               }
               else {
                  acquireCell();
                  if (!observedSignals.cmdack) // rule R3 p.16
                     // assume((observedSignals.rdata == rdataPrevious) && (observedSignals.rerror == rerrorPrevious));
                     phssHandshakeState = PHSSValTriggered;
               };
               break;
         };
      }

   int uReset;
   void setReset(int uResetSource = 8)
      {  if (!observedSignals.cmdack && !observedSignals.cmdval)
            uReset = 0;
         else
            uReset = uResetSource;
      }
   void testReset() // see p.11
      {  if (!observedSignals.cmdack && !observedSignals.cmdval)
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
            unsigned int uBE = observedSignals.be.read(); // see p.21
            if (uBE >= 4) {
               if (uBE >> 2 <= 2)
                  assume(uBE & 3 == 0);
               else // uBE >> 2 == 3
                  assume(uBE & 3 == 0 || uBE & 3 == 3);
            };
         };
         if (uCells == 0)
            aPreviousAddress = observedSignals.address;
         else { // see p.13
            aPreviousAddress += vci_param::B; // cell_size()
            assume(aPreviousAddress == observedSignals.address);
         };
         ++uCells;
         if (observedSignals.eop) {
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

  protected:
   SC_HAS_PROCESS(PVciAssert);

  public:
   PVciAssert(VciSignals<vci_param>& observedSignalsReference, sc_module_name insname)
      :  soclib::caba::BaseModule(insname), plog_file(NULL), fDefaultMode(true),
         observedSignals(observedSignalsReference), phssHandshakeState(PHSSNone), uReset(0),
         aPreviousAddress(0), uCells(0), valPrevious(0)
      {  SC_METHOD(reset);
         dont_initialize();
         sensitive << resetn.pos();

         SC_METHOD(filter);
         dont_initialize();
         sensitive << p_clk.pos();
      }
   void reset()
      {  setReset(); }
   void filter() 
      {  testHandshake();
         if (uReset > 0) testReset();

         valPrevious = observedSignals.cmdval;
         eopPrevious = observedSignals.eop;
         cmdPrevious = observedSignals.cmd;
         addressPrevious = observedSignals.address;
         bePrevious = observedSignals.be;
         wdataPrevious = observedSignals.wdata;
      }
   void setLogOut(std::ostream& osOut) { plog_file = &osOut; }
   void setDefaultMode() { fDefaultMode = true; }
   void setFreeMode() { fDefaultMode = false; }
};

}} // end of namespace soclib::caba

/*
#include <systemc.h>
#include <fstream>

#include "caba/util/base_module.h"
#include "caba/interface/vci_initiator.h"
#include "caba/interface/vci_target.h"
#include "caba/verification/pvci_filter.h"

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
  soclib::caba::PVciAssert<MyVciParams> vciAssert(vciSignals, "VciAssert_verif");
  vciAssert.p_clk(clk);
  vciAssert.setLogOut(log_file);
  
  vciFst.p_clk(clk);
  vciFst.p_vci(vciSignals);
  vciSnd.p_clk(clk);
  vciSnd.p_vci(vciSignals);

  sc_start(clk, -1);
  return 0;
}

*/

#endif // SOCLIB_CABA_PVDC_ASSERTH

