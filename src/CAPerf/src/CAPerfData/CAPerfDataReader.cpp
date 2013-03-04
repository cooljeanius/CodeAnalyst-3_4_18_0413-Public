// CAPerfDataReader.cpp
//

#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/perf_event.h>

#include "CAPerfDataReader.h"
#include "CAError.h"

extern const char * _ca_perfRecNames[];

CAPerfDataReader::CAPerfDataReader() : PerfDataReader()
{
	clear();
}


CAPerfDataReader::~CAPerfDataReader()
{
	deinit();
}


void 
CAPerfDataReader::clear()
{
	memset(&m_fileHeader, 0, sizeof(m_fileHeader));

	m_numSections = 0;
	m_pSectionHdrs = NULL;

	m_numEvents = 0;
	m_pEventCfg = NULL;

	m_numSampleIds = 0;
	m_pSampleIds = NULL;
}


int 
CAPerfDataReader::init(std::string filepath)
{
	int ret;

	// if the file does not exist return error
	if (access(filepath.c_str(), F_OK)) {
		return E_FILE_ALREADY_EXISTS;
	}

	// Open the file in read mode.
	m_fd = open(filepath.c_str(), O_RDONLY);
	if (-1 == m_fd) {
		return E_FAIL;
	}

	// read the caperf.data header
	ret = readHeader();
	if (S_OK != ret) {
		goto error_exit;
	}

	// read the section headers
	ret = readSectionHdrs();
	if (S_OK != ret) {
		goto error_exit;
	}

	// read and process the various sections
	for (int i=0; i<m_numSections; i++) {
		switch(m_pSectionHdrs[i].type) {
		case CAPERF_SECTION_RUN_INFO:
			CA_DBG_PRINTF(3, "Not Implemented\n");
			return E_FAIL;

		case CAPERF_SECTION_CPU_INFO:
			ret = readCpuInfoSection(m_pSectionHdrs[i].offset,
					         m_pSectionHdrs[i].size);
			break;

		case CAPERF_SECTION_EVENT_ATTRIBUTE:
			ret = readEventConfigs(m_pSectionHdrs[i].offset,
					       m_pSectionHdrs[i].size);
			break;

		case CAPERF_SECTION_EVENT_ID:
			ret = readEventSampleIds(m_pSectionHdrs[i].offset,
						 m_pSectionHdrs[i].size);
			break;

		case CAPERF_SECTION_SAMPLE_DATA:
			m_dataStartOffset = m_pSectionHdrs[i].offset;
			m_dataSize = m_pSectionHdrs[i].size;
			break;

		case CAPERF_SECTION_COUNTER_DATA:
			// TODO: !! not supported now !!
			break;

		default:
			CA_DBG_PRINTF(3, "Unknown Section Header \n");
			return E_FAIL;
		}

		if (S_OK != ret) {
			CA_DBG_PRINTF(3, "Error while reading Section(%d).\n",
				 m_pSectionHdrs[i].type);
			return E_FAIL;		
		}
	}

	// if there are sample ids, assign them to the respective events
	ret = createEvtIdMap();
	if (S_OK != ret) {
		CA_DBG_PRINTF(3, "Error while creating EventIdMap.\n");
		return E_FAIL;		
	}

error_exit:
	return ret;
}


void
CAPerfDataReader::deinit()
{
	if (m_fd != -1) {
		close(m_fd);
		m_fd = -1;
	}

	if (m_pSectionHdrs) {
		free(m_pSectionHdrs);
		m_pSectionHdrs = NULL;
	}

	if (m_pEventCfg) {
		free(m_pEventCfg);
		m_pEventCfg = NULL;
	}

	if (m_pSampleIds) {
		free(m_pSampleIds);
		m_pSampleIds = NULL;
	}

	if (m_pBuf) {
		free(m_pBuf);
		m_pBuf = NULL;
	}

	// clear the internal structures
	clear();
}


