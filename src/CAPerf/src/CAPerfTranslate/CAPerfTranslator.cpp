
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>

#ifdef HAVE_LIBELF_GELF_H
#include <libelf/gelf.h>
#else
#include <gelf.h>
#endif // HAVE_LIBELF_GELF_H

#include "linux/perf_event.h"
#include "linux/perfStruct.h"

#include "CommonProjects_typedefs.h"
#include "CaProfileWriter.h"
#include "PerfDataReader.h"
#include "CAPerfDataReader.h"
#include "CAPerfTranslator.h"
#include "CAPerfData.h"
#include "CaProfileInfo.h"
#include "calog.h"

#if HAS_LIBCSS
#include "CSSTranslator.h"
#endif

/* NOTE: [Suravee]
 * 1: Enable module aggregation caching
 * 0: Disable
 */
#define _ENABLE_MOD_AGG_CACHED_ 1

CAPerfTranslator::CAPerfTranslator()
{
	_init();
}


CAPerfTranslator::CAPerfTranslator(string perfDataPath)
{
	_init();
	m_inputFile = perfDataPath;
}

void CAPerfTranslator::_init()
{
	m_pCurProc = NULL;
	m_pPerfDataRdr = NULL;
	m_cachedMod = m_modLoadInfoMap.rend(); 
	m_cachedPid = 0;
	m_bVerb = false;

	// Initialize the record handler dispatcher
	memset(&m_handlers, 0, 
		sizeof(PerfRecordHandler_t) * PERF_RECORD_MAX);

	m_handlers[PERF_RECORD_EXIT] = 
		(PerfRecordHandler_t) &CAPerfTranslator::process_PERF_RECORD_EXIT;	

	m_pLogFile = NULL;

	m_pCalogCss = NULL;	

	m_numComm = 0;
	m_numMmap = 0;
	m_numSamples = 0;
	m_numUnknownSamples = 0;
	m_numUnknownKernSamples = 0;
	timerclear(&m_pass1Start);
	timerclear(&m_pass1Stop);
	timerclear(&m_pass2Start);
	timerclear(&m_pass2Stop);
}


CAPerfTranslator::~CAPerfTranslator()
{
	if (m_pPerfDataRdr) {
		m_pPerfDataRdr->deinit();
		delete m_pPerfDataRdr;
		m_pPerfDataRdr = NULL;
	}
	
	if (m_pLogFile) {
		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}
	
	if (m_pCalogCss) {
		calog_close(&m_pCalogCss);
		m_pCalogCss = NULL;
	}

	if (!m_cssFileDir.empty()) {
#if HAS_LIBCSS
		fnWriteCallSiteFiles((wchar_t*) m_cssFileDir.c_str());
#endif
	}
}


int CAPerfTranslator::dumpPerfData(string perfDataPath)
{
	int retVal = S_OK;

	// Sanity check
	if (perfDataPath.empty())
		return E_INVALID_PATH;

	// Initialize the reader
	if (!m_pPerfDataRdr 
	&&  S_OK != (retVal = _setupReader(perfDataPath)))
		return retVal; 

	m_pPerfDataRdr->dumpHeaderSections();
	m_pPerfDataRdr->dumpData();

	m_pPerfDataRdr->deinit();

	return S_OK;
}


