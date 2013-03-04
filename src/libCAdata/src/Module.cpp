#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "stdafx.h"
#include "libCAdata.h"
#include "Function.h"
#include "Module.h"
#include "assert.h"


using namespace std;

CA_Module::CA_Module()
{
	m_base		= 0;
	m_size		= 0;
	m_modType	= 0;	
	m_isImdRead	= false;
	m_is32Bit	= true;

	m_total		= 0;
	m_imdIndex	= -1;	

	m_aggTotal	= 0;
	m_funcTotal	= 0;
}


CA_Module::~CA_Module()
{
	m_funcMap.clear();
	m_aggPidMap.clear();
}


bool CA_Module::doesAddressBelongToModule( VADDR addr, 
						const CA_Function ** ppFunc ) const
{
	for (AddrFunctionMultMap::const_reverse_iterator rit = m_funcMap.rbegin();
		rit != m_funcMap.rend(); rit++) 
	{
		if (addr < rit->first)
			continue;		

		const CA_Function * pFunc = &(rit->second);
		if (addr >= pFunc->m_baseAddr
		|| (pFunc->getEndSample() != pFunc->find(AptKey(addr-(pFunc->m_baseAddr), 0, 0)))) {
			*ppFunc = pFunc;
			return true;
		}				
	}

	*ppFunc = NULL;
	return false;
}


// HOTSPOT
void CA_Module::recordSample(const SampleInfo & sampleInfo, 
			  UINT32 sampleCnt,
			  VADDR funcAddr,
			  const wstring & funcName,
			  const wstring & jncName,
			  const wstring & javaSrcName)
{
	SampleKey sKey (sampleInfo.cpu, sampleInfo.event);

	m_total += sampleCnt;
	
	/////////////////
	// Update pid map
	PidAggregatedSampleMap::iterator ag_it = m_aggPidMap.find(sampleInfo.pid);
	if (ag_it == m_aggPidMap.end()) {		
		// Add new Pid
		AggregatedSample agSamp(sKey, sampleCnt);
		m_aggPidMap.insert(PidAggregatedSampleMap::value_type(sampleInfo.pid, agSamp));		
	} else {
		ag_it->second.addSamples(sKey, sampleCnt);
	}
	
	/////////////////
	// Update Function (IMD) Map 
	AddrFunctionMultMap::iterator fit = m_funcMap.find(funcAddr);
	if (fit == m_funcMap.end()) {
		// Add new Function (IMD sub-section)
		CA_Function func(funcName, funcAddr, jncName, javaSrcName);				
		fit = m_funcMap.insert(AddrFunctionMultMap::value_type(funcAddr, func));				
		fit->second.insertSample(sampleInfo, sampleCnt);								
	} else {								
		fit->second.addSample(sampleInfo, sampleCnt);
	}
} //CA_Module::updateFunctionMap



void CA_Module::recordSample(PID_T pid, AggregatedSample *aggSample)
{
	m_total += (*aggSample).getTotal();
	
	// Update pid map
	PidAggregatedSampleMap::iterator ag_it = m_aggPidMap.find(pid);
	if (ag_it == m_aggPidMap.end()) {		
		// Add new Pid
		// fprintf(stderr, "Record Samples... Insert new Pid(%d).\n", pid);
		m_aggPidMap.insert(PidAggregatedSampleMap::value_type(pid, *aggSample));		
	} else {
		// fprintf(stderr,"Record Samples... Add samples new Pid(%d).\n", pid);
		ag_it->second.addSamples(aggSample);
	}
} //CA_Module::recordSample


// void CA_Module::recordSample(AptKey aKey, AggregatedSample *aggSample)
void CA_Module::recordSample(VADDR addr, CA_Function *caFunc)
{
	// Update Function (IMD) Map 

	// TODO:
	// Check if the function is available,
	// if not, m_funcMap.insert
	// else,
	//	iterate over the samples...
	//		find the function and addSample..

	// For each sample
	AptAggregatedSampleMap::const_iterator ait = caFunc->getBeginSample();
	AptAggregatedSampleMap::const_iterator aend = caFunc->getEndSample();

	for (; ait != aend; ait++)
	{
		// AddrFunctionMultMap::iterator fit = m_funcMap.find(ait->first.m_addr);
		AddrFunctionMultMap::iterator fit = m_funcMap.find(addr);

		if (fit == m_funcMap.end()) {
			// fprintf(stderr, "Function NOT found for vaddr(0x%x) sampleaddr(0x%x).\n", addr, ait->first.m_addr);

			// Add new Function (IMD sub-section)
			// CA_Function func(funcName, funcAddr, jncName, javaSrcName);				
			fit = m_funcMap.insert(AddrFunctionMultMap::value_type(addr, *caFunc));				
			// fit->second.insertSample(sampleInfo, sampleCnt);								
		} else {
			// fprintf(stderr, "Function found for vaddr(0x%x) sampleaddr(0x%x).\n", addr, ait->first.m_addr);

			fit->second.addSample(ait->first, (AggregatedSample *)&(ait->second));
		}
	}

} //CA_Module::recordSample


