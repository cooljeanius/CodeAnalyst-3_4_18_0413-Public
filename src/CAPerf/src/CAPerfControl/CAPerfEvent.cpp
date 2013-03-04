//
// CAPerfEvent.cpp
//

// standard headers
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <inttypes.h>

// project headers
#include "CAError.h"
#include "CAPerfEvent.h"
#include "CAPerfCfg.h"

int
sys_perf_event_open(struct perf_event_attr *attr,
                    pid_t pid, int cpu, int group_fd,
                    unsigned long flags)
{
  attr->size = sizeof(*attr);
  return syscall(__NR_perf_event_open, attr, pid, cpu,
                 group_fd, flags);
}

//
// Structure to hold pre-defined PERF 'type' and 'config' vlaues
//
struct defConfig {
  uint32_t  type;
  uint32_t  config;
};

//
// Pre-defined PERF 'type' and 'config' vlaues
//
struct defConfig defaultCfgs[] = {
  // Harwdare Types
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_BUS_CYCLES },

  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK },
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK },
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS },
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES },
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS },
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MIN },
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MAJ },
#if defined(LINUX_PERF_SW_FAULTS_SUPPORT)
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_ALIGNMENT_FAULTS },
  { PERF_TYPE_SOFTWARE, PERF_COUNT_SW_EMULATION_FAULTS }, 
#else
  { 0, 0 }, // dummy values
  { 0, 0 }, // dummy values
#endif

  { PERF_TYPE_TRACEPOINT, PERF_TYPE_TRACEPOINT },  // ??
#if defined(LINUX_PERF_BREAKPOINT_SUPPORT)
  { PERF_TYPE_BREAKPOINT, PERF_TYPE_BREAKPOINT },  // ??
#else
  { 0, 0 }, // dummy values
#endif

  { PERF_TYPE_RAW, 0 }, // config should be SET later ?

  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1D << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1D << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1D << 0) | (PERF_COUNT_HW_CACHE_OP_WRITE <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1D << 0) | (PERF_COUNT_HW_CACHE_OP_WRITE <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1D << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1D << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },

  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1I << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1I << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1I << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_L1I << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },

  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_LL << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_LL << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_LL << 0) | (PERF_COUNT_HW_CACHE_OP_WRITE <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_LL << 0) | (PERF_COUNT_HW_CACHE_OP_WRITE <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_LL << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_LL << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },

  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_DTLB << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_DTLB << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_DTLB << 0) | (PERF_COUNT_HW_CACHE_OP_WRITE <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_DTLB << 0) | (PERF_COUNT_HW_CACHE_OP_WRITE <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_DTLB << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_DTLB << 0) | (PERF_COUNT_HW_CACHE_OP_PREFETCH <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
  
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_ITLB << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_ITLB << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },

  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_BPU << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
  { PERF_TYPE_HW_CACHE, ((PERF_COUNT_HW_CACHE_BPU << 0) | (PERF_COUNT_HW_CACHE_OP_READ <<  8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) }
};


static int
readn(int fd, void *buf, size_t n)
{
  void *buf_start = buf;

  while (n) {
    int ret = read(fd, buf, n);
    if (ret <= 0) {
      return ret;
    }

    n -= ret;
    // buf += ret;
    buf = (void *)((uintptr_t)buf + ret);
  }

  return (uintptr_t)buf - (uintptr_t)buf_start;
}



//
//  class CAPerfEvent
//

