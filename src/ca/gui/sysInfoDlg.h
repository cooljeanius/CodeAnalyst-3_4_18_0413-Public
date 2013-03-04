//$Id: sysInfoDlg.h,v 1.2 2005/02/14 16:52:40 jyeh Exp $
//The definitions of the system information dialog.

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
//$Log: sysInfoDlg.h,v $
//Revision 1.2  2005/02/14 16:52:40  jyeh
//Updated header.
//
//Revision 1.1.1.1  2004/10/05 17:03:23  jyeh
//initial import into CVS
//
//Revision 1.4  2003/12/12 23:04:00  franksw
//Code Review
//
//Revision 1.3  2003/11/03 17:42:02  franksw
//Sys Info Dialog works
//

#ifndef SYSINFODLG_H
#define SYSINFODLG_H

#include <q3process.h>
#include <qstringlist.h> 

//generated from iSysInfoDlg.ui
#include "iSysInfoDlg.h"

class SysInfoDlg : public QDialog, public Ui::ISysInfoDlg
{ 
	Q_OBJECT

public:
	SysInfoDlg( QWidget* parent, 
		Qt::WindowFlags fl = 0);
	~SysInfoDlg();
	bool initialize();


public slots:
	void onOk();
	void onCopytoClipboard();
};

#endif // OPTIONSDLG_H
