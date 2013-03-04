#ifndef _INLINEINST_H_
#define _INLINEINST_H_

#include "typedefs.h"
#include <strings.h>

using namespace std;

/*
 * Class to hold address key
 */
class ADDRESSKEY
{
public:
	VADDR startAddr;
	VADDR stopAddr;

	ADDRESSKEY()
	{
		startAddr = 0;
		stopAddr = 0;
	};	

	// Note: address key is sorted in descending order;
	// to make easy search in the map
	bool operator < (const ADDRESSKEY & other) const 
	{
		if(startAddr < other.startAddr)
		{
			return true;	
		}
		else if (startAddr == other.startAddr)
		{
			if(stopAddr > other.stopAddr)
				return true;
			else
				return false;
		}
		else
		{
			return false;
		}
	};

};

typedef std::vector<ADDRESSKEY> RANGEVEC;

typedef std::list<unsigned int> NestedIlList;

/*
 * Class to hold inline instance information
 */
class InlineInst
{
public:
	string		symName;
	VADDR		parentAddr;
	unsigned int	gOff;
	unsigned int	callLineNo;
	unsigned int	callFileNo;
	string		callFileName;
	unsigned int	declLineNo;
	unsigned int	declFileNo;
	string		declFileName;
	VADDR		startAddr;
	VADDR		stopAddr;
	unsigned int 	nestedIlParent;
	RANGEVEC	rangeVec;
	NestedIlList	nestedIlList;
	
	
	InlineInst()
	{
		parentAddr = 0;
		gOff = 0;
		callLineNo = 0;
		callFileNo = 0;
		declLineNo = 0;
		declFileNo = 0;
		startAddr = 0;
		stopAddr = 0;
		nestedIlParent = 0;
	};

	const InlineInst & operator= (InlineInst rhs)
	{
		symName = rhs.symName;
		parentAddr = rhs.parentAddr;
		gOff = rhs.gOff;
		callLineNo = rhs.callLineNo;
		callFileNo = rhs.callFileNo;
		callFileName = rhs.callFileName;
		declLineNo = rhs.callLineNo;
		declFileNo = rhs.callFileNo;
		declFileName = rhs.callFileName;
		startAddr = rhs.startAddr;
		stopAddr = rhs.stopAddr;
		nestedIlParent = rhs.nestedIlParent;
		
		for (unsigned int i = 0; i < rhs.rangeVec.size(); i++) {
			rangeVec.push_back(rhs.rangeVec[i]);
		}
		for (NestedIlList::iterator nit = rhs.nestedIlList.begin();
				nit != rhs.nestedIlList.end() ; nit++)
		{
			nestedIlList.push_back(*nit);
		}
		return *this;
	};
};

// NOTE: Key is the DIE CU offset 
typedef std::map<unsigned int , InlineInst> InlinelInstMap;

#endif
