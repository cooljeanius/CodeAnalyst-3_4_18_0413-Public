//
// CAPerfEvent.h
//

#ifndef _CAPERFEVENT_H_
#define _CAPERFEVENT_H_

// Standard Headers
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <wchar.h>
#include <linux/version.h>

// C++ Headers
#include <list>

// PERF header file
#ifdef __cplusplus
extern "C" {
  #include <poll.h>
  #include <linux/perf_event.h>
}
#endif // __cplusplus

// Project HEaders
#include "CAEvent.h"
#include "CAEventInternal.h"
#include "CAProfileConfig.h"
#include "CAPMUTarget.h"


#ifdef __cplusplus
extern "C" {
int sys_perf_event_open(struct perf_event_attr *attr,
                    pid_t pid, int cpu, int group_fd,
                    unsigned long flags);
}
#endif // __cplusplus

// Certain fields in perf_event_attr structure are added post 2.6.32
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
  // Breakpoint support
  #define LINUX_PERF_BREAKPOINT_SUPPORT	1

  // alignment and emulation faults
  #define	LINUX_PERF_SW_FAULTS_SUPPORT	1
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,34)
  #define	LINUX_PERF_PRECISE_IP_SUPPORT	1
  #define	LINUX_PERF_GUEST_KERNEL_SUPPORT	1
  #define	LINUX_PERF_GUEST_USER_SUPPORT	1
#endif 

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
  #define	LINUX_PERF_MMAP_DATA_SUPPORT	1
#endif 

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,37)
  #define	LINUX_PERF_SAMPLE_ID_ALL_SUPPORT	1
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,38)
  #define	LINUX_PERF_EXT_CONFIG_SUPPORT	1
#endif


class CAPerfEvent;
class CAPerfCfg;

//
// typedefs
//
typedef std::list<CAPerfEvent> CAPerfEvtGrp;
typedef std::list<CAPerfEvent> CAPerfEvtList;
typedef std::list<CAPerfEvtGrp> CAPerfEvtGrpList;


// class CAPerfEvent
//
//   - inherit CAEvent and implement PERF specific Event Configuration
//   - maintains PERF specific - fd, mmap etc
//   - NoT UsEr ViSiBlE
//
class CAPerfEvent : public CAEventInternal
{
public:

  CAPerfEvent(CAEventInternal &event, CAPerfCfg *profConfig)
    : CAEventInternal(event),
      m_profConfig(profConfig),
      m_samplingEvent(false), m_hasCountData(false), m_pCountData(NULL)
  {
    // initialize perf_event_attr structure from CAEventInternal object
    initialize();

    m_eventDataList.clear();

    m_hasCountData  = false;
    m_groupId       = -1;	// _UNUSED_
  };

  CAPerfEvent(const CAPerfEvent &evt);
  CAPerfEvent& operator=(const CAPerfEvent &evt);

  ~CAPerfEvent();

  void initialize();
  void clear();
  void print();

  // profile control APIs
  int startEvent(bool enable = false);
  int enableEvent();
  int disableEvent();

  int readCounterValue();
  int readSampleBuffer();

  perf_event_attr  getAttribute() { return m_attr; };
  uint64_t         getSampleType() { return m_attr.sample_type; }
#ifdef LINUX_PERF_SAMPLE_ID_ALL_SUPPORT
  bool             getSampleIdAll() { return m_attr.sample_id_all; }
#endif

  // APIs to set the perf_event_attr::mmap, mmap_data and comm fields
  void setPerfAttrMmap(bool value) { m_attr.mmap = (true == value) ? 1 : 0; }
#ifdef LINUX_PERF_MMAP_DATA_SUPPORT
  void setPerfAttrMmapData(bool value) { m_attr.mmap_data = (true == value) ? 1 : 0; }
#endif
  void setPerfAttrComm(bool value) { m_attr.comm = (true == value) ? 1 : 0; }

  // returns the number of entries in CAEventCountData
  int getEventData(CAEventCountData **data) const {
    // if (! m_hasCountData) {
    //   return 0;
    // }

    if (!m_pCountData) {
      return -1;
    }

    if (data) {
      *data = m_pCountData;
    }

    return m_nbrfds;
  }

  const CAEventDataList * getEventDataList() const {
    return &m_eventDataList;
  }

private:
  int initEventData();

private:
  CAPerfCfg         *m_profConfig;

  // PERF event attribute structure
  perf_event_attr    m_attr;
  // bool               m_countingEvent;
  bool               m_samplingEvent;

  bool               m_hasCountData;
  int                m_nbrfds;

  // TODO: Use list and remove m_pCountData;
  CAEventCountData  *m_pCountData;
  CAEventDataList    m_eventDataList;

  int                m_groupId;

  // From perf/design.txt:
  //
  // Notification of events is possible through poll()/select()/epoll() and
  // fcntl() managing signals.
  // Normally a notification is generated for every page filled, however one
  // can additionally set perf_event_attr.wakeup_events to generate one every
  // so many counter overflow events.
};

#endif //  _CAPERFEVENT_H_
