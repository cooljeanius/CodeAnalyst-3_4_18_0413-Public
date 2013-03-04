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
// 
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
*/


#ifndef IMPORT_WIZARD_H_
#define IMPORT_WIZARD_H_

#include <qwidget.h>
#include <Q3Wizard>

#include "stdafx.h"
#include "iImportWizard.h"
enum 
{
	WIZARD_PAGE_0 = 0,
	REMOTE_PAGE_0,
	LOCAL_PAGE_0,
	EBP_PAGE_0,
	XML_PAGE_0,
	PERF_PAGE_0
};

enum
{
	NO_ERR = 0,
	OP_SAMPLE_DIR_NOT_EXIST,
	OP_SAMPLE_NOT_FOUND,
	OP_INVALID_SAMPLE_DIR,
	SESSION_DIR_EMPTY,
	XML_INVALID_PATH,
	XML_CPUINFO_INVALID_PATH,
	PERF_INVALID_FILE
};

class ImportWizard : public Q3Wizard, public Ui::IImportWizard
{
	Q_OBJECT
public:
	ImportWizard(QWidget* parent = 0, Qt::WindowFlags fl = 0);
	~ImportWizard();
	QString get_packed_prd_path();
	QString get_op_sample_dir();
	QString get_session_dir();
	QString get_xml_path();
	QString get_xml_cpuinfo_path();
	QString get_cpuinfo_path();
	#ifdef CAPERF_SUPPORT_ENABLED
		QString get_caperf_file();
	#endif //CAPERF_SUPPORT_ENABLED

	bool get_op_merge_lib();
	bool get_exclude_dep();
	bool is_remote_import();
	bool is_session_import();
	bool is_xml_import();
	#ifdef CAPERF_SUPPORT_ENABLED
		bool is_perf_import();
	#endif //CAPERF_SUPPORT_ENABLED

	// Process Filter API
	bool isApplyProcessFilter();
	QStringList getProcessFilter();
	void setProcessFilter(QStringList list);
	void setApplyProcessFilter(bool t);

public slots:
	virtual void browse_packed();
	virtual void browse_opsample_dir();
	virtual void browse_session_dir();
	virtual void browse_xml_dir();
	virtual void browse_xml_cpuinfo();
	virtual void browse_perf_datafile();
	virtual void default_clicked();
	virtual void onToggleApplyFilter(bool t);
	virtual void onClickApplyFilterAdvanceBtn();

protected slots:
	void next();
	void back();
	void accept();

protected:
	unsigned int validate_path();
	void show_error_message(unsigned int code);
	void set_up_pages();
	void load_settings();
	unsigned int import_packed_file();
	void find_prd_files(QString & dir_name);

	QString m_packed_path;
	QString m_temp_dir;
	QString m_packed_prd_path;
	QString m_op_sample_path;
	QString m_cpuinfo_path;
	#ifdef CAPERF_SUPPORT_ENABLED
		QString m_perf_file;
	#endif //CAPERF_SUPPORT_ENABLED
	bool  m_op_merge_lib;
	bool  m_op_exclude_dep;
	QStringList m_filterList;
};

#endif
