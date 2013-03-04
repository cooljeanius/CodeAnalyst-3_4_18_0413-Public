//$Id: sysdataview.cpp,v 1.5 2005/02/14 16:52:29 jyeh Exp $
// implementation for SystemDataView class

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

#include <qlabel.h>
#include <q3valuelist.h>
#include <qcombobox.h>
#include <qicon.h>
#include <qlayout.h>
#include <qlabel.h>
#include <QPixmap>
#include <Q3PopupMenu>

#include "stdafx.h"
#include "sysdataview.h"
#include "atuneoptions.h"
#include "EventMaskEncoding.h"
#include "helperAPI.h"

#define _SYSDATAVIEW_ICONS_
#include "xpm.h"
#undef  _SYSDATAVIEW_ICONS_

SystemItem::SystemItem (ViewShownData *pViewShown, int indexOffset, 
			Q3ListView * pParent)
			:DataListItem (pViewShown, indexOffset, pParent)
{
	m_pMod = NULL;
	m_taskId = 0;
}


SystemItem::SystemItem (ViewShownData *pViewShown, int indexOffset, 
			SystemItem * pParent)
			:DataListItem (pViewShown, indexOffset, pParent)
{
	m_pMod = NULL;
	m_taskId = 0;
}


QString SystemItem::key (int column, bool ascending) const
{
	Q_UNUSED (ascending);
	QString ret_string;

	if (SYS_MOD_NAME == column) {
		return text(column);
	}

	// for anything else, do integer comparison
	//20 characters is > the max length of a uint64 as a decimal.
	QString targetStr = text (column);
	targetStr.replace(QChar('.'),"");
	targetStr.replace(QChar('%'),"");

	ret_string.fill ('0', (20 - targetStr.length()));
	ret_string.append (targetStr);
	return ret_string;
}


SystemDataTab::SystemDataTab (CaProfileReader * pProfReader, ViewShownData *pViewShown, 
				  QWidget * parent, const char *name, Qt::WindowFlags wflags)
:  DataTab (pViewShown, parent, name, wflags )
{
	setDockMenuEnabled (false);
	m_userCanClose = false;
	m_exportString = "&Export System Data...";
	m_indexOffset = SYS_OFFSET_INDEX;
	m_pToolbar = NULL;
	m_pExpandCollapse = NULL;
	m_dataAggType = SYS_AGG_MOD_PROC;
	m_pidColWidth = 40;
	m_64ColWidth = 40;
		
	m_pProfReader	= pProfReader;
	m_pProfInfo	= m_pProfReader->getProfileInfo();
	m_pProcMap	= m_pProfReader->getProcessMap();
	m_pModMap	= m_pProfReader->getModuleMap();

	CA_SampleMap::iterator tit = m_pProfInfo->m_totalMap.begin();
	CA_SampleMap::iterator tend = m_pProfInfo->m_totalMap.end();
	for (; tit != tend; tit++) {
		SampleKey key(-1, tit->first.event);
		m_totalAgg.addSamples(key, tit->second);
	}
}




SystemDataTab::~SystemDataTab ()
{
}


QString SystemDataTab::getProcNameForPid(PidAggregatedSampleMap::const_iterator ait)
{
	wstring procName;
	PidProcessMap::const_iterator pit = m_pProcMap->find(ait->first);
	if (pit == m_pProcMap->end())
		procName = L"Unknown Process";
	else
		procName = pit->second.getPath();
	return wstrToQStr(procName);
}


