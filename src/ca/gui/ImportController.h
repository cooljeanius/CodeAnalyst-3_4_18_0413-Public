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


#ifndef _IMPORTCONTROLLER_H_
#define _IMPORTCONTROLLER_H_

#include <qstring.h>

#include "stdafx.h"
#include "eventsfile.h"
#include "cawfile.h"
#include "translatedlg.h"
#include "catranslate.h"
#include "catranslate_display.h"

class ImportController
{
public:
	ImportController(bool isGui = true);

	~ImportController();

	bool onImportXmlFile(QString import_xml_path,
				QString import_xml_cpuinfo_path);

	bool onImportSessionDir(QString session_dir);


	bool onImportLocalRemoteFile (QString op_sample_dir,
				QString import_cpuinfo_path,		
				bool isApplyProcessFilter,
				QStringList processFilter,
				bool is_remote_import);
#ifdef CAPERF_SUPPORT_ENABLED
	bool onImportPerfFile(QString strPerfDataFile);
#endif //CAPERF_SUPPORT_ENABLED
	QString getErrorMessage() { return m_importErrorMsg; };
	
	void setCawFile( CawFile * cawFile) { m_pCawFile = cawFile; };

	QString getCurrentSessionName() { return m_curSessionName; };

	TRIGGER getCurrentTrigger() { return m_curTrigger; };
	
	void setTranslationDisplay(ca_translate_display * display)
	{ m_pCATranslateDisplay = display; };

	void showWarning(QString msg);
	
	void showError(QString msg); 

private:
	bool renameSessionFileAndCopy(
			QString projDir, QString srcDir, 
			QString newName, QString type );

	bool onImportEbpData ( OP_EVENT_PROPERTY_MAP events_name_map,
			QString sessionName,
			QString sessionNote,
			CEventsFile * eventFile,
			QString import_cpuinfo_path,
			QString op_sample_dir,
			bool isApplyProcessFilter,
			QStringList processFilter);

	void PopulateEbpSettingsForImportSession( EBP_OPTIONS *pEbs,
			OP_EVENT_PROPERTY_MAP &pmc_events_name_map,
			CEventsFile * eventFile,
			EventMaskEncodeMap &events_map);

	bool findOprofileEvents (const QDir & _d, OP_EVENT_PROPERTY_MAP & events_name);

	static bool findOPSamplesFile(const QDir &_d, CEventsFile & _e);

	static bool validateDirContainsOPSamples (const QDir &_d, CEventsFile & _e);

	static void parseOPSampleFileName(const QString & fname, 
			OP_EVENT_PROPERTY_MAP & events_name);

	static QString parseOPSampleFileForEventName(const QString & fname);

	QString getNextImportSessionName(TRIGGER trigger, QString curName = "");
	
	QString getSessionNote();

	QString			m_importErrorMsg;
	QStringList		m_processFilter;
	TRIGGER			m_curTrigger;
	QString			m_curSessionName;
	bool			m_isGui;

	CawFile			*m_pCawFile;
	SESSION_OPTIONS 	*m_pSession;

	ca_translate_display 	*m_pCATranslateDisplay;
	CATranslate 		*m_pCATranslate;
};

#endif
