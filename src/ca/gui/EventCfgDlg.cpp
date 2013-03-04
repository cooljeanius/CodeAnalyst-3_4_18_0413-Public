//$Id: EventCfg.cpp 16863 2009-07-22 18:07:49Z ssuthiku $

/**
*   (c) 2006 Advanced Micro Devices, Inc.
*  YOUR USE OF THIS CODE IS SUBJECT TO THE TERMS
*  AND CONDITIONS OF THE GNU GENERAL PUBLIC
*  LICENSE FOUND IN THE "GPL.TXT" FILE THAT IS
*  INCLUDED WITH THIS FILE AND POSTED AT
*  <http://www.gnu.org/licenses/gpl.html>
*  Support: codeanalyst@amd.com
*/
#include <stdlib.h>
#include <qlayout.h> 
#include <q3textedit.h>
#include <qcombobox.h>
#include <QTextStream>
#include <stdafx.h>
#include "EventCfgDlg.h"
#include "iruncontrol.h"
#include "atuneoptions.h"
#include "helperAPI.h"
#include "EventMaskEncoding.h"
#include "ManageCfgs.h"

UnitCheckBox::UnitCheckBox (QWidget *parent, int index) 
: QCheckBox (parent)
{
	m_index = index;
	setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	connect (this, SIGNAL (clicked ()), SLOT (onChecked ())); 
}


UnitCheckBox::~UnitCheckBox ()
{
}


void UnitCheckBox::onChecked ()
{
	emit checked (m_index);
}


///////////////////////////////////////////////////////////////////////////////
EventCfgDlg::EventCfgDlg ( QWidget* parent,Qt::WindowFlags fl) 
: QDialog (parent, fl|CA_DLG_FLAGS)
{
	setupUi(this);
	m_modified 		= false;
	m_isViewProperty 	= false;
	m_pCaEventsFile		= NULL;
	m_pProfiles		= NULL;
	m_gFetchCount		= 0;
	m_gFetchUmask		= 0;
	m_gOpCount		= 0;
	m_gOpUmask		= 0;
	m_closeEventsFile 	= false;
} //EventCfgDlg::EventCfgDlg


EventCfgDlg::~EventCfgDlg ()
{
	if (m_closeEventsFile) {
		m_pCaEventsFile->close();
		delete m_pCaEventsFile;
		m_pCaEventsFile = NULL;
	}
}


EventWithProps * EventCfgDlg::addAvailableEvent(Q3ListView * lv, CpuEvent ev) 
{
	EventWithProps * pEv = new EventWithProps(lv,
					(EventWithProps *)lv->lastItem());
	if (!pEv)
		return pEv;

	pEv->setText(ColSel, QString().sprintf("0x%04x", ev.value));
	pEv->setText(ColSrc, ev.source);
	pEv->setText(ColAvailName, ev.name);
	pEv->opName = ev.op_name;
	return pEv;
}


bool EventCfgDlg::checkMaxPmcEvent()
{
	// Check if number of PMC events reach limit (32)
	if(m_pmcEventCount >= MAX_EVENTNUM_MULTIPLEXING ) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("The maximum number of PMC events (")
			+ QString::number(MAX_EVENTNUM_MULTIPLEXING) + ") is reached.");
		return false;
	}
	return true;
}



EventWithProps * EventCfgDlg::addSelectedEvent( QString name,
						QString opName,
						QString src,
						QString evSel, 
						unsigned long count, 
						unsigned int umask,
						QString usr,
						QString os,
						QString edge) 
{
	EventWithProps * pEv = new EventWithProps(m_pSelectedList, 
				(EventWithProps *)m_pSelectedList->lastItem());
	if (!pEv)
		return pEv;

	pEv->setText(ColSel, evSel);
	pEv->setText(ColSrc, src);
	pEv->setText(ColCount, QString::number(count));
	pEv->setText(ColUmask, QString().sprintf("0x%02x",umask));
	pEv->setText(ColUsr, usr);
	pEv->setText(ColOs, os);
//	pEv->setText(ColEdge, edge);
	pEv->setText(ColSelName, name);
	pEv->opName = opName;

	// Check if duplicate
	if(hasDuplicateEvent(pEv, pEv->text(ColUmask))) {
		delete pEv;
		return NULL;
	}

	// Keeping track of number of PMC events	
	if (!opName.startsWith("IBS_FETCH_") && !opName.startsWith("IBS_OP_")) {
		m_pmcEventCount++;
		onPmcCountChanged();
	}

	return pEv;
}


