#include <qpushbutton.h>
#include <q3listbox.h>
#include <q3buttongroup.h>
#include <q3listview.h>
#include <qcombobox.h>
#include <Q3ValueList>

#include "NewDiffSessionDlg.h"

NewDiffSessionDlg::NewDiffSessionDlg( 
	QString cawPath,
	QWidget* parent, 
	Qt::WFlags fl )
: QDialog( parent, fl)
{
	m_cawPath = cawPath;
}

NewDiffSessionDlg::~NewDiffSessionDlg()
{

}

bool NewDiffSessionDlg::init(DiffSessionInfo *sess)
{
	bool ret =false;
	m_pSessionListView->setResizeMode(Q3ListView::AllColumns);
	m_pSessionListView->setAllColumnsShowFocus(true);
	m_pSessionListView->setColumnAlignment(0,Qt::AlignRight);
	m_pSessionListView->setColumnAlignment(1,Qt::AlignRight);
	m_pSessionListView->setColumnAlignment(2,Qt::AlignRight);
	m_pSessionListView->setSorting(-1);

	m_pSessionFile->insertItem(QString("--- Please Select Session File ---"));	

	connect(m_pSessionFile,SIGNAL(activated(const QString&)),
		this, SLOT(onSessionFileChanged()));
	connect(m_pTask,SIGNAL(highlighted(const QString&)),
		this, SLOT(populateModule()));
	connect(m_pModule,SIGNAL(highlighted(const QString&)),
		this, SLOT(onModuleChanged()));

	ret = initSessionComboBox(sess);

	if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
	{
		buttonOk->setEnabled(true);
	}else{
		buttonOk->setEnabled(false);
	}

	m_pEditBtn->setEnabled(false);
	m_pRemoveBtn->setEnabled(false);

	return ret;
}

bool NewDiffSessionDlg::initSessionComboBox(DiffSessionInfo *sess)
{
	bool ret = false;
	// Query CAW file for list of sessions	


	m_pCawFile = new CawFile(m_cawPath);
	if( m_pCawFile == NULL
	|| !m_pCawFile->openCawFile())
	{
		return ret;
	}

//	QString path = 	m_cawPath.section("/",0,-2) + "/";
	QString path ;	
	QStringList Tsessions =	m_pCawFile->getSessionList(TIMER_TRIGGER);
	QStringList Esessions = m_pCawFile->getSessionList(EVENT_TRIGGER);

	QStringList::Iterator it = Tsessions.begin();
	for( unsigned int i = 0; i < Tsessions.count() ; i++, it++)
	{
		QString ebpFile =  path + (*it) + ".tbp.dir/" + (*it) + ".tbp";
		m_pSessionFile->insertItem(ebpFile);
	}
	
	it = Esessions.begin();
	for( unsigned int i = 0; i < Esessions.count() ; i++, it++)
	{
		QString ebpFile = path + (*it) + ".ebp.dir/" + (*it) + ".ebp";
		m_pSessionFile->insertItem(ebpFile);
	}
	
	if(sess != NULL)
	{
		QString searchStr;
		if(sess->type == 0)
			searchStr = path + sess->sessionFile + ".tbp.dir/" + sess->sessionFile + ".tbp";
		else if(sess->type == 1 || sess->type == 2)
			searchStr = path + sess->sessionFile + ".ebp.dir/" + sess->sessionFile + ".ebp";

		//Search for session in the ComboBox
		int index = -1;
		for(int i = 0 ; i < m_pSessionFile->count() ; i++)
		{
			if(m_pSessionFile->text(i) == searchStr)
			{
				index = i;
				break;
			}
		}

		m_pSessionFile->setCurrentItem(index);
	}else{
		m_pSessionFile->setCurrentItem(0);

	}	
	onSessionFileChanged();	

	ret = true;
	
	return ret;
}

void NewDiffSessionDlg::onAdd()
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
	|| m_pModule->currentText().startsWith(QString("---"))
	|| m_pSessionFile->currentText().isEmpty()
	|| m_pTask->currentText().isEmpty()
	|| m_pModule->currentText().isEmpty())
	{
		return;
	}
	
	Q3ListViewItem *tmp = new Q3ListViewItem(m_pSessionListView,
		m_pSessionListView->lastItem(),
		m_pSessionFile->currentText(),
		m_pTask->currentText(),
		m_pModule->currentText());
	
	m_pSessionListView->insertItem(tmp);
	onReset();	

	if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
	{
		buttonOk->setEnabled(true);
	}else{
		buttonOk->setEnabled(false);
	}
	
	m_pEditBtn->setEnabled(true);
	m_pRemoveBtn->setEnabled(true);

}