int
CAPerfDataReader::readHeader()
{
	if (-1 == m_fd) {
		return E_FAIL;
	}

	lseek(m_fd, 0, SEEK_SET);

	int ret = read(m_fd, &m_fileHeader, sizeof(m_fileHeader));

	if (ret != sizeof(m_fileHeader)) {
		CA_DBG_PRINTF(3, "Error while reading the file header.\n");
		return E_FAIL;
	}

	// set the offset
	m_offset = lseek(m_fd, 0, SEEK_CUR);

	return S_OK;
}

int
CAPerfDataReader::readSectionHdrs()
{
	lseek(m_fd, m_fileHeader.section_hdr_off, SEEK_SET);

	m_pSectionHdrs = (caperf_section_hdr_t *)malloc(m_fileHeader.section_hdr_size);

	int ret = read(m_fd, m_pSectionHdrs, m_fileHeader.section_hdr_size);
	if (ret != m_fileHeader.section_hdr_size) {
		CA_DBG_PRINTF(3, "Error while reading the section headers.\n");
		return E_FAIL;
	}

	m_numSections = m_fileHeader.section_hdr_size / sizeof(caperf_section_hdr_t);

	return S_OK;
}

int
CAPerfDataReader::readCpuInfoSection(uint64_t offset, uint64_t size)
{
	off_t ret = lseek(m_fd, offset, SEEK_SET);
	if (-1 == ret) {
		CA_DBG_PRINTF(3, "Invalid offset (0x%lx) in readCpuInfo.\n", offset);
		return E_FILE_INVALID_OFFSET;
	}

	// Currently there is only one CPUID fn 0000_0001 is written
	// in caperf.data file.

	ssize_t rv = read(m_fd, &m_cpuFamilyInfo, size);
	if (rv != size) {
		CA_DBG_PRINTF(3, "Error while reading the cpu info.\n");
		return E_FAIL;
	}

	return S_OK;
}


int
CAPerfDataReader::readEventConfigs(uint64_t offset, uint64_t size)
{
	off_t ret = lseek(m_fd, offset, SEEK_SET);
	if (-1 == ret) {
		CA_DBG_PRINTF(3, "Invalid offset (0x%lx) in readEventConfigs.\n", offset);
		return E_FILE_INVALID_OFFSET;
	}

	// alloc memory to hold the event configs (attributes in PERF's terms)
	m_pEventCfg = (caperf_section_evtcfg_t *)malloc(size);

	ssize_t rv = read(m_fd, m_pEventCfg, size);
	if (rv != size) {
		CA_DBG_PRINTF(3, "Error while reading the event configuration.\n");
		return E_FAIL;
	}

	m_numEvents = size / sizeof(caperf_section_evtcfg_t);

	return S_OK;
}


int
CAPerfDataReader::readEventSampleIds(uint64_t offset, uint64_t size)
{
	off_t ret = lseek(m_fd, offset, SEEK_SET);
	if (-1 == ret) {
		CA_DBG_PRINTF(3, "Invalid offset (0x%lx) in readEventSampleIds.\n",offset);
		return E_FILE_INVALID_OFFSET;
	}

	// alloc memory to hold the event configs (attributes in PERF's terms)
	m_pSampleIds = (caperf_section_sample_id_t *)malloc(size);

	ssize_t rv = read(m_fd, m_pSampleIds, size);
	if (rv != size) {
		CA_DBG_PRINTF(3, "Error while reading the event configuration.\n");
		return E_FAIL;
	}

	m_numSampleIds = size / sizeof(caperf_section_sample_id_t);

	return S_OK;
}

