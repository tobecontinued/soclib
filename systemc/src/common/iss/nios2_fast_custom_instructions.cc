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
 * NIOSII Instruction Set Simulator for the Altera NIOSII processor core
 * developed for the SocLib Projet
 * 
 * Copyright (C) IRISA/INRIA, 2007-2008
 *         François Charot <charot@irisa.fr>
 *
 * Contributing authors:
 * 				Delphine Reeb
 * 				François Charot <charot@irisa.fr>
 * 
 * Maintainer: charot
 *
 * History:
 * - summer 2006: First version developed on a first SoCLib template by Reeb, Charot.
 * - september 2007: the model has been completely rewritten and adapted to the SocLib 
 * 						rules defined during the first months of the SocLib ANR project
 *
 * Functional description:
 * Four files: 
 * 		nios2_fast.h
 * 		nios2_ITypeInst.cc
 * 		nios2_RTypeInst.cc
 * 		nios2_customInst.cc
 * define the Instruction Set Simulator for the NIOSII processor.
 *
 * 
 */

#include "common/iss/nios2_fast.h"

namespace soclib {
namespace common {

#define ci(x) &Nios2fIss::custom_##x
#define ci4(x, y, z, t) ci(x), ci(y), ci(z), ci(t)
Nios2fIss::func_t const Nios2fIss::customInstTable[] = { ci4(ill, addsf3, subsf3, mulsf3), 
      ci4(divsf3,   minsf3,     maxsf3,     negsf2), 
      ci4(abssf2,   sqrtsf2,    cossf2,     sinsf2), 
      ci4(tansf2,   atansf2,    expsf2,     logsf2), 
      ci4(fcmplts,  fcmples,    fcmpgts,    fcmpges), 
      ci4(fcmpeqs,  fcmpnes,    floatsisf2, floatunsisf2), 
      ci4(fixsfsi2, fixunsfsi2, ill,        ill), };
#undef ci

#if DEBUGCustom
#define ci(x) #x
		static const char *customNameTable[] = {
			ci4(ill, addsf3, subsf3, mulsf3),
			ci4(divsf3, minsf3, maxsf3, negsf2),
			ci4(abssf2, sqrtsf2, cossf2, sinsf2),
			ci4(tansf2, atansf2, expsf2, logsf2),
			ci4(fcmplts, fcmples, fcmpgts, fcmpges),
			ci4(fcmpeqs, fcmpnes, floatsisf2, floatunsisf2),
			ci4(fixsfsi2, fixunsfsi2, ill, ill),
		};
#endif

		//custom p.8-49
void Nios2fIss::op_custom()
		{
			// opx field carrries readra, readrb, m_writerc information
		m_readra = (bool) (m_instruction.r.opx >> 5) & 1;
		m_readrb = (bool) (m_instruction.r.opx >> 4) & 1;
		m_writerc = (bool) (m_instruction.r.opx >> 3) & 1;
#if DEBUGCustom
		std::cout << "CU - m_readra: " << m_readra << " m_readrb: " << m_readrb<< " m_writerc: " << m_writerc << std::endl;	
#endif
      //opx carries part of N field (3 bits)
      //this part has to be concatenenated to sh field
      uint32_t nField = (uint32_t(m_instruction.r.opx & 0x07) << 3) | (uint32_t)m_instruction.r.sh;
#if DEBUGCustom
      std::cout << "CU - N: " << nField << std::endl;
#endif
 	
      func_t custom = customInstTable[nField];
      (this->*custom)();
 	
#if DEBUGCustom
      std::cout << " execution of " << customNameTable[nField] << " custom instruction " << std::endl;
#endif 	
    }
    
    void  Nios2fIss::custom_ill()
    {
    }
         
    void  Nios2fIss::custom_addsf3()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f + m_operandB.f ;
#if DEBUGCustom
      printf("operand of addsf3 operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("addsf3 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }


