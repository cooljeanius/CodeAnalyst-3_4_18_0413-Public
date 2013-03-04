//$Id: ViewManageDlg.h 14389 2008-04-01 16:58:45Z ssuthiku $
//  This is our derivative of the iViewMangeDlg.ui

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

#ifndef _VIEWMANAGEDLG_H_
#define _VIEWMANAGEDLG_H_

#include <qstring.h>
#include <qlistbox.h> 
#include <qwidget.h>

#include "iViewManageDlg.h"
#include "ViewCollection.h"
#include "DiffViewShownData.h"
#include "diffStruct.h"
#include "DiffSession.h"


enum
{
	VIEWCFG_MANAGE_TAB = 0,
	SESSION_VIEW_OPTION_TAB
};

class ShownListBoxItem : public QListBoxText
{
public:
	ShownListBoxItem ( QListBox * listbox, const QString & text, int index);
	int m_index;
};

class ViewManageDlg : public iViewManageDlg 
{ 
	Q_OBJECT

public:
	ViewManageDlg( QWidget* parent = 0,
		DiffSession* pDiffSession = NULL, 
		const char* name = 0, 
		bool modal = true,
		WFlags fl = WStyle_Customize | 
		WStyle_DialogBorder | WStyle_Title | WDestructiveClose );

	virtual ~ViewManageDlg();

	void setViews (QString current, 
			unsigned long cpuFamily,
			unsigned long cpuModel);

	void setTitle(QString name)
	{setCaption(QString("View Management : [ ") + name + " ]");};

	void setCurrentTab(int index)
	{ tabWidget->setCurrentPage(index); };

	void setTabEnabled(int index, bool b)
	{ tabWidget->setTabEnabled(tabWidget->page(index),b); };

protected slots:
	virtual void accept ();

private slots:
	void onOk ();
	void onChangeView (const QString & viewName);
	void onChangePlatform (const QString & platformName);
	void onModified ();
	void onAddShown ();
	void onRemoveShown ();
	void onSessionSelectionChanged(QListViewItem* cur);
	void onViewNameComboChanged();

private:
	void setSessionDiffInfo();
	bool updateCurrentViewNames();
	void updateUi (QString viewName);
	void updateView (QString viewName);

private:
	DiffSession *m_pDiffSession;
	SESSION_DIFF_INFO_VEC *m_pSessInfoVec;
	ViewCollection *m_pViewCol;
	ViewCollection m_ViewCol;
	QString m_lastView;
	bool m_modified;
	unsigned long m_cpuFamily;
	unsigned long m_cpuModel;
	int m_curSessionIndex;
};
#endif //_VIEWMANAGEDLG_H_