void NewDiffSessionDlg::onReset()
{
	m_pSessionFile->setCurrentText(QString("--- Please Select Session File ---"));	
	m_pTask->clear();
	m_pModule->clear();
	onModuleChanged();
}

void NewDiffSessionDlg::onEdit()
{
	Q3ListViewItem *cur = m_pSessionListView->selectedItem();
	if(cur==NULL)
	{
		QMessageBox::warning(this,"CodeAnalyst Warning", "Please select session to edit.\n");
		return;
	}
	
	m_pSessionFile->setCurrentText(cur->text(0));	
	onSessionFileChanged();

	// Find Task text
	for(unsigned int i = 0 ; i < m_pTask->count() ; i++)
	{
		if(!m_pTask->text(i).compare(cur->text(1)))
		{
			m_pTask->setCurrentItem(i);
			break;
		}
	}
	populateModule();

	// Find Module text
	for(unsigned int i = 0 ; i < m_pModule->count() ; i++)
	{
		if(!m_pModule->text(i).compare(cur->text(2)))
		{
			m_pModule->setCurrentItem(i);
			break;
		}
	}
	onModuleChanged();
	
	m_pSessionListView->takeItem(cur);
	delete cur;
	cur = NULL;
	
	if(m_pSessionListView->childCount() == MAX_NUM_DIFF)
	{
		buttonOk->setEnabled(true);
	}else{
		buttonOk->setEnabled(false);
	}
	
	if(m_pSessionListView->childCount() == 0)
	{	
		m_pEditBtn->setEnabled(false);
		m_pRemoveBtn->setEnabled(false);
	}
}

void NewDiffSessionDlg::onRemove()
{
	Q3ListViewItem *cur = m_pSessionListView->selectedItem();
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
		buttonOk->setEnabled(true);
	}else{
		buttonOk->setEnabled(false);
	}

	if(m_pSessionListView->childCount() == 0)
	{	
		m_pEditBtn->setEnabled(false);
		m_pRemoveBtn->setEnabled(false);
	}
}

bool NewDiffSessionDlg::populateTask()
{
	//NOTE: This should be the same as in DA
	bool ret = false;
	Q3ValueList<SYS_LV_ITEM> list;
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
	Q3ValueList<SYS_LV_ITEM>::iterator it = list.begin();
	Q3ValueList<SYS_LV_ITEM>::iterator end = list.end();
	for(;it != end; it++)
	{
		m_pTask->changeItem((*it).ModName,(*it).taskId);
	}

	m_pTask->setSelected(0,true);
	
	ret = true;
	return ret;
}

bool NewDiffSessionDlg::populateModule()
{
	bool ret = false;
	Q3ValueList<SYS_LV_ITEM> list;
	if(!(ret = m_pTbsReader->readModInfo(list)))
		return ret;	

	m_pModule->clear();
	onModuleChanged();

	// Get current Task
	unsigned int taskId = m_pTask->currentItem();
	Q3ValueList<SYS_LV_ITEM>::iterator it = list.begin();
	Q3ValueList<SYS_LV_ITEM>::iterator end = list.end();
	for(;it != end; it++)
	{
		if ( m_pModule->findItem((*it).ModName) == 0
		&& ((*it).taskId == taskId || taskId == 0))
		{
			m_pModule->insertItem((*it).ModName);
		}
	}
	
	ret = true;
	return ret;
}

void NewDiffSessionDlg::onSessionFileChanged()
{
	QString path = m_cawPath.section("/",0,-2);
	QString ebpFile = path + "/" + m_pSessionFile->currentText();


	// Query TbpReader to fill in the Task combo box
	m_pTbsReader = new TbsReader();
	if(m_pTbsReader == NULL) return ;	

	m_pTask->clear();	
	m_pModule->clear();	
	
	if (!m_pTbsReader->OpenTbsFile(ebpFile)) 
	{
		return;
	}

	populateTask();
}

void NewDiffSessionDlg::onModuleChanged()
{
	if( m_pModule->selectedItem() != NULL)
	{
		m_pAddBtn->setEnabled(true);
	}else{
		m_pAddBtn->setEnabled(false);
	}
}

QStringList NewDiffSessionDlg::getDiffSession()
{
	QStringList list;
	QString path = 	m_cawPath.section("/",0,-2) + "/";
	Q3ListViewItem *pItem = m_pSessionListView->firstChild();
	for(; pItem != NULL ; pItem = pItem->nextSibling())
	{
		list.push_back( path + pItem->text(0) + ":" + 
				pItem->text(1) + ":" +
				pItem->text(2) );
	}
	return list;
}

