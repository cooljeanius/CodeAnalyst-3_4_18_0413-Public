//$Id: TbsPropertiesDlg.cpp,v 1.2 2005/02/14 16:52:29 jyeh Exp $
// The implementation of the tbs session properties dialog.

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2005 Advanced Micro Devices, Inc.
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

//Revision history
//$Log: TbsPropertiesDlg.cpp,v $
//Revision 1.2  2005/02/14 16:52:29  jyeh
//Updated Header.
//
//Revision 1.1.1.1  2004/10/05 17:03:23  jyeh
//initial import into CVS
//
//Revision 1.3.2.2  2004/09/10 15:25:14  jyeh
//BUG22061 	2.3 Linux - Java Source Files tab does not show any java source
//
//Revision 1.3.2.1  2004/08/13 20:16:30  jyeh
//Added Java Source line info.
//
//Revision 1.3  2003/12/12 23:04:53  franksw
//Code Review
//
//Revision 1.2  2003/11/14 16:02:15  franksw
//code cleanup
//

#include "stdafx.h"
#include "TbsPropertiesDlg.h"
#include "EbsPropertiesDlg.h"

using namespace EbsPropertiesDlgSpace;


TbsPropertiesDlg::TbsPropertiesDlg ( QWidget* parent , const char* name,
				bool modal, Qt::WFlags fl, bool warning )
: iTbsProperties ( parent, name, modal, fl )
{
	Q_UNUSED (warning);
}


TbsPropertiesDlg::~TbsPropertiesDlg()
{
	// no need to delete child widgets, Qt does it all for us
}


//Sets the text fields with the appropriate values from the options.
//
bool TbsPropertiesDlg::showTbsOptions (TBS_SESSION_OPTIONS * options,
				   QString session_dir)
{
	QString caption;
	caption = "Properties for tbs session ";
	caption += options->SessionName;
	setCaption ( caption );

	m_session_name->setText ( options->SessionName );
	m_file_name->setText ( options->FileName );
	if (options->msInterval >0)
		m_interval->setText ( QString::number ( options->msInterval) );
	else
		m_interval->setEnabled (false);
	if (!options->ProgramPath.isEmpty())
		m_program->setText ( options->ProgramPath );
	else
		m_program->setEnabled (false);
	m_stop_on_app->setChecked ( options->bStopOnApp );
	m_break_after->setChecked ( options->bBreakAfter );
	m_duration->setText ( QString::number ( options->sDuration ) );
	m_no_duration->setChecked (0 == options->sDuration);
	m_duration->setEnabled (0 != options->sDuration);
	m_delay->setText ( QString::number ( options->sDelay ) );
	if (!options->WorkDir.isEmpty())
		m_work_dir->setText ( options->WorkDir);
	else
		m_work_dir->setEnabled (false);

	chkJavaInfo->setChecked (options->bJitInfo);
	TabInfo->setTabEnabled (TabInfo->page (1), options->bJitInfo);
	if (options->bJitInfo) {
		QString temp_dir = session_dir;
		temp_dir += options->FileName;
		temp_dir += ".dir";
		m_jnc_dir = temp_dir;
		LoadJavaFiles ();
	}

	return true;
} //TbsPropertiesDlg::ShowTbsOptions

void TbsPropertiesDlg::LoadJavaFiles ()
{
	JavaSrcFileMap	 src_file_map;
	ReadJavaSourceFileMap (m_jnc_dir, &src_file_map);
	JavaSrcFileMap::Iterator src_it;

	for( src_it = src_file_map.begin(); src_it != src_file_map.end(); ++src_it)
	{
		new Q3ListViewItem (LvJavaFiles, src_it.key(), src_it.data());
	}

	src_file_map.clear();
}
