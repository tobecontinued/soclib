#ifndef SOCLIB_TLMT_VCI_MWMR_CONTROLLER_H
#define SOCLIB_TLMT_VCI_MWMR_CONTROLLER_H

#include <tlmt>
#include "vci_ports.h"
#include "fifo_ports.h"
#include "tlmt_base_module.h"
#include "mapping_table.h"
#include "soclib_endian.h"

template<typename vci_param>
struct channel_struct{
  typename vci_param::addr_t status_address;
  typename vci_param::addr_t base_address;
  uint32_t                   depth;
  bool                       running;
};

template<typename vci_param>
struct fifos_struct{
  typename vci_param::data_t *data;
  bool                       full;
  tlmt_core::tlmt_time       time;
};

template<typename vci_param>
struct request_struct{
  typename vci_param::data_t *data;
  bool                       pending;
  tlmt_core::tlmt_time       time;
};

namespace soclib { namespace tlmt {

template<typename vci_param>
class VciMwmrController 
  : public soclib::tlmt::BaseModule
{
 private:
  soclib::common::IntTab m_initiator_index;
  soclib::common::IntTab m_target_index;
  soclib::common::MappingTable m_mt;
  std::list<soclib::common::Segment> m_initiator_segments;
  std::list<soclib::common::Segment> m_target_segments;

  tlmt_core::tlmt_thread_context c0;

////////////// hardware FIFOs ////////////////////////
  fifos_struct<vci_param>   *m_read_fifo;
  fifos_struct<vci_param>   *m_write_fifo;

/////// memory mapped channels registers ////////////
  uint32_t                  m_channel_index;
  bool                      m_channel_read;
  channel_struct<vci_param> *m_read_channel;
  channel_struct<vci_param> *m_write_channel;

///////// registers arrays for coprocessor requests //////////////////
  request_struct<vci_param> *m_read_request;
  request_struct<vci_param> *m_write_request;

  ////////////////////// signals /////////////////////////////////
  sc_core::sc_event m_vci_event;
  sc_core::sc_event m_fifo_event;
  sc_core::sc_event m_active_coprocessor_event;

  uint32_t m_srcid;
  uint32_t m_destid;
  uint32_t m_pktid;
  uint32_t m_read_fifo_depth;
  uint32_t m_write_fifo_depth;
  uint32_t m_read_channels;
  uint32_t m_write_channels;
  uint32_t m_config_registers;
  uint32_t m_status_registers;
  uint32_t m_waiting_time; 
  bool     m_reset_request; 
  bool     m_active_coprocessor; 

  tlmt_core::tlmt_return    m_return;
  vci_cmd_packet<vci_param> m_cmd;
  vci_rsp_packet<vci_param> m_rsp;

 protected:
  SC_HAS_PROCESS(VciMwmrController);
 public:
  tlmt_core::tlmt_in<bool> p_state;

  soclib::tlmt::VciInitiator<vci_param> p_vci_initiator;
  soclib::tlmt::VciTarget<vci_param>    p_vci_target;

  std::vector<tlmt_core::tlmt_out<typename vci_param::data_t> *>  p_config;
  std::vector<tlmt_core::tlmt_out<typename vci_param::data_t*> *> p_status;

  std::vector<soclib::tlmt::FifoTarget<vci_param> *> p_read_fifo;
  std::vector<soclib::tlmt::FifoTarget<vci_param> *> p_write_fifo;

  VciMwmrController(sc_core::sc_module_name name,
		    uint32_t src,
		    const soclib::common::IntTab &initiator_index,
		    uint32_t dest,
		    const soclib::common::IntTab &target_index,
		    const soclib::common::MappingTable &mt,
		    uint32_t read_fifo_depth,
		    uint32_t write_fifo_depth,
		    uint32_t n_read_channels,
		    uint32_t n_write_channels,
		    uint32_t n_config,
		    uint32_t n_status);
  
  tlmt_core::tlmt_return &setStateCoprocessor(bool state,
					      const tlmt_core::tlmt_time &time,
					      void *private_data);

  tlmt_core::tlmt_return &vciRspReceived(soclib::tlmt::vci_rsp_packet<vci_param> *pkt,
					 const tlmt_core::tlmt_time &time,
					 void *private_data);

  tlmt_core::tlmt_return &vciCmdReceived(soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					 const tlmt_core::tlmt_time &time,
					 void *private_data);
  
  tlmt_core::tlmt_return &vciCmdReceived_read(size_t segIndex,soclib::common::Segment &s,
					      soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					      const tlmt_core::tlmt_time &time,
					      void *private_data);
  
  tlmt_core::tlmt_return &vciCmdReceived_write(size_t segIndex,soclib::common::Segment &s,
					       soclib::tlmt::vci_cmd_packet<vci_param> *pkt,
					       const tlmt_core::tlmt_time &time,
					       void *private_data);
  
  tlmt_core::tlmt_return &readRequestReceived(soclib::tlmt::fifo_cmd_packet<vci_param> *req,
					      const tlmt_core::tlmt_time &time,
					      void *private_data);

  tlmt_core::tlmt_return &writeRequestReceived(soclib::tlmt::fifo_cmd_packet<vci_param> *req,
					       const tlmt_core::tlmt_time &time,
					       void *private_data);

  void reset();
  void execLoop();
  void getLock(typename vci_param::addr_t status_address, uint32_t *status);
  void releaseLock(typename vci_param::addr_t status_address, uint32_t *status);
  void readStatus(typename vci_param::addr_t status_address, uint32_t *status);
  void updateStatus(typename vci_param::addr_t status_address, uint32_t *status);
  void readFromChannel(uint32_t fifo_index, uint32_t *status);
  void writeToChannel(uint32_t fifo_index, uint32_t *status);
  void releasePendingFifos();
};

}}

#endif
