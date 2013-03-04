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
// :
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
*/


#include <qradiobutton.h>
#include <q3filedialog.h>
#include <qcheckbox.h>
#include <stdlib.h>
#include "ImportWizard.h"
#include "application.h"
#include "eventsfile.h"
#include "time.h"
#include "atuneoptions.h"
#include "ProcessFilterDlg.h"
#include "helperAPI.h"

#define CA_DEFAULT_IMPORT_PRD_PATH "/var/lib/oprofile/anon_samples.prd"

#define SHOW_ERR_AND_RETURN  if (NO_ERR != ret) { \
	show_error_message(ret); \
	return; }

ImportWizard::ImportWizard(QWidget* parent, Qt::WindowFlags fl)
: Q3Wizard(parent,0,true,fl|CA_DLG_FLAGS)
{
	setupUi(this);
	m_cpuinfo_path = "/proc/cpuinfo";
	set_up_pages();
	load_settings();
}


ImportWizard::~ImportWizard()
{

}

void ImportWizard::set_up_pages()
{
	helpButton()->setHidden(true);

	setHelpEnabled(page(LOCAL_PAGE_0),false);

	setNextEnabled(page(LOCAL_PAGE_0), false);
	setNextEnabled(page(REMOTE_PAGE_0), false);
	setNextEnabled(page(EBP_PAGE_0), false);
	setNextEnabled(page(XML_PAGE_0), false);
#ifdef CAPERF_SUPPORT_ENABLED
	setNextEnabled(page(PERF_PAGE_0),false);
#endif //CAPERF_SUPPORT_ENABLED 
	


	setFinishEnabled(page(LOCAL_PAGE_0), true);
	setFinishEnabled(page(REMOTE_PAGE_0), true);
	setFinishEnabled(page(EBP_PAGE_0), true);
	setFinishEnabled(page(XML_PAGE_0), true);
#ifdef CAPERF_SUPPORT_ENABLED
	setFinishEnabled(page(PERF_PAGE_0), true);
#endif //CAPERF_SUPPORT_ENABLED 
	// We no longer allow user to specify option for import
	m_ck_exclude_dep->hide();
	m_ck_merge_lib->hide();
#ifndef CAPERF_SUPPORT_ENABLED
	m_rd_Perf2CA->hide();
#endif //CAPERF_SUPPORT_ENABLED 


        m_session_dir->setText(QDir::home().path() + "/AMD/CodeAnalyst");
        m_xml_path->setText(QDir::home().path() + "/AMD/CodeAnalyst");
}

bool ImportWizard::is_remote_import()
{
	return m_rd_remote->isChecked();	
}
#ifdef CAPERF_SUPPORT_ENABLED
bool ImportWizard::is_perf_import()
{
	return m_rd_Perf2CA->isChecked();
}
#endif //CAPERF_SUPPORT_ENABLED 

bool ImportWizard::is_session_import()
{
	return m_rd_session->isChecked();	
}

bool ImportWizard::is_xml_import()
{
	return m_rd_xml->isChecked();	
}

void ImportWizard::load_settings()
{
	unsigned int import_type = LOCAL_PROFILE;
	CATuneOptions ao;

	ao.getImportType(&import_type);
	if (LOCAL_PROFILE == import_type)
		m_rd_local->setChecked(true);
	else if (REMOTE_PROFILE == import_type)
		m_rd_remote->setChecked(true);
	else if (SESSION_PROFILE == import_type)
		m_rd_session->setChecked(true);
	else if (XML_PROFILE == import_type)
		m_rd_xml->setChecked(true);
#ifdef CAPERF_SUPPORT_ENABLED
	else if (PERF_PROFILE == import_type)
		m_rd_Perf2CA->setChecked(true);
#endif //CAPERF_SUPPORT_ENABLED 
	QString op_dir;
	ao.getImportOpdir(op_dir);
	m_op_dir_path->setText(op_dir);

	QString tar;
	ao.getImportTAR(tar);
	m_tar_path->setText(tar);

	unsigned int value;
	ao.getImportExcludeDep(&value);
	m_ck_exclude_dep->setChecked(value);

	ao.getImportMergeByLib(&value);
	m_ck_merge_lib->setChecked(value);
}


