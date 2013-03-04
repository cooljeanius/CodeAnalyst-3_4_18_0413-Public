//$Id: opsampledata_handler.h $
// Interface for processing profile data using Oprofile interfaces.

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2009 Advanced Micro Devices, Inc.
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

#ifndef _OPSAMPLEDATA_HANDLER_H_
#define _OPSAMPLEDATA_HANDLER_H_

#include "opdata_handler.h"

using namespace std;

class opsampledata_handler : public opdata_handler
{
public:
	opsampledata_handler();

	virtual ~opsampledata_handler();

	void init(const char * op_data_dir, EventMaskEncodeMap evt_map);
	
	bool read_op_module_data(QStringList task_filter );

protected:
	void dumpresult();

	void dumpDataMap(SampleDataMap &data);

private:
	// check if modname is /var/lib/oprofile/jit/
	// if true, convert the java argument into converted. 
	// otherwise, return false, fill the modname into converted.
	bool isJavaApp(string modname, string &converted, int & javaId);
	
	bool getInfoFromOpSampleFileName(parsed_filename parsed, 
				UINT32 * cpu, 
				EVMASK_T * event);

	bool processOneOpDataFile(parsed_filename parsed, 
				op_bfd *p_bfd, 
				int pid, 
				int tid, 
				unsigned long long jitBaseAddr, 
				string & jitSym,
				unsigned int filesize,
				CA_Module * pMod,
				CA_Process * pProc);

	bool getJitInfo(op_bfd *p_bfd, string jitfile, 
				string *p_str, 
				unsigned long long *p_addr,
				unsigned int *p_size);

	void enumeratefiles(QString dir, list<string> & flist);

	bool isJoFile(parsed_filename parsed);

	string 			m_op_data_dir;
	java_app_name_map 	m_java_name_map;
};

#endif //_OPSAMPLEDATA_HANDLER_H_
