//
// CAPerfCfg.h
//

#ifndef _CAPERFCFG_H_
#define _CAPERFCFG_H_

// Standard Headers
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wchar.h>

#if defined(__i386__)
// #include "../../arch/x86/include/asm/unistd.h"
#define rmb()           asm volatile("lock; addl $0,0(%%esp)" ::: "memory")
#define cpu_relax()     asm volatile("rep; nop" ::: "memory");
// #define CPUINFO_PROC    "model name"
#endif

#if defined(__x86_64__)
// #include "../../arch/x86/include/asm/unistd.h"
#define rmb()           asm volatile("lfence" ::: "memory")
#define cpu_relax()     asm volatile("rep; nop" ::: "memory");
// #define CPUINFO_PROC    "model name"
#endif

// PERF header file
#ifdef __cplusplus
extern "C" {
  #include <poll.h>
  #include <linux/perf_event.h>
}
#endif // __cplusplus


// C++ Headers
#include <list>

// Project Headers
#include "CAError.h"
#include "CAPerfEvent.h"
#include "CAProfileConfig.h"
#include "CAPMUTarget.h"
#include "CAPerfDataWriter.h"

struct CAPerfMmap
{
  void     *m_base;
  int       m_mask;   // TODO: Use proper name
  uint32_t  m_prev;

  void set(void *base, int mask, uint32_t prev) {
    m_base = base;
    m_mask = mask;
    m_prev = prev;
  };

  void setBase(void *base) {
    m_base = base;
  };

  void setMask(int mask) {
    m_mask = mask;
  };

  void setBase(uint32_t prev) {
    m_prev = prev;
  };
};

// class CAPerfCfg
//
// PERF specific Profile Event Configuration (attributes ?)
// Inherits CAProfileConfig to construct PERF specific structs.
//
//
class CAPerfCfg : public CAProfileConfig
{
public:
  CAPerfCfg(CAProfileConfig &cfg, CAPMUTarget &tgt);
  ~CAPerfCfg();

  CAPerfCfg(const CAPerfCfg& cfg);
  CAPerfCfg& operator=(const CAPerfCfg& cfg);

  int initialize();
  int startProfile(bool enable=true);
  int stopProfile();
  int enableProfile();
  int disableProfile();

  int readSampleData();
  int readMmapBuffers();
  int readCounters(CAEvtCountDataList **countData = NULL);

  uint16_t            getSampleRecSize(uint64_t sampleType, bool sampleIdAll);
  CAPMUTarget &       getPMUTarget() const { return m_pmuTgt; };
  const CACpuVec &    getCpuVec() const { return m_cpuVec; };
  const CAThreadVec & getThreadVec() const { return m_threadVec; };

  void clear();

  // Debug APIs
  void print();
  int  printSample(void *data);
  int  printCounterValues();

private:
  int initSamplingEvents();

protected:
  // PMU Target
  CAPMUTarget        &m_pmuTgt;

  uint32_t            m_nbrCtrPerfEvents;
  CAPerfEvtList       m_ctrPerfEventList;
  uint32_t            m_nbrSamplingPerfEvents;
  CAPerfEvtList       m_samplingPerfEventList;

  uint16_t	      m_sampleRecSize;

  // Event Count Data
  CAEvtCountDataList  m_countData;

  int                 m_nbrFds; // UNUSED

  uint32_t            m_mmapPages;
  uint32_t            m_pageSize;
  size_t              m_mmapLen;  // length of the memory mmap
  int                 m_mmapProt;
  int                 m_nbrMmaps;
  CAPerfMmap         *m_mmap;

  // Fds for reading PERF sample data buffers
  int                 m_pollFds;
  struct pollfd      *m_samplingPollFds;

  // cpus that are online in the system
  CACpuVec            m_cpuVec;
  // target-pids' threads
  CAThreadVec         m_threadVec;

  bool                m_profileCompleted;
  bool                m_useIoctlRedirect;

  // Testing..
  void               *m_pData;

  //TODO: do it properly..
  CAPerfDataWriter    m_dataWriter; 

  // TODO: 
  // - support for event groups, Multiplexing
  // - mmap overwrite
  // - union perf_event event_copy; ??
  // - need a flag for perf output redirect ?
};

#endif //  _CAPERFCFG_H_
