
/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2008 Advanced Micro Devices, Inc.
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

#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qcombobox.h>
#include <qlistview.h>
#include <qvaluelist.h>
#include <qtextedit.h>
#include <qmessagebox.h>
#include <qlistbox.h>
#include "NewSessionWizard.h"
#include "DiffAnalystWindow.h"

QString m_CADiffDescription = 
	"Create diff view from CodeAnalyst profiling session files (*.TBP, *.EBP).\n"
	"\n"
	"This mode allows comparision of performance profiling data collected "
	"previously by CodeAnalyst.\n"  
	"\n"
	"Example use cases :\n"
	"    - Compare performance from multiple runs.\n"
	"    - Compare performance of appications which are compiled differently.\n"
	"    - Correlate performance data collected by different methods.\n" 
	"      (i.e. Event-based vs.Instruction-based sampling)\n"
	"\n"
	"Besides, users can use can also statically analyze Basic-Block, \n"
	"Memory accesses pattern, and Inline functions\n" 
;

QString m_BinDiffDescription = 
	"Create diff view from any ELF Binary files. Users can use this mode to "
	"statically analyze Basic-Block, Memory accesses pattern, and Inline functions\n\n" 
	;
			
NewSessionWizard::NewSessionWizard( QWidget* parent, 
			QString nextName,
			const char* name, 
			bool modal, 
			WFlags fl)
: iNewSessionWizard(parent, name, modal, fl)
{
	m_pDiffSessionName->setText(nextName);
	setHelpEnabled(page(WIZARD_PAGE),false);
	setHelpEnabled(page(SESSION_DIFF_PAGE),false);
	setHelpEnabled(page(MODULE_DIFF_PAGE),false);
}

NewSessionWizard::~NewSessionWizard()
{

}

bool NewSessionWizard::init()
{
	bool ret =false;
	m_pModeButtonGroup->setExclusive(true);
	m_pCAProfDiff->setChecked(true);

	m_pSessionListView->setResizeMode(QListView::AllColumns);
	m_pSessionListView->setAllColumnsShowFocus(true);
	m_pSessionListView->setColumnAlignment(0,Qt::AlignRight);
	m_pSessionListView->setColumnAlignment(1,Qt::AlignRight);
	m_pSessionListView->setColumnAlignment(2,Qt::AlignRight);
	m_pSessionListView->setSorting(-1);
	
	m_pModuleListView->setResizeMode(QListView::AllColumns);
	m_pModuleListView->setAllColumnsShowFocus(true);

	m_pSessionFile->insertItem(QString("--- Please Browse Session File ---"));	

	connect(m_pSessionFile,SIGNAL(activated(const QString&)),
		this, SLOT(onSessionFileChanged()));
	connect(m_pTask,SIGNAL(highlighted(const QString&)),
		this, SLOT(populateModuleComboBox()));
	connect(m_pModule1,SIGNAL(highlighted(const QString&)),
		this, SLOT(onModule1Changed()));

	m_pEditBtn->setEnabled(false);
	m_pRemoveBtn->setEnabled(false);

	ret = true;
	return ret;
}

int NewSessionWizard::getSessionDiffInfo(SESSION_DIFF_INFO_VEC *vec)
{
	int count = 0;
	if(vec == NULL)
		return -1;
	vec->clear();
	QListViewItem *cur = NULL;
	for(count=0, cur=m_pSessionListView->firstChild() 
	; count < m_pSessionListView->childCount()
	; count++, cur=cur->nextSibling())
	{
		SESSION_DIFF_INFO info;
		info.sessionFile = cur->text(0);
		info.task	= cur->text(1);
		info.module	= cur->text(2);
		vec->push_back(info);
	}
	return count;
}