int CAPerfTranslator::translatePerfDataToCaData (
	string perfDataPath,
	bool bVerb)
{
	int retVal = S_OK;
	struct perf_event_header hdr;
	const void * pBuf = NULL;
	unsigned int numCpus = 0;

	// Sanity check
	if (perfDataPath.empty()) 
	{
		if (m_inputFile.empty())
			return E_INVALID_PATH;
	} else {	
		m_inputFile = perfDataPath;
	}

	// Initialize the reader
	if (!m_pPerfDataRdr 
	&&  S_OK != (retVal = _setupReader(m_inputFile)))
		return retVal; 

	m_bVerb = bVerb;
	if (m_bVerb)
		m_pPerfDataRdr->dumpHeaderSections();

	m_sampleType = m_pPerfDataRdr->getAttrSampleType();

	if ((numCpus = m_pPerfDataRdr->getNumCpus()) == 0)
		return E_PERFDATAREADER_FAILED;

	if (m_pLogFile) {
		fprintf(m_pLogFile, "Translating: %s\n", m_inputFile.c_str());
	}

	// Setup the process for storing vmlinux and kernel modules
	CA_Process * pProc = getProcess (-1);
	if (!pProc)
		return E_FAILED;

	//-------------------------------------------------------
	// Process the data PASS1 (for COMM and MMAP)

	gettimeofday(&m_pass1Start, NULL);

	m_handlers[PERF_RECORD_MMAP] = 
		(PerfRecordHandler_t) &CAPerfTranslator::process_PERF_RECORD_MMAP;

	m_handlers[PERF_RECORD_COMM] = 
		(PerfRecordHandler_t) &CAPerfTranslator::process_PERF_RECORD_COMM;
	
	if (m_pPerfDataRdr->getFirstRecord(&hdr, &pBuf) != S_OK)
		return E_PERFDATAREADER_FAILED;

	do {
		if (hdr.size == 0 || !pBuf) {
			retVal = E_PERFDATAREADER_FAILED;
			break;
		}
		if (m_handlers[hdr.type] != NULL) {
			(this->*(m_handlers[hdr.type])) (&hdr, pBuf);
		}
	
	} while	(m_pPerfDataRdr->getNextRecord(&hdr, &pBuf) == S_OK);

#if 0
	// Debug module loading info
	fprintf(stdout, "DEBUG: Num of modules = %d\n", m_modLoadInfoMap.size());
	ModLoadInfoMap::iterator it = m_modLoadInfoMap.begin();	
	ModLoadInfoMap::iterator itEnd = m_modLoadInfoMap.end();
	for (; it != itEnd; it++) {
		fprintf(stdout, "DEBUG: pid:%08d, time:0x%016llx, addr:0x%016llx, len:0x%08llx, pgoff:0x%08llx, filename:%s (0x%llx)\n",
			it->first.pid, it->first.time, it->first.addr, it->second.len,
			it->second.pgoff, it->second.name.c_str(),
			it->second.pProc);
	}	
#endif

	gettimeofday(&m_pass1Stop, NULL);

	if (m_pLogFile) {
		struct timeval diff;
		timersub(&m_pass1Stop, &m_pass1Start, &diff);
		fprintf(m_pLogFile, "Pass1 Time                 : %u sec, %u usec\n", diff.tv_sec, diff.tv_usec);
	}

	//-------------------------------------------------------
	// Process the data PASS2 (for SAMPLE)

	gettimeofday(&m_pass2Start, NULL);

	m_handlers[PERF_RECORD_MMAP] = NULL;
	m_handlers[PERF_RECORD_COMM] = NULL; 
	m_handlers[PERF_RECORD_SAMPLE] = 
		(PerfRecordHandler_t) &CAPerfTranslator::process_PERF_RECORD_SAMPLE;

	if (m_pPerfDataRdr->getFirstRecord(&hdr, &pBuf) != S_OK)
		return E_PERFDATAREADER_FAILED;

	do {
		if (hdr.size == 0 || !pBuf) {
			retVal = E_PERFDATAREADER_FAILED;
			break;
		}
		if (m_handlers[hdr.type] != NULL) {
			(this->*(m_handlers[hdr.type])) (&hdr, pBuf);
		}
	
	} while	(m_pPerfDataRdr->getNextRecord(&hdr, &pBuf) == S_OK);
	
	gettimeofday(&m_pass2Stop, NULL);
	
	if (m_pLogFile) {
		struct timeval diff;
		timersub(&m_pass2Stop, &m_pass2Start, &diff);
		fprintf(m_pLogFile, "Pass2 Time                 : %u sec, %u usec\n", diff.tv_sec, diff.tv_usec);

		fprintf(m_pLogFile, "Num Comm                   : %u\n", m_numComm);
		fprintf(m_pLogFile, "Num Mmap                   : %u\n", m_numMmap);
		fprintf(m_pLogFile, "Num Samples                : %u\n", m_numSamples);
		fprintf(m_pLogFile, "Num Unknown Samples        : %u\n", m_numUnknownSamples);
		fprintf(m_pLogFile, "Num Unknown Kernel Samples : %u\n", m_numUnknownKernSamples);
	}

	return retVal;
} // CAPerfTranslator::translatePerfDataToCaData



