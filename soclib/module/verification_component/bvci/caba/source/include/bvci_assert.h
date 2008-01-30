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
 * SoCLib is distributed observedSignals the hope that it will be useful, but
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

#ifndef SOCLIB_CABA_PVDC_BASIC_ASSERTH
#define SOCLIB_CABA_PVDC_BASIC_ASSERTH

#include "caba_base_module.h"
#include "vci_initiator.h"
#include "vci_target.h"

#include <list>
#include <set>

namespace soclib { namespace caba {

template <typename vci_param>
class BasicVciAssert : public soclib::caba::BaseModule {
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

   enum State { SIdle, SValid, SDefault_Ack, SSync };
   State sRequestState, sResponseState;

   void testHandshake(); // see p 41, 42, verified on the waveforms

   int uReset;
   void setReset(int uResetSource = 8)
      {  if (!observedSignals.rspack && !observedSignals.cmdval)
            uReset = 0;
         else
            uReset = uResetSource;
      }
   void testReset() // see p.31
      {  if (!observedSignals.cmdval && !observedSignals.cmdack && !observedSignals.rspval && !observedSignals.rspack)
            uReset = 0;
         else {
            assume(uReset > 1);
            --uReset;
         };
      }

   class Packet {
     public:
      typename vci_param::cmd_t cmd;
      typename vci_param::addr_t address;
      typename vci_param::contig_t contig;
      typename vci_param::plen_t plen;
      int uLength;
      
     public:
      Packet(const VciSignals<vci_param>& observedSignals)
         :  cmd(observedSignals.cmd), address(observedSignals.address), contig(observedSignals.contig), plen(observedSignals.plen), uLength(0) {}
      Packet(const Packet& source)
         :  cmd(source.cmd), address(source.address), contig(source.contig), plen(source.plen),
            uLength(source.uLength) {}
      int& length() { return uLength; }
   };
   class PacketsList {
     private:
      std::list<Packet*> lpContent;

     public:
      PacketsList() {}
      PacketsList(const PacketsList& source)
         {  for (typename std::list<Packet*>::const_iterator iter = source.lpContent.begin();
                  iter != source.lpContent.end(); ++iter)
               if (*iter) lpContent.push_back(new Packet(**iter));
         }
      ~PacketsList()
         {  for (typename std::list<Packet*>::iterator iter = lpContent.begin(); iter != lpContent.end(); ++iter)
               if (*iter) delete *iter;
         }
      int count() const { return lpContent.size(); }
      void add(const Packet& packet) { lpContent.push_back(new Packet(packet)); }
      Packet& last() const { return *lpContent.front(); }
      void pop()
         {  Packet* last = lpContent.front();
            lpContent.pop_front();
            if (last) delete last;
         } 
   };

   class AddressInterval {
     private:
      typename vci_param::addr_t aMin, aMax;

     public:
      AddressInterval() : aMin(0), aMax(0) {}
      AddressInterval(typename vci_param::addr_t aMinSource)
         :  aMin(aMinSource), aMax(aMinSource + vci_param::B) {}
      AddressInterval(typename vci_param::addr_t aMinSource, int plen)
         :  aMin(aMinSource), aMax(aMinSource + plen * vci_param::B) {}

      AddressInterval& operator=(const AddressInterval& source)
         {  aMin = source.aMin; aMax = source.aMax; return *this; }
      bool operator<(const AddressInterval& source) const
         {  return aMax < source.aMin; }
      bool operator>(const AddressInterval& source) const
         {  return aMax > source.aMin; }
      bool operator==(const AddressInterval& source) const
         {  return aMax >= source.aMin && aMin <= source.aMax; }
      void merge(const AddressInterval& source)
         {  if (aMin > source.aMin)
               aMin = source.aMin;
            if (aMax < source.aMax)
               aMax = source.aMax;
         }
      bool isValid() const { return aMax > aMin; }
      bool remove(const AddressInterval& source, AddressInterval*& paiOther, bool& fError)
         {  fError = source.aMin < aMin || source.aMax > aMax;
            bool fResult = (source.aMin <= aMin && source.aMax >= aMax);
            if (!fResult) {
               if (aMin < source.aMin) {
                  if (aMax > source.aMax) {
                     paiOther = new AddressInterval(source.aMax, aMax);
                     aMax = source.aMin;
                  }
                  else // aMax == source.aMax
                     aMax = source.aMin;
               }
               else { // aMin == source.aMin
                  if (aMax > source.aMax)
                     aMin = source.aMax;
               };
            };
            return fResult;
         }
   };
   class LockedAddress {
     private:
      std::set<AddressInterval> saiAddresses;

