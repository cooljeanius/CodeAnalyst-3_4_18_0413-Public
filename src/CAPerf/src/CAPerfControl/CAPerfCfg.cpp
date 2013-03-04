//
// CAPerfCfg.cpp
//

// system headers
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

// C++ headers
#include <list>

// project headers
#include "CAPerfCfg.h"
#include "CAPerfDataWriter.h"


static int
getCpuInfo(CACpuVec *cpuVec)
{
	FILE *cpuOnline;
	int cpu, prev;
	char sep;
	int ret;

	if (! cpuVec) {
		return E_INVALID_ARG;
	}

	cpuVec->clear();

	cpuOnline = fopen("/sys/devices/system/cpu/online", "r");
	if (!cpuOnline) {
		return E_FAIL;
	}

	sep = 0;
	prev = -1;
	for (;;) {
		ret = fscanf(cpuOnline, "%u%c", &cpu, &sep);
		CA_DBG_PRINTF(3, "cpu(%u), sep(%c), ret(%d).\n",
				cpu, sep, ret);

		if (ret <= 0) {
			break;
		}

		if (prev >= 0) {
			while (++prev < cpu) {
				cpuVec->push_back(prev);
			}
		}

		cpuVec->push_back(cpu);

		if (ret == 2) {
			prev = (sep == '-') ? cpu : -1;
		} 
		if (ret == 1 || sep == '\n') {
			break;
		}
	}

	fclose(cpuOnline);

	return cpuVec->size();
}


// getThreadList
//
// The caller should clear the CAThreadVec, if required.
//
static pid_t
getThreadList (pid_t pid, CAThreadVec *threadVec)
{
	char filename[PATH_MAX];
	char bf[BUFSIZ];
	FILE *fp;
	DIR *tasks;
	struct dirent dirent, *next;
	pid_t tgid = 0;

	if (!threadVec) {
		return E_INVALID_ARG;
	}

	// Read the status file to get the Tgid.
	snprintf(filename, sizeof(filename), "/proc/%d/status", pid);

	fp = fopen(filename, "r");
	if (fp == NULL) {
		// process exited ?
		CA_DBG_PRINTF(3, "couldn't open %s\n", filename);
		return E_FAIL;
	}

	while (! tgid) {
		if (fgets(bf, sizeof(bf), fp) == NULL) {
			CA_DBG_PRINTF(3, "couldn't read %s file\n", filename);
			goto error_exit;
		}

		if (memcmp(bf, "Tgid:", 5) == 0) {
			char *tgids = bf + 5;
			while (*tgids && isspace(*tgids)) {
				++tgids;
			}
			tgid = atoi(tgids);
			break;
		}
	}

	// read the task file to get the list of threads
	snprintf(filename, sizeof(filename), "/proc/%d/task", pid);

	tasks = opendir(filename);
	if (tasks == NULL) {
		CA_DBG_PRINTF(3, "couldn't open %s\n", filename);
		return E_FAIL;
	}

	while (!readdir_r(tasks, &dirent, &next) && next) {
		char *end;
		pid = strtol(dirent.d_name, &end, 10);
		if (*end) {
			continue;
		}

		threadVec->push_back(pid);
	}

	closedir(tasks);

error_exit:
	fclose (fp);
	return tgid;
}


CAPerfCfg::CAPerfCfg(CAProfileConfig &cfg, CAPMUTarget &tgt)
  : CAProfileConfig(cfg), m_pmuTgt(tgt)
{
  m_nbrCtrPerfEvents = 0;
  m_ctrPerfEventList.clear();
  m_nbrSamplingPerfEvents = 0;
  m_samplingPerfEventList.clear();

  m_sampleRecSize = 0;

  m_countData.clear();

  m_nbrFds = 0;
  m_nbrMmaps = 0;

  // Note: PERF expects the mmap pages for sample-data-buffer to be multiple of
  //       1 + 2^n pages. Hence, m_mmapPages should be multiple 1 + 2^n pages, 
  //         - first page is a meta-data page (struct perf_event_mmap_page)
  //         - other pages form ring buffer
  m_mmapPages = 16;
  m_pageSize = sysconf(_SC_PAGE_SIZE);
  m_mmapLen = (m_mmapPages + 1) * m_pageSize;
  m_mmapProt = PROT_READ;
  m_mmap = NULL;

  m_pollFds = 0;
  m_samplingPollFds = NULL;

  m_profileCompleted = false;

  m_cpuVec.clear();
  m_threadVec.clear();

  m_pData = (void *)malloc(sizeof(char) * m_mmapLen);

  m_useIoctlRedirect = false;
}


