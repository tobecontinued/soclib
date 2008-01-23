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

#ifndef SOCLIB_CABA_PVDC_BASIC_FILTERH
#define SOCLIB_CABA_PVDC_BASIC_FILTERH

#include "caba/util/base_module.h"
#include "caba/interface/vci_initiator.h"
#include "caba/interface/vci_target.h"

#include <list>
#include <set>

namespace soclib { namespace caba {

template <typename vci_param>
class BasicVciFilter : public soclib::caba::BaseModule {
  private:
   std::ostream* plog_file;
   bool fDefaultMode;

  public:
   struct In {
      sc_in<typename vci_param::val_t>     cmdval;
      sc_in<typename vci_param::addr_t>    address;
      sc_in<typename vci_param::be_t>      be;
      sc_in<typename vci_param::cfixed_t>  cfixed;
      sc_in<typename vci_param::clen_t>    clen;
      sc_in<typename vci_param::cmd_t>     cmd;
      sc_in<typename vci_param::contig_t>  contig;
      sc_in<typename vci_param::data_t>    wdata;
      sc_in<typename vci_param::eop_t>     eop;
      sc_in<typename vci_param::const_t>   cons;
      sc_in<typename vci_param::plen_t>    plen;
      sc_in<typename vci_param::wrap_t>    wrap;
      sc_in<typename vci_param::ack_t>     rspack;

      sc_out<typename vci_param::ack_t>    cmdack;
      sc_out<typename vci_param::val_t>    rspval;
   	sc_out<typename vci_param::data_t>   rdata;
      sc_out<typename vci_param::eop_t>    reop;
   	sc_out<typename vci_param::rerror_t> rerror;

#define __ren(x) x((name+"_in_" #x).c_str())
      In(const std::string &name)
         :  __ren(cmdval), __ren(address), __ren(be), __ren(cfixed), __ren(clen), __ren(cmd),
            __ren(contig), __ren(wdata), __ren(eop), __ren(cons), __ren(plen), __ren(wrap),
            __ren(rspack),
#undef __ren
#define __ren(x) x((name+"_out_" #x).c_str())
           __ren(cmdack), __ren(rspval), __ren(rdata), __ren(reop), __ren(rerror) {}
#undef __ren

      void operator()(VciSignals<vci_param> &sig)
         {  cmdval  (sig.cmdval);
            address (sig.address);
            be      (sig.be);
            cfixed  (sig.cfixed);
            clen    (sig.clen);
            cmd     (sig.cmd);
            contig  (sig.contig);
            wdata   (sig.wdata);
            eop     (sig.eop);
            cons    (sig.cons);
            plen    (sig.plen);
            wrap    (sig.wrap);
            rspack  (sig.rspack);
            
            cmdack  (sig.cmdack);
            rspval  (sig.rspval);
            rdata   (sig.rdata);
            reop    (sig.reop);
            rerror  (sig.rerror);
         }

	   void operator()(VciInitiator<vci_param> &ports) // To see : create a VciSignals between the ports
         {  cmdval  (ports.cmdval);
            address (ports.address);
            be      (ports.be);
            cfixed  (ports.cfixed);
            clen    (ports.clen);
            cmd     (ports.cmd);
            contig  (ports.contig);
            wdata   (ports.wdata);
            eop     (ports.eop);
            cons    (ports.cons);
            plen    (ports.plen);
            wrap    (ports.wrap);
            rspack  (ports.rspack);
            
            cmdack  (ports.cmdack);
            rspval  (ports.rspval);
            rdata   (ports.rdata);
            reop    (ports.reop);
            rerror  (ports.rerror);
         }
   };

   struct Out {
      sc_in<typename vci_param::ack_t>     cmdack;
      sc_in<typename vci_param::val_t>     rspval;
   	sc_in<typename vci_param::data_t>    rdata;
      sc_in<typename vci_param::eop_t>     reop;
   	sc_in<typename vci_param::rerror_t>  rerror;
      
      sc_out<typename vci_param::val_t>    cmdval;
      sc_out<typename vci_param::addr_t>   address;
      sc_out<typename vci_param::be_t>     be;
      sc_out<typename vci_param::cfixed_t> cfixed;
      sc_out<typename vci_param::clen_t>   clen;
      sc_out<typename vci_param::cmd_t>    cmd;
      sc_out<typename vci_param::contig_t> contig;
      sc_out<typename vci_param::data_t>   wdata;
      sc_out<typename vci_param::eop_t>    eop;
      sc_out<typename vci_param::const_t>  cons;
      sc_out<typename vci_param::plen_t>   plen;
      sc_out<typename vci_param::wrap_t>   wrap;
      sc_out<typename vci_param::ack_t>    rspack;