//////////////////////////////////////
// SLOTS
void NewSessionWizard::next()
{
	// Check if name already exist
	DiffAnalystWindow *parent = (DiffAnalystWindow*)parentWidget();	
	if(parent->diffSessionNameExists(getSessionDiffName()))
	{
		QString tmp = QString("Diff Session with name \"") + getSessionDiffName() 
				+ "\" already exists. Please enter a new name.";
		QMessageBox::critical(this,"DiffAnalyst Error",tmp);
		m_pDiffSessionName->setFocus();
		return;
	}

	if(getSessionDiffName().isEmpty())
	{
		QString tmp = QString("ERROR:\n")
				+ "Empty Diff Sessin Name. Please enter a new name.";
		QMessageBox::critical(this,"DiffAnalyst Error",tmp);
		m_pDiffSessionName->setFocus();
		return;
	}

	switch(indexOf(currentPage())) {
		case WIZARD_PAGE:
			if (m_pCAProfDiff->isChecked()) { 
				setFinishEnabled(page(SESSION_DIFF_PAGE), true);
				setNextEnabled(page(SESSION_DIFF_PAGE), false);
				
				showPage(page(SESSION_DIFF_PAGE));

				if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
				{
					finishButton()->setEnabled(true);
				}else{
					finishButton()->setEnabled(false);
				}


			} else if (m_pModDiff->isChecked()) {
				setFinishEnabled(page(MODULE_DIFF_PAGE), true);
				setNextEnabled(page(MODULE_DIFF_PAGE), false);
				showPage(page(MODULE_DIFF_PAGE));
			}
			break;
	}
}
void NewSessionWizard::back()
{
	switch(indexOf(currentPage())) {
		case SESSION_DIFF_PAGE:
		case MODULE_DIFF_PAGE:
			setFinishEnabled(currentPage(), false);
			showPage(page(WIZARD_PAGE));
			break;
	}
}


bool NewSessionWizard::populateTaskComboBox()
{
	//NOTE: This should be the same as in CA
	bool ret = false;
	QValueList<SYS_LV_ITEM> list;
	if(!(ret = m_pTbsReader->readProcInfo(list)))
		return ret;	

	//Set up empty list
	m_pTask->clear();
	int num = list.size();
	for (int i = 0 ; i < num; i++)
	{
		m_pTask->insertItem("",0);
	}	

	m_pTask->changeItem(QString("All Tasks"),0);

	QValueList<SYS_LV_ITEM>::iterator it = list.begin();
	QValueList<SYS_LV_ITEM>::iterator end = list.end();
	for(;it != end; it++)
	{
		m_pTask->changeItem((*it).ModName,(*it).taskId);
	}

	m_pTask->setSelected(0,true);
	
	ret = true;
	return ret;
}

bool NewSessionWizard::populateModuleComboBox()
{
	bool ret = false;
	QValueList<SYS_LV_ITEM> list;
	if(!(ret = m_pTbsReader->readModInfo(list)))
		return ret;	

	m_pModule1->clear();
	onModule1Changed();

	// Get current Task
	int taskId = m_pTask->currentItem();
	QValueList<SYS_LV_ITEM>::iterator it = list.begin();
	QValueList<SYS_LV_ITEM>::iterator end = list.end();
	for(;it != end; it++)
	{
		if ( m_pModule1->findItem((*it).ModName) == 0
		&& ((*it).taskId == taskId || taskId == 0))
		{
			m_pModule1->insertItem((*it).ModName);
		}
	}
	
	ret = true;
	return ret;
}
void NewSessionWizard::onBrowse()
{
        QString str;
	str = QDir::home ().path();
	str.replace ('\\', '/');
	str += "/AMD/CodeAnalyst";


	if(indexOf(currentPage()) == SESSION_DIFF_PAGE)
	{
		QString ebpFile = QFileDialog::getOpenFileName( str, 
			"TBP/EBP File (*.ebp *.tbp)", this, "FileBrowse",  
			"Select CodeAnalyst Session File" );

		if( ebpFile.isEmpty() ) 
			return;
		m_pSessionFile->insertItem(ebpFile,0);
		m_pSessionFile->setCurrentItem(0);
		
		onSessionFileChanged();	

	}else if(indexOf(currentPage()) == MODULE_DIFF_PAGE) {
		if (!m_pModule2->currentText().isEmpty())
		{
			str = m_pModule2->currentText().section ('\"', 1, 1);
		}
		QString moduleFile = QFileDialog::getOpenFileName( str, 
			"Binary File (*)", this, "FileBrowse",  
			"Select Binary File" );

		if( moduleFile.isEmpty() ) 
			return;
		m_pModule2->setEditText(moduleFile);
	}
}

void NewSessionWizard::onSessionFileChanged()
{
	QString ebpFile = m_pSessionFile->currentText();
	// Query TbpReader to fill in the Task combo box
	m_pTbsReader = new TbsReader();
	if(m_pTbsReader == NULL) return ;	

	m_pTask->clear();	
	m_pModule1->clear();	
	
	if (!m_pTbsReader->OpenTbsFile(ebpFile)) 
	{
		return;
	}

	populateTaskComboBox();
}

