#include "PerfEventContainer.h"
#include "stdafx.h"
#include "helperAPI.h"
#include "eventsfile.h"

PerfEventContainer::PerfEventContainer()
{
	m_cpuType = getFamilyFromCpuInfo("/proc/cpuinfo");
}


PerfEventContainer::PerfEventContainer(unsigned int cpuType)
{
	m_cpuType = cpuType;
}


PerfEventContainer::~PerfEventContainer()
{

}

bool PerfEventContainer::operator==(const PerfEventContainer & opt) const
{
	if (m_perfEventList.size() != opt.m_perfEventList.size())
		return false;

	PerfEventList::const_iterator it     = opt.m_perfEventList.begin();	
	PerfEventList::const_iterator it_end = opt.m_perfEventList.end();	
	for (; it != it_end ; it++) {
		PerfEventList::const_iterator cur = find((*it)); 
		if (cur == m_perfEventList.end())
			return false;

		if (*cur == *it
		&&  (*cur).count == (*it).count    // Also compare count value in this case
		&&  (*cur).flags == (*it).flags)   // Also compare flags value in this case
			continue;
		else
			return false;
	}
	return true;
}

bool PerfEventContainer::add(const PerfEvent & event)
{
	PerfEventList::const_iterator it = find(event);

	// Do not allow duplicate event
	if (it != m_perfEventList.end())
		return false;

	m_perfEventList.push_back(event);
	m_perfEventList.sort();

	return true;	
}


bool PerfEventContainer::remove(const PerfEvent & event)
{
	PerfEventList::const_iterator it = find(event);
	if (it == m_perfEventList.end())
		return false;

	m_perfEventList.remove(*it);

	return true;	

}


PerfEventList::const_iterator PerfEventContainer::find(const PerfEvent & perf) const
{
	PerfEventList::const_iterator it = m_perfEventList.begin();	
	PerfEventList::const_iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it) == perf)
			break;
	}
	return it;	
}


unsigned int PerfEventContainer::count()
{
	return m_perfEventList.size();
}


unsigned int PerfEventContainer::countPmc()
{
	int count = 0;
	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it).type() != PerfEvent::IbsFetch
		&&  (*it).type() != PerfEvent::IbsOp) {
			count++;
		}
	}
	return count;
}


unsigned int PerfEventContainer::countIbsFetch()
{
	return m_perfEventList.size();
	int count = 0;
	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::IbsFetch) {
			count++;
		}
	}
	return count;
}


unsigned int PerfEventContainer::countIbsOp()
{
	int count = 0;
	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::IbsOp) {
			count++;
		}
	}
	return count;
}


void PerfEventContainer::dumpPerfEvents()
{
	fprintf(stdout, "Total %d PerfEvents:\n", (int)m_perfEventList.size());
	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		(*it).dump();
	}
}


void PerfEventContainer::setIbsFetchCountUmask(unsigned long count, unsigned int umask)
{
	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::IbsFetch) {
			(*it).count = count;
			(*it).setUmask(umask);
		}
	}

	return;
}


bool PerfEventContainer::getIbsFetchCountUmask(unsigned long & count, unsigned int & umask)
{
	unsigned long tmpCount = 0;
	unsigned int  tmpUmask = 0;

	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::IbsFetch) {
			if (tmpCount == 0 && tmpUmask == 0) {
				tmpCount = (*it).count;
				tmpUmask = (*it).umask();
			} else if (tmpCount != (*it).count 
				|| tmpUmask != (*it).umask()) {
				return false;
			}
		}
	}
	count = tmpCount;
	umask = tmpUmask;
	
	return true;
}


void PerfEventContainer::setIbsOpCountUmask(unsigned long count, unsigned int umask)
{
	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::IbsOp) {
			(*it).count = count;
			(*it).setUmask(umask);
		}
	}

	return;
}


bool PerfEventContainer::getIbsOpCountUmask(unsigned long & count, unsigned int & umask)
{
	unsigned long tmpCount = 0;
	unsigned int  tmpUmask = 0;

	PerfEventList::iterator it = m_perfEventList.begin();	
	PerfEventList::iterator it_end = m_perfEventList.end();	
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::IbsOp) {
			if (tmpCount == 0 && tmpUmask == 0) {
				tmpCount = (*it).count;
				tmpUmask = (*it).umask();
			} else if (tmpCount != (*it).count 
				|| tmpUmask != (*it).umask()) {
				return false;
			}
		}
	}
	count = tmpCount;
	umask = tmpUmask;

	return true;
}


