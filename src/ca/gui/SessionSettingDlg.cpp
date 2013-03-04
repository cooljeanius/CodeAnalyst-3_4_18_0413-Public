//$Id: SessionSettingDlg.cpp 16863 2009-07-22 18:07:49Z ssuthiku $
//  This is the implementation for the SessionSettingDlg class, which derives from
//	ISessionOptions and has different functionality for express vs normal mode

/**
*   (c) 2006 Advanced Micro Devices, Inc.
*  YOUR USE OF THIS CODE IS SUBJECT TO THE TERMS
*  AND CONDITIONS OF THE GNU GENERAL PUBLIC
*  LICENSE FOUND IN THE "GPL.TXT" FILE THAT IS
*  INCLUDED WITH THIS FILE AND POSTED AT
*  <http://www.gnu.org/licenses/gpl.html>
*  Support: codeanalyst@amd.com
*/
#include <unistd.h>
#include <q3textedit.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <QTextStream>
#include <qradiobutton.h>
#include <Q3PopupMenu>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "SessionSettingDlg.h"
#include "auxil.h"
#include "TimerCfg.h"
#include "PerfTimerCfg.h"
#include "EventCfgDlg.h"
#include "atuneoptions.h"
#include "helperAPI.h"
#include "stdafx.h"
#include "ProcessFilterDlg.h"
#include "elf.h"
#include "helperAPI.h"
#include "atuneoptionsdlg.h"
#include "affinity.h"

SessionSettingDlg::SessionSettingDlg (CawFile *pCawFile, 
					CEventsFile * pEventsFile,
					QWidget* parent,
					const char* name, 
					bool modal, 
					Qt::WindowFlags fl)
: QDialog (parent, name, modal, fl|CA_DLG_FLAGS)
{
	setupUi(this);
	m_pCawFile = pCawFile;
	m_pProfiles = NULL;
	m_lastDuration = "20";
	m_lastDurationChecked = false;
	m_pAffinityDlg  = NULL;
	m_pCurrentSetting = NULL;
	m_pEventsFile = pEventsFile;
	m_noChange = false;
	m_isAccept = false;
	m_isProperty = false;


	// Setting up slots
	connect (m_pSave, SIGNAL (clicked()), SLOT (onSave()));
	connect (m_pOk, SIGNAL (clicked()), SLOT (onOk()));
	connect (m_pCancel, SIGNAL (clicked()), SLOT (reject()));

	//-----------------
	// Setting Explorer

	QObject::connect(m_pSettingExplorer, 
		SIGNAL (rightButtonClicked (Q3ListBoxItem *, const QPoint &)),
		SLOT (onSettingRightClicked (Q3ListBoxItem *, const QPoint &)) );


	m_pSettingMenu = new Q3PopupMenu ( m_pSettingExplorer);

	if (NULL != m_pSettingMenu) {
		m_pSettingMenu->insertItem( "&Duplicate", this, 
			SLOT (onSettingDuplicate()) );
		m_pSettingMenu->insertItem ("&Rename", this, 
			SLOT (onSettingRename()) );
		m_pSettingMenu->insertItem ("De&lete", this, 
			SLOT (onSettingRemove()));
	}


	connect (m_pSettingExplorer, SIGNAL (currentChanged(Q3ListBoxItem *)), 
		SLOT (onSettingChange(Q3ListBoxItem *)));
	connect (m_pSettingRemove, SIGNAL (clicked()), SLOT (onSettingRemove()));
	

	//-----------------
	// General Tab
	connect (m_pLaunch, SIGNAL (textChanged (const QString &)), 
		SLOT (onLaunchChange (const QString &)));
	connect (m_pBrowseLaunch, SIGNAL (clicked ()), 
		SLOT (onBrowseLaunch()));
	connect (m_pBrowseWorkDir, SIGNAL (clicked ()), 
		SLOT (onBrowseWorkDir()));
	connect (m_pProfileDropBox, SIGNAL (activated (const QString &)), 
		SLOT (onProfileChange (const QString &)));
	connect (m_pProfileEdit, SIGNAL (clicked ()),
		SLOT (onProfileEdit()));
	connect (this, SIGNAL (enableLaunchItems (bool)), 
		m_pStopOnExit, SLOT (setEnabled (bool)));
	connect (this, SIGNAL (enableLaunchItems (bool)), 
		m_pTerminateAfter, SLOT (setEnabled (bool)));
	connect (this, SIGNAL (enableLaunchItems (bool)), 
		m_pShowTerminal, SLOT (setEnabled (bool)));
	connect (m_pStopOnExit, SIGNAL (toggled (bool)), 
		this, SLOT (onToggleStopOnExit (bool)));
	connect (m_pProfileDuration, SIGNAL (clicked()),
		SLOT (onCheckDuration()));

	// Affinity
	if (CA_Affinity::isSupported()) {
		connect (this, SIGNAL (enableLaunchItems (bool)), 
			m_pAffinityEnable, SLOT (setEnabled (bool)));
		connect (m_pAffinityEnable, SIGNAL (toggled(bool)), 
			SLOT(onToggleAffinityEnable(bool)));
		connect (m_pAffinitySelect, SIGNAL (clicked ()), 
			SLOT(onCpuAffinity()));
	} else {
		onToggleAffinityEnable(false);

		m_pAffinityEnable->setEnabled(false);
		m_pAffinityValue->setEnabled(false);
		m_pAffinitySelect->setEnabled(false);
	}

	// Filter
	connect (m_pProcessFilterEnable, SIGNAL (toggled(bool)), 
		SLOT(onToggleApplyFilter(bool)));
	connect (m_pProcessFilterAdvance, SIGNAL (clicked ()), 
		SLOT(onSetAdvanceFilter()));

	
	//-----------------
	// Advance Tab

	// CSS
	connect (m_pCssEnableBox, SIGNAL (toggled(bool)), 
		SLOT(onToggleCssEnable(bool)));
	connect (m_pCssUseTgid, SIGNAL (toggled(bool)), 
		SLOT(onToggleCssUseTgid(bool)));

	// vmlinux
	connect (m_pVmlinuxEnable, SIGNAL (toggled(bool)),
		SLOT(onToggleVmlinuxEnable(bool)));
	connect (m_pVmlinuxBrowse, SIGNAL (clicked ()), 
		SLOT(onVmlinuxBrowse()));
	
	// OProfile buffer
	connect (m_pOpResetBufferSize, SIGNAL (clicked ()), 
		SLOT(onResetBufferSize()));

	//-----------------
	// Note Tab
	connect (m_pSessionNoteClear, SIGNAL (clicked ()), 
		SLOT(onClearSessionNote()));

	//-----------------
	// Info Tab

} //SessionSettingDlg::SessionSettingDlg


