#include <st231_isa.hh>
#include <unistd.h>

namespace st231 {

//using full_system::utils::debug::Symbol;
	
SystemResetException::SystemResetException()
{
}

const char * SystemResetException::what () const throw ()
{
	return "system reset exception";
}

MemException::MemException(const char *name, uint32_t addr)
{
	stringstream sstr;
	this->addr = addr;
	sstr.setf(ios::right | ios::hex | ios::showbase);
	sstr << "Mem " << name << " exception at " << hex << addr;
	what_str = sstr.str();
}

MemException::~MemException() throw()
{
}

const char * MemException::what () const throw ()
{
	return what_str.c_str();
}

uint32_t MemException::GetAddr() const
{
	return addr;
}

IllInstException::IllInstException()
{
}

const char * IllInstException::what () const throw ()
{
	return "Illegal instruction exception";
}

DBreakException::DBreakException(uint32_t addr) : MemException("Access to the data Break point", addr)
{
}

CRegAccessViolationException::CRegAccessViolationException(uint32_t addr) : MemException("Control register access violation", addr)
{
}

CRegNoMappingException::CRegNoMappingException(uint32_t addr) : MemException("Control register non mapping", addr)
{
}

MisAlignedException::MisAlignedException(uint32_t addr) : MemException("Address not aligned", addr)
{
}

DTLBException::DTLBException(uint32_t addr) : MemException("DTLB Error", addr)
{
}

} // end of namespace st231
