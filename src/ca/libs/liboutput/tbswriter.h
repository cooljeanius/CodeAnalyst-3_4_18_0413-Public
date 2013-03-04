#ifndef _LIB_EBSWRITER_H_
#define _LIB_EBSWRITER_H_

#include <string>
#include <map>
#include <set>
#include <bfd.h>

#include "stdafx.h"

using namespace std;
enum output_stage {
	evOut_OK = 0,
	evOut_TaskSummary,
	evOut_ModSummary,
	evOut_ModDetail,
	evOut_JitDetail,
};

class CTbsWriter 
{
public:
    CTbsWriter(); 
    ~CTbsWriter();

	void close();

    bool open_file_to_write(string path);

	// Write Environment section
    bool writeEnvHeader(unsigned int ncpu, unsigned int nmodules,
			unsigned int nevents, UINT64 nsamples, 
			EventMaskEncodeMap ev_map, unsigned long cpuFamily,
			unsigned long cpuModel, unsigned int missed = 0);

	// Write ProcessData section
	bool writeProcSectionProlog();

	bool writeProcLineData(string taskname, 
			unsigned long long taskid, SampleDataMap &data, 
			bool b32Bit, bool bHasCallStack = false);

	bool writeProcSectionEpilog();

	// Write ModData section
	bool writeModSummaryProlog();

	bool writeModSummaryLine(string mod, unsigned long long taskid,
			SampleDataMap &data,	bool b32Bit);
	bool writeModSummaryEpilog();

	// Write prolog for each module section
	bool writeModDetailProlog(string modname, unsigned int modtype,
			unsigned long long total, unsigned int itemcount,
			unsigned long long base = 0,
	 		unsigned int size = 0);

	bool writeModDetailLine( unsigned long long taskid, 
				unsigned long long threadid,
				unsigned long long sampAddr, 
				SampleDataMap &data);

	bool writeModDetailEpilog();

	bool writeJitProlog( string jitSym, 
			string jitSrc,
			unsigned long long jitAddr,
			unsigned int jitSize,
			unsigned int itemcount);
	
	bool writeJitEpilog();

private:
    string m_output_name;
	FILE* m_out_file;

    EventMaskEncodeMap m_ev_map;
	
	unsigned m_stage;
};

#endif
