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

#include <string>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <linux/types.h>

#ifndef _JCLWRITER_H_
#define _JCLWRITER_H_

using namespace std;
const unsigned int JCL_VERSION_0 = 0x00;
const unsigned int JCL_VERSION = 0x01;

const char ZERO_PAD = 0x0a;

//The structure of the JCL file is:
//	JCLHeader, contains the # of records in the file
//
//	__int32 record-type
//	record-type record
//	...


// JCLHeader
struct JCLHeader {
	char signature[8];	// this always be AMDCAJCL
	__u32 version;	// UINT32 version:[15-0] Major, version:[31-16] minor
	pid_t processID;
	__u32 numRecords; // UINT32
	string javaAppName;
	__u32	b32_bit;	// if its 32-bit 
};


// JIT Records
enum JIT_RECORDS {
	JIT_LOAD = 0,
	JIT_UNLOAD = 1
};


struct JIT_Load_ELEMENT {
	__u64 loadTimestamp; 
    __u64 blockStartAddr; 
	__u64 blockEndAddr;
	__u64 threadID; 
    string  classFunctionName;
    string jncFileName;
    string javaSrcFileName;
};


struct JIT_Unload_ELEMENT {
	__u64 unloadTimestamp;
    __u64 blockStartAddr;
};


using namespace std;

class JCLWriter 
{
    public:
        JCLWriter(string jclName, string javaAppName, pid_t pid);
        ~JCLWriter();
        bool WriteLoadRecord(JIT_Load_ELEMENT * element);
		bool WriteUnloadRecord (JIT_Unload_ELEMENT * element);

    protected:
        void WriteHeader();
    protected:
        ostream * m_JCLFileStream;
        filebuf m_JCLFileFB;
        pid_t m_PID;
		int m_recordCount;
        string m_JavaApp;
};

#endif
