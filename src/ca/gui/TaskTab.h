//$Id: TaskTab.h 12927 2007-07-17 19:32:30Z ssuthiku $

/*
// CodeAnalyst for Open Source
// Copyright 2007 Advanced Micro Devices, Inc.
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

#ifndef _TASK_TAB_H
#define	_TASK_TAB_H

#include "sessionnav.h"
#include "tbsreader.h"
#include <Q3PopupMenu>
enum TASK_COLUMNS {
	TSK_NAME = 0,
	TSK_64_BIT,
	TSK_OFFSET_INDEX,
	TSK_INVALID
};

enum TASK_POPUPS {
	TSK_POP_COPY = 0,
	TSK_POP_DATA,
	TSK_POP_GRAPH,
	TSK_POP_PRO,
	//TSK_POP_CSS,
	TSK_POP_SHOWN
};

class TaskItem : public DataListItem 
{
public: 
	unsigned int m_taskId;
	bool m_hasCss;
public:
	TaskItem (ViewShownData *pViewShown, int indexOffset, Q3ListView *pParent);

	//let's us alter the color of text, like results with css data
	virtual void paintCell (QPainter *p, const QColorGroup & cg, int column,
		int width, int align);
protected:
	QString key (int column, bool ascending) const;
};

class TaskTab : public DataTab 
{
	Q_OBJECT
public:
	TaskTab (TbsReader* tbp_file, ViewShownData *pViewShown, 
		QWidget * parent, const char *name, int wflags,
		unsigned int taskId = 0);
	~TaskTab ();

	bool display (QString caption, unsigned int taskId);

private:
	bool displayTaskData ();
	bool displayModuleData (unsigned long taskId);

	Q3PopupMenu *m_pMenu;
	Q3ToolBar *m_pToolbar;
	QToolButton *m_pCssButton;
	TbsReader *m_pTbpFile;
	Q3ListViewItem *m_pItemSelected;
	unsigned int m_taskId;

public slots:
	void onDblClicked (Q3ListViewItem *pItem);
	void onRightClick (Q3ListViewItem *pItem, const QPoint & pt, int col);
	void onViewSystemGraph ();
	void onViewSystemData ();
	void onCssShow ();
	virtual void onViewChanged (ViewShownData* pShownData);
	void onSelectionChanged (Q3ListViewItem *pItem);


signals:
	void moduleClicked (QString modName, TAB_TYPE type, unsigned int taskId);
	void taskDblClicked (unsigned int taskId, QString name);
	void showTaskCss (unsigned int taskId);
	void viewGraph ();
	void viewSysData ();
};

#endif //_TASK_TAB_H
