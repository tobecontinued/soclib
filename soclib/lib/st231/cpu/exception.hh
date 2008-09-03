#ifndef __FS_PROCESSORS_ST231_EXCEPTION_HH__
#define __FS_PROCESSORS_ST231_EXCEPTION_HH__

#include <sstream>

namespace st231 {
//using namespace std;

class Exception : public std::exception {};

class SystemResetException : public Exception
{
public:
	SystemResetException();
	virtual const char * what () const throw ();
};

class MemException : public Exception
{
public:
	MemException(const char *name, uint32_t addr);
	virtual ~MemException() throw();
	virtual const char * what () const throw ();
	uint32_t GetAddr() const;
private:
	uint32_t addr;
	std::string what_str;
};

class IllInstException : public Exception
{
public:
	IllInstException();
	virtual const char * what () const throw ();
};

class DBreakException : public MemException
{
public:
	DBreakException(uint32_t addr);
};

class CRegAccessViolationException : public MemException
{
public:
	CRegAccessViolationException(uint32_t addr);
};

class CRegNoMappingException : public MemException
{
public:
	CRegNoMappingException(uint32_t addr);
};

class MisAlignedException : public MemException
{
public:
	MisAlignedException(uint32_t addr);
};

class DTLBException : public MemException
{
public:
	DTLBException(uint32_t addr);
};

} // end of namespace st231

#endif
