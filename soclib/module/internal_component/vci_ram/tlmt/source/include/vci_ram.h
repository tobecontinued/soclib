#ifndef SOCLIB_TLMT_VCI_RAM_H
#define SOCLIB_TLMT_VCI_RAM_H

#include <tlmt>
#include "vci_ports.h"
#include "tlmt_base_module.h"
#include "mapping_table.h"
#include "soclib_endian.h"
#include "elf_loader.h"
#include "linked_access_buffer.h"

namespace soclib { namespace tlmt {

template<typename vci_param>
class VciRam 
  : public soclib::tlmt::BaseModule
{
 private:

  uint32_t m_id;
  soclib::common::IntTab m_index;
  soclib::common::MappingTable m_mt;
  soclib::common::ElfLoader *m_loader;
  std::list<soclib::common::Segment> m_segments;
  typedef typename vci_param::data_t ram_t;
  ram_t **m_contents;

  soclib::common::LinkedAccessBuffer<typename vci_param::addr_t,uint32_t> m_atomic;

  tlmt_core::tlmt_return m_return;
  vci_rsp_packet<vci_param> m_rsp;

    // Activity counters
  uint32_t m_cpt_cycle;   // Count Cycles

  uint32_t m_cpt_read;   // Count READ access
  uint32_t m_cpt_write;  // Count WRITE access
  
  uint32_t m_cpt_read_packet;   // Count READ packet
  uint32_t m_cpt_read_1;        // Count READ packet of 1 word
  
  uint32_t m_cpt_write_packet;  // Count WRITE packet
  uint32_t m_cpt_write_1;       // Count WRITE packet of 1 word
  
 protected:
  SC_HAS_PROCESS(VciRam);
 public:
  soclib::tlmt::VciTarget<vci_param> p_vci;

  VciRam(sc_core::sc_module_name name,
	 uint32_t id,
	 const soclib::common::IntTab &index,
	 const soclib::common::MappingTable &mt,
	 common::ElfLoader &loader);
  
  ~VciRam();

  tlmt_core::tlmt_return &callback(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
				   const tlmt_core::tlmt_time &time,
				   void *private_data);
  
  tlmt_core::tlmt_return &callback_read(size_t segIndex,soclib::common::Segment &s,
					soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					const tlmt_core::tlmt_time &time,
					void *private_data);
  
  tlmt_core::tlmt_return &callback_locked_read(size_t segIndex,soclib::common::Segment &s,
					       soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					       const tlmt_core::tlmt_time &time,
					       void *private_data);

  tlmt_core::tlmt_return &callback_write(size_t segIndex,soclib::common::Segment &s,
					 soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					 const tlmt_core::tlmt_time &time,
					 void *private_data);

  tlmt_core::tlmt_return &callback_store_cond(size_t segIndex,soclib::common::Segment &s,
					      soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					      const tlmt_core::tlmt_time &time,
					      void *private_data);

  int getNCycles();
  int getActiveCycles();
  int getIdleCycles();
  int getNReadPacket();
  int getNWritePacket();
  int getNReadPacket_1word();
  int getNWritePacket_1word();
  int getNReadPacket_Nwords();
  int getNWritePacket_Nwords();
};

}}

#endif
