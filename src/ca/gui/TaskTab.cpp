//$Id$

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
#include "stdafx.h"
#include "TaskTab.h"
#include "atuneoptions.h"
#include "EventMaskEncoding.h"
#include "dataagg.h"

#include "sysdataview.h" // For SystemItem
#include <Q3ValueList>
#include <Q3PopupMenu>

TaskItem::TaskItem (ViewShownData *pViewShown, int indexOffset, 
		Q3ListView *pParent)
		:DataListItem (pViewShown, indexOffset, pParent)
{
	m_hasCss = false;
}

//let's us alter the color of text, like results with css data
void TaskItem::paintCell (QPainter *p, const QColorGroup & cg, int column,
						  int width, int align)
{
	QColorGroup cg1 (cg);
	if (TSK_NAME == column)
	{
		if (m_hasCss)
		{
			cg1.setColor( QColorGroup::Text, Qt::darkGreen );
		}
	}	
	DataListItem::paintCell (p, cg1, column, width, align);
}


QString TaskItem::key (int column, bool ascending) const
{
	Q_UNUSED (ascending);

	QString ret_string;

	if (TSK_NAME == column)
		return text(column);

	// for anything else, do integer comparison
	//20 characters is > the max length of a uint64 as a decimal.
	QString targetStr = text(column);
	targetStr.replace(QChar('.'),"");
	targetStr.replace(QChar('%'),"");
	ret_string.fill ('0', (20 - targetStr.length()));
	ret_string.append (targetStr);
	return ret_string;
}

TaskTab::TaskTab (TbsReader* tbp_file, 
				  ViewShownData *pViewShown, 
				  QWidget * parent, 
				  const char *name, 
				  int wflags,
				  unsigned int taskId)
				  :  DataTab (pViewShown, parent, name, wflags )
{
	m_pTbpFile = tbp_file;
	setDockMenuEnabled (false);
	m_userCanClose = false;
	m_exportString = "&Export Task Data...";
	m_indexOffset = TSK_OFFSET_INDEX;
	m_pMenu = NULL;
	m_pToolbar = NULL;
	m_pCssButton = NULL;
	m_pList = NULL;
	m_taskId = taskId;
}


TaskTab::~TaskTab ()
{
}

bool TaskTab::display (QString caption, unsigned int taskId)
{
	bool ret_value;
	if (SHOW_ALL_TASKS == taskId) 
	{
		if(!m_pMenu)
		{
			delete m_pMenu;
			m_pMenu = NULL;
		}

		// Prepare the popup menu
		m_pMenu = new Q3PopupMenu( this, "Our Menu" );
		RETURN_FALSE_IF_NULL(m_pMenu, this);

		//keep the items in the same order as the sys data menu
		m_pMenu->insertItem ("Copy Selection", this, 
						SLOT (onCopySelection()), Qt::CTRL + Qt::Key_C);
		m_pMenu->insertItem( "System Data", this, SLOT (onViewSystemData()));
		m_pMenu->insertItem( "System Graph", this, 
						SLOT( onViewSystemGraph() ) );
		int id = m_pMenu->insertItem ("System Tasks");
		m_pMenu->setItemEnabled( id, false);
		m_pMenu->setItemChecked( id, true);

		//commenting out css stuff until needed
		/*
		m_pMenu->insertItem (QIconSet (QPixmap (cssIcon)), 
				"Call-stack data", this,
		SLOT (onCssShow ()));

		m_pToolbar = new QToolBar( this, "View Toolbar" );
		RETURN_FALSE_IF_NULL (m_pToolbar, this);
		m_pToolbar->setLabel ("View Toolbar" );

		m_pCssButton = new QToolButton (QIconSet (QPixmap (cssIcon)), 
		"Call-stack data", QString::null, this, SLOT (OnCssShow ()), m_pToolbar);
		RETURN_FALSE_IF_NULL (m_pCssButton, this);
		m_pCssButton->setEnabled (false);
		*/
	}
	else
	{
		// For one task view
		m_userCanClose = true;
	}

	// Initialize the list view 
	m_pList = new Q3ListView( this );
	RETURN_FALSE_IF_NULL(m_pList, this);	

	m_pList->addColumn( "Task Name" );
	m_pList->addColumn( "64-bit" );

	m_pList->setColumnAlignment( TSK_NAME, AlignLeft  );
	m_pList->setSelectionMode (Q3ListView::Extended);

	//add data columns
	if (!initMenu ())
		return false;

	if (SHOW_ALL_TASKS == taskId && m_pMenu != NULL) 
	{
		if (m_pMenu->idAt (TSK_POP_SHOWN) == -1)
		{
			m_pMenu->insertItem ("&Show", m_pColumnMenu);
		}
	}

	QObject::connect( m_pList, SIGNAL( doubleClicked( Q3ListViewItem* )  ), 
		SLOT( onDblClicked( Q3ListViewItem* ) ) );

	connect (m_pList, SIGNAL (selectionChanged (Q3ListViewItem *)),
		SLOT (onSelectionChanged (Q3ListViewItem *)));

	QObject::connect( m_pList, SIGNAL( rightButtonClicked ( Q3ListViewItem *, 
		const QPoint &, int ) ), SLOT( onRightClick( Q3ListViewItem *, 
		const QPoint &, int ) ) );

	setFocusProxy   ( m_pList );
	setCentralWidget( m_pList );

	m_taskId = taskId;
	if (SHOW_ALL_TASKS == taskId)
		ret_value = displayTaskData();
	else
		ret_value = displayModuleData (taskId);

	// Other options for the list view
	m_pList->setShowSortIndicator( true);
	m_pList->setSorting ( TSK_OFFSET_INDEX, FALSE );
	m_pList->setAllColumnsShowFocus( TRUE );

	setCaption (caption);

	return ret_value;
} //TaskTab::display

