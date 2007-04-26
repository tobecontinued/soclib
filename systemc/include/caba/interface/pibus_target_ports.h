#ifndef SOCLIB_CABA_PIBUS_TARGET_PORTS_H_
#define SOCLIB_CABA_PIBUS_TARGET_PORTS_H_

#include "caba/interface/pibus_signals.h"

namespace soclib { namespace caba {

class PibusTarget
{
public:
	sc_in<sc_uint<32> >		a;	// address
	sc_in<sc_uint<4> >		opc;	// codop
	sc_in<bool>			read;	// read transaction
	sc_in<bool>			lock;	// burst transaction
	sc_out<sc_uint<2> >		ack;	// response code
	sc_inout<sc_uint<32> >		d;	// data
	sc_in<bool>			tout;	// time_out

        void operator() (Pibus &sig)
        {
                a       (sig.a);
                opc     (sig.opc);
                read    (sig.read);
                lock    (sig.lock);
                ack     (sig.ack);
                d       (sig.d);
                tout    (sig.tout);
        }

};

}} // end namespace

#endif // SOCLIB_CABA_PIBUS_TARGET_PORTS_H_ 