void EventCfgDlg::setProfile ( ProfileCollection *pProfiles,
				QString profileName,
				CEventsFile * pEventsFile)
{
	if (!pEventsFile) {
		//Check type of cpu from /proc/cpuinfo
		detectCpuAndGetEventsFile ();
		m_pCaEventsFile = new CEventsFile();
		if (!m_pCaEventsFile || !m_pCaEventsFile->open(m_eventsFilePath)) {
			QMessageBox::critical(this, "CodeAnalyst Error",
				QString("Could not open events file. (") +
				m_eventsFilePath + ").");
			return;
		}
		m_closeEventsFile = true;
	} else {
		// File must already be open
		m_pCaEventsFile = pEventsFile;
	}

	/////////////////////////////////////////////////////////////
	/* Set up configuration combo box */
	if (!pProfiles)
		return;

	m_pProfiles = pProfiles;
	QStringList list = m_pProfiles->getListOfEbpProfiles();
	int index = list.findIndex(profileName);

	m_pDcConfigs->insertStringList(list);
	if (index != 0)
		m_pDcConfigs->setCurrentItem(index);

	/////////////////////////////////////////////////////////////
	/* Set up Available Events Tabs */
	EventList::const_iterator pmc_it	= m_pCaEventsFile->FirstPmcEvent();
	EventList::const_iterator fetch_it	= m_pCaEventsFile->FirstIbsFetchEvent();
	EventList::const_iterator op_it		= m_pCaEventsFile->FirstIbsOpEvent();
	EventList::const_iterator end_it	= m_pCaEventsFile->EndOfEvents();
	
	/////////////////////////////////////////////////////////////
	/* Set up PMC Tab */
	m_pPmcList->setColumnWidthMode(ColSel, Q3ListView::Manual);
	m_pPmcList->setColumnWidth(ColSel, 50);
	m_pPmcList->setColumnWidthMode(ColSrc, Q3ListView::Manual);
	m_pPmcList->setColumnWidth(ColSrc, 50);
	m_pPmcList->setColumnAlignment(ColSrc, Qt::AlignHCenter);

	for (; pmc_it != fetch_it; pmc_it++) {
		addAvailableEvent(m_pPmcList, *pmc_it);	
	}

	/////////////////////////////////////////////////////////////
	
	/* NOTE: MUST Check Op before Fetch due to removing page */

	/////////////////////////////////////////////////////////////
	/* Set up IBS Op Tab */
	m_pOpList->setColumnWidthMode(ColSel, Q3ListView::Manual);
	m_pOpList->setColumnWidth(ColSel, 50);
	m_pOpList->setColumnWidthMode(ColSrc, Q3ListView::Manual);
	m_pOpList->setColumnWidth(ColSrc, 50);
	m_pOpList->setColumnAlignment(ColSrc, Qt::AlignHCenter);

	if (op_it != end_it && isDriverIbsOk()) {
		for (; op_it != end_it; op_it++) {
			addAvailableEvent(m_pOpList, *op_it);	
		}
	} else {
		m_pAvailableEventTab->removePage(m_pAvailableEventTab->page(PAGE_OP));
	}

	/////////////////////////////////////////////////////////////
	/* Set up IBS Fetch Tab */
	op_it = m_pCaEventsFile->FirstIbsOpEvent();
	m_pFetchList->setColumnWidthMode(ColSel, Q3ListView::Manual);
	m_pFetchList->setColumnWidth(ColSel, 50);
	m_pFetchList->setColumnWidthMode(ColSrc, Q3ListView::Manual);
	m_pFetchList->setColumnWidth(ColSrc, 50);
	m_pFetchList->setColumnAlignment(ColSrc, Qt::AlignHCenter);

	if (fetch_it != end_it && isDriverIbsOk()) {
		for (; fetch_it != op_it; fetch_it++) {
			addAvailableEvent(m_pFetchList, *fetch_it);	
		}
	} else {
		m_pAvailableEventTab->removePage(m_pAvailableEventTab->page(PAGE_FETCH));
	}

	/////////////////////////////////////////////////////////////
	/* Set up Import Tab */
	m_pImportDcConfigs->insertStringList(list);
	m_pImportDcConfigs->setCurrentItem(0);
	onImportDcConfigChanged(m_pImportDcConfigs->currentText());

	/////////////////////////////////////////////////////////////
	/* Set up Info Tab */
	setupInfoTab();

	/////////////////////////////////////////////////////////////
	/* Set up Selected Event List and Description */
	m_pSelectedList->setColumnWidthMode(ColSel, Q3ListView::Manual);
	m_pSelectedList->setColumnWidth(ColSel, 50);
	m_pSelectedList->setColumnWidthMode(ColSrc, Q3ListView::Manual);
	m_pSelectedList->setColumnWidth(ColSrc, 50);
	m_pSelectedList->setColumnAlignment(ColSrc, Qt::AlignHCenter);
	m_pSelectedList->setColumnWidthMode(ColCount, Q3ListView::Manual);
	m_pSelectedList->setColumnWidth(ColCount, 70);
	m_pSelectedList->setColumnAlignment(ColCount, Qt::AlignRight);
	m_pSelectedList->setColumnWidthMode(ColUmask, Q3ListView::Manual);
	m_pSelectedList->setColumnWidth(ColUmask, 50);
	m_pSelectedList->setColumnAlignment(ColUmask, Qt::AlignHCenter);
	m_pSelectedList->setColumnWidthMode(ColUsr, Q3ListView::Manual);
	m_pSelectedList->setColumnWidth(ColUsr, 30);
	m_pSelectedList->setColumnAlignment(ColUsr, Qt::AlignHCenter);
	m_pSelectedList->setColumnWidthMode(ColOs, Q3ListView::Manual);
	m_pSelectedList->setColumnWidth(ColOs, 30);
	m_pSelectedList->setColumnAlignment(ColOs, Qt::AlignHCenter);
//	m_pSelectedList->setColumnWidthMode(ColEdge, QListView::Manual);
//	m_pSelectedList->setColumnWidth(ColEdge, 35);
//	m_pSelectedList->setColumnAlignment(ColEdge, Qt::AlignHCenter);
	m_pSelectedList->setColumnWidthMode(ColSelName, Q3ListView::Manual);

	onDcConfigNameChanged(profileName);
	buildUmaskBoxes (m_pUmaskBox, m_pUmask);

	/////////////////////////////////////////////////////////////
	/* Set up connection */
	connect (m_pSelectedList, SIGNAL (selectionChanged(Q3ListViewItem *)), 
		this, SLOT (onSelectedSelectionChanged(Q3ListViewItem *)));
}


void EventCfgDlg::onRemove()
{
	EventWithProps * pEvent = (EventWithProps *) m_pSelectedList->selectedItem();
	if (!pEvent) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Please select event to remove."));
		return;
	}

	QString opName = pEvent->opName;
	if (!opName.startsWith("IBS_FETCH_") && !opName.startsWith("IBS_OP_")) {
		m_pmcEventCount--;
		onPmcCountChanged();
	}

	Q3ListViewItem * above = pEvent->itemAbove();
	Q3ListViewItem * below = pEvent->itemBelow();
	
	delete pEvent;

	m_pCount->clear();
	clearUmasks(m_pUmask);

	if (above)
		m_pSelectedList->setSelected(above, true);
	else if (below)
		m_pSelectedList->setSelected(below, true);

}


