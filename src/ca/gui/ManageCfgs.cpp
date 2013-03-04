//$Id: ManageCfgs.cpp 20183 2011-08-26 11:50:45Z sjeganat $

/**
*   (c) 2006 Advanced Micro Devices, Inc.
*  YOUR USE OF THIS CODE IS SUBJECT TO THE TERMS
*  AND CONDITIONS OF THE GNU GENERAL PUBLIC
*  LICENSE FOUND IN THE "GPL.TXT" FILE THAT IS
*  INCLUDED WITH THIS FILE AND POSTED AT
*  <http://www.gnu.org/licenses/gpl.html>
*  Support: codeanalyst@amd.com
*/

#include <q3listbox.h>
#include <qdir.h>
#include "ManageCfgs.h"
#include "TimerCfg.h"
#include "PerfTimerCfg.h"
#include "EventCfgDlg.h"


ManageCfgsDlg::ManageCfgsDlg ( ProfileCollection *pProfiles, 
				CEventsFile * pEventsFile,
				QWidget* parent,
				bool showEdit, 
				const char* name, 
				bool modal, 
				Qt::WindowFlags fl ) 
: QDialog (parent, fl|CA_DLG_FLAGS)
{
	setupUi(this);
	m_pCollection	= pProfiles;
	m_pEventsFile	= pEventsFile;
	
	if (!showEdit)
		m_pEdit->hide();	

	refreshList();
	connect (m_pEdit, SIGNAL (clicked()), SLOT (onEdit ()));
	connect (m_pRemove, SIGNAL (clicked()), SLOT (onRemove ()));
	connect (m_pExport, SIGNAL (clicked()), SLOT (onExport ()));
	connect (m_pImport, SIGNAL (clicked()), SLOT (onImport ()));
	connect (m_pRename, SIGNAL (clicked()), SLOT (onRename ()));
}


ManageCfgsDlg::~ManageCfgsDlg ()
{
}


void ManageCfgsDlg::refreshList ()
{
	m_pCfgList->clear ();
	if (NULL == m_pCollection)
		return;
	m_pCfgList->insertStringList (m_pCollection->getListOfProfiles ());
}


void ManageCfgsDlg::onRemove ()
{
	//Ignore if nothing is selected.
	if (m_pCfgList->currentText().isEmpty())
	{
		QMessageBox::warning ( this, "CodeAnalyst Warning",
			"Please select profile configuration to remove.");
		return;
	}

	if(QMessageBox::warning(this,"Remove Confirmation",
			QString("CodeAnalyst is removing configuration \"")
				+ m_pCfgList->currentText() + "\"\n\n"
				+ "Confirm removal?\n",
				QMessageBox::Yes,QMessageBox::No)
		!= QMessageBox::Yes)
	{
		return;
	}

	if (!m_pCollection->removeProfile (m_pCfgList->currentText()))
	{
		QMessageBox::warning (this, "CodeAnalyst warning",
			"Cannot remove predefined profie configuration.");
		return;
	}

	refreshList ();
}


void ManageCfgsDlg::onEdit ()
{
	int ret;
	//
	QString profile = m_pCfgList->currentText();

	if(profile.isEmpty())
	{
		QMessageBox::warning ( this, "CodeAnalyst Warning",
			"Please select profile configuration to edit.");
		return;
	}

	switch (m_pCollection->getProfileType (profile))
	{
	case TIMER_TRIGGER: 
	{
		TimerCfgDlg *pTimer = new TimerCfgDlg (this);
		RETURN_IF_NULL (pTimer, this);
		pTimer->setProfile (m_pCollection, profile);
		ret = pTimer->exec ();
		delete pTimer;
		break; 
	}
	case PERF_TIMER_TRIGGER:
	{
		PerfTimerCfgDlg *pTimer = new PerfTimerCfgDlg (this);
		RETURN_IF_NULL (pTimer, this);
		pTimer->setProfile (m_pCollection, profile);
		ret = pTimer->exec ();
		delete pTimer;
		break; 
	}
	case EVENT_TRIGGER: {
		EventCfgDlg *pEventDlg = new EventCfgDlg (this);
		RETURN_IF_NULL (pEventDlg, this);
		pEventDlg->setProfile (m_pCollection, profile);
		ret = pEventDlg->exec ();

		delete pEventDlg;
		break; 
						}
	default:
		ret = 0;
		// Ignore if nothing is selected.
		break;
	}
	
	refreshList ();
}