bool SystemDataTab::displayModuleToProcess()
{

	m_pList->setColumnText (SYS_MOD_NAME, "Module -> Process");

	// For each module
	NameModuleMap::const_iterator mit  = m_pModMap->begin();
	NameModuleMap::const_iterator mend = m_pModMap->end();
	for(; mit != mend; mit++) {
		SystemItem * pModItem = new SystemItem(m_pViewShown, SYS_OFFSET_INDEX, m_pList);
		RETURN_FALSE_IF_NULL (pModItem, this);

		// Set Module Name
		pModItem->setText (SYS_MOD_NAME, wstrToQStr(mit->first));

		// Set SixtyFourBit
		if (!mit->second.m_is32Bit)
			pModItem->setChecked (SYS_64_BIT);
		else
			pModItem->setChecked (-1);

		// Set Module
		pModItem->m_pMod = &(mit->second);
		pModItem->m_taskId = SHOW_ALL_TASKS;

		AggregatedSample modAgg;

		// For each Pid
		PidAggregatedSampleMap::const_iterator ait  = mit->second.getBeginSample();
		PidAggregatedSampleMap::const_iterator aend = mit->second.getEndSample();
		for(; ait != aend; ait++)  {
			SystemItem * pProcItem = new SystemItem(m_pViewShown, SYS_OFFSET_INDEX, pModItem);
			RETURN_FALSE_IF_NULL (pProcItem, this);

			// Set Process Name
			pProcItem->setText (SYS_MOD_NAME, getProcNameForPid(ait) 
				+ " (" 
				+ QString::number(ait->first)
				+ ")");

			// Set Module
			pProcItem->m_pMod = &(mit->second);
			pProcItem->m_taskId = ait->first;

			// Set Process Data
			pProcItem->setTotal(&m_totalAgg);
			pProcItem->drawData(&(ait->second), true);
			pProcItem->updateShown ();

			pProcItem->setBackgroundColor(Qt::lightGray);

			modAgg.addSamples(&(ait->second));
		}
	
		// Set Module Data	
		pModItem->setTotal(&m_totalAgg);
		pModItem->drawData(&(modAgg), true);
		pModItem->updateShown ();
	}

	return TRUE;
}

/*
bool SystemDataTab::displayProcessToModule()
{

	m_pList->setColumnText (SYS_MOD_NAME, "Process -> Module");

	// For each module
	PidProcessMap::const_iterator pit  = m_pProcMap->begin();
	PidProcessMap::const_iterator pend = m_pProcMap->end();
	for(; pit != pend; pit++) {
		SystemItem * pProcItem = new SystemItem(m_pViewShown, SYS_OFFSET_INDEX, m_pList);
		RETURN_FALSE_IF_NULL (pProcItem, this);

		// Set Process Name
		pProcItem->setText (SYS_MOD_NAME, wstrToQStr(pit->second.getPath()));

		// Set Process PID
//		pProcItem->setText (SYS_PID, QString::number(pit->first));

		// Set SixtyFourBit
		if (!pit->second.m_is32Bit)
			pProcItem->setChecked (SYS_64_BIT);
		else
			pProcItem->setChecked (-1);

		pProcItem->m_taskId = pit->first;

		AggregatedSample procAgg;

		// For each module
		NameModuleMap::const_iterator mit  = m_pModMap->begin();
		NameModuleMap::const_iterator mend = m_pModMap->end();
		for(; mit != mend; mit++) {
			// Find PID 
			PidAggregatedSampleMap::const_iterator ait = mit->second.findSampleForPid(pit->first);
			if(ait == mit->second.getEndSample())
				continue;

			SystemItem * pModItem = new SystemItem(m_pViewShown, SYS_OFFSET_INDEX, pProcItem);
			RETURN_FALSE_IF_NULL (pModItem, this);

			// Set Module Name
			pModItem->setText (SYS_MOD_NAME, wstrToQStr(mit->first));
			
			// Set Module
			if (mit->first == pit->second.getPath())
				pProcItem->m_pMod = &(mit->second);
			pModItem->m_pMod = &(mit->second);
			pModItem->m_taskId = ait->first;

			// Set Module Data
			pModItem->setTotal(&m_totalAgg);
			pModItem->drawData(&(ait->second), true);
			pModItem->updateShown ();

			pModItem->setBackgroundColor(Qt::lightGray);

			procAgg.addSamples(&(ait->second));
		}
	
		// Set Process Data	
		pProcItem->setTotal(&m_totalAgg);
		pProcItem->drawData(&(procAgg), true);
		pProcItem->updateShown ();
	}

	return TRUE;
}
*/


