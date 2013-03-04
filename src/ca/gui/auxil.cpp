//$Id: auxil.cpp,v 1.2 2005/02/14 16:52:29 jyeh Exp $
// interface for auxillary functions

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2005 Advanced Micro Devices, Inc.
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

#include <math.h>

#include "stdafx.h"
#include "auxil.h"
#include "helperAPI.h"


struct SessionGlobalData{
	int refCnt; // reference cnt;
	TaskSampleMap taskSampMap;
	JitBlockPtrVec jitBlkPtrVec;
	//JitBlockMap jitBlkMap;
};  

struct SessionModKey {
	QString sessionName;
	QString moduleName;
	
	bool operator < (const SessionModKey & other) const
	{
		if (sessionName < other.sessionName)
			return true;
		else if (sessionName > other.sessionName)
			return false;
		else if (moduleName < other.moduleName)
			return true;
		else
			return false;
	};
};  


// key is module name;
typedef map<SessionModKey, SessionGlobalData> SessionGlobalMap;

SessionGlobalMap gSessionDataMap;


void ClearTaskSampleMap(TaskSampleMap &tMap)
{
	TaskSampleMap::iterator tIt = tMap.begin();
	TaskSampleMap::iterator tEnd = tMap.end();
	for (; tIt != tEnd; tIt++) {
		tIt->second.clear();
	}
}


void ClearAllSessionGlobalMap()
{
	gSessionDataMap.clear();
}


void ClearSessionGlobalMap(QString sessionName)
{
	SessionModKey tKey;
	tKey.sessionName = sessionName;
	tKey.moduleName = "";

	SessionGlobalMap::iterator sIt = gSessionDataMap.lower_bound(tKey);
	SessionGlobalMap::iterator sPreIt;
	SessionGlobalMap::iterator sEnd = gSessionDataMap.end();
	
	while (sIt != sEnd) {

		if (sIt->first.sessionName != sessionName)
			break;

		sPreIt = sIt;
		sIt++;
		
		ClearTaskSampleMap(sPreIt->second.taskSampMap);
		//sPreIt->second.jitBlkMap.clear();
		sPreIt->second.jitBlkPtrVec.clear();

		gSessionDataMap.erase(sPreIt);

	} 
}
	
void AddModGlobalMap(QString sessionName, QString modName, 
				TaskSampleMap **pTaskMap, 
				JitBlockPtrVec **pJitBlkPtrVec)
{
	
	SessionModKey tKey;
	tKey.sessionName = sessionName;
	tKey.moduleName = modName;

	SessionGlobalMap::iterator sIt = gSessionDataMap.find(tKey);
	SessionGlobalMap::iterator sEnd = gSessionDataMap.end();

	if (sIt != sEnd) {
		sIt->second.refCnt++;
		if (pTaskMap) {
			*pTaskMap = &(sIt->second.taskSampMap);
		}
		if (pJitBlkPtrVec) {
			*pJitBlkPtrVec = &(sIt->second.jitBlkPtrVec);
		}
	} else {
		SessionGlobalData tData;
		tData.refCnt = 1;
		gSessionDataMap.insert(SessionGlobalMap::value_type(tKey, tData));
		sIt = gSessionDataMap.find(tKey);
		if (pTaskMap) {
			*pTaskMap = &(sIt->second.taskSampMap);
		}
		if (pJitBlkPtrVec) {
			*pJitBlkPtrVec = &(sIt->second.jitBlkPtrVec);
		}
	}
}
/*
void RemoveModGlobalMap(QString sessionName, QString modName)
{

	SessionModKey tKey;
	tKey.sessionName = sessionName;
	tKey.moduleName = modName;

	SessionGlobalMap::iterator sIt = gSessionDataMap.find(tKey);
	SessionGlobalMap::iterator sEnd = gSessionDataMap.end();

	if (sIt != sEnd) {
		sIt->second.refCnt--;
		if( sIt->second.refCnt == 0) {
			ClearTaskSampleMap(sIt->second.taskSampMap);
			sIt->second.jitBlkMap.clear();

			gSessionDataMap.erase(sIt);
		}
	} 
}
*/