// CAPerfEvent::initialize
//
// construct perf_event_attr object
//
void
CAPerfEvent::initialize()
{
  // TODO: use compile options  -std=c++0x or -std=gnu++0x - do m_attr = { 0 };
  memset(&m_attr, 0, sizeof(m_attr));

  m_attr.size = sizeof(struct perf_event_attr);

  // handle special cases...
  if (CAEvent::CAPERF_TYPE_RAW == m_event.getType()) {
    m_attr.type = PERF_TYPE_RAW;
    m_attr.config = m_event.getConfig();
  } else {
    m_attr.type = defaultCfgs[m_event.getType()].type;
    m_attr.config = defaultCfgs[m_event.getType()].config;
  }

  // Sampling Specification
  m_attr.sample_period = m_samplingValue;
  m_attr.freq = (m_samplebyFreq) ? 1 : 0;

  // if sampling event, specify the sample attributes - information that
  // needs to be gathered in the sample records..
  m_attr.sample_type = (m_attr.sample_period) ? m_sampleAttr : 0;

  // read format - format of the data returned by read() on a perf evebnt fd;
  m_attr.read_format = m_readFormat;

  uint64_t  taskFlags = m_event.getTaskFlags();
  uint64_t  profFlags = m_event.getPmuFlags();

  // set the flags
  m_attr.disabled       = 1;
  m_attr.inherit        = (taskFlags & CAEvent::CAPERF_INHERIT) ? 1 : 0;
  m_attr.pinned         = 0;
  m_attr.exclusive      = 0;

  m_attr.exclude_user   = (profFlags & CAEvent::CAPERF_EXCLUDE_USER) ? 1 : 0;
  m_attr.exclude_kernel = (profFlags & CAEvent::CAPERF_EXCLUDE_KERNEL) ? 1 : 0;
  m_attr.exclude_hv     = (profFlags & CAEvent::CAPERF_EXCLUDE_HYPERVISOR) ? 1 : 0;
  m_attr.exclude_idle   = (profFlags & CAEvent::CAPERF_EXCLUDE_IDLE) ? 1 : 0;

  // PERF's user-space-tool set mmap & comm to 1 for the first counter and 0 for
  // other counters; In CAPerfCfg::initialize, we disable this for other counters;
  m_attr.mmap           = (profFlags & CAEvent::CAPERF_INCLUDE_MMAP_DATA) ? 1 : 0;
  m_attr.comm           = (profFlags & CAEvent::CAPERF_INCLUDE_COMM_DATA) ? 1 : 0;

  m_attr.inherit_stat   = (taskFlags & CAEvent::CAPERF_INHERIT_STAT) ? 1 :0;
  m_attr.enable_on_exec = (taskFlags & CAEvent::CAPERF_ENABLE_ON_EXEC) ? 1 : 0;
  m_attr.task           = (taskFlags & CAEvent::CAPERF_TRACE_TASK) ? 1 : 0;

  // In perf_event_attr struct, certain fields are added post 2.6.32 version.
#if defined(LINUX_PERF_PRECISE_IP_SUPPORT)
  m_attr.precise_ip     = m_event.getSkidFactor();
#endif

#if defined(LINUX_PERF_MMAP_DATA_SUPPORT)
  // non-exec mmap data; This is required to set only if the user has
  // asked for sample address PERF_SAMPLE_ADDR
  // Perf user-tool set this to 1 for the first counter and 0 for other
  // counters; In CAPerfCfg::initialize, we disable this for other counters;
  m_attr.mmap_data      =  (m_sampleAttr & PERF_SAMPLE_ADDR) ? 1 : 0;
#endif

#if defined(LINUX_PERF_SAMPLE_ID_ALL_SUPPORT)
  // From /usr/include/linux/perf_event.h
  //
  // If perf_event_attr.sample_id_all is set then all event types will
  // have the sample_type selected fields related to where/when
  // (identity) an event took place (TID, TIME, ID, CPU, STREAM_ID)
  // described in PERF_RECORD_SAMPLE.
  //
  m_attr.sample_id_all  = 1;
#endif

  // watermark
  if (m_wakeupByEvents) {
     m_attr.watermark = 0;
     m_attr.wakeup_events = m_waterMark;
  } else {
     m_attr.watermark = 1;
     m_attr.wakeup_watermark = m_waterMark;
  }

  m_profConfig->getPMUTarget().print();
  // CAPMUTarget &tgt = m_profConfig->getPMUTarget();
  // tgt.print();

  m_samplingEvent = (m_attr.sample_period) ? true : false;

  initEventData();
}


int
CAPerfEvent::initEventData()
{
  int cnt = 0;
  CAPMUTarget &pmuTgt = m_profConfig->getPMUTarget();
  const CACpuVec &cpuVec = m_profConfig->getCpuVec();
  const CAThreadVec &threadVec = m_profConfig->getThreadVec();

  if (m_pCountData) {
    return E_FAIL;
  }

  // in per-process, it should be 
  // number of tasks * cpus available (online)
  size_t nbr  = (pmuTgt.isSWP()) ? pmuTgt.m_nbrCpus
                                : cpuVec.size() * threadVec.size();
  int   *tgts = (pmuTgt.isSWP()) ? pmuTgt.m_pCpus : pmuTgt.m_pPids;

  m_nbrfds = nbr;
  CA_DBG_PRINTF(3, "m_nbrfds : %d\n" , m_nbrfds);

  m_pCountData = new CAEventCountData[nbr];

  for (int i=0; i<m_nbrfds; i++) {
    m_pCountData[i].m_type              = m_attr.type;
    m_pCountData[i].m_config            = m_attr.config;
    m_pCountData[i].m_fd                = -1;
    m_pCountData[i].m_sampleId          = UINT_MAX;
    m_pCountData[i].m_hasValidCountData = false;
    m_pCountData[i].m_nbrValues         = 0;
    memset(m_pCountData[i].m_values, 0, sizeof(m_pCountData[i].m_values));

    // set pid and cpu values for SWP mode
    if (pmuTgt.isSWP()) {
      m_pCountData[i].m_pid = -1;
      m_pCountData[i].m_cpu = tgts[i];
    }
  }

  if (pmuTgt.isSWP()) {
    goto err_exit;
  }

  // Set the details for Per-Process mode
  for (int i=0; i<cpuVec.size(); i++) {
    int cpu = cpuVec[i];

    for (int j=0; j < threadVec.size(); j++, cnt++) {
      m_pCountData[cnt].m_pid = threadVec[j];
      m_pCountData[cnt].m_cpu = cpu;
    }
  }

err_exit:
  return S_OK;
}