void ImportWizard::next()
{
	CATuneOptions ao;

	switch(indexOf(currentPage())) {
	case WIZARD_PAGE_0: 
		if (m_rd_remote->isChecked()) {
			setFinishEnabled(page(REMOTE_PAGE_0), true);
			showPage(page(REMOTE_PAGE_0));
		} else if (m_rd_local->isChecked()) {
			m_cpuinfo_path = "/proc/cpuinfo";
			setFinishEnabled(page(LOCAL_PAGE_0), true);
			showPage(page(LOCAL_PAGE_0));
		} else if (m_rd_session->isChecked()) {
			setFinishEnabled(page(EBP_PAGE_0), true);
			showPage(page(EBP_PAGE_0));
		} else if (m_rd_xml->isChecked()) {
			setFinishEnabled(page(XML_PAGE_0), true);
			showPage(page(XML_PAGE_0));
		}
#ifdef CAPERF_SUPPORT_ENABLED
		else if (m_rd_Perf2CA->isChecked()) {
			setFinishEnabled(page(PERF_PAGE_0), true);
			showPage(page(PERF_PAGE_0));
		}
#endif //CAPERF_SUPPORT_ENABLED 
	break;
	}
	finishButton()->setFocus();
}

void ImportWizard::back()
{
	switch(indexOf(currentPage())) {
		case LOCAL_PAGE_0:
		case REMOTE_PAGE_0:
		case EBP_PAGE_0:
		case XML_PAGE_0:
#ifdef CAPERF_SUPPORT_ENABLED
		case PERF_PAGE_0:
#endif //CAPERF_SUPPORT_ENABLED 
			setApplyProcessFilter(false);
			setFinishEnabled(currentPage(), false);
			showPage(page(WIZARD_PAGE_0));
			break;
	}
}
 
// JNC and Oprofile sample directory must exist
unsigned int  ImportWizard::validate_path()
{
	// Sanity check for empty path.
	if(m_op_sample_path.isEmpty())
		return OP_SAMPLE_DIR_NOT_EXIST;

	QDir dir(m_op_sample_path);

	// Sanity check for not exist path.
	if (!dir.exists())
		return OP_SAMPLE_DIR_NOT_EXIST;

	// Sanity check to see if this directory really contains oprofile samples
	// by checking for directory {root} or {kern}
	if (!dir.exists("{root}") && !dir.exists("{kern}"))
	{
		return OP_INVALID_SAMPLE_DIR; 
	}

	return NO_ERR; 
}

void ImportWizard::show_error_message(unsigned int code)
{
	QString message;
	QString message2;
	QWidget * item = NULL;

	switch(code) {
		case OP_SAMPLE_DIR_NOT_EXIST:
			message = QString("Path \"") + m_op_sample_path + "\" does not exist.";
			item = m_op_dir_path;
			break;
		case OP_SAMPLE_NOT_FOUND:
			message = "Package contains no OProfile sample file."; 
			item = m_tar_path;
			break;
		case OP_INVALID_SAMPLE_DIR:
			message = QString("Invalid Importing Path.\n") + 
				"Please specify full-path to Oprofile sample directory.\n" +
				"It should contains subdirectory \"{kern}\" and/or \"{root}\"." ;
			item = m_op_dir_path;
			break;
		case SESSION_DIR_EMPTY:
			message = QString("Invalid Importing Path.\n") + 
				"Please specify full-path to EBP/TBP file.\n" ;
			item = m_session_dir;
			break;
		case XML_INVALID_PATH:
			message = QString("Invalid XML Path.\n") + 
				"Please specify full-path to XML file.\n";
			item = m_xml_path;
			break;
		case XML_CPUINFO_INVALID_PATH:
			message = QString("Invalid Cpuinfo Path.\n") + 
				"Please specify full-path to cpuinfo file.\n" +
				"(i.e. /proc/cpuinfo)\n";
			item = m_xml_path;
			break;
#ifdef CAPERF_SUPPORT_ENABLED
		case PERF_INVALID_FILE:
			message = QString("Invalid Perf file.\n") +
				"Please point to a valid Perf data file.\n";
			item = m_perfdata_file;
			break;
#endif //CAPERF_SUPPORT_ENABLED 
	}
	switch(indexOf(currentPage())) {
		case LOCAL_PAGE_0:
			message2 = "Please select a different directory.";
			break;
		case REMOTE_PAGE_0:
			message2 = "The packaged file is missing critical directory.\n" 
				"Please choose a different .tar.gz file or cancel.";
			break;
	}


	message = message + " \n" + message2;
	QMessageBox::critical (this, "Error", message);
	if (item != NULL)
		item->setFocus(); 
}