int
CAPerfDataReader::createEvtIdMap()
{
	if (! m_numEvents) {
		return E_INVALID_ARG;
	}

	uint32_t  idx = 0;
	for (int i = 0; i < m_numEvents; i++) {
		// push event_config (attribute) into PerfEventAttrVec
		m_perfEventAttrVec.push_back(m_pEventCfg[i].event_config);

		// sample-ids are per-core-per-event.
		if (! m_numCpus && m_pEventCfg[i].number_entries) {
			m_numCpus = m_pEventCfg[i].number_entries;
		}

		// get the sampleType info
		if (m_pEventCfg[i].event_config.sample_type) {
			m_sampleType = m_pEventCfg[i].event_config.sample_type;
		}

		// set the eventId-to-eventtype map
		if (! m_pEventCfg[i].number_entries) {
			// no event Id (sample Id) for this event
			continue;
		}

		for (int j = 0; j < m_pEventCfg[i].number_entries; j++) {
			// add entries into m_perfEvtIdToEvtTypeMap
			uint64_t id = m_pSampleIds[idx + j].sample_id;
			uint64_t data = m_pSampleIds[idx + j].cpuid;
			data = (data << 32) | i;

			m_perfEvtIdToEvtTypeMap.insert(
				PerfEvtIdToEvtTypeMap::value_type(id, data));
		}
		idx += m_pEventCfg[i].number_entries;
	}

	return S_OK;
}


int
CAPerfDataReader::dumpCAPerfHeader()
{
	uint32_t major, minor, micro;
	uint32_t ver = m_fileHeader.ca_version;

	major = (ver>>24);
	minor = (ver>>16) & 0x00ff;
	micro = ver & 0x0000ffff;

	CA_DBG_PRINTF(5, "CAPerf Header :-\n");
	CA_DBG_PRINTF(5, "-------------\n");

	CA_DBG_PRINTF(5, "CA Version : %d.%d.%d\n", major, minor, micro);
	CA_DBG_PRINTF(5, "Version : %d\n", m_fileHeader.version);
	CA_DBG_PRINTF(5, "section header table offset : 0x%x\n",
					m_fileHeader.section_hdr_off);
	CA_DBG_PRINTF(5, "section header table size : 0x%x\n",
					m_fileHeader.section_hdr_size);

	CA_DBG_PRINTF(5, "Number of Sections  : %d\n", m_numSections);
	for (int i=0; i<m_numSections; i++) {
		CA_DBG_PRINTF(5, "section(%d), misc(%d), offset(0x%lx), size(0x%lx)\n",
			m_pSectionHdrs[i].type,
			m_pSectionHdrs[i].misc,
			m_pSectionHdrs[i].offset,
			m_pSectionHdrs[i].size);
	}

	uint32_t family = 0;
	uint32_t model = 0;
	uint32_t stepping = 0;
	getCpuInfo(&family, &model, &stepping);

	CA_DBG_PRINTF(5, "\nCPU Info :-\n");
	CA_DBG_PRINTF(5, "--------\n");
	CA_DBG_PRINTF(5, "Number of CPUs : %d\n", getNumCpus());
	CA_DBG_PRINTF(5, "CPU Family : 0x%x\n", family);
	CA_DBG_PRINTF(5, "CPU Model : %u\n", model);
	CA_DBG_PRINTF(5, "CPU Stepping : %u\n", stepping);

	return S_OK;
}

