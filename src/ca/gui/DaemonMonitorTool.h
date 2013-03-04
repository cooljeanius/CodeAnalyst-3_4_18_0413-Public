
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

#include <q3listview.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qstringlist.h>

#include <stdio.h>

#include "MonitorDockView.h"

#ifndef __DAEMONMONITORTOOL_H_
#define __DAEMONMONITORTOOL_H_

class DaemonMonitorTool;

class DaemonMonitorSnapshot : public MonitorSnapshot
{
public:
	DaemonMonitorSnapshot();

	virtual ~DaemonMonitorSnapshot();
	
	virtual bool takeSnapshot();

	virtual void signalSnapshotReady(QObject * obj);

	QStringList getSnapshot();

private:

	QFile * m_pLockFile;
	FILE  * m_pCmdFile;
	QDir  m_fdDir;
	QFileInfo m_dumpFile;

	QStringList m_snapData;
	QString m_lock;
};


///////////////////////////////////////////////////////////////////////////////
class DaemonMonitorTool : public Q3ListView 
{
public:
	DaemonMonitorTool(QWidget * parent);
	~DaemonMonitorTool();
	
	bool init();
};

#endif // __DAEMONMONITORTOOL_H_
