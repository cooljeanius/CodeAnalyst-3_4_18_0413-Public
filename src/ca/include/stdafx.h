//$Id: stdafx.h,v 1.7 2006/05/15 22:09:22 jyeh Exp $
// include file for standard system include files, or project specific 
// include files that are used frequently, but are changed infrequently
//

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2007 Advanced Micro Devices, Inc.
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


#if !defined(_STDAFX_H_INCLUDED_)
#define _STDAFX_H_INCLUDED_

//All qt headers
#include <qapplication.h>
#include <qprocess.h>
#include <qmainwindow.h>
#include <qcheckbox.h>
#include <q3groupbox.h>
#include <qworkspace.h>
#include <q3toolbar.h>
#include <qmessagebox.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qfile.h>
#include <q3textstream.h>
#include <qtimer.h>
#include <qthread.h>
#include <q3progressbar.h>
#include <qlabel.h>
#include <q3listview.h>
#include <qpixmap.h>
#include <qtoolbutton.h>
#include <q3popupmenu.h>
#include <qmenubar.h>
#include <q3filedialog.h>
#include <qstatusbar.h>
#include <qpushbutton.h>
#include <q3accel.h>
#include <qpainter.h>
#include <q3paintdevicemetrics.h>
#include <q3whatsthis.h>
#include <q3vbox.h>
#include <qfontmetrics.h>
#include <q3sortedlist.h>
#include <qtooltip.h>
#include <qsizepolicy.h>
#include <qsplitter.h>
#include <qinputdialog.h>
#include <qdialog.h>
#include <qvalidator.h>
#include <qspinbox.h>
#include <qmap.h>
#include <q3progressdialog.h>
#include <qtabwidget.h>
#include <qtabbar.h>
#include <q3widgetstack.h>
#include <qstyle.h>
#include <q3header.h>
#include <q3valuevector.h>
#include <Q3CString>
#include <Q3ValueList>
#include <QEvent>

#include <map>

#include "libCAdata.h"
#include "typedefs.h"
#include "config.h"

using namespace std;

enum MODULE_TYPE_ENUMS {
    INVALIDMODTYPE = 0,
    UNMANAGEDPE,
    JAVAMODULE,
    MANAGEDPE,
    UNKNOWNMODULE
};

enum ProjectWideEnums {
	MAX_EVENTNUM_MULTIPLEXING = 32,
	MAX_EVENTGROUP_MULTIPLEXING = 8,
	MAX_EVENTNUM 		= 4,
	MAX_CPUS 		= 8,
	TBP_EVENT_MASK 		= 0xff,
	GREYHOUND_FAMILY 	= 0x10,
	GRIFFIN_FAMILY 		= 0x11,
	FAMILY12H_FAMILY	= 0x12,
	FAMILY14H_FAMILY	= 0x14,
	FAMILY15H_FAMILY	= 0x15,
	OPTERON_FAMILY 		= 15,
	ATHLON_FAMILY 		= 6,
	OPTERON_MODEL 		= 5,
	HEX_BASE 		= 16,
	REV_C_MODEL		= 0x04,
	DEFAULT_MUX_INTERVAL_MS	= 1

};

//
// TRIGGER : ways in which to sample
// 
enum TRIGGER {
    TIMER_TRIGGER,
    EVENT_TRIGGER,
    PERF_TIMER_TRIGGER,
    NO_TRIGGER
};

typedef QMap<EVMASK_T, float> EventNormValueMap;

/*
* This map contain sting of format 
* "event:<opName> count:<number> unit-mask:<mask>" as key
*/
typedef map <string , EventEncodeType> EventMaskEncodeMap;


// key is [cpu, event], data is sample count 
typedef map<SampleKey, unsigned int> SampleDataMap;
typedef map<SampleKey, float> SamplePercentMap;


typedef Q3ValueVector <float> DataArray;	// LEI: remove this later.

// structs
struct SYM_DATA {
	Q3CString SymFile;
	VADDR BaseAddr;
};

struct MODULE_INFO {
	Q3CString Name;
	VADDR BaseAddress;
	UINT32 Size;
	UINT32 Samples;
};