void PerfEventContainer::enumerateAllEvents(EventMaskEncodeMap & event_map)
{
	EventMaskEncodeMap::iterator ev_it, ev_end;
		
	PerfEventList::iterator it     = getPerfEventListBegin();
	PerfEventList::iterator it_end = getPerfEventListEnd();
	for (; it != it_end; it++) {
		// This string will be passed to Oprofile	
		string  key = (*it).getEventMaskEncodeMapKey().toAscii().data();
	
		EventEncodeType eet;
		eet.sortedIndex = 0; 
		//eet.weight = 1;
		eet.eventCount = (*it).count; 
		eet.eventMask = EncodeEventMask((*it).select(), (*it).umask());
		event_map.insert(EventMaskEncodeMap::value_type(key, eet));
	}

	// change the index order of events;
	ev_it = event_map.begin();
	ev_end = event_map.end();
	for(int i = 0; ev_it != ev_end; ev_it++, i++) {
		ev_it->second.sortedIndex = i;
	}
}


bool PerfEventContainer::hasIbs()
{
	unsigned long tmpCount = 0;
	unsigned int  tmpUmask = 0;

	if (getIbsFetchCountUmask(tmpCount, tmpUmask) && tmpCount > 0)
		return true;

	if (getIbsOpCountUmask(tmpCount, tmpUmask) && tmpCount > 0)
		return true;

	return false;
}


bool PerfEventContainer::validateEvents(CEventsFile * pEventFile, QString & errorStr)
{
	CpuEvent ev;
	unsigned long fetchCount = 0, opCount = 0;
	unsigned int  fetchUmask = 0, opUmask = 0;
	
	if (!pEventFile)
		return false;

	// For each event
	PerfEventList::iterator it     = getPerfEventListBegin();
	PerfEventList::iterator it_end = getPerfEventListEnd();
	for (; it != it_end; it++) {
		// Get event info from events file
		if (!pEventFile->findEventByValue((*it).select(), ev)) {
			errorStr.sprintf("Event 0x%x is not supported.", (*it).select()); 
			return false;
		}

		// Set opname 
		(*it).opName = ev.op_name;

		// check type 
		if ((*it).type() == PerfEvent::Invalid)
			return false;

		switch ((*it).type()) {
		case PerfEvent::IbsFetch:
			if (fetchCount == 0 && fetchUmask == 0) {
				fetchCount = (*it).count;
				fetchUmask = (*it).umask();
			} else if (fetchCount != (*it).count 
				|| fetchUmask != (*it).umask()) {
				return false;
			}

			break;
		case PerfEvent::IbsOp:
			if (opCount == 0 && opUmask == 0) {
				opCount = (*it).count;
				opUmask = (*it).umask();
			} else if (opCount != (*it).count 
				|| opUmask != (*it).umask()) {
				return false;
			}
			break;
		default:
			break;
		};
	} // for

	return true;
}


void PerfEventContainer::dumpAllEvents()
{
	PerfEventList::iterator it     = getPerfEventListBegin();
	PerfEventList::iterator it_end = getPerfEventListEnd();
	for (; it != it_end; it++) {
		(*it).dump();
	}
	
}


//////////////////////////////////////////////////////
/* Event Mapping Stuff */

/*
 * Current Possible Event Mapping upto family15h
 *
 * allocMask | num bits | value | order
 * ------------------------------
 * 000 001 | 1 | 0x01 | 1
 * 001 000 | 1 | 0x08 | 2
 * 001 001 | 2 | 0x09 | 3
 * 000 111 | 3 | 0x07 | 4
 * 111 000 | 3 | 0x38 | 5
 * 111 111 | 6 | 0x3F | 6
 *
 * Mapping order is determined by 
 * 1. Smaller number of bits with value "1" (give priority to events which 
 *    have more counter restrictions.
 * 2. allocMask value (so that the result list is sorted by counter)
 * 3. Event-select value (so that the result list is sorted by event select)
 */


/* Return the number of bits with value "1" */
unsigned int count1Bit(unsigned long long value) 
{
	value &= 0xFF;
	unsigned int count = 0;	
	while (value) {
		count += value & 0x1;
		value >>= 1;
	}
	
	return count;
}


/* Used for sorting event list */
bool compareMappingOrder(const PerfEvent a, const PerfEvent b)
{
	unsigned int a_bitCount = count1Bit(a.allocMask);
	unsigned int b_bitCount = count1Bit(b.allocMask);

	// Check num 1 bits
	if (a_bitCount < b_bitCount) 
		return true;
	else if (a_bitCount > b_bitCount)
		return false;

	// Check value
	if (a.allocMask < b.allocMask)
		return true;
	else if (a.allocMask > b.allocMask)
		return false;

	// Check evSelect 
	if (a.select() < b.select())
		return true;
	
	return false;
}


//////////////////////////////////////////////////////

unsigned int PerfEventContainer::getMuxGroup()
{
	/* On Linux, we fix the max number of MUX
 	 * to 32. So we calculate the number of MUX
 	 * group allowed.
 	 */
	return (MAX_EVENTNUM_MULTIPLEXING / getNumCntrForCpu(m_cpuType));
}


/* This function takes normal allocation mask (6 or 4 bits)
 * and extend to match the number of available MUX.
 */
