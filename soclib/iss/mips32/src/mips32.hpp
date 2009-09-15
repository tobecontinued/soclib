#ifndef MIPS32_HPP
#define MIPS32_HPP

#include "mips32.h"
#include "arithmetics.h"

namespace soclib { namespace common {

void Mips32Iss::jump_imm16(bool taken, bool likely)
{
	if ( !likely ) {
		if ( taken )
			m_jump_pc = sign_ext(m_ins.i.imd, 16)*4 + r_npc;
	} else {
		if ( taken ) {
			m_next_pc = sign_ext(m_ins.i.imd, 16)*4 + r_npc;
			m_jump_pc = m_next_pc+4;
		} else {
			m_next_pc = r_npc+4;
			m_jump_pc = m_next_pc+4;
		}
	}
}

bool Mips32Iss::check_irq_state() const
{
    return (m_microcode_func == NULL)
        && r_status.ie
        && !r_status.exl
        && !r_status.erl;
}



}}

#endif