CAPerfCfg::~CAPerfCfg()
{
  clear();
}


CAPerfCfg::CAPerfCfg(const CAPerfCfg& cfg)
  : CAProfileConfig(cfg), m_pmuTgt(cfg.m_pmuTgt)
{
  m_nbrCtrPerfEvents = cfg.m_nbrCtrPerfEvents;
  m_ctrPerfEventList = cfg.m_ctrPerfEventList;
  m_nbrSamplingPerfEvents = cfg.m_nbrSamplingPerfEvents;
  m_samplingPerfEventList = cfg.m_samplingPerfEventList;

  m_nbrFds = cfg.m_nbrFds;
  m_nbrMmaps = cfg.m_nbrMmaps;
  m_mmapLen = cfg.m_mmapLen;

  m_sampleRecSize = cfg.m_sampleRecSize;

  m_cpuVec.clear();
  m_cpuVec = cfg.m_cpuVec;

  m_threadVec.clear();
  m_threadVec = cfg.m_threadVec;

  // FIXME - m_pData

  m_useIoctlRedirect = cfg.m_useIoctlRedirect;
}


CAPerfCfg&
CAPerfCfg::operator=(const CAPerfCfg& cfg)
{
  CAProfileConfig::operator=(cfg);

  m_pmuTgt = cfg.m_pmuTgt;

  m_nbrCtrPerfEvents = cfg.m_nbrCtrPerfEvents;
  m_ctrPerfEventList = cfg.m_ctrPerfEventList;
  m_nbrSamplingPerfEvents = cfg.m_nbrSamplingPerfEvents;
  m_samplingPerfEventList = cfg.m_samplingPerfEventList;

  m_sampleRecSize = cfg.m_sampleRecSize;

  m_nbrFds = cfg.m_nbrFds;
  m_nbrMmaps = cfg.m_nbrMmaps;
  m_mmapLen = cfg.m_mmapLen;

  m_cpuVec.clear();
  m_cpuVec = cfg.m_cpuVec;

  m_threadVec.clear();
  m_threadVec = cfg.m_threadVec;

  // FIXME - m_pData

  m_useIoctlRedirect = cfg.m_useIoctlRedirect;

  return *this;
}


void
CAPerfCfg::clear()
{
  m_ctrPerfEventList.clear();
  m_nbrCtrPerfEvents = 0;

  // munmap
  if (m_mmap) {
    for (int i=0; i < m_nbrMmaps; i++) {
      if (m_mmap[i].m_base != NULL) {
        munmap(m_mmap[i].m_base, m_mmapLen);
        m_mmap[i].m_base = NULL;
      }
    }

    delete[] m_mmap;
    m_mmap = NULL;
  }

  m_pollFds = 0;

  if (m_samplingPollFds) {
    free(m_samplingPollFds);
    m_samplingPollFds = NULL;
  }

  m_samplingPerfEventList.clear();
  m_nbrSamplingPerfEvents = 0;

  m_countData.clear();

  m_nbrFds = 0;
  m_nbrMmaps = 0;
  m_mmapLen = 0;
  
  m_sampleRecSize = 0;

  if (m_pData) {
    free(m_pData);
    m_pData = NULL;
  }

  m_cpuVec.clear();
  m_threadVec.clear();
}