bool SystemDataTab::displayProcessToPidToModule()
{

	m_pList->setColumnText (SYS_MOD_NAME, "Process -> Pid -> Module");

	m_nameProgItemMap.clear();
	
	PidProcessMap::const_iterator pit  = m_pProcMap->begin();
	PidProcessMap::const_iterator pend = m_pProcMap->end();
	for(; pit != pend; pit++) {
		SystemItem * pProgItem = NULL;
		AggregatedSample * pProgAgg  = NULL;

		NameProgramItemMap::iterator nit = m_nameProgItemMap.find(pit->second.getPath());
		if (nit == m_nameProgItemMap.end()) {
			ProgramItem progItem;
			
			progItem.pProg = new SystemItem(m_pViewShown, SYS_OFFSET_INDEX, m_pList);
			RETURN_FALSE_IF_NULL (progItem.pProg, this);
		
			m_nameProgItemMap.insert(
				NameProgramItemMap::value_type(pit->second.getPath(), progItem));	
		
			// Refind
			nit = m_nameProgItemMap.find(pit->second.getPath());
			if (nit == m_nameProgItemMap.end())
				return false;

			pProgItem = nit->second.pProg;
			pProgAgg  = &(nit->second.progAgg);
			
			// Set Process Name
			pProgItem->setText (SYS_MOD_NAME, wstrToQStr(pit->second.getPath()));

			// Set SixtyFourBit
			if (!pit->second.m_is32Bit)
				pProgItem->setChecked (SYS_64_BIT);
			else
				pProgItem->setChecked (-1);

			pProgAgg->clear();

		} else {
			pProgItem = nit->second.pProg;
			pProgAgg  = &(nit->second.progAgg);
		}

		SystemItem * pPidItem = new SystemItem(m_pViewShown, SYS_OFFSET_INDEX, pProgItem);
		RETURN_FALSE_IF_NULL (pPidItem, this);

		// Set Process Name
		pPidItem->setText (SYS_MOD_NAME, QString("PID : ") + QString::number(pit->first));

		// Set SixtyFourBit
		if (!pit->second.m_is32Bit)
			pPidItem->setChecked (SYS_64_BIT);
		else
			pPidItem->setChecked (-1);

		pPidItem->m_taskId = pit->first;

		AggregatedSample pidAgg;

		// For each module
		NameModuleMap::const_iterator mit  = m_pModMap->begin();
		NameModuleMap::const_iterator mend = m_pModMap->end();
		for(; mit != mend; mit++) {
			// Find PID 
			PidAggregatedSampleMap::const_iterator ait = mit->second.findSampleForPid(pit->first);
			if(ait == mit->second.getEndSample())
				continue;

			SystemItem * pModItem = new SystemItem(m_pViewShown, SYS_OFFSET_INDEX, pPidItem);
			RETURN_FALSE_IF_NULL (pModItem, this);

			// Set Module Name
			pModItem->setText (SYS_MOD_NAME, wstrToQStr(mit->first));
			
			// Set Module for the process item
			if (mit->first == pit->second.getPath())
				pProgItem->m_pMod = &(mit->second);
			
			pModItem->m_pMod = &(mit->second);
			pModItem->m_taskId = ait->first;

			// Set Module Data
			pModItem->setTotal(&m_totalAgg);
			pModItem->drawData(&(ait->second), true);
			pModItem->updateShown ();

			pModItem->setBackgroundColor(Qt::lightGray);

			pidAgg.addSamples(&(ait->second));
		} // for each module
	
		// Set Pid Data	
		pPidItem->setTotal(&m_totalAgg);
		pPidItem->drawData(&(pidAgg), true);
		pPidItem->updateShown ();

		// Set Program Data
		pProgAgg->addSamples(&(pidAgg));
		pProgItem->setTotal(&m_totalAgg);
		pProgItem->drawData(pProgAgg, true);
		pProgItem->updateShown ();
		
	} // for each pid

	return TRUE;
}

void SystemDataTab::onViewChanged (ViewShownData* pShownData)
{
	if (!isDisplayReady())
		return;

	QMutexLocker locker(&m_displayMutex);

	CATuneOptions ao;
	ao.getPrecision ( &m_precision );

	if(pShownData != NULL)
	{
		m_pViewShown = pShownData;
	}

	m_pList->clear();

	//remove all columns
	clearShownColumns ();

// NOTE: [Suravee] 
//      In Q3ListView,this has caused the view to not
//      update properly.  So comment out for now.
//	m_pList->setUpdatesEnabled (false);

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

	//add data columns
	if (!initMenu ())
		return;

	if (NULL != m_menu)
	{	
		if (m_menu->idAt (SYS_POP_SHOWN) == -1)
		{
			m_menu->insertItem ("&Show", m_pColumnMenu);
		}
	}

	switch (m_dataAggType) {
	case SYS_AGG_PROC_PID_MOD:
		displayProcessToPidToModule();
		break;
	case SYS_AGG_MOD_PROC:
	default:
		displayModuleToProcess();
		break;
	}

	onExpandCollapseToggled(m_pExpandCollapse->isOn());
	on64Toggled(m_p64->isOn());

//	m_pList->setUpdatesEnabled (true);
	m_pList->setCurrentItem (m_pList->firstChild());
	m_pList->ensureItemVisible(m_pList->firstChild());
	m_pList->triggerUpdate ();

} //SystemDataTab::onViewChanged