ModLoadInfoMap::reverse_iterator 
CAPerfTranslator::_getModuleForSample( 
	uint32_t pid, uint64_t time, uint64_t ip,
	bool bIsUser)
{
	// Look for module with the corresponded 
	// ip and timestamp
	ModLoadInfoMap::reverse_iterator rit = m_modLoadInfoMap.rbegin();
	ModLoadInfoMap::reverse_iterator rend = m_modLoadInfoMap.rend();

	if (!pid || !time || !ip)
		return rend;


	if (!bIsUser) {
		ModKey key(time, ip, -1);
		for (; rit != rend; rit++) {
			if (-1 != rit->first.pid)
				continue;

			if (rit->first < key || rit->first == key) {
				break;
			}
		}
	} else {
		ModKey key(time, ip, pid);
		for (; rit != rend; rit++) {

			if (pid != rit->first.pid)
				continue;

			if (rit->first < key || rit->first == key) {
				break;
			}
		}
	}
	
	if (rit == rend)
		return rend;

	// Also check the len
	uint64_t endRng = rit->first.addr + rit->second.len;

	// NOTE: This is a hack to handle the MMAP of kernel.kallsyms stuff;
	//       MMAP: time:0x0000000000000000, addr:0x0000000000000000, pid:-1, tid:0, 
	//             len:0xffffffff9fffffff, pgoff:0xffffffff81000000, filename:[kernel.kallsyms]_text
	if (rit->first.pid == -1 && rit->second.len > rit->first.addr)
		endRng = -1ULL;

	if (ip >= endRng) {
		return rend;
	}

	return rit;
		
}


CA_Module * CAPerfTranslator::getModule(string modName) 
{
	CA_Module *pRet = NULL;
	wstring wModName (modName.begin(), modName.end());

	NameModuleMap::iterator it = m_modMap.find(wModName);
	if (it != m_modMap.end()) {
		pRet = &(it->second);
	} else {
		CA_Module mod;
		mod.setPath(wModName);

		// Find the bitness of the module
		bool is32Bit = false;
		_getModuleBitness(modName, &is32Bit);
		mod.m_is32Bit = is32Bit;

		m_modMap.insert(NameModuleMap::value_type(wModName, mod));
	
		// Re-find	
		it = m_modMap.find(wModName);
		if (it != m_modMap.end())
			pRet = &(it->second);
	}

	return pRet;
}


CA_Process * CAPerfTranslator::getProcess(int pid) 
{
	CA_Process *pRet = NULL;

	PidProcessMap::iterator it = m_procMap.find(pid);
	if (it != m_procMap.end()) {
		pRet = &(it->second);
	} else {
		CA_Process proc;
		m_procMap.insert(PidProcessMap::value_type(pid, proc));
	
		// Re-find	
		it = m_procMap.find(pid);
		if (it != m_procMap.end())
			pRet = &(it->second);
	}

	return pRet;
}


int CAPerfTranslator::process_PERF_RECORD_MMAP(
	struct perf_event_header *pHdr, 
	void * ptr)
{
	if (!ptr)
		return E_FAILED_PROCESS_RECORD;

	struct CA_PERF_RECORD_MMAP * pRec= (struct CA_PERF_RECORD_MMAP *) ptr;
	char * pCur = (char *) ptr;
	
	// Skip pid + tid + addr + len + pgoff
	pCur += 4 + 4 + 8 + 8 + 8;

	// Skip filename. 
	int len = strlen(pRec->filename) + 1;  
	int tmp = len % 8;
	if (tmp != 0)
		len += (8 - tmp);
	pCur += len;

	CA_PERF_RECORD_SAMPLE samp;
	parse_perf_event_sample(pCur, &samp); 


	if (m_bVerb) {
		fprintf(stdout, "MMAP: time:0x%016llx, addr:0x%016llx, pid:%d, tid:%d, "
			"len:0x%016llx, pgoff:0x%016llx, filename:%s\n",
			samp.time, pRec->addr, pRec->pid, pRec->tid, 
			pRec->len, pRec->pgoff, pRec->filename ); 
	}

	// Store module loading info
	ModKey key(samp.time, pRec->addr + pRec->pgoff, pRec->pid);

	ModInfo info(pRec->len, pRec->pgoff, pRec->filename);

	info.pMod = getModule(pRec->filename);
	if (info.pMod && m_pCurProc) {
		// Since module and process must have the same bitness
		m_pCurProc->m_is32Bit = info.pMod->m_is32Bit;
		info.pProc = m_pCurProc;
	}

	info.pMod->m_modType= CA_Module::UNMANAGEDPE;

	m_modLoadInfoMap.insert(ModLoadInfoMap::value_type(key, info)); 
		
	m_numMmap++;

	return S_OK;
}


