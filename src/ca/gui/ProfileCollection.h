//$Id: ProfileCollection.h 18421 2010-08-03 14:02:40Z ssuthiku $
//This file defines the ProfileCollection Interface, which handles most of the
//	profile file interaction
/*
// CodeAnalyst for Open Source
// Copyright 2006-2007 Advanced Micro Devices, Inc.
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

#ifndef PROFILE_COLLECTION_H
#define PROFILE_COLLECTION_H

#include "stdafx.h"
#include "DCConfigAPI.h"
#include "cawfile.h"

const QString CURRENT_EVENT = "Current event-based profile";
const QString CURRENT_IBS = "Current instruction-based profile";

class ProfileElementType
{
public:
	QString path;
	bool modifiable;
	CDCConfig * pConfigure;
	ProfileElementType () 
	{
		pConfigure = NULL;
	};
	~ProfileElementType()
	{
		if (NULL != pConfigure)
			delete pConfigure;
	};
};

typedef QMap<QString, ProfileElementType> ConfigMap;

enum {
	PROFILE_IMPORT_OKAY = 0,
	PROFILE_FILE_NOT_FOUND,
	PROFILE_DUPLICATE_NAME,
	PROFILE_INVALID_TYPES,
	PROFILE_INVALID_EVENTS,
};


class ProfileCollection 
{
public:
	ProfileCollection ();
	~ProfileCollection ();

	void readAvailableProfiles(CEventsFile * pEventsFile);
	QStringList getListOfProfiles ();
	QStringList getListOfEbpProfiles ();
	bool profileExists (QString name);

	TRIGGER getProfileType (QString name);
	bool getProfileTexts (QString name, QString *pToolTip, QString *pDescrip);
	
	//Note that the correct type needs to be passed in...
	bool getProfileConfig (QString name, SESSION_OPTIONS * pConfig);
	bool setProfileConfig (QString name, SESSION_OPTIONS * pConfig,
				QString tip = QString(), QString desc = QString());

	bool exportProfile (QString name, QString fileName);
	int importProfile (QString fileName, CEventsFile * pEventsFile);
	bool removeProfile (QString name);
	QString getFileName (QString name);

	QString getLastModified ();
	void setLastModified (QString name);
	void clearLastModified ();
	QString getErrorMsg() { return m_errMsg; };
private:
	int readAndAddProfile (QString fileName, CEventsFile * pEventsFile,
				bool bMod = false, bool bImport= false);
	void verifyAMDPlatform ();

	bool hasEvent(CEventsFile * pEventsFile, unsigned int select );

	ConfigMap 	m_configs;
	QString 	m_lastModified;
	QString		m_errMsg;
};
#endif