// The list view has been double clicked
// We must now get the module name that was clicked, and pass that to the
// function data view window.
//
void SystemDataTab::onDblClicked (Q3ListViewItem * item)
{
	SystemItem *pItem = (SystemItem *) item;
	if (!pItem || !(pItem->m_pMod))
		return;

	QApplication::setOverrideCursor (QCursor (Qt::WaitCursor));
	emit moduleDblClicked (pItem->m_pMod, TAB_DATA, pItem->m_taskId);
	QApplication::restoreOverrideCursor ();
}


// FIXME [3.1] Redesign new graph view
// We must create a new GraphView object
// The moduleName parameter will be "" since we want to display module data
//
//void SystemDataTab::onViewSystemGraph ()
//{
//	emit viewGraph ();
//}
//
//
//void SystemDataTab::onViewModuleGraph ()
//{
//	if (m_pItemRClick->m_pMod == NULL)
//		return;
//	
//	emit moduleRightClicked (m_pItemRClick->text (MF_MOD_NAME),
//		m_pItemRClick->m_pMod, TAB_GRAPH, m_pItemRClick->m_taskId);
//}


void SystemDataTab::onViewModuleData ()
{
	if (m_pItemRClick->m_pMod == NULL)
		return;
	
	emit moduleRightClicked (m_pItemRClick->m_pMod, TAB_DATA, m_pItemRClick->m_taskId);
}


void SystemDataTab::onViewSystemTask()
{
	emit viewTasks();
}

void SystemDataTab::onRightClick (Q3ListViewItem * item, const QPoint & pt,
								  int col)
{
	Q_UNUSED (col);
	m_pItemRClick = (SystemItem *)item;

	m_menu->removeItem (m_module_popup_id[POPUP_SEP]);
	m_menu->removeItem (m_module_popup_id[POPUP_DATA]);
//	m_menu->removeItem (m_module_popup_id[POPUP_GRAPH]);

	if (m_pItemRClick) {
		m_module_popup_id[POPUP_SEP] = m_menu->insertSeparator ();
		m_module_popup_id[POPUP_DATA] =
			m_menu->insertItem (item->text (MF_MOD_NAME) + " - Data", this,
			SLOT (onViewModuleData ()));
//		m_module_popup_id[POPUP_GRAPH] =
//			m_menu->insertItem (item->text (MF_MOD_NAME) + " - Graph", this,
//			SLOT (onViewModuleGraph ()));
		if (item->text (MF_MOD_NAME).startsWith ("anon (tgid"))
		{
			m_menu->setItemEnabled (m_module_popup_id[POPUP_DATA], false);
//			m_menu->setItemEnabled (m_module_popup_id[POPUP_GRAPH], false);
		}
	}

	m_menu->popup (pt);
}


bool SystemDataTab::display (QString caption)
{
	// Prepare the popup menu
	if(!m_menu)
	{
		delete m_menu;
		m_menu = NULL;
	}

	m_menu = new Q3PopupMenu (this, "Our Menu");
	if (NULL == m_menu) {
		QMessageBox::critical (this, "Memory Error", "Insufficient memory.");
		return FALSE;
	}

	m_menu->insertItem ("Copy Selection", this, SLOT (onCopySelection()), Qt::CTRL + Qt::Key_C);
	int id = m_menu->insertItem ("System Data");
	m_menu->setItemEnabled (id, false);
	m_menu->setItemChecked (id, true);

//	m_menu->insertItem ("System Graph", this, SLOT (onViewSystemGraph ()));

	// Initialize the list view 
	m_pList = new Q3ListView (this);
	if (NULL == m_pList) {
		QMessageBox::critical (this, "Memory Error", "Insufficient memory.");
		return FALSE;
	}

	m_pList->addColumn("");
	m_pList->addColumn("64-bit", m_64ColWidth);
	m_pList->setSelectionMode (Q3ListView::Extended);

	m_pList->setColumnAlignment(MF_MOD_NAME, Qt::AlignLeft);

	QObject::connect (m_pList, SIGNAL (doubleClicked (Q3ListViewItem *)),
		SLOT (onDblClicked (Q3ListViewItem *)));
	QObject::connect (m_pList, SIGNAL (rightButtonClicked (Q3ListViewItem *,
		const QPoint &, 
		int)),
		SLOT (onRightClick (Q3ListViewItem *, const QPoint &,
		int)));
	QObject::connect (m_pList, SIGNAL (contextMenuRequested (Q3ListViewItem *,
		const QPoint &, 
		int)),
		SLOT (onRightClick (Q3ListViewItem *, const QPoint &,
		int)));

	setFocusProxy (m_pList);
	setCentralWidget (m_pList);

	// Other options for the list view
	m_pList->setShowSortIndicator (TRUE);
	m_pList->setSorting (SYS_OFFSET_INDEX, FALSE);
	m_pList->setAllColumnsShowFocus (TRUE);
	m_pList->setRootIsDecorated(TRUE);

	setCaption (caption);
	setupAggregationToolbar();
	setupSystemToolbar();

	onViewChanged(m_pViewShown);

	return TRUE;
}// SystemDataTab::display()


