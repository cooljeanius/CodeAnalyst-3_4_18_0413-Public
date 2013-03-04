//
// CAPMUTarget.h
//

#ifndef _CAPMUTARGET_H
#define _CAPMUTARGET_H

// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <wchar.h>

// C++ Headers
#include <vector>

typedef std::vector<int> CACpuVec;
typedef std::vector<pid_t> CAThreadVec;

class CAPMUTarget
{
public:
  // Ctors
  // The cpus can be specified here, if the user wnats to profile
  // the target-pid on some select CPUs.
  CAPMUTarget(size_t  nbrPids,
              pid_t  *pids,
              size_t  nbrCpus = 0,
              int    *cpus = NULL)
    : m_nbrPids(nbrPids),
      m_pPids(pids),
      m_nbrCpus(nbrCpus),
      m_pCpus(cpus)
  {
    m_isSWP = false;
  };

  CAPMUTarget(size_t  nbrCpus,
              int    *cpus)
    : m_nbrPids(0),
      m_pPids(NULL),
      m_nbrCpus(nbrCpus),
      m_pCpus(cpus)
  {
    m_isSWP = true;
  };

  CAPMUTarget()
    : m_nbrPids(0), m_pPids(NULL), m_nbrCpus(0),
      m_pCpus(NULL), m_isSWP(false)
  {
  };

  // Dtor
  ~CAPMUTarget()
  {
    if (m_pPids) {
      delete[] m_pPids;
      m_pPids = NULL;
    }

    if (m_pCpus) {
      delete[] m_pCpus;
      m_pCpus = NULL;
    }
  }

  // Copy ctor
  CAPMUTarget (const CAPMUTarget &tgt);

  // Assignment operator
  CAPMUTarget& operator= (const CAPMUTarget &tgt);

  // int  setPids(size_t nbr, pid_t *pids);

  // specify target-pid(s) for per process mode
  // The cpus can be specified here, if the user wants to profile
  // the target-pid(s) on some select CPUs.
  int  setPids(size_t nbr, pid_t *pids, size_t nbrCpus=0, int *cpus=NULL);

  // specify cpu(s) for system wide mode
  int  setCpus(size_t nbr, int *cpus);
  bool isSWP() { return m_isSWP; };

  void getPids(size_t *nbr, int **pids) const
  {
    if (nbr) { *nbr = m_nbrPids; }
    if (pids) { *pids = m_pPids; }

    return;
  };

  void getCpus(size_t *nbr, int **cpus) const
  {
    if (nbr) { *nbr = m_nbrCpus; }
    if (cpus) { *cpus = m_pCpus; }

    return;
  };

  void print();

public:
  size_t  m_nbrPids;
  pid_t  *m_pPids;

  size_t  m_nbrCpus;
  int    *m_pCpus;

  bool    m_isSWP;
};

#endif // _CAPMUTARGET_H