CAPerfEvent::CAPerfEvent(const CAPerfEvent& evt)
  : CAEventInternal(evt),
    m_profConfig(evt.m_profConfig)
{
  memcpy(&m_attr, &(evt.m_attr), sizeof(m_attr));

  m_samplingEvent = evt.m_samplingEvent;
  m_hasCountData  = evt.m_hasCountData;

  m_eventDataList.clear();
  m_eventDataList = evt.m_eventDataList;

  m_nbrfds        = evt.m_nbrfds;
  if (evt.m_pCountData) {
    m_pCountData = new CAEventCountData[m_nbrfds];
    for (int i=0; i<m_nbrfds; i++) {
      m_pCountData[i] = evt.m_pCountData[i];
    }
  }

  m_groupId       = evt.m_groupId;
}


CAPerfEvent&
CAPerfEvent::operator=(const CAPerfEvent& evt)
{
  CAPerfEvent::operator=(evt);

  m_profConfig = evt.m_profConfig;

  memcpy(&m_attr, &(evt.m_attr), sizeof(m_attr));

  m_hasCountData  = evt.m_hasCountData;
  m_samplingEvent = evt.m_samplingEvent;

  m_nbrfds        = evt.m_nbrfds;

  if (m_pCountData) {
    delete[] m_pCountData;
    m_pCountData = NULL;
  }

  m_eventDataList.clear();
  m_eventDataList = evt.m_eventDataList;

  if (evt.m_pCountData) {
    m_pCountData = new CAEventCountData[m_nbrfds];
    for (int i=0; i<m_nbrfds; i++) {
      m_pCountData[i] = evt.m_pCountData[i];
    }
  }

  m_groupId       = evt.m_groupId;

  return *this;
}


