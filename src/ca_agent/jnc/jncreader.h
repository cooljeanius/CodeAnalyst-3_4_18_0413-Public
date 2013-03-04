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


#ifndef _JNCREADER_H_
#define _JNCREADER_H_

#include <bfd.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <list>

#include "jnc_types.h"

using namespace std;

typedef struct {
	unsigned long long	startAddr;
	unsigned long long	stopAddr;
} addrRanges;

typedef std::list<addrRanges> AddressRangeList;

typedef struct _LineNumSrc {
	int			lineNum;
	string			sourceFile;
	string			symName;
	unsigned long		methodId;
	AddressRangeList	addrs;

	bool operator== (const struct _LineNumSrc &other)
	{
		return( (methodId == other.methodId) &&
			(sourceFile == other.sourceFile) &&
//			(lineNum == other.lineNum) &&
			(symName == other.symName));
	}
} LineNumSrc;

// Key is starting PC
typedef std::multimap<unsigned long long, LineNumSrc> JNCInlineMap;

// Indexed by "parent" line number
// Has map of inlined function info for each "parent" line
typedef std::multimap<int, JNCInlineMap> JavaInlineMap;

class CJNCReader
{
public:
	CJNCReader();
	
	virtual ~CJNCReader();

	bool open(const char * pFileName);

	void close();
	
	void clear();

	bool getStringFromOffset(
		unsigned int offset, 
		string & str);

	void dumpStringTable(FILE * f);

	void dumpJncMethodMap(FILE * f);

	void dumpAddressRangeTable(FILE * f);

	void dumpJncPcStackInfoMap(FILE * f);

	bool getSrcInfoFromAddressWithFile(
		unsigned long long addr,
		int & srcLine,
		string & srcFile);

	bool getSrcInfoFromAddress(
		unsigned long long addr,
		int & srcLine,
		string & srcFile,
		int & inlineDepth);

	bool getSymbolAndRangeFromAddr(
		bfd_vma addr,
		string & symName,
		bfd_vma & startAddr,
		bfd_vma & endAddr);

	string getJittedFunctionName()
	{ return m_methodName; };

	bool getJittedStartEndAddr(
		bfd_vma & startAddr, 
		bfd_vma & endAddr)
	{
		if (m_loadAddr == 0)
			return false;

		startAddr = m_loadAddr;
		endAddr = m_loadAddr + m_textSize;
		return true;
	};

	bool getJittedFunctionSrcInfo(
		string & fileName,
		unsigned int & startLine,
		unsigned int & possibleEndLine)
	{
		if (m_loadAddr == 0)
			return false;
		fileName = m_srcFile;
		startLine = m_startLine;
		possibleEndLine = m_possibleEndLine;
		return true;
	};

	JavaInlineMap *getInlineMap(void)
	{
		return &m_javaInlineMap;
	}

	const char * getMethodName(unsigned long methodId);

	string getBfdErrorMsg() {return m_bfdErrorMsg;};
private:
	bool _process_stringtable_section(asection * p);
	bool _process_bc2src_section(asection * p);
	bool _process_pc2bc_section(asection * p);
	bool _process_inline_map(void);
	void _freePcStackInfo(void);

	void _dumpJavaInlineMap(void);

	jmethodID _getMethodFromAddr(unsigned long long addr);

	int _getLineFromAddrVec(
		unsigned long long addr, 
		JNCjvmtiLineNumberEntryVec &vec);

	void _fillLineNumSrcMap(
		JNCInlineMap &ilmap,
		int numFrames,
		void *vbcisArray,
		void *vmethodArray,
		unsigned long long thisPc,
		unsigned long long nextPc);

	bfd * m_abfd;	
	bfd_error_type m_bfdErrorType;
	string m_bfdErrorMsg;
	unsigned int m_sectionCounts;
	unsigned int m_string_table_size;
	char * m_string_table_buf;

	bool _getSrcInfoFromBcAndMethodID(
		jint bc,
		jmethodID id,
		int & srcLine,
		string & srcFile);

	// String table stuff
	JNCMethodMap m_jncMethodMap;
	jmethodID m_mainMethodId;

	// Method table stuff
	JNCAddressRangeVec m_addressRangeTable;

	// Inline stuff
	void * m_pcinfo;
	JNCPCStackInfoMap m_jncPcStackInfoMap;
	JavaInlineMap m_javaInlineMap;

	bfd_vma m_loadAddr;
	string m_methodName;
	string m_srcFile;
	bool m_mainInlineDepth;
	unsigned int m_startLine;
	unsigned int m_possibleEndLine;
	unsigned int m_textOffset;
	unsigned int m_textSize;
};	

#endif // #ifndef_JNCREADER_H_