void ImportWizard::accept()
{
	CATuneOptions ao;
	unsigned int value;

	// Check Process Filter
	if(isApplyProcessFilter())
	if( m_filterList.size() == 0 
	||  (m_filterList.size() == 1 && m_filterList[0].isEmpty() )) 
	{
		unsigned int answer = 	
		 QMessageBox::warning(this,QString("CodeAnalyst Warning"),
                        QString("Apply Process Filter is selected, but Process Filter List is empty.\n")
                        	+ "CodeAnalyst will disable process filtering.\n"
                                + "Are you sure you want to continue?\n",
                        QMessageBox::Yes, QMessageBox::No);
		if(answer != QMessageBox::Yes )
                {
			// Set focus
			if(m_pApplyFilter->isChecked())
				m_pApplyFilter->setFocus();
			
			return;	
		}else{
			setApplyProcessFilter(false);		
		}
	}

	switch(indexOf(currentPage())) {
		case LOCAL_PAGE_0:
			m_op_sample_path = m_op_dir_path->text();
			int ret; 
			if (NO_ERR != (ret = validate_path())) {
				show_error_message(ret);
				return;
			}
			ao.setImportOpdir(m_op_sample_path);

			if (m_ck_merge_lib->isChecked()) {
				value = MERGE_LIB;
				m_op_merge_lib = true;
			}
			else {
				value = NO_MERGE_LIB;
				m_op_merge_lib = false;
			}

			ao.setImportMergeByLib(value);
			ao.getImportMergeByLib(&value);

			if (m_ck_exclude_dep->isChecked())
				value = EXCLUDE_DEP;
			else
				value = NO_EXCLUDE_DEP;

			ao.setImportExcludeDep(value);
			ao.getImportExcludeDep(&value);
			break;
		case REMOTE_PAGE_0:
			m_packed_path = m_tar_path->text();
			ao.setImportTAR(m_packed_path);

			ret = import_packed_file();
			SHOW_ERR_AND_RETURN  

			ret = validate_path();
			SHOW_ERR_AND_RETURN  

			break;
		case EBP_PAGE_0:
			// Handling the Session dir
			if ( m_session_dir->text().isEmpty()
			|| !QFile::exists(m_session_dir->text()))
			{
				show_error_message(SESSION_DIR_EMPTY);
				return;	
			}

			// Check if empty

			break;
		case XML_PAGE_0:
			// Handling the XML file
			if (m_xml_path->text().isEmpty() 
			|| !QFile::exists(m_xml_path->text())
			|| !QFileInfo(m_xml_path->text()).isFile()) 
			{
				show_error_message(XML_INVALID_PATH);
				return;	
			}

			if (m_xml_cpuinfo_path->text().isEmpty() 
			|| !QFile::exists(m_xml_cpuinfo_path->text())
			|| !QFileInfo(m_xml_cpuinfo_path->text()).isFile()) 
			{
				show_error_message(XML_CPUINFO_INVALID_PATH);
				return;	
			}
			break;
#ifdef CAPERF_SUPPORT_ENABLED
		case PERF_PAGE_0:
			if (m_perfdata_file->text().isEmpty()
			|| !QFile::exists(m_perfdata_file->text())
			|| !QFileInfo(m_perfdata_file->text()).isFile())
			{
				show_error_message(PERF_INVALID_FILE);
				return;
			}
#endif //CAPERF_SUPPORT_ENABLED
		default:
			break;
	}

	if (m_rd_local->isChecked())
		value = LOCAL_PROFILE;
	else if (m_rd_remote->isChecked())
		value = REMOTE_PROFILE;
	else if (m_rd_session->isChecked())
		value = SESSION_PROFILE;
	else if (m_rd_xml->isChecked())
		value = XML_PROFILE;
#ifdef CAPERF_SUPPORT_ENABLED
	else if (m_rd_Perf2CA->isChecked())
		value = PERF_PROFILE;
#endif //CAPERF_SUPPORT_ENABLED
    
	ao.setImportType(value);

	QDialog::accept();
}

void ImportWizard::browse_packed()
{
	QString packed_path = Q3FileDialog::getOpenFileName (m_tar_path->text(), "*.tar.gz",
		this, NULL,
                                               "Choose a capackage file");

	if (!packed_path.isEmpty()) 
		m_tar_path->setText(packed_path);
}
 
void ImportWizard::browse_perf_datafile()
{
#ifdef CAPERF_SUPPORT_ENABLED
	QString perf_file = Q3FileDialog::getOpenFileName (m_perfdata_file->text(), "*.data",
		this, NULL,
                                               "Choose a Perf data file");

	if (!perf_file.isEmpty()) 
		m_perfdata_file->setText(perf_file);
#endif //CAPERF_SUPPORT_ENABLED
}


