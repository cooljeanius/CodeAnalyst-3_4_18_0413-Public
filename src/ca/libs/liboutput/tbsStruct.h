//$Id: tbsStruct.h 20128 2011-08-11 12:09:44Z sjeganat $
// Structure definitions

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2006 Advanced Micro Devices, Inc.
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


#ifndef _TBSSTRUCT_H_
#define _TBSSTRUCT_H_

#include "../../include/stdafx.h"
#include <qstring.h>
#include <q3valuelist.h>

///////////////////////////////////////////////////////////////////////////////
typedef struct __EnvValues {
	unsigned int m_num_cpus;
	unsigned long long m_num_events;
	unsigned int m_num_samples;
	unsigned int m_num_modules;
	unsigned int m_tbp_version;
	unsigned long m_cpufamily; 
	unsigned long m_cpumodel; 
	QString m_timestamp;

	Q3ValueList<unsigned long long> m_event_list;

	__EnvValues()
	{
		m_num_cpus	= 1;
		m_num_events	= 1;
		m_num_samples	= 0;
		m_num_modules	= 0;
		m_tbp_version	= 0;
		m_cpufamily	= 0; 
		m_cpumodel	= 0; 
		m_timestamp	= QString("");
	};
} EnvValues;


///////////////////////////////////////////////////////////////////////////////
struct ProfileAttribute {
    int cpuNum;
    int eventNum;
    unsigned int events[MAX_CPUS];
    unsigned int totalSamples;	// total samples in session
    // module load address. Opreport returns sample offset address for 
    // module stripped symbols. But it returns sample address for module 
    // with symbols.
    VADDR modPatchLoadAddr;
    QString modName;
    QString modPath;
    QString sessionPath;
    unsigned int modType;
    SampleDataMap modTotal;

    ProfileAttribute(){
		cpuNum = 0;
		eventNum = 0;
		totalSamples = 0;
		modType = 0;
    };
};




#endif //ifndef _TBSSTRUCT_H_
