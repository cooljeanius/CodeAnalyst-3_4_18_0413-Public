//$Id: $
//  The CaProfileWriter class.

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2011 Advanced Micro Devices, Inc.
// You may redistribute this program and/or modify this program under the terms
// of the GNU General Public License as published by the Free Software 
// Foundation; either version 2 of the License, or (at your option) any later 
// version.
//
// This program is distributed WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED 
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  See the 
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#if ( defined (_WIN32) || defined (_WIN64) )
#include "stdafx.h"
#endif
#include <sstream>
#include <vector>
#include "CaProfileWriter.h"
#include "ImdWriter.h"

using namespace std;

CaProfileWriter::CaProfileWriter() : CaDataWriter()
{
}


CaProfileWriter::~CaProfileWriter() 
{
	close();
}

bool CaProfileWriter::write(const wstring & path,
					  CaProfileInfo * profileInfo,
					  PidProcessMap * procMap,
					  NameModuleMap * modMap,
					  CoreTopologyMap * pTopMap) 
{
	if (open(path)
	&&  writeProfileInfo(profileInfo, procMap, pTopMap)
	&&  writeProcSection(procMap)
	&&  writeModSectionAndImd(modMap)) {
		close();
		return true;
	}
	return false;
}

bool CaProfileWriter::open(wstring path)
{
	bool bRet = true;
	m_err = OK;
	
	if (!(bRet = CaDataWriter::open(path))) {
		m_err = E_OPENFAILED;
		return bRet;
	}
	
	return bRet;
}


bool CaProfileWriter::writeProfileInfo(CaProfileInfo * profileInfo,
									   PidProcessMap * procMap,
									   CoreTopologyMap * pTopMap) 
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_OK) {
		m_err = E_UNKNOWN;
		return false;
	}

	// save eventmask in case we need it
	m_pEventVec = &(profileInfo->m_eventVec);

	//order is VERY important
	// write version info
	fwprintf(m_pFile, L"TBPFILEVERSION=%d\n", TBPVER_DEFAULT);

	// Start to write Environment section
	fwprintf(m_pFile, ENV_BEGIN);
	fwprintf(m_pFile, L"\n");
	fwprintf(m_pFile, L"CPU=%u\n", profileInfo->m_numCpus);
	fwprintf(m_pFile, L"NumEvents=%u\n",profileInfo->m_numEvents);
	fwprintf(m_pFile, L"MODULES=%u\n", profileInfo->m_numModules);
	fwprintf(m_pFile, L"SAMPS=%lu\n", profileInfo->m_numSamples);			    
	fwprintf(m_pFile, L"TIMESTAMP=%ls\n",profileInfo->m_timeStamp.c_str());
	fwprintf(m_pFile, L"MISSED=%u\n", profileInfo->m_numMisses);								    
	fwprintf(m_pFile, L"CPUFAMILY=%lu,%lu\n", profileInfo->m_cpuFamily, 
										profileInfo->m_cpuModel);

	// Event info
	EventEncodeVec::iterator eit = m_pEventVec->begin();
	EventEncodeVec::iterator eitEnd = m_pEventVec->end();
	for(int i = 0; eit != eitEnd; eit++, i++) {
		// Format : Event <index>,<event+mask>,<norm>
		// Example: Event 0,118,250000.000000
		// Note: calculate event normalization here - althought it was 
		// 		not used in current post processing
		// 
		fwprintf(m_pFile, L"Event %u,%lu,%llu\n",
				i, (EVMASK_T) eit->eventMask,
				(UINT64) eit->eventCount);

		if (m_evToIndexMap.find(eit->eventMask) != m_evToIndexMap.end()) {
			m_err = E_UNKNOWN;
			return false;
		}

		m_evToIndexMap.insert(
			map<EVMASK_T,int>::value_type(eit->eventMask,i));
	}

	if (NULL != pTopMap)
	{
		CoreTopologyMap::iterator tIt = pTopMap->begin();
		CoreTopologyMap::iterator tItEnd = pTopMap->end();
		for(; tIt != tItEnd; tIt++) 
		{
			// Format : TOPOLOGY,<logical core index>,<Processor>,<NUMA Node>
			// Example: TOPOLOGY,0,0,0
			fwprintf(m_pFile, L"TOPOLOGY,%u,%u,%u\n",
				tIt->first, tIt->second.processor, tIt->second.numaNode);
		}
	}

	// write profile total
	writeProfileTotal(procMap);

	// write end of environment section
	fwprintf(m_pFile, ENV_END);
	fwprintf(m_pFile, L"\n\n");
	fflush(m_pFile);

	return true;
}


