// $Id: $

//  Process Filter Dialog Class

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2007 Advanced Micro Devices, Inc.
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

#ifndef PROCESSFILTERDLG_H
#define PROCESSFILTERDLG_H

#include <qstringlist.h>
#include <q3listview.h>
#include "iProcessFilterDlg.h"

using namespace std;

class ProcessFilterDlg: public QDialog, public Ui::iProcessFilterDlg
{
	Q_OBJECT;
public:
	ProcessFilterDlg ( QWidget* parent = 0,	Qt::WindowFlags fl = 0 );

	~ProcessFilterDlg();

	void init(QStringList strList );
	QStringList getStringList();

private:

public slots:
	virtual void onNew();
	virtual void onEdit();
	virtual void onRemove();

signals:

};

#endif /*PROCESSFILTERDLG_H*/