typedef QMap < QString /* Module Name */ , SYM_DATA > SymbolMap;


#define TBS_FILE_COMMENT    "#"
#define TBS_LINE_DELIMITER  ","
#define TBS_FILE_VAR        "!"

#define STD_FONT	QFont( "Arial", 8 )
#define NEW_SESSION_NAME    "Session"
#define PATH_SLASH	"/"

#define XPC_PATH_SLASH		'//'
#define XPS_PATH_SLASH		"/"

enum {
	EVENT_DELAY_FINISH = QEvent::User + 1,
	EVENT_DURATION_FINISH,
	EVENT_TRACE_BEGIN,
	EVENT_RESET_PROGRESS,
	EVENT_SET_PROGRESS,
	EVENT_SET_STEPS,
	EVENT_LAUNCH_ERROR,
	EVENT_UPDATE_PROGRESS,
	EVENT_UPDATE_PROGRESS_BEGIN,
	EVENT_UPDATE_PROGRESS_DONE,
	EVENT_UPDATE_ERROR,
	EVENT_UPDATE_OPROFILED_MONITOR,
	EVENT_UPDATE_OPROFILEDRV_MONITOR
};

enum {
	NS_TO_MS = 1000000,
	FIVE_HUNDRED_MS = 500 * NS_TO_MS
};

enum JavaSourceColumnType
{
	JAVA_SOURCE_FILE = 0,
	JAVA_PATH
};

enum {
	LONG_STR_MAX = 20
};
#ifdef _x86_64_
#define LONG_FORMAT "0x%016lx"
#else
#define LONG_FORMAT "0x%08lx"
#endif

const QString TI_FILE_EXT = ".ti";
const QString PRD_FILE_EXT = ".prd";
const QString DIR_FILE_EXT = ".dir";
const QString TBS_FILE_EXT = ".tbp";
const QString EBS_FILE_EXT = ".ebp";
const QString NOMEM_MSG = "There is insufficient memory available.";
const QString NOMEM_HEADER = "CodeAnalyst memory error.";

#define RETURN_FALSE_IF_NULL(pointer,parent) if (NULL == pointer) { \
	QMessageBox::critical (parent, NOMEM_HEADER, NOMEM_MSG); \
	return false;}
#define RETURN_IF_NULL(pointer,parent) if (NULL == pointer) { \
	QMessageBox::critical (parent, NOMEM_HEADER, NOMEM_MSG); \
	return;}
#define DELETE_IF_NOT_NULL(pointer)    if (NULL != pointer) { \
	delete pointer; \
	pointer = NULL;} \

struct HitCounts {
	HitCounts () {
		Clear ();
	};

	void Clear () {
		for (int i = 0; i < MAX_CPUS; i++)
			for (int j = 0; j < MAX_EVENTNUM; j++)
				hitcounts[i][j] = 0;
	};
	UINT64 hitcounts[MAX_CPUS][MAX_EVENTNUM];
};

enum TAB_TYPE
{
	TAB_DATA,
	TAB_GRAPH,
	TAB_TASK
};

enum TASK_HARDCODED_TYPE {
	SHOW_ALL_TASKS = 0
};
///////////////////////////////////////////////////////////////////////////////

struct MOD_LV_ITEM {
	// TODO: [Suravee] Clean up non used java stuff
	void clear() {
		CsEip = 0;
		funcStartAddr = 0;
		Samples = 0;
		taskId = 0;
		threadId = 0;
		FuncName = "";
		JncFilePath = "";
		JavaSrcFile = "";
		CpuSamples.clear();
	};
	unsigned long long CsEip;
	unsigned long long funcStartAddr;
	unsigned long Samples;
	unsigned int taskId;
	unsigned int threadId;
	Q3CString FuncName;
	Q3CString JncFilePath;
	Q3CString JavaSrcFile;
	SampleDataMap CpuSamples;
};


struct SYS_LV_ITEM {
	SYS_LV_ITEM() {
		taskId = 0;
		CpuSamples.clear();
	};

	unsigned int taskId;
	QString ModName;
	unsigned long long TotalSamples;
	QString ThreeTwoBit;
	QString SixFourBit;
	Q3CString PercSamples;
	SampleDataMap CpuSamples;
};


