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
 *         Sylvain Leroy <sylvain.leroy@lip6.fr>
 *         Cassio Fraga <cassio.fraga@lip6.fr>
 *
 * Maintainers: alain
 */

#ifndef MULTI_NIC_REGS_H
#define MULTI_NIC_REGS_H

#define NIC_CHANNEL_SPAN  0x2000       // 32 Kbytes per channel

enum SoclibMultiNicHyperRegisters {
    NIC_G_VIS                        = 0,   // bitfield : bit N = 0 -> channel N disabled
    NIC_G_ON                         = 1,   // boolean : NIC component activated
    NIC_G_NB_CHAN                    = 2,   // Number of channels present in this NIC (read only)
    NIC_G_BC_ENABLE                  = 3,   // boolean : Enable Broadcast if non zero
    NIC_G_TDM_ENABLE                 = 4,   // boolean : TDM Scheduler if non zero
    NIC_G_TDM_PERIOD                 = 5,   // TDM time slot value
    NIC_G_BYPASS_ENABLE              = 6,   // boolean : Enable bypass for TX packets
    // alignement only
    NIC_G_MAC_4                      = 8,   // channel mac address 32 LSB bits array[8]
    // array
    NIC_G_MAC_2                      = 16,  // channel mac address 16 MSB bits array[8]
    // array
    NIC_G_NPKT_RX_G2S_RECEIVED       = 32,  // number of packets received on GMII RX port
    NIC_G_NPKT_RX_G2S_DISCARDED      = 33,  // number of RX packets discarded by RX_G2S FSM

    NIC_G_NPKT_RX_DES_SUCCESS        = 34,  // number of RX packets transmited by RX_DES FSM
    NIC_G_NPKT_RX_DES_TOO_SMALL      = 35,  // number of discarded too small RX packets (<60B)
    NIC_G_NPKT_RX_DES_TOO_BIG        = 36,  // number of discarded too big RX packets (>1514B)
    NIC_G_NPKT_RX_DES_MFIFO_FULL     = 37,  // number of discarded RX packets if fifo full
    NIC_G_NPKT_RX_DES_CRC_FAIL       = 38,  // number of discarded RX packets if CRC32 failure

    NIC_G_NPKT_RX_DISPATCH_RECEIVED  = 39,  // number of packets received by RX_DISPATCH FSM
    NIC_G_NPKT_RX_DISPATCH_BROADCAST = 40,  // number of broadcast RX packets received
    NIC_G_NPKT_RX_DISPATCH_DST_FAIL  = 41,  // number of discarded RX packets if DST MAC not found
    NIC_G_NPKT_RX_DISPATCH_CH_FULL   = 42,  // number of discarded RX packets if channel full

    NIC_G_NPKT_TX_DISPATCH_RECEIVED  = 43,  // number of packets received by TX_DISPATCH FSM
    NIC_G_NPKT_TX_DISPATCH_TOO_SMALL = 44,  // number of discarded too small TX packets (<60B)
    NIC_G_NPKT_TX_DISPATCH_TOO_BIG   = 45,  // number of discarded too big TX packets (>1514B)
    NIC_G_NPKT_TX_DISPATCH_SRC_FAIL  = 46,  // number of discarded TX packets if illegal SRC MAC
    NIC_G_NPKT_TX_DISPATCH_BROADCAST = 47,  // number of broadcast TX packets received
    NIC_G_NPKT_TX_DISPATCH_BYPASS    = 48,  // number of bypassed TX->RX packets
    NIC_G_NPKT_TX_DISPATCH_TRANSMIT  = 49,  // number of transmit TX packets
};

///////////////////////////////////////////////////////////////////////////////////////////////
// A buffer descriptor occupies 64 bytes, but only 8 bytes (two 32 bits words) are useful:
// LOW WORD   contains the 32 LSB bits of the buffer paddr                
// HIGH WORD  contains the 16 MSB bits of the buffer paddr, plus buffer status (leftmost bit)
///////////////////////////////////////////////////////////////////////////////////////////////

enum SoclibMultiNicChannelRegisters 
{
    NIC_RX_DESC_LO_0          = 0,   // RX_0 descriptor low word         (Read/Write)
    NIC_RX_DESC_HI_0          = 1,   // RX_0 descriptor high word        (Read/Write)
    NIC_RX_DESC_LO_1          = 16,  // RX_1 descriptor low word         (Read/Write)
    NIC_RX_DESC_HI_1          = 17,  // RX_1 descriptor high word        (Read/Write) 
    NIC_TX_DESC_LO_0          = 32,  // TX_0 descriptor low word         (Read/Write)
    NIC_TX_DESC_HI_0          = 33,  // TX_0 descriptor high word        (Read/Write) 
    NIC_TX_DESC_LO_1          = 48,  // TX_1 descriptor low word         (Read/Write)
    NIC_TX_DESC_HI_1          = 49,  // TX_1 descriptor high word        (Read/Write) 
    NIC_MAC_4                 = 64,  // channel mac address 32 LSB bits  (Read Only)
    NIC_MAC_2                 = 65,  // channel mac address 16 LSB bits  (Read Only)
    NIC_RX_RUN                = 66,  // RX packets can be received       (write_only)
    NIC_TX_RUN                = 67,  // TX packets can be transmitted    (write_only)
};

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4