uint16_t
CAPerfCfg::getSampleRecSize(uint64_t sampleType, bool sampleIdAll)
{
       struct ca_sample_data *data;
       uint16_t size = 0;

	if (! sampleIdAll) {
		return size;
	}

	if (sampleType & PERF_SAMPLE_TID) {
		size += sizeof(data->tid) * 2;
	}

	if (sampleType & PERF_SAMPLE_TIME) {
		size += sizeof(data->time);
	}

	if (sampleType & PERF_SAMPLE_ID) {
		size += sizeof(data->id);
	}

	if (sampleType & PERF_SAMPLE_STREAM_ID) {
		size += sizeof(data->stream_id);
	}

	if (sampleType & PERF_SAMPLE_CPU) {
		size += sizeof(data->cpu) * 2;
	}

	return size;
}


int
CAPerfCfg::initialize()
{
  // construct the CAPerfEvent objects from 
  //    CAProfileConfig::m_ctrEventList
  //    CAProfileConfig::m_samplingEventList

  uint64_t sampleType = 0;
  bool     sampleIdAll = false;
  bool     firstEvent = true;

  // initialze the m_cpuVec;
  int ret = getCpuInfo(&m_cpuVec);
  CA_DBG_PRINTF(3, "Number of CPUs : %d\n.", m_cpuVec.size());

  // For per-process get the target pid's thread list;
  // TODO: Not sure whether we need to maintain per process CAThreadVec.
  // It may be a problem during PERF fd redirect ??
  size_t numPids;
  pid_t  *pids;
  if (! m_pmuTgt.isSWP()) {
    m_pmuTgt.getPids(&numPids, &pids);

    for (int i = 0; i < numPids; i++) {
      ret = getThreadList(pids[i], &m_threadVec);
    }
  }

  // TODO: check whether the cpu-list in the pmutarget is valid or not ?

  // iterate over the counting event list...
  if (m_ctrEventList.size() != 0) {
    CAEventList::iterator iter = m_ctrEventList.begin();
    for (; iter != m_ctrEventList.end(); iter++) {
      CAPerfEvent evt(*iter, this);

      m_ctrPerfEventList.push_back(evt);
    }
  }
  m_nbrCtrPerfEvents = m_ctrPerfEventList.size();

  if (m_samplingEventList.size() != 0) {
    CAEventList::iterator iter = m_samplingEventList.begin();
    for (; iter != m_samplingEventList.end(); iter++) {
      CAPerfEvent evt(*iter, this);

      if (! firstEvent) {
        evt.setPerfAttrMmap(false);
#ifdef LINUX_PERF_MMAP_DATA_SUPPORT
        evt.setPerfAttrMmapData(false);
#endif
        evt.setPerfAttrComm(false);
      }
      firstEvent = false;

      m_samplingPerfEventList.push_back(evt);

      sampleType = evt.getSampleType();
#ifdef LINUX_PERF_SAMPLE_ID_ALL_SUPPORT
      sampleIdAll = evt.getSampleIdAll();
#endif
    }
  }
  m_nbrSamplingPerfEvents = m_samplingPerfEventList.size();

  // FIXME;
  if (sampleType) {
    m_sampleRecSize = getSampleRecSize(sampleType, sampleIdAll);
  }

  // FIXME: just for testing
  ret = m_dataWriter.init((char *)(getOutputFile().c_str()),
			  isOverwrite());
  if (S_OK != ret) {
    CA_DBG_PRINTF(3, "initializing datafile failed... ret(%d)\n", ret);
    return E_FAIL;
  }
  CA_DBG_PRINTF(3, "initializing datafile succeedded...\n");

  return S_OK;
}

