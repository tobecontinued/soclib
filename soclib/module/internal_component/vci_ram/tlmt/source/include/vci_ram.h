#ifndef SOCLIB_TLMT_VCI_RAM_H
#define SOCLIB_TLMT_VCI_RAM_H

#include <tlmt>
#include "vci_ports.h"
#include "tlmt_base_module.h"
#include "mapping_table.h"
#include "soclib_endian.h"
#include "elf_loader.h"
#include "linked_access_buffer.h"

template<typename vci_param>
struct line_table{
  typename vci_param::addr_t address;
  uint32_t                   srcid;
  line_table<vci_param>*     previous;
  line_table<vci_param>*     next;
};

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
  tlmt_core::tlmt_return m_return;
  vci_rsp_packet<vci_param> rsp;

  line_table<vci_param> *tableLL_begin;
  line_table<vci_param> *tableLL_end;
  //soclib::common::LinkedAccessBuffer<typename vci_param::addr_t,uint32_t> m_atomic;

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
  
  void insertLineTable(typename vci_param::addr_t address, uint32_t srcid);
  void removeLineTable(typename vci_param::addr_t address);
  bool existsAddress(typename vci_param::addr_t address);
  bool isOwner(typename vci_param::addr_t address, uint32_t srcid);
  void showTable();

};

}}

#endif