void NewSessionWizard::onAdd()
{

	if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
	{
		QMessageBox::critical(this,"DiffAnalyst Error"
			,"Exceed maximum number of CA profiles allowed.");	
		return;
	}

	// Verify
	if(m_pSessionFile->currentText().startsWith(QString("---"))
	|| m_pTask->currentText().startsWith(QString("---"))
	|| m_pModule1->currentText().startsWith(QString("---"))
	|| m_pSessionFile->currentText().isEmpty()
	|| m_pTask->currentText().isEmpty()
	|| m_pModule1->currentText().isEmpty())
	{
		return;
	}
	
	QListViewItem *tmp = new QListViewItem(m_pSessionListView,
		m_pSessionListView->lastItem(),
		m_pSessionFile->currentText(),
		m_pTask->currentText(),
		m_pModule1->currentText());
	
	m_pSessionListView->insertItem(tmp);
	onReset();	

	if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
	{
		finishButton()->setEnabled(true);
	}else{
		finishButton()->setEnabled(false);
	}
	
	m_pEditBtn->setEnabled(true);
	m_pRemoveBtn->setEnabled(true);
}

void NewSessionWizard::onReset()
{
	m_pSessionFile->setCurrentText(QString("--- Please Browse Session File ---"));	
	m_pTask->clear();
	m_pModule1->clear();
	onModule1Changed();
}

void NewSessionWizard::onEdit()
{
	QListViewItem *cur = m_pSessionListView->selectedItem();
	if(cur==NULL)
	{
		QMessageBox::warning(this,"CodeAnalyst Warning", "Please select session to edit.\n");
		return;
	}
	
	m_pSessionFile->setCurrentText(cur->text(0));	
	onSessionFileChanged();

	// Find Task text
	for(int i = 0 ; i < m_pTask->count() ; i++)
	{
		if(!m_pTask->text(i).compare(cur->text(1)))
		{
			m_pTask->setCurrentItem(i);
			break;
		}
	}
	populateModuleComboBox();

	// Find Module text
	for(int i = 0 ; i < m_pModule1->count() ; i++)
	{
		if(!m_pModule1->text(i).compare(cur->text(2)))
		{
			m_pModule1->setCurrentItem(i);
			break;
		}
	}
	onModule1Changed();
	
	m_pSessionListView->takeItem(cur);
	delete cur;
	cur = NULL;
	
	if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
	{
		finishButton()->setEnabled(true);
	}else{
		finishButton()->setEnabled(false);
	}
	
	if(m_pSessionListView->childCount() == 0)
	{
		m_pEditBtn->setEnabled(false);
		m_pRemoveBtn->setEnabled(false);
	}
}

void NewSessionWizard::onRemove()
{
	QListViewItem *cur = m_pSessionListView->selectedItem();
	if(cur==NULL)
	{
		QMessageBox::warning(this,"CodeAnalyst Warning", "Please select session to remove.\n");
		return;
	}
	m_pSessionListView->takeItem(cur);
	delete cur;
	cur = NULL;
	
	if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
	{
		finishButton()->setEnabled(true);
	}else{
		finishButton()->setEnabled(false);
	}
	
	if(m_pSessionListView->childCount() == 0)
	{
		m_pEditBtn->setEnabled(false);
		m_pRemoveBtn->setEnabled(false);
	}
}

void NewSessionWizard::onModuleAddBtn()
{
}

void NewSessionWizard::onModuleRemoveBtn()
{
}

void NewSessionWizard::onModule1Changed()
{
	if( m_pModule1->selectedItem() != NULL)
	{
		m_pAddBtn->setEnabled(true);
	}else{
		m_pAddBtn->setEnabled(false);
	}
}

void NewSessionWizard::onCAProfileDiffChanged(bool b)
{
	if(b)
	{
		m_pDescription->setText(m_CADiffDescription);	
	}
}

void NewSessionWizard::onModuleDiffChanged(bool b)
{
	if(b)
	{
		m_pDescription->setText(m_BinDiffDescription);	
	}
}

QString NewSessionWizard::getSessionDiffName()
{ 
	return m_pDiffSessionName->text();
}
