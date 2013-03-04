#ifndef _INLINEINSTANCE_H_
#define _INLINEINSTANCE_H_

#include <string>
#include <list>

using namespace std;

/*******************************************
 * class InlineInstance
 *
 * Description:
 * This class represent each Java inline instance
 */
class LIBCADATA_API InlineInstance
{
public:
	InlineInstance()
	{
		m_addr	= 0;
		m_size	= 0;
	};

	~InlineInstance(){};

	bool operator< (const InlineInstance & i) const 
	{
		if (m_addr < i.m_addr)
			return true;
		else if (m_symbol < i.m_symbol)
			return true;
		else
			return false;
	};

public:
	VADDR	m_addr;
	wstring	m_symbol;
	UINT32	m_size;
};	

/*******************************************
 * Description:
 * This map represents the Java inline instance
 * map listed in the prolog of IMD file
 */
typedef std::list<InlineInstance> InlineInstanceList;

#endif // _INLINEINSTANCE_H_