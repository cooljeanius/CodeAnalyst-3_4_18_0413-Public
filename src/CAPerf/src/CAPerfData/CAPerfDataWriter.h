#ifndef _CAPERFDATAWRITER_H_
#define _CAPERFDATAWRITER_H_

// System Headers
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/perf_event.h>

// C++ Headers
#include <list>
#include <string>
#include <map>

// Project Headers
#include "caheader.h"
#include "CAPerfEvent.h"

//
// Class CAPerfDataWriter
//
// Description
// This class writes the ca_perf.data file
//

struct kModule {
	char		m_modName[PATH_MAX];
	std::string	m_absPath;
	// char		m_absPath[PATH_MAX];

	uint64_t	m_startAddress;
	uint64_t	m_endAddress;

	void addAbsPath(std::string path) {
		m_absPath = path;
	}

	void setEndAddress(uint64_t addr) {
		m_endAddress = addr;
	}
};

// typedef std::List<kModule>  kModuleList;
typedef std::map<std::string, kModule>  kModuleNameMap;
typedef std::map<uint64_t, kModule>  kModuleAddrMap;

typedef std::list<caperf_section_sample_id_t>  sampleInfoList;

class CAPerfDataWriter
{
public:
	CAPerfDataWriter();
	~CAPerfDataWriter();

	int init(char *filepath, bool overwrite=false);

	int writeRunInfoSection() { return E_NOT_IMPLEMENTED; };
	int writeCpuInfoSection() { return E_NOT_IMPLEMENTED; };

	int writeEventsConfig (CAPerfEvtList &countEventsList,
			       CAPerfEvtList &sampleEventsList);

	int writePMUSampleData(void *data, ssize_t size);
	int writePMUCountData() { return E_NOT_IMPLEMENTED; };

	int writeEventSection();

	int writeSWPProcessMmaps(uint16_t sampleRecSize);
	int writePidMmaps (pid_t pid, pid_t tgid, uint16_t  sampleRecSize);
	pid_t writePidComm (pid_t pid, uint16_t sampleRecSize);

	int writeKernelInfo(uint64_t sampleRecSize);

private:
	int writeHeader();
	int updateFileHeader(uint32_t sect_hdr_offset, uint32_t sect_hdr_size);

	uint32_t getIdx(uint32_t section);

	int writeSectionHdr(caperf_section_type_t section,
			    uint64_t startOffset=0, uint64_t size=0);
	int updateSectionHdr(caperf_section_type_t section,
			     uint64_t startOffset, uint64_t size);

	int getCpuId (uint32_t  fn,
		      uint32_t  *pEax,
		      uint32_t  *pEbx,
		      uint32_t  *pEcx,
		      uint32_t  *pEdx);
	int writeCpuInfo();
	int writeCpuTopology() { return E_NOT_IMPLEMENTED; }

	int writeCountEvent(CAPerfEvent &event);
	int writeSampleEvent(CAPerfEvent &event);
	int writeSampleEventsInfo();

	int findKernelSymbol (const char  *symbol,
			      uint8_t      type,
			      uint64_t    *startAddress);
	int writeKernelMmap(const char	  *symbolName,
			    uint16_t	   sampleRecSize);
	int getKernelVersion();
	int readKernelModules();
	int setModulesPath();
	int setModulesAbsolutePath(std::string dirName);

	int writeKernelModules(uint16_t sampleRecSize);
	int writeGuestOsInfo() { return E_NOT_IMPLEMENTED; }

	int m_fd;
	int m_offset;

	caperf_file_header_t  m_fileHeader;

	uint64_t m_nbrSections;
	uint64_t m_maxNbrSections;
	uint64_t m_sectionHdrOffsets[CAPERF_MAX_SECTIONS];

	// size of the sample records; based on 
	// perf_event_attr::sample_type; This does not include
	// perf_event_header
	// Compute this when we write a sampling event
	uint16_t m_sampleRecSize;

	ssize_t m_dataStartOffset;
	ssize_t m_dataSize;

	ca_event_t	*m_pCommEvent;
	ca_event_t	*m_pMmapEvent;

	// list<struct caperf_section_sample_id_t>  m_sampleIdList;
	sampleInfoList m_sampleIdList;

	// TODO: all the kernel/kallsyms related stuff should be
	// in a separate class
	bool		m_guestOs;
	std::string	m_kVersion;
	std::string	m_rootDir;
	std::string	m_kernelSymbolFile;
	std::string	m_kModulesPath;
	kModuleNameMap	m_kModuleNameMap;
	kModuleAddrMap	m_kModuleAddrMap;
};

#endif // _CAPERFDATAWRITER_H_
