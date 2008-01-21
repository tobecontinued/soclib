// -*- c++ -*-

#include <systemc>
#include <stdint.h>
#include "common/inst/inst_arg.h"
#include "common/inst/factory.h"
#include "caba/inst/signal_wrapper"

namespace soclib { namespace common {

enum inst_mode_e {
	MODE_CABA,
	MODE_TLMT,
};

class TopCell
{
	struct cell_ref {
		enum inst_mode_e mode;
		std::string cell;
		std::string name;
		soclib::common::inst::InstArg *args;
	};

	sc_core::sc_clock m_clk;
	sc_core::sc_signal<bool> m_resetn;
	soclib::caba::inst::Signal<sc_core::sc_clock> m_wrapped_clk;
	soclib::caba::inst::Signal<sc_core::sc_signal<bool> > m_wrapped_resetn;
	soclib::common::inst::InstArg m_env;
	std::vector<struct cell_ref> m_cells;
	
	static void cleanup_args(soclib::common::inst::InstArg*);

	template<typename base_t>
	void cell_instanciate( const std::string &cell,
						   const std::string &name,
						   soclib::common::inst::InstArg *args );
	void instanciateAll();
	template<typename base_t>
	soclib::common::Factory<base_t> & get_factory( const std::string &cell );
	void finalize();
public:
	TopCell( const std::string &filename );
	void run();
	void reset();
	void run(const uint32_t ncycles);
	void prepare( enum inst_mode_e mode,
				  const std::string &cell,
				  const std::string &name,
				  soclib::common::inst::InstArg *args );
	static soclib::common::inst::InstArg* new_args();
	inline soclib::common::inst::InstArg &env() {return m_env;}
};

}}
