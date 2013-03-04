//$Id: EventCfg.cpp 20178 2011-08-25 07:40:17Z sjeganat $

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
#include <Q3GridLayout>
#include "EventCfg.h"
#include "iruncontrol.h"
#include "atuneoptions.h"
#include "xp.h"
#include "EventMaskEncoding.h"
#include "affinity.h"

UnitCheckBox::UnitCheckBox (QWidget *parent, int index) : QCheckBox (parent)
{
	m_index = index;
	connect (this, SIGNAL (clicked ()), SLOT (onChecked ())); 
}


UnitCheckBox::~UnitCheckBox ()
{
}


void UnitCheckBox::onChecked ()
{
	emit checked (m_index);
}


EventWithProps::EventWithProps (Q3ListViewItem *pParent) 
: Q3ListViewItem (pParent)
{
	m_eventUnitMask = 0;
}

EventWithProps::EventWithProps (Q3ListView *pParent, EventWithProps *pAfter)
: Q3ListViewItem (pParent, pAfter)
{
}



EventListWithProps::EventListWithProps (Q3ListBox *pParent, const QString & text)
: Q3ListBoxText (pParent, text)
{
}


EventListWithProps::~EventListWithProps()
{
}


EventCfgDlg::EventCfgDlg ( QWidget* parent, const char* name, bool modal, 
						  Qt::WFlags fl) : IEventCfg (parent, name, modal, fl)
{
	setupUi(this);
	m_pProfiles = NULL;
	m_modified = false;
	m_isViewProperty = false;	

	buildCheckBoxes();

	connect (m_pProfileName, SIGNAL (textChanged ( const QString &)), 
		SLOT (onModified ()));
	connect (m_pMuxInterval, SIGNAL (textChanged ( const QString &)), 
		SLOT (onModified ()));
	connect (m_pEventCnt, SIGNAL (textChanged ( const QString &)), 
		SLOT (onModified ()));
	connect (m_pAddEvent, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pAddCfgEvents, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pEventUp, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pEventDown, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pRemoveEvent, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pUsr, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pOs, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pEdge, SIGNAL (clicked()), SLOT (onModified ()));

	connect (m_pUsr, SIGNAL (toggled (bool)), SLOT (onUsrChecked (bool)));
	connect (m_pOs, SIGNAL (toggled (bool)), SLOT (onOsChecked (bool)));
	connect (m_pEdge, SIGNAL (toggled (bool)), SLOT (onEdgeChecked (bool)));
	connect (m_pEventCnt, SIGNAL (textChanged ( const QString &)), 
		SLOT (onCountChanged (const QString &)));
	connect (m_pGroupsList, SIGNAL (currentChanged ( Q3ListViewItem * ) ),
		SLOT (onEventChanged (Q3ListViewItem *)));

	connect (m_pConfigs, SIGNAL (activated ( const QString &)), 
		SLOT (onConfigChanged (const QString &)));
	connect (m_pAddEvent, SIGNAL (clicked ()), SLOT (onAddEvent ()));
	connect (m_pAddCfgEvents, SIGNAL (clicked ()), SLOT (onAddConfig ()));
	connect (m_pEventUp, SIGNAL (clicked ()), SLOT (onMoveEventUp ()));
	connect (m_pEventDown, SIGNAL (clicked ()), SLOT (onMoveEventDown ()));
	connect (m_pRemoveEvent, SIGNAL (clicked ()), SLOT (onRemoveEvent ()));
	connect (m_pUsr, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pNewGroup, SIGNAL (clicked ()), SLOT (onNewGroup ()));

	connect (m_pOk, SIGNAL (clicked ()), SLOT (onOk()));

} //EventCfgDlg::EventCfgDlg


EventCfgDlg::~EventCfgDlg ()
{
	m_pGroupsList->clear();
}


//This allocates the array of unit boxes which greatly reduces the ammount 
//	of redundant code
void EventCfgDlg::buildCheckBoxes ()
{
	m_pUnitMaskBox->setColumnLayout(0, Qt::Vertical );

	Q3GridLayout * pTempLayout = new Q3GridLayout (m_pUnitMaskBox->layout() );
	RETURN_IF_NULL (pTempLayout, this);
	pTempLayout->setAlignment( Qt::AlignTop );

	for (unsigned int j = 0; j < UNIT_MASK_COUNT; j++) 
	{
		m_pUnitMask[j] = new UnitCheckBox( m_pUnitMaskBox, j);
		RETURN_IF_NULL (m_pUnitMask[j], this);
		m_pUnitMask[j]->setEnabled (FALSE);
		pTempLayout->addWidget( m_pUnitMask[j], j, 0 );
		m_pUnitMask[j]->show();
		connect (m_pUnitMask[j], SIGNAL (clicked ()), SLOT (onModified ()));
		connect (m_pUnitMask[j], SIGNAL (checked (int)), SLOT (onUnitChecked (int)));
	}
}


void EventCfgDlg::onModified ()
{
	m_modified = true;
}


