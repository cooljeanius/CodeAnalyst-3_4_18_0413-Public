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


#ifndef _MYPROFILER_H_
#define _MYPROFILER_H_

#include <string>
#include <map>
#include <jvmpi.h>
#include <jni.h>
#include <linux/types.h>

#define UNKNOWNSOURCEFILE  "UnknownJavaSource"
#define UNKNOWNCLASSNAME	"UnknownClassName"
#define UNKNOWNFUNCTION		"UnknownFuncName"



using namespace std;

struct  _MethodMapElement
{
	string javaSrcFileName;
    string className;
    string functionName;
    bool jit;
};

typedef struct _MethodMapElement MethodMapElement;
typedef struct map<jmethodID, MethodMapElement> MethodMap;

typedef struct _PerfMonRecord {
      uint  timeStampLo;
      uint  timeStampHi;
      uint  counter0Lo;
      uint  counter0Hi;
      uint  timeStampDeltaLo;
      uint  timeStampDeltaHi;
      uint  counter0DeltaLo;
      uint  counter0DeltaHi;
} PerfMonRecord;


void printJitMethods();
void getCPUTimeStamp(__u64 & timeStamp);

#endif