      Out(const std::string &name)
#define __ren(x) x((name+"_in_" #x).c_str())
         :  __ren(cmdack), __ren(rspval), __ren(rdata), __ren(reop), __ren(rerror),
#undef __ren
#define __ren(x) x((name+"_out_" #x).c_str())
            __ren(cmdval), __ren(address), __ren(be), __ren(cfixed), __ren(clen), __ren(cmd),
            __ren(contig), __ren(wdata), __ren(eop), __ren(cons), __ren(plen), __ren(wrap),
            __ren(rspack) {}
#undef __ren

      void operator()(VciSignals<vci_param> &sig)
         {  cmdack  (sig.cmdack);
            rspval  (sig.rspval);
            rdata   (sig.rdata);
            reop    (sig.reop);
            rerror  (sig.rerror);

            cmdval  (sig.cmdval);
            address (sig.address);
            be      (sig.be);
            cfixed  (sig.cfixed);
            clen    (sig.clen);
            cmd     (sig.cmd);
            contig  (sig.contig);
            wdata   (sig.wdata);
            eop     (sig.eop);
            cons    (sig.cons);
            plen    (sig.plen);
            wrap    (sig.wrap);
            rspack  (sig.rspack);
         }

      void operator()(VciTarget<vci_param> &ports)
         {  cmdack  (ports.cmdack);
            rspval  (ports.rspval);
            rdata   (ports.rdata);
            reop    (ports.reop);
            rerror  (ports.rerror);

            cmdval  (ports.cmdval);
            address (ports.address);
            be      (ports.be);
            cfixed  (ports.cfixed);
            clen    (ports.clen);
            cmd     (ports.cmd);
            contig  (ports.contig);
            wdata   (ports.wdata);
            eop     (ports.eop);
            cons    (ports.cons);
            plen    (ports.plen);
            wrap    (ports.wrap);
            rspack  (ports.rspack);
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

   enum State { SIdle, SValid, SDefault_Ack, SSync };
   State sRequestState, sResponseState;

   void testHandshake(); // see p 41, 42, verified on the waveforms

   int uReset;
   void setReset(int uResetSource = 8)
      {  if (!out.rspack && !in.cmdval)
            uReset = 0;
         else
            uReset = uResetSource;
      }
   void testReset() // see p.31
      {  if (!in.cmdval && !out.cmdack && !out.rspval && !in.rspack)
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
      Packet(const In& in)
         :  cmd(in.cmd), address(in.address), contig(in.contig), plen(in.plen), uLength(0) {}
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
   SC_HAS_PROCESS(BasicVciFilter);

  public:
   BasicVciFilter(sc_module_name insname)
      :  soclib::caba::BaseModule(insname), plog_file(NULL), fDefaultMode(true),
         in((const char*) insname), out((const char*) insname),
         sRequestState(SIdle), sResponseState(SIdle), uReset(0), packetAddress(0),
         uRequestCells(0), uResponseCells(0), cmdvalPrevious(0)
      {  SC_METHOD(transition);
         dont_initialize();
         sensitive << in.cmdval << in.address << in.be << in.cfixed << in.clen << in.cmd
            << in.contig << in.wdata << in.eop << in.cons << in.plen << in.wrap << in.rspack
            << out.cmdack << out.rspval << out.rdata << out.reop << out.rerror;
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
      {  out.cmdval = in.cmdval;
         out.address = in.address;
         out.be = in.be;
         out.cfixed = in.cfixed;
         out.clen = in.clen;
         out.cmd = in.cmd;
         out.contig = in.contig;
         out.wdata = in.wdata;
         out.eop = in.eop;
         out.cons = in.cons;
         out.plen = in.plen;
         out.wrap = in.wrap;
         out.rspack = in.rspack;
         in.cmdack = out.cmdack;
         in.rspval = out.rspval;
         in.rdata = out.rdata;
         in.reop = out.reop;
         in.rerror = out.rerror;
      }
   void filter() 
      {  testHandshake();
         if (uReset > 0) testReset();

         cmdvalPrevious = in.cmdval;
         addressPrevious = in.address;
         bePrevious = in.be;
         cfixedPrevious = in.cfixed;
         clenPrevious = in.clen;
         cmdPrevious = in.cmd;
         contigPrevious = in.contig;
         wdataPrevious = in.wdata;
         eopPrevious = in.eop;
         consPrevious = in.cons;
         plenPrevious = in.plen;
         wrapPrevious = in.wrap;

         rspvalPrevious = out.rspval;
         rdataPrevious = out.rdata;
         reopPrevious = out.reop;
         rerrorPrevious = out.rerror;
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

#include "caba/verification/bvci_filter.h"

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
  soclib::caba::BasicVciFilter<MyVciParams> vciFilter("VciFilter_verif");
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

#endif // SOCLIB_CABA_PVDC_BASIC_FILTERH