int
CAPerfCfg::initSamplingEvents()
{
  // number of tgts
  size_t tgts;
  tgts  = (m_pmuTgt.isSWP()) ? m_pmuTgt.m_nbrCpus
                             : m_cpuVec.size() * m_threadVec.size();

  int maxMmaps = tgts;
  if (m_samplingPerfEventList.size()) { 
    maxMmaps *= m_samplingPerfEventList.size();
  }

  if (! tgts) {
    return E_INVALID_STATE;
  }

  // allocate memory for mmap
  // m_mmap = (CAPerfMmap *)malloc(sizeof(CAPerfMmap) * tgts);
  m_mmap = new CAPerfMmap[maxMmaps];
  if (! m_mmap) {
    CA_DBG_PRINTF(3, "malloc failed while allocing m_mmap.\n");
    return E_FAIL;
  }

  if (! m_samplingPerfEventList.size()) {
    return E_INVALID_STATE;
  }

  m_pollFds = 0;
  m_samplingPollFds = (struct pollfd *)malloc(sizeof(struct pollfd) * maxMmaps);
  if (! m_samplingPollFds) {
    CA_DBG_PRINTF(3, "malloc failed while allocing m_samplingPollFds.\n");
    return E_FAIL;
  }


  // TODO: i need to revist the logic for using ioctl-redirect..
  // It seems, we can use ioctl redirect only the fd's corresponding to one particular cpu..
  // i may have change the internal structures..
  CAPerfEvtList::iterator iter = m_samplingPerfEventList.begin();
  for (; iter != m_samplingPerfEventList.end(); iter++)
  {
    CAEventCountData *data;
    int ret = iter->getEventData(&data);

    if (ret <= 0) {
      CA_DBG_PRINTF(3, "Error in CAPErfCfg::initSamplingEvents.\n");
      return E_INVALID_PROF_CONFIG;
    }

    int first_fd = -1;
    for (int i=0; i<ret; i++) {
      int fd = data[i].m_fd;

      if (-1 == fd) {
        CA_DBG_PRINTF(3, "Invalid fd in CAEventCountData.\n");
        return E_INVALID_PROF_CONFIG;
      }

      if (first_fd == -1) {
        first_fd = fd;

        CA_DBG_PRINTF(3, "fd(%d), m_mmapLen(%d), pagesize(%d)\n",
                     fd, m_mmapLen, m_pageSize);
        void *base = mmap(NULL,
                          m_mmapLen,
                          PROT_READ, //m_mmapProt,
                          MAP_SHARED,
                          fd,
                          0);

        if (MAP_FAILED == base) {
          CA_DBG_PRINTF(3, "mmap failed in CAPErfCfg::initSamplingEvents. errno(%d)\n", errno);
          return E_INVALID_PROF_CONFIG;
        }
 
        CA_DBG_PRINTF(3, "mmap succedded..\n");

        // mask used to handle the ring-buffer boundary
        int mask = (m_mmapPages * m_pageSize) - 1;
        m_mmap[m_nbrMmaps].set(base, mask, 0);
        m_nbrMmaps++;

        // add it to pollfd list
        fcntl(fd, F_SETFL, O_NONBLOCK);

        m_samplingPollFds[m_pollFds].fd = fd;
        m_samplingPollFds[m_pollFds].events = POLLIN;
        m_pollFds++;

        // if we are not going to use ioctl redirect, reset first_fd to -1;
        if (! m_useIoctlRedirect) {
          first_fd = -1;
        }
      } else {
        // ioctl redirect
        int rv = ioctl(first_fd, PERF_EVENT_IOC_SET_OUTPUT, fd);
        if ( 0 != rv ) {
          CA_DBG_PRINTF(3, "Error while ioctl - ret(%d) fd(%d), first_fd(%d), errno(%d).\n",
		rv, fd, first_fd, errno);
          return E_INVALID_STATE;

        }
      }
    }
  }

  // The state is maintained by CAPerfProfiler and CAPMUSession objects.
  return S_OK;
}