SessionSettingDlg::~SessionSettingDlg()
{
	
}

void SessionSettingDlg::onToggleStopOnExit (bool b) {
	if (b) {
		m_pProfileDuration->setEnabled (b);
			m_pProfileDuration->setChecked(m_lastDurationChecked);	
			onCheckDuration();
	} else {
		if (m_pProfileDuration->isChecked()) {
			m_lastDurationChecked = true;
			m_pProfileDuration->setChecked(false);	
			onCheckDuration();
		}else{
			m_lastDurationChecked = false;
		}
		m_pProfileDuration->setEnabled (b);
	}
}

void SessionSettingDlg::onProfileEdit ()
{
	//Get index from m_pProfileDropBox
	m_pProfiles->clearLastModified ();
	QString profile = m_pProfileDropBox->currentText();
	TRIGGER type = m_pProfiles->getProfileType (profile);
	int ret = QDialog::Rejected;
	switch (type)
	{
	case TIMER_TRIGGER: {
		TimerCfgDlg *pTimer = new TimerCfgDlg (this);
		RETURN_IF_NULL (pTimer, this);
		pTimer->setProfile (m_pProfiles, profile);
		ret = pTimer->exec ();
		delete pTimer;
		break; 
	}
	case PERF_TIMER_TRIGGER: {
		PerfTimerCfgDlg *pTimer = new PerfTimerCfgDlg (this);
		RETURN_IF_NULL (pTimer, this);
		pTimer->setProfile (m_pProfiles, profile);
		ret = pTimer->exec ();
		delete pTimer;
		break; 
	}
	case EVENT_TRIGGER: {
		EventCfgDlg *pEventDlg = new EventCfgDlg (this);
		RETURN_IF_NULL (pEventDlg, this);
		pEventDlg->setProfile (m_pProfiles, profile);
		ret = pEventDlg->exec ();
		delete pEventDlg;
		break; 
	}
	default:
		QMessageBox::information (this, 
			"CodeAnalyst Error", 
			"Profile selected is not supported. Please select new profile.");
		break;
	}

	m_pProfileDropBox->clear();
	m_pProfileDropBox->insertStringList (m_pProfiles->getListOfProfiles ());
	
	QString curProfile = m_pProfiles->getLastModified ();
	if (!curProfile.isEmpty())
	{
		// Search if exists
		for (int i = m_pProfileDropBox->count()-1 ; i >= 0 ; i--)
		{
			if(m_pProfileDropBox->text(i) == curProfile)
			{
				m_pProfileDropBox->setCurrentItem (i);
			}
		}
	} else {
		// Search if exists
		for (int i = m_pProfileDropBox->count()-1 ; i >= 0 ; i--)
		{
			if(m_pProfileDropBox->text(i) == profile)
			{
				m_pProfileDropBox->setCurrentItem (i);
			}
		}
	}
	onProfileChange(m_pProfileDropBox->currentText());
} //SessionSettingDlg::onClickEditProfile


void SessionSettingDlg::onBrowseWorkDir ()
{
	QString     ProjPath;

	//ProjPath = QFileDialog::getExistingDirectory();
	//If we want to use the shortcuts, we'll need to use SHBrowseForFolder
	ProjPath = Q3FileDialog::getExistingDirectory ( m_pWorkDir->currentText(), 
		this, NULL, "Please select a working directory.");

	// Make sure the project directory format is valid
	if( !ProjPath.isNull() )
	{
		ProjPath.replace ('\\', '/');
		m_pWorkDir->setEditText ( ProjPath );
	}
}

void SessionSettingDlg::onBrowseLaunch ()
{
	QString start;
	if (!m_pLaunch->currentText().isEmpty())
	{
		start = m_pLaunch->currentText().section ('\"', 1, 1);
	}
	QString Prog = Q3FileDialog::getOpenFileName( start, 
		"All files (*)", this, "FileBrowse", 
		"Select an application to launch" );

	if( Prog.isEmpty() ) 
		return;
	/*just in case, all paths use / instead of \ */
	Prog.replace ("\\", "/");
	m_pLaunch->setEditText( "\"" + Prog + "\"" );
}


void SessionSettingDlg::onLaunchChange (const QString & string)
{
	if (!string.isEmpty())
	{
		QString temp = string.section ('\"', 1, 1);
		if (m_pWorkDir->currentText().isEmpty())
			m_pWorkDir->setEditText (temp.section ('/', 0, -2));
		if (QFile::exists (temp))
		{
			emit enableLaunchItems (true);
		} else {
			emit enableLaunchItems (false);
		}
	} else {
		m_pStopOnExit->setChecked (false);
		m_pTerminateAfter->setChecked (false);
		m_pProfileDuration->setChecked (false);
		m_pShowTerminal->setChecked (false);
		if (CA_Affinity::isSupported()) {
			m_pAffinityEnable->setChecked (false);
		}
	
		onCheckDuration ();
		emit enableLaunchItems (false);
	}

	// Reset filter when launch application is changed, 
	m_filterList.clear();

	// If filter is enable, reset process filter to the launch application.
	if(isApplyProcessFilter())
	{
		onToggleApplyFilter(true);
	}

}

QString SessionSettingDlg::getEventName(unsigned int value)
{
	CpuEvent event;
	
	if (!m_pEventsFile)
		return QString();

	m_pEventsFile->findEventByValue( value, event);
	return event.name;	
}


void SessionSettingDlg::setupProfileViewer(TBP_OPTIONS opt)
{
	unsigned int tickPerSec = getTick();
		
	Q3ListViewItem * item = new Q3ListViewItem(m_pProfileView);
	item->setText(0, QString("0x0076"));
	item->setText(1, QString::number(
		(unsigned long long)(tickPerSec / 1000 * opt.msInterval))
		+ " (" +QString::number(opt.msInterval) + " msec)");
	item->setText(2, QString("0x0")); 
	item->setText(3, getEventName(0x76));
	m_pProfileView->insertItem(item);
}


void SessionSettingDlg::setupProfileViewer(PERF_TBP_OPTIONS opt)
{
	unsigned int tickPerSec = getTick();
		
	Q3ListViewItem * item = new Q3ListViewItem(m_pProfileView);
	item->setText(0, QString("Software Time Based Profiling"));
	item->setText(1, QString::number(opt.msInterval) + " msec");
	item->setText(3, QString("PERF Sofware CPU Clock Event"));
	m_pProfileView->insertItem(item);
}