void EventCfgDlg::onAddPmcEvent()
{

	bool found = false;
	EventWithProps * pEvent = NULL;
	for (pEvent = (EventWithProps *) m_pPmcList->firstChild()
		; pEvent != NULL
		; pEvent = (EventWithProps *)pEvent->nextSibling()) 
	{
		if (m_pPmcList->isSelected(pEvent)) {
			found = true;
	
			if (!checkMaxPmcEvent())
				break;	

			EventWithProps * tmp = addSelectedEvent(pEvent->text(ColAvailName),
						pEvent->opName,
						pEvent->text(ColSrc),
						pEvent->text(ColSel),
						getDefaultCountForEvent(pEvent->opName),
						getDefaultUmask(pEvent->opName),
						"1",
						"1",
						"0");
			if (tmp) {
				m_pSelectedList->setCurrentItem(tmp);
				m_pSelectedList->setSelected(tmp,true);
			}
		}

	}

	if (!found) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("No event selected. Please select event to add.\n"));
		goto out;
	}
out:
	// Clear current setting
	m_pPmcList->clearSelection();
}


void EventCfgDlg::onAddFetchEvent()
{ 
	bool found = false;
	EventWithProps * pEvent = NULL;
	unsigned long count;
	unsigned int umask;

	if (0 != getCurrentIbsCountUmask(&count, &umask, PerfEvent::IbsFetch)) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Bad Event Configuration. Please make sure that all "
				"IBS Fetch events have same count and umask.\n"));
		return;
	}

	for (pEvent = (EventWithProps *) m_pFetchList->firstChild()
		; pEvent != NULL
		; pEvent = (EventWithProps *)pEvent->nextSibling()) 
	{
		if (m_pFetchList->isSelected(pEvent)) {
			found = true;

			if (count == 0)
				count = getDefaultCountForEvent(pEvent->opName);

			EventWithProps * tmp = addSelectedEvent(pEvent->text(ColAvailName),
					pEvent->opName,
					pEvent->text(ColSrc),
					pEvent->text(ColSel),
					count,	// Count
					umask, 	// umask
					"0",	// usr
					"0",	// os
					"0");	// edge
			if (tmp) {
				m_pSelectedList->setCurrentItem(tmp);
				m_pSelectedList->setSelected(tmp,true);
			}
		}
	}
	
	if (!found) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("No event selected. Please select event to add.\n"));
		return;
	}

	// Clear current setting
	m_pFetchList->clearSelection();
}


void EventCfgDlg::onAddOpEvent()
{ 
	bool found = false;
	EventWithProps * pEvent = NULL;
	unsigned long count;
	unsigned int umask;

	if (0 != getCurrentIbsCountUmask(&count, &umask, PerfEvent::IbsOp)) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Bad Event Configuration. Please make sure that all "
				"IBS Op events have same count and umask.\n"));
		return;
	}

	if (umask == 0)
		umask = (isDriverIbsOpDispatchOpOk())? 1: 0;

	for (pEvent = (EventWithProps *) m_pOpList->firstChild()
		; pEvent != NULL
		; pEvent = (EventWithProps *)pEvent->nextSibling()) 
	{
		if (m_pOpList->isSelected(pEvent)) {
			found = true;

			if (count == 0)
				count = getDefaultCountForEvent(pEvent->opName);

			EventWithProps * tmp = addSelectedEvent( pEvent->text(ColAvailName),
					pEvent->opName,
					pEvent->text(ColSrc),
					pEvent->text(ColSel),
					count,	// count
					umask,	// umask
					"0",	// usr
					"0",	// os
					"0");	// edge
			if (tmp) {
				m_pSelectedList->setCurrentItem(tmp);
				m_pSelectedList->setSelected(tmp,true);
			}
		}
	}
	
	if (!found) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("No event selected. Please select event to add.\n"));
		return;
	}

	// Clear current setting
	m_pOpList->clearSelection();
}


void EventCfgDlg::onAddImportEvent()
{ 
	EBP_OPTIONS ebpOpt;
	if (!m_pProfiles->getProfileConfig(m_pImportDcConfigs->currentText()
					, (SESSION_OPTIONS *) &ebpOpt )) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			m_pProfiles->getErrorMsg() + 
			QString("\nCannot add events from " + 
			m_pImportDcConfigs->currentText() + "."));
		return;
	}

	PerfEventList::iterator it = ebpOpt.getEventContainer()->getPerfEventListBegin();
	PerfEventList::iterator it_end = ebpOpt.getEventContainer()->getPerfEventListEnd();
	for (; it != it_end; it++) {
		CpuEvent ev;
		if (!(*it).opName.isEmpty()) {
			if (!m_pCaEventsFile->findEventByOpName((*it).opName, ev))
				return;
		} else {
			if (!m_pCaEventsFile->findEventByValue((*it).select(), ev))
				return;
		}

		if (PerfEvent::isPmcEvent((*it).select()) && !checkMaxPmcEvent())
			return;	

		addSelectedEvent(ev.name,
				ev.op_name, 
				ev.source, 
				QString().sprintf("0x%04x",ev.value),
				(*it).count,
				(*it).umask(),
				(*it).usr()? "1": "0",
				(*it).os()? "1": "0",
				(*it).edge()? "1": "0");
	}

	updateMuxInterval(ebpOpt.msMpxInterval);
}


void EventCfgDlg::onManageDcConfigs()
{ 
	QString curName = m_pDcConfigs->currentText();

	ManageCfgsDlg * pManageDlg = new ManageCfgsDlg (m_pProfiles, m_pCaEventsFile, this, false);
	RETURN_IF_NULL (pManageDlg, this);

	pManageDlg->exec();

	QStringList list = m_pProfiles->getListOfEbpProfiles();
	
	// Update list of DC configs
	m_pDcConfigs->clear();
	m_pDcConfigs->insertStringList (list);

	int index = list.findIndex(curName);

	if (index != -1) {
		m_pDcConfigs->setCurrentItem(index);
	}
	onDcConfigNameChanged(m_pDcConfigs->currentText());
	
	// Update list of Import DC configs
	m_pImportDcConfigs->clear();
	m_pImportDcConfigs->insertStringList (list);
	m_pImportDcConfigs->setCurrentItem(0);
	onImportDcConfigChanged(m_pImportDcConfigs->currentText());
}


