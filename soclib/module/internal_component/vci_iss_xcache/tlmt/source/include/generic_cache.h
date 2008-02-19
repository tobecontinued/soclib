#ifndef GENERIC_CACHE_H
#define GENERIC_CACHE_H

#include "arithmetics.h"
#include "mapping_table.h"

namespace soclib { namespace tlmt {

template<typename vci_param>
class genericCache 
{

private:
	typename vci_param::data_t**   m_data ;
	typename vci_param::addr_t*    m_tag ;
	bool*                          m_val ;
	soclib::common::AddressMaskingTable<uint32_t>  m_x ;
	soclib::common::AddressMaskingTable<uint32_t>  m_y ;
	soclib::common::AddressMaskingTable<uint32_t>  m_z ;
	uint32_t m_words;
	uint32_t m_lines;	
	uint32_t m_yzmask;	

public:
///////// Constructor ////////////////
genericCache( uint32_t nlines, uint32_t nwords ) :
    m_x( soclib::common::uint32_log2(nwords), 2 ),
    m_y( soclib::common::uint32_log2(nlines), 2+soclib::common::uint32_log2(nwords) ),
    m_z( 32-soclib::common::uint32_log2(nwords)-soclib::common::uint32_log2(nlines)-2, 
    soclib::common::uint32_log2(nwords)+soclib::common::uint32_log2(nlines)+2 ),
    m_yzmask((~0)<<(soclib::common::uint32_log2(nwords) + soclib::common::uint32_log2(4)))
{
    m_data = new typename vci_param::data_t*[nlines] ;
    for ( uint32_t i = 0 ; i < nlines ; i++ ) m_data[i] = new typename vci_param::data_t[nwords] ;
    m_tag = new typename vci_param::addr_t[nlines] ;
    m_val = new bool[nlines] ;
    m_words = nwords ;
    m_lines = nlines ;
}

//////////////////////////////
inline void reset( )
    {
    for ( size_t i = 0 ; i < m_lines ; i++ ) m_val[i] = false ;
    }

//////////////////////////////
inline bool miss(typename vci_param::addr_t ad )
    {
    uint32_t y = m_y[ad] ;
    uint32_t z = m_z[ad] ;
    return !( ( m_tag[y] == z ) && m_val[y] ) ;
    }

///////////////////////////////
inline typename vci_param::data_t read(typename vci_param:: addr_t ad )
    {
    uint32_t y = m_y[ad] ;
    uint32_t x = m_x[ad] ;
    return m_data[y][x] ;
    }

/////////////////////////////////////////
inline void write( typename vci_param::addr_t ad, typename vci_param::data_t dt )
    {
// comment fait-on si l'écriture est sur un half ou un byte ?
    uint32_t y = m_y[ad] ;
    uint32_t x = m_x[ad] ;
    m_data[y][x] = dt ;
    }

//////////////////////////////
inline void inval( typename vci_param::addr_t ad )
    {
    uint32_t y = m_y[ad] ;
    m_val[y] = false ;
    }

//////////////////////////////
inline void update( typename vci_param::addr_t ad, typename vci_param::data_t *buf )
    {
    uint32_t y = m_y[ad] ;
    uint32_t z = m_z[ad] ;
    m_val[y] = true ;
    m_tag[y] = z ;
    for ( uint32_t i = 0 ; i < m_words ; i++ ) m_data[y][i] = buf[i] ;
    }

inline uint32_t get_nlines()
{
	return m_lines;
}

inline uint32_t get_nwords()
{
	return m_words;
}

inline uint32_t get_yzmask()
{
	return m_yzmask;
}

};

}}

#endif
