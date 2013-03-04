// $Id: CpuAffinityDlg.cpp 20137 2011-08-17 19:15:13Z franksw $

// CPU Affinity Dialog Header

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

#include <q3vgroupbox.h>
#include <qcheckbox.h>
#include <qsizegrip.h>
#include <qlabel.h>
#include <qfile.h>
#include <q3textedit.h>
#include <q3textstream.h>
#include <Q3VBoxLayout>
#include <Q3Frame>
#include "stdafx.h"
#include "affinity.h"
#include "CpuAffinityDlg.h"

#define MAX_DLG_HEIGHT		600
#define MAX_CORES_PER_ROW 	4
#define MARGIN_SIZE		15

Socket::Socket(const QString & title, QWidget * parent )
: Q3ButtonGroup (MAX_CORES_PER_ROW, Qt::Horizontal, title, parent)
{
	m_pGLayout = NULL;

}

Socket::~Socket()
{
	if(m_pGLayout)
	{
		delete m_pGLayout;
		m_pGLayout = NULL;
	}
}

void Socket::init( unsigned int size, unsigned int afMask)
{
	unsigned int coreMask = 1;
	QCheckBox *coreChk;
	QString coreName = "Core ";

	for(unsigned int i=0; i< size; i++)
	{
		coreChk= new QCheckBox(this);
		RETURN_IF_NULL(coreChk,this);


		// Parse out the afMask	
		if((afMask & coreMask) != 0)	
			coreChk->setChecked(true);
		else
			coreChk->setChecked(false);

		coreMask = coreMask << 1;	

		connect(this,SIGNAL(selectAllCores(bool)),
			coreChk,SLOT(setChecked(bool)));

		coreChk->setText(coreName + QString::number(i));

		m_chkBox_vec.push_back(coreChk);
	}
}

void Socket::onSelectAllCores(bool state)
{
	emit selectAllCores(state);
}


void Socket::getCheckedAffinity(unsigned int * checkedMask)
{
	unsigned int count = m_chkBox_vec.size();

	// Clear mask
	*checkedMask = 0;

	for(unsigned int i=0 ; i<count ; i++)
	{
		unsigned int coreMask = 1;
		if(m_chkBox_vec[i]->isOn())
		{
			// Set mask
			*checkedMask = *checkedMask | (coreMask << i);
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////

CpuAffinityDlg::CpuAffinityDlg( QWidget* parent, 
			   Qt::WFlags fl) :
QDialog(parent,fl|CA_DLG_FLAGS)
{
	setupUi(this);
	m_numSockets = 0;
	m_numCpusPerSocket = 0;
	m_allState = true;
	m_pScrollView = NULL;

	// SelectAll Button
	QObject::connect((QPushButton*)m_pSelectAllBtn, SIGNAL(clicked()), 
		this, SLOT(onSelectAllCores()));
	// ClearAll Button
	QObject::connect((QPushButton*)m_pClearAllBtn, SIGNAL(clicked()), 
		this, SLOT(onClearAllCores()));
	// OK Button
	QObject::connect((QPushButton*)m_pOk, SIGNAL(clicked()), 
		this, SLOT(onOk()));

	setModal(true);
}


CpuAffinityDlg::~CpuAffinityDlg()
{

}

void CpuAffinityDlg::init(unsigned int numSockets, 
			  unsigned int numCpusPerSocket,
			  QString afMaskString)

{
	Socket *socket;
	QString cpuName = "Socket ";

	// Sanity check
	if( numSockets == 0 
	||  numCpusPerSocket == 0) {
		return;
	}

	m_numSockets = numSockets;
	m_numCpusPerSocket = numCpusPerSocket;

	// Sanity check
	if(afMaskString.isEmpty()) {
		m_afMask.init();
		m_afMask.setAllCpus();
	} else {
		m_afMask.setAffinityFromHexString(string(afMaskString.toAscii().data()));
	}

	//***********************************************************
	// mainLayout is inside m_pFrame
	Q3VBoxLayout *mainLayout = new Q3VBoxLayout(m_pFrame);
	RETURN_IF_NULL(mainLayout,this);

	//***********************************************************
	// ScrollView is inside mainLayout 
	m_pScrollView = new Q3ScrollView(m_pFrame);
	RETURN_IF_NULL(m_pScrollView,this);

	m_pScrollView->setHScrollBarMode(Q3ScrollView::Auto);
	m_pScrollView->setVScrollBarMode(Q3ScrollView::AlwaysOn);

	mainLayout->add(m_pScrollView);	

	//***********************************************************
	// subLayout is inside m_pScrollView->viewport()
	Q3VBoxLayout *subLayout = new Q3VBoxLayout(m_pScrollView->viewport());
	RETURN_IF_NULL(subLayout,this);

	//***********************************************************
	// frame1 is inside m_pScrollView->viewport()
	Q3Frame * frame1 = new Q3Frame(m_pScrollView->viewport());
	RETURN_IF_NULL(frame1,this);

	m_pScrollView->addChild(frame1);	
	subLayout->add(frame1);

	//***********************************************************
	// subLayout2 is inside frame1. This is the layout for sockets
	Q3VBoxLayout *subLayout2 = new Q3VBoxLayout(frame1,MARGIN_SIZE);
	RETURN_IF_NULL(subLayout,this);

	//***********************************************************
	// For each socket
	for( unsigned int i=0; i<numSockets ; i++)
	{
		unsigned int sockMask = 0;
		socket = new Socket(cpuName + QString::number(i),frame1);
		RETURN_IF_NULL(socket,this);

		for (unsigned int j=0 ; j < m_numCpusPerSocket; j++) {
			unsigned int cpuIndex = (i * m_numCpusPerSocket) + j;
			int m = m_afMask.getAffinityMaskForCpu(cpuIndex);
			if (m != -1) {
				sockMask = sockMask | (m << j);
			}
		}

		socket->init( numCpusPerSocket,sockMask);

		connect(this, SIGNAL(selectAllCores(bool)), 
			socket, SLOT(onSelectAllCores(bool)));

		subLayout2->add(socket);	
		m_socket_vec.push_back(socket);
	}

	// Size constrain
	this->setMaximumHeight(MAX_DLG_HEIGHT);

	// Force Resize to make everything look nice
	this->show();
	this->resize(400,(100*numSockets));


	return ;
}

void CpuAffinityDlg::onSelectAllCores()
{
	emit selectAllCores(true);
}

void CpuAffinityDlg::onClearAllCores()
{
	emit selectAllCores(false);
}

CA_Affinity * CpuAffinityDlg::getAffinityMask()
{
	return & m_afMask;;
}

void CpuAffinityDlg::onOk()
{
	unsigned int count = m_socket_vec.size();
	unsigned int socketCheck = 0;	

	// Clear mask;	
	m_afMask.clear();

	// For each socket
	for(unsigned int i=0 ; i<count ; i++) {
		m_socket_vec[i]->getCheckedAffinity(&socketCheck);

		// Get checked core	
		unsigned int cpuIndex = (i*m_numCpusPerSocket);
		for (unsigned int j = 0 ; j < m_numCpusPerSocket; j++) {
			m_afMask.setAffinityMaskForCpu(cpuIndex+j,
					((socketCheck >> j) & 1));
		}
	}

	if(!m_afMask.isZero()) {
		accept ();
	} else {
		QMessageBox::critical (this, "CodeAnalyst Error", 
			"The Cpu affinity cannot be 0, please check at least one core.");
		return;
	}
}
