#ifndef SOCLIB_CABA_PIBUS_SIGNALS_H_
#define SOCLIB_CABA_PIBUS_SIGNALS_H_

namespace soclib { namespace caba {

class Pibus
{
public:
	enum pibus_ack_type_e {
	ACK_WAT = 0x0,
	ACK_ERR = 0x1,
	ACK_RDY = 0x2,
	ACK_RTR = 0x3,
	};

	enum pibus_opc_type_e {
	OPC_NOP  = 0x0,
	OPC_WD32 = 0x1,	// 32 words burst
	OPC_WDU  = 0x2,	// undefined length
	OPC_WDC  = 0x3,	// ??
	OPC_WD2  = 0x4,	// 2 words burst
	OPC_WD4  = 0x5,	// 4 words burst
	OPC_WD8  = 0x6,	// 8 words burst
	OPC_WD16 = 0x7,	// 16 words burst
	OPC_HW0  = 0x8,	// lower half word
	OPC_HW1  = 0x9,	// upper half word
	OPC_TB0  = 0xA,	// ??         
	OPC_TB1  = 0xB,	// ??         
	OPC_BY0  = 0xC,	// byte 0     
	OPC_BY1  = 0xD,	// byte 1     
	OPC_BY2  = 0xE,	// byte 2     
	OPC_BY3  = 0xF,	// byte 3     
	};

	// signals
	sc_signal<sc_uint<4> >		opc;	// codop
	sc_signal<bool>			lock;	// burst transaction when true
	sc_signal<bool>			read;	// read transaction when true
	sc_signal<sc_uint<32> >		a;	// address
	sc_signal<sc_uint<2> >		ack;	// response
	sc_signal<sc_uint<32> >		d;	// data (bidirectionnal)
	sc_signal<bool>        		tout; 	// Time-Out

#define ren(x) x(((std::string)(name_ + "_"#x)).c_str())
	Pibus(std::string name_ = (std::string)sc_gen_unique_name("pibus"))
	  : ren(opc),
	  ren(lock),
	  ren(read),
	  ren(a),
	  ren(ack),
	  ren(d),
	  ren(tout)
	  {
	  }
#undef ren
};

}} // end namespace

#endif /* SOCLIB_CABA_PIBUS_SIGNALS_H_ */
