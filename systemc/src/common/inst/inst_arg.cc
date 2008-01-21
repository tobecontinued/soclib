
#include "common/exception.h"
#include "common/inst/inst_arg.h"

namespace soclib {
namespace common {
namespace inst {

InstArg::InstArg()
{
	
}

bool InstArg::has( const std::string &name ) const
{
	for ( std::vector<InstArgBaseItem*>::const_iterator i = m_items.begin();
		  i != m_items.end();
		  ++i ) {
		if ( (*i)->name() == name )
			return true;
	}
	return false;
}

InstArgBaseItem &InstArg::get( const std::string &name )
{
	for ( std::vector<InstArgBaseItem*>::iterator i = m_items.begin();
		  i != m_items.end();
		  ++i ) {
		if ( (*i)->name() == name )
			return **i;
	}
	throw soclib::exception::RunTimeError(
		std::string("Cant find item `")+name+
		"' in environment");
}

void InstArg::add( InstArgBaseItem*item )
{
	m_items.push_back( item );
}

}}}
