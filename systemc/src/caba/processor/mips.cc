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

#include "common/register.h"
#include "caba/processor/mips.h"

#define MIPS_DEBUG 0

namespace soclib { namespace caba {

static const uint32_t NO_EXCEPT = (uint32_t)-1;

static inline uint32_t align_data( uint32_t data, int shift, int width )
{
	uint32_t mask = (1<<width)-1;
	uint32_t ret = data;

	ret >>= shift*width;
	return ret & mask;
}

#define bypass()														\
do {																	\
        if ( mem_dest == ins.i.rs )										\
                gpr_rs = mem_data;										\
        else if ( mem_dest == ins.i.rt )								\
                gpr_rt = mem_data;										\
        gpr[mem_dest] = mem_data;										\
} while(0)


void Mips::transition()
{
	if ( ! p_resetn ) {
		mem_access = MEM_NONE;
		pc = RESET_ADDR;
		next_pc = RESET_ADDR+4;
		cp0_reg[CP0_IDENT] = id;
		cp0_reg[CP0_COUNT] = 0;
		cp0_reg[CP0_INFOS] = id|0x80000000;
		except = NO_EXCEPT;
		m_interrupt_delayed = NO_EXCEPT;
		return;
	}

	char cant_run_code = 0;

	except = NO_EXCEPT;

	if ( p_icache.berr ) {
		cant_run_code = 'I';
		except = X_IBE;
	} else if ( p_dcache.berr ) {
		cant_run_code = 'D';
		except = X_DBE;
		cp0_reg[CP0_BADVADDR] = mem_addr;
	}

	if ( !cant_run_code && (p_icache.frz || p_dcache.frz) )
		cant_run_code = 'F';

	if ( except == NO_EXCEPT && m_interrupt_delayed != NO_EXCEPT )
		except = m_interrupt_delayed.read();

	// Get interrupts
	uint32_t intrs = 0;
	for ( int i=0; i<6; ++i )
		intrs |= ((int)p_irq[i].read()) << (10+i);
	intrs &= cp0_reg[CP0_STATUS];

	if ( except == NO_EXCEPT &&
		 intrs &&
		 (cp0_reg[CP0_STATUS]&1) ) {
#if MIPS_DEBUG
		std::cout << "Interrupt on "<<name()<<": "<<intrs<<std::endl;
#endif
		except = X_INT;
	}

	future_next_pc = next_pc.read()+4;
	bool branching = false;
	if ( !cant_run_code ) {
		// Get ins and decode common fields
		ins.ins = p_icache.ins.read();
		gpr_rs = gpr[ins.i.rs];
		gpr_rt = gpr[ins.i.rt];

		// We may bypass data
		if ( mem_access ) {
			uint32_t data = p_dcache.rdata.read();
			char *type = NULL;
			switch (mem_access) {
			case MEM_LB:
				mem_data = (int32_t)(int8_t)align_data(data, mem_addr&0x3, 8);
				bypass();
				type = "LB";
				break;
			case MEM_LBU:
				mem_data = align_data(data, mem_addr&0x3, 8);
				bypass();
				type = "LBU";
				break;
			case MEM_LH:
				mem_data = (int32_t)(int16_t)align_data(data, (mem_addr&0x2)/2, 16);
				bypass();
				type = "LH";
				break;
			case MEM_LHU:
				mem_data = align_data(data, (mem_addr&0x2)/2, 16);
				bypass();
				type = "LHU";
				break;
			case MEM_LW:
				mem_data = data;
				bypass();
				type = "LW";
				break;
			}
#if MIPS_DEBUG
			if (type)
				std::cout
					<< "Mem " << type << ": " << mem_addr
					<< ", raw data: " << data
					<< " data: " << mem_data
					<< std::endl;
#endif
			// Now we can reset it
			mem_access = MEM_NONE;
		}

		// Default values, may be overridden by exec
		pc = next_pc;
		// Let's roll
		run();
		if ( next_pc != pc.read()+4 )
			branching = true;
#if MIPS_DEBUG
		std::cout
			<< std::endl
			<< std::hex
			<< name() << " PC: " << pc << " nextPC: " << next_pc << " fnextPC: " << future_next_pc
			<< (branching?" branching":"")
			<< std::endl
			<< " except: " << std::hex << except
			<< " status: " << cp0_reg[CP0_STATUS]
			<< std::endl;
		for ( int row=0; row<32; row+=8 ) {
			std::cout << " r";
			std::cout.width(2);
			std::cout << std::dec << row << "-";
			std::cout.width(2);
			std::cout <<row+7<<": ";
			for ( int col=row; col<row+8; ++col ) {
				std::cout.width(11);
				std::cout << std::hex << gpr[col];
			}
			std::cout << std::endl;
		}
#endif
	}
#if MIPS_DEBUG
	if ( cant_run_code )
		std::cout
			<< name() << cant_run_code
			<< " PC: " << pc << " nextPC: " << next_pc << " fnextPC: " << future_next_pc
			<< std::endl;
#endif

	if ( except != NO_EXCEPT ) {
		uint32_t cause = (except<<2) | intrs;
		uint32_t status = (
			(cp0_reg[CP0_STATUS] & ~0x3F) |
			((cp0_reg[CP0_STATUS] << 2) & 0x3C) |
			0x2 );
		uint32_t epc;

		bool imprecise_interrupt = (
			except == X_INT ||
			except == X_ADES ||
			except == X_DBE );

		m_interrupt_delayed = NO_EXCEPT;
		if ( imprecise_interrupt && branching && ! cant_run_code ) {
			// If we have both at the same time, we must delay the
			// interrupt a little, or we wont be able to return at the
			// right place.
			m_interrupt_delayed = except;
			except = NO_EXCEPT;
#if MIPS_DEBUG
			std::cout
				<< name()<<" delaying imprecise exception: "<<except<<std::endl
				<< " PC: " << pc << " nextPC: " << next_pc
				<< " fnextPC: " << future_next_pc << std::endl;
#endif
		} else if ( imprecise_interrupt && cant_run_code == 'F' && ! branching ) {
			cp0_reg[CP0_CAUSE] = cause;
			cp0_reg[CP0_STATUS] = status;
			cp0_reg[CP0_EPC] = pc;
			pc = EXCEPT_ADDR;
			future_next_pc = EXCEPT_ADDR+4;
#if MIPS_DEBUG
			std::cout
				<< name()<<" exception while frozen:"<<std::endl
				<< " EPC: " << pc
				<< " CAUSE: " << cause
				<< " STATUS: " << status
				<< std::endl;
#endif
		} else {
			if (imprecise_interrupt) {
				epc = future_next_pc;
			} else {
				epc = pc;
				if ( branching ) {
					// put jump ins addr, and add BD in CAUSE
					epc = pc-4;
					cause |= (uint32_t)0x80000000;
				}
			}
			cp0_reg[CP0_CAUSE] = cause;
			cp0_reg[CP0_STATUS] = status;
			cp0_reg[CP0_EPC] = epc;
#if MIPS_DEBUG
			std::cout
				<< name()<<" exception:"<<std::endl
				<< " EPC: " << epc
				<< " CAUSE: " << cause
				<< " STATUS: " << status
				<< std::endl;
#endif
			future_next_pc = EXCEPT_ADDR;
		}
	}
	if ( ! cant_run_code || except != NO_EXCEPT )
		next_pc = future_next_pc;
	cp0_reg[CP0_COUNT] = cp0_reg[CP0_COUNT]+1;
	gpr[0] = 0;
}

static const int dreq[] = {
	/* MEM_NONE*/   0,
	/* MEM_LB  */   caba::DCacheSignals::RW,
	/* MEM_LBU */   caba::DCacheSignals::RW,
	/* MEM_LH  */   caba::DCacheSignals::RW,
	/* MEM_LHU */   caba::DCacheSignals::RW,
	/* MEM_LW  */   caba::DCacheSignals::RW,
	/* MEM_SB  */   caba::DCacheSignals::WB,
	/* MEM_SH  */   caba::DCacheSignals::WH,
	/* MEM_SW  */   caba::DCacheSignals::WW,
	/* MEM_INVAL */ caba::DCacheSignals::RZ
};

void Mips::genMoore()
{
	p_icache.req = true;
	p_icache.adr = (unsigned long)pc;

	if (mem_access != MEM_NONE) {
		p_dcache.req = true;
		p_dcache.adr = mem_addr;
		p_dcache.wdata = mem_data;
		p_dcache.type = dreq[mem_access];
	} else
		p_dcache.req = false;
}

Mips::Mips( sc_module_name insname, int ident )
	: BaseModule(insname)
{
	id = ident;

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	dont_initialize();
	sensitive << p_clk.neg();

	for (size_t i=0; i<32; ++i ) {
		SOCLIB_REG_RENAME_N(gpr, (int)i);
		SOCLIB_REG_RENAME_N(cp0_reg, (int)i);
	}
	SOCLIB_REG_RENAME(pc);
	SOCLIB_REG_RENAME(next_pc);
	SOCLIB_REG_RENAME(hi);
	SOCLIB_REG_RENAME(lo);
	SOCLIB_REG_RENAME(m_interrupt_delayed);
}

Mips::~Mips()
{

}

}}