void SessionSettingDlg::setupProfileViewer(EBP_OPTIONS opt)
{
	PerfEventList::iterator it = opt.getEventContainer()->getPerfEventListBegin();
	PerfEventList::iterator it_end = opt.getEventContainer()->getPerfEventListEnd();
	for (; it != it_end; it++) {
		Q3ListViewItem * item = new Q3ListViewItem(m_pProfileView);
		item->setText(0, QString().sprintf("0x%04x",(*it).select()));
		item->setText(1, QString::number((*it).count));
		item->setText(2, QString().sprintf("0x%02x",(*it).umask()));
		item->setText(3, getEventName((*it).select()));
		m_pProfileView->insertItem(item);
	}
}


void SessionSettingDlg::onProfileChange (const QString &orgProfile)
{
	// Show Events in the m_pProfileView
	QString profile;

	if (orgProfile.isEmpty()) {
		if (m_pProfileDropBox->count() > 0)
			profile = m_pProfileDropBox->text(0);
	} else {
		if (m_pProfiles) {	
			if (m_pProfiles->profileExists(orgProfile)) {
				profile = orgProfile;
			} else {
				QMessageBox::warning(this, "CodeAnalyst Warning", 
					QString("Profile Configuration (") 
					+ orgProfile 
					+") is not supported, invalid, or no longer exist.\n");
				profile = m_pProfileDropBox->text(0);
			}
		}
	}

	if (profile.isEmpty())
		return;

	m_pProfileView->clear();

	TRIGGER type = m_pProfiles->getProfileType (profile);
	bool bOpEnable = true;

	switch (type)
	{
	case TIMER_TRIGGER: {
		TBP_OPTIONS tbpOpt;
		if (!m_pProfiles->getProfileConfig( profile, &tbpOpt))
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
		setupProfileViewer(tbpOpt);
		break; 
	} 
	case PERF_TIMER_TRIGGER: {
		PERF_TBP_OPTIONS tbpOpt;
		if (!m_pProfiles->getProfileConfig( profile, &tbpOpt))
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
		setupProfileViewer(tbpOpt);
		bOpEnable = false; 
		break; 
	} 
	case EVENT_TRIGGER: {
		EBP_OPTIONS ebpOpt;
		if (!m_pProfiles->getProfileConfig( profile, &ebpOpt))
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pProfiles->getErrorMsg());
		setupProfileViewer(ebpOpt);
		break;
	} 
	default:
		QMessageBox::information (this, 
			"CodeAnalyst Error", 
			QString("Profile Configuration (") + orgProfile 
				+") is not supported.\n."
				+ "Please select a new one.");
		break;
	}
	m_pOpBufSize->setEnabled(bOpEnable);
	m_pOpWatershedSize->setEnabled(bOpEnable);
	m_pOpCpuBufSize->setEnabled(bOpEnable);
	m_pVmlinuxEnable->setEnabled(bOpEnable); 
}


void SessionSettingDlg::onSave ()
{
	if (m_isProperty)
		onSaveProperty();
	else
		onSaveSetting();
}


void SessionSettingDlg::onSaveProperty ()
{
	m_pCawFile->updateSessionNote(m_propertyTrigger, m_pName->text(), m_pSessionNote->text());
	m_pCawFile->saveCawFile();
	m_isAccept = true;	
	return;
}


void SessionSettingDlg::onSaveSetting ()
{
	if (!validateCurrentSetting()) {
		return;
	}

	// Save Launch History
	m_pCawFile->addLaunchHistory(m_pLaunch->currentText());	
	
	// Save Working Dir History
	m_pCawFile->addWorkDirHistory(m_pWorkDir->currentText());

	int index    = m_pSettingExplorer->currentItem();
	QString name = m_pSettingExplorer->text(index);
	if (name.compare(m_pName->text()) != 0) {

		// Create new setting
		RUN_SETTING * pRunSetting = new RUN_SETTING();
		if (pRunSetting == NULL)
			return;

		// Copy From current setting
		saveSettingFromGui(pRunSetting);		

		// Remove existing setting with the same name 
		if(m_pCawFile->getRunSetting (m_pName->text()))
			onSettingRemove(m_pName->text());	

		// Save new setting
		onSettingAdd(pRunSetting->sessionName, pRunSetting);
	} else {
		if (!saveToCawFile()) {
			return;
		}
	}
	m_isAccept = true;
}


void SessionSettingDlg::onOk()
{
	m_isAccept = false;
	onSave();
	if (m_isAccept)
		accept();
}


void SessionSettingDlg::onCheckDuration()
{
	if (m_pProfileDuration->isChecked())
	{
		m_lastDuration = m_pDuration->text();
		m_pDuration->setText ("0");
		m_pDuration->setEnabled (false);
	} else {
		m_pDuration->setEnabled (true);
		if (m_pDuration->text().toUInt() > 0)
			return;	
		else if (m_lastDuration.toUInt() > 0)
			m_pDuration->setText (m_lastDuration);
		else
			m_pDuration->setText ("20");
	}
}