void ImportWizard::browse_opsample_dir()
{
	QString path;

	path = Q3FileDialog::getExistingDirectory(m_op_dir_path->text(), this,
		NULL,
		"Choose a directory containing Oprofile Session");

	if (!path.isEmpty()) 
		m_op_dir_path->setText(path);
}

void ImportWizard::browse_session_dir()
{
    QString session_dir;

    session_dir = Q3FileDialog::getExistingDirectory(m_session_dir->text(), 
                this, NULL,
                "Choose CodeAnalyst Session directory.");
    
    if (!session_dir.isEmpty()) 
        m_session_dir->setText(session_dir);
}

void ImportWizard::browse_xml_dir()
{
    QString xml_path;

    xml_path = Q3FileDialog::getOpenFileName(m_xml_path->text(), "CodeAnalyst Event File (*.xml)",
                this, NULL,
                "Choose an Oprofile XML file.");
    
    if (!xml_path.isEmpty()) 
        m_xml_path->setText(xml_path);
}

void ImportWizard::browse_xml_cpuinfo()
{
    QString path;

    path = Q3FileDialog::getOpenFileName(m_xml_cpuinfo_path->text(), "cpuinfo file",
                this, NULL,
                "Choose cpuinfo file.");
    
    if (!path.isEmpty()) 
        m_xml_cpuinfo_path->setText(path);
}

void ImportWizard::default_clicked()
{
	m_op_dir_path->setText(OP_SAMPLES_CURRENT_DIR);
}

unsigned int ImportWizard::import_packed_file()
{
	untar_packed_file(&m_packed_path, &m_temp_dir);
	return rename_files(&m_temp_dir, 
			&m_op_sample_path,
			&m_cpuinfo_path);
}

void ImportWizard::find_prd_files(QString & dir_name)
{
	QDir dir(dir_name);

	QStringList file_list = dir.entryList(); 
	QStringList::iterator it = file_list.begin();
	for (; it != file_list.end(); ++it) {
		QFileInfo info(dir, *it);
		QString path = info.filePath(); 

		if (*it == "." || *it == "..")
			continue;

		if (path.contains(".prd")) {
			m_packed_prd_path = path;
			return;
		}

		find_prd_files(path);
	}
}

#ifdef CAPERF_SUPPORT_ENABLED
QString ImportWizard::get_caperf_file()
{
	return m_perfdata_file->text();
}
#endif //CAPERF_SUPPORT_ENABLED

QString ImportWizard::get_packed_prd_path()
{
	return m_packed_prd_path; 
}

QString ImportWizard::get_session_dir()
{
   return m_session_dir->text(); 
}

QString ImportWizard::get_xml_path()
{
   return m_xml_path->text(); 
}

QString ImportWizard::get_xml_cpuinfo_path()
{
   return m_xml_cpuinfo_path->text(); 
}

QString ImportWizard::get_op_sample_dir()
{
	return m_op_sample_path;
}


QString ImportWizard::get_cpuinfo_path()
{
	return m_cpuinfo_path; 
}


bool ImportWizard::get_op_merge_lib()
{
	return m_op_merge_lib;
}


bool ImportWizard::get_exclude_dep()
{
	return m_op_exclude_dep;
}

void ImportWizard::onToggleApplyFilter(bool t)
{
	setApplyProcessFilter(t);
	
	// We pop the Process Filter Dialog automatically.
	if(t)
	{
		onClickApplyFilterAdvanceBtn();
	}

}

void ImportWizard::onClickApplyFilterAdvanceBtn()
{
	ProcessFilterDlg *pfd = new ProcessFilterDlg(this);
	
	pfd->init(m_filterList);
	if( QDialog::Accepted == pfd->exec())
	{
		m_filterList.clear();
		m_filterList = pfd->getStringList();
	}

	if(pfd)
	{
		delete pfd;
		pfd = NULL;
	}
}

bool ImportWizard::isApplyProcessFilter()
{
	return m_pApplyFilter->isChecked();
}

QStringList ImportWizard::getProcessFilter()
{
	return m_filterList;
}

void ImportWizard::setProcessFilter(QStringList list)
{
	m_filterList = list;
}

void ImportWizard::setApplyProcessFilter(bool t)
{
	m_pApplyFilter->setChecked(t);
	m_pApplyFilterAdvanceBtn->setEnabled(t);
}
