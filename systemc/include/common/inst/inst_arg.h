// -*- c++ -*-
#ifndef COMMON_ENVIRONMENT_H
#define COMMON_ENVIRONMENT_H

#include "common/exception.h"
#include <string>
#include <vector>
#include <typeinfo>

namespace soclib {
namespace common {
namespace inst {

class InstArgBaseItem
{
	const std::string m_name;
public:
	InstArgBaseItem( const std::string &name )
		: m_name(name)
	{}
	const std::string &name() const
	{
		return m_name;
	}
	virtual ~InstArgBaseItem() {}
};

template<typename type_t>
class InstArgItem
	: public InstArgBaseItem
{
	type_t m_item;
public:
	InstArgItem( const std::string &name, type_t item )
		: InstArgBaseItem(name), m_item(item)
	{}
	type_t &operator*()
	{
		return m_item;
	}
};

class InstArg
{
	std::vector<InstArgBaseItem*> m_items;
public:
	InstArg();
	InstArgBaseItem &get( const std::string &name );
	bool has( const std::string &name ) const;
	void add( InstArgBaseItem* );
	template<typename T> void add( const std::string &name, T data )
	{
		add( new InstArgItem<T>( name, data ) );
	}
	template<typename T> T& get( const std::string &name )
	{
		typedef InstArgItem<T> item_t;
		InstArgBaseItem &a = get(name);
		if ( item_t *item = dynamic_cast<item_t*>(&a) )
			return **item;
		const std::type_info& asked = typeid(item_t);
		const std::type_info& got = typeid(a);
		throw soclib::exception::RunTimeError(
			std::string("Trying to get an incompatible type when getting `")
			+name+"': asked for "+asked.name()+" got "+got.name());
	}
	template<typename T> T& operator[]( const std::string &name )
	{
		return get<T>(name);
	}
};

}}}

#endif
