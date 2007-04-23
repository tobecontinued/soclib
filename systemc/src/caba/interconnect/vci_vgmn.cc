/* -*- c++ -*-
 * This file is part of SoCLIB.
 *
 * SoCLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SoCLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SoCLIB; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, SoC
 *         Nicolas Pouillon <nipo@ssji.net>, 2007
 *
 * Maintainers: nipo
 */

#include "caba/interconnect/vci_vgmn.h"
#include "caba/interface/vci_buffers.h"
#include "common/register.h"

namespace soclib { namespace caba {

#define tmpl(x)                                 \
template<                                       \
    typename vci_param,                         \
    size_t NB_INITIAT,                          \
    size_t NB_TARGET,                           \
    size_t MIN_LATENCY,                         \
    size_t FIFO_DEPTH>                          \
x VciVgmn<                                      \
    vci_param,                                  \
    NB_INITIAT,                                 \
    NB_TARGET,                                  \
    MIN_LATENCY,                                \
    FIFO_DEPTH>

tmpl(void)::transition()
{
    if (p_resetn == false) {
        for ( size_t i=0 ; i<NB_INITIAT ; i++) {
              T_ALLOC_STATE[i]     = false;
              T_ALLOC_VALUE[i]     = 0;
            GLOBAL_RSP_STATE[i]  = 0;
            for ( size_t k=0 ; k<MIN_LATENCY ; k++){
                T_DELAY_VALID[i][k] = false;
                T_DELAY_VCICMD[i][k] = 0;
                T_DELAY_VCIDATA[i][k] = 0;
                T_DELAY_VCIID[i][k] = 0;
            }
            T_DELAY_PTR[i] = 0;
            for ( size_t j=0; j<NB_TARGET ; j++){
                CMD_FIFO_PTR[j][i]   = 0;
                CMD_FIFO_PTW[j][i]   = 0;
                CMD_FIFO_STATE[j][i] = 0;
                CMD_COUNTER [j][i]   = 0;
            }
        }
        for ( size_t i=0 ; i<NB_TARGET ; i++) {
              I_ALLOC_STATE[i]     = false;
              I_ALLOC_VALUE[i]     = 0;
            GLOBAL_CMD_STATE[i]  = 0;
            for ( size_t k=0 ; k<MIN_LATENCY; k++){
                I_DELAY_VALID[i][k] = false;
                I_DELAY_VCIDATA[i][k] = 0;
                I_DELAY_VCICMD[i][k] = 0;
            }
            I_DELAY_PTR[i] = 0;
            for ( size_t j=0 ; j<NB_INITIAT ; j++){
                RSP_FIFO_PTR[j][i]   = 0;
                RSP_FIFO_PTW[j][i]   = 0;
                RSP_FIFO_STATE[j][i] = 0;
                RSP_COUNTER [j][i]   = 0;
            }
        }
        return;
    }

    int global_rsp_temp[NB_INITIAT];
    int global_cmd_temp[NB_TARGET];

    int rsp_temp[NB_INITIAT][NB_TARGET];
    int cmd_temp[NB_TARGET][NB_INITIAT];

    ////////////////////////////////////////////////////////////
    //    LOOP 0
    // initialise the temporary variables for the
    // CMD_FIFO & RSP_FIFO states
    ////////////////////////////////////////////////////////////

    for ( size_t i=0 ; i<NB_INITIAT ; i++) {
        global_rsp_temp[i]=GLOBAL_RSP_STATE[i];
        for ( size_t j=0 ; j<NB_TARGET ; j++) {
            rsp_temp[i][j]=RSP_FIFO_STATE[i][j];
        }
    }

    for ( size_t i=0 ; i<NB_TARGET ; i++) {
        global_cmd_temp[i]=GLOBAL_CMD_STATE[i];
        for ( size_t j=0 ; j<NB_INITIAT ; j++) {
            cmd_temp[i][j]=CMD_FIFO_STATE[i][j];
        }
    }

    ////////////////////////////////////////////////////////////
    //    LOOP 1
    // - write T_DELAY_FIFOs
    // - write CMD_FIFOs
    ////////////////////////////////////////////////////////////

    for ( size_t i=0 ; i<NB_INITIAT ; i++) {
        bool wok = p_from_initiator[i].cmdval.read();

        int ptr = T_DELAY_PTR[i];

        size_t k = m_target_from_addr[T_DELAY_VCIADR[i][ptr].read()];

        bool valid = T_DELAY_VALID[i][ptr];

        bool full = (CMD_FIFO_STATE[k][i] == FIFO_DEPTH);

        int ptw = CMD_FIFO_PTW[k][i];

        // Write fifo retard if there is an empty slot
        // or the fifo cmd is not full
        if (!valid || !full) {
            // VCI command word
            int t_vci_command =
                ((int)p_from_initiator[i].be.read()    <<0 )+
                ((int)p_from_initiator[i].cmd.read()   <<4 )+
                ((int)p_from_initiator[i].plen.read()  <<6 )+
                ((int)p_from_initiator[i].eop.read()   <<14)+
                ((int)p_from_initiator[i].cons.read()  <<15)+
                ((int)p_from_initiator[i].contig.read()<<16)+
                ((int)p_from_initiator[i].cfixed.read()<<17)+
                ((int)p_from_initiator[i].wrap.read()  <<18)+
                ((int)p_from_initiator[i].clen.read()  <<19);

            // VCI ident word
            int t_vci_id =
                ((int)p_from_initiator[i].trdid.read() <<0)+
                ((int)p_from_initiator[i].pktid.read() <<8)+
                ((int)p_from_initiator[i].srcid.read() <<16);

            T_DELAY_PTR[i]          = (ptr + 1) % MIN_LATENCY ;
            T_DELAY_VCIDATA[i][ptr] = (int)p_from_initiator[i].wdata.read();
            T_DELAY_VCIADR[i][ptr]  = (int)p_from_initiator[i].address.read();
            T_DELAY_VCICMD[i][ptr]  = t_vci_command;
            T_DELAY_VCIID[i][ptr]   = t_vci_id;
        }

        //  update the VALID flag in the fifo delay
        T_DELAY_VALID[i][ptr] =  wok || (T_DELAY_VALID[i][ptr] && full);

        //  Write fifo cmd when it is not full and
        //  the fifo delay has a valid data
        if (!full && valid){
            cmd_temp[k][i] += 1;
            global_cmd_temp[k] += 1;


            CMD_FIFO_DATA[k][i][ptw] = T_DELAY_VCIDATA[i][ptr];
            CMD_FIFO_ADR [k][i][ptw] = T_DELAY_VCIADR [i][ptr];
            CMD_FIFO_CMD [k][i][ptw] = T_DELAY_VCICMD [i][ptr];
            CMD_FIFO_ID  [k][i][ptw] = T_DELAY_VCIID  [i][ptr];
            CMD_FIFO_PTW [k][i]      = (ptw + 1) % FIFO_DEPTH;

            CMD_COUNTER  [k][i] = CMD_COUNTER [k][i] + 1;

            if(wok == true)
                if (i != m_initiator_from_srcid[
                        (int)p_from_initiator[i].srcid.read()]) {
                    printf("Error in the SOCLIB_VCI_GMN component\n");
                    printf("The SRCID field does not match the port index!!!\n");
                    printf("i=%lu, SRCID =%d\n\n",(unsigned long)i,(int)p_from_initiator[i].srcid.read() );
                    sc_stop();
                }

        }
    } // end loop 1

    ////////////////////////////////////////////////////////////
    //    LOOP 2
    // - write I_DELAY_FIFOs
    // - write RSP_FIFOs
    ////////////////////////////////////////////////////////////

    for ( size_t j=0; j<NB_TARGET;j++) {

        int ptr = I_DELAY_PTR[j];

        size_t k = I_DELAY_VCICMD[j][ptr] >> (16+m_initiator_from_srcid.getDrop());

        bool full = (RSP_FIFO_STATE[k][j] == FIFO_DEPTH);

        bool valid = I_DELAY_VALID[j][ptr];

        // Write fifo retard if there is an empty slot
        // or the fifo rsp is not full
        if (!valid || !full) {
            int i_vci_cmd_id =
                (int)p_to_target[j].rerror.read()        + // RERROR = bits 2 to 0
                ((int)p_to_target[j].reop.read()   << 3) + // REOP = bit 3
                ((int)p_to_target[j].rtrdid.read() << 4) + // RTRDID = bits 11 to 4
                ((int)p_to_target[j].rpktid.read() << 12)+ // RPKTID = bits 15 to 12
                ((int)p_to_target[j].rsrcid.read() << 16); // RSRCID = bits 31 to 16
            I_DELAY_PTR[j]          = (ptr + 1) % MIN_LATENCY;
            I_DELAY_VCIDATA[j][ptr] = (int)p_to_target[j].rdata.read();
            I_DELAY_VCICMD [j][ptr] = i_vci_cmd_id;
        }

        //  update the VALID flag in the fifo retard
        bool wok = p_to_target[j].rspval.read();
        I_DELAY_VALID[j][ptr] = wok || (I_DELAY_VALID[j][ptr] && full);

        //  Write fifo rsp when it is not full and
        //  the fifo retard has a valid data
        if (!full && valid){
            rsp_temp[k][j] += 1;
            global_rsp_temp[k] += 1;

            int ptw = RSP_FIFO_PTW[k][j];
            RSP_FIFO_DATA[k][j][ptw] = I_DELAY_VCIDATA[j][ptr];
            RSP_FIFO_CMD [k][j][ptw] = I_DELAY_VCICMD [j][ptr];
            RSP_FIFO_PTW [k][j]      = (ptw + 1) % FIFO_DEPTH;

            RSP_COUNTER  [k][j] = RSP_COUNTER [k][j] + 1;
        }
    } // end Loop 2

    ////////////////////////////////////////////////////////////
    //    LOOP 3
    // - Write T_ALLOC registers
    // - Read RSP FIFOs
    // The T_ALLOC FSM controls the initiator output port
    // allocation to a given target input port.
    // The allocation policy is round-robin.
    ////////////////////////////////////////////////////////////

    for ( size_t i=0; i<NB_INITIAT ; i++) {

        if (T_ALLOC_STATE[i] == false) {
            if (GLOBAL_RSP_STATE[i] != 0) { // new allocation
                for ( size_t j = 0 ; ((j < NB_TARGET)) ; j++) {
                    size_t k = (j + T_ALLOC_VALUE[i]+1) % NB_TARGET;
                    if (RSP_FIFO_STATE[i][k] != 0) {
                        T_ALLOC_VALUE[i] = k;
                        T_ALLOC_STATE[i] = true;
                        break;
                    }
                } // end for
            }
        } else {
            size_t k = T_ALLOC_VALUE[i];
            int ptr = RSP_FIFO_PTR[i][k];
            if ((RSP_FIFO_STATE[i][k] != 0) && p_from_initiator[i].rspack.read()){
                global_rsp_temp[i] -= 1;
                rsp_temp[i][k] -= 1;
                RSP_FIFO_PTR[i][k] = (RSP_FIFO_PTR[i][k] + 1) % FIFO_DEPTH;

                if (((int)RSP_FIFO_CMD[i][k][ptr] & 0x00000008) == 0x00000008){
                    T_ALLOC_STATE[i] = false; //  desallocation
                }
            }
        } // end else
    } // end Loop 3

    ////////////////////////////////////////////////////////////
    //    LOOP 4
    // - Write I_ALLOC registers
    // - Read CMD FIFOs
    // The I_ALLOC FSM controls the target output port
    // allocation to a given initiator input port.
    // The allocation policy is round-robin.
    ////////////////////////////////////////////////////////////

    for ( size_t i=0 ; i<NB_TARGET ; i++) {

        if (I_ALLOC_STATE[i] == false) {
            if (GLOBAL_CMD_STATE[i] != 0) { // new allocation
                for ( size_t j = 0 ; (j < NB_INITIAT) ; j++) {
                    size_t k = (j + I_ALLOC_VALUE[i]+1) % NB_INITIAT;
                    if (CMD_FIFO_STATE[i][k] != 0) {
                        I_ALLOC_VALUE[i] = k;
                        I_ALLOC_STATE[i] = true;
                        break;
                    }
                } // end for
            }
        } else {
            size_t k = I_ALLOC_VALUE[i];
            int ptr = CMD_FIFO_PTR[i][k];
            if ((CMD_FIFO_STATE[i][k] != 0) && p_to_target[i].cmdack.read()){
                global_cmd_temp[i] -= 1;
                cmd_temp[i][k] -= 1;
                CMD_FIFO_PTR[i][k] = (CMD_FIFO_PTR[i][k] + 1) % FIFO_DEPTH;
                if (((int)CMD_FIFO_CMD[i][k][ptr] & 0x00004000) == 0x00004000) {
                    I_ALLOC_STATE[i] = false; //  desallocation
                }
            }
        } // end else
    } // end Loop 4

    ////////////////////////////////////////////////////////////
    //    LOOP 5
    // - Write FIFO RSP & FIFO CMD global states
    ////////////////////////////////////////////////////////////

    for ( size_t i=0 ; i<NB_INITIAT ; i++) {
        GLOBAL_RSP_STATE[i] = global_rsp_temp[i];
        for ( size_t j=0 ; j<NB_TARGET ; j++) {
            RSP_FIFO_STATE[i][j] = rsp_temp[i][j];
        }
    }

    for ( size_t i=0 ; i<NB_TARGET ; i++) {
        GLOBAL_CMD_STATE[i] = global_cmd_temp[i];
        for ( size_t j=0 ; j<NB_INITIAT ; j++) {
            CMD_FIFO_STATE[i][j] = cmd_temp[i][j];
        }
    }
}

tmpl(void)::genMoore()
{
    //////////////////////////////////
    //  loop on the initiator ports
    //////////////////////////////////

    for ( size_t i=0 ; i<NB_INITIAT ; i++) {

        size_t k        = T_ALLOC_VALUE[i];
        int ptr   = RSP_FIFO_PTR[i][k];
        int cmd   = RSP_FIFO_CMD[i][k][ptr];

        if ((T_ALLOC_STATE[i] == true) && (RSP_FIFO_STATE[i][k] !=0)) {
            p_from_initiator[i].rspval    = true;
            p_from_initiator[i].rerror    = (cmd    & 0x00000007);
            p_from_initiator[i].reop    = (bool)         (cmd>>3    & 0x00000001);
            p_from_initiator[i].rtrdid    = (cmd>>4 & 0x000000FF);
            p_from_initiator[i].rpktid    = (cmd>>12 & 0x0000000F);
            p_from_initiator[i].rsrcid    = (cmd>>16 & 0x0000FFFF);
            p_from_initiator[i].rdata    = RSP_FIFO_DATA[i][k][ptr].read();
        }else{
            p_from_initiator[i].rspval    = false;
        }

        int ptr2 = T_DELAY_PTR[i];

        int k2 = m_target_from_addr[T_DELAY_VCIADR[i][ptr2].read()];
        bool full = (CMD_FIFO_STATE[k2][i] == FIFO_DEPTH);
        bool valid = T_DELAY_VALID[i][ptr2];

        p_from_initiator[i].cmdack = !valid || !full;

    } // end loop initiators

    //////////////////////////////////
    //    loop on the target ports
    //////////////////////////////////

    for ( size_t j=0 ; j<NB_TARGET ; j++) {

        size_t k      = I_ALLOC_VALUE[j];
        int ptr    = CMD_FIFO_PTR[j][k];
        int cmd    = CMD_FIFO_CMD[j][k][ptr];
        int id     = CMD_FIFO_ID [j][k][ptr];

        if ((I_ALLOC_STATE[j] == true) && (CMD_FIFO_STATE[j][k] !=0)) {
            p_to_target[j].cmdval    = true;
            p_to_target[j].address = CMD_FIFO_ADR[j][k][ptr].read();
            p_to_target[j].wdata   = CMD_FIFO_DATA[j][k][ptr].read();
            p_to_target[j].be      = (cmd & 0xF);
            p_to_target[j].cmd     = (cmd>>4 & 0x3);
            p_to_target[j].plen    = (cmd>>6 & 0x3);
            p_to_target[j].eop     = (bool)         (cmd>>14 & 0x1);
            p_to_target[j].cons    = (bool)         (cmd>>15 & 0x1);
            p_to_target[j].contig  = (bool)         (cmd>>16 & 0x1);
            p_to_target[j].cfixed  = (bool)         (cmd>>17 & 0x1);
            p_to_target[j].wrap    = (bool)         (cmd>>18 & 0x1);
            p_to_target[j].clen    = (cmd>>19 & 0xFF);
            p_to_target[j].trdid   = (id & 0xFF);
            p_to_target[j].pktid   = (id>>8 & 0xF);
            p_to_target[j].srcid   = (id>>16 & 0xFFFF);
        }else{
            p_to_target[j].cmdval    = false;
        }

        int ptr2 = I_DELAY_PTR[j];
        int k2 = I_DELAY_VCICMD[j][ptr2] >> (16+m_initiator_from_srcid.getDrop());
        bool full = (RSP_FIFO_STATE[k2][j] == FIFO_DEPTH);
        bool valid = I_DELAY_VALID[j][ptr2];

        p_to_target[j].rspack = !valid || !full;

    } // end loop targets
}

tmpl(/**/)::VciVgmn(
    sc_module_name name,
    const soclib::common::MappingTable &mt
    )
    : soclib::caba::BaseModule(name),
      m_target_from_addr(mt.getRoutingTable(soclib::common::IntTab(), 0)),
      m_initiator_from_srcid(mt.getIdMaskingTable(0))
{
#if 0
    for ( size_t init=0;init<NB_INITIAT;init++) {
        for ( size_t target=0;target<NB_TARGET;target++) {
            for ( size_t depth=0;depth<FIFO_DEPTH;depth++) {
                SOCLIB_REG_RENAME_N3(RSP_FIFO_DATA, init, target, depth);
                SOCLIB_REG_RENAME_N3(RSP_FIFO_CMD, init, target, depth);
                SOCLIB_REG_RENAME_N3(CMD_FIFO_DATA, target, init, depth);
                SOCLIB_REG_RENAME_N3(CMD_FIFO_ADR, target, init, depth);
                SOCLIB_REG_RENAME_N3(CMD_FIFO_CMD, target, init, depth);
                SOCLIB_REG_RENAME_N3(CMD_FIFO_ID, target, init, depth);
            }
            SOCLIB_REG_RENAME_N2(RSP_FIFO_PTR, init, target);
            SOCLIB_REG_RENAME_N2(RSP_FIFO_PTW, init, target);
            SOCLIB_REG_RENAME_N2(RSP_FIFO_STATE, init, target);
            SOCLIB_REG_RENAME_N2(RSP_COUNTER, init, target);
            SOCLIB_REG_RENAME_N2(CMD_FIFO_PTR, target, init);
            SOCLIB_REG_RENAME_N2(CMD_FIFO_PTW, target, init);
            SOCLIB_REG_RENAME_N2(CMD_FIFO_STATE, target, init);
            SOCLIB_REG_RENAME_N2(CMD_COUNTER, target, init);
            SOCLIB_REG_RENAME_N2(I_DELAY_VCIDATA, target, init);
            SOCLIB_REG_RENAME_N2(I_DELAY_VCICMD, target, init);
            SOCLIB_REG_RENAME_N2(I_DELAY_VALID, target, init);
        }
        for ( size_t lat_n=0;lat_n<MIN_LATENCY;lat_n++) {
            SOCLIB_REG_RENAME_N2(T_DELAY_VCIDATA, init, lat_n);
            SOCLIB_REG_RENAME_N2(T_DELAY_VCIADR, init, lat_n);
            SOCLIB_REG_RENAME_N2(T_DELAY_VCICMD, init, lat_n);
            SOCLIB_REG_RENAME_N2(T_DELAY_VCIID, init, lat_n);
            SOCLIB_REG_RENAME_N2(T_DELAY_VALID, init, lat_n);
        }
        SOCLIB_REG_RENAME_N(T_ALLOC_VALUE, init);
        SOCLIB_REG_RENAME_N(T_ALLOC_STATE, init);
        SOCLIB_REG_RENAME_N(T_DELAY_PTR, init);
        SOCLIB_REG_RENAME_N(GLOBAL_RSP_STATE, init);
    }
    for ( size_t target=0; target<NB_TARGET; target++) {
        SOCLIB_REG_RENAME_N(I_ALLOC_VALUE, target);
        SOCLIB_REG_RENAME_N(I_ALLOC_STATE, target);
        SOCLIB_REG_RENAME_N(I_DELAY_PTR, target);
        SOCLIB_REG_RENAME_N(I_DELAY_ACK, target);
        SOCLIB_REG_RENAME_N(GLOBAL_CMD_STATE, target);
    }
#endif

    SC_METHOD(transition);
    dont_initialize();
    sensitive << p_clk.pos();

    SC_METHOD(genMoore);
    dont_initialize();
    sensitive << p_clk.neg();
}

}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