void SystemDataTab::setupSystemToolbar()
{
	m_pToolbar = new Q3ToolBar( this, "System Data View Toolbar" );
	RETURN_IF_NULL (m_pToolbar, this);
	m_pToolbar->setLabel ("System Data View Toolbar" );
	m_pToolbar->setMovingEnabled (true);

	/* Expand / Collapse */
	QIcon iconExpandCollapse(QPixmap((const char**)expandCollapseIcon));
	m_pExpandCollapse = new QPushButton(iconExpandCollapse, "",m_pToolbar);
	m_pExpandCollapse->setMaximumHeight(20);
	m_pExpandCollapse->setMaximumWidth(20);
	m_pExpandCollapse->setToggleButton(true);
	connect(m_pExpandCollapse,SIGNAL(toggled(bool)), 
		this,SLOT(onExpandCollapseToggled(bool)));
	QToolTip::add(m_pExpandCollapse,QString("Expand / Collapse"));

	/* 64-bit */
	QIcon icon64(QPixmap((const char**)sixFourIcon));
	m_p64 = new QPushButton(icon64, "",m_pToolbar);
	m_p64->setMaximumHeight(20);
	m_p64->setMaximumWidth(20);
	m_p64->setToggleButton(true);
	connect(m_p64,SIGNAL(toggled(bool)), 
		this,SLOT(on64Toggled(bool)));
	QToolTip::add(m_p64,QString("Show 64-bit Column"));

/* FIXME [3.1] 
 * Hack for BUG279959:Modules shown as 64-bit on a 32-bit OS
 * The hack is to hide the "Show 64-bit Column" button on 32-bit OS
 * since this is not useful on the OS anyways.
 */
#if defined(__i386__)
	m_p64->hide();
#endif
}


void SystemDataTab::onExpandCollapseToggled(bool b)
{
	Q3ListViewItem *pLine = m_pList->firstChild();
	while(pLine != NULL)
	{
		m_pList->setOpen(pLine,b);
		pLine = pLine->nextSibling();
	} 
	
	pLine = m_pList->currentItem();
	if(pLine)
	{
		m_pList->ensureItemVisible(pLine);
	}
}


void SystemDataTab::on64Toggled(bool b)
{
	if (b) {
		m_pList->setColumnWidth(SYS_64_BIT, m_64ColWidth);
		m_pList->header()->setResizeEnabled(true, SYS_64_BIT);
	} else {
		UINT tmp =  m_pList->columnWidth(SYS_64_BIT);
		if (tmp > 0)
			m_64ColWidth = tmp;
		m_pList->setColumnWidth(SYS_64_BIT, 0);
		m_pList->header()->setResizeEnabled(false, SYS_64_BIT);
	}
}


void SystemDataTab::setupAggregationToolbar()
{
	m_pAggregationToolbar = new Q3ToolBar( this, "Aggregation Toolbar" );
	RETURN_IF_NULL (m_pAggregationToolbar, this);
	
	m_pAggregationToolbar->setLabel ("Aggregation Toolbar" );

	// Add Label
	QLabel * pLabel= new QLabel(QString("Aggregate by : "), m_pAggregationToolbar);
	RETURN_IF_NULL (pLabel, this);
	
	// Add Combobox
	m_pAggregationId = new QComboBox (m_pAggregationToolbar);
	RETURN_IF_NULL (m_pAggregationId, this);
	
	m_pAggregationId->insertItem ("Modules", SYS_AGG_MOD_PROC);
	m_pAggregationId->insertItem ("Processes", SYS_AGG_PROC_PID_MOD);

	connect (m_pAggregationId, SIGNAL (activated (int)), SLOT (onAggregationChanged(int)));
}


void SystemDataTab::onAggregationChanged(int aggregationType)
{
	/* Conversion from index to aggregation type */
	m_dataAggType = aggregationType;
	
	onViewChanged (NULL);
}
