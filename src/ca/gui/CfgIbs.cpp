//$Id: CfgIbs.cpp 20178 2011-08-25 07:40:17Z sjeganat $

/*
// CodeAnalyst for Open Source
// Copyright 2002 - 2008 Advanced Micro Devices, Inc.
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

#include "tbsreader.h"
#include "CfgIbs.h"
#include "iruncontrol.h"
#include "atuneoptions.h"
#include "oprofile_interface.h"
#include <q3buttongroup.h>
#include <q3textedit.h>
#include <Q3ValueList>
#include "helperAPI.h"
#include "affinity.h"

IbsCfgDlg::IbsCfgDlg ( QWidget* parent, const char* name, bool modal, Qt::WFlags fl) 
: IIbsCfg (parent, name, modal, fl)
{
	m_pProfiles = NULL;
	m_dispatchAvail = false;

	connect (buttonOk, SIGNAL (clicked ()), SLOT (onOk ()));
	connect (buttonCancel, SIGNAL (clicked()), SLOT (reject()));
	connect (m_pProfileName, SIGNAL (textChanged ( const QString &)), SLOT (onModified ()));

	//Fetch
	connect (m_pFetchInterval, SIGNAL (textChanged ( const QString &)), SLOT (onModified ()));
	connect (m_pFetch, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pFetchList, SIGNAL (selectionChanged()), SLOT (onModified ()));
	connect (m_pFetch, SIGNAL (toggled ( bool )), m_pFetchLabel,
		SLOT (setEnabled ( bool )));
	connect (m_pFetch, SIGNAL (toggled ( bool )), m_pFetchInterval,
		SLOT (setEnabled ( bool )));
	connect (m_pFetch, SIGNAL (toggled ( bool )), m_pFetchList,
		SLOT (setEnabled ( bool )));
	connect (m_pFetch, SIGNAL (toggled ( bool )), m_pSelectAllFetch,
		SLOT (setEnabled ( bool )));

	//Op
	connect (m_pOp, SIGNAL (clicked ()), SLOT (onModified ()));
	connect (m_pOpMode, SIGNAL (clicked (int)), SLOT (onModified ()));
	connect (m_pOpInterval, SIGNAL (textChanged ( const QString &)), SLOT (onModified ()));
	connect (m_pOpList, SIGNAL (selectionChanged()), SLOT (onModified ()));
	connect (m_pDcMissInfoEnabled, SIGNAL (toggled( bool )), SLOT (onModified ()));
	
	connect (m_pOp, SIGNAL (toggled ( bool )), m_pOpLabel,
		SLOT (setEnabled ( bool )));
	connect (m_pOp, SIGNAL (toggled ( bool )), m_pOpInterval,
		SLOT (setEnabled ( bool )));
	connect (m_pOp, SIGNAL (toggled ( bool )), m_pOpCycle,
		SLOT (setEnabled ( bool )));
	connect (m_pOp, SIGNAL (toggled ( bool )), m_pOpMode,
		SLOT (setEnabled ( bool )));
	connect (m_pOp, SIGNAL (toggled ( bool )), m_pOpList,
		SLOT (setEnabled ( bool )));
	connect (m_pOp, SIGNAL (toggled ( bool )), m_pSelectAllOp,
		SLOT (setEnabled ( bool )));
	connect (m_pOp, SIGNAL (toggled ( bool )), m_pDcMissInfoEnabled,
		SLOT (setEnabled ( bool )));
	connect (m_pOp, SIGNAL (toggled ( bool )), this,
		SLOT (onOpEnabled ( bool )));

	// IBS Event List
	m_pFetchList->setColumnAlignment(IBS_VALUE,Qt::AlignHCenter);
	m_pFetchList->setColumnAlignment(IBS_SRC,Qt::AlignHCenter);
	m_pFetchList->setResizeMode(Q3ListView::LastColumn);
	m_pFetchList->setAllColumnsShowFocus(true);
	m_pFetchList->setShowSortIndicator(true);
	m_pFetchList->clear();

	m_pOpList->setColumnAlignment(IBS_VALUE,Qt::AlignHCenter);
	m_pOpList->setColumnAlignment(IBS_SRC,Qt::AlignHCenter);
	m_pOpList->setResizeMode(Q3ListView::LastColumn);
	m_pOpList->setAllColumnsShowFocus(true);
	m_pOpList->setShowSortIndicator(true);
	m_pOpList->clear();
}

IbsCfgDlg::~IbsCfgDlg ()
{
}


void IbsCfgDlg::onModified ()
{
	m_modified = true;
}

void IbsCfgDlg::onOpEnabled (bool enabled)
{
	m_pOpDispatch->setEnabled (enabled && m_dispatchAvail);
}

void IbsCfgDlg::setProperties (IBS_OPTIONS *pSession)
{
	setCaption ("Review Ibs configuration");

	// Hide 
	buttonCancel->hide();
	m_pSelectAllFetch->hide();
	m_pSelectAllOp->hide();
	m_pDcMissInfoEnabled->hide();

	//------------------------
	//Add Setting info
	m_pProfileName->setText (pSession->sessionName);

	//------------------------
	// Add Launch Setting Info
	m_pProfileName->setReadOnly (true);
	m_pLaunch->setReadOnly (true);
	m_pWorkDir->setReadOnly (true);
	m_pLaunch->setText (pSession->launch);
	m_pWorkDir->setText (pSession->workDir);
	m_modified = false;

	//------------------------
	// Add Fetch Setting Info
	m_pFetch->setChecked (pSession->fetchSample == 1);
	m_pFetchInterval->setText (QString::number (pSession->fetchInterval));
	m_pFetchInterval->setReadOnly (true);
	m_pFetch->setEnabled (false);

	//------------------------
	// Add Op Setting Info
	m_pOp->setChecked (pSession->opSample == 1);
	m_pOpInterval->setText (QString::number (pSession->opInterval));
	m_pOpInterval->setReadOnly (true);
	m_pOp->setEnabled (false);
	m_pOpCycle->setChecked (pSession->opCycleCount);
	m_pOpCycle->setEnabled (pSession->opCycleCount);
	m_pOpDispatch->setChecked (!pSession->opCycleCount);
	m_pOpDispatch->setEnabled (!pSession->opCycleCount);

	// Get Family from TBP file to locate eventFile
	TbsReader tbsReader;
	tbsReader.OpenTbsFile(pSession->sessionFile);
	unsigned long Family = tbsReader.getCpuFamily();	
	unsigned long Model = tbsReader.getCpuModel();	
	QString file = helpGetEventFile(Family, Model);
	if( !m_eventsFile.open (file) )
	{
		QMessageBox::information( this, "Error", 
			"Unable to open the events file: " + file);
	}

	// Load events to IBS Fetch/Op List
	addIbsEventFromProfileToList(Model, pSession);

	// If no IBS event in both lists, add all events	
	if( m_pFetchList->firstChild() == NULL
	&&  m_pOpList->firstChild() == NULL) {
		addIbsEventFromFileToList(Model);
	}

	m_pFetchList->setSelectionMode(Q3ListView::NoSelection);
	m_pOpList->setSelectionMode(Q3ListView::NoSelection);

	//------------------------
	// ADD RunControl
	IRunControl * pRunControl = new IRunControl (this);
	RETURN_IF_NULL (pRunControl, this);
	setExtension (pRunControl);
	setOrientation (Vertical);
	showExtension (true);
	
	pRunControl->m_pSessionNote->setText (pSession->sessionNote);
	pRunControl->m_pSessionNote->setReadOnly (true);
	pRunControl->m_pClearSessionNoteBtn->setEnabled(false);
	pRunControl->m_pAffinityBtn->hide();
	pRunControl->m_pDuration->setReadOnly (true);
	pRunControl->m_pStartDelay->setReadOnly (true);
	pRunControl->m_pAffinityValue->setReadOnly(true);	
	pRunControl->m_pStartPaused->setEnabled (false);
	pRunControl->m_pApplyFilter->setEnabled(false);
	pRunControl->m_pDuration->setText (
		(pSession->duration <= 0)? "N/A": QString::number( pSession->duration));
	pRunControl->m_pProfileDuration->setChecked (0 == pSession->duration);
	pRunControl->m_pStartDelay->setText (
		(pSession->startDelay < 0)? "N/A": QString::number( pSession->startDelay));
	pRunControl->m_pStartPaused->setChecked (pSession->startPaused);
	pRunControl->m_pStopOnExit->setChecked (pSession->stopOnAppExit);
	pRunControl->m_pTerminateAfter->setChecked (pSession->termAppOnDur);
	QString afStr; 

	if(pSession->affinity.isZero())
		afStr = QString("N/A");
	else
		afStr = QString("0x") + QString(pSession->affinity.getAffinityMaskInHexString());

	pRunControl->m_pAffinityValue->setText(afStr);
	
} //IbsCfgDlg::setProperties


void IbsCfgDlg::setProfile (ProfileCollection *pProfiles, QString name)
{
	m_profileName = name;
	m_pProfiles = pProfiles;
	IBS_OPTIONS ibsOptions;
	if (!m_pProfiles->getProfileConfig (name, &ibsOptions))
		QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());

	m_pProfileName->setText (m_profileName);
	m_pFetchInterval->setText (QString::number (ibsOptions.fetchInterval));
	m_pFetch->setChecked (ibsOptions.fetchSample);
	m_pOpInterval->setText (QString::number (ibsOptions.opInterval));
	m_pOp->setChecked (ibsOptions.opSample);

	oprofile_interface opIf;
	if (opIf.checkDCMissInfoInDaemon() > 0) 
		m_pDcMissInfoEnabled->setChecked (ibsOptions.dcMissInfoEnabled);
	else
		m_pDcMissInfoEnabled->hide();

	// Check REV C support
	char VendorId[15];
	unsigned long Family;
	unsigned long Model;
	unsigned long Stepping;
	unsigned long Features;
	CpuId( VendorId, &Family, &Model, &Stepping, &Features );

	if ((GREYHOUND_FAMILY== Family) && (REV_C_MODEL <= Model))
	{
		m_dispatchAvail = true;
		m_pOpCycle->setEnabled (true);
		m_pOpCycle->setChecked (ibsOptions.opCycleCount);
		m_pOpDispatch->setEnabled (true);
		m_pOpDispatch->setChecked (!ibsOptions.opCycleCount);
	} else {
		m_dispatchAvail = false;
		m_pOpCycle->setEnabled (true);
		m_pOpCycle->setChecked (true);
		m_pOpDispatch->setEnabled (false);
		m_pOpDispatch->setChecked (false);
	}

	if (opIf.checkIbsSupportInDaemon() >= 2) {
		// Load events to IBS Fetch List
		QString file = helpGetEventFile(Family, Model);
		if( !m_eventsFile.open (file) )
		{
			QMessageBox::information( this, "Error", 
				"Unable to open the events file: " + file);
		}
		addIbsEventFromFileToList(Model);

		selectIbsEvents(&ibsOptions);
	} else {
		m_pFetchList->hide();
		m_pSelectAllFetch->hide();
		m_pOpList->hide();
		m_pSelectAllOp->hide();
		this->resize(0,0);
	}
	
	m_pLaunchBox->hide();
	m_modified = false;
		
} //IbsCfgDlg::setProfile

void IbsCfgDlg::onOk ()
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
			QMessageBox::Yes, QMessageBox::No) == QMessageBox::No))
		{
			//Set the focus, so the user can change the name
			m_pProfileName->setFocus ();
			return;
		}

		// Check if either type is checked
		if ((!m_pFetch->isChecked ()) && (! m_pOp->isChecked ()))
		{
			QMessageBox::warning (this, "CodeAnalyst warning",
				"At least one sampling method needs to be selected for a valid profile.");
			m_pOp->setFocus ();
			return;
		}

		// Check Fetch Interval	
		if (m_pFetch->isChecked()
			&& (m_pFetchInterval->text().toULong() < CA_IBS_FETCH_MIN
			||  m_pFetchInterval->text().toULong() > CA_IBS_FETCH_MAX))
		{
			QString msg = QString("IBS Fetch Interval : ") 
				+ m_pFetchInterval->text() 
				+ " is not a valid.\n"
				+ "Please specify value between "
				+ QString::number(CA_IBS_FETCH_MIN)
				+ " and "
				+ QString::number(CA_IBS_FETCH_MAX)
				+ ".\n";
			QMessageBox::warning (this, "CodeAnalyst warning",msg);
			m_pFetch->setFocus ();
			return;
		}

		// Check Op Interval	
		if (m_pOp->isChecked()
			&& (m_pOpInterval->text().toULong() < CA_IBS_FETCH_MIN
			||  m_pOpInterval->text().toULong() > CA_IBS_FETCH_MAX))
		{
			QString msg = QString("IBS Op Interval : ")
				+ m_pOpInterval->text() 
				+ " is not a valid.\n"
				+ "Please specify value between "
				+ QString::number(CA_IBS_FETCH_MIN)
				+ " and "
				+ QString::number(CA_IBS_FETCH_MAX)
				+ ".\n";
			QMessageBox::warning (this, "CodeAnalyst warning",msg);
			m_pOp->setFocus ();
			return;
		}

		//Save modified profile
		IBS_OPTIONS ibsOptions;
		ibsOptions.fetchInterval = m_pFetchInterval->text().toULong ();
		ibsOptions.opInterval = m_pOpInterval->text().toULong ();
		ibsOptions.fetchSample = m_pFetch->isChecked ();
		ibsOptions.opSample = m_pOp->isChecked ();
		ibsOptions.opCycleCount = m_pOpCycle->isChecked ();
		ibsOptions.dcMissInfoEnabled = m_pDcMissInfoEnabled->isChecked ();

		IbsEventListViewItem * curItem;
		// Save Fetch List
		ibsOptions.fetchList.clear();
		for(curItem = (IbsEventListViewItem*) m_pFetchList->firstChild();
			curItem != NULL ;
			curItem = (IbsEventListViewItem*) curItem->nextSibling())
		{	
			if(curItem->isSelected()) {
				ibsOptions.fetchList.append(curItem->value);
			}
		}

		// Save Op List
		ibsOptions.opList.clear();
		for(curItem = (IbsEventListViewItem*) m_pOpList->firstChild();
			curItem != NULL ;
			curItem = (IbsEventListViewItem*) curItem->nextSibling())
		{
			if(curItem->isSelected()) {
				ibsOptions.opList.append(curItem->value);
			}
		}

		if(!m_pProfiles->setProfileConfig (m_pProfileName->text(), &ibsOptions))
		{
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
			m_pProfileName->setFocus();
			return;
		}	
	}
	accept();
} //IbsCfgDlg::onOk

bool IbsCfgDlg::wasModified ()
{
	return m_modified;
}

void IbsCfgDlg::addIbsEventFromFileToList(int model)
{
	EventList::const_iterator elit;
	for( elit = m_eventsFile.FirstEvent(); elit != m_eventsFile.EndOfEvents(); elit++ )
	{
		//Don't show events which are not valid on this cpu
		if (elit->m_minValidModel > model)
			continue;

		// Filter out non-IBS derived events (0xF00+)
		if(0xF000 <= elit->value && elit->value < 0xF100)	
		{
			IbsEventListViewItem * pItem = new IbsEventListViewItem( m_pFetchList, 
								elit->name,
								elit->value,
								elit->source);
			m_pFetchList->insertItem(pItem);
		} else if (0xF100 <= elit->value) {
			IbsEventListViewItem * pItem = new IbsEventListViewItem( m_pOpList, 
								elit->name,
								elit->value,
								elit->source);
			m_pOpList->insertItem(pItem);
		}	
	}
}

void IbsCfgDlg::addIbsEventFromProfileToList(int model, IBS_OPTIONS *pSession)
{
	EventList::const_iterator elit;
	for( elit = m_eventsFile.FirstEvent(); elit != m_eventsFile.EndOfEvents(); elit++ )
	{
		// Filter out non-IBS derived events (0xF00+)
		if (0xF000 <= elit->value && elit->value < 0xF100	
		&&  pSession->fetchList.find(elit->value) != pSession->fetchList.end()) 
		{
			IbsEventListViewItem * pItem = 
						new IbsEventListViewItem( m_pFetchList, 
							elit->name,
							elit->value,
							elit->source);
			m_pFetchList->insertItem(pItem);
		} else if (0xF100 <= elit->value
		       &&  pSession->opList.find(elit->value) != pSession->opList.end()) 
		{
			IbsEventListViewItem * pItem = 
						new IbsEventListViewItem( m_pOpList, 
							elit->name,
							elit->value,
							elit->source);
			m_pOpList->insertItem(pItem);
		}	
	}
}

void IbsCfgDlg::onSelectAllFetch()
{
	m_pFetchList->selectAll(true);
}

void IbsCfgDlg::onSelectAllOp()
{
	m_pOpList->selectAll(true);
}

void IbsCfgDlg::selectIbsEvents( IBS_OPTIONS * ibsOptions)
{
	bool isFetchSelected = false;
	bool isOpSelected = false;
	IbsEventListViewItem * curItem = NULL;

	// Fetch
	curItem = (IbsEventListViewItem *)m_pFetchList->firstChild();

	Q3ValueList<unsigned int>::iterator it = ibsOptions->fetchList.begin();
	Q3ValueList<unsigned int>::iterator it_end = ibsOptions->fetchList.end();
	while(it != it_end)
	{
		if(curItem->value == *it) {
			m_pFetchList->setSelected(curItem,true);
			isFetchSelected = true;
			if(it != it_end)
				it++;
			curItem = (IbsEventListViewItem*) curItem->nextSibling();
		} else if (curItem->value < *it) {
			m_pFetchList->setSelected(curItem,false);
			curItem = (IbsEventListViewItem*) curItem->nextSibling();
		} else if (curItem->value > *it) {
			if(it != it_end)
				it++;		
		}
	}
	
	// Op
	curItem = (IbsEventListViewItem *)m_pOpList->firstChild();

	it = ibsOptions->opList.begin();
	it_end = ibsOptions->opList.end();
	while(it != it_end)
	{
		if(curItem->value == *it) {
			m_pOpList->setSelected(curItem,true);
			isOpSelected = true;
			if(it != it_end)
				it++;
			curItem = (IbsEventListViewItem*) curItem->nextSibling();
		} else if (curItem->value < *it) {
			m_pOpList->setSelected(curItem,false);
			curItem = (IbsEventListViewItem*) curItem->nextSibling();
		} else if (curItem->value > *it) {
			if(it != it_end)
				it++;		
		}
	}

	// Sanity check if no selection
	if(m_pFetch->isChecked() && !isFetchSelected)
		m_pFetchList->setSelected(m_pFetchList->firstChild(),true);
	if(m_pOp->isChecked() && !isOpSelected)
		m_pOpList->setSelected(m_pOpList->firstChild(),true);
		
	
}