int
CAPerfEvent::startEvent(bool enable)
{
  CAPMUTarget &pmuTgt = m_profConfig->getPMUTarget();
  const CACpuVec &cpuVec = m_profConfig->getCpuVec();
  const CAThreadVec &threadVec = m_profConfig->getThreadVec();

  size_t nbr = m_nbrfds;
  int   *tgts = (pmuTgt.isSWP()) ? pmuTgt.m_pCpus : pmuTgt.m_pPids;

  CA_DBG_PRINTF(3, "nbr(%d) in startEvent\n", nbr);

  pid_t pid = -1;
  int group_fd = -1;
  int ret;
  int cpu;
  int fd;

  // Set perf_event_attr::disabled field
  m_attr.disabled = ~enable;
 
  if (! pmuTgt.isSWP()) {
    goto handle_per_process;
  }

  for (int i=0; i < nbr; i++) {
    cpu = tgts[i];

    fd = sys_perf_event_open(&m_attr, pid, cpu, group_fd, 0);
    int err = errno;

    if (fd < 0)
    {
      CA_DBG_PRINTF(3, "sys_perf_event_open failed..\n");
      if ((EACCES == err) || (EPERM == err))
      {
        CA_DBG_PRINTF(3, "sys_perf_event_open failed due to Access Permissions..\n");
        return E_ACCESSDENIED;
      }
      else if (ENODEV == err) {
          // invalid cpu list
         CA_DBG_PRINTF(3, "sys_perf_event_open failed - ENODEV..\n");
         return E_INVALIDARG;
      } else {
         CA_DBG_PRINTF(3, "sys_perf_event_open failed - errno(%d).\n", err);
         return E_FAIL;
      }
    } else {
      CA_DBG_PRINTF(3, "for cpu(%d) fd(%d)\n", cpu, fd);
    }

    m_pCountData[i].m_fd = fd;

    // If the event is sampling event, get the sample-id
    // We set the attr::read_format to PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_TOTAL_TIME_RUNNING
    // | PERF_FORMAT_ID;
    // for this the format of the data returned by read() on the fd is:
    //      struct read_format {
    //         u64		value;
    //         u64		time_enabled;
    //         u64		time_running;
    //         u64		id;
    //      } 
    //
    if (m_samplingEvent) {
      ret = readn(fd, m_pCountData[i].m_values, (CAPERF_MAX_NBR_VALUES * sizeof(uint64_t)));

      // on error, return
      if (-1 == ret) {
        CA_DBG_PRINTF(3, "error in reading samplie-id for event(%d, %lx), fd(%d) errno(%d)\n",
               m_attr.type, m_attr.config, fd, errno);
        return E_FAIL;
      }

      m_pCountData[i].m_sampleId =  m_pCountData[i].m_values[3];

      // Now push this into m_eventDataList
      // TODO: Eventually we will discard pCountData stuff
      CA_DBG_PRINTF(3, "push elemnt in m_eventDataList i(%d)\n", i);
      m_eventDataList.push_back(m_pCountData[i]);
    }
  }

handle_per_process:
  CA_DBG_PRINTF(3, "cpuVec size(%d), threadVec size(%d).\n",
		cpuVec.size(), threadVec.size());

  int cnt = 0;
  for (int i=0; i < cpuVec.size(); i++)
  {
    cpu = cpuVec[i];

    for (int j =0; j < threadVec.size(); j++, cnt++)
    {
      pid = threadVec[j];

      fd = sys_perf_event_open(&m_attr, pid, cpu, group_fd, 0);

      if (-1 == fd) {
        CA_DBG_PRINTF(3, 
               "sys_perf_event_open failed cpu(%d), pid(%d), cnt(%d).\n",
               cpu, pid, cnt);
      } else {
        CA_DBG_PRINTF(3, "for cpu(%d), pid(%d), cnt(%d), fd(%d)\n",
                      cpu, pid, cnt, fd);
      }

      m_pCountData[cnt].m_fd = fd;

      // If the event is sampling event, get the sample-id
      if (m_samplingEvent) {
        ret = readn(fd, m_pCountData[cnt].m_values, (CAPERF_MAX_NBR_VALUES * sizeof(uint64_t)));

        // on error, return
        if (-1 == ret) {
          CA_DBG_PRINTF(3, "error in reading samplie-id for event(%d, %lx), fd(%d) errno(%d)\n",
               m_attr.type, m_attr.config, fd, errno);
          return E_FAIL;
        }

        // If the event is sampling event, get the sample-id
        // We set the attr::read_format to PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_TOTAL_TIME_RUNNING
        // | PERF_FORMAT_ID;
        // for this the format of the data returned by read() on the fd is:
        //      struct read_format {
        //         u64		value;
        //         u64		time_enabled;
        //         u64		time_running;
        //         u64		id;
        //      } 
        //
        m_pCountData[cnt].m_sampleId =  m_pCountData[cnt].m_values[3];

        // Now push this into m_eventDataList
        // TODO: Eventually we will discard pCountData stuff
        CA_DBG_PRINTF(3, "push elemnt in m_eventDataList cnt(%d)\n", cnt);
        m_eventDataList.push_back(m_pCountData[i]);
      }
    }
  }

  CA_DBG_PRINTF(3, "size of m_eventDataList (%d)\n", m_eventDataList.size());
  return S_OK;
}


int
CAPerfEvent::enableEvent()
{
  int ret;
  int nbr_tgts = m_nbrfds;
  int fd;

  for (int i = 0; i < nbr_tgts; i++) {
    fd = m_pCountData[i].m_fd;
 
    if (-1 == fd) {
      continue;
    }

    ret = ioctl(fd, PERF_EVENT_IOC_ENABLE);
    // on error, return
    if (-1 == ret) {
      CA_DBG_PRINTF(3, "error in enabling event(%u, 0x%llx), fd(%d)\n",
             m_attr.type, m_attr.config, fd);
      return E_FAIL;
    }
  }

  return S_OK;
}


int
CAPerfEvent::disableEvent()
{
  int ret;
  int nbr_tgts = m_nbrfds;
  int fd;

  for (int i = 0; i < nbr_tgts; i++) {
    fd = m_pCountData[i].m_fd;

    ret = ioctl(fd, PERF_EVENT_IOC_DISABLE);

    // on error, return
    if (-1 == ret) {
      CA_DBG_PRINTF(3, "error in disabling event(%u, %lu), fd(%d)\n",
             m_attr.type, m_attr.config, fd);
      return E_FAIL;
    }
  }

  return S_OK;
}