void SessionSettingDlg::updateWithCurrentSetting()
{
	int dcConfigIndex = 0;
	if (m_pCurrentSetting == NULL) {
		return;
	}

	//-----------------
	// Name
	m_pName->setText(m_pCurrentSetting->sessionName);

	//-----------------
	// General Tab
	m_pLaunch->clear();

	if (!m_isProperty) {
		m_pLaunch->insertStringList ( m_pCawFile->getLaunchHistory());
		m_pLaunch->setCurrentText (m_pCurrentSetting->launch);
	} else {
		//m_pLaunch->insertStringList (m_pCurrentSetting->launch );
		m_pLaunch->insertItem (m_pCurrentSetting->launch );
	}

	m_pWorkDir->clear();
	if (!m_isProperty) {
		m_pWorkDir->insertStringList (m_pCawFile->getWorkDirHistory());
		m_pWorkDir->setCurrentText (m_pCurrentSetting->workDir);
	} else {
		m_pWorkDir->insertItem (m_pCurrentSetting->workDir);
	}

	m_pStopOnExit->setChecked (m_pCurrentSetting->stopOnAppExit);
	onToggleStopOnExit((m_pCurrentSetting->stopOnAppExit == 1));
	m_pTerminateAfter->setChecked (m_pCurrentSetting->termAppOnDur);
	m_pStartPaused->setChecked (m_pCurrentSetting->startPaused);
	m_pProfileDuration->setChecked (
		(m_pCurrentSetting->duration == 0) && (!m_pCurrentSetting->launch.isEmpty()));
	m_pDuration->setText (QString::number (m_pCurrentSetting->duration));
	onCheckDuration ();
	m_pStartDelay->setText (
		(m_pCurrentSetting->startDelay < 0)? "0" : 
			QString::number (m_pCurrentSetting->startDelay));

	for (int i = 0 ; i < m_pProfileDropBox->count(); i++) {
		if (m_pCurrentSetting->dcConfig.compare(m_pProfileDropBox->text(i)) == 0) {
			dcConfigIndex = i;
			break;
		}
	}
	
	m_pProfileDropBox->setCurrentItem(dcConfigIndex);
	onProfileChange (m_pCurrentSetting->dcConfig);

	// Process Filter
	m_filterList = m_pCurrentSetting->filterList.split(",");
	if(m_pCurrentSetting->filterEnable) {
		m_pProcessFilterEnable->setChecked(true);
		m_pProcessFilterAdvance->setEnabled(true);
	}else{
		m_pProcessFilterEnable->setChecked(false);
		m_pProcessFilterAdvance->setEnabled(false);
	}

	// CPU Affinity 
	if (CA_Affinity::isSupported()) {
		if (m_pCurrentSetting->affinity.isZero())
			m_pCurrentSetting->affinity.setAllCpus();
		QString strTmpAFMask = QString::fromStdString(m_pCurrentSetting->affinity.getAffinityMaskInHexString());
		QString strFormat("0x");
		QString afStr = QString("%1%2").arg(strFormat,strTmpAFMask);
		m_pAffinityValue->setText(afStr);

		onToggleAffinityEnable(m_pCurrentSetting->affinityEnable);
	} else {
		onToggleAffinityEnable(false);
		m_pAffinityValue->setText("0x0");
	}

	if (!helpFindXterm().isEmpty()) {
		// Show Terminal
		m_pShowTerminal->setChecked(m_pCurrentSetting->showTerminalEnable);
	} else {
		m_pShowTerminal->setEnabled(false);
	}

	//-----------------
	// Advance Tab

	// CSS
	m_pCssEnableBox->setChecked(m_pCurrentSetting->cssEnable);
	bool bTgid = m_pCurrentSetting->cssUseTgid;
	if (bTgid) {
		if (m_pCurrentSetting->cssTgid > 0) 
			m_pCssUseTgid->setChecked(true);
		else	
			m_pCssUseLaunchedTgid->setChecked(true);
	}

	if (m_pCurrentSetting->opWaterShed == -1 
	||  m_pCurrentSetting->opBuffer == -1 
	||  m_pCurrentSetting->opCpuBuffer == -1) {
		m_pCssDepth->setText("10");
		m_pCssInterval->setText("1000");
	} else {
		m_pCssDepth->setText(QString::number(m_pCurrentSetting->cssDepth, 10));
		m_pCssInterval->setText(QString::number(m_pCurrentSetting->cssInterval, 10));
	}
	onToggleCssEnable(m_pCurrentSetting->cssEnable);

	// vmlinux
	m_pVmlinuxEnable->setChecked(m_pCurrentSetting->vmlinuxEnable); 
	m_pVmlinuxPath->setText(m_pCurrentSetting->vmlinux); 

	// OProfile
	if (m_pCurrentSetting->opWaterShed == -1 
	||  m_pCurrentSetting->opBuffer == -1 
	||  m_pCurrentSetting->opCpuBuffer == -1)
		onResetBufferSize();
	else {	
		m_pOpWatershedSize->setText (QString::number (m_pCurrentSetting->opWaterShed));
		m_pOpBufSize->setText (QString::number (m_pCurrentSetting->opBuffer));
		m_pOpCpuBufSize->setText (QString::number (m_pCurrentSetting->opCpuBuffer));
	}

	//-----------------
	// Note Tab
	m_pSessionNote->setText (m_pCurrentSetting->sessionNote);

	//-----------------
	// Info Tab

} // void SessionSettingDlg::updateWithCurrentSetting()


void SessionSettingDlg::saveSettingFromGui(RUN_SETTING * pCurrentSetting)
{
	if (pCurrentSetting == NULL) {
		return;
	}

	pCurrentSetting->sessionName  	= m_pName->text();
	//-----------------
	// General Tab
	pCurrentSetting->launch 	= m_pLaunch->currentText();
	pCurrentSetting->workDir	= m_pWorkDir->currentText();
	pCurrentSetting->stopOnAppExit 	= m_pStopOnExit->isChecked();
	pCurrentSetting->termAppOnDur	= m_pTerminateAfter->isChecked();
	pCurrentSetting->startPaused	= m_pStartPaused->isChecked();
	pCurrentSetting->duration	= m_pDuration->text().toInt();
	pCurrentSetting->startDelay	= m_pStartDelay->text().toInt();
	pCurrentSetting->dcConfig	= m_pProfileDropBox->currentText();
	
	// Process Filter
	pCurrentSetting->filterEnable 	= m_pProcessFilterEnable->isChecked();
	pCurrentSetting->filterList	= m_filterList.join(",");

	// Affinity
	pCurrentSetting->affinityEnable = m_pAffinityEnable->isChecked();

	pCurrentSetting->affinity.setAffinityFromHexString(m_pAffinityValue->text().toStdString());

	if (!helpFindXterm().isEmpty()) {
		// Show Terminal
		pCurrentSetting->showTerminalEnable	= m_pShowTerminal->isChecked();
	} else {
		pCurrentSetting->showTerminalEnable	= false;
	}

	//-----------------
	// Advance Tab

	// CSS
	pCurrentSetting->cssEnable	= m_pCssEnableBox->isChecked();
	pCurrentSetting->cssDepth 	= m_pCssDepth->text().toInt();
	pCurrentSetting->cssInterval	= m_pCssInterval->text().toInt();
	pCurrentSetting->cssUseTgid	= m_pCssUseTgid->isChecked() | 
					  m_pCssUseLaunchedTgid->isChecked();
	pCurrentSetting->cssTgid	= m_pCssTgid->text().toInt();
	
	// vmlinux
	pCurrentSetting->vmlinuxEnable = m_pVmlinuxEnable->isChecked();
	pCurrentSetting->vmlinux	= m_pVmlinuxPath->text();

	// OProfile
	pCurrentSetting->opWaterShed	= m_pOpWatershedSize->text().toInt();
	pCurrentSetting->opBuffer	= m_pOpBufSize->text().toInt();
	pCurrentSetting->opCpuBuffer	= m_pOpCpuBufSize->text().toInt();

	//-----------------
	// Note Tab
	pCurrentSetting->sessionNote	= m_pSessionNote->text();
} //void SessionSettingDlg::saveSettingFromGui()


