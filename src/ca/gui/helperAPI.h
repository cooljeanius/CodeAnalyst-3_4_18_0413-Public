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

#ifndef _HELPERAPI_H_
#define _HELPERAPI_H_

#include <qdir.h>
#include <string.h>

using namespace std;

extern bool folderRemove (const QDir & _d);

extern void untar_packed_file(QString * packed_path, QString * temp_dir );

extern unsigned int rename_files(QString * temp_dir, 
			QString * op_sample_path,
			QString * cpuinfo_path);

extern int getBitness(const char * path);

extern int ca_exec_command(QStringList & args, 
			QString & stdOut,
			QString & stdErr,
			int & status,
			bool isBlock = true);

extern int ca_exec_sudo_command(QStringList & args, 
			QString & stdOut,
			QString & stdErr,
			int & status,
			bool isBlock = true);

extern bool cssConversion(QString cgInput, QString cgOutput);

extern unsigned long getModelFromCpuInfo(const char * file);

extern unsigned long getFamilyFromCpuInfo(const char * file);

extern unsigned int  XpGetNumCpus();

extern unsigned int  XpGetNumCoresPerSocket();

extern void XpGetAppPath(QString & path);

//returns the count of lines that contain str in file
extern int XpFileContains (const char * file, const char * str);

extern void CpuId ( char *vendor_id, 
		unsigned long *family, 
		unsigned long *model, 
		unsigned long *stepping);

extern bool hasLocalApic();

extern bool isCpuAMD ();

extern bool isCpuIbsOk();

extern bool isDriverIbsOk();

extern bool isDriverMuxOk();

extern bool isDriverIbsOpDispatchOpOk();

extern bool isCpuIbsOpDispatchOpOk(unsigned long ibsFeatureFlag);

extern bool isCpuIbsBrTgtAddrOk(unsigned long ibsFeatureFlag);

extern bool isDriverIbsOpBranchTargetAddrOk();

extern bool isCpuIbsCountExtOk(unsigned long ibsFeatureFlag);

extern void getCpuIdIbs( unsigned long *ibsFeatureFlag);

extern QString getCurrentCpuType();

extern QString helpGetEventFile(QString cpuType);

extern QString helpGetEventFile(unsigned long cpuFamily, unsigned long cpuModel = 0);

extern QString helpGetOpXmlEventFile(QString cpuType);

extern unsigned int getTickSysFS();

extern unsigned int getTickCPUInfo();

extern unsigned int getTick ();

extern QString helpGetFamilyDir(unsigned long cpuFamily, unsigned long cpuModel = 0);

extern bool isSupportedAmdCpuType(QString cpuType);

extern QString getDefaultBrowser();

extern QString getDefaultPdfReader();

extern unsigned int getNumCntrForCpu(unsigned long cpuFamily);

extern unsigned int getCounterAvailMask();

extern QString wstrToQStr(wstring wstr);

extern QString helpFindXterm();
#endif //_HELPERAPI_H_