/* NOTE:
 * This has been moved into sourceview.h.
 */
//struct SRC_LV_ITEM {
//
//	enum {
//		SRCLINE
//	};
//
//	QCString Location;
//	QCString Samples;
//	QCString Codebytes;
//	QCString SourceLine;
//	VADDR address;
//	SampleDataMap CpuSamples;
//	unsigned int lineNumber;
//	unsigned int samples;
//
//	SRC_LV_ITEM() {
//		address = 0;
//		lineNumber = 0;
//		samples = 0;
//	};
//};

struct DASM_LV_ITEM {
	QString CsEip;
	Q3CString Symbol;
	QString Samples;
	Q3CString CodeBytes;
	Q3CString Instruction;
	SampleDataMap CpuSamples;
	Q3CString JncFilePath; 

	DASM_LV_ITEM() {
		Symbol = "";
		CpuSamples.clear();
		Samples = "";
	}
};

typedef map < VADDR, SampleDataMap> SampleMap;

class JIT_BLOCK_INFO {
public:
	VADDR jit_load_addr;

	// since we did not write JIT func size into .ebp,
	// this possible end is the largest sample RIP in the function.
	VADDR jit_possible_end;

	QString jit_fun_name;
	QString jnc_file_path;
	QString javaSrcFile;
	SampleMap javaSampleMap;
	
	JIT_BLOCK_INFO() {
		jit_load_addr = 0;
		jit_possible_end = 0;
	};
};

struct ProfModEventInfo {
	unsigned int	cpuNum;
	unsigned int	eventNum;
	unsigned char eventIndex[MAX_EVENTNUM];
	QString modName;
	QString modPathName;
	VADDR modLoadAddr;
	unsigned int modType;
	unsigned int SessionTotalSamples;
};


// key is task id,  data is sample map for the task.
// 
// Note: the task id is generated by CA duration data translation.
// CA make sure 0 is unique to represent "ALL Tasks".
//
typedef map <unsigned int , SampleMap> TaskSampleMap;

typedef JIT_BLOCK_INFO *		JIT_BLOCK_INFO_PTR;
typedef vector<JIT_BLOCK_INFO_PTR>	JitBlockPtrVec;


#define UNKNOWNSOURCEFILE	"UnknownJavaSource"
#define UNKNOWNCLASSNAME	"UnknownClassName"
#define UNKNOWNFUNCTION		"UnknownFuncName"

#define MAXBUFFERSIZE      4096
#define HALFPAGE           2048
#define QUARTERPAGE        1024

#define CA_PROG_DLG_FLAGS 	Qt::CustomizeWindowHint \
				| Qt::WindowTitleHint \
				| Qt::WindowStaysOnTopHint

#define CA_DLG_FLAGS 		Qt::WindowStaysOnTopHint

//same as optionsdlg.cpp
const char K7_EVENTS_FILE[] = "/k7-events.xml";
const char K8_EVENTS_FILE[] = "/k8-events.xml";
const char GH_EVENTS_FILE[] = "/gh-events.xml";
const char GR_EVENTS_FILE[] = "/gr-events.xml";
const char FAMILY12H_EVENTS_FILE[] = "/family12h-events.xml";
const char FAMILY14H_EVENTS_FILE[] = "/family14h-events.xml";
const char FAMILY15H_EVENTS_FILE[] = "/family15h-events.xml";
const char FAMILY15H_1XH_EVENTS_FILE[] = "/family15h_1xh-events.xml";


//This vector contains the order of indexes to the available data for what's 
//	shown, see below for more detail
typedef Q3ValueVector<int> IndexVector;
//given a cpu/event key, get the column index
struct CpuEventType {
	int cpu;
	unsigned int event;
	unsigned char umask;	
	CpuEventType () 
	{cpu = 0; event = 0; umask = 0;};

	CpuEventType (int a, unsigned int b, unsigned char c = 0) 
	{cpu = a; event = b; umask = c;};

