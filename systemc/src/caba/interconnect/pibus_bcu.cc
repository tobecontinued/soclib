//////////////////////////////////////////////////////////////////////////
// File : pibus_bcu.cc
// Date : 20/07/2006
// author :  Alain Greiner 
// Copyright : UPMC - LIP6
// This program is released under the GNU general public license
//////////////////////////////////////////////////////////////////////////

#include "caba/interconnect/pibus_bcu.h"
#include "common/register.h"

#define Pibus soclib::caba::Pibus

namespace soclib { namespace caba {

///////////////////////////////////////
//	constructor
///////////////////////////////////////

PibusBcu::PibusBcu (	sc_module_name 				name,
                        const soclib::common::MappingTable 	&maptab,
                        size_t 					nb_master,
                        size_t 					nb_slave,
                        uint32_t 				time_out)
	: soclib::caba::BaseModule(name)
{
	m_target_table 	= maptab.getRoutingTable(soclib::common::IntTab());
	m_time_out	= time_out;
	m_nb_master	= nb_master;
	m_nb_target	= nb_slave;
	
	// The number of p_req, p_gnt, p_sel ports and the number of counters
	// depend on the m_nb_master & m_nb_target parameters
	p_req		= new 	sc_in<bool>[m_nb_master];
	p_gnt		= new 	sc_out<bool>[m_nb_master];
	p_sel		= new 	sc_out<bool>[m_nb_target];
	r_req_counter 	= new	sc_signal<size_t>[m_nb_master];
	r_wait_counter 	= new	sc_signal<uint32_t>[m_nb_master];

	SC_METHOD(transition);
	dont_initialize();
	sensitive << p_clk.pos();

	SC_METHOD(genMoore);
	sensitive << p_clk.neg();

	SC_METHOD(genMealy_sel);
	sensitive << p_clk.neg();
	sensitive << p_a;

	SC_METHOD(genMealy_gnt);
	sensitive << p_clk.neg();
	for (size_t i = 0 ; i < m_nb_master; i++)
        sensitive << p_req[i];

	if (!m_target_table.isAllBelow( m_nb_target )) 
		throw soclib::exception::ValueError(
           "At least one target index is larger than the number of targets");
  
	SOCLIB_REG_RENAME(r_fsm_state);
	SOCLIB_REG_RENAME(r_current_master);
	SOCLIB_REG_RENAME(r_tout_counter);
	for (size_t i=0; i<m_nb_master; ++i) {
		SOCLIB_REG_RENAME_N(r_wait_counter, (int)i);
		SOCLIB_REG_RENAME_N(r_req_counter, (int)i);
	}
}

PibusBcu::~PibusBcu()
{
    delete [] p_req;
    delete [] p_gnt;
    delete [] p_sel;
    delete [] r_req_counter;
    delete [] r_wait_counter;
}

/////////////////////////////////////
//	transition()
/////////////////////////////////////
void PibusBcu::transition()
{
    if (p_resetn == false) {
        r_fsm_state = FSM_IDLE;
        r_current_master = 0;
        for(size_t i = 0 ; i < m_nb_master ; i++) {
            r_req_counter[i] = 0;
            r_wait_counter[i] = 0;
        }
        return;
    } // end p_resetn

    for(size_t i = 0 ; i < m_nb_master ; i++) {
        if(p_req[i])
            r_wait_counter[i] = r_wait_counter[i] + 1;
	}
	
    switch(r_fsm_state) {
	case FSM_IDLE:
        r_tout_counter = m_time_out;
        for(size_t i = 0 ; i < m_nb_master ; i++) {
            int j = (i + 1 + r_current_master) % m_nb_master;
            if(p_req[j]) {
                r_current_master = j;
                r_req_counter[j] = r_req_counter[j] + 1;
                r_fsm_state = FSM_AD;
                break;
            }
        } // end for
        break;

	case FSM_AD:
        if(p_lock)   r_fsm_state = FSM_DTAD;  
        else	     r_fsm_state = FSM_DT; 
        break;

	case FSM_DTAD:
        if(r_tout_counter == 0) {
            r_fsm_state = FSM_IDLE;
        } else if(p_ack.read() == Pibus::ACK_RDY) {
            if(p_lock == false) { r_fsm_state = FSM_DT; } 
        } else if((p_ack.read() == Pibus::ACK_ERR) || (p_ack.read() == Pibus::ACK_RTR)) {
            r_fsm_state = FSM_IDLE;
        } else { 
            r_tout_counter = r_tout_counter - 1;
        }
        break;

	case FSM_DT:
        if(r_tout_counter == 0) {
            r_fsm_state = FSM_IDLE;
        } else if(p_ack.read() != Pibus::ACK_WAT) { // new allocation
            r_tout_counter = m_time_out;
            bool found = false;
            for(size_t i = 0 ; i < m_nb_master ; i++) {
                int j = (i + 1 + r_current_master) % m_nb_master;
                if((p_req[j] == true) && (found == false)) {
                    r_current_master = j;
                    r_req_counter[j] = r_req_counter[j] + 1;
                    found = true;
                    break;
                }
            } // end for
            if(found == true)
                r_fsm_state = FSM_AD; 
            else
                r_fsm_state = FSM_IDLE; 
        } else { 
            r_tout_counter = r_tout_counter - 1;
        }
        break;
    } // end switch FSM
}

//////////////////////////////////////////////////////:
//	genMealy_gnt()
//////////////////////////////////////////////////////:
void PibusBcu::genMealy_gnt()
{
    bool	found = false;
    if((r_fsm_state == FSM_IDLE) || 
       ((r_fsm_state == FSM_DT) && (p_ack.read() != Pibus::ACK_WAT))) {
        for(size_t i = 0 ; i < m_nb_master ; i++) {
            int j = (i + 1 + r_current_master) % m_nb_master;
            if((p_req[j] == true) && (found == false)) {
                p_gnt[j] = true;
                found = true;
            } else {
                p_gnt[j] = false;
            }
        } // end for
    } else {
        for (size_t i = 0 ; i < m_nb_master ; i++) {
            p_gnt[i] = false;
        } // end for
    } // end if
}

//////////////////////////////////////////////////////:
//	genMealy_sel()
//////////////////////////////////////////////////////:
void PibusBcu::genMealy_sel()
{
    if((r_fsm_state == FSM_AD) || (r_fsm_state == FSM_DTAD)) {
        size_t index = m_target_table[p_a.read()];
        for(size_t i = 0; i < m_nb_target ; i++) {
            if(i == index)  	p_sel[i] = true;
            else			p_sel[i] = false;
        } // end for
    } else {
        for(size_t i = 0 ; i < m_nb_target ; i++) {
            p_sel[i] = false;
        } // end for
    } // end if
} // end genMealy_sel()

//////////////////////////////////////////////////////:
//	genMoore
//////////////////////////////////////////////////////:
void PibusBcu::genMoore()
{
	p_tout = (r_tout_counter == 0);
}  // end genMoore 

}} // end namespace


// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