bool TaskTab::displayTaskData ()
{
	Q3ValueList<SYS_LV_ITEM> task_list;

	m_pTbpFile->readProcInfo (task_list);
	SampleDataMap totalSample;
	
	Q3ValueList<SYS_LV_ITEM>::iterator it = task_list.begin();
	Q3ValueList<SYS_LV_ITEM>::iterator end = task_list.end();
	
	for(; it != end; it++) {
		AggregateSamples(totalSample, (*it).CpuSamples);
	}
	m_TotalSampleDataMap.clear();
	SampleDataMap::iterator sit= totalSample.begin ();
	SampleDataMap::iterator send= totalSample.end ();
	for(; sit != send ; ++sit) {
		// We use -1 to denote "ALL CPU"
		SampleKey key(-1,sit->first.event);
                SampleDataMap::iterator tit  = m_TotalSampleDataMap.find(key);
                SampleDataMap::iterator tend  = m_TotalSampleDataMap.end();
                if( tit != tend)
                {
			tit->second += sit->second;
                }else{
			m_TotalSampleDataMap.insert(SampleDataMap::value_type(key,sit->second));
			
		}
	}

	it = task_list.begin();
	for(; it != end; it++) 
	{
		SampleDataMap::iterator sample;

		TaskItem *pItem = new TaskItem( m_pViewShown, 
						TSK_OFFSET_INDEX, m_pList);

		RETURN_FALSE_IF_NULL (pItem, this);

		//add column data
		QString name = (*it).ModName;

		pItem->m_taskId = (*it).taskId;

		pItem->setText (TSK_NAME, name);
		if ((*it).SixFourBit.toULong () > 0)
			pItem->setChecked (TSK_64_BIT);
		else
			pItem->setChecked (-1);

		// Data column
		pItem->setTotal(&m_TotalSampleDataMap);
		pItem->drawData(&(*it).CpuSamples, 200, false);
		pItem->updateShown ();
	}
	
	totalSample.clear();

	return true;
} //TaskTab::displayTaskData ()


