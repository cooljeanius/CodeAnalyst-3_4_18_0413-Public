
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

#include "OprofileDriverDockView.h"
#include "stdafx.h"

OprofileDriverDockView::OprofileDriverDockView(
	QWidget * parent, 
	const char * name , 
	Qt::WFlags f)
: MonitorDockView(parent,name,f)
{
	m_pDriverMonitorTool	= NULL;
	m_pSnapDriver		= NULL;
	m_pBLayout		= NULL;
	setCaption(QString("OProfile Driver Monitor"));
}


OprofileDriverDockView::~OprofileDriverDockView()
{

}

// Note: This function is re-entrant safe
bool OprofileDriverDockView::init()
{
	bool ret = false;

	m_pBLayout = this->boxLayout();
	if (!m_pBLayout)
		return false; 

	///////////////////////////////////////////////////////////
	// Driver Monitor
	if (!m_pDriverMonitorTool) {
		m_pDriverMonitorTool = new DriverMonitorTool(this);
		if (!m_pDriverMonitorTool)
			return false;

		m_pDriverMonitorTool->init();
		m_pBLayout->addWidget((QWidget*)m_pDriverMonitorTool);
	}

	///////////////////////////////////////////////////////////
	// Setup Snapshot
	if (!m_pSnapDriver) {
		m_pSnapDriver = new DriverMonitorSnapshot();
		if (!m_pSnapDriver)
			return false;
	}
	m_pSnap = (MonitorSnapshot*) m_pSnapDriver;
	
	ret = MonitorDockView::init();
	return ret;
}


void OprofileDriverDockView::updateWithSnapshot()
{
	if (!m_pDriverMonitorTool || !m_pSnapDriver)
		return;

	QStringList data;
	QStringList::iterator it;
	QStringList::iterator it_end;
	Q3ListViewItem * item = NULL;

	////////////////////////////////////////////	
	// General
	m_snapMutex.lock();
	data = m_pSnapDriver->getSnapGeneral();
	m_snapMutex.unlock();
	it 	= data.begin();
	it_end 	= data.end();
	
	item = m_pDriverMonitorTool->m_pGeneralTab->firstChild();
	if (!item) {
		// Insert item
		for (; it != it_end; it++) {
			item = new Q3ListViewItem(m_pDriverMonitorTool->m_pGeneralTab, 
				item, *(it), *(++it));
		}
	} else {
		// update existing item	
		for (; item
		     ; item = item->nextSibling()) {
			if (it != it_end) 
			{
				it++;
			}
			if (it != it_end) 
			{
				item->setText(1,*(it++));
			}
		}
	}
	
	////////////////////////////////////////////	
	// Stats 
	m_snapMutex.lock();
	data = m_pSnapDriver->getSnapStats();
	m_snapMutex.unlock();
	it 	= data.begin();
	it_end 	= data.end();
	
	item = m_pDriverMonitorTool->m_pStatsTab->firstChild();
	if (!item) {
		// Insert item
		for (; it != it_end; it++) {
			item = new Q3ListViewItem(m_pDriverMonitorTool->m_pStatsTab, 
				item, *(it), *(++it));
		}
	} else {
		// update existing item	
		for (; item
		     ; item = item->nextSibling()) {
			if (it != it_end) 
			{
				it++;
			}
			if (it != it_end) 
			{
				item->setText(1,*(it++));
			}
		}
	}
	
	////////////////////////////////////////////	
	// Cpu
	m_snapMutex.lock();
	data = m_pSnapDriver->getSnapCpu();
	m_snapMutex.unlock();
	
	if (data.isEmpty()) {
		m_pDriverMonitorTool->m_pCpuTab->clear();
	} else {
		it 	= data.begin();
		it_end 	= data.end();
		
		item = m_pDriverMonitorTool->m_pCpuTab->firstChild();
		if (!item) {
			// Insert item
			for (; it != it_end; it++) {
				item = new Q3ListViewItem(m_pDriverMonitorTool->m_pCpuTab, 
					item, *(it), *(++it), *(++it), *(++it), *(++it));
			}
		} else {
			// Update existing item	
			for (; item
			     ; item = item->nextSibling()) {
				if (it != it_end) item->setText(0, *it++);
				if (it != it_end) item->setText(1, *(it++));
				if (it != it_end) item->setText(2, *(it++));
				if (it != it_end) item->setText(3, *(it++));
				if (it != it_end) item->setText(4, *(it++));
			}
		}
	}
	
	////////////////////////////////////////////	
	// Ibs 
	m_snapMutex.lock();
	data = m_pSnapDriver->getSnapIbs();
	m_snapMutex.unlock();
	it 	= data.begin();
	it_end 	= data.end();
	
	item = m_pDriverMonitorTool->m_pIbsTab->firstChild();
	if (!item) {
		// Insert item
		item = new Q3ListViewItem(m_pDriverMonitorTool->m_pIbsTab, item, 
						*(it), *(++it), *(++it), *(++it));
		item = new Q3ListViewItem(m_pDriverMonitorTool->m_pIbsTab, item, 
						*(it), *(++it), *(++it), *(++it));
	} else {
		// Update existing item	
		for (; item
		     ; item = item->nextSibling()) {
			if (it != it_end) 
				it++;
			if (it != it_end) item->setText(1, *(it++));
			if (it != it_end) item->setText(2, *(it++));
			if (it != it_end) item->setText(3, *(it++));
		}
	}

	////////////////////////////////////////////	
	// Pmc
	m_snapMutex.lock();
	data = m_pSnapDriver->getSnapPmc();
	m_snapMutex.unlock();

	if (data.isEmpty()) {
		m_pDriverMonitorTool->m_pPmcTab->clear();
	} else {
		it 	= data.begin();
		it_end 	= data.end();
		Q3ListViewItem * item = NULL;
		item = m_pDriverMonitorTool->m_pPmcTab->firstChild();
		if (!item) {
			// Insert item
			for (; it != it_end; it++) {
				item = new Q3ListViewItem(m_pDriverMonitorTool->m_pPmcTab, 
					item, *(it), *(++it), *(++it), *(++it), *(++it));
			}
		} else {
			// Update existing item	
			for (; item
			     ; item = item->nextSibling()) {
				if (it != it_end) item->setText(0, *it++);
				if (it != it_end) item->setText(1, *(it++));
				if (it != it_end) item->setText(2, *(it++));
				if (it != it_end) item->setText(3, *(it++));
				if (it != it_end) item->setText(4, *(it++));
			}
		}
	}
}