int CAPerfTranslator::parse_perf_event_sample(
	void * ptr, 
	void * pSamp)
{
	if (!ptr || !pSamp)
		return E_FAILED;

	char * pCur = (char *) ptr;
	
	if (m_sampleType & PERF_SAMPLE_TID) {
		((CA_PERF_RECORD_SAMPLE *)pSamp)->pid = *((u32*) pCur);
		pCur += 4;
		((CA_PERF_RECORD_SAMPLE *)pSamp)->tid = *((u32*) pCur);
		pCur += 4;
	}
	
	if (m_sampleType & PERF_SAMPLE_TIME) {
		((CA_PERF_RECORD_SAMPLE *)pSamp)->time = *((uint64_t *) pCur);
	}

	// TODO .... 

	return S_OK;	
}


int CAPerfTranslator::process_PERF_RECORD_COMM(
	struct perf_event_header *pHdr, 
	void * ptr)
{
	if (!ptr)
		return E_FAILED_PROCESS_RECORD;
	
	struct CA_PERF_RECORD_COMM * pRec= (struct CA_PERF_RECORD_COMM *) ptr;
	char * pCur = (char *) ptr;

	// Skip pid and tid
	pCur += (4 + 4);

	// Skip comm. comm is 64-bit align and 
	// max length at TASK_COMM_LEN byte
	int len = strlen(pRec->comm) + 1; 
	if (len <= 8)
		pCur += 8;
	else
		pCur += TASK_COMM_LEN;

	CA_PERF_RECORD_SAMPLE samp;
	parse_perf_event_sample(pCur, &samp); 
	

	if (m_bVerb) {
		fprintf(stdout, "COMM: time:0x%016llx, pid:%d, tid:%d, comm:%s\n", 
				samp.time, pRec->pid, pRec->tid, pRec->comm);
	}

	CA_Process * pProc = getProcess (pRec->pid);
	if (!pProc)
		return E_FAILED_PROCESS_RECORD;

	// Set process name
	string procName(pRec->comm);
	wstring wProcName (procName.begin(), procName.end());
	pProc->setPath(wProcName);

	m_pCurProc = pProc;	
	m_numComm++;

	return S_OK;
} // CAPerfTranslator::process_PERF_RECORD_COMM


ModLoadInfoMap::reverse_iterator
CAPerfTranslator::getModuleForSample(
	struct perf_event_header *pHdr,  
	uint32_t pid, uint64_t time, uint64_t ip,
	bool bStat)
{
	ModLoadInfoMap::reverse_iterator modRit = m_modLoadInfoMap.rend();

	bool bIsUser = pHdr->misc == PERF_RECORD_MISC_USER; 
	modRit = _getModuleForSample( pid, time, ip, bIsUser);
	if (modRit == m_modLoadInfoMap.rend()) {
		if (bIsUser) {
			if (m_bVerb) 
				fprintf(stdout, 
					"\nWarning: Unknown Sample: ip:0x%016llx, "
					"time:0x%016llx\n",
					ip, time);
			if (bStat)
				m_numUnknownSamples++;
		} else {
			if (m_bVerb) 
				fprintf(stdout, 
					"\nWarning: Unknown Kernel Sample: "
					"ip:0x%016llx, time:0x%016llx\n",
					ip, time);
			if (bStat)
				m_numUnknownKernSamples++;
		}
	}

	return modRit;
}


