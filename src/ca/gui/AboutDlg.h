//$Id: AboutDlg.h,v 1.2 2005/02/14 16:52:40 jyeh Exp $
//The definitions of the about dialog.

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
//$Log: AboutDlg.h,v $
//Revision 1.2  2005/02/14 16:52:40  jyeh
//Updated header.
//
//Revision 1.1.1.1  2004/10/05 17:03:23  jyeh
//initial import into CVS
//
//Revision 1.6  2003/12/09 16:54:50  franksw
//Update to coding standards
//
//Revision 1.5  2003/12/01 21:04:39  franksw
//Cleaned up and styled
//
//Revision 1.4  2003/11/12 22:52:55  franksw
//Code cleanup
//
//Revision 1.3  2003/11/03 16:14:34  franksw
//About box works
//

#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include "stdafx.h"
#include <Q3Process>

//generated from iAboutDlg.ui
#include "iAboutDlg.h"

class AboutDlg : public QDialog, public Ui::IAboutDlg
{
	Q_OBJECT

public:
	AboutDlg( QWidget* parent = 0, Qt::WindowFlags fl = 0);
	~AboutDlg();

private slots:
	void readStdout();
	void displayBuffer();

private:
	Q3Process * ldd_process;
	QStringList buffer;
};

#endif // ABOUTDLG_H