int
CAPerfDataReader::dumpCAPerfEvents()
{
	CA_DBG_PRINTF(5, "\nEvent Attributes :-\n");
	CA_DBG_PRINTF(5, "----------------\n");
	CA_DBG_PRINTF(5, "Number of Events  : %d\n", getNumEvents());

	PerfEventAttrVec::iterator iter = m_perfEventAttrVec.begin();
	for (; iter != m_perfEventAttrVec.end(); iter++) {
		CA_DBG_PRINTF(5, "\ntype          : %d\n", iter->type);
		CA_DBG_PRINTF(5, "config        : 0x%llx\n", iter->config);
		CA_DBG_PRINTF(5, "sample_period : %lld\n", iter->sample_period);
		CA_DBG_PRINTF(5, "sample_type   : 0x%llx\n", iter->sample_type);
	}

	// print the eventIds
	PerfEvtIdToEvtTypeMap::iterator it, itEnd;
	int cpu;
	uint32_t event;
	uint32_t umask;
	uint32_t os;
	uint32_t usr;

	it = m_perfEvtIdToEvtTypeMap.begin(); 
	itEnd = m_perfEvtIdToEvtTypeMap.end(); 

	CA_DBG_PRINTF(5, "\nSample IDs :-\n");
	CA_DBG_PRINTF(5, "----------\n");
	for (; it != itEnd; it++) {
		getEventAndCpuFromSampleId(it->first, &cpu, &event, &umask, &os, &usr);

		CA_DBG_PRINTF(5, "eventId(0x%llx), event:umask:os:usr (0x%x:0x%x:0x%x:0x%x) cpu(%d)\n",
			it->first, event, umask, os, usr,cpu);
	}

	CA_DBG_PRINTF(5, "\nSample Type : %llx\n", getAttrSampleType());

	return S_OK;
}


// Copied from Suravee's PerfDataReader implementation
int
CAPerfDataReader::dump_PERF_RECORD(
	struct perf_event_header *pHdr, 
	void			 *ptr,
	uint32_t		  recNum
)
{
	CA_DBG_PRINTF(5, "[rec#:%5d, offet:0x%05x size:%3d] PERF_RECORD_%s\n",
		recNum, m_offset, pHdr->size, _ca_perfRecNames[pHdr->type]);

	return S_OK;
}

int
CAPerfDataReader::dumpCAPerfData()
{
	if (!isOpen()) {
		return E_FILE_INVALID;
	}

	// Move to the data Section 
	if ((m_offset = lseek(m_fd, m_dataStartOffset, SEEK_SET)) < 0) {
		return E_FILE_INVALID_OFFSET;
	}

	CA_DBG_PRINTF(5, "\n-------- PERF DATA SECTION (offset:0x%lx, size:%lu)-------- \n", 
		m_dataStartOffset, m_dataSize);

	int retVal = S_OK;
	struct perf_event_header hdr;
	void * pBuf = NULL;
	int curBufSz = 0;
	ssize_t rdSz = 0;
	uint32_t recCnt = 0;

	if (getFirstRecord(&hdr, (const void **)(&pBuf)) != S_OK) {
		return E_FAIL;
	}

	do {
		if (hdr.size == 0 || !pBuf) {
			retVal = E_FAIL;
			break;
		}

		dump_PERF_RECORD(&hdr, pBuf, recCnt);

		recCnt++;
	} while	(getNextRecord(&hdr, (const void **)(&pBuf)) == S_OK);

	return S_OK;
}


void CAPerfDataReader::dumpHeaderSections()
{
	dumpCAPerfHeader();
	dumpCAPerfEvents();
}	


void CAPerfDataReader::dumpData()
{
	dumpCAPerfData();
}


#define CPUID_FN_0000_0001	0x00000001
#define CPUID_FN_8000_0005	0x80000005

int CAPerfDataReader::getCpuInfo(
	uint32_t *pFamily,
	uint32_t *pModel,
	uint32_t *pStepping)
{
	if (!pFamily) {
		return E_FAIL;
	}

	if (CPUID_FN_0000_0001 != m_cpuFamilyInfo.function) {
		return E_FAIL;
	}

	union {
		unsigned eax;
		struct {
			unsigned stepping : 4;
			unsigned model : 4;
			unsigned family : 4;
			unsigned res : 4;
			unsigned ext_model : 4;
			unsigned ext_family : 8;
			unsigned res2 : 4;
		};
	} v;

	v.eax = m_cpuFamilyInfo.value[0];

	*pFamily = v.family + v.ext_family;

	if (pModel) {
		*pModel = v.model + v.ext_model;
	}

	if (pStepping) {
		*pStepping = v.stepping;
	}

	return S_OK;
}
