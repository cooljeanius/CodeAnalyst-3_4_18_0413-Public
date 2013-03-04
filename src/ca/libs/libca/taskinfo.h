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


#ifndef _TASKINFO_H_
#define _TASKINFO_H_

#include <bfd.h>
#include <vector>
#include <string>
#include <qmap.h>
#include <q3ptrlist.h>
#include <q3cstring.h>
#include <qstring.h>
#include <limits.h>

#include "timapper.h"
#include "tireader.h"
#include "jclreader.h"

using namespace std;

typedef struct {
	/* [in] */	u64 processID;
	/* [in] */	unsigned CSvalue; 
	/* [in] */	u64 sampleAddr; 
	/* [in] */	unsigned cpuIndex;  
	/* [in] */	u64 timestamp; 
	/* [out] */	u64 ModuleStartAddr;
	/* [out] */	u64 Modulesize;
	/* [out] */ bool JitModule;
	/* [in] */  unsigned funSize;
	/* [out] */ char *pFunctionName;
	/* [out] */ u64 FunStartAddr;
	/* [in] */  unsigned jncSize;
	/* [out] */ char *pJncName;
    /* [out] */ char * pJavaSrcName;
	/* [in] */	unsigned namesize;
	/* [out] */	char* pModulename; 
}TI_MODULE_INFO ;


struct  _JNC_VALUE{
	u64 loadTimestamp; // ULONG64
	u64 unloadTimestamp;
    u64 blockStartAddr; // unsigned long
	u64 blockEndAddr;
	u64 threadID; // ULONG32
    QString javaSrcFile;
    QString functionName;
    QString jncFile;
	QString translatedJncFile;

	_JNC_VALUE() : translatedJncFile(""), jncFile("") {
		unloadTimestamp = ULONG_LONG_MAX;
		loadTimestamp = 0;
		blockStartAddr = 0;
		blockEndAddr = 0;
		threadID = 0;
	};
};

typedef _JNC_VALUE JNC_VALUE;

typedef Q3PtrList <JNC_VALUE> JNC_LIST;
typedef QMap <unsigned long, JNC_LIST *> JNCLIST_MAP;

class CTaskInfo {
public:
    // constructor
    CTaskInfo();
    // destructor
    ~CTaskInfo();

    bool ReadTaskInfoFile(string filename); 

    void PrintTIVector();

    bool GetModuleInfo(TI_MODULE_INFO * mod_info, unsigned int novmlinux);
    
    bool copyJNCFile(QString & sessionDir, QString & jncPath);
//private:
    void ProcessTaskInfoVector();
    
    void OnProcessStart(task_info_record & record);

    void OnProcessEnd(task_info_record & record);

    void OnModuleMap(task_info_record & record);

    bool ReadJitInformation(const char * directory);

    void PrintJITMap();

    void ProcessJCLFiles();

    void AdjustModuleUnloadTime();

    void AddkernelModule(bfd_vma start_addr, bfd_vma size, QString & name);

private:
    string m_TIFileName;
    cTaskInfoReader * m_TIReader;
    TIVector m_TIVector;
    ModuleMap m_tiModMap;

	//java Jit data map
	JNCLIST_MAP m_tiJncMap;

    unsigned int m_jnc_counter;

    task_info_header m_TIHeader;
};




#endif
