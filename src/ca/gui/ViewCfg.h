//$Id: ViewCfg.h 20172 2011-08-23 11:08:33Z sjeganat $
//  This is our derivative of the iViewCfg.ui

/*
// CodeAnalyst for Open Source
// Copyright 2006 Advanced Micro Devices, Inc.
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

#ifndef VIEW_CFG_H
#define VIEW_CFG_H

#include <qstring.h>
#include <qcombobox.h>
#include "iViewCfg.h"
#include "ViewCollection.h"
#include <q3listbox.h> 

class ShownListBoxItem : public Q3ListBoxText
{
public:
	ShownListBoxItem ( Q3ListBox * listbox, const QString & text, int index);
	int m_index;
};

class ViewCfgDlg : public QDialog, public Ui::IViewDlg
{ 
	Q_OBJECT

public:
	ViewCfgDlg ( QWidget* parent = 0, 
		Qt::WindowFlags fl = 0);
	virtual ~ViewCfgDlg();

	void setViews (QString current, 
		unsigned long cpuFamily,
			unsigned long cpuModel,
		bool globalView,
		ViewCollection* pViews = NULL );
	void setCurrentViewNamePtr(QString* pStr) 
	{ 
		if(pStr) m_pLastView = pStr ; 
	};

private slots:
	void onApply ();
	void onOk ();
	void onChangeView (const QString & viewName);
	void onChangePlatform (const QString & platformName);
	void onModified ();
	void onAddShown ();
	void onRemoveShown ();
	void onResetCurrentView();

private:
	void updateUi (QString viewName);
	void updateView (QString viewName);

	ViewCollection *m_pViews;
	ViewCollection m_Views;
	QString *m_pLastView;
	QString m_lastView;
	bool m_modified;
	unsigned long m_cpuFamily;
	unsigned long m_cpuModel;
	bool m_globalView;
};
#endif