void EventCfgDlg::onDcConfigNameChanged(const QString & profileName)
{
	EBP_OPTIONS ebpOpt;
	if (!m_pProfiles->getProfileConfig(profileName, (SESSION_OPTIONS *) &ebpOpt )) {
		QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
		return;
	}

	m_pSelectedList->clear();
	m_pmcEventCount = 0;
	onPmcCountChanged();

	PerfEventList::iterator it = ebpOpt.getEventContainer()->getPerfEventListBegin();
	PerfEventList::iterator it_end = ebpOpt.getEventContainer()->getPerfEventListEnd();
	for (; it != it_end; it++) {
		CpuEvent ev;
		if (!(*it).opName.isEmpty()) {
			if (!m_pCaEventsFile->findEventByOpName((*it).opName, ev))
				return;
		} else {
			if (!m_pCaEventsFile->findEventByValue((*it).select(), ev))
				return;
		}

		addSelectedEvent(ev.name,
				ev.op_name, 
				ev.source, 
				QString().sprintf("0x%04x",ev.value),
				(*it).count,
				(*it).umask(),
				(*it).usr()? "1": "0",
				(*it).os()? "1": "0",
				(*it).edge()? "1": "0");
	}
	
	/* Set up Description */
	QString toolTip;
	if (!m_pProfiles->getProfileTexts (profileName, &toolTip, &m_description))
		return;
	m_pDescription->setText(m_description);

	updateMuxInterval(ebpOpt.msMpxInterval);
}

void EventCfgDlg::updateMuxInterval(unsigned int ms)
{
	if (ms > 0) {
		m_pMuxInterval->setText(QString::number(ms));
	} else {
		// Use Default msMpxInterval
		m_pMuxInterval->setText(QString::number(DEFAULT_MUX_INTERVAL_MS));
	}
}

void EventCfgDlg::onOk()
{
	QString saveName;
	saveName = m_pDcConfigs->currentText();
	if (!onSave(saveName))
		return;
	accept();
}


void EventCfgDlg::onSaveAs ()
{
	QString newName;
	QString curName = m_pDcConfigs->currentText();

        while(1) {
                bool isOk = false;
                newName = QInputDialog::getText("Save As",
			"Enter new name for the Event Configuration",
                        QLineEdit::Normal,curName,&isOk,this );

                if (newName.isEmpty()) {
                        if( isOk)
                                QMessageBox::critical(this,"CodeAnalyst Error",
				"Please specify configuration name.\n");
                        else
                                return;
		} else if (newName.contains("/")) {
                        if( isOk)
                                QMessageBox::critical(this,"CodeAnalyst Error",
				"\"/\" is an invalid character for profile name.\n");
                } else {
                        break;
		}
        }

	if (!m_pProfiles)
		return;

	// Ask if overwrite an existing profile
	if (NO_TRIGGER != m_pProfiles->getProfileType (newName)
	&&  QMessageBox::question (this, "CodeAnalyst warning",
		"Event Configuration with name \""+ newName +"\" is already existed.\n"
		"Do you want to overwrite the Configuration?",
		QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
	{
		return;
	}

	if(!onSave(newName))
		return;	
	
	// Update DC Config Combobox
	m_pDcConfigs->clear();
	QStringList list = m_pProfiles->getListOfEbpProfiles();
	m_pDcConfigs->insertStringList(list);

	
	// Update the currently shown profile
	int index = list.findIndex(newName);
	if (index != 0)
		m_pDcConfigs->setCurrentItem(index);

} //EventCfgDlg::onSaveAs


void EventCfgDlg::onSave ()
{
	onSave(QString());
}


bool EventCfgDlg::onSave (QString saveName)
{
	/* NOTE: Here, we assume the saveName is valid */
	if (saveName.isEmpty())
		saveName = m_pDcConfigs->currentText();

	if (!m_pProfiles)
		return false;

	
	////////////////////////////////////////////// 
	// Check if list is empty
	if (m_pSelectedList->childCount() == 0) {
		QMessageBox::critical(this, "CodeAnalyst Error",
		QString("There is no events selected.\n") + 
		+ "Please specify at least one event.");
		return false;
	}

	////////////////////////////////////////////// 
	// Build up PerfEventContainer
	EBP_OPTIONS ebpOpt;

	PerfEventContainer * pContainer = ebpOpt.getEventContainer();
	if (!pContainer)
		return false;
			
	EventWithProps * cur  = (EventWithProps *) m_pSelectedList->firstChild();
	EventWithProps * last = (EventWithProps *) m_pSelectedList->lastItem();
	if (!cur)
		return false;

	// Clear existing value before verification
	m_gFetchCount		= 0;
	m_gFetchUmask		= 0;
	m_gOpCount		= 0;
	m_gOpUmask		= 0;

	do {
		/*
		 * Verify each event
		 */
		if (!verifySelectedEvent(cur, 
					cur->text(ColCount).toULong(0,10), 
					cur->text(ColUmask).toUInt(0,16)))
			return false;

		PerfEvent ev;
		ev.opName	= cur->opName;
		ev.name		= cur->text(ColSelName);
		ev.setSelect(cur->text(ColSel).toUInt(0,16));
		ev.count	= cur->text(ColCount).toULong(0,10);
		ev.setUmask(cur->text(ColUmask).toUInt(0,16));
		ev.setOs(cur->text(ColOs) == "1"? true: false);
		ev.setUsr(cur->text(ColUsr) == "1"? true: false);
//		ev.setEdge(cur->text(ColEdge) == "1"? true: false);

		switch (ev.type()) {
			case PerfEvent::IbsFetch:
			case PerfEvent::IbsOp:
				ev.setHost(true);
				ev.setGuest(true);
				break;
			default:
				// TODO: Suravee: Add check for non-amd events
				break;
		};

		if (!pContainer->add(ev)) {
			QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Failed to add\n")
			+ ("Event:") + cur->text(ColSelName) 
			+ " count:" + cur->text(ColCount) 
			+ " umask:" + cur->text(ColUmask) + "\n"
			+ " to DC Config " + saveName + ".\n");
			m_pSelectedList->setFocus();
			m_pSelectedList->setSelected(cur, true);
			return false;
		}

		if (cur == last)
			break;

		cur = (EventWithProps *)cur->nextSibling();
	} while (cur); 
	
	////////////////////////////////////////////// 
	//Validate event mapping 
	unsigned int availMask = 0;

	// Get available counters
	if (!(availMask = getCounterAvailMask())) {
		if (QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Cannot determine the currently available counters.\n\n")
			+ "Do you want to continue?", QMessageBox::Yes, QMessageBox::No) 
			!= QMessageBox::Yes)
		return false;
	}

	// Try to map events to counters
	QString errorStr;
	if (!pContainer->getCounterAllocation(availMask, &errorStr)) {
		QMessageBox::critical(this, "CodeAnalyst Error", errorStr) ;
		m_pSelectedList->setFocus();
		return false;
	}	

	////////////////////////////////////////////// 
	// Save MUX information
	if (m_pMuxInterval->isEnabled()) {
		ebpOpt.msMpxInterval = m_pMuxInterval->text().toUInt(0,10);
		if (ebpOpt.msMpxInterval == 0) {
			QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Invalid multiplexing interval.\n"));
			m_pMuxInterval->setFocus();
			return false;
		}
	} else  {
		ebpOpt.msMpxInterval = 0;
	}

	////////////////////////////////////////////// 
	// If no change in the event setting, skip saving but set for last modified
	// so that the Session Setting Dialog can pick up the most recent one to use.
	EBP_OPTIONS tmpSession;
	if (m_pProfiles->getProfileConfig(saveName, (SESSION_OPTIONS *) &tmpSession)) {

		if (*pContainer == *(tmpSession.getEventContainer())
		&&   ebpOpt.msMpxInterval == tmpSession.msMpxInterval
		&&   m_pDescription->text() == m_description) {
			m_pProfiles->setLastModified(saveName);
			goto out;
		}
	} 

	////////////////////////////////////////////// 
	// Saving the current DCConfig
	if (!m_pProfiles->setProfileConfig (saveName, &ebpOpt, "", m_pDescription->text())) {
		//QMessageBox::critical(this, "CodeAnalyst Error",
		//	QString("Failed to add DC Config " + saveName + ".\n"));
		QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
		return false; 
	}
	
out:
	return true;
} //EventCfgDlg::onSave


