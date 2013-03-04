// $Id: CpuAffinityDlg.h 20178 2011-08-25 07:40:17Z sjeganat $

// CPU Affinity Dialog Class 

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

#ifndef CPUAFFINITYDLG_H
#define CPUAFFINITYDLG_H

#include <sched.h>
#include <vector>
#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <q3scrollview.h>
#include <Q3GridLayout>
#include "iCpuAffinityDlg.h"
#include "stdafx.h"
#include "affinity.h"

using namespace std;

class Socket : public Q3ButtonGroup
{
	Q_OBJECT;
public:
	Socket(const QString & title, QWidget * parent );
	~Socket();

	void init( unsigned int size,
		unsigned int afMask);

	void getCheckedAffinity(unsigned int * checkedMask);

private:
	Q3GridLayout *m_pGLayout;
	vector<QCheckBox*> m_chkBox_vec;

private slots:
	void onSelectAllCores(bool state);

signals:
	void selectAllCores(bool state);

};

//////////////////////////////////////////////////////////////////////////

class CpuAffinityDlg : public QDialog, public Ui::iCpuAffinityDlg
{
	Q_OBJECT;
public:
	CpuAffinityDlg( QWidget* parent = 0, 
		Qt::WindowFlags fl = 0 );

	~CpuAffinityDlg();

	void init(unsigned int numSockets, 
		unsigned  int numCpusPerSocket, 
		QString afMaskString);

	CA_Affinity * getAffinityMask();

private:
	unsigned int m_numSockets;
	unsigned int m_numCpusPerSocket;
	Q3ScrollView * m_pScrollView;
	bool m_allState;
	vector<Socket*> m_socket_vec;
	CA_Affinity m_afMask;

private slots:
	void onSelectAllCores();	
	void onClearAllCores();	
	void onOk();

signals:
	void selectAllCores(bool state);
};

#endif /*CPUAFFINITYDLG_H*/
