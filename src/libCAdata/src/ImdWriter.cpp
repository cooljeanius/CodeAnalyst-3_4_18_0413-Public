#include "stdafx.h"
#include <string>
#include <map>
#include "CaDataWriter.h"
#include "Module.h"
#include "ImdWriter.h"

using namespace std;

ImdWriter::ImdWriter(): CaDataWriter()
{
}


ImdWriter::~ImdWriter()
{
	close();

}

bool ImdWriter::writeToFile(CA_Module & mod,
					map<EVMASK_T, int> * pEvToIndexMap)
{
	if (!m_pFile || !pEvToIndexMap)
		return false;

	m_pEvToIndexMap = pEvToIndexMap;

	if (!writeModDetailProlog(mod))
		return false;
	
	// For each [SUB] section
	AddrFunctionMultMap::const_iterator it = mod.getBeginFunction();
	AddrFunctionMultMap::const_iterator itEnd = mod.getEndFunction();
	for(; it != itEnd; it++) {
	
		if ((CA_Module::JAVAMODULE == mod.getModType()) ||
			(CA_Module::MANAGEDPE == mod.getModType())) 
		{
			CA_Function *func = (CA_Function*)&(it->second); 
			func->computeJavaAggregatedMetadata();
		}

		if (!writeSubProlog(it->second))
			return false;

		// For each sample
		AptAggregatedSampleMap::const_iterator ait = it->second.getBeginSample();
		AptAggregatedSampleMap::const_iterator aend = it->second.getEndSample();
		for (; ait != aend; ait++) {
			if (!writeModDetailLine(ait->first.m_pid,
						ait->first.m_tid,
						ait->first.m_addr - it->second.m_baseAddr, 
						ait->second))
				return false;
		}

		if (!writeSubEpilog())
			return false;
	}

	if (!writeModDetailEpilog())
		return false;

	return true;
}

bool ImdWriter::writeModDetailProlog(CA_Module & mod)
{
	if (!m_pFile)
		return false;

	if (m_stage != evOut_OK)
		return false;

	fwprintf (m_pFile, L"[%ls]\n", mod.getPath().c_str());
	fwprintf (m_pFile, L"SIZE=%u\n", mod.m_size);
	fwprintf (m_pFile, L"SAMPS=%llu\n", mod.getTotal());
	fwprintf (m_pFile, L"ModuleType=%d\n", mod.m_modType);
	fwprintf (m_pFile, L"NUMSUBSECTIONS=%u\n", mod.getNumSubSection() );	
	fflush(m_pFile);

	fflush(m_pFile);

	m_stage = evOut_ModDetail;

	return true;
}


// This function writes a line under [each module] section.
// The format is:
// TGID,TID,TOTALSAMPLE,#EVENTSET,[CPU INDEX] #SAMPLE,OFFSET,FUNCNAME,
//		FUNCBASEA+DDR,SRCFILE,JNCFILE 
//
// Note: Since verison 8, the JNCFile will only contain file name.
//		It's because the path can ge calculated by ebp file path + /jit/.
//	-Lei 08/06/2008.
// 
bool ImdWriter::writeModDetailLine(TGID_T tgid, 
								TID_T tid,
								VADDR sampAddr, 
								const AggregatedSample & agSamp)
{
	if (!m_pFile)
		return false;

	if (m_stage != evOut_ModDetail
	&&  m_stage != evOut_JitDetail)
		return false;

	// PID,TID: 
	fwprintf (m_pFile, L"%u,%u", tgid, tid);

	// total
	fwprintf (m_pFile, L",%lu", agSamp.getTotal());

	// event set number
	fwprintf(m_pFile, L",%u", agSamp.getSampleMapSize());

	CA_SampleMap::const_iterator it = agSamp.getBeginSample();
	CA_SampleMap::const_iterator it_end = agSamp.getEndSample();
	//[CPU EVENT-INDEX] #Sample
	for (; it!= it_end; it++) {
		SampleKey key = it->first;
		if (it->second) {
			map<EVMASK_T, int>::iterator eit;
			if ((eit = m_pEvToIndexMap->find(key.event)) == m_pEvToIndexMap->end())
				return false;

			fwprintf(m_pFile, L",[%u %d] %u", 
				key.cpu, eit->second, it->second);
		}
	}

	fwprintf(m_pFile, L",0x%llx\n", sampAddr);
	return true;
}

bool ImdWriter::writeModDetailEpilog()
{
	if (!m_pFile)
		return false;

	if (m_stage != evOut_ModDetail)
		return false;

	fwprintf(m_pFile,IMD_END);
	fwprintf(m_pFile, L"\n\n");
	fflush(m_pFile);

	m_stage = evOut_OK;

	return true;
}


bool ImdWriter::writeSubProlog(const CA_Function & func)
{
	if (!m_pFile)
		return false;

	if (m_stage != evOut_ModDetail)
		return false;

	fwprintf(m_pFile, L"\n");
	fwprintf(m_pFile,SUB_BEGIN);
	fwprintf(m_pFile, L"\n");
	if (!(func.getJncFileName()).empty())
		fwprintf(m_pFile, L"BINARYFILE=%ls\n", func.getJncFileName().c_str());
	if (!(func.getJavaSrcName()).empty())
		fwprintf(m_pFile, L"SRCFILE=%ls\n", func.getJavaSrcName().c_str());
	if (!func.getFuncName().empty())
		fwprintf(m_pFile, L"SYMBOL=%ls\n", func.getFuncName().c_str());
	if (func.m_baseAddr != 0)
	fwprintf(m_pFile, L"BASEADDR=0x%llx\n", func.m_baseAddr, 0);

	InlineInstanceList::const_iterator it = func.getBeginJil(); 
	InlineInstanceList::const_iterator itEnd = func.getEndJil(); 
	for (; it != itEnd; it++) {
		fwprintf(m_pFile, L"INSTANCE=%llx:%u:%s\n", 
							it->m_addr,
							it->m_size,
							it->m_symbol.c_str());
	}

	// For each pid/tid aggregated sample metadata
	AptAggregatedSampleMap::const_iterator ait = func.getBeginMetadata();
	AptAggregatedSampleMap::const_iterator aend = func.getEndMetadata();
	for (; ait != aend; ait++) {
		fwprintf(m_pFile, L"AGGREGATED=");
		if (!writeModDetailLine(ait->first.m_pid,
					ait->first.m_tid,
					ait->first.m_addr, 
					ait->second))
			return false;
	}

	fwprintf(m_pFile, L"LINECOUNT=%u\n", func.getSampleMapSize());
	fflush(m_pFile);

	m_stage = evOut_JitDetail;

	return true;
}


bool ImdWriter::writeSubEpilog()
{
	if (!m_pFile)
		return false;

	if (m_stage != evOut_JitDetail)
		return false;

	fwprintf(m_pFile,SUB_END);
	fwprintf(m_pFile, L"\n");
	fflush(m_pFile);

	m_stage = evOut_ModDetail;

	return true;
}