void SessionSettingDlg::initFromCawFile(ProfileCollection * pProfiles)
{

	//fill in profiles
	fillProfiles (pProfiles);
	m_pSave->setFocus();
	
	//-----------------
	// SettingExplorer
	QStringList settingList = m_pCawFile->getSettingList();

	if (settingList.count() == 0) {
		// No setting use default everything
		onSettingAdd();
	} else {
		m_pSettingExplorer->insertStringList(settingList);
		QString lastSettingName = m_pCawFile->getLastRunSettingName(); 
		
		Q3ListBoxItem * pItem = NULL;
		if (!lastSettingName.isEmpty()
		&& ((pItem = m_pSettingExplorer->findItem(lastSettingName
							, Q3ListBox::ExactMatch)) != NULL)) 
		{
			onSettingChange(pItem);
		} else {
			onSettingChange(m_pSettingExplorer->item(0));
		}
	}

	// Hide Log tab
	m_pSettingTab->removePage(m_pSettingTab->page(TAB_LOG));
}


void SessionSettingDlg::initPropertyView(QString sessionName, TRIGGER trigger)
{
	m_isProperty = true;
	m_propertyTrigger = trigger;

	setCaption(QString("Sssion Property : ") + sessionName);

	//-----------------
	// Set Readonly (need to be done first before adding items)
	m_pLaunch->setEditable(false);
	m_pWorkDir->setEditable(false);

	
	switch (trigger)
	{
	case TIMER_TRIGGER: {
		TBP_OPTIONS tbpOpt;
		m_pCawFile->getSessionDetails(sessionName, &tbpOpt);
		m_pCurrentSetting = (SESSION_OPTIONS *) &tbpOpt;
		setupProfileViewer(tbpOpt);
		importOprofileLog(sessionName + ".tbp");
		break; 
	} 
	case PERF_TIMER_TRIGGER: {
		PERF_TBP_OPTIONS tbpOpt;
		m_pCawFile->getSessionDetails(sessionName, &tbpOpt);
		m_pCurrentSetting = (SESSION_OPTIONS *) &tbpOpt;
		setupProfileViewer(tbpOpt);
		importOprofileLog(sessionName + ".tbp");
		break; 
	} 
	case EVENT_TRIGGER: {
		EBP_OPTIONS ebpOpt;
		m_pCawFile->getSessionDetails(sessionName, &ebpOpt);
		m_pCurrentSetting = (SESSION_OPTIONS *) &ebpOpt;
		setupProfileViewer(ebpOpt);
		importOprofileLog(sessionName + ".ebp");
		break;
	} 
	default:
		QMessageBox::information (this, 
			"CodeAnalyst Error", 
			QString("Data Collection Configuration for session (") + sessionName 
				+") is not supported.\n");
		break;
	}

	updateWithCurrentSetting();
	
	//-----------------
	// Set Readonly
	m_pNameLabel->setText("Session Name");
	m_pName->setReadOnly(true);
	m_pStartDelay->setReadOnly(true);
	m_pDuration->setReadOnly(true);
	m_pAffinityValue->setReadOnly(true);
	m_pCssDepth->setReadOnly(true);
	m_pCssInterval->setReadOnly(true);
	m_pCssTgid->setReadOnly(true);
	m_pVmlinuxPath->setReadOnly(true);
	m_pOpWatershedSize->setReadOnly(true);
	m_pOpBufSize->setReadOnly(true);
	m_pOpCpuBufSize->setReadOnly(true);
	m_pLog->setReadOnly(true);

	m_pTerminateAfter->setEnabled(false);
	m_pShowTerminal->setEnabled(false);
	m_pStopOnExit->setEnabled(false);
	m_pProfileDuration->setEnabled(false);
	m_pStartPaused->setEnabled(false);
	m_pAffinityEnable->setEnabled(false);
	m_pProcessFilterEnable->setEnabled(false);
	m_pCssEnableBox->setEnabled(false);
	m_pCssUseLaunchedTgid->setEnabled(false);
	m_pCssUseTgid->setEnabled(false);
	m_pVmlinuxEnable->setEnabled(false);

	//-----------------
	// Hide unused widget
	m_pProfileEdit->hide();
	m_pProfileDropBox->hide();
	m_pExplorerGrpBox->hide();
	m_pBrowseLaunch->hide();
	m_pBrowseWorkDir->hide();
	m_pVmlinuxBrowse->hide();
	m_pAffinitySelect->hide();
	m_pProcessFilterAdvance->hide();
	m_pOpResetBufferSize->hide();
	m_pSessionNoteClear->hide();
	m_pSave->hide();
	
}


/*virtual */ 
void SessionSettingDlg::fillProfiles (ProfileCollection * pProfiles)
{
	m_pProfiles = pProfiles;
	m_pProfileDropBox->insertStringList (m_pProfiles->getListOfProfiles ());
	QString curProfile = m_pCawFile->getLastProfile ();
	if (!curProfile.isEmpty())
	{
		// Search if exists
		for (int i = m_pProfileDropBox->count()-1 ; i >= 0 ; i--)
		{
			if(m_pProfileDropBox->text(i) == curProfile)
			{
				m_pProfileDropBox->setCurrentItem (i);
			}
		}
	} else {
		m_pProfileDropBox->setCurrentItem (0);
	}
	curProfile = m_pProfileDropBox->currentText ();
}


bool SessionSettingDlg::saveToCawFile()
{
	if (!m_pCurrentSetting)
		return false;

	if (! validateCurrentSetting()) {
		return false;
	}

	saveSettingFromGui(m_pCurrentSetting);
	if (!m_isProperty) {
		m_pCawFile->setLastRunSetting (m_pCurrentSetting->sessionName);
		m_pCawFile->setLastProfile (m_pProfileDropBox->currentText());
	}

	m_pCawFile->saveCawFile();
	return true;
}