void EventCfgDlg::setProperties (EBP_OPTIONS *pSession
								 ,const char* path)
{
	if(path == NULL)
		return;

	m_isViewProperty = true;	
	setCaption ("Review event configuration");
	//add run control
	IRunControl * pRunControl = new IRunControl (this);
	RETURN_IF_NULL (pRunControl, this);
	setExtension (pRunControl);
	setOrientation (Vertical);
	showExtension (true);

	//set readonly
	m_pProfileName->setReadOnly (true);
	m_pMuxInterval->setReadOnly (true);
	m_pEventCnt->setReadOnly (true);
	m_pLaunch->setReadOnly (true);
	m_pWorkDir->setReadOnly (true);
	m_pEventUp->setEnabled (false);
	m_pEventDown->setEnabled (false);
	m_pNewGroup->setEnabled (false);
	m_pRemoveEvent->setEnabled (false);
	m_pUsr->setEnabled(false);
	m_pEdge->setEnabled(false);
	m_pOs->setEnabled(false);
	pRunControl->m_pDuration->setReadOnly (true);
	pRunControl->m_pClearSessionNoteBtn->setEnabled(false);
	pRunControl->m_pStartDelay->setReadOnly (true);
	pRunControl->m_pStartPaused->setEnabled (false);
	pRunControl->m_pAffinityValue->setReadOnly(true);	
	pRunControl->m_pAffinityBtn->hide();
	pRunControl->m_pApplyFilter->setEnabled(false);

	//hide cancel
	buttonCancel->hide();
	m_pConfigBox->hide();
	m_pNewEventBox->hide();

	//Add setting info
	m_pProfileName->setText (pSession->sessionName);

	//Check type of cpu from "path"
	detectCpu (path);

	//add event info from session
	insertEvents (pSession);
	m_pMuxInterval->setText ((pSession->msMpxInterval == 0)? "N/A": QString::number( pSession->msMpxInterval));

	//Add run control info
	pRunControl->m_pSessionNote->setText (pSession->sessionNote);
	pRunControl->m_pSessionNote->setReadOnly (true);
	pRunControl->m_pDuration->setText ((pSession->duration <= 0)? "N/A": QString::number( pSession->duration));
	pRunControl->m_pProfileDuration->setChecked (0 == pSession->duration);
	pRunControl->m_pStartDelay->setText ((pSession->startDelay < 0)? "N/A": QString::number( pSession->startDelay));
	pRunControl->m_pStartPaused->setChecked (pSession->startPaused);
	pRunControl->m_pStopOnExit->setChecked (pSession->stopOnAppExit);
	pRunControl->m_pTerminateAfter->setChecked (pSession->termAppOnDur);
	QString afStr; 

	if(pSession->affinity.isZero())
		afStr = QString("N/A");
	else
		afStr = QString("0x") + QString(pSession->affinity.getAffinityMaskInHexString());

	pRunControl->m_pAffinityValue->setText(afStr);

	m_pLaunch->setText (pSession->launch);
	m_pWorkDir->setText (pSession->workDir);
	m_modified = false;

	// Resize to fit only visible widget
	this->resize(0,0);
} //EventCfgDlg::setProperties


void EventCfgDlg::setProfile (ProfileCollection *pProfiles, QString name)
{
	m_profileName = name;
	m_pProfiles = pProfiles;
	EBP_OPTIONS eventOptions;
	if (!m_pProfiles->getProfileConfig (name, &eventOptions))
		QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
	m_pProfileName->setText (m_profileName);
	m_pMuxInterval->setText (QString::number (eventOptions.msMpxInterval));


	QStringList pList = m_pProfiles->getListOfEbpProfiles();
	QStringList::iterator it;
	it = pList.find(QString("Time-based profile"));
	if(it != pList.end()) pList.remove(it);

	it = pList.find(QString("Instruction-based sampling"));
	if(it != pList.end()) pList.remove(it);

	it = pList.find(QString("Current instruction-based profile"));
	if(it != pList.end()) pList.remove(it);
	
	it = pList.find(QString("Current time-based profile"));
	if(it != pList.end()) pList.remove(it);

	m_pConfigs->insertStringList (pList);

	//Check type of cpu from /proc/cpuinfo
	detectCpu ("/proc/cpuinfo");

	//add events from profile
	insertEvents (&eventOptions);
	m_pMuxInterval->setEnabled (1 != m_pGroupsList->childCount());

	m_pLaunchBox->hide();
	m_modified = false;
}