bool CaProfileWriter::writeProfileTotal(PidProcessMap * procMap)
{
	AggregatedSample aggSamp;

	PidProcessMap::iterator pit  = procMap->begin();
	PidProcessMap::iterator pend = procMap->end();
	for(; pit != pend; pit++) {
		aggSamp.addSamples((const AggregatedSample *)&(pit->second));
	}
	
	fwprintf(m_pFile, L"TOTAL=");

	CA_SampleMap::const_iterator ait  = aggSamp.getBeginSample();
	CA_SampleMap::const_iterator aend = aggSamp.getEndSample();
	while (ait != aend) {
		SampleKey key = ait->first;
	
		map<EVMASK_T, int>::iterator eit;
		if ((eit = m_evToIndexMap.find(key.event)) == m_evToIndexMap.end()) {
			m_err = E_INVALIDPROCESSMAP;
			return false;
		}

		fwprintf(m_pFile, L"[%u %d] %u", key.cpu, eit->second, 
											ait->second);
		if (++ait != aend)
			fwprintf(m_pFile, L",");
		else
			fwprintf(m_pFile, L"\n");
	}
	
	return true;
}

bool CaProfileWriter::writeProcSectionProlog()
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_OK) {
		m_err = E_UNKNOWN;
		return false;
	}

	fwprintf(m_pFile, PROCESSDATA_BEGIN);
	fwprintf(m_pFile, L"\n");
	fflush(m_pFile);

	m_stage = evOut_TaskSummary;

	return true;
}

bool CaProfileWriter::writeProcSectionEpilog()
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_TaskSummary) {
		m_err = E_UNKNOWN;
		return false;
	}

	fwprintf(m_pFile,PROCESSDATA_END);
	fwprintf(m_pFile, L"\n\n");
	fflush(m_pFile);

	m_stage = evOut_OK;

	return true;
}

// Description: This function writes a line under [PROCESSDATA] section
//
// Format of the line in [PROCESSDATA] section in version 6:
//		PID,TOTALSAMPLE,#EVENTSET,[CPU INDEX] #SAMPLE,32-BIT-FLAG,CSS-FLAG,
//		MODULE-NAME
// Note: this is only for Version 6 or higher.
bool CaProfileWriter::writeProcLineData(CA_Process & proc, PID_T pid)
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_TaskSummary) {
		m_err = E_UNKNOWN;
		return false;
	}	

	// PID : Note Linux uses taskID instead
	fwprintf (m_pFile, L"%u", pid);

	// TOTALSAMPLE 
	fwprintf (m_pFile, L",%lu",proc.getTotal());

	// number of EventSet
	fwprintf (m_pFile, L",%u",proc.getSampleMapSize());

	// [cpu EVENT-INDEX] #SAMPLE
	CA_SampleMap::const_iterator it = proc.getBeginSample();
	CA_SampleMap::const_iterator itEnd = proc.getEndSample();
	for (; it != itEnd; it++) {
		SampleKey key = it->first;

		if (it->second) {			
			map<EVMASK_T, int>::iterator eit;
			if ((eit = m_evToIndexMap.find(key.event)) == m_evToIndexMap.end()) {
				m_err = E_INVALIDPROCESSMAP;
				return false;
			}

			fwprintf(m_pFile, L",[%u %d] %u", key.cpu, eit->second, 
												it->second);
		}
	}

	// 32-bit-Flag
	fwprintf(m_pFile, L",%d", proc.m_is32Bit? 1 : 0);

	// CSSFlag 
	fwprintf(m_pFile, L",%d", proc.m_hasCss ? 1 : 0);

	// task name
	fwprintf(m_pFile, L",%ls\n", proc.getPath().c_str());

	return true;	
}


bool CaProfileWriter::writeProcSection(PidProcessMap * procMap)
{
	m_err = OK;

	if (!writeProcSectionProlog())
		return false;
			
	PidProcessMap::iterator it = procMap->begin();
	PidProcessMap::iterator it_end = procMap->end();
	for (; it != it_end; it++) {
		// Only writing out the one that has samples
		if (0 != (*it).second.getTotal()) {
			if(!writeProcLineData((*it).second, (*it).first))
				return false;
		}
	}

	if (!writeProcSectionEpilog())
		return false;

	return true;
}