int CAPerfTranslator::process_PERF_RECORD_SAMPLE(
	struct perf_event_header *pHdr, 
	void * ptr)
{
	struct CA_PERF_RECORD_SAMPLE rec;
	char * pCur = (char* )ptr;
	
	memset(&rec, 0, sizeof(struct CA_PERF_RECORD_SAMPLE));
	
	if (!ptr)
		return E_FAILED_PROCESS_RECORD;

	if (m_bVerb) {
		fprintf(stdout, "SAMP:");
	}

	if (m_bVerb && m_sampleType & PERF_SAMPLE_TIME) {
		fprintf(stdout, " time:0x%016llx",
			*((u64 *)(pCur + 8 + 4 + 4)));
	}

	if (m_sampleType & PERF_SAMPLE_IP) {
		rec.ip = *((u64 *)pCur);
		pCur += 8;
		if (m_bVerb)
			fprintf(stdout, ", ip:0x%016llx", rec.ip);
	}
	
	if (m_sampleType & PERF_SAMPLE_TID) {
		rec.pid = *((u32 *)pCur);
		pCur += 4;
		if (m_bVerb)
			fprintf(stdout, ", pid:%d", rec.pid);
		rec.tid = *((u32 *)pCur);
		pCur += 4;
		if (m_bVerb)
			fprintf(stdout, ", tid:%d", rec.tid);
	}

	if (m_sampleType & PERF_SAMPLE_TIME) {
		rec.time = *((u64 *)pCur);
		pCur += 8;
//		fprintf(stdout, ", time:0x%016llx", rec.time);
	}

	if (m_sampleType & PERF_SAMPLE_ADDR) {
		rec.addr = *((u64 *)pCur);
		pCur += 8;
		if (m_bVerb) 
			fprintf(stdout, ", addr:0x%016llx", rec.ip);
	}
	
	if (m_sampleType & PERF_SAMPLE_ID) {
		rec.id = *((u64 *)pCur);
		pCur += 8;
		if (m_bVerb) 
			fprintf(stdout, ", id:0x%016llx", rec.id);
	}
		
	if (m_sampleType & PERF_SAMPLE_CPU) {
		rec.cpu = *((u32 *)pCur);
		pCur += 4;

		// This is for the "res" as list in teh perf_event.h
		pCur += 4;

		if (m_bVerb)
			fprintf(stdout, ", cpu:0x08%x", rec.cpu);
	}
		
	if (m_sampleType & PERF_SAMPLE_PERIOD) {
		rec.period = *((u64 *)pCur);
		pCur += 8;
		if (m_bVerb)
			fprintf(stdout, ", period:0x%016llx", rec.period);
	}
		
	if (m_sampleType & PERF_SAMPLE_STREAM_ID) {
		rec.stream_id = *((u64 *)pCur);
		pCur += 8;
		if (m_bVerb)
			fprintf(stdout, ", stream_id:0x%016llx", rec.stream_id);
	}

	if (m_sampleType & PERF_SAMPLE_RAW) {
		rec.raw_size = *((u32*)pCur);
		pCur += 4;
		
		rec.raw_data = (void *)pCur;
		if (m_bVerb) {
			fprintf(stdout, ", raw_data[%u]: ", rec.raw_size);
			for (int i = 0 ; i < rec.raw_size; i++) {
				fprintf(stdout, "%x", (u32*) pCur);
				pCur += 1;
			}
		}
	}

	if (m_sampleType & PERF_SAMPLE_CALLCHAIN) {
		rec.callchain = (struct ip_callchain *)pCur;
		pCur += sizeof(uint64_t) * (rec.callchain->nr + 1);
		if (m_bVerb)
			fprintf(stdout, ", nr_callchain:%03llu", rec.callchain->nr);
	} else {
		rec.callchain = NULL;
	}

	
	if (m_bVerb)
		fprintf(stdout, " hdr.misc:0x%x", pHdr->misc);

	ModLoadInfoMap::reverse_iterator modRit;
#if _ENABLE_MOD_AGG_CACHED_
	/* MODULE AGGREGATION CACHING:
	 * We cache the pid and module that matched the last sample.
	 * This greatly help reducing the data procesing time.
	 */

	bool bCached = false;
	//Check if sample belong to the last cached module
	if (rec.pid  == m_cachedPid
	&&  rec.time >= m_cachedMod->first.time
	&&  rec.ip   >= m_cachedMod->first.addr 
	&& (rec.ip < m_cachedMod->first.addr + m_cachedMod->second.len 
		|| rec.ip < m_cachedMod->second.len))
	{
		modRit = m_cachedMod;
		bCached = true;
	} else 
#endif // _ENABLE_MOD_AGG_CACHED_
	{
		modRit = getModuleForSample(pHdr, rec.pid, rec.time, rec.ip, true);
		if (modRit == m_modLoadInfoMap.rend())
			return E_FAILED_GET_MODULE_FOR_SAMPLE;

		m_cachedPid = rec.pid;
		m_cachedMod = modRit;
	}


	wstring wModName = modRit->second.pMod->getPath();
	string modName;
	modName.assign(wModName.begin(), wModName.end());

	if (m_bVerb)
	{
		fprintf(stdout, ", MmapTime:0x%016llx, MmapAddr:0x%016llx, MmapLen:0x%016llx, (%s)", 
			modRit->first.time, modRit->first.addr, modRit->second.len, modName.c_str());
		if (bCached)	
			fprintf(stdout," (CACHED)");
		fprintf(stdout, "\n");

		if (rec.callchain) {
			for (int i = 0; i < rec.callchain->nr; i++)
			{
				fprintf(stdout, "callchain %03d:0x%016llx\n", i ,rec.callchain->ips[i]);
			}
		}
	}

	int cpu = 0;
	uint32_t event;
	uint32_t umask;
	uint32_t os;
	uint32_t usr;

	if (!m_pPerfDataRdr)
		return E_PERFDATAREADER_FAILED;	

	if (S_OK != m_pPerfDataRdr->getEventAndCpuFromSampleId(rec.id, &cpu, &event, &umask, &os, &usr))
		return E_INVALID_EVENT_ID;
	
	EVMASK_TENC evMaskEnc;
	evMaskEnc.ucEventSelect = event;
	evMaskEnc.ucUnitMask = umask;
	evMaskEnc.bitOsEvents = os;
	evMaskEnc.bitUsrEvents = usr;
	evMaskEnc.bitReserved = 0;

	EVMASK_T evMask = 0;
	memcpy (&evMask, &evMaskEnc, sizeof(EVMASK_TENC));

	// Add sample to process
	if (modRit->second.pProc) {
		SampleKey key(cpu, evMask);
		modRit->second.pProc->addSamples(key, 1);
	}

	// Add sample to this module
	unsigned int elfType = 0;
	if (S_OK != _getElfFileType(modName.c_str(), &elfType))
		return E_FAILED;

	if (elfType == ET_EXEC) {
		SampleInfo sampInfo (rec.ip, rec.pid, rec.tid, cpu, evMask);

		modRit->second.pMod->recordSample (sampInfo, 1, 0,
			wstring(L""), wstring(L""), wstring(L""));
	} else {
		SampleInfo sampInfo (rec.ip - modRit->first.addr, rec.pid, rec.tid, cpu, evMask);

		modRit->second.pMod->recordSample (sampInfo, 1, 0, 
			wstring(L""), wstring(L""), wstring(L""));
	}

	if (rec.callchain && m_pCalogCss) 
	{
		//------------------------------------------
		// Write write out CSS sample records
		calog_data entry;
		cacss_data data;

		/* Setup entry */
		entry.app_cookie = 0; // Not used
		entry.cpu        = cpu;
		entry.tgid       = rec.pid;
		entry.tid        = rec.tid;
		entry.cnt        = 1;

		for (int i = 1; i < rec.callchain->nr; i++)
		{
			// We need to setup cookie and offset
			// accordingly
			ModLoadInfoMap::reverse_iterator rit;
			entry.cookie     = 0;
			entry.offset     = 0;

			UINT64 tmpIp = rec.callchain->ips[i];

			if (rec.pid  == m_cachedPid
			&&  rec.time >= m_cachedMod->first.time
			&&  tmpIp >= m_cachedMod->first.addr 
			&& (tmpIp < m_cachedMod->first.addr + m_cachedMod->second.len 
				|| tmpIp < m_cachedMod->second.len))
			{
				rit = modRit;
				// Use MMAP time as app cookie 
				entry.cookie = rit->first.time; 
				entry.offset = tmpIp - rit->first.addr;
			} else {
				// NOTE [Suravee]: 
				// If we could cache this,
				// it would be much faster.
				rit = getModuleForSample(
					pHdr, rec.pid, rec.time, tmpIp, false);
				if (rit != m_modLoadInfoMap.rend()) {
					// Use MMAP time as app cookie 
					entry.cookie = rit->first.time; 
					entry.offset = tmpIp - rit->first.addr;
				} else {
					continue;
				}
			}

			/* Setup data */
			if (i == 1)
				data.cg_type = 1;
			else
				data.cg_type = 0;

			if (0 != calog_add_data(m_pCalogCss,
					&entry,
					&data,
					sizeof(cacss_data),
					rit->second.name.c_str(),
					NULL))
				break;
		}
	} else if (rec.callchain && !m_cssFileDir.empty()) {
		//TODO:
#if HAS_LIBCSS
		wstring wstr(modRit->second.name.begin(), modRit->second.name.end());

		for (int i = 1; i < rec.callchain->nr; i++)
		{
			fnAddProcMod(
				rec.pid, // pid, 
				modRit->first.addr, // modLoadAddr, 
				(wchar_t*) wstr.c_str()); // pModName);

			// FIXME [Suravee]: Need to really populate the calleeSite
			fnAddCallInfo(
				rec.pid, // pid
				rec.tid, // tid
				(rec.callchain->ips[i] - modRit->first.addr), // callerSite
				0, // calleeSite
				(i == 1)? true: false); // bSelf
		}
#endif
	}


	m_numSamples++;

	return S_OK;
		
} // CAPerfTranslator::process_PERF_RECORD_SAMPLE


