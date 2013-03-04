//$Id: $

/*
// CodeAnalyst for Open Source
// Copyright 2007 Advanced Micro Devices, Inc.
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

#include <q3filedialog.h>
#include <q3listview.h>
#include <qtoolbutton.h>
#include <qmessagebox.h>
#include "ProcessFilterDlg.h"
#include "stdafx.h"

ProcessFilterDlg::ProcessFilterDlg ( 
	QWidget* parent,
	Qt::WFlags fl )
:QDialog( parent, fl|CA_DLG_FLAGS)
{
	setupUi(this);
}

ProcessFilterDlg::~ProcessFilterDlg()
{
}

void ProcessFilterDlg::init(QStringList strList )
{
	// Add the column to ListView and adjust width
	m_pListView->addColumn("Processes");
	m_pListView->setResizeMode(Q3ListView::AllColumns);	

  	// Insert each filter path to the view
	QStringList::iterator it = strList.begin();
	QStringList::iterator it_end = strList.end();
	for(; it != it_end ; it++)
	{
		Q3ListViewItem *item = new Q3ListViewItem(m_pListView);
		item->setText(0,(*it));
		m_pListView->insertItem(item);
	}

	m_pEdit->setEnabled(m_pListView->childCount() > 0);
	m_pRemove->setEnabled(m_pListView->childCount() > 0);
}

void ProcessFilterDlg::onNew()
{
	QString newPath;

	newPath = Q3FileDialog::getOpenFileName(NULL, NULL,
                this, NULL,
                "Please Specify Path to an Application.");

	// Insert new item
	if(!newPath.isEmpty())
	{
		Q3ListViewItem *item = new Q3ListViewItem(m_pListView);
		item->setText(0,newPath);
		m_pListView->insertItem(item);
	}
	m_pEdit->setEnabled(m_pListView->childCount() > 0);
	m_pRemove->setEnabled(m_pListView->childCount() > 0);
}

void ProcessFilterDlg::onEdit()
{
	// Get the selected item
	Q3ListViewItem* cur = m_pListView->selectedItem();
	if(cur == NULL){
		QMessageBox::warning(this,"CodeAnalyst Warning", "Please select a process to edit.\n");
		return;	
	}

	QString newPath = Q3FileDialog::getOpenFileName(cur->text(0), NULL,
                this, NULL,
                "Please Specify Path to an Application.");

	// Edit Item
	if(!newPath.isEmpty())
	{
		cur->setText(0,newPath);
	}
	m_pEdit->setEnabled(m_pListView->childCount() > 0);
	m_pRemove->setEnabled(m_pListView->childCount() > 0);
}

void ProcessFilterDlg::onRemove()
{
	// Get the selected item
	Q3ListViewItem* cur = m_pListView->selectedItem();
	if(cur == NULL){
		QMessageBox::warning(this,"CodeAnalyst Warning", "Please select a process to remove.\n");
		return;	
	}

	m_pListView->takeItem((Q3ListViewItem*)cur);
	delete cur;
	m_pEdit->setEnabled(m_pListView->childCount() > 0);
	m_pRemove->setEnabled(m_pListView->childCount() > 0);
}

QStringList ProcessFilterDlg::getStringList()
{
	QStringList list;

	Q3ListViewItem *cur = m_pListView->firstChild();
	if(cur == NULL) return QStringList();
	do
	{
		list.push_back(cur->text(0));	

		if( cur == m_pListView->lastItem())
			break;
		else
			cur =  cur->nextSibling();
	}while(cur != NULL);	
	
	return list;
}
