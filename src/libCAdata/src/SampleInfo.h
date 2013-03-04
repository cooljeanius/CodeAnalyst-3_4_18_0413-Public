#ifndef _SAMPLEINFO_H_
#define _SAMPLEINFO_H_

#include <map>
#include <string>
#include "libCAdata_typedefs.h"

using namespace std;

/**********************************************************
 * Class SampleInfo
 * 
 * Description:
 * This is a container for passing around information 
 * related to a particular sample.
 */
class LIBCADATA_API SampleInfo
{
public:
	SampleInfo()
	{
		address		= 0;
		tid			= 0;
		pid			= 0;
		cpu			= 0;
		event		= 0;
	};

	SampleInfo(VADDR a, PID_T p, TID_T t, UINT32 c, EVMASK_T e)
	{
		address = a;
		pid = p;
		tid = t;
		cpu = c;
		event = e;
	};

	~SampleInfo(){};

public:
	VADDR		address;	
	TID_T		tid;
	PID_T		pid;
	UINT32		cpu;
	EVMASK_T	event;
};


/**********************************************************
 * Class SampleKey
 * 
 * Description:
 * This is the key for CA_SampleMap.
 */
class LIBCADATA_API SampleKey 
{
public:
	SampleKey ()
	{
		cpu = 0;
		event = 0;
	};

	SampleKey (int a, EVMASK_T b)
	{
		cpu = a;
		event = b;
	};

	SampleKey (const SampleKey &s)
	{
		cpu = s.cpu;
		event = s.event;
	};

	~SampleKey() {};

	bool operator< (const SampleKey & m) const
	{
		if (cpu < m.cpu)
			return true;
		else if ((cpu == m.cpu) && (event < m.event))
			return true;
		else
			return false;
	};

	bool operator== (const SampleKey & m) const
	{
		if (cpu == m.cpu)
		if (event == m.event)
			return true;
		return false;
	};


public:
	int		cpu;
	EVMASK_T	event;
};

/**********************************************************
 * CA_SampleMap
 *
 * Description:
 * This is the smallest granularity map 
 * which contain sample per cpu per event.
 */
typedef	std::map<SampleKey, unsigned long> CA_SampleMap;



/**********************************************************
 * Class AggregatedSample
 *
 * Description:
 * This is the general purpose container for storing samples.
 * It contains a map and provide keep track of total number
 * of samples within the map.
 */
class LIBCADATA_API AggregatedSample
{
public:
	AggregatedSample() 
	{
		m_total = 0;
		m_sampleMap.clear();
	};

	AggregatedSample(SampleKey & key, unsigned long count) 
	{
		m_total = 0;
		m_sampleMap.clear();
		insertSamples(key, count);
	};

	AggregatedSample(const AggregatedSample &p)
	{
		m_total = p.m_total;
		m_sampleMap.clear();

		CA_SampleMap::const_iterator it    = p.m_sampleMap.begin();
		CA_SampleMap::const_iterator itEnd = p.m_sampleMap.end();
		for (; it != itEnd; it++) {
			SampleKey key = it->first;
			unsigned long data = it->second;
			m_sampleMap.insert(CA_SampleMap::value_type(key, data));
		}
	};

	virtual ~AggregatedSample() {clear();};

	void clear() 
	{
		m_sampleMap.clear();
		m_total = 0;
	};

	UINT64 getTotal() const 
	{ return m_total; };

	int getSampleMapSize() const 
	{ return (unsigned int)m_sampleMap.size(); };

	void aggregateForAllCpus(AggregatedSample * agg) const
	{
		CA_SampleMap::const_iterator sit    = m_sampleMap.begin();
		CA_SampleMap::const_iterator sitEnd = m_sampleMap.end();
		for (; sit != sitEnd; sit++) {
			// CPU -1 denotes for all CPUs.
			SampleKey key(-1, sit->first.event);
			agg->addSamples(key, sit->second);
		}
	};