int EventCfgDlg::getCpuInfo(const char* path, 
				QString &vendorId, 
				QString &name, 
				QString &family, 
				QString &model, 
				QString &stepping,
				QString &flags
				)
{
	QFile * file = new QFile (path);
	if (NULL == file) 
		return -1;

	QTextStream stream(file);
	stream.setDevice (file);
	if (!file->open(QIODevice::ReadOnly)) 
		return -1;

		QString line;	

		do {
			line = stream.readLine();	

			if (line.contains("vendor_id")) 
			{
				vendorId = line.section(": ",1,1);
			}
			else if (line.contains("cpu family")) 
			{
				family = line.section(": ",1,1);
			}
			else if (line.contains("model name")) 
			{
				name = line.section(": ",1,1);
			}
			else if (line.contains("model")) 
			{
				model = line.section(": ",1,1);
			}
			else if (line.contains("stepping")) 
			{
				stepping = line.section(": ",1,1);
			}
			else if (line.contains("flags")) 
			{
				flags = line.section(": ",1,1);
				break;
			}
		} while (!line.isNull());	

	return 0;
}


bool EventCfgDlg::detectCpuAndGetEventsFile()
{
	// Detect From cpuinfo
	if(getCpuInfo("/proc/cpuinfo", m_cpuVendorId, 
			m_cpuName,
			m_cpuFamily,
			m_cpuModel,
			m_cpuStepping,
			m_cpuFlags) != 0)
		return false;

	// Reading Events file
	if (m_cpuVendorId == QString("AuthenticAMD")) {
		m_eventsFilePath = helpGetEventFile(m_cpuFamily.toULong() , 
							m_cpuModel.toULong());
	} else  {
		// TODO: Suravee: This could be enhanced to load driver if not already loaded.
		// Note: require Driver to be loaded.
		m_eventsFilePath = helpGetEventFile(getCurrentCpuType());
	}

	return true;	
}


void EventCfgDlg::buildUmaskBoxes (Q3GroupBox * pBox, UnitCheckBox ** pUmasks)
{
	if (!pBox)
		return;

	pBox->setColumnLayout(8,Qt::Vertical);

	for (unsigned int j = 0; j < UNIT_MASK_COUNT; j++) {
		pUmasks[j] = new UnitCheckBox( pBox, j);
		RETURN_IF_NULL (pUmasks[j], this);
		pUmasks[j]->show();
	}
	
	clearUmasks(pUmasks);
}


bool EventCfgDlg::verifySelectedEvent(EventWithProps * pEvent, 
					unsigned long count,
					unsigned int umask)
{
	/*
	 * Check if count is not 0
	 */
	if (count == 0) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Invalid event count."));
		m_pCount->setFocus();
		return false;
	}
	
	/*
	 * Check duplicate
	 */
	if(hasDuplicateEvent(pEvent, QString().sprintf("0x%02x", umask)))
		return false;

	if (PerfEvent::isIbsFetchEvent(pEvent->text(ColSel).toUInt(NULL,16))) {
		if (!verifyIbsFetchEvents(pEvent, count, umask)) {
			return false;
		}

	} else if (PerfEvent::isIbsOpEvent(pEvent->text(ColSel).toUInt(NULL,16))) {
		if (!verifyIbsOpEvents(pEvent, count, umask)) {
			return false;
		}
	} else {
		if (!verifyPmcEvents(pEvent, count, umask)) {
			return false;
		}
	}

	return true;
}


