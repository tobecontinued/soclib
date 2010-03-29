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

#ifndef _SOCLIB_ARM_ISS_H_
#define _SOCLIB_ARM_ISS_H_

#include "static_assert.h"
#include "iss2.h"
#include "soclib_endian.h"
#include "register.h"

namespace soclib { namespace common {

#define ARM_OPS_PROTO(n) void n()

class ArmIss
    : public Iss2
{
	typedef uint32_t addr_t;
	typedef uint32_t data_t;

public:

    void dump() const;

    inline void setWriteBerr()
    {
        m_data_error = true;
    }

    ArmIss( const std::string &name, uint32_t cpuid );

    void reset();

    void getRequests( struct InstructionRequest &,
                      struct DataRequest & ) const;

    uint32_t executeNCycles(
        uint32_t ncycle,
        const struct InstructionResponse &,
        const struct DataResponse &,
        uint32_t irq_bit_field );

    inline unsigned int debugGetRegisterCount() const
    {
        return 26;
    }

    virtual debug_register_t debugGetRegisterValue(unsigned int reg) const;

    virtual void debugSetRegisterValue(unsigned int reg, debug_register_t value)
    {
        if ( reg == 25 )
            r_cpsr.whole = value;
        
        if ( reg <= 15 )
            r_gp[reg] = value;
    }

    inline size_t debugGetRegisterSize(unsigned int reg) const
    {
        return 32;
    }

    static const unsigned int s_sp_register_no = 13;
    static const unsigned int s_fp_register_no = 11;
    static const unsigned int s_pc_register_no = 15;
    static const Iss2::debugCpuEndianness s_endianness = Iss2::ISS_LITTLE_ENDIAN;

    static const size_t n_irq = 2;

    void setCacheInfo( const struct CacheInfo &info );

private:

    enum {
        ARM_RESET_ADDR = 0
    };

    REG32_BITFIELD(
        uint32_t zero:3,
        uint32_t ctype:4,
        uint32_t separated:1,
        uint32_t dcache_p:1,
        uint32_t dcache_zero:1,
        uint32_t dcache_size:4,
        uint32_t dcache_assoc:3,
        uint32_t dcache_mult:1,
        uint32_t dcache_len:2,
        uint32_t icache_p:1,
        uint32_t icache_zero:1,
        uint32_t icache_size:4,
        uint32_t icache_assoc:3,
        uint32_t icache_mult:1,
        uint32_t icache_len:2,
        ) m_cache_info;

	typedef void (ArmIss::*func_t)();

    typedef int8_t (*decod_func_t)(data_t);

    template<size_t byte_count, bool pre, bool load, bool signed_> void op_ldstrh();
    template<bool reg, bool pre, bool load> void op_ldstr();
    static const uint16_t cond_table[16];

# include "arm_instruction_formats.inc"
# include "arm_ops.inc"

	typedef union {
		REG32_BITFIELD(
			uint32_t sign:1,
			uint32_t zero:1,
			uint32_t carry:1,
			uint32_t overflow:1,
			uint32_t reserved1:20,
			uint32_t irq_disabled:1,
			uint32_t fiq_disabled:1,
			uint32_t reserved2:1,
			uint32_t mode:5,
			);
		PACKED_BITFIELD(
			uint32_t flags:4,
			uint32_t unused0:28,
			);
	} psr_t;

	/** Arm execution mode */
	enum ArmMode {
		MOD_USER32,
		MOD_FIQ32,
		MOD_IRQ32,
		MOD_SUPER32,
		MOD_ABORT32,
		MOD_UNDEF32,
		MOD_Count,
	};

	enum ArmPsrMode {
		MOD_PSR_USER32 = 0x10,
		MOD_PSR_FIQ32 = 0x11,
		MOD_PSR_IRQ32 = 0x12,
		MOD_PSR_SUPER32 = 0x13,
		MOD_PSR_ABORT32 = 0x17,
		MOD_PSR_UNDEF32 = 0x1b,
	};

    enum exception_e {
        EXCEPT_NONE = 0,
        EXCEPT_UNDEF = 1,
        EXCEPT_SWI = 2,
        EXCEPT_FIQ = 3,
        EXCEPT_IRQ = 4,
        EXCEPT_PABT = 5,
        EXCEPT_DABT = 6,
        EXCEPT_Count = 7,
    };

	ins_t m_opcode;
	static const ArmMode psr_to_mode[32];
	static const ArmPsrMode mode_to_psr[MOD_Count];
    struct except_info_s {
        bool disable_fiq;
        ArmPsrMode new_mode;
        addr_t vector_address;
        addr_t return_offset;
    };
    static const struct except_info_s except_info[EXCEPT_Count];

	/** genral pupose register set */
	data_t r_gp[16];

	/** gp reg save slices */
	data_t r_r13_r14[MOD_Count][2];
	data_t r_r8_r12[2][5];
    
    /** tls registers */
	data_t m_tls_regs[3];

	/** program status rgisters */
	psr_t r_cpsr;
	psr_t r_spsr[MOD_Count];

    addr_t m_exception_pc;
    addr_t m_exception_dptr;

    enum exception_e m_exception;

    addr_t m_current_pc;
    uint32_t m_cycle_count;
    uint32_t m_run_count;
	bool m_instruction_asked;

	bool m_ins_error;
	bool m_data_error;


	/** switch register bank on mode switch */
	void cpsr_update(psr_t new_psr);

	/* microcoded part */
	void do_microcoded_swp_ll();
	void do_microcoded_swp_sc();
	void do_microcoded_swp_decide();
	void do_microcoded_ldstm();

	union {
		struct {
			addr_t address;
			data_t tmp_data;
		} swp;
		struct {
			addr_t base_address;
			addr_t sp_value;
		} bdt;
	} m_microcode_status;
	ins_t m_microcode_opcode;
	func_t m_microcode_func;


	enum post_memaccess_op_e {
		POST_OP_NONE,
		POST_OP_WB_UNSIGNED,
		POST_OP_WB_SIGNED,
		POST_OP_WB_SWAP_HALFWORDS,
		POST_OP_WB_SC,
	};

	void do_mem_access(
		addr_t address,
		enum DataOperationType operation,
		int byte_count,
		data_t wdata,
		data_t *rdata_dest,
		enum post_memaccess_op_e post_op
		);


	bool cond_eval() const;
    void run();
    void do_microcoded_ldstm_user();
    void do_sleep();

    data_t x_get_rot() const;

    bool coproc_put(
        unsigned int coproc,
        unsigned int crn,
        unsigned int opcode1,
        unsigned int crm,
        unsigned int opcode2,
        data_t val );
    bool coproc_get(
        unsigned int coproc,
        unsigned int crn,
        unsigned int opcode1,
        unsigned int crm,
        unsigned int opcode2,
        data_t &val );

    bool m_irq_in;
    bool m_fiq_in;

    static const char *decode_instr( uint32_t ins );

	template <bool update_carry> uint32_t arm_shifter();
	template <bool update_carry> uint32_t arm_shifter_shift();


	struct DataRequest m_dreq;
    bool handle_data_response( const struct DataResponse & );
	bool m_dbe;
	bool m_dreq_ok;
	enum Iss2::ExecMode r_bus_mode;
    addr_t r_mem_byte_le;
    size_t r_mem_byte_count;
    data_t *r_mem_dest_addr;
    enum post_memaccess_op_e r_mem_post_op;

    soclib_static_assert(sizeof(ins_t) == 4);
};

}}

#endif

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
