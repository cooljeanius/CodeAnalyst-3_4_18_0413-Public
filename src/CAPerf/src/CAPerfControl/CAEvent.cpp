//
// CAEvent.cpp
//

// Standard headers
#include <string.h>
#include <stdlib.h>

// Project headers
#include "CAEvent.h"

//
//  class CAEvent
//

CAEvent::CAEvent()
{
  uint32_t eventid = UINT_MAX;   // dummy event id
  uint32_t unitmask = UINT_MAX;  // dummy unitmask

  init(CAPERF_TYPE_HW_CPU_CYCLES,
       eventid,
       unitmask,
       (uint32_t) (CAPERF_EXCLUDE_IDLE | CAPERF_EXCLUDE_HYPERVISOR),
       (uint32_t) ( CAPERF_INHERIT | CAPERF_TRACE_TASK));
}

CAEvent::CAEvent (
  uint32_t  type,
  uint32_t  pmuFlags,
  uint32_t  taskFlags
)
{
  uint32_t eventid = UINT_MAX;   // dummy event id
  uint32_t unitmask = UINT_MAX;  // dummy unitmask

  init(type,
       eventid,
       unitmask,
       pmuFlags,
       taskFlags);
}

CAEvent::CAEvent (
  uint32_t  eventid,
  uint32_t  unitmask,
  uint32_t  pmuFlags,
  uint32_t  taskFlags
)
{
  init(CAPERF_TYPE_RAW, 
       eventid,
       unitmask,
       pmuFlags,
       taskFlags);
}

void
CAEvent::init (
  uint32_t  type,
  uint32_t  eventid,
  uint32_t  unitmask,
  uint32_t  pmuFlags,
  uint32_t  taskFlags
)
{
  m_eventId  = eventid;
  m_unitMask = unitmask;

  // set the m_type and m_config fields
  m_type     = type;
  if (CAPERF_TYPE_RAW == m_type)
  {
    // m_config = (((unitmask & 0xff) << 8) | (eventid & 0xff))
    //            & AMD_PMU_EVENT_AND_UMASK;
    m_config = (  (uint64_t)(((unitmask & 0xff) << 8) | (eventid & 0xff))
                | ((uint64_t)(eventid & 0xff00) << 24));
  }

  m_pEventName   = NULL;
  m_pEventAlias  = NULL;
  m_pmuFlags     = pmuFlags;
  m_taskFlags    = taskFlags;
  m_skidFactor   = CAPERF_SAMPLE_ARBITRARY_SKID;
  m_eventCounter = UINT_MAX;
  m_initialized  = true;
}

int
CAEvent::initialize(
  uint32_t  type,
  uint32_t  pmuFlags,
  uint32_t  taskFlags
)
{
  uint32_t eventid  = UINT_MAX;   // dummy event id
  uint32_t unitmask = UINT_MAX;  // dummy unitmask

  init(type, 
       eventid,
       unitmask,
       pmuFlags,
       taskFlags);

  int ret = validate();
  m_initialized = (! ret) ? true: false;
  return ret;
}

int
CAEvent::initialize(
  uint32_t  eventid,
  uint32_t  unitmask,
  uint32_t  pmuFlags,
  uint32_t  taskFlags
)
{
  init(CAPERF_TYPE_RAW, 
       eventid,
       unitmask,
       pmuFlags,
       taskFlags);

  int ret = validate();
  m_initialized = (! ret) ? true: false;
  return ret;
}


// CAEvent::validate
//
// check for the validity of all the fields..
//   - event-id, uintmask, profileflags, taslflags
//
// TODO: return proper errorcode
//
int
CAEvent::validate()
{
  int ret;

  // Validate event & unitmask
  if ( (ret = validateEvent()) != 0) {
    return ret;
  }

  // validate PMU flags
  if (! validatePmuFlags()) {
    return false;
  }

  // validate task flags
  if (! validateTaskFlags()) {
    return false;
  }

  return true;
}


// copy ctor
CAEvent::CAEvent(const CAEvent &evt)
{
  copy(evt);
}


// assignment operator
CAEvent &
CAEvent::operator= (const CAEvent &evt)
{
  if (m_pEventName) {
    free(m_pEventName);
    m_pEventName = NULL;
  }
  if (m_pEventAlias) {
    free(m_pEventAlias);
    m_pEventAlias = NULL;
  }

  copy(evt);

  return *this;
}


void
CAEvent::copy(const CAEvent &evt)
{
  m_type     = evt.m_type;
  m_config   = evt.m_config;
  m_eventId  = evt.m_eventId;
  m_unitMask = evt.m_unitMask;

  m_pEventName = (! evt.m_pEventName)
                    ? NULL
                    : strdup(evt.m_pEventName);

  m_pEventAlias = (! evt.m_pEventAlias)
                         ? NULL
                         : strdup(evt.m_pEventAlias);

  m_pmuFlags      = evt.m_pmuFlags;
  m_taskFlags     = evt.m_taskFlags;
  m_eventCounter  = evt.m_eventCounter;
  m_initialized   = evt.m_initialized;
}

int
CAEvent::validateEvent()
{
  // TODO
  // Validate the event - use CEventsFile::findEventByValue
  // Validate the unitmask - use CEventsFile::UnitMaskList iter
  return true;
}


// dtor
CAEvent::~CAEvent()
{
  clear();
}

void
CAEvent::clear()
{
  m_initialized = false;
  // Re-set all the fields..

  if (m_pEventName) {
    free(m_pEventName);
  }

  if (m_pEventAlias) {
    free(m_pEventAlias);
  }
}

void
CAEvent::print()
{
   CA_DBG_PRINTF(3,"type          : %d\n", m_type);
   CA_DBG_PRINTF(3,"config        : 0x%lx\n", m_config);
   CA_DBG_PRINTF(3,"eventId       : 0x%x\n", m_eventId);
   CA_DBG_PRINTF(3,"uintmask      : 0x%x\n", m_unitMask);
   CA_DBG_PRINTF(3,"profile flags : 0x%x\n", m_pmuFlags);
   CA_DBG_PRINTF(3,"task flags    : 0x%x\n", m_taskFlags);

   return;
}
