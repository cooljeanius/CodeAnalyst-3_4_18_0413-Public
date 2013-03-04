//$Id: projectwindow.h,v 1.2 2005/02/14 16:52:40 jyeh Exp $
// interface for the ProjectWindow class.

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2005 Advanced Micro Devices, Inc.
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


//Revision history
//$Log: projectwindow.h,v $
//Revision 1.2  2005/02/14 16:52:40  jyeh
//Updated header.
//
//Revision 1.1.1.1  2004/10/05 17:03:23  jyeh
//initial import into CVS
//
//Revision 1.4  2003/12/11 00:11:56  franksw
//Code Review Project Window
//
//Revision 1.3  2003/11/17 23:24:40  franksw
//code cleanup
//

#ifndef _PROJECTWINDOW_H
#define _PROJECTWINDOW_H

#include <q3listview.h>
#include <qmap.h>
#include <q3popupmenu.h>
#include <QResizeEvent>
#include <QString>
#include <QKeyEvent>
// #include "NewDiffSessionDlg.h"
#include "stdafx.h"

static const QString TBP_SESSION_STR = "TBP Sessions";
static const QString PERF_TBP_SESSION_STR = "TBP Perf Sessions";
static const QString EBP_SESSION_STR = "EBP Sessions";
static const QString NO_PROJECTS_OPENED = "No Project Opened";

//////////////////////////////////////////////////////////////////////////////
//This tooltip is modified for a specific list view column
//class ProjWndListTip : public QToolTip
class ProjWndListTip : public QWidget
{
public:
	//ProjWndListTip ( int tipColumn, Q3ListView * pParent);
	ProjWndListTip (Q3ListView * pParent);
	virtual ~ProjWndListTip() {};

protected:
//	virtual void maybeTip (const QPoint &pt);
	bool event (QEvent *pEvent);
	

private:
	Q3ListView * m_pList;
	int m_tipColumn;
};


//////////////////////////////////////////////////////////////////////////////
class ProjWndListItem : public Q3ListViewItem
{
public:
	ProjWndListItem(Q3ListView * parent,
				ProjWndListItem * after,
				QString name);

	ProjWndListItem(ProjWndListItem * parent,
				ProjWndListItem * after,
				QString name);
	
	ProjWndListItem(Q3ListView * parent,
				QString name);

	ProjWndListItem(ProjWndListItem * parent,
				QString name);

	~ProjWndListItem();

	void setTip(QString tip) { m_tip = tip; };
	QString getTip() { return m_tip; };
private:
	QString m_tip;
};


//////////////////////////////////////////////////////////////////////////////
// ProjectWindow
//
class ProjectWindow : public QWidget
{
	Q_OBJECT
public:

	ProjectWindow ( QWidget * pParentWnd );
	~ProjectWindow();

	void setCaption ( QString str );
	void deleteAllItems();
	void initialize();
	void insertTbpSamplingSession ( QString session_name, QString session_note );
	void insertPerfTbpSamplingSession ( QString session_name, QString session_note );
	void insertEbpSamplingSession ( QString session_name, QString session_note );

protected:
	void resizeEvent ( QResizeEvent * e );
	void keyReleaseEvent ( QKeyEvent * e ); 

signals:
	void newSessionSelected ( QString session_name );

	void SessionDblClicked (TRIGGER trigger, QString SessionName);
	void reinitProjectWindow();	
	void SessionDeletedAndReinit (TRIGGER trigger, QString SessionName);
	void SessionDeletedOnly (TRIGGER trigger, QString SessionName);
	void SessionRenamed(TRIGGER trigger, QString OldSessionName); 

	void tbpSessionProperties ( QString session_name );
	void perftbpSessionProperties ( QString session_name );
	void ebpSessionProperties ( QString session_name );

	void CopySessionToCurrent (TRIGGER trigger, QString SessionName);
	// void openSessionInDiffAnalyst(DiffSessionInfo *sess);
	void openKcachegrind(QString sessionDirName);

public slots:
	void expandTbp();
	void expandTbpPerf();
	void expandEbp();
	void onRenameSession();
	void onPropertiesSession();
	void OnCopyToCurrent();
	void onDblClicked ( Q3ListViewItem * item );
	void onSelChange ( Q3ListViewItem * item );	
	void onExpanded ( Q3ListViewItem * item );
	void onRightButtonClicked ( Q3ListViewItem * item, const QPoint &pt,
				    int col );
	void deleteCurrentSessionItem();
	void deleteAllSessionItem();

private:
	ProjWndListItem *m_tbp;
	ProjWndListItem *m_perftbp;
	ProjWndListItem *m_ebp;
	Q3ListView 	*m_tree;
	ProjWndListItem *m_current_item;
	Q3PopupMenu 	*m_menu;
	Q3PopupMenu 	*m_menu_parent;
	ProjWndListTip	*m_tabListTip; 

private slots:
	// void onOpenSessionInDiffAnalyst();
	void onOpenKcachegrind();
	
};




#endif