	//////////////////////////////////////////////////////////
	/* NOTE: Inserting assumes the key doesn't exist 
	 *       in the current map
	 */
	void insertSamples(SampleKey & key, unsigned long count) 
	{ 
		m_sampleMap.insert(CA_SampleMap::value_type(key, count)); 
		m_total += count;
	};


	//////////////////////////////////////////////////////////
	/* NOTE: Adding assumes the key might already exist 
	 *       in the current map. Need to search!!!
	 */
	void addSamples(SampleKey & key, unsigned long count) 
	{ 
		CA_SampleMap::iterator it = m_sampleMap.find(key);
		if (it == m_sampleMap.end()) {
			m_sampleMap.insert(CA_SampleMap::value_type(key, count)); 
		} else {
			it->second += count;
		}
		m_total += count;
	};

	void addSamples(const CA_SampleMap * sampMap) 
	{
		if (!sampMap)
			return;
 
		CA_SampleMap::const_iterator sit    = sampMap->begin();
		CA_SampleMap::const_iterator sitEnd = sampMap->end();
		for (; sit != sitEnd; sit++) {			
			CA_SampleMap::iterator it = m_sampleMap.find(sit->first);
			if (it == m_sampleMap.end()) {
				m_sampleMap.insert(
					CA_SampleMap::value_type(sit->first, sit->second)); 
			} else {
				it->second += sit->second;
			}
			m_total += sit->second;
		}
	};

	void addSamples(const AggregatedSample * aggSamp) { 		
		if (!aggSamp)
			return;

		addSamples(&(aggSamp->m_sampleMap));
	};


	CA_SampleMap::const_iterator getBeginSample() const 
	{ return m_sampleMap.begin(); };
	
	CA_SampleMap::const_iterator find(SampleKey & key) const 
	{ return m_sampleMap.find(key); };
	
	CA_SampleMap::const_iterator getEndSample() const 
	{ return m_sampleMap.end(); };

	bool compares(const AggregatedSample & p) const 
	{
		if (m_total != p.m_total) {			
			return false;
		}

		if (m_sampleMap != p.m_sampleMap) {
			return false;
		}
		return true;
	}

protected:
	UINT64		m_total;
	CA_SampleMap	m_sampleMap;
};


/**********************************************************
 * PidAggregatedSampleMap
 *
 * Description:
 * This map contain samples for each PID. Mainly used for
 * the data of [MODULE] section of the TBP/EBP file.
 */
typedef std::map<PID_T, AggregatedSample> PidAggregatedSampleMap;


/**********************************************************
 * class AptKey:
 * 
 * Description:
 * This class is the key containing "Addr + Pid + Tid" (APT)
 */
class LIBCADATA_API AptKey
{
public:
	AptKey() 
	{
		m_addr	= 0;
		m_pid	= 0;
		m_tid	= 0;
	};

	AptKey(	VADDR a, PID_T p, TID_T t)
	{
		m_addr	= a;
		m_pid	= p;
		m_tid	= t;
	};

	~AptKey(){};

	bool operator< (const AptKey &temp2) const 
	{
		if (m_addr < temp2.m_addr)
			return true;
		else if (m_addr > temp2.m_addr)
			return false;
		else if (m_pid < temp2.m_pid)
			return true;
		else if (m_pid > temp2.m_pid)
			return false;
		else if (m_tid < temp2.m_tid)
			return true;
		else 
			return false;
	};

	bool compares (const AptKey & a) const
	{
		if (m_addr == a.m_addr)
		if (m_pid  == a.m_pid)
		if (m_tid  == a.m_tid)
			return true;
		return false;
	};

	VADDR m_addr;
	PID_T m_pid;
	TID_T m_tid;
};

/**********************************************************
 * This map represent the data in [SUB] section of the IMD file.
 * This contains the samples per address per pid per tid.
 */
typedef std::map<AptKey, AggregatedSample>	AptAggregatedSampleMap;

#endif //_SAMPLEINFO_H_
