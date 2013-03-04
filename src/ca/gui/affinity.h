#ifndef __AFFINITY_H__
#define __AFFINITY_H__

#include <sched.h>
#include <string>

using std::string;

class CA_Affinity
{
public:

	CA_Affinity();

	~CA_Affinity();

	int init(unsigned int numCpus = 0);

	int setAllCpus();

	CA_Affinity operator=(const CA_Affinity & rhs);

	void clear();
	
	int zero();
	
	int setAffinityFromHexString(string str);

	string getAffinityMaskInHexString();

	int setAffinityMaskForCpu(unsigned int cpu, bool bSet);

	int getAffinityMaskForCpu(unsigned int cpu);

	bool isZero();
	
	unsigned int getAffinityMaskSize()
	{ return m_maskSize; };

	unsigned int getNumCpus()
	{ return m_numCpus; };

	int ca_sched_setaffinity(pid_t pid);

	bool isValidForCurrentSystem();

	static bool isSupported();

private:
	cpu_set_t * m_pMask;
	unsigned int m_maskSize;
	unsigned int m_numCpus;

};


#endif //__AFFINITY_H__