    void  Nios2fIss::custom_subsf3()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f - m_operandB.f ;
#if DEBUGCustom
      printf("operand of subsf3 operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("subsf3 result: %f (%x) \n", resultCustomm_instruction.f, resultCustomm_instruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_mulsf3()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f * m_operandB.f ;
#if DEBUGCustom
      printf("operand of mulsf3 operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("mulsf3 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i; 
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }
  
    void  Nios2fIss::custom_divsf3()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f / m_operandB.f ;
#if DEBUGCustom
      printf("operand of divsf3 operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("divsf3 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_minsf3()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = (m_operandA.f < m_operandB.f) ? m_operandA.f : m_operandB.f ;
#if DEBUGCustom
      printf("operand of minsf3 operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("minsf3 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }
   
    void  Nios2fIss::custom_maxsf3()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = (m_operandA.f > m_operandB.f) ? m_operandA.f : m_operandB.f ;
#if DEBUGCustom
      printf("operand of maxsf3 operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("maxsf3 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_negsf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = -m_operandA.f;
#if DEBUGCustom
      printf("operand of negsf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("negsf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_abssf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = fabs(m_operandA.f);
#if DEBUGCustom
      printf("operand of abssf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("abssf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_sqrtsf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = sqrt(m_operandA.f);
#if DEBUGCustom
      printf("operand of sqrtsf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("sqrtsf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_cossf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = cos(m_operandA.f);
#if DEBUGCustom
      printf("operand of cossf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("cossf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_sinsf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = sin(m_operandA.f);
#if DEBUGCustom
      printf("operand of sinsf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("sinsf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_tansf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = tan(m_operandA.f);
#if DEBUGCustom
      printf("operand of tansf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("tansf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_atansf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = atan(m_operandA.f);
#if DEBUGCustom
      printf("operand of atansf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("atansf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_expsf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = exp(m_operandA.f);
#if DEBUGCustom
      printf("operand of expsf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("expsf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_logsf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = log(m_operandA.f);
#if DEBUGCustom
      printf("operand of logsf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("logsf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_fcmplts()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f < m_operandB.f  ? 1 : 0;
#if DEBUGCustom
      printf("operand of fcmplts operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("fcmplts result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_fcmples()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f <= m_operandB.f  ? 1 : 0;
#if DEBUGCustom
      printf("operand of fcmples operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("fcmples result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_fcmpgts()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f > m_operandB.f  ? 1 : 0;
#if DEBUGCustom
      printf("operand of fcmpgts operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("fcmpgts result: %f (%x) \n", m_resultCustomInstruction.f, resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_fcmpges()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f >= m_operandB.f  ? 1 : 0;
#if DEBUGCustom
      printf("operand of fcmpges operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("fcmpges result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_fcmpeqs()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f == m_operandB.f  ? 1 : 0;
#if DEBUGCustom
      printf("operand of fcmpeqs operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("fcmpeqs result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    void  Nios2fIss::custom_fcmpnes()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_operandB.i = m_readrb ? m_gprB : r_cr[m_instruction.r.b];     
      m_resultCustomInstruction.f = m_operandA.f != m_operandB.f  ? 1 : 0;
#if DEBUGCustom
      printf("operand of fcmpnes operation: (%f,%x) (%f,%x)\n", m_operandA.f, m_operandA.i, m_operandB.f, m_operandB.i);
      printf("fcmpnes result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    // int to float conversion
    void  Nios2fIss::custom_floatsisf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = float(m_operandA.i);
#if DEBUGCustom
      printf("operand of floatsisf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("floatsisf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    // unsigned to float conversion
    void  Nios2fIss::custom_floatunsisf2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = float(m_operandA.i);
#if DEBUGCustom
      printf("operand of floatunsisf2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("floatunsisf2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    // float to int conversion
    void  Nios2fIss::custom_fixsfsi2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = m_operandA.f;
#if DEBUGCustom
      printf("operand of fixsfsi2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("fixsfsi2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }

    // float to unsigned conversion
    void  Nios2fIss::custom_fixunsfsi2()
    {
      m_operandA.i = m_readra ? m_gprA : r_cr[m_instruction.r.a];     
      m_resultCustomInstruction.f = m_operandA.f;
#if DEBUGCustom
      printf("operand of fixunsfsi2 operation: (%f,%x) \n", m_operandA.f, m_operandA.i);
      printf("fixunsfsi2 result: %f (%x) \n", m_resultCustomInstruction.f, m_resultCustomInstruction.i);
#endif
      if (m_writerc)
	r_gpr[m_instruction.r.c] = m_resultCustomInstruction.i;
      else
	r_cr[m_instruction.r.c] = m_resultCustomInstruction.i;
	
	  // late result management
	  m_listOfLateResultInstruction.add(m_instruction.r.c);
    }
  }
}


