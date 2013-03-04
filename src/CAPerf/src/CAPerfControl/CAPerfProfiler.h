//
// CAPerfProfiler.h
//

#ifndef _CAPERFPROFILER_H_
#define _CAPERFPROFILER_H_

// Standard Headers
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wchar.h>

// PERF header file
#ifdef __cplusplus
extern "C" {
  #include <poll.h>
  #include <linux/perf_event.h>
  #include <pthread.h>
}
#endif // __cplusplus

// Project HEaders
#include "CAPerfEvent.h"
#include "CAPerfCfg.h"
#include "CAProfiler.h"
#include "CAError.h"


// class CAPerf
//
//	class for PERF profiler
//
class CAPerfProfiler : public CAProfiler
{
public:
  CAPerfProfiler()
    : CAProfiler(), m_pPerfCfg(NULL), m_threadId(0)
  { 
  }

  ~CAPerfProfiler() {
    // If the sample reader thread is still running, stop it.
    stopSampleReaderThread();

    if (m_pPerfCfg) {
      delete m_pPerfCfg;
    }
  };

  // initialize the internal data structures..
  // check if the relevant profiling subsytem is available
  int initialize(CAProfileConfig  *config, CAPMUTarget *tgt = NULL);
  int setProfileConfig(CAProfileConfig  *config);
  int setPMUTarget(CAPMUTarget *tgt);

  // Profile Control 
  int startProfile(bool enable);
  int stopProfile();
  int enableProfile();
  int disableProfile();

  // Read the profile Data - counter values and sampled profile data
  int readCounters(CAEvtCountDataList **countData = NULL);
  // int getCounterValues();
  // int printCounterValues();

  int readSampleBuffers();

  // Clear the internal data structures
  void clear();

private:
  //This creates the thread to read PERF samples
  int createSampleReaderThread();
  int stopSampleReaderThread();

  pthread_t			m_threadId;

  // CAPerfCfg - PERF specific Profile Configuration
  // Contains:-
  //   - list of counting events 
  //   - list of sampling events 
  //   - target to be profiled - pids/cpus 
  CAPerfCfg        *m_pPerfCfg;
};

#endif // _CAPERFPROFILER_H_