unsigned long EventCfgDlg::getDefaultCountForEvent(QString opName)
{
	/* NOTE:
	 * We define default value to be 10 * MinValue.
	 * Otherwise, set to 50000.
	 */
	unsigned long val = getMinCountForEvent(opName);
	if (val > 0)	
		return (val * 10);
	else
		return 50000;
}


unsigned long EventCfgDlg::getMinCountForEvent(QString opName)
{
	unsigned long countMin = 0;
	QString line;
	bool isOk = false;

	QString path = QString(OP_DATADIR) + "/" + getCurrentCpuType() + "/events";
	QFile eventFile(path);

	if(!eventFile.open(QIODevice::ReadOnly))
		return 0;
	
	QTextStream eventFileStream(&eventFile);

	do
	{
		int i=0;

		// Parse each line of Oprofile events file
		line = eventFileStream.readLine();
		if(line.startsWith("#"))
			continue;

		if(line.isEmpty())
			continue;

		// Check opName
		if ((i = line.find(QRegExp(" name:"))) != -1) {
			QString nameStr = line.mid(i+6).section(QRegExp(" "), 0, 0);
			if( opName != nameStr)
				continue;
		}

		// Get min count
		if ((i = line.find(QRegExp(" minimum:"))) != -1) {
			QString minStr = line.mid(i+9).section(" ", 0, 0);
			countMin = minStr.toUInt();
			break;
		}
	} while(!line.isNull());
	eventFile.close();
	return countMin;
}


bool EventCfgDlg::checkEventsMinCount(QString opName, unsigned long count)
{
	unsigned long countMin = getMinCountForEvent(opName);
	if (countMin == 0)
		countMin = 500; // Default value

	if (countMin > count) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("The count is too low. (minimum:") 
			+ QString::number(countMin) + ")");
		return false;
	}
	return true;
}


bool EventCfgDlg::checkEventsMaxCount(unsigned long count)
{
	/* Check count max value 
 	* Max: 0x7fff ffff
 	*      since OProfile driver only 
 	*      support 32-bit value
 	*/
	unsigned int countMax = -1;
	countMax = countMax >> 1;

	if (countMax < count) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("The count is too high. (maximum:") 
			+ QString::number(countMax) + ")");
		return false;
	}
	return true;
}


bool EventCfgDlg::checkIbsMaxCount(unsigned long count, bool bExt)
{
	/* Check IBS count max value 
 	* Max: 0xfffff (Family10h)
 	* Max: 0xfffff (Family12h)
 	*/
	unsigned int countMax;
	unsigned long ibsFeatureFlag;
	
	getCpuIdIbs(&ibsFeatureFlag);
	if (bExt && isCpuIbsCountExtOk(ibsFeatureFlag)) {
		countMax = 0x7ffffff;
	} else {
		countMax = 0xfffff;
	}

	if (countMax < count) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("The count is too high. (maximum:") 
			+ QString::number(countMax) + ")");
		return false;
	}
	return true;
}


bool EventCfgDlg::verifyPmcEvents(EventWithProps * pEvent, 
				unsigned long count, 
				unsigned int umask)
{

	if (!checkEventsMinCount(pEvent->opName, count)
	||  !checkEventsMaxCount(count)) {
		m_pCount->setFocus();
		return false;
	}

	return true;
}


bool EventCfgDlg::verifyIbsFetchEvents(EventWithProps * pEvent, 
				unsigned long count, 
				unsigned int umask)
{
	if (!checkEventsMinCount(pEvent->opName, count)
	||  !checkIbsMaxCount(count)) {
		m_pCount->setFocus();
		return false;
	}

	if (m_gFetchCount == 0) {
		m_gFetchCount = count;
		m_gFetchUmask = umask;
		return true;
	} 
		
	// Count must be the same
	if (count != m_gFetchCount) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("All IBS Fetch events must have the same count.\n"));
		m_pCount->setFocus();
		return false;
	}

	// Umask must be the same 
	if (umask != m_gFetchUmask) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("All IBS Fetch events must have the same option.\n"));
		m_pUmaskBox->setFocus();
		return false;
	}
	
	return true;
}


bool EventCfgDlg::verifyIbsOpEvents(EventWithProps * pEvent, 
				unsigned long count, 
				unsigned int umask)
{

	if (!checkEventsMinCount(pEvent->opName, count)
	||  !checkIbsMaxCount(count, true)) {
		m_pCount->setFocus();
		return false;
	}

	if (!isDriverIbsOpDispatchOpOk() 
	&&  (umask & 0x1) > 0 ) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("IBS Op Dispatch op is not supported (unitmask:") 
			+ QString().sprintf("0x%02x", umask) + ")");
		m_pSelectedList->setFocus();
		m_pSelectedList->setSelected(pEvent, true);
		return false;
	}

	if (m_gOpCount == 0) {
		m_gOpCount = count;
		m_gOpUmask = umask;
		return true;
	} 
	
	// Count must be the same
	if (count != m_gOpCount) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("All IBS Op events must have the same count.\n"));
		m_pCount->setFocus();
		return false;
	}

	// Umask must be the same 
	if (umask != m_gOpUmask) {
		QMessageBox::critical(this, "Profile Configuration Error",
			QString("All IBS Op events must have the same option.\n"));
		m_pUmaskBox->setFocus();
		return false;
	}

	return true;
}


QString EventCfgDlg::helpGetCheckBoxValue(QCheckBox * pBox)
{
	if (!pBox || !pBox->isEnabled())
		return QString("N/A");
	else if (pBox->isChecked())
		return QString("1");
	else
		return QString("0");
}


