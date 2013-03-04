//$Id: opdata_handler.cpp $
// Interface for processing profile data.

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


#include <stdlib.h>
#include <vector>
#include <string>
#include <list>
#include <iomanip>
#include <qmessagebox.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

// CodeAnalyst headers
#include "CaProfileWriter.h"
#include "opdata_handler.h"
#include "proctree.h"

#ifdef HAVE_LIBELF_GELF_H
#include <libelf/gelf.h>
#else
#include <gelf.h>
#endif

using namespace std;


opdata_handler::opdata_handler()
{
	m_ca_display = NULL;
	m_missed = 0;
}

opdata_handler::~opdata_handler()
{
	reset_data();
}


void opdata_handler::reset_data()
{
}


void opdata_handler::init(EventMaskEncodeMap evt_map)
{
	m_event_encode_map = evt_map;
}


CA_Module * opdata_handler::getModule(string modName) 
{
	CA_Module *pRet = NULL;
	wstring wModName (modName.begin(), modName.end());

	NameModuleMap::iterator it = m_modMap.find(wModName);
	if (it != m_modMap.end()) {
		pRet = &(it->second);
	} else {
		CA_Module mod;
		mod.setPath(wModName);
		m_modMap.insert(NameModuleMap::value_type(wModName, mod));
	
		// Re-find	
		it = m_modMap.find(wModName);
		if (it != m_modMap.end())
			pRet = &(it->second);
	}

	return pRet;
}


CA_Process * opdata_handler::getProcess(unsigned int pid) 
{
	CA_Process *pRet = NULL;

	PidProcessMap::iterator it = m_procMap.find(pid);
	if (it != m_procMap.end()) {
		pRet = &(it->second);
	} else {
		CA_Process proc;
		m_procMap.insert(PidProcessMap::value_type(pid, proc));
	
		// Re-find	
		it = m_procMap.find(pid);
		if (it != m_procMap.end())
			pRet = &(it->second);
	}

	return pRet;
}


bool opdata_handler::write_ebp_output( string outputFile, 
					unsigned int ncpu, 
					unsigned long cpuFamily, 
					unsigned long cpuModel)
{
	bool bRet = false;

	CaProfileWriter writer;

	m_profInfo.m_numCpus   = ncpu;
	m_profInfo.m_cpuFamily = cpuFamily;
	m_profInfo.m_cpuModel  = cpuModel;
	m_profInfo.m_numModules = m_modMap.size();
	m_profInfo.m_numEvents  = m_event_encode_map.size();

	////////////////////////////////
	// Get total number of sample
	m_profInfo.m_numSamples = 0;
	PidProcessMap::iterator pit  = m_procMap.begin();
	PidProcessMap::iterator pend = m_procMap.end();
	for (; pit != pend; pit++) {
		m_profInfo.m_numSamples += pit->second.getTotal();
	}
	
	m_profInfo.m_numMisses = 0; // This is not available

	////////////////////////////////	
	// Get time.
	time_t wrTime = time(NULL);
	string str(ctime(&wrTime));
	m_profInfo.m_timeStamp = wstring(str.begin(), str.end());

	// Remove the newline at the end
	if (*(m_profInfo.m_timeStamp.rbegin()) == L'\n')
		m_profInfo.m_timeStamp.resize(m_profInfo.m_timeStamp.size() -1 ); 

	/////////////////////////////////
	// Convert map to vector	
	// FIXME: Should optimize this
	EventMaskEncodeMap::iterator eit = m_event_encode_map.begin();
	EventMaskEncodeMap::iterator eit_end = m_event_encode_map.end();
	for (; eit != eit_end; eit++) {
		m_profInfo.m_eventVec.push_back(eit->second);
	}


	bRet = writer.write(wstring(outputFile.begin(), outputFile.end())	
				, &m_profInfo, &m_procMap, &m_modMap);

	return bRet;
}