bool CaProfileWriter::writeModSectionProlog()
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_OK)  {
		m_err = E_UNKNOWN;
		return false;
	}

	fwprintf(m_pFile, MODDATA_BEGIN);
	fwprintf(m_pFile, L"\n");
	fflush(m_pFile);

	m_stage = evOut_ModSummary;

	return true;
	
}
bool CaProfileWriter::writeModData(CA_Module & mod)
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_ModSummary) {
		m_err = E_UNKNOWN;
		return false;
	}

	// For each PID
	PidAggregatedSampleMap::const_iterator it = mod.getBeginSample();
	PidAggregatedSampleMap::const_iterator end = mod.getEndSample();
	for (; it != end; it++) {
		if(!writeModLineData(mod, it->first, it->second)) {
			m_err = E_INVALIDMODULEMAP;
			return false;
		}
	}
	return true;
}

bool CaProfileWriter::writeModLineData(CA_Module & mod,
								 PID_T pid,
								 const AggregatedSample & agSamp)
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_ModSummary) {
		m_err = E_UNKNOWN;
		return false;
	}

	// taskid 
	fwprintf (m_pFile, L"%u", pid);

	// TOTALSAMPLE
	fwprintf (m_pFile, L",%llu", agSamp.getTotal());

	// #EVENTSET
	fwprintf (m_pFile, L",%u",agSamp.getSampleMapSize());

	// [CPU EVENT-INDEX] #SAMPLE
	CA_SampleMap::const_iterator it = agSamp.getBeginSample();
	CA_SampleMap::const_iterator itEnd = agSamp.getEndSample();
	for (; it != itEnd; it++) {
		SampleKey key = (*it).first;
		if ((*it).second) {
			map<EVMASK_T, int>::iterator eit;
			if ((eit = m_evToIndexMap.find(key.event)) == m_evToIndexMap.end()) {
				m_err = E_INVALIDMODULEMAP;
				return false;
			}
			fwprintf(m_pFile, L",[%u %d] %u", key.cpu, eit->second, (*it).second);
		}
	}
	
	// 32-bit-Flag
	fwprintf(m_pFile, L",%d", mod.m_is32Bit? 1 : 0);

	// module name
	fwprintf(m_pFile, L",%ls\n", (wchar_t*)(mod.getPath().c_str()));	

	return true;
}

bool CaProfileWriter::writeModSectionEpilog()
{
	m_err = OK;

	if (!m_pFile) {
		m_err = E_OPENFAILED;
		return false;
	}

	if (m_stage != evOut_ModSummary) {
		m_err = E_UNKNOWN;
		return false;
	}

	fwprintf(m_pFile, MODDATA_END);
	fwprintf(m_pFile, L"\n\n");
	fflush(m_pFile);

	m_stage = evOut_OK;

	return true;
}


bool CaProfileWriter::writeModSectionAndImd(NameModuleMap * modMap)
{
	m_err = OK;

	if (!writeModSectionProlog())
		return false;
	
	/* 
	 * IMD File Naming Scheme: <index>.imd
	 * The index is assigned using the order of the module 
	 * listed in the [MODDATA] section of TBP/EBP file.
	 */
	NameModuleMap::iterator it = modMap->begin();
	NameModuleMap::iterator it_end = modMap->end();
	// Write out each module 
	for (int i = 0; it != it_end; it++) {
		// We won't write out IMD file if module
		// contains no sample.
		if (it->second.getTotal() <= 0)
			continue;
	
		int index = it->second.getImdIndex();
		if (index == -1) {
			index = i;
		}

		// IMD file path
#if ( defined (_WIN32) || defined (_WIN64) )
		int stop = m_path.rfind(L"\\");
		wstring imdPath = m_path.substr(0,stop);

		wchar_t buf[10] = {L'\0'};
		swprintf(buf, 10, L"\\%u", index);
#else
		int stop = m_path.rfind(L"/");
		wstring imdPath = m_path.substr(0,stop);

		wchar_t buf[10] = {L'\0'};
		swprintf(buf, 10, L"/%u", index);
#endif
		imdPath += buf;
		imdPath += L".imd";
				
		// Write Module section lines
		if(!writeModData((*it).second))
			return false;
		
		// Write IMD file
		ImdWriter imdWriter;

		if (!imdWriter.open(imdPath)) {
			m_err = E_IMD_OPENFAILED;
			return false;
		}
		
		if (!imdWriter.writeToFile((*it).second, &m_evToIndexMap)) {
			m_err = E_INVALIDMODULEDETAIL;
			return false;
		}
		
		imdWriter.close();
		i++;
	}
		
	if (!writeModSectionEpilog())
		return false;

	return true;
}