int CAPerfTranslator::process_PERF_RECORD_EXIT(
	struct perf_event_header *pHdr, 
	void * ptr)
{
	// TODO: Do we need to do anything here?
	return S_OK;
}


int CAPerfTranslator::writeEbpOutput(string outputFile)
{
	bool bRet;
	CaProfileWriter	profWriter;
	int numMod = 0;
	const PerfEventAttrVec * pAttrVec;
	struct timeval timerStart;
	struct timeval timerStop;
	
	gettimeofday(&timerStart, NULL);

	if (!m_pPerfDataRdr
	||  (pAttrVec = m_pPerfDataRdr->getPerfEventAttrVec()) == NULL)
		return E_PERFDATAREADER_FAILED;

#if 0
	// Debug process info
	fprintf(stdout, "DEBUG: writeEbpOutput: Num of processes = %d\n", m_procMap.size());
	PidProcessMap::iterator pIt = m_procMap.begin();	
	PidProcessMap::iterator pEnd = m_procMap.end();
	for (; pIt != pEnd; pIt++) {
		
		wstring wstr = pIt->second.getPath();
		string str;
		str.assign(wstr.begin(), wstr.end());
		fprintf(stdout, "DEBUG: pid:%d comm:%s numSamples:%llu\n",
			pIt->first, str.c_str(), pIt->second.getTotal());
	}
#endif

	// We need to count the number of modules which contains samples
	NameModuleMap::iterator it = m_modMap.begin();
	NameModuleMap::iterator itEnd = m_modMap.end();
	for (; it != itEnd; it++) {
		if (it->second.getTotal() != 0)
			numMod++;	
	}

	unsigned int family = 0;
	unsigned int model = 0;
	if ( S_OK != m_pPerfDataRdr->getCpuInfo(&family, &model))
		return E_FAILED;

	m_profInfo.m_numCpus    = m_pPerfDataRdr->getNumCpus();
	m_profInfo.m_cpuFamily  = family;
	m_profInfo.m_cpuModel   = model;
	m_profInfo.m_numEvents  = m_pPerfDataRdr->getNumEvents();
	m_profInfo.m_numModules = numMod;

	////////////////////////////////
	// Get total number of sample
	m_profInfo.m_numSamples = 0;
	PidProcessMap::iterator pit  = m_procMap.begin();
	PidProcessMap::iterator pend = m_procMap.end();
	for (; pit != pend; pit++) {
		m_profInfo.m_numSamples += pit->second.getTotal();
	}
	
	m_profInfo.m_numMisses = 0; // This is not available

	////////////////////////////////	
	// Get time.
	time_t wrTime = time(NULL);
	string str(ctime(&wrTime));
	m_profInfo.m_timeStamp = wstring(str.begin(), str.end());

	// Remove the newline at the end
	if (*(m_profInfo.m_timeStamp.rbegin()) == L'\n')
		m_profInfo.m_timeStamp.resize(m_profInfo.m_timeStamp.size() -1 ); 

	/////////////////////////////////
	// Get Events
	for (int i = 0 ; i < m_profInfo.m_numEvents; i++ ) {
	
		uint32_t event;
		uint32_t umask;
		m_pPerfDataRdr->getEventInfoFromEvTypeAndConfig(
			(*pAttrVec)[i].type, (*pAttrVec)[i].config, 
			&event, &umask);

		EVMASK_TENC evMaskEnc;
		evMaskEnc.ucEventSelect = event;
		evMaskEnc.ucUnitMask = umask;
		evMaskEnc.bitOsEvents = !(*pAttrVec)[i].exclude_kernel;
		evMaskEnc.bitUsrEvents = !(*pAttrVec)[i].exclude_user;
		evMaskEnc.bitReserved = 0;

		EVMASK_T evMask = 0;
		memcpy (&evMask, &evMaskEnc, sizeof(EVMASK_TENC));
	
		EventEncodeType ev(evMask , (*pAttrVec)[i].sample_period, 0);
		m_profInfo.m_eventVec.push_back(ev);
	}

	bRet = profWriter.write(wstring(outputFile.begin(), outputFile.end())	
				, &m_profInfo, &m_procMap, &m_modMap);

	gettimeofday(&timerStop, NULL);
	
	if (m_pLogFile) {
		struct timeval diff;
		timersub(&timerStop, &timerStart, &diff);
		fprintf(m_pLogFile, "Write EBP time             : %u sec, %u usec\n", diff.tv_sec, diff.tv_usec);
	}

	if (bRet)
		return 0;
	else
		return 1;
} //CAPerfTranslator::writeEbpOutput


