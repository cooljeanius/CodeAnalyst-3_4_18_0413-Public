#ifndef _PERFEVENTCONTAINER_H_
#define _PERFEVENTCONTAINER_H_

#include <qmap.h>
#include <q3valuelist.h>
#include <list>

#include "PerfEvent.h"
#include "stdafx.h"
#include "EventMaskEncoding.h"
#include "eventsfile.h"

//typedef QValueList<PerfEvent> 		PerfEventList;
typedef std::list<PerfEvent> 		PerfEventList;

class PerfEventContainer
{
public:
	PerfEventContainer();

	PerfEventContainer(unsigned int cpuType);

	~PerfEventContainer();

	bool operator==(const PerfEventContainer & opt) const;

	bool add(const PerfEvent & event);

	bool remove(const PerfEvent & event);
	
	unsigned int count();
	
	unsigned int countPmc();

	unsigned int countIbsFetch();
	
	unsigned int countIbsOp();

	PerfEventList * getPerfEventList() 
	{ return &m_perfEventList; };
	
	PerfEventList::iterator getPerfEventListBegin() 
	{ return m_perfEventList.begin(); };
	
	PerfEventList::iterator getPerfEventListEnd() 
	{ return m_perfEventList.end(); };
	
	void dumpPerfEvents();

	void setIbsFetchCountUmask(unsigned long count, unsigned int umask);

	bool getIbsFetchCountUmask(unsigned long & count, unsigned int & umask);

	void setIbsOpCountUmask(unsigned long count, unsigned int umask);

	bool getIbsOpCountUmask(unsigned long & count, unsigned int & umask);

	bool hasIbs();

	bool validateEvents(CEventsFile * pEventFile, QString & errorStr);

	void enumerateAllEvents(EventMaskEncodeMap & event_map);

	void dumpAllEvents();

	bool getCounterAllocation(unsigned long long availMask, QString * errorStr = NULL);

private:
	PerfEventList::const_iterator find(const PerfEvent & perf) const;
	
	unsigned int getMuxGroup();

	unsigned long long getMuxAllocMask(unsigned int allocMask);

	unsigned int getAllocMask(unsigned int evSel);

	int mapEachEvToCounter(unsigned long long allocMask, 
				unsigned long long & availMask);

	void printList();
	
protected:
	PerfEventList	m_perfEventList;
	unsigned int 	m_cpuType;
	
};

#endif /*_PERFEVENTCONTAINER_H_*/
