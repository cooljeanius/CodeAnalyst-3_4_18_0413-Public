//$Id: CaProfileWriter.h 17060 2009-08-25 15:41:38Z ssuthiku $

//  Interface for the CADataWriter class.

/*
// CodeAnalyst for Open Source
// Copyright 2002 - 2011 Advanced Micro Devices, Inc.
// You may redistribute this program and/or modify this program under the terms
// of the GNU General Public License as published by the Free Software 
// Foundation; either version 2 of the License, or (at your option) any later 
// version.
//
// This program is distributed WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED 
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  See the 
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifndef _CAPROFILEWRITER_H_
#define _CAPROFILEWRITER_H_

#include <string>
#include <map>
#if ( !defined (_WIN32) && !defined (_WIN64) )
#include <bfd.h>
#endif

#include "libCAdata.h"
#include "CaDataWriter.h"
#include "CaProfileInfo.h"
#include "libCAdata_Process.h"
#include "Module.h"

using namespace std;

/**********************************
 * class CaProfileWriter
 *
 * Description:
 * This class implements TBP/EBP writer which is the
 * main interface for creating profile data
 */
class LIBCADATA_API CaProfileWriter : public CaDataWriter 
{
public:

	CaProfileWriter(); 

	~CaProfileWriter();

	int getErrorCode() {return m_err;};

	bool write(wstring const & path,
			CaProfileInfo * profileInfo,
			PidProcessMap * procMap,
			NameModuleMap * modMap,
			CoreTopologyMap * topMap = 0);

private:
	
	enum profile_output_stage 
	{
		evOut_TaskSummary = 1,
		evOut_ModSummary,
	};

	virtual bool open(wstring path);

	bool writeProfileInfo(CaProfileInfo * env, PidProcessMap * procMap, CoreTopologyMap * pTopMap);

	bool writeProfileTotal(PidProcessMap * procMap);

	bool writeProcSection(PidProcessMap * procMap);

	bool writeModSectionAndImd(NameModuleMap * modMap);

	bool writeProcSectionProlog();

	bool writeProcLineData(CA_Process & proc, PID_T pid);

	bool writeProcSectionEpilog();

	bool writeModSectionProlog();

	bool writeModData(CA_Module & mod);

	bool writeModLineData(CA_Module & mod, 
							PID_T pid, 
							const AggregatedSample & agSamp);

	bool writeModSectionEpilog();

private:
	EventEncodeVec 			*m_pEventVec;
	map<EVMASK_T, int>		m_evToIndexMap;
	int						m_err;
};

#endif //#ifndef _CADATAWRITER_H_