int CAPerfTranslator::_setupReader(string perfDataPath)
{
	if (m_pPerfDataRdr)
		delete m_pPerfDataRdr;
		
	// First, try using PerfDataReader
	m_pPerfDataRdr = new PerfDataReader();
	if (!m_pPerfDataRdr)
		return E_PERFDATAREADER_FAILED;

	if (S_OK == m_pPerfDataRdr->init(perfDataPath))
		return S_OK;

	delete m_pPerfDataRdr;

	// Retry using CAPerfDataReader
	m_pPerfDataRdr = new CAPerfDataReader();
	if (!m_pPerfDataRdr)
		return E_PERFDATAREADER_FAILED;
	
	if (S_OK == m_pPerfDataRdr->init(perfDataPath))
		return S_OK;
	
	delete m_pPerfDataRdr;
		
	return E_FAILED;
}


int CAPerfTranslator::setupLogFile(string logFile)
{
	if (m_pLogFile) {
		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}

	m_pLogFile = fopen(logFile.c_str(), "w");

	if (!m_pLogFile)
		return E_FAILED;

	fprintf(m_pLogFile, "====================\n");
	fprintf(m_pLogFile, "CAPerfTranslator Log\n");
	fprintf(m_pLogFile, "====================\n");
	return S_OK;	
}