bool SessionSettingDlg::validateCurrentSetting()
{
	/*
	 * Validate Template Name
	 */
	if (m_pName->text().isEmpty()) {
		QMessageBox::critical (this, "CodeAnalyst Error", 
			"Please specify template name.\n");
		m_pName->setFocus ();
		return false;
	}


	/*
	 * Validate launch
	 */
	//get the part of the launch string between the first set of quotes
	QString launchString = m_pLaunch->currentText ();
	QString launchApp = launchString.section ('\"', 1, 1);
	if (!launchApp.isEmpty()) {
		if (!QFile::exists (launchApp)) {
			QMessageBox::critical (this, "CodeAnalyst Error", 
				"Unable to find the application to launch, please locate it.");
			return false;
		}

#ifndef DISABLE_CA_JVMTIA
		if (launchApp.endsWith ("java")) {
		    verifyJavaAgent (verifyJavaBitness(launchApp));
		}
#endif

	} else {
		//if there is no "" in the launch string, notify the user
		if (!launchString.isEmpty()) {
			m_pLaunch->setFocus ();
			QMessageBox::critical (this, "CodeAnalyst Error", 
				QString("Your specified launch command")
				+ "needs to be formated like:\n"
				+ "\"<path>/<application>\" <arguments>");
			return false;
		}
	}

	/*
	 * Validate working dir
	 */
	if (!m_pWorkDir->currentText ().isEmpty()) {
		QDir working (m_pWorkDir->currentText ());
		if (!working.exists()) {
			QMessageBox::critical (this, "CodeAnalyst Error", 
				QString("Your specified working directory ")
				+ "does not exist, please choose an existing "
				+ "directory to launch from.");
			return false;
		}
	}

	/*
	 * Validate duration
	 */
	bool isOk = false;
	if(!m_pProfileDuration->isChecked()) {
		int duration = m_pDuration->text().toUInt(&isOk);

	 	if(!isOk || (duration == 0))
		{
			QMessageBox::critical(this, "CodeAnalyst Error",
				QString("Invalid profiling duration (")
				+ m_pDuration->text() + ")");	
			
			m_pDuration->setFocus();
			return false;
		}
	}

	/*
	 * Validate delay
	 */
	m_pStartDelay->text().toUInt(&isOk,0);
	if (!isOk) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			m_pStartDelay->text() + 
			" is an invalid input for start delay value.");	
		
		m_pStartDelay->setFocus();
		return false;
	}

	/*
	* Validate Affinity
	*/
	if ( CA_Affinity::isSupported()) {
		CA_Affinity test;
		if ( test.setAffinityFromHexString( m_pAffinityValue->text().toStdString()) != 0 
		||  test.isZero() // Must not be zero
		||  test.isValidForCurrentSystem() == false) // Must be valid for current system) 
		{ 
			QMessageBox::critical (this, "CodeAnalyst Error", 
						"Invalid CPU Affinity Bitmask.");

			m_pAffinityValue->selectAll();
			m_pAffinityValue->setFocus ();
			return false;
		}
	}
	/*
	 * Validate Process Filter
	 */
	if (m_pProcessFilterEnable->isChecked() && !validateProcessFilter())
		return false;

	/*
	 * Validate CSS
	 */
	if (m_pCssEnableBox->isChecked() && !validateCss()) {
		return false;	
	}

	/*
	 * Validate Vmlinux
	 */
	if (m_pVmlinuxEnable->isChecked() && !QFile::exists(m_pVmlinuxPath->text())) {
		QMessageBox::critical(this, "CodeAnalyst Error",
			QString("vmlinux path (") + m_pVmlinuxPath->text() + 
			") not found.");	
		m_pVmlinuxPath->setFocus();
		return false;
	}

	/*
	 * Validate OProfile buffer setting
	 */
	if (!validateOprofileBufferSetting())
		return false;

	return true;
} //SessionSettingDlg::validateCurrentSetting


void SessionSettingDlg::verifyJavaAgent (QString bitness)
{
	//get everything past the second quote
	QString arguments = m_pLaunch->currentText ().section ('\"', 2);

	//check for pi agent.
	if ((!arguments.contains ("libCAJVMPIA", false)) &&
		(!arguments.contains ("libCAJVMTIA", false)))
	{
		QString launch = m_pLaunch->currentText ();
		int secondquote	= launch.find ("\"", 1);

		QString CAJVMTIA_BIT_SO = CAJVMTIA + bitness + ".so";

		launch.insert (secondquote + 1, CAJVMTIA_BIT_SO);

		//Save so changing the launch string doesn't will the working dir
		QString saveWorkDir = m_pWorkDir->currentText ();
		m_pLaunch->setEditText (launch);
		m_pWorkDir->setEditText (saveWorkDir);
	}
} //SessionSettingDlg::verifyJavaAgent

bool SessionSettingDlg::isFile32Bit (const char* fileName)
{
	bool bRet = false;
	int ret = getBitness(fileName);
	if (ret == -1)
		goto system_default;
	else if (ret == 32)
		bRet = true;
	else if (ret == 64)
		bRet = false;
	
	goto out;

system_default:
#ifdef __x86_64__
        bRet = false;
#else
        bRet = true;
#endif

out:
	return bRet; 
} 

QString SessionSettingDlg::verifyJavaBitness (QString javaFile)
{
	if(isFile32Bit(javaFile.toAscii().data()))
	{
		return QString("32");	
	} else {
		return QString("64");	
	}
} //SessionSettingDlg::verifyJavaBitness

void SessionSettingDlg::onCpuAffinity()
{
	unsigned int numCpus = XpGetNumCpus();
	unsigned int numCpusPerSocket = XpGetNumCoresPerSocket();
	unsigned int numSockets = numCpus/numCpusPerSocket;


	m_pAffinityDlg = new CpuAffinityDlg(this);
	RETURN_IF_NULL(m_pAffinityDlg,this)

	m_pAffinityDlg->init(numSockets, numCpusPerSocket, m_pAffinityValue->text());

	if(QDialog::Accepted == m_pAffinityDlg->exec())
	{
		// If clicked OK, make changes to the session setting
		CA_Affinity * pAffinity = m_pAffinityDlg->getAffinityMask();
		QString afStr = QString("0x") + QString::fromStdString(pAffinity->getAffinityMaskInHexString());
		m_pAffinityValue->setText(afStr);
	}

	// Clean up
	if(m_pAffinityDlg)
	{
		delete m_pAffinityDlg;
		m_pAffinityDlg = NULL;
	}
}

void SessionSettingDlg::onClearSessionNote()
{
	m_pSessionNote->clear();
}

void SessionSettingDlg::onToggleApplyFilter(bool t)
{
	m_pProcessFilterEnable->setChecked(t);
	m_pProcessFilterAdvance->setEnabled(t);

	if(t)
	{
		// If filter is enabled, we automatically put the application
		// in the filter by default if the list is empty.
		if ( m_filterList.size() == 0 
		||   ( m_filterList.size() == 1 && m_filterList[0].isEmpty() ))
		{
			m_filterList.clear();
			QString txt = m_pLaunch->currentText().section("\"",1,1);
			m_filterList.push_back(txt);
		}
	}
}

void SessionSettingDlg::onSetAdvanceFilter()
{
	// Pop up process filter dialog
	ProcessFilterDlg *pfd = new ProcessFilterDlg(this);
	pfd->init(m_filterList);
	if( QDialog::Accepted == pfd->exec())
	{
		m_filterList.clear();
		m_filterList = pfd->getStringList();			
	}

	// Clean up
	if(pfd)
	{
		delete pfd;
		pfd = NULL;
	}
}

QStringList SessionSettingDlg::getProcessFilter()
{
	return m_filterList;
}
	

bool SessionSettingDlg::isApplyProcessFilter()
{
	return m_pProcessFilterEnable->isChecked();
}


