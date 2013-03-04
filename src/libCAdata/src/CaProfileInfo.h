#ifndef _CAPROFILEINFO_H_
#define _CAPROFILEINFO_H_

#include <string>

using namespace std;

/****************************************************
 * class CaProfileInfo
 *
 * Description:
 * This class contain the metadata for each profile
 * session. This is equivalent to the [ENV] section
 * of TBP/EBP file
 */
class LIBCADATA_API CaProfileInfo 
{
public:
	CaProfileInfo()	{
		m_numNodes = 0;
		m_numCpus	= 0;
		m_numEvents	= 0;
		m_numSamples	= 0;
		m_numMisses	= 0;
		m_numModules	= 0;
		m_tbpVersion = TBPVER_UNKNOWN;
		m_cpuFamily	= 0; 
		m_cpuModel	= 0; 		
	};

	~CaProfileInfo(){
		//m_eventVec.clear();
	};

	bool addEvent(EVMASK_T m, UINT64 c) {
		EventEncodeType ev(m, c, (unsigned int)m_eventVec.size());
		m_eventVec.push_back(ev);
		m_numEvents++;
		return true;
	};

	void setTimeStamp(wstring & dateTime) {
		m_timeStamp = dateTime;
	};

public:
	unsigned int		m_numNodes;
	unsigned int 		m_numCpus;
	unsigned long 		m_numEvents;
	unsigned int 		m_numSamples;
	unsigned int 		m_numMisses;
	unsigned int 		m_numModules;
	unsigned int 		m_tbpVersion;
	unsigned long 		m_cpuFamily; 
	unsigned long 		m_cpuModel; 
	wstring 		m_timeStamp;
	EventEncodeVec		m_eventVec;
	CA_SampleMap		m_totalMap;
};

#endif //_CAPROFILEINFO_H_
