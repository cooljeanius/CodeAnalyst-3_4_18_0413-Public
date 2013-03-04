//$Id: newProjectDlg.cpp 20183 2011-08-26 11:50:45Z sjeganat $

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

#include "newProjectDlg.h"

#include <qlineedit.h>
#include <q3button.h>
#include <q3filedialog.h>
#include <qmessagebox.h>
#include "stdafx.h"

NewProjectDlg::NewProjectDlg( QWidget* parent,Qt::WindowFlags fl ) 
: QDialog (parent, fl|CA_DLG_FLAGS)
{
	setupUi(this);
	m_pProjectName->setFocus ();
}

NewProjectDlg::~NewProjectDlg()
{
}

void NewProjectDlg::onClickBrowse ()
{
	QString     ProjPath;

	//ProjPath = QFileDialog::getExistingDirectory();
	ProjPath = Q3FileDialog::getExistingDirectory ( m_pProjectDir->text(), this,
		NULL, "Please select a project directory.");

	// Make sure the project directory format is valid
	if( !ProjPath.isNull() )
	{
		ProjPath.replace ('\\', '/');
		m_pProjectDir->setText( ProjPath );
	}
}

void NewProjectDlg::onClickOk ()
{
	if (m_pProjectName->text().simplifyWhiteSpace().isEmpty())
	{
		QMessageBox::warning (this, "CodeAnalyst warning", "The project name must not be empty or contains only whitespaces.");
		m_pProjectName->setFocus ();
		m_pProjectName->selectAll ();
	} else {
		accept ();
	}
}
