//$Id: sysdataview.h,v 1.2 2005/02/14 16:52:40 jyeh Exp $
// interface for SystemDataView class

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2007 Advanced Micro Devices, Inc.
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
//$Log: sysdataview.h,v $
//Revision 1.2  2005/02/14 16:52:40  jyeh
//Updated header.
//
//Revision 1.1.1.1  2004/10/05 17:03:23  jyeh
//initial import into CVS
//
//Revision 1.4.2.1  2004/08/13 20:16:31  jyeh
//Added Java Source line info.
//
//Revision 1.4  2003/12/15 23:39:39  franksw
//Code Review for TbsFile
//
//Revision 1.3  2003/12/12 23:04:00  franksw
//Code Review
//
//Revision 1.2  2003/11/13 20:39:06  franksw
//code cleanup
//

#ifndef _SYSDATAVIEW_H
#define	_SYSDATAVIEW_H

#include "stdafx.h"
#include "CaProfileReader.h"
#include "sessionnav.h"
#include <Q3PopupMenu>

enum SYSTEM_COLUMNS {
	SYS_MOD_NAME,
	SYS_64_BIT,
	SYS_OFFSET_INDEX,
	SYS_INVALID
};

enum SYSTEM_POPUPS {
	SYS_POP_COPY = 0,
	SYS_POP_DATA,
//	SYS_POP_GRAPH,
	SYS_POP_SHOWN
};

enum SYSTEM_AGGREGATION_TYPE {
	SYS_AGG_MOD_PROC,
	SYS_AGG_PROC_PID_MOD,
};

////////////////////////////////////////////////////////////////////////////
// Structure to hold list view data
//
enum SysDataViewEnum {
	MF_MOD_NAME = 0,
	MF_TOTAL,
	MF_PERC,
	MF_CPU0,
	POPUP_SEP = 0,
	POPUP_DATA,
//	POPUP_GRAPH
};


//////////////////////////////////////////////////////////////////////////////

class SystemItem : public DataListItem {
public:
	SystemItem ( ViewShownData *pViewShown, int indexOffset, 
			Q3ListView * pParent);

	SystemItem (ViewShownData *pViewShown, int indexOffset, 
			SystemItem * pParent);

	const CA_Module * m_pMod;
	unsigned int m_taskId;
protected:
	QString key( int column, bool ascending ) const;
};

//////////////////////////////////////////////////////////////////////////////

class SystemDataTab : public DataTab 
{
	Q_OBJECT
public:
	SystemDataTab (CaProfileReader * pProfReader, ViewShownData *pViewShown, 
		QWidget * parent, const char *name, Qt::WindowFlags wflags = 0);
	~SystemDataTab ();

	Q3ListView *getListView ();
	bool display (QString caption);

public slots: 
	void onDblClicked (Q3ListViewItem * item);
	void onRightClick (Q3ListViewItem * item, const QPoint & pt, int col);
//	void onViewSystemGraph ();
	void onViewSystemTask ();
//	void onViewModuleGraph ();
	void onViewModuleData ();
	virtual void onViewChanged (ViewShownData* pShownData);
	void onExpandCollapseToggled(bool b);
	void onAggregationChanged(int aggregationType);
	void on64Toggled(bool b);

signals: 
	void moduleDblClicked (const CA_Module *, TAB_TYPE type, 
					unsigned int taskId = SHOW_ALL_TASKS);
	void moduleRightClicked (const CA_Module *, TAB_TYPE type, 
					unsigned int taskId = SHOW_ALL_TASKS);
//	void viewGraph ();
	void viewTasks ();

private:
	enum {
		SHOW_ALL_PID = 0
	};


	bool displayModuleToProcess();
	bool displayProcessToModule();
	bool displayProcessToPidToModule();
	int m_module_popup_id[SYS_POP_SHOWN + 1];
	QString getProcNameForPid(PidAggregatedSampleMap::const_iterator ait);
	void setupAggregationToolbar();
	void setupSystemToolbar();

	Q3PopupMenu	*m_menu;
	CaProfileReader	*m_pProfReader;
	CaProfileInfo	*m_pProfInfo;
	SystemItem	*m_pItemRClick;
	PidProcessMap	*m_pProcMap;
	NameModuleMap 	*m_pModMap;
	
	// System Toolbar 
	Q3ToolBar 	*m_pToolbar;
	QPushButton 	*m_pExpandCollapse;
	QPushButton 	*m_pPid;
	UINT		m_pidColWidth;
	QPushButton 	*m_p64;
	UINT		m_64ColWidth;
	
	// System Aggregation Toolbar
	Q3ToolBar 	*m_pAggregationToolbar;
	QComboBox 	*m_pAggregationId;
	UINT		m_dataAggType;

	typedef struct _ProgramItem {
		SystemItem 	* pProg;
		AggregatedSample progAgg;
	} ProgramItem;

	typedef map<wstring, ProgramItem>	NameProgramItemMap;
	NameProgramItemMap	m_nameProgItemMap;
};

#endif // _SYSDATAVIEW_H