void SessionSettingDlg::onSettingRightClicked ( Q3ListBoxItem * item, const QPoint &pt)
{
	if ( NULL == item ) 
		return;

	m_pSettingMenu->popup ( pt );
}


void SessionSettingDlg::onSettingAdd(QString name, RUN_SETTING * pRunSetting)
{
	QString tmpName = name;
	int count = 1;

	if (name.isEmpty()) {
		// Case when create first setting
		// or click on add setting button
		tmpName = name = QString("Session");
	} 

	if (!pRunSetting) {
		pRunSetting = new RUN_SETTING();
		if (pRunSetting == NULL)
			return;
	}
	
	// Figure out the next available name
	pRunSetting->sessionName = tmpName;
	while (!m_pCawFile->addRunSetting(tmpName, pRunSetting)) {
		tmpName = name + QString(" ") + QString::number(count++);
		pRunSetting->sessionName = tmpName;
	}

	m_pSettingExplorer->insertItem(tmpName);
	onSettingChange(m_pSettingExplorer->findItem(tmpName, Q3ListBox::ExactMatch));
	m_pSettingExplorer->setFocus();		

	saveToCawFile();
}


void  SessionSettingDlg::onSettingChange(Q3ListBoxItem * item) 
{
	if (item == NULL)
		return;

	// Setup for new setting
	int index = m_pSettingExplorer->index(item);
	QString name = m_pSettingExplorer->text(index);
	m_pSettingExplorer->setSelected(item, true);	

	m_pCurrentSetting = m_pCawFile->getRunSetting(name);
	if ( m_pCurrentSetting != NULL) {
		updateWithCurrentSetting();
	}
	m_pSettingExplorer->setFocus();		
}


void SessionSettingDlg::onSettingRemove(QString name)
{
	int index = 0;
	if (name.isEmpty()) {
		index = m_pSettingExplorer->currentItem();
		name = m_pSettingExplorer->text(index);
	} else {
		index = m_pSettingExplorer->index(
				m_pSettingExplorer->findItem(name, Q3ListBox::ExactMatch));
	}

	if (m_pSettingExplorer->count() == 1) {

		QMessageBox::critical(this, "Session Setting Error",
				"There must be at least one run setting.\n");
		return;
	}
			
	if(m_pCawFile->deleteRunSetting(name)) {
		m_pCurrentSetting = NULL;
		
		if (index > 0)
			onSettingChange(m_pSettingExplorer->item(index-1));
		else 
			onSettingChange(m_pSettingExplorer->item(index+1));
		saveToCawFile();
		m_pSettingExplorer->removeItem(index);
	}
	m_pSettingExplorer->setFocus();		
}


void SessionSettingDlg::onSettingDuplicate() 
{
	// Get current setting
	int index 		= m_pSettingExplorer->currentItem();
	QString curName 	= m_pSettingExplorer->text(index);
	RUN_SETTING * pCurSetting = m_pCawFile->getRunSetting(curName);
	if (!pCurSetting)
		return;
	
	QString newName = QString("Copy of ") + curName;
	onSettingAdd(newName);
	
	// Note: onSettingAdd with get the next available newname. 
	// We need to store it here before copying the setting info
	// and then put back the name.
	newName = m_pCurrentSetting->sessionName;
	*m_pCurrentSetting = *pCurSetting;
	m_pCurrentSetting->sessionName = newName;
	
	updateWithCurrentSetting();
	saveToCawFile();
}


void SessionSettingDlg::onSettingRename()
{
	QString newName;
	int index = m_pSettingExplorer->currentItem();
	QString curName = m_pSettingExplorer->text(index);

	while(1) {
		bool isOk = false;
		newName = QInputDialog::getText( "Enter new name for the session setting", 
			"Rename session setting '" + curName + "'.",
			QLineEdit::Normal,curName,&isOk,this );

		if (!isOk)
			return;

		// Check blank name
		if (isOk && newName.isEmpty()) {
			QMessageBox::critical(this,"Session Setting Rename Error",
				"Blank name is invalid.\n");
			continue;
		}
	
		if (isOk
		&& (curName == newName 
			|| m_pSettingExplorer->findItem(newName, Q3ListBox::ExactMatch) != NULL)) {
			QMessageBox::critical(this,"Session Setting Rename Error",
				"Name is already in used.\n");
			continue;
		}

		QRegExp rex ("[/:\\\\]");
		if (rex.search (newName) != -1) {
			QMessageBox::critical (this, "Session Setting Rename Error",
				"The name must not contain the characters ':', '\\', or '/'");
			continue;
		}


		break; 
	}
	
	RUN_SETTING * pCurSetting = m_pCawFile->getRunSetting(curName);
	if (!pCurSetting)
		return;

	// Add new setting
	onSettingAdd(newName);

	// Note: onSettingAdd with get the next available newname. 
	// We need to store it here before copying the setting info
	// and then put back the name.
	newName = m_pCurrentSetting->sessionName;
	*m_pCurrentSetting = *pCurSetting;
	m_pCurrentSetting->sessionName = newName;

	// Delete old setting
	onSettingRemove(curName); // Setting is also save to caw here
	
	m_pSettingExplorer->setCurrentItem(m_pSettingExplorer->findItem(
				newName, Q3ListBox::ExactMatch));	
}


void SessionSettingDlg::onToggleVmlinuxEnable(bool t)
{
	m_pVmlinuxPath->setEnabled(t);
	m_pVmlinuxBrowse->setEnabled(t);
}


void SessionSettingDlg::onVmlinuxBrowse()
{
	QString path = Q3FileDialog::getOpenFileName( "/boot", "vmlinux "
		"(vmlinux*);;All files "
		"(*)", this, 
		"FileBrowse",
		"Select vmlinux" );

	if (!path.isNull()) {
		m_pVmlinuxPath->setText(path);
	}
}


void SessionSettingDlg::onResetBufferSize()
{
	m_pOpBufSize->setText (QString::number (OP_DEFAULT_BUFFER_SIZE));
	m_pOpWatershedSize->setText (QString::number (OP_DEFAULT_WATERSHED_SIZE));
	m_pOpCpuBufSize->setText (QString::number (OP_DEFAULT_CPU_BUFFER_SIZE));
}