     public:
      LockedAddress() {}
      LockedAddress(const LockedAddress& source) : saiAddresses(source.saiAddresses) {}

      void add(typename vci_param::addr_t aAddress);
      void add(typename vci_param::addr_t aAddress, int plen);
      void remove(typename vci_param::addr_t aAddress);
      void remove(typename vci_param::addr_t aAddress, int plen);
      int count() const { return saiAddresses.size(); }
   };

   PacketsList plPendingPackets;
   LockedAddress laLockedAddresses;

   typename vci_param::clen_t packetClen;
   typename vci_param::cfixed_t packetCfixed;
   typename vci_param::addr_t packetAddress;
   typename vci_param::plen_t packetPlen;
   typename vci_param::cmd_t packetCmd;
   typename vci_param::contig_t packetContig;
   typename vci_param::wrap_t packetWrap;
   typename vci_param::const_t packetCons;
   
   int uRequestCells, uResponseCells;
   void acquireRequestCell();
   void acquireResponseCell();

   typename vci_param::val_t     cmdvalPrevious;
   typename vci_param::addr_t    addressPrevious;
   typename vci_param::be_t      bePrevious;
   typename vci_param::cfixed_t  cfixedPrevious;
   typename vci_param::clen_t    clenPrevious;
   typename vci_param::cmd_t     cmdPrevious;
   typename vci_param::contig_t  contigPrevious;
   typename vci_param::data_t    wdataPrevious;
   typename vci_param::eop_t     eopPrevious;
   typename vci_param::const_t   consPrevious;
   typename vci_param::plen_t    plenPrevious;
   typename vci_param::wrap_t    wrapPrevious;

   typename vci_param::val_t     rspvalPrevious;
   typename vci_param::data_t    rdataPrevious;
   typename vci_param::eop_t     reopPrevious;
   typename vci_param::rerror_t  rerrorPrevious;

  protected:
   SC_HAS_PROCESS(BasicVciAssert);

  public:
   BasicVciAssert(VciSignals<vci_param>& observedSignalsReference, sc_module_name insname)
      :  soclib::caba::BaseModule(insname), plog_file(NULL), fDefaultMode(true),
         observedSignals(observedSignalsReference),
         sRequestState(SIdle), sResponseState(SIdle), uReset(0), packetAddress(0),
         uRequestCells(0), uResponseCells(0), cmdvalPrevious(0)
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

         cmdvalPrevious = observedSignals.cmdval;
         addressPrevious = observedSignals.address;
         bePrevious = observedSignals.be;
         cfixedPrevious = observedSignals.cfixed;
         clenPrevious = observedSignals.clen;
         cmdPrevious = observedSignals.cmd;
         contigPrevious = observedSignals.contig;
         wdataPrevious = observedSignals.wdata;
         eopPrevious = observedSignals.eop;
         consPrevious = observedSignals.cons;
         plenPrevious = observedSignals.plen;
         wrapPrevious = observedSignals.wrap;

         rspvalPrevious = observedSignals.rspval;
         rdataPrevious = observedSignals.rdata;
         reopPrevious = observedSignals.reop;
         rerrorPrevious = observedSignals.rerror;
      }
   bool isFinished() const { return plPendingPackets.count() == 0 && laLockedAddresses.count() == 0; }
   
   void setLogOut(std::ostream& osOut) { plog_file = &osOut; }
   void setDefaultMode() { fDefaultMode = true; }
   void setFreeMode() { fDefaultMode = false; }
};

}} // end of namespace soclib::caba

/*
#include <systemc.h>
#include <fstream>

#include "../include/bvci_assert.h"

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
  soclib::caba::BasicVciAssert<MyVciParams> vciAssert(vciSignals, "VciAssert_verif");
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

#endif // SOCLIB_CABA_PVDC_BASIC_ASSERTH

