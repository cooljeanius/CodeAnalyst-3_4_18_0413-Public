// CodeAnalyst for Open Source
// Copyright 2002 . 2011 Advanced Micro Devices, Inc.
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

//  Interface for the CADataReader class.

#ifndef _CAPROFILEREADER_H_
#define _CAPROFILEREADER_H_

#include "ImdReader.h"
#include "libCAdata_Process.h"

#ifndef UNKNOWNSOURCEFILE
#define UNKNOWNSOURCEFILE	"UnknownJITSource"
#endif

/**********************************
 * class CaProfileReader
 *
 * Description:
 * This class implements TBP/EBP reader which is the
 * main interface for accessing profile data
 */
class LIBCADATA_API CaProfileReader : public CaDataReader
{
public:

	CaProfileReader();

	~CaProfileReader();
		

	/* Open profile file and read profile info.
	 * On success, the CaProfileInfo should be 
	 * populated.
	 */
	virtual bool open(wstring path);
	
	bool open(string path);

	virtual void close ();

	/* Return the pointer to CaProfileInfo on success. */
	CaProfileInfo * getProfileInfo();

	/* Return the pointer to populated PidProcessMap on success. 
	 * If the map has not yet been populated, the reader will read the 
	 * [PROCESSDATA] section in the profile file.
	 */
	PidProcessMap * getProcessMap();

	/* Return the pointer to populated NameModuleMap on success. 
	 * If the map has not yet been populated, the reader will read the 
	 * [MODDATA] section in the profile file.
	 */
	NameModuleMap * getModuleMap();

	/* Return the pointer to populated CoreTopologyMap on success. */
	CoreTopologyMap * getTopologyMap();

	/* Return the pointer to CA_Module stored in the NameModuleMap 
	 * on success.  If the CA_Module entry has not yet been filled with 
	 * module detail from the IMD file, it will read the appropriated
	 * IMD file.
	 */
	CA_Module * getModuleDetail(wstring modName);

	wstring getPath() {return m_path;};
	
	/* Get the latest error code */
	int getErrorCode() {return m_err;};

private:
	bool checkVersion();

	bool readEnvSection();

	void processEnvLine(wstring const & line);

	bool getImd(CA_Module & mod);

	void processProcLine(PidProcessMap & procMap, wstring & line);

	void processModLine(NameModuleMap & modMap,	wstring & line);

private:
	CaProfileInfo	m_profileInfo;
	PidProcessMap	m_procMap;
	NameModuleMap	m_modMap;
	CoreTopologyMap m_topMap;
	int				m_err;
};

#endif //ifndef _CADATAREADER_H_
