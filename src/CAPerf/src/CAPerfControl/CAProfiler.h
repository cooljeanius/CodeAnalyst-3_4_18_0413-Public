//
// CAProfiler.h
//

#ifndef _CAPROFILER_H_
#define _CAPROFILER_H_

// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <wchar.h>

// C++ STL headers
#include <string>
#include <list>

// project headers
#include "CAEvent.h"
#include "CAProfileConfig.h"
#include "CAPMUTarget.h"
#include "CAError.h"


// class CAProfiler
//
// Generic class to provide the profiling mechanism
//	Can be used to support PERF, Oprofile
//	Windows driver, OpenCL library tracing, GPU counter profiling ??
//
//	A Layer on top of OS perf layer - PERF/OProfile/Windows Driver etc
//
class CAProfiler
{
public:
  CAProfiler()
    : m_pProfileConfig(NULL), m_pTarget(NULL),
      m_state(CAPERF_PROFILER_NOT_INITIALIZED)
  {
  }

  virtual ~CAProfiler ()
  {
    // DO Nothing
  }

  // initialize the internal data structures..
  // check if the relevant profiling subsytem is available
  virtual int initialize(CAProfileConfig  *config, CAPMUTarget *tgt) { };
  virtual int setProfileConfig(CAProfileConfig  *config) {};
  virtual int setPMUTarget(CAPMUTarget *tgt) {};

  // Profiling related APIs
  virtual int startProfile(bool enable) = 0;
  virtual int stopProfile() = 0;
  virtual int enableProfile() = 0;
  virtual int disableProfile() = 0;
  // virtual int pauseProfile() = 0;
  // virtual int resumeProfile() = 0;

  // Read the profile Data
  virtual int readCounters(CAEvtCountDataList **countData = NULL) = 0;
  // virtual int getCounterValues() { return E_NOT_IMPLEMENTED; }
  // virtual int printCounterValues() { return E_NOT_IMPLEMENTED; }

  // TODO: Provide a callback mechanism; The user can register
  // a callback function. If a callback function is registered
  // it will be invoked whenever a buffer overflow notification
  // occurs. The base address of the sample-buffer and size of
  // the sample buffer will be passed to the callback routine.
  //
  // Its upto the callback to do whatever it wants ..
  //   - write into a file
  //   - process and constructs histograms..
  //   - print/dump...
  //
  virtual int readSampleBuffers() = 0;

  // Clear the internal data structures
  // clears both CAProfileConfig and CAPMUTarget
  void clear() {
    m_pTarget = NULL;
    m_pProfileConfig = NULL;
  }

  void setOutputFile(std::string file, bool overwrite=false) {
    m_outputFile = file;
    if (m_pProfileConfig) {
      m_pProfileConfig->setOutputFile(m_outputFile, overwrite);
    }
  };
  std::string getOutputFile() { return m_outputFile; };

  // Profile(r) state
  enum  CAProfileState {
        CAPERF_PROFILER_NOT_INITIALIZED = -1,

        // Set when a profile config is associated with the profiler
        CAPERF_PROFILER_INITIALIZED     =  1,

        // Set when a PMU Target is associated with the profiler,
        CAPERF_PROFILER_READY           =  2,

        CAPERF_PROFILER_ACTIVE          =  3,
        CAPERF_PROFILER_INACTIVE        =  4,
        CAPERF_PROFILER_ERROR           =  5,
        CAPERF_PROFILER_PAUSED          =  6
  };

protected:
  // Profile Configuration;
  CAProfileConfig  *m_pProfileConfig;

  // Target to be profiled;
  CAPMUTarget      *m_pTarget;

  // Profile state
  CAProfileState    m_state;

  // output profile data file
  std::string       m_outputFile;
};

#endif // _CAPROFILER_H_
