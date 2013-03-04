//
// CAEventInternal.cpp
//

// Standard headers
#include <string.h>
#include <stdlib.h>

// Project headers
#include "CAEventInternal.h"


//
//  class CAEventInternal
//

CAEventInternal::CAEventInternal(const CAEventInternal &evt)
{
  m_event           = evt.m_event;
  m_groupLeader     = evt.m_groupLeader;

  m_samplebyFreq    = evt.m_samplebyFreq;
  m_samplingValue   = evt.m_samplingValue;
  m_sampleAttr      = evt.m_sampleAttr;

  m_readFormat      = evt.m_readFormat;

  m_wakeupByEvents  = evt.m_wakeupByEvents;
  m_waterMark       = evt.m_waterMark;
}

CAEventInternal&
CAEventInternal::operator= (const CAEventInternal &evt)
{
  m_event           = evt.m_event;
  m_groupLeader     = evt.m_groupLeader;

  m_samplebyFreq    = evt.m_samplebyFreq;
  m_samplingValue   = evt.m_samplingValue;
  m_sampleAttr      = evt.m_sampleAttr;

  m_readFormat      = evt.m_readFormat;

  m_wakeupByEvents  = evt.m_wakeupByEvents;
  m_waterMark       = evt.m_waterMark;

  return *this;
};

void
CAEventInternal::print()
{
  CA_DBG_PRINTF(3, "CAEventInternal Details :-\n");
  m_event.print();

  CA_DBG_PRINTF (3, "Sampling %s : %ld\n", (m_samplebyFreq) ? "Frequency" : "Period",
          m_samplingValue);

  CA_DBG_PRINTF (3, "Sample attributes : %lx\n", m_sampleAttr);
  CA_DBG_PRINTF (3, "read  format : %lx\n", m_readFormat);

  CA_DBG_PRINTF (3, "Wakeup by %s\n", (m_wakeupByEvents) ? "Events" : "Bytes");
  CA_DBG_PRINTF (3, "Wakeup Watermark : %d\n", m_waterMark);
  CA_DBG_PRINTF (3, "\n");
  return;
}