int
CAPerfCfg::startProfile(bool enable)
{
  int ret;
  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      ret = iter->startEvent(false);
      if (S_OK != ret) {
        return ret;
      }
    }
  }

  if (m_samplingPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_samplingPerfEventList.begin();
    for (; iter != m_samplingPerfEventList.end(); iter++) {
      // Don't start the profile here, even if the user has requested so
      // We need the PERF's "file descriptor" of sampling-events
      // to mmap. This is handled in initSamplingEvents()
      ret = iter->startEvent(false);
      if (S_OK != ret) {
        return ret;
      }
    }
  }

  // If there are any sampling events, initialize the respective 
  // internal structures.
  ret = initSamplingEvents();
  
  ret = m_dataWriter.writeEventsConfig(m_ctrPerfEventList, m_samplingPerfEventList);
  if (m_pmuTgt.isSWP()) {
    ret = m_dataWriter.writeKernelInfo(m_sampleRecSize);
    ret = m_dataWriter.writeSWPProcessMmaps(m_sampleRecSize);
  } else {
    // for all the pids write the COMM and MMAP records
    size_t numPids;
    pid_t  *pids;

    m_pmuTgt.getPids(&numPids, &pids);

    for (int i = 0; i < numPids; i++) {
      ret = m_dataWriter.writeKernelInfo(m_sampleRecSize);
      pid_t tgid = m_dataWriter.writePidComm(pids[i], m_sampleRecSize);
      ret = m_dataWriter.writePidMmaps(pids[i], tgid, m_sampleRecSize);
    }
  }

  if (enable) {
    enableProfile();
  }

  return S_OK;
}


int
CAPerfCfg::enableProfile()
{
  int ret;

  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      ret = iter->enableEvent();
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in enableProfile - counting events\n");
        break;
      }
    }
  }

  if (m_samplingPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_samplingPerfEventList.begin();
    for (; iter != m_samplingPerfEventList.end(); iter++) {
      ret = iter->enableEvent();
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in enableProfile - sampling events\n");
        break;
      }
    }
  }

  return ret;
}

int
CAPerfCfg::disableProfile()
{
  int ret;

  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      ret = iter->disableEvent();
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in disableProfile - counting event\n");
        break;
      }
    }
  }

  if (m_samplingPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_samplingPerfEventList.begin();
    for (; iter != m_samplingPerfEventList.end(); iter++) {
      ret = iter->disableEvent();
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in disableProfile - sampling event\n");
        break;
      }
    }
  }

  // While disabling the profile, don't set this flag,
  // m_profileCompleted = true;

  return ret;
}


int
CAPerfCfg::stopProfile()
{
  int ret;

  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      ret = iter->disableEvent();
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in stopProfile - counting event\n");
        break;
      }
    }
  }

  if (m_samplingPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_samplingPerfEventList.begin();
    for (; iter != m_samplingPerfEventList.end(); iter++) {
      ret = iter->disableEvent();
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in stopProfile - sampling event\n");
        break;
      }
    }
  }

  m_profileCompleted = true;

  return S_OK;
  // return disableProfile();
}


int CAPerfCfg::printSample(void *data)
{
  struct perf_event_header  *hdr = (perf_event_header *)data;

  CA_DBG_PRINTF(3, "type(%d), misc(%d), size(%d)\n",
                hdr->type, hdr->misc, hdr->size);

  return S_OK;
}