int
CAPerfEvent::readCounterValue()
{
  int ret;
  int nbr_tgts = m_nbrfds;
  int fd;
  uint64_t  *values;

  for (int i = 0; i < nbr_tgts; i++) {
    fd = m_pCountData[i].m_fd;
    values = m_pCountData[i].m_values;

    if (fd < 0) {
      continue;
    }

    ret = readn(fd, values, (4*sizeof(uint64_t)));

    // on error, return
    if (-1 == ret) {
      CA_DBG_PRINTF(3, "error in reading event(%d, %lx), fd(%d) errno(%d)\n",
             m_attr.type, m_attr.config, fd, errno);
      return E_FAIL;
    }

    // CAEventCountData has valid counter values read..
    m_pCountData[i].m_hasValidCountData = true;

    // print the counter values...
    CA_DBG_PRINTF(3, " Event(%d, %ld), Value(%ld, %ld, %ld, %d)\n", m_attr.type,
           m_attr.config, values[0], values[1], values[2], values[3]);
  }

  m_hasCountData = true;

  return S_OK;
}


CAPerfEvent::~CAPerfEvent()
{
  // if fds are open close them
  // int nbr_tgts = (m_tgt.isSWP()) ? m_tgt.m_nbrCpus : m_tgt.m_nbrPids;
  int nbr_tgts = m_nbrfds;
  int fd;

  for (int i = 0; i < nbr_tgts; i++) {
    fd = m_pCountData[i].m_fd;

    if (fd < 0) {
      continue;
    }

    close(fd);
  }

  delete[] m_pCountData;
  m_eventDataList.clear();
}

void
CAPerfEvent::clear()
{
  return;
}


void
CAPerfEvent::print()
{
  // print m_attr fields..
  CA_DBG_PRINTF(3, "Event type : 0x%x\n", m_attr.type);
  CA_DBG_PRINTF(3, "Config : 0x%llx\n", m_attr.config);
  CA_DBG_PRINTF(3, "sample value %llu, sample by %s\n", m_attr.sample_period,
             (m_attr.freq) ? "frequency" : "period");
  CA_DBG_PRINTF(3, "sample type : 0x%llx\n",  m_attr.sample_type);
  CA_DBG_PRINTF(3, "read format : 0x%llx\n", m_attr.read_format);
  CA_DBG_PRINTF(3, "disabled : %s\n", (m_attr.disabled) ? "true" : "false");
  CA_DBG_PRINTF(3, "inherit : %llu\n", m_attr.inherit);
  CA_DBG_PRINTF(3, "pinned : %llu\n", m_attr.pinned);
  CA_DBG_PRINTF(3, "exclusive : %llu\n", m_attr.exclusive);
  CA_DBG_PRINTF(3, "exclude_user : %llu\n", m_attr.exclude_user);
  CA_DBG_PRINTF(3, "exclude_kernel : %llu\n", m_attr.exclude_kernel);
  CA_DBG_PRINTF(3, "exclude_idle : %llu\n", m_attr.exclude_idle);
  CA_DBG_PRINTF(3, "exclude_hv : %llu\n", m_attr.exclude_hv);
  CA_DBG_PRINTF(3, "mmap : %llu\n", m_attr.mmap);
  CA_DBG_PRINTF(3, "comm : %llu\n", m_attr.comm);
  CA_DBG_PRINTF(3, "inherit_stat : %llu\n", m_attr.inherit_stat);
  CA_DBG_PRINTF(3, "enable_on_exec : %llu\n", m_attr.enable_on_exec);
  CA_DBG_PRINTF(3, "task : %llu\n", m_attr.task);
#if defined(LINUX_PERF_PRECISE_IP_SUPPORT)
  CA_DBG_PRINTF(3, "precise_ip : %llu\n", m_attr.precise_ip);
#endif
#if defined(LINUX_PERF_MMAP_DATA_SUPPORT)
  CA_DBG_PRINTF(3, "mmap_data : %llu\n", m_attr.mmap_data);
#endif
#if defined(LINUX_PERF_SAMPLE_ID_ALL_SUPPORT)
  CA_DBG_PRINTF(3, "sample_id_all : %llu\n", m_attr.sample_id_all);
#endif

  CA_DBG_PRINTF(3, "wakeup %s, watermark %d\n", (m_attr.watermark) ? "bytes" : "events",
                m_attr.wakeup_events);

  CA_DBG_PRINTF(3, "nbr of fds : %d\n", m_nbrfds);
  return;
}
