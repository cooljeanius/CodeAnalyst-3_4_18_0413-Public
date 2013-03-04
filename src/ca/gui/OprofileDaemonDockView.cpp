
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

#include <qtooltip.h>

#include "OprofileDaemonDockView.h"
#include "stdafx.h"

OprofileDaemonDockView::OprofileDaemonDockView(
	QWidget * parent, 
	const char * name , 
	Qt::WFlags f)
: MonitorDockView(parent,name,f)
{
	m_pDaemonMonitorTool 	= NULL;
	m_pSnapDaemon		= NULL;
	m_pBLayout		= NULL;
	setCaption(QString("OProfile Daemon Monitor"));
}


OprofileDaemonDockView::~OprofileDaemonDockView()
{

}

// Note: This function is re-entrant safe
bool OprofileDaemonDockView::init()
{
	bool ret = false;

	m_pBLayout = this->boxLayout();
	if (!m_pBLayout)
		return false; 

	///////////////////////////////////////////////////////////
	// Daemon Monitor 
	if (!m_pDaemonMonitorTool) {
		m_pDaemonMonitorTool = new DaemonMonitorTool(this);
		if (!m_pDaemonMonitorTool)
			return false;

		m_pDaemonMonitorTool->init();	
		m_pBLayout->addWidget(m_pDaemonMonitorTool);
	}

	///////////////////////////////////////////////////////////
	// Setup Snapshot
	if (!m_pSnapDaemon) {
		m_pSnapDaemon = new DaemonMonitorSnapshot();
		if (!m_pSnapDaemon)
			return false;
	}
	m_pSnap = (MonitorSnapshot*) m_pSnapDaemon;
	
	ret = MonitorDockView::init();
	return ret;
}


void OprofileDaemonDockView::updateWithSnapshot()
{
	if (!m_pDaemonMonitorTool || !m_pSnapDaemon)
		return;

	m_snapMutex.lock();
	QStringList data = m_pSnapDaemon->getSnapshot();
	m_snapMutex.unlock();
	
	QStringList::iterator it = data.begin();
	QStringList::iterator it_end = data.end();

	Q3ListViewItem * item = m_pDaemonMonitorTool->firstChild();
	if (!item) {
		// Insert item
		item = new Q3ListViewItem(m_pDaemonMonitorTool, item, 
						*(it), *(++it));
		item = new Q3ListViewItem(m_pDaemonMonitorTool, item, 
						*(it), *(++it));
	} else {
		// Update existing item	
		for (; item ; item = item->nextSibling()) {
			if (it != it_end) 
				it++;
			if (it != it_end) 
				item->setText(1, *(it++));
		}
	}
}