int CAPerfCfg::readMmapBuffers()
{
  CAPerfMmap *mmapBuffer;
  struct perf_event_mmap_page *mmapMetaData; // first page in mmapBuffer
  unsigned int dataHead;
  unsigned int dataTail;
  unsigned char *dataBuf;
  void *data;

  // Iterate over all the mmaps and read them..
  for (int i = 0; i < m_nbrMmaps; i++) {
    mmapBuffer = &m_mmap[i];

    if (mmapBuffer->m_base != NULL) {
      mmapMetaData = (struct perf_event_mmap_page*)mmapBuffer->m_base;
      dataHead = mmapMetaData->data_head;
      dataTail = mmapMetaData->data_tail;
      rmb();

      dataBuf = (unsigned char*)mmapMetaData + m_pageSize; 
      CA_DBG_PRINTF(3, "dataHead(%u), prevHead(%u)\n",
                    dataHead, mmapBuffer->m_prev);

      if (dataHead == dataTail) {
        CA_DBG_PRINTF(3, "No Samples collected so far..dataHead(%d),"
           " dataTail(%d)\n", dataHead, dataTail);
      }

      unsigned int prevHead = mmapBuffer->m_prev;
      if ( (dataHead - prevHead) <= 0) {
        CA_DBG_PRINTF(3, "Invalid data.. Current Head(%u), Prev Head(%u).\n",
                      dataHead, prevHead);
        // dataHead = prevHead;
        continue;
      }

      unsigned long dataSize = dataHead - prevHead;
      unsigned long tmp = 0;
      if (   ((prevHead & mmapBuffer->m_mask) + dataSize)
          != (dataHead & mmapBuffer->m_mask))
      {
        data = &dataBuf[prevHead & mmapBuffer->m_mask];
        dataSize = mmapBuffer->m_mask + 1 - (prevHead & mmapBuffer->m_mask);
        prevHead += dataSize;
        errno = 0;
        memcpy(m_pData, data, dataSize);
        CA_DBG_PRINTF(3, "memcpy.. errno(%d).\n", errno);
        CA_DBG_PRINTF(3, "1 - dataSize(%u), prevHead(%u)\n",
                      dataSize, prevHead);
        tmp = dataSize;
        // printSample(data);
      }
      data = &dataBuf[prevHead & mmapBuffer->m_mask];
      dataSize = dataHead - prevHead;
      prevHead += dataSize;

      CA_DBG_PRINTF(3, "2 - dataSize(%u), prevHead(%u)\n",
                      dataSize, prevHead);
      errno = 0;
      memcpy((void*)((uintptr_t)m_pData+tmp), data, dataSize);
      CA_DBG_PRINTF(3, "memcpy.. errno(%d).\n", errno);

      m_dataWriter.writePMUSampleData(m_pData, tmp+dataSize);

      // printSample(data);
      printSample(m_pData);

      mmapBuffer->m_prev = prevHead;

      // TODO:
      // when the mapping is PROT_WRITE, the data_tail value should be 
      // writen by userspace to reflect the last read data. In this case
      // the kernel will not overwrite unread data.
#if 0 
      if (! overwrite) { 
        mmapMetaData->data_tail = prevHead;
      }
#endif // 0
    }
  }

  return S_OK;
}

int
CAPerfCfg::readSampleData()
{

  // read the contents from mmap buffers
  // wait for the poll event
  // if profiling is done,
  //    stop the profile
  for (; ;) {
    readMmapBuffers();

    if (m_profileCompleted) {
      break;
    }

    int ret = poll(m_samplingPollFds, m_pollFds, -1);

    if (m_profileCompleted) {
      // FIXME: stop only if the profile is still running
      stopProfile();
    }
  }
}


int
CAPerfCfg::readCounters(CAEvtCountDataList **countData)
{
  int ret;

  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      ret = iter->readCounterValue();
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in reading Counter Values\n");
        break;
      }
    }
  }

  // Now that we got the counter values, populate them:
  CAEventCountData *data;
  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      ret = iter->getEventData(&data);
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in retrieving Counter Values\n");
        break;
      }

      for (int i=0; i<ret; i++) {
        // data[i].print();
        m_countData.push_back(data[i]);
      }
    }
  }

  if (countData) {
    *countData = &m_countData;
  }

  return S_OK;
}


int
CAPerfCfg::printCounterValues()
{
  int ret;

  CAEventCountData *data;

  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      ret = iter->getEventData(&data);
      if (S_OK != ret) {
        CA_DBG_PRINTF(3, "Error in retrieving Counter Values\n");
        break;
      }

      for (int i=0; i<ret; i++) {
        data[i].print();
      }
    }
  }

  return ret;
}

void
CAPerfCfg::print()
{
  // iterate over the counting event list...
  CA_DBG_PRINTF(3, "\nNbr of Counting PERF Events : %d\n", m_nbrCtrPerfEvents);
  if (m_ctrPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_ctrPerfEventList.begin();
    for (; iter != m_ctrPerfEventList.end(); iter++) {
      iter->print();
    }
  }

  CA_DBG_PRINTF(3, "\nNbr of Sampling PERF Events : %d\n", m_nbrSamplingPerfEvents);
  if (m_samplingPerfEventList.size() != 0) {
    CAPerfEvtList::iterator iter = m_samplingPerfEventList.begin();
    for (; iter != m_samplingPerfEventList.end(); iter++) {
      iter->print();
    }
  }

  return;
}