void ManageCfgsDlg::onExport ()
{
	//Ignore if nothing is selected.
	if (m_pCfgList->currentText().isEmpty())
	{
		QMessageBox::warning ( this, "CodeAnalyst Warning",
			"Please select profile configuration to export.");
		return;
	}

	//Get file to export to
	QString exportFile = Q3FileDialog::getSaveFileName(
		//		QString::null, "Profile Configurations (*.xml);;All files (*.*)", this,
		m_pCollection->getFileName (m_pCfgList->currentText()), 
		"Profile Configurations (*.xml);;All files (*.*)", this,
		NULL, "Please enter the file name to export the configuration to" );
	if (exportFile.isEmpty())
	{
		return;
	}
	while (QFile::exists (exportFile))
	{
		if (QMessageBox::Yes == QMessageBox::question (this, 
			"CodeAnalyst Warning", "Do you want to overwrite the existing file?",
			QMessageBox::Yes, QMessageBox::No))
		{
			break;
		}
		exportFile = Q3FileDialog::getSaveFileName(
			QString::null, "Profile Configurations (*.xml);;All files (*.*)", 
			this, NULL, 
			"Please enter the file name to export the configuration to" );
	}
	if (!exportFile.endsWith (".xml", false))
	{
		exportFile += ".xml";
	}
	m_pCollection->exportProfile (m_pCfgList->currentText(), exportFile);
	refreshList ();
}


void ManageCfgsDlg::onImport ()
{
	QString importFile = Q3FileDialog::getOpenFileName(
		QString::null, "Profile Configurations (*.xml);;All files (*.*)", this,
		NULL, "Please find the file name to import" );
	if (importFile.isEmpty())
	{
		return;
	}

	switch ( m_pCollection->importProfile (importFile, m_pEventsFile))
	{
	case PROFILE_FILE_NOT_FOUND:
		QMessageBox::critical (this,
			"CodeAnalyst error", "The specified file (" + importFile 
			+ ") is not a valid profile configuration.\n"
			"Please choose another file.");
		break;
	case PROFILE_DUPLICATE_NAME:
		QMessageBox::warning (this,
			"CodeAnalyst error", "The specified file (" + importFile 
			+ ") contains a configuration which already exists.\n"
			"Please choose another file.");
		break;
	default:
	case PROFILE_IMPORT_OKAY:
		break;
	}
	refreshList ();
}


void ManageCfgsDlg::onRename()
{
	QString newName;
	QString curName = m_pCfgList->currentText();

	//Ignore if nothing is selected.
	if (curName.isEmpty())
	{
		QMessageBox::warning ( this, "CodeAnalyst Warning",
				"Please select profile configuration to rename.");
		return;
	}

	// Get new name
	while(1) {
		bool isOk = false;
		newName = QInputDialog::getText( "Enter new name for profile configuration", 
				"Rename Profile Configuration '" + curName + "'.",
				QLineEdit::Normal,curName,&isOk,this );

		if (!isOk)
			return;

		// Check blank name
		if (isOk && newName.isEmpty()) {
			QMessageBox::critical(this,"Profile configuration Rename Error",
				"Blank name is invalid.\n");
			continue;
		}
	
		break; 
	}

	QString tip, desc;
	TRIGGER trigger = m_pCollection->getProfileType(curName);
	if (trigger == NO_TRIGGER)
		goto errOut;
		
	m_pCollection->getProfileTexts(curName, &tip, &desc);

	switch (trigger) {
	case TIMER_TRIGGER: 
	{
		TBP_OPTIONS opt;
		if (!m_pCollection->getProfileConfig(curName, &opt)) {
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pCollection->getErrorMsg());
			goto errOut;
		}
		if (!m_pCollection->removeProfile(curName)) 
			goto errOut;
		if (!m_pCollection->setProfileConfig(newName, (SESSION_OPTIONS*) &opt, tip, desc)) {
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pCollection->getErrorMsg());
			goto errOut;
		}
		break;
	}
	case PERF_TIMER_TRIGGER:
	{
		PERF_TBP_OPTIONS opt;
		if (!m_pCollection->getProfileConfig(curName, &opt)) {
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pCollection->getErrorMsg());
			goto errOut;
		}
		if (!m_pCollection->removeProfile(curName)) 
			goto errOut;
		if (!m_pCollection->setProfileConfig(newName, (SESSION_OPTIONS*) &opt, tip, desc)) {
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pCollection->getErrorMsg());
			goto errOut;
		}
		break;
	}
	case EVENT_TRIGGER: {
		EBP_OPTIONS opt;
		if (!m_pCollection->getProfileConfig( curName, &opt)) {
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pCollection->getErrorMsg());
			goto errOut;
		}
		if (!m_pCollection->removeProfile( curName)) 
			goto errOut;
		if (!m_pCollection->setProfileConfig(newName, (SESSION_OPTIONS*) &opt, tip, desc)) { 
			QMessageBox::warning(this, "CodeAnalyst Error" ,m_pCollection->getErrorMsg());
			goto errOut;
		}
		break;
	}
	default:
		goto errOut;	
	}	

	refreshList ();
	return;
errOut:
	QMessageBox::critical(this, "CodeAnalyst Error",
			"Cannot rename profile configuration.");
	return;	
}
