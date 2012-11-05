/*
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
 * Copyright (c) UPMC, Lip6, Asim
 *         Alain Greiner <alain.greiner@lip6.fr>, 2012
 *         Clement Devigne <clement.devigne@etu.upmc.fr>
 *
 * Maintainers: alain
 */

#ifndef MULTI_NIC_REGS_H
#define MULTI_NIC_REGS_H

#define NIC_CHANNEL_SPAN  0x2000       // 32 Kbytes per channel

enum SoclibMultiNicHyperviseurRegisters 
{
    NIC_G_VIS                   = 0,   // bitfield : bit N = 0 -> channel N disabled
    NIC_G_ON                    = 1,   // boolean : NIC component activated
    NIC_G_BC_ENABLE             = 2,   // boolean : Enable Broadcast if non zero
    NIC_G_TDM_ENABLE            = 3,   // boolean : TDM Scheduler if non zero / Round-Robin if zero
    NIC_G_TDM_PERIOD            = 4,   // TDM time slot value
    NIC_G_BYPASS_ENABLE         = 5,   // boolean : Enable bypass for TX packets
    //
    NIC_G_MAC_4                 = 8,   // channel mac address 32 LSB bits array[8]
    //
    NIC_G_MAC_2                 = 16,  // channel mac address 16 MSB bits array[8]
};

enum SoclibMultiNicChannelRegisters 
{
    NIC_C_RX_FULL_0             = 0,   // RX_0 container status            (Read/Write)
    NIC_C_RX_PBUF_0             = 1,   // RX_0 container base address      (Read/Write) 
    NIC_C_RX_FULL_1             = 2,   // RX_1 container status            (Read/Write)
    NIC_C_RX_PBUF_1             = 3,   // RX_1 container base address      (Read/Write) 
    NIC_C_TX_FULL_0             = 4,   // RX_0 container status            (Read/Write)
    NIC_C_TX_PBUF_0             = 5,   // RX_0 container base address      (Read/Write) 
    NIC_C_TX_FULL_1             = 6,   // RX_1 container status            (Read/Write)
    NIC_C_TX_PBUF_1             = 7,   // RX_1 container base address      (Read/Write) 
    NIC_C_MAC_4                 = 8,   // channel mac address 32 LSB bits  (Read Only)
    NIC_C_MAC_2                 = 9,   // channel mac address 16 LSB bits  (Read Only)

    // instrumentation registers
    NIC_G_NPKT_BYPASS           = 16,  // number of channel to channel packets
    NIC_G_NPKT_RX_G2S_RECEVEID  = 17,  // number of packets received on GMII RX port
    NIC_G_NPKT_RX_G2S_DISCARDED = 18,  // number of packets discarded by RX_G2S FSM
    NIC_G_NPKT_RX_G2S_ERROR     = 18,  // number of error packets transmit by RX_G2S FSM
};

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

 