unsigned long long PerfEventContainer::getMuxAllocMask(unsigned int allocMask)
{
	unsigned long long allocMaskMux = allocMask;
	unsigned int muxGrp = getMuxGroup();

	for ( unsigned int i = 0 ; i < muxGrp-1  ; i++ ) {
		allocMaskMux |= (allocMaskMux << getNumCntrForCpu(m_cpuType));
	}
	
	return allocMaskMux;
}


/* This fuction returns the allocMask. */
unsigned int PerfEventContainer::getAllocMask(unsigned int evSel)
{
	CEventsFile evFile;
	if (!evFile.open(helpGetEventFile(m_cpuType).toAscii().data()))
		return 0;

	CpuEvent event;
	if (!evFile.findEventByValue(evSel, event))
		goto errOut;

	evFile.close();
	return event.counters;

errOut:
	evFile.close();
	return 0;
}


/* This function return the next first available counter 
 * to be mapped based in the allocMask and the availMask
 */
int PerfEventContainer::mapEachEvToCounter(unsigned long long allocMask, 
					   unsigned long long & availMask)
{
	unsigned long long cur = 0;
	unsigned int ind = 0;
	unsigned int numMux = getMuxGroup() * getNumCntrForCpu(m_cpuType);

	// Extend allocMask to the MUX number
	allocMask = getMuxAllocMask(allocMask);

	//printf("Before allocMask = 0x%016llx\n", allocMask);
	//printf("       availMask = 0x%016llx\n", availMask);

	// Map event to counter
	do {
		cur = (1ULL << ind);
		
		if (!availMask || !allocMask) {
			//printf("Error: Cannot map:\nallocMask = 0x%016llx\navailMask = 0x%016llx\n",
			//	allocMask, availMask);
			return -1;
		}
 
		if ( (cur & availMask) && (cur & allocMask)) {
			// Event can be mapped
			availMask &= ~cur; 	// Update availMask
			//printf("       alloc to  = 0x%016llx (%d)\n", cur, ind); 
			//printf(" After availMask = 0x%016llx\n", availMask);
			return ind;
		} 

		// Get next counter
		ind++;
	} while ( ind < numMux);
	
	//printf("Error: Cannot map:\nallocMask = 0x%016llx\navailMask = 0x%016llx\n",
	//	allocMask, availMask);
	return -1;
}


/* On success, the each event in perfList will be mapped to a counter, 
 * specified in availMask. 
 */
bool PerfEventContainer::getCounterAllocation( unsigned long long availMask, QString * errorStr)
{
	bool ret = true;

	// Step1: Get allocation mask for each event 
	PerfEventList::iterator evIt  = m_perfEventList.begin();	
	PerfEventList::iterator evEnd = m_perfEventList.end();	
	for (; evIt != evEnd ; evIt++) {
		if ((*evIt).type() != PerfEvent::Pmc 
		&&  (*evIt).type() != PerfEvent::OProfile)
			continue;

		/* NOTE: We "&" the availMask to mask out the counters
		 *       which are not currently available since this affects
		 *       the sorting order.
		 */
		(*evIt).allocMask = getAllocMask((*evIt).select()) & availMask;
	}

	// Step2: Get mapping order
	m_perfEventList.sort(compareMappingOrder);
// For DEBUG
//	printf("\nAfter sort:\n");
//	printList();

	// Step3: Map events to counter
	unsigned long long curAvailMask = availMask;
	evIt  = m_perfEventList.begin();	
	evEnd = m_perfEventList.end();	
	for (; evIt != evEnd; evIt++) {
		if ((*evIt).allocMask == 0
		||  ((*evIt).type() != PerfEvent::Pmc 
		    && (*evIt).type() != PerfEvent::OProfile))
			continue;

		if (0 > ((*evIt).counter = mapEachEvToCounter((*evIt).allocMask, curAvailMask))) 
		{
			if (errorStr)
				errorStr->sprintf("Event 0x%x cannot be mapped in this set of events\n"
					"(Allocation mask : 0x%x, Current available mask : 0x%x )."
					, (*evIt).select(), (*evIt).allocMask, curAvailMask); 
			
			ret = false;
			break;
		} 
// For DEBUG
//		printf("\nDEBUG: Map event 0x%03x to counter %d (current available mask = 0x%x)\n", 
//			(*evIt).select(), (*evIt).counter, curAvailMask);

	}

	// Step4: Restore original order
	m_perfEventList.sort();
	return ret;
}


void PerfEventContainer::printList()
{
	PerfEventList::iterator it    = m_perfEventList.begin();
	PerfEventList::iterator itEnd = m_perfEventList.end();
	for (int i = 0; it != itEnd; it++, i++) {
		printf("%d:", i);
		printf("EventSelect:0x%03x, allocMask:0x%03llx\n", 
			(*it).select(), (*it).allocMask);
	}
}