bool opdata_handler::copyJncFiles(QString sessiondir)
{
	QString jitdir;
	QDir t_dir;

	QString pre_java_id;
	NameModuleMap::iterator m_it     = m_modMap.begin();
	NameModuleMap::iterator m_it_end = m_modMap.end();
	for(; m_it != m_it_end; m_it++)
	{
		if(m_it->second.getModType() != JAVAMODULE)
			continue;

		if (jitdir.isEmpty()) {
			jitdir = sessiondir + "/jit";
			t_dir.mkdir (jitdir);
		}	

		AddrFunctionMultMap::const_iterator fit = m_it->second.getBeginFunction();
		AddrFunctionMultMap::const_iterator fend = m_it->second.getEndFunction();
		for (; fit != fend; fit++) {
			
			wstring wPath =fit->second.getJncFileName();
			QString src(QString::fromStdString(string(wPath.begin(), wPath.end())));

			if (!src.startsWith("jit"))
				continue;

			QString java_id = src.section("/", 1, 1);
			if (pre_java_id != java_id) {
				// this is new java task;
				t_dir.mkdir(jitdir + "/" + java_id);
				pre_java_id = java_id;
			}

			QString dest = sessiondir + "/" + src;
		
			QFile oldfile (QString("/var/lib/oprofile/") + src);
			if (oldfile.open(QIODevice::ReadOnly)) {
				QFile newfile (dest);
				if (newfile.open(QIODevice::WriteOnly)) {
					const QByteArray tmpbuffer = oldfile.readAll();
					newfile.writeBlock(tmpbuffer);
					newfile.close();
				}
				oldfile.close();	
			}
				
		}
	}
	return true;
}


bool opdata_handler::copyJoFile(QString dirPath)
{
	NameModuleMap::iterator m_it     = m_modMap.begin();
	NameModuleMap::iterator m_it_end = m_modMap.end();
	for(; m_it != m_it_end; m_it++)
	{
		wstring wPath = m_it->second.getPath();
		QString modName(QString::fromStdString(string(wPath.begin(), wPath.end())));
		if(modName.endsWith(".jo"))
		{
			QFile joFile;
			joFile.setName(modName);
			while(!joFile.exists())
			{
				QString msg = "Could not find " + modName + ".\nBrowse for File?";

				// the image does not exist on the hard disk
				if (QMessageBox::information (qApp->activeWindow(), "Error", msg,
					QMessageBox::Yes, QMessageBox::No) != 
					QMessageBox::Yes) 
				{
					return false;
				}else {
					modName = Q3FileDialog::getOpenFileName ();
					if (modName.isEmpty ())
						return false;
					else
						joFile.setName(modName);
				}
			}

			QString cmd_cp("cp -f \"" + modName + "\" \"" + dirPath + "\"");
			if(system(cmd_cp.toAscii().data()) == -1)
				return false;
		}
	}
	return true;
}


unsigned int opdata_handler::getTaskBitness(string taskname)
{
	unsigned int ret = evUnknown;
	int fd;
	int elfClass;
	Elf * e = NULL;

	if(elf_version(EV_CURRENT) == EV_NONE)
		return ret;
	
	if(( fd = open (taskname.c_str(), O_RDONLY, 0)) >= 0)
	{
		if(( e = elf_begin(fd, ELF_C_READ, NULL)) != NULL )
		{
			elfClass = gelf_getclass(e);

			if(elfClass == ELFCLASS32){
				ret = ev32bit;	
			} else if(elfClass == ELFCLASS64) {
				ret = ev64bit;
			}
	
			elf_end(e);
		}
		close(fd);
	}

	if (ret == evUnknown) {
		QString cmd_uname("uname -m | grep x86_64 > /dev/null");
		if(system(cmd_uname.toAscii().data()) != 0)
			ret = ev32bit;
		else
			ret = ev64bit;
	}
	
	return ret;
}
