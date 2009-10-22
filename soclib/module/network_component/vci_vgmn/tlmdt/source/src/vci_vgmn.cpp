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
 * Maintainers: fpecheux, alinevieiramello@hotmail.com
 *
 * Copyright (c) UPMC / Lip6, 2008
 *     François Pêcheux <francois.pecheux@lip6.fr>
 *     Aline Vieira de Mello <aline.vieira-de-mello@lip6.fr>
 */

#include "../include/vci_vgmn.h"

namespace soclib { namespace tlmdt {

/////////////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////////////
VciVgmn::VciVgmn
( sc_core::sc_module_name name,                      // module name
  const soclib::common::MappingTable &mt,            // mapping table
  size_t nb_init,                                    // number of initiators
  size_t nb_target,                                  // number of targets
  size_t min_latency,                                // minimal latency
  size_t fifo_depth )                                // parameter do not use 
  : m_centralized_buffer(nb_init)                   // centralized buffer
{
  // Phase 1, allocate nb_target CmdArbRspRout blocks
  for (size_t i=0;i<nb_target;i++){
    std::ostringstream tmpName;
    tmpName << name << "_CmdArbRspRout" << i;
    m_CmdArbRspRout.push_back(new VciCmdArbRspRout(tmpName.str().c_str(), mt, soclib::common::IntTab(), min_latency * UNIT_TIME, false));
  }
  
  // Phase 2, allocate nb_init RspArbCmdRout blocks
  for (size_t i=0;i<nb_init;i++){
    std::ostringstream tmpName;
    tmpName << name << "_RspArbCmdRout" << i;
    m_RspArbCmdRout.push_back(new VciRspArbCmdRout(tmpName.str().c_str() ,mt, soclib::common::IntTab(), i, min_latency * UNIT_TIME, &m_centralized_buffer));
  }
  
  // Phase 3, each cmdArbRspRout sees all the RspArbCmdRout
  for (size_t i=0;i<nb_target;i++){
    m_CmdArbRspRout[i]->setRspArbCmdRout(m_RspArbCmdRout);
  }
  
  // Phase 4, each rspArbCmdRout sees all the CmdArbRspRout
  for (size_t i=0;i<nb_init;i++){
    m_RspArbCmdRout[i]->setCmdArbRspRout(m_CmdArbRspRout);
  }
}

VciVgmn::VciVgmn
( sc_core::sc_module_name name,                      // module name
  const soclib::common::MappingTable &mt,            // mapping table
  const soclib::common::IntTab &global_index,        // global index
  int nb_init,                                       // number of initiators
  int nb_target,                                     // number of targets
  sc_core::sc_time delay )                           // interconnect delay
  : m_centralized_buffer(nb_init)                    // centralized buffer
{
  // Phase 1, allocate nb_target CmdArbRspRout blocks
  for (int i=0;i<nb_target;i++)
    {
      std::ostringstream tmpName;
      tmpName << name << "_CmdArbRspRout" << i;
      m_CmdArbRspRout.push_back(new VciCmdArbRspRout(tmpName.str().c_str(), mt, global_index, delay, false));
    }
  
  // Phase 2, allocate nb_init RspArbCmdRout blocks
  for (int i=0;i<nb_init;i++)
    {
      std::ostringstream tmpName;
      tmpName << name << "_RspArbCmdRout" << i;
      m_RspArbCmdRout.push_back(new VciRspArbCmdRout(tmpName.str().c_str() ,mt, global_index, i, delay, &m_centralized_buffer));
    }
  
  // Phase 3, each cmdArbRspRout sees all the RspArbCmdRout
  for (int i=0;i<nb_target;i++)
    {
      m_CmdArbRspRout[i]->setRspArbCmdRout(m_RspArbCmdRout);
    }
  
  // Phase 4, each rspArbCmdRout sees all the CmdArbRspRout
  for (int i=0;i<nb_init;i++)
    {
      m_RspArbCmdRout[i]->setCmdArbRspRout(m_CmdArbRspRout);
    }
}
    
}}
