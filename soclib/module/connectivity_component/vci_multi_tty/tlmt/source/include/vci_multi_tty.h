#ifndef SOCLIB_VCI_MULTI_TTY_H
#define SOCLIB_VCI_MULTI_TTY_H

#include <tlmt>
#include "vci_ports.h"
#include "mapping_table.h"
#include "tty_wrapper.h"

namespace soclib { namespace tlmt {

template <typename vci_param>
class VciMultiTty
        : public tlmt_core::tlmt_module
{
 private:
  std::vector<soclib::common::TtyWrapper*> m_term;

  soclib::common::IntTab m_index;
  soclib::common::MappingTable m_mt;
  std::list<soclib::common::Segment> segList;

  tlmt_core::tlmt_return m_return;
  vci_rsp_packet<vci_param> m_rsp;

protected:
  SC_HAS_PROCESS(VciMultiTty);
public:
  soclib::tlmt::VciTarget<vci_param> p_vci;
  std::vector<tlmt_core::tlmt_out<bool> *> p_irq;

  VciMultiTty(sc_core::sc_module_name name,
	      const soclib::common::IntTab &index,
	      const soclib::common::MappingTable &mt,
	      const char *first_name,
	      ...);

 VciMultiTty(sc_core::sc_module_name name,
	     const soclib::common::IntTab &index,
	     const soclib::common::MappingTable &mt,
	     const std::vector<std::string> &names);
 
  tlmt_core::tlmt_return &callback(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
				   const tlmt_core::tlmt_time &time,
				   void *private_data);
  
  tlmt_core::tlmt_return &callback_read(size_t segIndex,soclib::common::Segment &s,
					soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					const tlmt_core::tlmt_time &time,
					void *private_data);
  
  tlmt_core::tlmt_return &callback_write(size_t segIndex,soclib::common::Segment &s,
					 soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					 const tlmt_core::tlmt_time &time,
					 void *private_data);

  void init(const std::vector<std::string> &names);
};
}}

#endif /* SOCLIB_VCI_MULTI_TTY_H */
