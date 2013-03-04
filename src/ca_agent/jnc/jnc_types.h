#ifndef _JNC_TYPES_H_
#define _JNC_TYPES_H_

#include <map>
#include <string.h>

using namespace std;

///////////////////////////////////////////////
typedef struct _JNCPCStackInfo {
	void* pc;             /* the pc address for this compiled method */
	jint numstackframes;  /* number of methods on the stack */
	jmethodID* methods;   /* array of numstackframes method ids */
	jint* bcis;           /* array of numstackframes bytecode indices */
} JNCPCStackInfo;

typedef map<unsigned long long, JNCPCStackInfo *> JNCPCStackInfoMap;

///////////////////////////////////////////////
typedef struct _JNCMethodLoadLineInfo {
	jint numinfos;          /* number of pc descriptors in this nmethod */
	JNCPCStackInfo* pcinfo; /* array of numpcs pc descriptors */
} JNCMethodLoadLineInfo;

///////////////////////////////////////////////
typedef struct _JNCjvmtiLineNumberEntry {
	jlocation start_location;
	jint line_number;
	bool operator<(const struct _JNCjvmtiLineNumberEntry rhs) const
	{
		return start_location < rhs.start_location;
	};
} JNCjvmtiLineNumberEntry;

typedef vector<JNCjvmtiLineNumberEntry> JNCjvmtiLineNumberEntryVec;  


///////////////////////////////////////////////
typedef struct _JNCMethod
{
	jmethodID id;
	string name;
	string signature;
	string sourceName;
	JNCjvmtiLineNumberEntryVec lineNumberVec;
} JNCMethod;

typedef map<jmethodID,JNCMethod> JNCMethodMap;


///////////////////////////////////////////////
typedef struct _JNCAddressRange {
	jmethodID id;
	void* pc_start;
	void* pc_end;
	bool operator<(const struct _JNCAddressRange rhs) const
	{
		return pc_start < rhs.pc_start;
	};
} JNCAddressRange;

typedef vector<JNCAddressRange> JNCAddressRangeVec;

#endif //_JNC_TYPES_H_
