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
 * Copyright (c) UPMC, Lip6
 *         Alexandre Becoulet <alexandre.becoulet@free.fr>, 2009
 *         Nicolas Pouillon <nipo@ssji.net>, 2009
 *
 * Maintainers: nipo becoulet
 *
 * $Id$
 *
 */

static const char * const func_names[87];
static decod_func_t const decod_funcs[18];
static func_t const funcs[87];
static int8_t const decod_table_0000XXX00001[8];
static int8_t const decod_table_0000XXX00XX0[32];
static int8_t const decod_table_0000XXX01XX1[32];
static int8_t const decod_table_0000XXX10000[8];
static int8_t const decod_table_0000XXX11XX1[32];
static int8_t const decod_table_0001XXX00XX0[32];
static int8_t const decod_table_0001XXX00XX1[32];
static int8_t const decod_table_0001XXX010X0[16];
static int8_t const decod_table_0001XXX01XX1[32];
static int8_t const decod_table_0001XXX10000[8];
static int8_t const decod_table_0001XXX11XX1[32];
static int8_t const decod_table_0011XXX00000[8];
static int8_t const decod_table_01101X110X11[4];
static int8_t const decod_table_01101X111011[2];
static int8_t const decod_table_01101XX00111[4];
static int8_t const decod_table_0110X_X10_X1[8];
static int8_t const decod_table_0110X_X11XX1[16];
static int8_t const decod_table_0110X__00XX1[8];
static int8_t const decod_table_main[128];
static int8_t decod_0000XXX00001(data_t opcode);
static int8_t decod_0000XXX00XX0(data_t opcode);
static int8_t decod_0000XXX01XX1(data_t opcode);
static int8_t decod_0000XXX10000(data_t opcode);
static int8_t decod_0000XXX11XX1(data_t opcode);
static int8_t decod_0001XXX00XX0(data_t opcode);
static int8_t decod_0001XXX00XX1(data_t opcode);
static int8_t decod_0001XXX010X0(data_t opcode);
static int8_t decod_0001XXX01XX1(data_t opcode);
static int8_t decod_0001XXX10000(data_t opcode);
static int8_t decod_0001XXX11XX1(data_t opcode);
static int8_t decod_0011XXX00000(data_t opcode);
static int8_t decod_01101X110X11(data_t opcode);
static int8_t decod_01101X111011(data_t opcode);
static int8_t decod_01101XX00111(data_t opcode);
static int8_t decod_0110X_X10_X1(data_t opcode);
static int8_t decod_0110X_X11XX1(data_t opcode);
static int8_t decod_0110X__00XX1(data_t opcode);
static int8_t decod_main(data_t opcode);
void op_ill();
void op_swp();
void op_adc();
void op_adcs();
void op_add();
void op_adds();
void op_and();
void op_ands();
void op_bic();
void op_bics();
void op_eor();
void op_eors();
void op_mov();
void op_movs();
void op_mvn();
void op_mvns();
void op_orr();
void op_orrs();
void op_rsb();
void op_rsbs();
void op_rsc();
void op_rscs();
void op_sbc();
void op_sbcs();
void op_sub();
void op_subs();
void op_cmns();
void op_cmps();
void op_tsts();
void op_teqs();
void op_b();
void op_bl();
void op_cdp();
void op_ldc();
void op_ldstm();
void op_mcr();
void op_mla();
void op_mrc();
void op_mrs();
void op_msr();
void op_mul();
void op_smlal();
void op_smull();
void op_stc();
void op_swi();
void op_umull();
void op_umlal();
void op_bx();
void op_blx();
void op_bkpt();
void op_clz();
void op_smla_xy();
void op_smlaw_y();
void op_smulw_y();
void op_smul_xy();
void op_strex();
void op_ldrex();
void op_rev();
void op_rev16();
void op_revsh();
void op_sxtb16();
void op_uxtb16();
void op_sxtb();
void op_uxtb();
void op_sxth();
void op_uxth();
void op_umaal();


// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4