	void setKey ( SampleKey sample) {
		cpu = sample.cpu;

		// Decoding 
		event = sample.event & 0x0000ffff;
		umask = sample.event >> 16;
	};
};
inline BOOL operator< (const CpuEventType &temp1, const CpuEventType &temp2) 
{
	if ((temp1.event < temp2.event))
		return true;
	else if ((temp1.event == temp2.event) 
		&& (temp1.umask < temp2.umask))
		return true;
	else if ((temp1.event == temp2.event) 
		&& (temp1.umask == temp2.umask)
		&& (temp1.cpu < temp2.cpu))
		return true;
	else
		return false;
};

typedef QMap <CpuEventType, int> CpuEventViewIndexMap;
struct ComplexComponents
{
	int complexType;
	int columnIndex;
	int op1Index;
	float op1NormValue;
	int op2Index;
	float op2NormValue;
};

typedef Q3ValueList<ComplexComponents> ListOfComplex;
//this map will allow changes to either of the dependent values to recalculate
//	the complex column during aggregation
//every complex column (currently) depends on two operand columns - the complex
//	column data will be added to the list at each index, since one column 
//	change may affect multiple complex columns
typedef QMap <int, ListOfComplex> ComplexDependentMap;
class ViewShownData
{
public:
	IndexVector shown;
	QStringList available;
	QStringList tips;
	CpuEventViewIndexMap indexes;
	ComplexDependentMap calculated;
	bool separateCpus;
	bool separateTasks;
	bool separateThreads;
	bool showPercentage;
	static float setComplex (ComplexComponents *pComplex, DataArray *pData);
	
	ViewShownData()
	{
		separateCpus		= false;
		separateTasks		= false;
		separateThreads		= false;
		showPercentage		= false;
	};

	void clear()
	{
		shown.clear();
		available.clear();
		tips.clear();
		indexes.clear();
		calculated.clear();
		separateCpus		= false;
		separateTasks		= false;
		separateThreads		= false;
		showPercentage		= false;
			
	};

	~ViewShownData()
	{
		clear();
	};
};

//////////////////////////////////////////////////////////////
typedef struct _op_event_property {
	unsigned long eventmask;
	unsigned long long count;
	unsigned long unitmask;
	bool os;
	bool usr;
	QString op_name;
	
	_op_event_property() {
		eventmask	= 0;
		count 		= 0;
		unitmask 	= 0;
		os 		= true;
		usr 		= true;
	};

	_op_event_property & operator = (_op_event_property & rhs) {
		if (&rhs != NULL && &rhs != this) {
			eventmask	= rhs.eventmask;
			count 		= rhs.count;
			unitmask 	= rhs.unitmask;
			os 		= rhs.os;
			usr 		= rhs.usr;
			op_name 	= rhs.op_name;
		}
		return rhs;
	};
} op_event_property;

typedef map <string, op_event_property> OP_EVENT_PROPERTY_MAP;

typedef struct _op_ibs_property { 
	unsigned long fetch_count;
	unsigned long fetch_um;
	unsigned long op_count;
	unsigned long op_um;
	unsigned int dispatched_ops;
	bool dcMissInfoEnabled;
	QStringList fetch_strList;
	QStringList op_strList;
	
	_op_ibs_property() {
		fetch_count	= 0;
		fetch_um	= 0;
		op_count	= 0;
		op_um		= 0;
		dispatched_ops	= 1;
		dcMissInfoEnabled = false;
	};
	
	_op_ibs_property & operator = (_op_ibs_property & rhs) {
		if (&rhs != NULL && &rhs != this) {
			fetch_count	= rhs.fetch_count;
			fetch_um	= rhs.fetch_um;
			op_count	= rhs.op_count;
			op_um		= rhs.op_um;
			dispatched_ops	= rhs.dispatched_ops;
			fetch_strList  	= rhs.fetch_strList;
			op_strList     	= rhs.op_strList;
			dcMissInfoEnabled = rhs.dcMissInfoEnabled;
		}
		return rhs;
	};
	
} op_ibs_property;

typedef map <string, op_ibs_property> OP_IBS_PROPERTY_MAP;

typedef DWORD	AffinityMask_t;

#endif // ! defined(_STDAFX_H_INCLUDED_)