void EventCfgDlg::onOk ()
{
	if (m_pProfileName->text().isEmpty())
	{
		QMessageBox::question (this, "CodeAnalyst error",
			"Please specify profile name.");
		m_pProfileName->setFocus ();
		return;
	}

	if ((m_modified) && (NULL != m_pProfiles))
	{
		//If the name would overwrite an existing profile
		if ((m_profileName != m_pProfileName->text()) && 
			(NO_TRIGGER != m_pProfiles->getProfileType (m_pProfileName->text()))
			&& (QMessageBox::question (this, "CodeAnalyst warning",
			"There is already a profile with that name,"
			"\ndo you want to overwrite the profile?",
			QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes))
		{
			//Set the focus, so the user can change the name
			m_pProfileName->setFocus ();
			return;
		}

		//Check if more than 8 groups
		if( m_pGroupsList->childCount()	> MAX_EVENTGROUP_MULTIPLEXING )
		{
			QMessageBox::critical(this, "CodeAnalyst Error",
				QString("Too many event groups.") 
				+ " The maximum number of groups is "
				+ QString::number(MAX_EVENTGROUP_MULTIPLEXING)+".\n");
			return;
		}

		//Save modified profile
		EBP_OPTIONS eventOptions;
		if (!getEvents (&eventOptions))
			return;

		if (!m_pProfiles->setProfileConfig (m_pProfileName->text(), &eventOptions))
		{
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
			m_pProfileName->setFocus ();
			return;
		}
	}
	accept();
} //EventCfgDlg::onOk


bool EventCfgDlg::wasModified ()
{
	return m_modified;
}

void EventCfgDlg::onConfigChanged (const QString & configName)
{
	QString toolTip;
	QString description;
	//look up description & tool tip
	if (!m_pProfiles->getProfileTexts (configName, &toolTip, &description))
	{
		return;
	}

	//set tooltip and description
	m_pDescription->setText (description);
	m_pConfigs->setToolTip("");
	m_pConfigs->setToolTip(toolTip);
}


void EventCfgDlg::onAddConfig ()
{
	EBP_OPTIONS configData;
	//get event list from config
	if (m_pProfiles->getProfileConfig (m_pConfigs->currentText(), &configData))
	{
		//Add the events to the event view
		insertEvents (&configData);
	} else {
		QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
	}
}


void EventCfgDlg::onAddEvent ()
{
	//add to appropriate group
	Q3ListViewItem *pGroup = ifNeedAddGroup();
	if(!pGroup) return;

	EventWithProps *pEvent = new EventWithProps (pGroup);
	RETURN_IF_NULL (pEvent, this);
	pEvent->setText (0, m_pEventList->currentText());

	EventListWithProps * pBaseEvent = static_cast<EventListWithProps *>(
		m_pEventList->item(m_pEventList->currentItem()));
	if (!pBaseEvent)
		return;

	pEvent->m_count = pBaseEvent->m_count;
	pEvent->m_eventSelect = pBaseEvent->m_eventSelect;
	pEvent->m_eventUnitMask = pBaseEvent->m_eventUnitMask;
	pEvent->m_edgeDetect = pBaseEvent->m_edgeDetect;
	pEvent->m_eventOS = pBaseEvent->m_eventOS;
	pEvent->m_eventUser = pBaseEvent->m_eventUser;

	//set as current
	m_pGroupsList->setSelected (pEvent, true);
	m_pGroupsList->setCurrentItem (pEvent);
	m_pGroupsList->ensureItemVisible (pEvent);
} //EventCfgDlg::onAddEvent


void EventCfgDlg::onNewGroup ()
{
	//if there's at least one event, add a new group line to event list view
	ifNeedAddGroup (1);
}


//moves event to the previous group
void EventCfgDlg::onMoveEventUp ()
{
	Q3ListViewItem * pTarget = m_pGroupsList->currentItem();
	if ((NULL == pTarget) || (0 == pTarget->depth()))
		return;

	//Get item's parent
	Q3ListViewItem * pGroup = pTarget->parent();
	Q3ListViewItem *pAboveGroup = pGroup->itemAbove();
	while ((NULL != pAboveGroup) && (0 != pAboveGroup->depth()))
	{
		pAboveGroup = pAboveGroup->itemAbove();
	}
	//No group above current
	if (NULL == pAboveGroup)
	{
		QMessageBox::warning (this, "CodeAnalyst warning", 
			"There are no previous groups.");
		return;
	}
	pGroup->takeItem (pTarget);
	//
	if (pAboveGroup->childCount() == MAX_EVENTNUM)
	{
		// swap
		Q3ListViewItem * pSwap = pAboveGroup->firstChild();
		while (NULL != pSwap->nextSibling())
		{
			pSwap = pSwap->nextSibling();
		}
		pAboveGroup->takeItem (pSwap);
		pGroup->insertItem (pSwap);
	}
	pAboveGroup->insertItem (pTarget);
	m_pGroupsList->setSelected (pTarget, true);
	m_pGroupsList->setCurrentItem (pTarget);
	m_pGroupsList->ensureItemVisible (pTarget);
	if (pGroup->childCount() == 0)
	{
		delete pGroup;
		// Rename All group
		int curGroup = 1;
		EventWithProps *pTmp= (EventWithProps *)m_pGroupsList->firstChild();
		while (NULL != pTmp)
		{
			pTmp->setText (0, QString("Event Group ") + QString().sprintf("%02d",curGroup));
			pTmp= (EventWithProps *)pTmp->nextSibling ();
			curGroup++;
		}
	}
	updateMux ();
} //EventCfgDlg::onMoveEventUp


//Move item to next group
void EventCfgDlg::onMoveEventDown ()
{
	Q3ListViewItem * pTarget = m_pGroupsList->currentItem();
	if ((NULL == pTarget) || (0 == pTarget->depth()))
		return;

	//Get item's parent
	Q3ListViewItem * pGroup = pTarget->parent();
	Q3ListViewItem *pBelowGroup = pGroup->itemBelow();
	while ((NULL != pBelowGroup) && (0 != pBelowGroup->depth()))
	{
		pBelowGroup = pBelowGroup->itemBelow();
	}
	//No group below current
	if (NULL == pBelowGroup)
	{
		//Not useful to do anything.
		if (pGroup->childCount() > 1)
		{
			QMessageBox::warning (this, "CodeAnalyst warning", 
				"There are no following groups.");
			return;
		}
		pBelowGroup = new EventWithProps (m_pGroupsList, NULL);
		RETURN_IF_NULL (pBelowGroup, this);
	}
	pGroup->takeItem (pTarget);

	//
	if (pBelowGroup->childCount() >= MAX_EVENTNUM)
	{
		// swap
		Q3ListViewItem * pSwap = pBelowGroup->firstChild();
		pBelowGroup->takeItem (pSwap);
		pGroup->insertItem (pSwap);

	}
	pBelowGroup->insertItem (pTarget);
	m_pGroupsList->setSelected (pTarget, true);
	m_pGroupsList->setCurrentItem (pTarget);
	m_pGroupsList->ensureItemVisible (pTarget);
	if (pGroup->childCount() == 0)
	{
		delete pGroup;

		// Rename All group
		int curGroup = 1;
		EventWithProps *pTmp= (EventWithProps *)m_pGroupsList->firstChild();
		while (NULL != pTmp)
		{
			pTmp->setText (0, QString("Event Group ") + QString().sprintf("%02d",curGroup));
			pTmp= (EventWithProps *)pTmp->nextSibling ();
			curGroup++;
		}
	}

	updateMux ();
} //EventCfgDlg::onMoveEventDown


void EventCfgDlg::onRemoveEvent ()
{
	Q3ListViewItem *pItem = m_pGroupsList->currentItem();
	if ((NULL == pItem) || (0 == pItem->depth()))
		return;
	//pParent can't be NULL due to pItem->depth != 0
	Q3ListViewItem *pParent = pItem->parent();

	m_pGroupsList->setSelected (pItem->itemAbove(), true);
	m_pGroupsList->setCurrentItem (pItem->itemAbove());
	m_pGroupsList->ensureItemVisible (pItem->itemAbove());
	pParent->takeItem(pItem);
	if(pItem)
	{
		delete pItem;
		pItem = NULL;
	}
	if (0 == pParent->childCount ())
	{
		m_pGroupsList->setSelected (pParent->itemAbove(), true);
		m_pGroupsList->setCurrentItem (pParent->itemAbove());
		m_pGroupsList->ensureItemVisible (pParent->itemAbove());
		m_pGroupsList->takeItem(pParent);
		if(pParent)
		{
			delete pParent;
			pParent = NULL;
		}

		// Rename All group
		int curGroup = 1;
		EventWithProps *pTmp= (EventWithProps *)m_pGroupsList->firstChild();
		while (NULL != pTmp)
		{
			pTmp->setText (0, QString("Event Group ") + QString().sprintf("%02d",curGroup));
			pTmp= (EventWithProps *)pTmp->nextSibling ();
			curGroup++;
		}
	}
	updateMux ();
}


void EventCfgDlg::onEventChanged (Q3ListViewItem *pEvent)
{
	if (NULL == pEvent)
	{
		resetEventProps();
		return;
	}
	//If group, turn off properties
	if (0 == pEvent->depth())
	{
		resetEventProps();
		return;
	}

	//Change properties to reflect
	EventWithProps *pEventProp = static_cast<EventWithProps *>(pEvent);
	m_pEventCnt->setText (QString::number (pEventProp->m_count));
	m_pEventCnt->setEnabled (true);

	//reflect unit mask, usr, os
	m_pOs->setChecked (pEventProp->m_eventOS);
	if(!m_isViewProperty) m_pOs->setEnabled (true);
	m_pEdge->setChecked (pEventProp->m_edgeDetect);
	if(!m_isViewProperty) m_pEdge->setEnabled (true);
	m_pUsr->setChecked (pEventProp->m_eventUser);
	if(!m_isViewProperty) m_pUsr->setEnabled (true);

	for (unsigned int un = 0; un < UNIT_MASK_COUNT; un++)
	{
		m_pUnitMask[un]->setText ("Reserved");
		m_pUnitMask[un]->setChecked (false);
		m_pUnitMask[un]->setEnabled (false);
	}

	CpuEvent eventData;
	if (!m_eventsFile.findEventByValue (pEventProp->m_eventSelect, eventData))
		return;

	//read unit mask from event file
	UnitMaskList::const_iterator umit;
	for( umit = m_eventsFile.FirstUnitMask (eventData); 
		umit != m_eventsFile.EndOfUnitMasks (eventData); ++umit )
	{
		if (umit->value >= UNIT_MASK_COUNT)
		{
			QMessageBox::critical (this, "CodeAnalyst error",
				"Your list of events seems to have been corrupted.\n"
				"Please re-install CodeAnalyst.");
			continue;
		}
		if(!m_isViewProperty) 
			m_pUnitMask[umit->value]->setEnabled (true);
		else 
			m_pUnitMask[umit->value]->setEnabled (false);
		m_pUnitMask[umit->value]->setText (umit->name);
		m_pUnitMask[umit->value]->setChecked ((pEventProp->m_eventUnitMask & (1 << umit->value)) > 0);
	}
} //EventCfgDlg::onEventChanged


void EventCfgDlg::onUnitChecked (int index)
{
	//If it's properties only, don't accept modifications
	if (NULL == m_pProfiles)
		return;

	Q3ListViewItem *pItem = m_pGroupsList->currentItem();
	if ((NULL == pItem) || (0 == pItem->depth()))
		return;

	EventWithProps *pEvent = static_cast<EventWithProps *>(pItem);

	//Since we're just toggling back and forth...
	if ((unsigned int) index < UNIT_MASK_COUNT)
		pEvent->m_eventUnitMask ^= 1 << index;
}


void EventCfgDlg::onUsrChecked (bool on)
{
	Q_UNUSED (on);
	//If it's properties only, don't accept modifications
	if (NULL == m_pProfiles)
		return;

	EventWithProps *pEvent = static_cast<EventWithProps *>(m_pGroupsList->currentItem());

	//Since we're just toggling back and forth...
	pEvent->m_eventUser = m_pUsr->isChecked ();
}


void EventCfgDlg::onOsChecked (bool on)
{
	Q_UNUSED (on);
	//If it's properties only, don't accept modifications
	if (NULL == m_pProfiles)
		return;

	EventWithProps *pEvent = static_cast<EventWithProps *>(m_pGroupsList->currentItem());

	//Since we're just toggling back and forth...
	pEvent->m_eventOS = m_pOs->isChecked ();
}


void EventCfgDlg::onCountChanged (const QString & count)
{
	Q_UNUSED (count);
	//If it's properties only, don't accept modifications
	if (NULL == m_pProfiles)
		return;

	Q3ListViewItem *pItem = m_pGroupsList->currentItem();
	if ((NULL == pItem) || (0 == pItem->depth()))
		return;

	EventWithProps *pEvent = static_cast<EventWithProps *>(pItem);

	pEvent->m_count = m_pEventCnt->text().toULongLong();
}


void EventCfgDlg::insertEvents (EBP_OPTIONS *pSession)
{
	int curGroup = 0;
	EventWithProps *pEvent = NULL;
	EventWithProps *pGroup = NULL;

	// Count current group
	if(m_pGroupsList->childCount() != 0)
	{	
		EventWithProps *pTmp= (EventWithProps *)m_pGroupsList->firstChild();
		while (NULL != pTmp)
		{
			pTmp= (EventWithProps *)pTmp->nextSibling ();
			curGroup++;
		}
	}

	for (int gp = 0; gp < pSession->countEventGroups; gp++)
	{

		if( curGroup >= MAX_EVENTGROUP_MULTIPLEXING )
		{
			QMessageBox::critical(this, "CodeAnalyst Error",
				QString("Cannot have more than ")
				+ QString::number(MAX_EVENTGROUP_MULTIPLEXING)
				+ "event groups.\n");
			return;
		}

		pGroup = new EventWithProps (m_pGroupsList, pGroup);
	//	pGroup = new EventWithProps (m_pGroupsList, (EventWithProps*)pGroupsList->lastItem());
		RETURN_IF_NULL (pGroup, this);
		pGroup->setOpen (true);
		pGroup->setText (0, QString("Event Group ") + QString().sprintf("%02d",++curGroup));

		for (int ev = 0; ev < pSession->pEvents[gp].numberOfValidEvents; ev++)
		{
			pEvent = new EventWithProps (pGroup);
			RETURN_IF_NULL (pEvent, this);

			pEvent->m_count = pSession->pEvents[gp].eventCount[ev];
			pEvent->m_eventSelect = pSession->pEvents[gp].eventSelect[ev];
			pEvent->m_eventUnitMask = pSession->pEvents[gp].eventUnitMask[ev];
			pEvent->m_edgeDetect =  pSession->pEvents[gp].edgeDetect[ev];
			pEvent->m_eventOS = pSession->pEvents[gp].eventOS[ev];
			pEvent->m_eventUser = pSession->pEvents[gp].eventUser[ev];

			QString name = makeEventName (pEvent->m_eventSelect);
			pEvent->setText (0, name);
		}
	}

	if (NULL != pEvent)
	{
		m_pGroupsList->setSelected (pEvent, true);
		m_pGroupsList->setCurrentItem (pEvent);
		m_pGroupsList->ensureItemVisible (pEvent);
	}
	updateMux ();

}


void EventCfgDlg::updateMux ()
{
	bool mux = (1 != m_pGroupsList->childCount());
	m_pMuxInterval->setEnabled (mux);
	if (!mux)
	{
		m_pMuxInterval->setText ("0");
	} else if (0 == m_pMuxInterval->text().toInt())
	{
		m_pMuxInterval->setText ("1");		
	}
}


void EventCfgDlg::resetEventProps ()
{
	for (unsigned int j = 0; j < UNIT_MASK_COUNT; j++) 
	{
		m_pUnitMask[j]->setText ("Reserved");
		m_pUnitMask[j]->setChecked (false);
		m_pUnitMask[j]->setEnabled (false);
	}
	m_pOs->setChecked (false);
	m_pOs->setEnabled (false);
	m_pUsr->setChecked (false);
	m_pUsr->setEnabled (false);
	m_pEdge->setChecked (false);
	m_pEdge->setEnabled (false);
	m_pEventCnt->setEnabled (false);
}


void EventCfgDlg::readEventsFile (QString fileName, int model)
{
	if( !m_eventsFile.open (fileName) )
	{
		QMessageBox::information( this, "Error", 
			"Unable to open the events file: " + fileName);
		return;
	}

	EventList::const_iterator elit;
	for( elit = m_eventsFile.FirstEvent(); elit != m_eventsFile.EndOfEvents(); elit++ )
	{
		//Don't show events which are not valid on this cpu
		if (elit->m_minValidModel > model)
			continue;

		// Filter out IBS derived events (0xF000+)
		if(elit->value < 0xF000)	
		{	
			QString name = makeEventName (elit->value);

			EventListWithProps *pEvent = new EventListWithProps (m_pEventList, name);
			RETURN_IF_NULL (pEvent, this);

			pEvent->m_eventSelect = elit->value;
			pEvent->m_eventUnitMask = 0;
			//Set all unit masks on by default
			UnitMaskList::const_iterator umit;
			for( umit = m_eventsFile.FirstUnitMask (*elit); 
				umit != m_eventsFile.EndOfUnitMasks (*elit); ++umit )
			{
				if (umit->value >= UNIT_MASK_COUNT)
					continue;
				pEvent->m_eventUnitMask |= 1 << umit->value;
				//umit->name
			}

			switch (elit->value)
			{
			case 0x76:
				pEvent->m_count = 1000000;
				break;
			case 0x40:
			case 0xC0:
				pEvent->m_count = 50000;
				break;
			case 0xd1:
				pEvent->m_count = 10000;
				break;
			default:
				pEvent->m_count = 5000;
				break;
			}
			pEvent->m_edgeDetect = false;
			pEvent->m_eventOS = true;
			pEvent->m_eventUser = true;
		}
	}
	m_pEventList->sort ();

	m_pEventList->setCurrentItem (m_pEventList->item(0));
} //EventCfgDlg::readEventsFile


QString EventCfgDlg::makeEventName (unsigned int evSelect)
{
	/*
	CpuEvent eventData;
	m_eventsFile.findEventByValue (evSelect, eventData);

	QString name = "[" + QString::number (eventData.value, 16) + "] ";
	name += eventData.name;
	return name;
	*/

	CpuEvent eventData;
	char tmp[10] = {'\0'};
	m_eventsFile.findEventByValue (evSelect, eventData);

	QString name = "[" ;

	// Note: Adding 0s to keep the same number of digits (4)
	// 	to help with sorting
	sprintf(tmp,"%04x",eventData.value);

	name += QString(tmp) + "] ";
	name += eventData.name;
	return name;
}


bool EventCfgDlg::getEvents (EBP_OPTIONS *pSession)
{
	EventMaskMap eventMaskMap;

	if (NULL == pSession)
		return false;
	if (NULL != pSession->pEvents)
	{
		delete [] pSession->pEvents;
	}
	//Allocate groups in session
	pSession->countEventGroups = m_pGroupsList->childCount();
	if (0 == pSession->countEventGroups)
	{
		QMessageBox::critical (this, "CodeAnalyst Error", 
			"No events were selected.  The configuration is invalid.");
		return false;
	}
	pSession->pEvents = new EventGroup[pSession->countEventGroups];
	RETURN_FALSE_IF_NULL (pSession->pEvents, this);

	//add each group
	int index = 0;
	Q3ListViewItem *pGroup = m_pGroupsList->firstChild ();
	while ((NULL != pGroup) && (index < pSession->countEventGroups))
	{
		int numEvents = 0;
		//Add the up to four events for each group
		int evIndex = 0;
		EventWithProps * pEvent = static_cast<EventWithProps *>(pGroup->firstChild ());
		while ((NULL != pEvent) && (evIndex < MAX_EVENTNUM))
		{
			// Count has type unsigned int, and max counter value is 0x7FFF FFFF
			unsigned int countMax = -1;
			countMax = countMax >> 1;
			unsigned int countMin = getMinCountForEvent(pEvent->m_eventSelect);

			// Check zero
			if(countMin == 0)
			{
				QMessageBox::critical (this, "CodeAnalyst Error",
					"Cannot acquire the minimum value for event 0x"
					+ QString::number(pEvent->m_eventSelect,16) +
					+ ".");
				return false;
			}

			// Check boundary;	
			if(pEvent->m_count < countMin
			|| pEvent->m_count > countMax)
			{
				QMessageBox::critical (this, "CodeAnalyst Error", 
					"Invalid value for parameter \"count\" of Event 0x"
					+ QString::number(pEvent->m_eventSelect,16) 
					+ ".\n"
					+ "The value range is [ "
					+ QString::number(countMin)+" , "
					+ QString::number(countMax)+" ]");
				return false;
			}

			// Check if already exist
			EVMASK_T cur = EncodeEventMask(pEvent->m_eventSelect,
								pEvent->m_eventUnitMask);
			EventMaskMap::iterator it = eventMaskMap.find(cur);
			if(  it != eventMaskMap.end()
			&&  it.data() != pEvent->m_count)
			{
				QMessageBox::critical (this, "CodeAnalyst Error", 
					"Event:0x" + QString::number(pEvent->m_eventSelect,16) 
					+ " with mask:0x" + QString::number(pEvent->m_eventUnitMask,16)
					+ " is already exist with count "
					+ QString::number(it.data()) + ".\n"
					+ "Same events with different counts are not allowed. Please reconfigure events.");
				return false;
			}else{
				eventMaskMap.insert(EventMaskMap::value_type(cur,pEvent->m_count));
			}	

			pSession->pEvents[index].eventSelect[evIndex] = pEvent->m_eventSelect;
			pSession->pEvents[index].eventUnitMask[evIndex] = pEvent->m_eventUnitMask;
			pSession->pEvents[index].edgeDetect[evIndex] = pEvent->m_edgeDetect;
			pSession->pEvents[index].eventOS[evIndex] = pEvent->m_eventOS;
			pSession->pEvents[index].eventUser[evIndex] = pEvent->m_eventUser;
			pSession->pEvents[index].eventCount[evIndex] = pEvent->m_count;
			evIndex++;
			numEvents++;
			pEvent = static_cast<EventWithProps *>(pEvent->nextSibling());
		}
		pSession->pEvents[index].numberOfValidEvents = evIndex;
		index++;
		if(numEvents == 0)
		{
			if( pGroup == m_pGroupsList->firstChild())
			{
				QMessageBox::critical (this, "CodeAnalyst Error",
					"No events were selected in group "+ pGroup->text(0) +".  The configuration is invalid.");
				return false;
			}else{
				QMessageBox::warning(this, "CodeAnalyst Warning",
					"No events were selected in group "+ pGroup->text(0) +".  This group will be removed.");
				Q3ListViewItem* pNextGroup = static_cast<Q3ListViewItem *>(pGroup->nextSibling ());
				m_pGroupsList->takeItem(pGroup);
				delete pGroup;
				pGroup = pNextGroup;
				pSession->countEventGroups--;
			}
		}else{
			pGroup = static_cast<Q3ListViewItem *>(pGroup->nextSibling ());
		}
	}

	bool isConvOk = false;
	unsigned int tmp = m_pMuxInterval->text().toUInt(&isConvOk);	
	if(m_pMuxInterval->isEnabled())
	{
		if(!isConvOk || tmp < 1 || tmp > 1000)
		{
			QMessageBox::critical (this,"CodeAnalyst Error",
					QString("\"") + m_pMuxInterval->text() + "\" is an invalid"
					+ " multiplexing interval.\n"
					+ "Please enter integer value between 1 to 1000 msec.\n");
			return false;
		}
	}

	pSession->msMpxInterval = tmp;
	pSession->sessionName   = m_pProfileName->text();
	return true;
} //EventCfgDlg::getEvents


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


void EventCfgDlg::detectCpu(const char* path)
{
	QString CpuDesc;
	QString name;
	QString family;
	QString model;
	QString stepping;
	QString vendorId;
	QString flags;

	if(path == NULL)
		return;

	m_bGeodeChip = false;

	if(getCpuInfo(path,vendorId,name,family,model,stepping,flags) != 0)
		return;

	CpuDesc =   "Model name : " + name 
		+ "\nFamily : " +  family 
		+ "\nModel : " + model 
		+ "\nStepping : " +  stepping;

	m_pCpuName->setText (CpuDesc);

	// Reading Events file
	if ( vendorId == QString("AuthenticAMD"))
	{
		QString file = helpGetEventFile(family.toULong() , model.toULong());
		readEventsFile (file, model.toULong());
		m_oprofileEventFile = OP_DATADIR;
		QString tmp;
		// Get Oprofile "events" file.
		switch(family.toULong()) 
		{
		case OPTERON_FAMILY:
			m_oprofileEventFile += "/x86-64/hammer/events";
			break;
		case ATHLON_FAMILY:
			m_oprofileEventFile += "/i386/athlon/events";
			break;
		case GREYHOUND_FAMILY:
			tmp = m_oprofileEventFile + "/x86-64/family10/events";
			if(QFile::exists(tmp)){
				m_oprofileEventFile = tmp;
				break;
			}	
		
			tmp = m_oprofileEventFile + "/x86-64/family10h/events";
			if(QFile::exists(tmp)){
				m_oprofileEventFile = tmp;
				break;
			}	
			break;
		case GRIFFIN_FAMILY:
			m_oprofileEventFile += "/x86-64/family11h/events";
			break;
		}
	}


	// determine if we have a local apic for sampling
	if( flags.contains("apic")) 
	{
		m_bHasLocalApic = true;
	} 
	else 
	{
		m_bHasLocalApic = false;

		qDebug( "OptionsDlg::DetectCpu(): cpu features = %s", flags.data());
		qDebug( "OptionsDlg::DetectCpu(): no onboard local APIC!!!" );
	}

	if ((m_bGeodeChip) || (!m_bHasLocalApic))
	{
		QMessageBox::critical (this, "CodeAnalyst warning",
			"This computer cannot perform event-based profiling.\n"
			"It may be an embedded computer or have no local APIC.");
		//Don't allow events to be set
		m_pOk->setEnabled (false);
	}

} //EventCfgDlg::detectCpu


//If the number of children of the last group is >= minEventCount, creates a 
//	new group
Q3ListViewItem * EventCfgDlg::ifNeedAddGroup (int minEventCount)
{
	int curGroup = 1;
	EventWithProps *pGroup = (EventWithProps *)m_pGroupsList->firstChild();
	EventWithProps *previousGroup = pGroup;

	if ((NULL != pGroup) && (pGroup->depth () > 0))
	{
		previousGroup = pGroup = (EventWithProps *)pGroup->parent();
	}

	// Traversing the last group.
	while (NULL != pGroup)
	{
		previousGroup = pGroup;
		pGroup = (EventWithProps *)pGroup->nextSibling ();
		curGroup++;
	}

	if ((NULL == previousGroup) || (minEventCount <= previousGroup->childCount ()))
	{
		//Check if more than 8 groups
		if( curGroup > MAX_EVENTGROUP_MULTIPLEXING )
		{
			QMessageBox::critical(this, "CodeAnalyst Error",
				QString("Cannot have more than ")
				+ QString::number(MAX_EVENTGROUP_MULTIPLEXING)
				+ "event groups.\n"); 
			return pGroup;
		}

		pGroup = new EventWithProps (m_pGroupsList, previousGroup);
		pGroup->setText (0, QString("Event Group ") + QString().sprintf("%02d",curGroup));
		if (NULL != pGroup) 
		{
			pGroup->setOpen (true);
		}
	} else {
		pGroup = previousGroup;
	}

	// Set visibility and selection.	
	if (NULL != pGroup)
	{
		m_pGroupsList->setSelected (pGroup, true);
		m_pGroupsList->setCurrentItem (pGroup);
		m_pGroupsList->ensureItemVisible (pGroup);
	}
	updateMux ();

	return pGroup;
} //EventCfgDlg::ifNeedAddGroup


void EventCfgDlg::onEdgeChecked (bool on)
{
	Q_UNUSED (on);
	//If it's properties only, don't accept modifications
	if (NULL == m_pProfiles)
		return;

	EventWithProps *pEvent = static_cast<EventWithProps *>(m_pGroupsList->currentItem());

	//Since we're just toggling back and forth...
	pEvent->m_edgeDetect = m_pEdge->isChecked ();
}


unsigned int EventCfgDlg::getMinCountForEvent(unsigned int event)
{
	unsigned int countMin = 0;
	QString line;
	bool isOk = false;

	QFile eventFile(m_oprofileEventFile);

	if(!eventFile.open(QIODevice::ReadOnly))
		return 0;
	
	QTextStream eventFileStream(&eventFile);
	do
	{
		// Parse each line of Oprofile events file
		line = eventFileStream.readLine();
		if(line.startsWith("#"))
			continue;
		QString eventStr = line.section(" ",0,0);
		eventStr = eventStr.section(":",1,1);
		if( event != eventStr.toUInt(&isOk,16)
		||  !isOk)
			continue;
		QString minStr = line.section(" ",3,3);
		minStr = minStr.section(":",1,1);
		countMin = minStr.toUInt();
		break;
	} while(!line.isNull());
	eventFile.close();
	return countMin;
}