bool CA_Module::compares(const CA_Module& m) const 
{
	if (m_base != m.m_base) {
		fwprintf (stderr, L"Warning: %s: m_base is different(%lx, %lx)\n", 
			m_path.c_str(), m_base, m.m_base);
		//return false;
	}

	if (m_size != m.m_size) {
		fwprintf (stderr, L"Error: %s: m_size is different(%x, %x)\n", 
			m_path.c_str(), m_size, m.m_size);
		return false;
	}

	if (m_modType != m.m_modType) {
		fwprintf (stderr, L"Error: %s: m_modType is different(%x, %x)\n", 
			m_path.c_str(), m_modType, m.m_modType);
		return false;
	}

	if (m_is32Bit != m.m_is32Bit) {
		fwprintf (stderr, L"Error: %s: m_is32Bit is different(%x, %x)\n", 
			m_path.c_str(), m_is32Bit, m.m_is32Bit);
		return false;
	}

	if (m_total != m.m_total) {
		fwprintf (stderr, L"Error: %s: m_total is different(%llu, %llu)\n", 
			m_path.c_str(), m_total, m.m_total);
		return false;
	}

	if (m_path != m.m_path) {
		fwprintf (stderr, L"Error: %s: m_path is different(%s, %s)\n", 
			m_path.c_str(), m_path.c_str(), m.m_path.c_str());
		return false;
	}

	if (m_imdIndex != m.m_imdIndex) {
		fwprintf (stderr, L"Error: %s: m_imdIndex is different(%x, %x)\n", 
			m_path.c_str(), m_imdIndex, m.m_imdIndex);
		return false;
	}

	if (m_aggTotal != m.m_aggTotal) {
		fwprintf (stderr, L"Error: %s: m_aggTotal is different(%llx, %llx)\n", 
			m_path.c_str(), m_aggTotal, m.m_aggTotal);
		return false;
	}

	if (m_funcTotal != m.m_funcTotal) {
		fwprintf (stderr, L"Error: %s: m_funcTotal is different(%llx, %llx)\n", 
			m_path.c_str(), m_funcTotal, m.m_funcTotal);
		return false;
	}

	if (m_aggPidMap.size() != m.m_aggPidMap.size()) {
		fwprintf (stderr, L"Error: %s: m_aggPidMap size is different\n", m_path.c_str());
		return false;
	}

	PidAggregatedSampleMap::const_iterator pit1  = m_aggPidMap.begin();
	PidAggregatedSampleMap::const_iterator pend1 = m_aggPidMap.end();
	PidAggregatedSampleMap::const_iterator pit2  = m.m_aggPidMap.begin();
	PidAggregatedSampleMap::const_iterator pend2 = m.m_aggPidMap.end();	
	for (; pit1 != pend1; pit1++, pit2++) {
		if (pit1->first != pit2->first) {
			fwprintf (stderr, L"Error: %s: m_aggPidMap pid is different (%d, %d)\n",
				m_path.c_str(), pit1->first, pit2->first);
			return false;
		}

		if (!pit1->second.compares(pit2->second)) {
			fwprintf (stderr, L"Error: %s: AggregatedSample data for pid %d is different.\n",
				m_path.c_str(), pit1->first);
			return false;
		}
	}

	if (m_funcMap.size() != m.m_funcMap.size()) {
		fwprintf (stderr, L"Error: %s: m_funcMap size is different\n", m_path.c_str());
		return false;
	}

	AddrFunctionMultMap::const_iterator fit1  = m_funcMap.begin();
	AddrFunctionMultMap::const_iterator fend1 = m_funcMap.end();
	AddrFunctionMultMap::const_iterator fit2  = m.m_funcMap.begin();
	AddrFunctionMultMap::const_iterator fend2 = m.m_funcMap.end();	
	for (; fit1 != fend1; fit1++, fit2++) {
		if (fit1->first != fit2->first) {
			fwprintf (stderr, L"Error: %s: m_funMap address is different (%lx, %lx)\n",
				m_path.c_str(), fit1->first, fit2->first);
			return false;
		}
		
		wchar_t strerr[1024] = {L'\0'};
		if (!fit1->second.compares(fit2->second, strerr)) {
			fwprintf (stderr, L"Error: %s: CA_Function comparison for base address %lx failed with reason:\n    %s",
				m_path.c_str(), fit1->first, strerr);
			return false;
		}
	}
	
	return true;
}

