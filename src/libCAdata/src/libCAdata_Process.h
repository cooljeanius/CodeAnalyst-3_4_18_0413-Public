#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <string>
#include <map>
#include "SampleInfo.h"

using namespace std;

//#pragma warning (disable : 4996)	// swprintf

/***********************************************************
 * class CA_Process
 * 
 * Description:
 * This class represent a process. It represents each
 * line in the [PROCESS] section
 */
class LIBCADATA_API CA_Process 
: public AggregatedSample 
{
public:
	CA_Process() : AggregatedSample() 
	{
		m_is32Bit	= false;
		m_hasCss	= false;
	};

	CA_Process(const CA_Process &p) 
		: AggregatedSample((const AggregatedSample &) p) 
	{		
		m_is32Bit	= p.m_is32Bit;		
		m_fullPath	= p.m_fullPath;
		m_hasCss	= p.m_hasCss;		
	};

	~CA_Process(){
		m_sampleMap.clear();	
	};

	void clear() {
		m_sampleMap.clear();
	}

	bool compares(const CA_Process & p, wchar_t * strerr, size_t maxlen = LIBCADATA_MAXLEN) const 
	{
		if (m_is32Bit != p.m_is32Bit) {
			swprintf(strerr, maxlen, L"Error: m_is32Bit is different (%x, %x)\n", m_is32Bit, p.m_is32Bit);
			return false;
		}

		if (m_hasCss  != p.m_hasCss) {
			swprintf(strerr, maxlen, L"Error: m_hasCss is different (%x, %x)\n", m_hasCss, p.m_hasCss);
			return false;
		}
		if (m_fullPath != p.m_fullPath) {
			swprintf(strerr, maxlen, L"Error: m_fullPath is different (%s, %s)\n", m_fullPath.c_str(), p.m_fullPath.c_str());
			return false;
		}

		if (m_total != p.m_total) {
			swprintf(strerr, maxlen, L"Error: m_total is different (%llu, %llu)\n", m_total, p.m_total);
			return false;
		}

		if (m_sampleMap != p.m_sampleMap) {
			swprintf(strerr, maxlen, L"Error: m_sampleMap is different.\n");
			return false;
		}
		return true;
	};

	wstring getPath() const 
	{ return m_fullPath; };

	void setPath(wstring & path) 
	{ m_fullPath = path; };
	
public:
	bool			m_is32Bit;	
	bool			m_hasCss;	

private:	
	wstring			m_fullPath;
};


/***********************************************************
 * Description:
 * This map represent the [PROCESS] section.
 */
typedef map<PID_T, CA_Process>  PidProcessMap;

#endif // _PROCESS_H_