void SessionSettingDlg::onToggleCssEnable(bool t)
{
	QString profile = m_pProfileDropBox->currentText();
	TRIGGER type = m_propertyTrigger;

	//Enabled only for non-TBP sessions, the idea is to disable depth and interval
	//for PERF as PERF doesn't support limiting Call Stack Depth and Interval
	
	bool bEnable = (PERF_TIMER_TRIGGER == type) ? false : t;

	m_pCssDepth->setEnabled(bEnable);
	m_pCssInterval->setEnabled(bEnable);

	m_pCssUseLaunchedTgid->setEnabled (t);
	m_pCssUseTgid->setEnabled (t);

	if (!m_pCurrentSetting->cssUseTgid
	&&   m_pCurrentSetting->cssTgid == -1)  {
		m_pCssUseLaunchedTgid->setChecked(true);
	}	
}


void SessionSettingDlg::onToggleCssUseTgid(bool t)
{
	m_pCssTgid->setEnabled(t);
}

void SessionSettingDlg::onToggleAffinityEnable(bool t)
{
	m_pAffinityEnable->setChecked(t);
	m_pAffinityValue->setEnabled(t);
	m_pAffinitySelect->setEnabled(t);
}


bool SessionSettingDlg::validateOprofileBufferSetting() 
{
	QString msg;
	unsigned int bufferSize    = m_pOpBufSize->text().toUInt();
	unsigned int watershedSize = m_pOpWatershedSize->text().toUInt();
	bool ret = false;

	if (bufferSize < OP_MIN_BUF_SIZE) {
		msg.sprintf ("The minimum buffer size is %d\n",
			OP_MIN_BUF_SIZE);
		QMessageBox::critical (this, "Error", msg);
		m_pOpBufSize->setFocus();
		m_pOpBufSize->selectAll();
		return ret;
	} else if (bufferSize > OP_MAX_BUF_SIZE) {
		msg.sprintf ("The maximum buffer size is %d\n",
			OP_MAX_BUF_SIZE);
		QMessageBox::critical (this, "Error", msg);
		m_pOpBufSize->setFocus();
		m_pOpBufSize->selectAll();
		return ret;
	}

	if (bufferSize <= watershedSize ) {
		msg.sprintf ("Invalid Watershed Size. Watershed should be smaller than Event Buffer.\n");
		QMessageBox::critical (this, "Error", msg);
		m_pOpWatershedSize->setFocus();
		m_pOpWatershedSize->selectAll();
		return ret;
	}


	unsigned int cpuBufSize = m_pOpCpuBufSize->text().toUInt();

	if (cpuBufSize < OP_MIN_CPU_BUF_SIZE) {
		msg.sprintf("The minimum cpu buffer size is %d\n",
			OP_MIN_CPU_BUF_SIZE);
		QMessageBox::critical(this, "Error", msg);
		m_pOpCpuBufSize->setFocus();
		m_pOpCpuBufSize->selectAll();
		return ret;	
	} else if (cpuBufSize > OP_MAX_CPU_BUF_SIZE) {
		msg.sprintf("The maximum cpu buffer size is %d\n",
			OP_MAX_CPU_BUF_SIZE);
		QMessageBox::critical(this, "Error", msg);
		m_pOpCpuBufSize->setFocus();
		m_pOpCpuBufSize->selectAll();
		return ret;
	}

	ret = true;
	return ret;
}

bool SessionSettingDlg::validateProcessFilter()
{
	// Check if process filter is empty and warn
	if ( m_filterList.size() == 0 
	   ||( m_filterList.size() == 1 && m_filterList[0].isEmpty())) {
		unsigned int answer = 
			QMessageBox::warning(this,QString("CodeAnalyst Warning"),
			QString("Apply Process Filter is selected, but Process ")
				+ "Filter List is empty.\n"
				+ "CodeAnalyst will disable process filtering.\n"
				+ "Are you sure you want to continue?\n",
			QMessageBox::Yes, QMessageBox::No);	
		if (answer != QMessageBox::Yes ) {
			// Reconfigure
			m_pProcessFilterEnable->setFocus();	
			return false;
		} else {
			m_pProcessFilterEnable->setChecked(false);	
		}
	}else{
		// Sanity check to see if the launch string is in the process filter list
		QString txt = m_pLaunch->currentText().section("\"",1,1);
		bool foundLaunch = false;
		// Insert each filter path to the view
		QStringList::iterator it = m_filterList.begin();
		QStringList::iterator it_end = m_filterList.end();
		for (; it != it_end ; it++) {
			if (txt.compare(*it) == 0) {
				foundLaunch = true;
				break;
			}
		}

		if (!foundLaunch) {
			unsigned int answer = 
			QMessageBox::warning(this,QString("CodeAnalyst Warning"),
			QString("Process Filter List does not contain the application ")
				+ "to be launched.\n"
				+ "This might result in no profiling data shown.\n"
				+ "Are you sure you want to continue?\n",
			QMessageBox::Yes, QMessageBox::No);	
			if(answer != QMessageBox::Yes ) {
				// Reconfigure
				m_pProcessFilterEnable->setFocus();	
				return false;
			}
		}
	}

	return true;
}

bool SessionSettingDlg::validateCss()
{
	if (!m_pCssEnableBox->isChecked()) 
		return true;

	int cssDepth 	= m_pCssDepth->text().toInt();
	int cssInterval	= m_pCssInterval->text().toInt();

	// TGID stuff	
	bool cssUseTgid	= m_pCssUseTgid->isChecked() | 
			  m_pCssUseLaunchedTgid->isChecked();
	int  cssTgid	= m_pCssTgid->text().toInt();

	if (cssDepth <= 0) {
		QMessageBox::critical(this,"Error",
			QString("Invalid CSS depth"));
		m_pCssDepth->setFocus();
		return false;
	}

	if (cssInterval < 100) {
		QMessageBox::critical(this,"Error",
			QString("Invalid CSS interval\n") 
			+ " The minimum CSS interval range is 100.\n");
		m_pCssInterval->setFocus();
		return false;
	}

	if (cssUseTgid) {
		if (m_pCssUseLaunchedTgid->isChecked()) {
			if (m_pLaunch->currentText().isEmpty()) {
				QMessageBox::critical(this,"Error",
					QString("Current CSS setting requires")
					+" launch application to be specified.");
				m_pLaunch->setFocus();
				return false;
			}
		} else {
			if (cssTgid <= 0) { 
				QMessageBox::critical(this,"Error",
					QString("Invalid CSS tgid"));
				m_pCssTgid->setFocus();
				return false;
			} 
		}
	}
	return true;
}

void SessionSettingDlg::importOprofileLog(QString sessionName)
{
	QString fileName = m_pCawFile->getDir() + sessionName + ".dir/oprofiled.log";
	QFile logFile(fileName);
	
	if (!logFile.open(QIODevice::ReadOnly)) {
		return;
	}

	QTextStream stream (&logFile);
	QString line;
	while (! stream.atEnd())
	{
		line = stream.readLine();
		line.append("\n");
		m_pLog->append(line);
	}
}