void EventCfgDlg::onApplyEventSetting()
{
	EventWithProps * pEvent = (EventWithProps *) m_pSelectedList->selectedItem();
	if (!pEvent) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Please select event to apply setting."));
		return;
	}

	unsigned int umask = getCurrentUmask(m_pUmask);
	

	/*
	 * Verify Selected Event
	 */
	
	//Clear existing value before verification
	m_gFetchCount		= 0;
	m_gFetchUmask		= 0;
	m_gOpCount		= 0;
	m_gOpUmask		= 0;

	if (!verifySelectedEvent(pEvent, m_pCount->text().toULong(0,10), umask))
		return;

	/*
	 * Update Selected Event
	 */
	if (PerfEvent::isIbsFetchEvent(
			pEvent->text(ColSel).toUInt(NULL,16))) {
		syncAllIbsFetchEvents(m_pCount->text().toULong(0,10), umask);
	} else if(PerfEvent::isIbsOpEvent(
			pEvent->text(ColSel).toUInt(NULL,16))) {
		syncAllIbsOpEvents(m_pCount->text().toULong(0,10), umask);
		
	} else {
		pEvent->setText(ColCount, m_pCount->text());
		pEvent->setText(ColUmask, 
				QString().sprintf("0x%02x", umask));
		pEvent->setText(ColUsr, helpGetCheckBoxValue(m_pUsr));
		pEvent->setText(ColOs,  helpGetCheckBoxValue(m_pOs));
//		pEvent->setText(ColEdge, helpGetCheckBoxValue(m_pEdge));
	}

	m_pSelectedList->setFocus();

	return;
}


unsigned int EventCfgDlg::getDefaultUmask(QString opName)
{
	unsigned int mask = 0;
	CpuEvent eventData;

	if (!m_pCaEventsFile->findEventByOpName (opName, eventData))
		return 0;

	//read unit mask from event file
	for( UnitMaskList::const_iterator umit = m_pCaEventsFile->FirstUnitMask (eventData); 
	    umit != m_pCaEventsFile->EndOfUnitMasks (eventData); ++umit) 
	{
		if (umit->value >= UNIT_MASK_COUNT) {
			QMessageBox::critical (this, "CodeAnalyst error",
				"Your list of events seems to have been corrupted.\n"
				"Please re-install CodeAnalyst.");
			continue;
		}

		mask |= 1 << umit->value;
	}
	return mask;
}

void EventCfgDlg::onSelectedSelectionChanged(Q3ListViewItem * pEvent)
{
	if (!pEvent)
		return;

	/*
 	 * Set up new selected event
 	 */

	EventWithProps * pEventProp = static_cast<EventWithProps *>(pEvent);

	/*
 	 * Setup Count 
 	 */
	m_pCount->setText(pEvent->text(ColCount));

	/*
 	 * Setup Umask
 	 */
	clearUmasks(m_pUmask);

	if (PerfEvent::isIbsFetchEvent(pEvent->text(ColSel).toUInt(NULL,16))) {
		m_pUmaskBox->setTitle("IBS Fetch Options");
	} else if (PerfEvent::isIbsOpEvent(pEvent->text(ColSel).toUInt(NULL,16))) {
		m_pUmaskBox->setTitle("IBS Op Options");

		if (isDriverIbsOpDispatchOpOk()) {
			// NOTE: This is hardcoded
			m_pUmask[0]->setEnabled (true);
			m_pUmask[0]->setText("Enable Dispatch count mode");
			m_pUmask[0]->setChecked(
				(pEvent->text(ColUmask).toUInt(NULL,16) & (IbsOpEvent::DispatchCount)) > 0);
		}

		// EXPERIMENTAL
		m_pUmask[1]->setEnabled (true);
		m_pUmask[1]->setText("Enable Mem Access Log");
		m_pUmask[1]->setChecked(
			(pEvent->text(ColUmask).toUInt(NULL,16) & (IbsOpEvent::DcMissAddr)) > 0);

		// EXPERIMENTAL
		unsigned long ibsFeatureFlag;
		getCpuIdIbs(&ibsFeatureFlag);
		if (isDriverIbsOpBranchTargetAddrOk()
		&&  isCpuIbsBrTgtAddrOk(ibsFeatureFlag)) {
			m_pUmask[2]->setEnabled (true);
			m_pUmask[2]->setText("Enable Branch Target Address Log");
			m_pUmask[2]->setChecked(
				(pEvent->text(ColUmask).toUInt(NULL,16) & (IbsOpEvent::BranchTargetAddr)) > 0);
		}
	} else {
		m_pUmaskBox->setTitle("Unit Masks");
	
		CpuEvent eventData;
		if (!m_pCaEventsFile->findEventByOpName (pEventProp->opName, eventData))
			return;

		//read unit mask from event file
		for( UnitMaskList::const_iterator umit = m_pCaEventsFile->FirstUnitMask (eventData); 
			umit != m_pCaEventsFile->EndOfUnitMasks (eventData); ++umit) {
			if (umit->value >= UNIT_MASK_COUNT) {
				QMessageBox::critical (this, "CodeAnalyst error",
					"Your list of events seems to have been corrupted.\n"
					"Please re-install CodeAnalyst.");
				continue;
			}

			m_pUmask[umit->value]->setEnabled (true);
			m_pUmask[umit->value]->setText (umit->name);
			// TODO: get default
			m_pUmask[umit->value]->setChecked (
				(pEvent->text(ColUmask).toUInt(NULL,16) & (1 << umit->value)) > 0);
		}
	}

	/*
 	 * Setup Usr, Os, Edge
 	 */
	bool isIbs = (PerfEvent::isIbsFetchEvent(pEvent->text(ColSel).toUInt(NULL,16))
		   || PerfEvent::isIbsOpEvent(pEvent->text(ColSel).toUInt(NULL,16)));

	m_pUsr->setChecked(pEvent->text(ColUsr) == "1");
	m_pOs->setChecked(pEvent->text(ColOs) == "1");
//	m_pEdge->setChecked(pEvent->text(ColEdge) == "1");

	m_pOs->setEnabled(!isIbs);
	m_pUsr->setEnabled(!isIbs);
//	m_pEdge->setEnabled(!isIbs);
}


