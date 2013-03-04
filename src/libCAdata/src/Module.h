#ifndef _MODULE_H_
#define _MODULE_H_

#include <string>
#include <map>
#include "Function.h"

/****************************************
 * class CA_Module
 *
 * Description:
 * This class represent the item in [MODULE] section
 */
class LIBCADATA_API CA_Module
{
public:

	enum MOD_MODE_TYPE {
		INVALIDMODTYPE = 0,
		UNMANAGEDPE,
		JAVAMODULE,
		MANAGEDPE,
        OCLMODULE,
		UNKNOWNMODULE,
		UNKNOWNKERNELSAMPLES
	};

	CA_Module();

	~CA_Module();

	bool operator< (const CA_Module& m) const 
	{ return (m_base < m.m_base);  };

	bool operator==(const CA_Module& m) const 
	{ return (m_base == m.m_base); };

	void setPath(wstring & path) 
	{ m_path = path; };

	wstring getPath() const 
	{ return m_path; };

	int getImdIndex() const 
	{ return m_imdIndex; };

	void setImdIndex (int imdIndex)
	{ m_imdIndex = imdIndex; };

	int getNumSubSection() const 
	{ return (unsigned int)m_funcMap.size(); };

	UINT64 getTotal() const 
	{ 
		if (m_total > 0)
			return m_total; 
		else if (m_aggTotal >= m_funcTotal)
			return m_aggTotal;
		else
			return m_funcTotal;
	};

	VADDR getBaseAddr() const 
	{ 
		// NOTE [Suravee]:
		// In case of non-java module, we make assumption that 
		// there is one [SUB] section.  Therefore, we can just
		// return the m_baseAddr of the first sub-section.
		// In case of Java module, this is not the case, so it
		// returns 0, and we need to get the m_baseAddr of the specified
		// JAVA function instead.
		if (1 == m_funcMap.size()) 
			return getBeginFunction()->second.m_baseAddr; 
		return 0;
	};

	UINT32 getModType() const
	{ return m_modType; };

	AddrFunctionMultMap::const_iterator getBeginFunction() const 
	{ return m_funcMap.begin(); };
	
	AddrFunctionMultMap::const_reverse_iterator getRbeginFunction() const 
	{ return m_funcMap.rbegin(); };

	AddrFunctionMultMap::const_iterator getEndFunction() const 
	{ return m_funcMap.end(); };
	
	AddrFunctionMultMap::const_reverse_iterator getRendFunction() const 
	{ return m_funcMap.rend(); };

	AddrFunctionMultMap::const_reverse_iterator getFunction(const VADDR addr) const
	{
		AddrFunctionMultMap::const_reverse_iterator rit = m_funcMap.rbegin();
		AddrFunctionMultMap::const_reverse_iterator rend = m_funcMap.rend();
		for (; rit != rend; rit++) {
			if (rit->first <= addr)
				break;
		}
		
		return rit;
	};

	AddrFunctionMultMap::const_iterator getLowerBoundFunction(const VADDR addr) const
	{ return m_funcMap.lower_bound(addr); };

	PidAggregatedSampleMap::const_iterator getBeginSample() const 
	{ return m_aggPidMap.begin(); };
	
	PidAggregatedSampleMap::const_iterator getEndSample() const 
	{ return m_aggPidMap.end(); };
	
	PidAggregatedSampleMap::const_iterator findSampleForPid(PID_T pid) const
	{ return m_aggPidMap.find(pid); };

	/*
	 * NOTE:
	 * This function is used by CaDataTranslator to 
	 * record sample after PRD translation. It adds 
	 *sample into the PidAggregatedSampleMap (for [MODULE] section) 
	 * and the AddrFunctionMultMap (for IMD file).
	 */
	void recordSample(const SampleInfo & sampleInfo, 
						UINT32 sampleCnt,
						VADDR funcAddr,
						const wstring & funcName,
						const wstring & jncName,
						const wstring & javaSrcName);


	void recordSample(PID_T pid, AggregatedSample *aggSample);
	void recordSample(VADDR addr, CA_Function *caFunc);

	void insertModMetaData(wstring modPath, int imdIndex, 
					PID_T pid, AggregatedSample & agSamp) 
	{
		m_path = modPath;
		m_imdIndex = imdIndex;
		insertModMetaData(pid, agSamp);
	};

	void insertModMetaData(PID_T pid, AggregatedSample & agSamp) 
	{		
		m_aggPidMap.insert(PidAggregatedSampleMap::value_type(pid, agSamp));
		m_aggTotal += agSamp.getTotal();
	};

	void insertModDetailData(VADDR addr, CA_Function & func) 
	{
		m_funcMap.insert(AddrFunctionMultMap::value_type(addr, func));
		m_funcTotal += func.getTotal();
	};

	bool doesAddressBelongToModule(VADDR addr, const CA_Function **ppFunc) const;

	bool compares(const CA_Module& m) const;

	void clear()
	{
		m_aggPidMap.clear();
		m_funcMap.clear();
	}

public:
	VADDR		m_base;
	UINT32		m_size;
	UINT32		m_modType;
	bool		m_isImdRead;
	bool		m_is32Bit;
	VADDR startVA;		// Starting VA of codeBytes
	VADDR endVA;		// Ending VA of codeBytes

private:
	//Updated by recordSample
	UINT64		m_total;
	wstring		m_path;
	int			m_imdIndex;

	// Used by CAProfileReader/Writer to store 
	// information from [MODDATA] section
	UINT64		m_aggTotal;
	PidAggregatedSampleMap	m_aggPidMap;

	// Used by IMDReader/Writer to store 
	// information from module detail section
	UINT64		m_funcTotal;
	AddrFunctionMultMap	m_funcMap;
};

/***********************************************
 * Description:
 * This represent the [MODULE] section
 */
typedef map<wstring, CA_Module> NameModuleMap;

#endif // _MODULE_H_
