
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

#include "stdafx.h"
#include "DaemonMonitorTool.h"
#include <QEvent>
#include <QDateTime>
#include <QTextStream>

DaemonMonitorSnapshot::DaemonMonitorSnapshot()
{
	m_pLockFile = new QFile(QString("/var/lib/oprofile/lock"));
	m_dumpFile = QFileInfo(QString("/var/lib/oprofile/complete_dump"));
}


DaemonMonitorSnapshot::~DaemonMonitorSnapshot()
{
}


bool DaemonMonitorSnapshot::takeSnapshot()
{
	bool hasDaemon = false;

	// MUTEX BEGIN
	m_snapData.clear();

	// Lock file
	if (m_pLockFile
	&&  m_pLockFile->open(QIODevice::ReadOnly)) 
	{
		QTextStream stream(m_pLockFile);	
		m_lock = stream.readLine();
		m_pLockFile->close();
	}

	if (m_lock.isEmpty())
		m_lock = "N/A";

	m_snapData.push_back(QString("/var/lib/oprofile/lock (pid)"));
	m_snapData.push_back(m_lock);

	// cmdline
	QString tmp = "N/A";
	QString cmdFile = QString("/proc/") + m_lock + "/cmdline";
	if ((m_pCmdFile = fopen(cmdFile.toAscii().data(),"r"))!= NULL) {
		char buffer[1024] = {'\0'};
		tmp = "";
		while (!feof(m_pCmdFile)) {
			fread(buffer, 1, 1, m_pCmdFile);
			tmp = tmp + QString(buffer);
		}
		hasDaemon = true;
	}
	m_snapData.push_back(QString("Command Line"));
	m_snapData.push_back(tmp);

	// File descriptor count
	if (!hasDaemon) {
		tmp = "N/A";	
	} else {
		m_fdDir = QDir(QString("/proc/") + m_lock + "/fd");
		tmp = QString::number(m_fdDir.count(),10);
	}
	m_snapData.push_back(QString("File Descriptor Count"));
	m_snapData.push_back(tmp);

	// DumpFile
	if (!m_dumpFile.exists()) {
		tmp = "N/A";
	} else {
		tmp = (m_dumpFile.lastModified()).toString("ddd MMMM d yyyy");
	}
	m_snapData.push_back(QString("Last Dump Timestamp"));
	m_snapData.push_back(tmp);
	// MUTEX END

	return true;
}


void DaemonMonitorSnapshot::signalSnapshotReady(QObject *obj)
{
	QApplication::postEvent (obj, new QEvent ((QEvent::Type) EVENT_UPDATE_OPROFILED_MONITOR));	
}


QStringList DaemonMonitorSnapshot::getSnapshot()
{
	QStringList tmp;
	// MUTEX BEGIN

	tmp = m_snapData;	


	// MUTEX END 
	
	return tmp;
}


///////////////////////////////////////////////////////////////////////////////
DaemonMonitorTool::DaemonMonitorTool(QWidget * parent) :
Q3ListView(parent)
{
	// Set column	
	this->addColumn(QString("OProfile Daemon Information"));
	this->setColumnAlignment(0, Qt::AlignLeft);

	this->addColumn(QString("Value"), 200);
	this->setColumnAlignment(1, Qt::AlignLeft);

	// Set property
	setSortColumn(-1);
	setSelectionMode(Q3ListView::NoSelection);
	setAllColumnsShowFocus(true);
	setResizeMode(Q3ListView::NoColumn);

}


DaemonMonitorTool::~DaemonMonitorTool()
{
}


bool DaemonMonitorTool::init()
{
	bool ret = false;

	// First time
	DaemonMonitorSnapshot snap;
	snap.takeSnapshot();

	// Insert QListViewItems
	QStringList list = snap.getSnapshot();

	QStringList::iterator it     = list.begin();
	QStringList::iterator it_end = list.end();
	Q3ListViewItem * item = NULL;
	for (; it != it_end; it++) {
		item = new Q3ListViewItem(this, item, *(it), *(++it));
	}

	ret = true;	
	return ret;	
}

