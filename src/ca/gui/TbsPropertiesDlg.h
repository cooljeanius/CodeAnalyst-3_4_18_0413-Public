//$Id: TbsPropertiesDlg.h,v 1.2 2005/02/14 16:52:40 jyeh Exp $
// The definitions of the tbs session properties dialog.

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

#ifndef TBS_PROPERTIES_DLG_H
#define TBS_PROPERTIES_DLG_H

#include "cawfile.h"

//generated from iTbsProperties.ui
#include "iTbsProperties.h"


// This class will display the properties of the session.
class TbsPropertiesDlg : public iTbsProperties
{ 
	Q_OBJECT

public:
	TbsPropertiesDlg( QWidget* parent = 0, const char* name = 0, 
		      bool modal = FALSE, Qt::WFlags fl = Qt::WStyle_Customize 
		      | Qt::WStyle_NormalBorder | Qt::WStyle_Title | Qt::WStyle_SysMenu, 
		bool warning = FALSE );

	~TbsPropertiesDlg();

	bool showTbsOptions(TBS_SESSION_OPTIONS *options,
		QString session_dir);
private:
	void LoadJavaFiles ();

private:
	QString m_jnc_dir;

};

#endif // TBS_PROPERTIES_DLG_H