unsigned int EventCfgDlg::getCurrentUmask(UnitCheckBox ** pBox)
{
	unsigned int umask = 0;

	for ( unsigned int i = 0; i < UNIT_MASK_COUNT; i++) {
		if (pBox[i]->isChecked()) {
			umask |= (1 << i);
		}
	}
	return umask;
}


void EventCfgDlg::setCurrentUmask(UnitCheckBox ** pBox, unsigned int umask)
{
	unsigned int mask = 1;
	
	for (unsigned int i  = 0; i < UNIT_MASK_COUNT; i++) {
		unsigned int tmp = 0;
		tmp = (mask << i) & umask; 
		if (tmp == 0)
			pBox[i]->setChecked (false);
		else
			pBox[i]->setChecked (true);
	}
}


void EventCfgDlg::setupInfoTab()
{
	QString CpuDesc =   "Model name : " + m_cpuName 
			+ "\nFamily : " + m_cpuFamily 
			+ "\nModel : " + m_cpuModel 
			+ "\nStepping : " + m_cpuStepping
			+ "\nEvents file : " + m_eventsFilePath;

	m_pInfo->setText(CpuDesc);
}

void EventCfgDlg::clearUmasks( UnitCheckBox ** pBox)
{
	for (unsigned int i = 0; i < UNIT_MASK_COUNT; i++) {
		pBox[i]->setText ("Reserved");
		pBox[i]->setChecked (false);
		pBox[i]->setEnabled (false);
	}

}


int EventCfgDlg::getCurrentIbsCountUmask(unsigned long * count, unsigned int * umask,
					PerfEvent::PerfEventType ibsType) 
{
	EventWithProps * cur  = (EventWithProps *) m_pSelectedList->firstChild();
	EventWithProps * last = (EventWithProps *) m_pSelectedList->lastItem();
	QString curCnt;
	QString curUmask;
	QString ibsPrefix;

	if (!cur)
		goto out;
	
	if (ibsType == PerfEvent::IbsFetch)
		ibsPrefix = "IBS_FETCH_";
	else if (ibsType == PerfEvent::IbsOp)
		ibsPrefix = "IBS_OP_";
	else
		return -1;

	do {
		if (cur->opName.startsWith(ibsPrefix)) {
			if (curCnt.isEmpty()) {
				curCnt   = cur->text(ColCount);
				curUmask = cur->text(ColUmask);
			} else {
				if (curCnt.compare(cur->text(ColCount)) 
				||  curUmask.compare(cur->text(ColUmask)))
				return -1; 
			}
		}

		if (cur == last)
			break;

		cur = (EventWithProps *)cur->nextSibling();
	} while (cur); 
out:
	*count = curCnt.toUInt(0);
	*umask = curUmask.toUInt(0, 16);
	return 0;

}


void EventCfgDlg::syncAllIbsFetchEvents(unsigned long count, unsigned int umask) 
{
	EventWithProps * cur  = (EventWithProps *) m_pSelectedList->firstChild();
	EventWithProps * last = (EventWithProps *) m_pSelectedList->lastItem();
	if (!cur)
		return;

	do {
		if (cur->opName.startsWith("IBS_FETCH_")) {
			cur->setText(ColCount, QString::number(count,10));
			cur->setText(ColUmask, QString().sprintf("0x%02x",umask));
		}

		if (cur == last)
			break;

		cur = (EventWithProps *)cur->nextSibling();
	} while (cur); 
}


void EventCfgDlg::syncAllIbsOpEvents(unsigned long count, unsigned int umask) 
{
	EventWithProps * cur  = (EventWithProps *) m_pSelectedList->firstChild();
	EventWithProps * last = (EventWithProps *) m_pSelectedList->lastItem();
	if (!cur)
		return;

	do {
		if (cur->opName.startsWith("IBS_OP_")) {
			cur->setText(ColCount, QString::number(count,10));
			cur->setText(ColUmask, QString().sprintf("0x%02x",umask));
		}

		if (cur == last)
			break;

		cur = (EventWithProps *)cur->nextSibling();
	} while (cur); 
}


bool EventCfgDlg::hasDuplicateEvent(EventWithProps * pEvent, QString umask)
{
	QString name = pEvent->text(ColSelName);

	EventWithProps * cur = NULL;
	for (cur = (EventWithProps *) m_pSelectedList->firstChild()
		; cur != NULL
		; cur = (EventWithProps *)cur->nextSibling())
	{
		if (cur == pEvent)
			continue;
 
		// Check name
		if (name != cur->text(ColSelName)) {
			continue;
		}

		// Check umask
		if (umask != cur->text(ColUmask)) {
			continue; 
		}

		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("Event:") + name +
			+ " umask:" + umask + " already exist.");
		return true;
	}	
	return false;
}


void EventCfgDlg::onImportDcConfigChanged(const QString & configName)
{
	QString toolTip;

	if (!m_pProfiles->getProfileTexts (configName, &toolTip, &m_description))
		return;

	//set description
	m_pImportDescription->setText (m_description);
}


void EventCfgDlg::onPmcCountChanged()
{
	// MUX could be enabled when PMC is more than 1
	if (m_pmcEventCount > 1) {
		m_pMuxIntervalLabel->setEnabled(true);
		m_pMuxInterval->setEnabled(true);
		if (m_pMuxInterval->text().toUInt() <= 0)
			m_pMuxInterval->setText("1");
	} else {
		m_pMuxIntervalLabel->setEnabled(false);
		m_pMuxInterval->setEnabled(false);
	}
}

void EventCfgDlg::onAvailableTabChanged(QWidget *w)
{
	int index = m_pAvailableEventTab->indexOf(w);
	
	switch (index) {
	case PAGE_PMC:
		m_pPmcList->setFocus();	
		break;
	case PAGE_FETCH:
		m_pFetchList->setFocus();	
		break;
	case PAGE_OP:
		m_pOpList->setFocus();	
		break;
	default:
		break;
	}
}
