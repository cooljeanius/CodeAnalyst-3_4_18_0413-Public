//
// CAPMUSession.h
//

#ifndef _CAPMUSESSION_H_
#define _CAPMUSESSION_H_

// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <wchar.h>

// C++ STL headers
#include <list>
#include <string>

// project headers
#include "CAEvent.h"
#include "CAProfileConfig.h"
#include "CAPMUTarget.h"
#include "CAProfiler.h"

// Info:-
//
// There are 2 layers in this DataCollection model:-
//
// 1. OS independent Layer:-
//       - classes that are visible to user;
//       - contains more generic details (irrespective of underlying 
//         OS/profiling mechanism)  
//       - classes :- CAEvent
//                    CAProfileConfig
//                    CAPMUTarget
//                    CAEventCountData (output counter values)
//                    CAProfiler (abstract class)
//
// 2. OS/Profiler Dependent Layer:-
//       - This layer is more specific to the underlying OS/Profiling mechanism.
//       - We need to extend "CAProfiler" and implement the interfaces
///        to program the PMU, start/stop the profile & to collect
//         profile data.
//       - Each underlying profiling mechanism can have its own class
///        Ex: for PERF, 
//              CAPerfProfiler : public CAProfiler
//       - The CAProfileConfing and CAPMUTarget is available to this profiler
//       - They can exten "CAProfileConfig" & "CAEventInternal" to
//         create their own profiler specific internal data structures..
// 
// Well there should one more layer - that provides APIs to gather Hardware
//  PMU details like.. 
//     - processor model, cpu family
//     - Nbr of PMU counters available
//     - for the given event-name, retrieve event-id, umask, etc
//
// A data-collection tool (caprofile) is a client/user of this DataCollection
// library. The tool can defibe a "CAPMUSession" class that contains ..
//	- the profiler
//	- ProfilerConfig
//	- PMU Target
//	- Profile Data File  
//	- etc
//

//
// Classes
//
// CAEvent
//   - Generic class for Event Configuration 
//   - contains event-id, unitmaks, pmu-specific flags, etc;
//   - visible to client
//
// CAEventInternal
//   - Represnets either a Counting or Sampling Event to the backend 
//     "OS/Profiler" specific layer.
//   - internal class

// CAProfileConfig
//   - Generic class for the Profile Configuration
//   - has list of counting and sampling events represented by
//     CAEventInternal class
//   - visible to client
//
// CAPMUTarget
//   - describes the profiling target - pids/cpus
//   - visible to client
//
// CAProfiler
//   - Abstract class for a profiling mechanism
//   - Each profiling mechanism can inherit this to implement their
//     own start/stop profiling methods
//   - PERF / Oprofile / Windows driver / OpenCL ect
//
// CAEventCountData
//   - holds the counter values (in count mode)
//     event id, target info (pid/cpu-id), counter value
//   - visible to client

// CAPMUSession
//   - PMU Profile Session / Measurement Run
//   - associate CAProfiler, CAProfileConfig and CAPMUTarget;
//   - profile the target and gather the profile data
//   - For every data colelction run, a session object should be created
//   - write into profile data file
//
//	User Visisble


// If we implement for PERF
//
// CAPerfProfiler : public CAProfiler
//   - PERF specific profile mechanism APIs
//   - has its own internal profile configuration object - CAPerfCfg
//
// CAPerfCfg : public CAProfileConfig
//   - inherit CAProfileConfig and implement PERF specific stuff
//   - not user visible
//
// CAPerfEvent : public CAEventInternal
//   - inherit CAEventInternal and implement PERF specific stuff
//   - maintain fds and mmaps and other PERF specific internals
//   - not user visible
//


// class CAPMUSession
//	PMU Profile Session / Measurement Run
//	Should be associated with a CAProfileConfig object
//	Each session per active measurement run (data collection run)
//	For every data colelction run, a session object should be created
//
//	User Visisble
//
class CAPMUSession
{
public:
  CAPMUSession(CAProfiler       *profiler = NULL,
               CAProfileConfig  *profConfig = NULL,
               CAPMUTarget      *tgt = NULL)
    : m_pProfiler(NULL), m_pProfileConfig(NULL), m_pPmuTarget(NULL),
      m_state(CAPERF_PMU_SESSION_UN_INTIALIZED)
  { };

  ~CAPMUSession() { };
   
  int initialize(CAProfiler *profiler,
                 CAProfileConfig  *config,
                 CAPMUTarget *tgt);

  int setProfiler(CAProfiler  *profiler);
  int setProfileConfig(CAProfileConfig  *config);
  int setPMUTarget(CAPMUTarget *tgt);

  void setOutputFile(std::string file, bool overwrite=false) { 
    m_outputFile = file;
    if (m_pProfiler) {
      m_pProfiler->setOutputFile(m_outputFile, overwrite);
    }
  };

  // Profile Output file
  // int setOutputFile(char *file);
  std::string getOutputFile() const { return m_outputFile; };

  int startProfile(bool enable);
  int stopProfile();
  int enableProfile();
  int disableProfile();

  int readPMUCounters(CAEvtCountDataList  **countData);
  int writePMUCounterValues(); // dumps counter values into OutputFile;
  int printPMUCounterValues(); // prints counter values into stderr/stdout

  // CPU topology details
  int getCPUTopology();
  int writeCPUTopology(); // dumps CPU Topology values into OutputFile;
  int printCPUTopology(); // prints CPU Topology values into stderr/stdout

  enum m_CAPMUSessionStates {
    CAPERF_PMU_SESSION_UN_INTIALIZED  = -1, 
    // CA_PMU_SESSION_INTIALIZED     = 1, // when a profiler & config is added
    CAPERF_PMU_SESSION_READY          = 2,  // when a tgt is associated
    CAPERF_PMU_SESSION_ACTIVE         = 3,
    CAPERF_PMU_SESSION_INACTIVE       = 4, 
    CAPERF_PMU_SESSION_ERROR          = 5,
    CAPERF_PMU_SESSION_PAUSED         = 6,
  };

private:
  CAProfiler       *m_pProfiler;
  CAProfileConfig  *m_pProfileConfig;
  CAPMUTarget      *m_pPmuTarget;

  uint64_t          m_flags;        // Not Reqd ?
  uint64_t          m_state;        // session state

  std::string       m_outputFile;	// QFile ?

  int init();
};

#endif // _CAPMUSESSION_H_
