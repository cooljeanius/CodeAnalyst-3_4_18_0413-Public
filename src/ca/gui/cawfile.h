//$Id: cawfile.h,v 1.4 2006/08/17 14:24:57 ssuthiku Exp $

/*
// CodeAnalyst for Open Source
// Copyright 2002 - 2007 Advanced Micro Devices, Inc.
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
#ifndef _CAWFILE_H
#define _CAWFILE_H

#include "stdafx.h"
#include "tbsStruct.h"
#include "DCConfigAPI.h"
#include "PerfEventContainer.h"
#include "affinity.h"
#include <QTextStream>

#define GENERAL_SESSION	0
#define MAX_SESSIONS	100
#define MAX_HISTORY 10

// forward declarations
class CWorkspaceOptions;
struct EBS_SESSION_OPTIONS;
struct TBS_SESSION_OPTIONS;

// typedefs
const QString PROJECT_FILE_VERSION = "8";
const int PROJECT_INT_FILE_VERSION =  8 ;
const QString CURRENT_TIME = "Current time-based profile";
const QString PERF_CURRENT_TIME = "Current PERF time-based profile";

////////////////////////////////////////////////////////////////////////////
// Structures
//
class RUN_SETTING
{
public:
	QString		sessionName;
	QString 	sessionNote;
	QString 	dcConfig;
	QString 	launch;
	QString 	workDir;
	bool 		stopOnAppExit;
	bool 		termAppOnDur;
	int 		duration;
	bool 		startPaused;
	int 		startDelay;
	CA_Affinity	affinity;
	bool		affinityEnable;
	bool		filterEnable;
	QString		filterList;
	bool		cssEnable;
	int		cssDepth;
	int		cssInterval;
	bool 		cssUseTgid;
	int		cssTgid;
	bool		vmlinuxEnable;
	QString		vmlinux;
	int		opWaterShed;
	int		opBuffer;
	int		opCpuBuffer;
	bool		showTerminalEnable;
	QString		info;
	

	RUN_SETTING () {
		stopOnAppExit 	= false;
		termAppOnDur 	= false;
		startPaused 	= false;
		duration 	= -1;		// To mark N/A
		startDelay 	= -1;		// To mark N/A
		affinityEnable	= false;
		filterEnable	= false;
		cssEnable	= false;
		cssDepth	= -1;
		cssInterval	= -1;
		cssUseTgid	= false;
		cssTgid		= -1;
		vmlinuxEnable	= false;
		opWaterShed	= -1;
		opBuffer	= -1;
		opCpuBuffer	= -1;
		showTerminalEnable	= false;
	};
};

typedef QMap<QString, RUN_SETTING *> SettingMap;

///////////////////////////////////////////////////////////////////////////////////
//

class SESSION_OPTIONS : public RUN_SETTING
{
public:
	//Note that the sessionFile is not the path. 
	QString		sessionFile;  
	TRIGGER		Trigger;
	int 		Cpus;
	PerfEventContainer m_eventContainer;

	SESSION_OPTIONS() : RUN_SETTING() {
		Trigger 	= NO_TRIGGER;
		Cpus 		= 1;
	};

	SESSION_OPTIONS operator= (const RUN_SETTING opt) {
		RUN_SETTING * tmp = this;
		*tmp 		= opt;
		return *this;
	};

	SESSION_OPTIONS operator=(const SESSION_OPTIONS opt ) {
		*this		= (RUN_SETTING) opt;
		sessionFile	= opt.sessionFile;
		Trigger		= opt.Trigger;
		m_eventContainer = opt.m_eventContainer;
		return *this;
	};

	void setSessionName(QString name, QString ext) {
		sessionFile 	= name + ext;	
		sessionName 	= name;	
	};
	
	PerfEventContainer * getEventContainer() {return & m_eventContainer;};
};

typedef QMap<QString, SESSION_OPTIONS *> SessionMap;

///////////////////////////////////////////////////////////////////////////////////
//
struct TBP_OPTIONS : public SESSION_OPTIONS
{
	float		msInterval;

	TBP_OPTIONS()  : SESSION_OPTIONS() { 
		Trigger 	= TIMER_TRIGGER;
		msInterval	= 0;
	}

	TBP_OPTIONS operator=(const SESSION_OPTIONS opt ) {
		SESSION_OPTIONS * tmp = this;
		*tmp 		= opt;
		Trigger 	= TIMER_TRIGGER;
		return *this;
	};

};

struct PERF_TBP_OPTIONS : public SESSION_OPTIONS
{
	float		msInterval;

	PERF_TBP_OPTIONS()  : SESSION_OPTIONS() { 
		Trigger 	= PERF_TIMER_TRIGGER;
		msInterval	= 0;
	}

	PERF_TBP_OPTIONS operator=(const SESSION_OPTIONS opt ) {
		SESSION_OPTIONS * tmp = this;
		*tmp 		= opt;
		Trigger 	= PERF_TIMER_TRIGGER;
		return *this;
	};

};

///////////////////////////////////////////////////////////////////////////////////
//
class EBP_OPTIONS : public SESSION_OPTIONS
{
public:
	unsigned int 	msMpxInterval;	

	EBP_OPTIONS ()  : SESSION_OPTIONS() {
		Trigger 	= EVENT_TRIGGER;
		msMpxInterval 	= 0;	// To mark N/A
	};

	EBP_OPTIONS operator=(const SESSION_OPTIONS opt ) {
		SESSION_OPTIONS * tmp = this;
		*tmp 		= opt;	
		Trigger		= EVENT_TRIGGER;
		return *this;
	};

	EBP_OPTIONS operator=(const EBP_OPTIONS opt ) {
		*this		= (SESSION_OPTIONS) opt;
		msMpxInterval	= opt.msMpxInterval;
		return *this;
	};
};


/////////////////////////////////////////////////////////////////////////
// CawFile class
//
class CawFile {
public:
	CawFile (QString FileName);
	~CawFile ();

public:
	static bool create (const QString fileName);
	bool create ();
	bool openCawFile ();
	bool saveCawFile ();

	// Run Setting  Stuff	
	bool addRunSetting (QString name, RUN_SETTING * pRunSetting);
	bool deleteRunSetting (QString name);
	bool setLastRunSetting(QString name);
	QString getLastRunSettingName() {return m_lastRunSettingName;};
	QStringList getSettingList ();
	RUN_SETTING * getRunSetting (QString name);
	RUN_SETTING * getLastRunSetting();

	// History Stuff	
	QString getLastProfile ();
	QStringList getLaunchHistory ();
	void addLaunchHistory(QString launch);
	QStringList getWorkDirHistory ();
	void addWorkDirHistory(QString workDir);
	void setLastProfile (QString profileName);

	// Session Name
	QString getNextSessionName (TRIGGER trigger, QString orgName = QString());
	bool IsSessionNameTaken (TRIGGER trigger, QString SessionName);
	void deleteSession (TRIGGER trigger, QString SessionName);
	void renameSession (TRIGGER trigger,  QString oldname, QString newname );
	void addSession (SESSION_OPTIONS *const pSession);
	
	// Session Info 
	bool getSessionCfg (TRIGGER trigger, QString name, SESSION_OPTIONS *pSession);
	bool getSessionDetails (QString name, SESSION_OPTIONS *pSession);
	QStringList getSessionList (const TRIGGER trigger);
	bool updateSessionNote(TRIGGER trigger, QString name, QString notes);


	QString getPath () { return m_szFilePath; };
	QString getName () { return m_szFileName; };
	QString getDir () { return m_szFileDir; };
	QString getWorkspaceDir ();

private:
	int  getVersion (QString first_line);
	bool helpReadRunSetting (QString parm, QString value, RUN_SETTING *pRun);
	void readGeneralSection (QTextStream * pStream);
	void readSettingSection (QTextStream * pStream);
	void readTbpSection ( QTextStream *pStream );
	void readPerfTbpSection ( QTextStream *pStream );
	void readEventSection( QTextStream *pStream );

	void helpWriteRunSetting(QTextStream *pStream, RUN_SETTING *pRun);
	void writeSettingSection (QTextStream *pStream );
	void writeTbpSection (QTextStream *pStream );
	void writePerfTbpSection (QTextStream *pStream );
	void writeEventSection (QTextStream *pStream );
	void writeIbsSection (QTextStream *pStream );

	SettingMap 	m_settingMap;
	SessionMap 	m_sessions[NO_TRIGGER];

	QString 	m_lastRunSettingName;
	QStringList 	m_launchHistory;
	QStringList 	m_workDirHistory;

	QString 	m_szFileName;
	QString 	m_szFileDir;
	QString 	m_szFilePath;

	int 		m_cawVersion;
};

#endif
