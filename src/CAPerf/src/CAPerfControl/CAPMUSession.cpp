//
// CAPMUSession.cpp
//

#include "CAPMUSession.h"
#include "CAError.h"

int
CAPMUSession::initialize(
  CAProfiler       *profiler,
  CAProfileConfig  *profConfig,
  CAPMUTarget      *pmuTarget
)
{
  int ret;

  ret = setProfiler(profiler);
  if (S_OK != ret) {
    CA_DBG_PRINTF(3, "setProfiler failed, ret(%d).\n", ret);
    return E_INVALID_ARG;
  }

  ret = setProfileConfig(profConfig);
  if (S_OK != ret) {
    CA_DBG_PRINTF(3, "setProfileConfig failed, ret(%d).\n", ret);
    return E_INVALID_ARG;
  }

  ret = setPMUTarget(pmuTarget);
  if (S_OK != ret) {
    CA_DBG_PRINTF(3, "setPMUTarget failed, ret(%d).\n", ret);
    return E_INVALID_ARG;
  }

  return ret;
}

int
CAPMUSession::setProfiler(CAProfiler  *profiler)
{
  int ret;

  if (! profiler) {
    return E_INVALID_ARG;
  }

  m_pProfiler = profiler;
  // TODO: Check for valid CAProfiler
  // get the state from the profiler and check it is initialized and ready...

  return init();
}

int
CAPMUSession::setProfileConfig(CAProfileConfig  *config)
{
  int ret;

  if (! config) {
    return E_INVALID_ARG;
  }

  m_pProfileConfig = config;
  // TODO: Check for valid CAProfileConfig 
  // get the state from the profileconfig and check it is initialized

  return init();
}

int
CAPMUSession::setPMUTarget(CAPMUTarget  *pmuTarget)
{
  int ret;

  if (! pmuTarget) {
    return E_INVALID_ARG;
  }

  m_pPmuTarget = pmuTarget;
  // TODO: Check for valid CAPMUTarget
  // get the state from the profileconfig and check it is initialized

  return init();
}


int
CAPMUSession::init()
{
  int ret;

  // If its already in READY state, return
  if (CAPERF_PMU_SESSION_READY == m_state) {
    return S_OK; // error ?
  }

  // If all the components are available, initialize them
  if (m_pProfiler && m_pProfileConfig && m_pPmuTarget)
  {
    // set the output file
    // FIXME: check for valid value in string
    m_pProfiler->setOutputFile(m_outputFile);

    ret = m_pProfiler->initialize(m_pProfileConfig, m_pPmuTarget);
    if (S_OK != ret) {
      CA_DBG_PRINTF(3, "Error while initializing the profiler, ret(%d).\n",
                   ret);
      return ret;
    }
    // set the state to READY

    // post condition  - TODO: Check the profiler state
    m_state = CAPERF_PMU_SESSION_READY;
  }

  return S_OK;
}


int
CAPMUSession::startProfile(bool enable)
{
  if (CAPERF_PMU_SESSION_READY != m_state) {
    return E_INVALID_STATE;
  }
  int ret = m_pProfiler->startProfile(enable);
  if (S_OK == ret) {
    m_state = (enable) ? CAPERF_PMU_SESSION_ACTIVE
                       : CAPERF_PMU_SESSION_INACTIVE;
  }

  return ret;
}

int
CAPMUSession::stopProfile()
{
  // should be Active or Paused state
  if (   (CAPERF_PMU_SESSION_ACTIVE != m_state) 
      && (CAPERF_PMU_SESSION_PAUSED != m_state))
  {
    return E_INVALID_STATE;
  }

  int ret = m_pProfiler->stopProfile();
  m_state = (S_OK == ret) ? CAPERF_PMU_SESSION_INACTIVE
                          : CAPERF_PMU_SESSION_ERROR;

  return ret;
}


int
CAPMUSession::enableProfile()
{
  // Can either be ins INACTIVE or PAUSED state
  if (   (CAPERF_PMU_SESSION_INACTIVE != m_state)
     &&  (CAPERF_PMU_SESSION_PAUSED != m_state) )
  {
    return E_INVALID_STATE;
  }
  int ret = m_pProfiler->enableProfile();
  m_state = (S_OK == ret) ? CAPERF_PMU_SESSION_ACTIVE
                          : CAPERF_PMU_SESSION_ERROR;
  return ret;
};


int
CAPMUSession::disableProfile()
{
  if (CAPERF_PMU_SESSION_ACTIVE != m_state) {
    return E_INVALID_STATE;
  }
  int ret = m_pProfiler->disableProfile();
  m_state = (S_OK == ret) ? CAPERF_PMU_SESSION_PAUSED
                          : CAPERF_PMU_SESSION_ERROR;
  return ret;
};

int
CAPMUSession::readPMUCounters(CAEvtCountDataList  **countData)
{
  if (NULL == countData) {
    return E_INVALID_ARG;
  }

  if (   (CAPERF_PMU_SESSION_ACTIVE != m_state)
      && (CAPERF_PMU_SESSION_INACTIVE != m_state) 
      && (CAPERF_PMU_SESSION_PAUSED != m_state) )
  {
    return E_INVALID_STATE;
  }

  int ret = m_pProfiler->readCounters(countData);
  if (S_OK != S_OK) {
    CA_DBG_PRINTF(3,
            "Error in CAPMUSession::readPMUCounters, ret(%d).\n", ret);
  }

  return ret;
}

