
/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2008 Advanced Micro Devices, Inc.
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

#include <qmessagebox.h>
#include "PropDockView.h"
#include "bfd.h"
#include "stdafx.h"
#include <qiconset.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include "xpm.h"

PropDockView::PropDockView(QWidget * parent, 
			const char * name , 
			WFlags f)
: QDockWindow(parent,name,f)
{
	m_pBLayout = NULL;
	m_pDiffSession = NULL;
	m_pMainWindow = NULL;
	setNewLine(true);
	setResizeEnabled(true);
	
	// Setup size when doc
	setFixedExtentHeight(100);
	setCaption("Property : [ ]");
}

PropDockView::~PropDockView()
{
	// Do no delete this.
	m_pDiffSession = NULL;
}

bool PropDockView::init()
{
	// Setup docking property
	bool ret=false;

	// Get layout
	m_pBLayout = boxLayout();

	m_pView = new QListView(this);
	m_pBLayout->addWidget(m_pView);
	m_pView->setAllColumnsShowFocus(true);
	m_pView->setSortColumn(-1);
	m_pView->setResizePolicy( QScrollView::Manual );

	addColumns();
	setProperty();

	ret = true;
	return ret;
}

void PropDockView::onPropDockViewShowChanged(bool b)
{
	if(b)
	{
		this->show();
	}else{
		this->hide();
	}
}

bool PropDockView::close(bool alsoDelete)
{
	Q_UNUSED(alsoDelete);
	// DO NOT CLOSE, just hide it
	this->hide();
	emit propDockViewNoShow();
	return false;
}

void PropDockView::onChangeSession(DiffSession *s)
{
	m_pView->clear();
	
	m_pDiffSession = s;
	if(m_pDiffSession == NULL)
	{
		return;
	}
	
	setCaption(QString("Property : [")+ m_pDiffSession->name() +"]");
	setProperty();
}

void PropDockView::addColumns()
{
	m_pView->setResizeMode(QListView::AllColumns);
	m_pView->addColumn("Property");
	m_pView->setColumnWidthMode(0,QListView::Manual);
	m_pView->addColumn("Session 0 ");
	m_pView->addColumn("Session 1 ");
}

void PropDockView::fillProperty()
{
	if(m_pDiffSession == NULL) return;
	SESSION_DIFF_INFO_VEC* pDiffInfoVec = m_pDiffSession->getSessionDiffInfo();
	if(pDiffInfoVec->size() == 0) return;
	QString title, tmp0, tmp1;	
	QListViewItem *pSess;
	
	// EBP File
	title = "Profile Session File";
	tmp0 = ((*pDiffInfoVec)[0]).sessionFile;
	tmp1 = ((*pDiffInfoVec)[1]).sessionFile;
	pSess = new QListViewItem(m_pView,title,tmp0,tmp1);
	
	// Task Name
	title = "Task Name";
	if(!((*pDiffInfoVec)[0]).task.isEmpty())
	{
		tmp0 = ((*pDiffInfoVec)[0]).task;
	}else{
		tmp0 = "All Tasks";
	}
	if(!((*pDiffInfoVec)[1]).task.isEmpty())
	{
		tmp1 = ((*pDiffInfoVec)[1]).task;
	}else{
		tmp1 = "All Tasks";
	}
	pSess = new QListViewItem(m_pView,pSess,title,tmp0,tmp1);

	// Module Name
	title = "Module Name";
	tmp0 = ((*pDiffInfoVec)[0]).module;
	tmp1 = ((*pDiffInfoVec)[1]).module;
	pSess = new QListViewItem(m_pView,pSess,title,tmp0,tmp1);
	
	m_pView->setResizeMode(QListView::AllColumns);
}

void PropDockView::setProperty()
{
	fillProperty();

}