bool TaskTab::displayModuleData (unsigned long taskId)
{

	Q3ValueList<SYS_LV_ITEM> mod_list;

	m_pTbpFile->readModInfo (mod_list);
	SampleDataMap totalSample;

	Q3ValueList<SYS_LV_ITEM>::iterator it = mod_list.begin();
	Q3ValueList<SYS_LV_ITEM>::iterator end = mod_list.end();

	for(; it != end; it++) {

		if ((*it).taskId != taskId)
			continue;
		AggregateSamples(totalSample, (*it).CpuSamples);
	}

	m_TotalSampleDataMap.clear();
	SampleDataMap::iterator sit= totalSample.begin ();
	SampleDataMap::iterator send = totalSample.end ();
	for(; sit != send; ++sit) {
		// We use -1 to denote "ALL CPU"
		SampleKey key(-1,sit->first.event);
                SampleDataMap::iterator tit  = m_TotalSampleDataMap.find(key);
                SampleDataMap::iterator tend  = m_TotalSampleDataMap.end();
                if( tit != tend)
                {
			tit->second += sit->second;
                }else{
			m_TotalSampleDataMap.insert(SampleDataMap::value_type(key,sit->second));
			
		}
	}

	it = mod_list.begin();

	for(; it != end; it++) 
	{
		if ((*it).taskId != taskId)
			continue;

		TaskItem *pItem = new TaskItem( m_pViewShown, TSK_OFFSET_INDEX, m_pList);
		RETURN_FALSE_IF_NULL (pItem, this);

		//add column data
		QString name = (*it).ModName;
		pItem->m_taskId = taskId;

		pItem->setText (TSK_NAME, name);
		if ((*it).SixFourBit.toULong () > 0)
			pItem->setChecked (TSK_64_BIT);
		else
			pItem->setChecked (-1);
		
		pItem->setTotal(&m_TotalSampleDataMap);
		pItem->drawData(&(*it).CpuSamples, 200, false);
		pItem->updateShown ();
	}
	return true;
} //TaskTab::displayModuleData (unsigned long taskId)

void TaskTab::onDblClicked (Q3ListViewItem *pItem)
{
	TaskItem * pTask = (TaskItem *)pItem;
	if (SHOW_ALL_TASKS == m_taskId) 
	{
		unsigned int taskId = pTask->m_taskId;
		QString name = pTask->text (TSK_NAME);
		if (SHOW_ALL_TASKS != taskId)
			emit taskDblClicked (taskId, name);
	} else
		emit moduleClicked ( pTask->text (TSK_NAME), TAB_DATA, m_taskId);
}

void TaskTab::onRightClick (Q3ListViewItem *pItem, const QPoint & pt, int col)
{
	Q_UNUSED (col);
	Q_UNUSED (pItem);
	//commenting out css stuff until needed
	/*
	TaskItem * pTask = (TaskItem * )pItem;
	bool allowCss = ((NULL != pItem) && (pTask->m_hasCss));
	if (NULL != m_pMenu)
	{
	m_pMenu->setItemEnabled (m_pMenu->idAt (TSK_POP_CSS), allowCss);
	m_pMenu->popup( pt );
	} else
	m_pColumnMenu->popup (pt);
	*/
	if (NULL != m_pMenu)
	{
		m_pMenu->popup( pt );
	} else{
		m_pColumnMenu->popup (pt);
	}

}

void TaskTab::onViewSystemGraph ()
{
	emit viewGraph ();
}

void TaskTab::onViewSystemData ()
{
	emit viewSysData ();
}

//Probably not used until css is implemented
void TaskTab::onCssShow ()
{
	TaskItem * pTask = (TaskItem * )m_pList->currentItem ();
	if ((NULL != pTask) && (pTask->m_hasCss))
	{
		emit showTaskCss (pTask->m_taskId);
	}
}

void TaskTab::onViewChanged (ViewShownData* pShownData)
{
	if (NULL == m_pList)
		return;
	m_pList->setUpdatesEnabled (false);

	CATuneOptions ao;
	ao.getPrecision ( &m_precision );

	if(pShownData != NULL)
	{
		m_pViewShown = pShownData;
	}

	m_pList->clear();

	//remove all columns
	clearShownColumns ();

	//update shown columns
	if (NULL != m_pColumnMenu)
	{
		m_pColumnMenu->clear();
	}

	if (NULL != m_pColumnIndex)
	{
		delete [] m_pColumnIndex;
		m_pColumnIndex = NULL;
	}

	if(!initMenu ())
		return;

	if (NULL != m_pMenu)
	{
		if(m_pMenu->idAt(TSK_POP_SHOWN) == -1)
		{
			m_pMenu->insertItem ("&Show", m_pColumnMenu);
		}
	}

	if (SHOW_ALL_TASKS == m_taskId)
		displayTaskData();
	else
		displayModuleData (m_taskId);

	m_pList->setUpdatesEnabled (true);
}


void TaskTab::onSelectionChanged (Q3ListViewItem *pItem)
{
	//useful once css is enabled
	TaskItem * pTask = (TaskItem * )pItem;
	if (NULL != m_pCssButton)
	{
		bool allowCss = ((NULL != pItem) && (pTask->m_hasCss));
		m_pCssButton->setEnabled (allowCss);
	}
}