int CAPerfTranslator::setupCalogCssFile(string file)
{
	if (m_pCalogCss) {
		calog_close(&m_pCalogCss);
		m_pCalogCss = NULL;
	}

	m_pCalogCss = calog_init(file.c_str(), CALOG_CACSS);
	if (!m_pCalogCss)
		return E_FAILED;

	return S_OK;	
}


int CAPerfTranslator::setupCssFile(string file)
{
	m_cssFileDir = wstring(file.begin(), file.end());

	return S_OK;
}


// Note [Suravee]: 
// This can only be used with module name since PERF doesn't
// always return fullpath as proceess name in COMM            
int CAPerfTranslator::_getModuleBitness(string modName, bool *pIs32Bit)
{
	int fd;
	int elfClass;
	Elf *e = NULL;
	int ret = E_FAILED;

	if (! pIs32Bit) {
		return E_INVALID_ARGUMENTS;
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		goto unknown_binary_type;
	}

	if (0 > ( fd = open (modName.c_str(), O_RDONLY, 0))) {
		goto unknown_binary_type;
	}

	if(( e = elf_begin(fd, ELF_C_READ, NULL)) != NULL )
	{
		elfClass = gelf_getclass(e);
		*pIs32Bit = (ELFCLASS32 == elfClass);
		ret = S_OK;
		elf_end(e);
	}
	close(fd);

unknown_binary_type:

	if (ret != S_OK) {
		// Note [Suravee]:
		// This is normally when :
		// - The modName is not a fullpath.
		// - The modName has (deleted) appended.
		// - [kernel.kallsyms]

		// Use system bitness as default
		struct utsname buf;
		if (0 != uname(&buf)) {
			return E_FAILED;	
		}

		// Compare
		if (strncmp("x86_64", buf.machine, 6) != 0) {
			*pIs32Bit = true;
		} else {
			*pIs32Bit = false;
		}
		ret = S_OK;
	}

	return ret;
}

int CAPerfTranslator::_getElfFileType(string modName, unsigned int *pElfType)
{
	int ret = E_FAILED;

	if (! pElfType) {
		return E_INVALID_ARGUMENTS;
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		return E_FAILED;
	}

	int fd;
	if (0 > ( fd = open (modName.c_str(), O_RDONLY, 0))) {
		// Note: This is often for [kernel.kallsyms] and deleted stuff.
		// fprintf (stderr, "ERROR : Failed to open file : %s\n", modName.c_str());
		// fprintf (stderr, "      : bfd_openr error : %s\n", msg.c_str());
		return E_FAILED;
	}

	Elf *e = elf_begin(fd, ELF_C_READ, NULL);
	if (e != NULL) {
		
		int elfClass = gelf_getclass(e);
		bool bIs32Bit = (ELFCLASS32 == elfClass);

		GElf_Ehdr elfHdr;
		if (NULL != gelf_getehdr(e, &elfHdr)) {
			if (bIs32Bit) {
				Elf32_Ehdr * p32 = (Elf32_Ehdr*) &elfHdr;
				*pElfType = p32->e_type;
			} else {
				Elf64_Ehdr * p64 = (Elf64_Ehdr*) &elfHdr;
				*pElfType = p64->e_type;
			}
			
			ret = S_OK;
		}
		elf_end(e);
	}
	close(fd);

}
