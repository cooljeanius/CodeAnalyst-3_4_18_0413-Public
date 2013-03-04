// implementation for CSessionNavigator class

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

#include <qcombobox.h>
#include <qdatetime.h>
#include <qclipboard.h>
#include <QKeyEvent>
#include <QPixmap>
#include <Q3PopupMenu>

#include "stdafx.h"
#include "auxil.h"
#include "helperAPI.h"
#include "sessionnav.h"
#include "sysdataview.h"
#include "moddataview.h"
#include "TaskTab.h"
#include "sourceview.h"
#include "atuneoptions.h"
#include "ViewCfg.h"
#include "EventMaskEncoding.h"
#include <math.h>
#include "eventsfile.h"
#include "dataagg.h"
#include "amdArrow.xpm"
#include "SeparateCpuIcon.xpm"
#include "percentIcon.xpm"
#include "PerfEvent.h"

#define _SESSIONNAV_ICONS_
#include "xpm.h"
#undef  _SESSIONNAV_ICONS_

static const char SYS_DATA_STRING[] = "System Data";
//static const char SYS_GRAPH_STRING[] = "System Graph";
static const char SYS_TASK_STRING[] = "System Tasks";

static const char SYS_DATA_WIDGET[] = "SysData";
static const char MOD_DATA_WIDGET[] = "ModData";
static const char DASM_WIDGET[] = "DASM";
static const char SRC_VIEW_WIDGET[] = "SRCVIEW";


CSessionNavigator::CSessionNavigator (
	QString sampling_file_path,
	QWidget * parent, 
	const char *name,
	int wflags)
:  Q3MainWindow (parent, name, (QFlags<Qt::WindowType>)wflags)
{
	m_pViewBox = NULL;
	m_pViewCfg = NULL;
	m_pProfReader = NULL;
	m_pProfInfo = NULL;
	m_tbp_file_path = sampling_file_path;
	setDockMenuEnabled (false);
	setIcon (QPixmap (amdArrowLogo));

	setAttribute(Qt::WA_DeleteOnClose);
}


CSessionNavigator::~CSessionNavigator ()
{
	if (NULL != m_pProfReader)
	{
		delete m_pProfReader;
		m_pProfReader = NULL;
	}
	ClearSessionGlobalMap(m_tbp_file_path);
}

void clearTaskSampleMap(TaskSampleMap &tMap)
{
	TaskSampleMap::iterator tIt = tMap.begin();
	TaskSampleMap::iterator tEnd = tMap.end();
	for (; tIt != tEnd; tIt++) {
		tIt->second.clear();
	}
}


void CSessionNavigator::setupViewToolbar()
{
	// NOTE: This must be called after the tbp file is 
	// actually initialized in order to read the CPU family 
	// information
	if (!m_pProfInfo)
		return;

	unsigned long family, model, stepping;
	family = m_pProfInfo->m_cpuFamily;
	model = m_pProfInfo->m_cpuModel;
	if (family == 0)
		getDefaultCpuFamilyModel(&family, &model, &stepping);
	m_views.readAvailableViews(family);

	//*********************************************************
	/*
	* Add View Toolbar
	*/
	m_pViewBar = new Q3ToolBar( this, "View Tool Bar" );
	RETURN_IF_NULL (m_pViewBar, this);
	addToolBar( m_pViewBar, "View Tool Bar", Qt::DockTop, FALSE );

	//*********************************************************
	/*
	* Add View Combo Box 
	*/
	m_pViewBox = new QComboBox (m_pViewBar);
	RETURN_IF_NULL (m_pViewBox, this);

	connect (m_pViewBox, SIGNAL (activated (const QString &)), 
		SLOT (onViewCfgChanged (const QString &)));
	connect (m_pViewBox, SIGNAL (highlighted ( const QString &)),
		SLOT (onViewMaybeTip (const QString &)));

	//*********************************************************
	/*
	* Add View Config Button
	*/
	m_pViewCfg = new QPushButton ("Manage", m_pViewBar);
	RETURN_IF_NULL (m_pViewCfg, this);

	connect (m_pViewCfg, SIGNAL (clicked ()), SLOT (onConfigureView ()));
	
	m_pViewBar->addSeparator();
	
	//*********************************************************
	/* Separate CPU button*/
	QIcon iconCpu(QPixmap((const char**)SeparateCpuIcon));
	m_pCpu = new QPushButton(iconCpu, "",m_pViewBar);
	m_pCpu->setMaximumHeight(20);
	m_pCpu->setMaximumWidth(20);
	m_pCpu->setToggleButton(true);
	connect(m_pCpu,SIGNAL(toggled(bool)), 
		this,SLOT(onCpuToggled(bool)));
	m_pCpu->setToolTip(QString("Separated by Cpu"));
	
	/* Show Percentage Button*/
	QIcon iconPercent(QPixmap((const char**)percentIcon));
	m_pPercent = new QPushButton(iconPercent, "",m_pViewBar);
	m_pPercent->setMaximumHeight(20);
	m_pPercent->setMaximumWidth(20);
	m_pPercent->setToggleButton(true);
	connect(m_pPercent,SIGNAL(toggled(bool)), 
		this,SLOT(onPercentToggled(bool)));
	m_pPercent->setToolTip(QString("Show Percentage"));

	updateViewButtons();

	//*********************************************************
	// Get list of event from TBP files
	// Note: In TBP version 6, these events are 64-bit Event+UMask encoded.
	m_views.addAllDataView (&(m_pProfInfo->m_eventVec), family, model);

	int indexDefault = 0;
	//gets the list of views applicable to the data set
	QStringList viewList = m_views.getListOfViews (&(m_pProfInfo->m_eventVec), &indexDefault);
	m_pViewBox->insertStringList (viewList);

	//*********************************************************

	// Modified by Suravee
	// Set default view
	//m_pViewBox->setCurrentItem (indexDefault);
	m_pViewBox->setCurrentItem (0);

	// TODO: Modify to add cpufamily information
	onViewCfgChanged (m_pViewBox->currentText());
	QString newTip;
	if (m_views.getViewTexts (m_pViewBox->currentText(), &newTip, NULL))
		m_pViewBox->setToolTip(newTip);
	m_pViewBox->show();
}


void CSessionNavigator::updateViewButtons()
{
	QString view = m_pViewBox->currentText();
	m_pCpu->setOn(m_views.getSeparateCpu (view));
	m_pPercent->setOn(m_views.getShowPercentage(view));
	
}


void CSessionNavigator::onCpuToggled(bool b)
{
	m_pCpu->setEnabled(false);
	m_views.setSeparateCpu (m_pViewBox->currentText(), b);
	onViewCfgChanged (m_pViewBox->currentText());
	m_pCpu->setEnabled(true);
}


void CSessionNavigator::onPercentToggled(bool b)
{
	m_pPercent->setEnabled(false);
	m_views.setShowPercentage(m_pViewBox->currentText(), b);
	onViewCfgChanged (m_pViewBox->currentText());
	m_pPercent->setEnabled(true);
}


bool CSessionNavigator::display (QString caption, 
				CawFile *pProject, 
				QString eventProfile)
{
	m_pProject = pProject;
	if (m_tbp_file_path.isEmpty ())
	{
		return false;
	}
	m_pProfReader = new CaProfileReader();
	RETURN_FALSE_IF_NULL (m_pProfReader, this);

	
	if (!m_pProfReader->open(string(m_tbp_file_path.toAscii().data()))) {
		QMessageBox::critical (this, "CodeAnalyst Error", 
			QString("Could not open profile ") + m_tbp_file_path);
		return false;
	}

	m_pProfInfo = m_pProfReader->getProfileInfo();
	if (!m_pProfInfo) {
		QMessageBox::critical (this, "CodeAnalyst Error", 
			QString("Read profile info for ") + m_tbp_file_path);
		return false;
	}

	calculateEventNorms (eventProfile);

	//*********************************************************
	// Setup the session tab
	m_pTabWidget = new QTabWidget( this );
	RETURN_FALSE_IF_NULL ( m_pTabWidget, this);
	QObject::connect( m_pTabWidget, 
		SIGNAL(currentChanged(QWidget *)), 
		SLOT(onCurrentTabChanged(QWidget *)) );	
	setFocusProxy   ( m_pTabWidget );
	setCentralWidget( m_pTabWidget );

	setupViewToolbar();

	//*********************************************************
	// Allocate the button to close source tabs
	m_pCloseTab = new QToolButton (m_pTabWidget);
	RETURN_FALSE_IF_NULL (m_pCloseTab, this);
	m_pCloseTab->setText (" X ");
	m_pCloseTab->setMaximumHeight(23);
	m_pCloseTab->setMaximumWidth(25);
	m_pCloseTab->setToolTip( "Close the current tab");
	connect (m_pCloseTab, SIGNAL (clicked()), SLOT (OnCloseTab()));

	m_pTabWidget->setCornerWidget (m_pCloseTab);
	m_pCloseTab->show();
	
	//*********************************************************
	// Add Tabs to the Navigator for the Data and Graph Views

	/*
	* System Data View
	*/
	m_sys_dat_view = new SystemDataTab (m_pProfReader, &m_viewShown,
						this, SYS_DATA_WIDGET); 
	RETURN_FALSE_IF_NULL (m_sys_dat_view, this);
	addTabToNavigatorBar (m_sys_dat_view, SYS_DATA_STRING);

	connect (this, SIGNAL (shownDataChange(ViewShownData*)), 
		m_sys_dat_view, SLOT (onViewChanged(ViewShownData*)));

	connect (m_sys_dat_view, SIGNAL (shownDataChange(ViewShownData*)), 
		this , SLOT (onShownColumnToggled(ViewShownData*)));

	connect (m_sys_dat_view,
		SIGNAL (moduleDblClicked (const CA_Module *, TAB_TYPE, unsigned int)),
		SLOT (onModuleDblClicked (const CA_Module *, TAB_TYPE, unsigned int)));

	connect (m_sys_dat_view,
		SIGNAL (moduleRightClicked (const CA_Module *, TAB_TYPE,unsigned int)),
		SLOT (onModuleDblClicked (const CA_Module *, TAB_TYPE, unsigned int)));

//	connect (m_sys_dat_view, 
//		SIGNAL (viewGraph ()),
//		SLOT (onViewSystemGraph ()));

	connect (m_sys_dat_view, 
		SIGNAL (viewTasks ()),
		SLOT (onViewSystemTask ()));

// FIXME [3.0]: This will be ported to use libCAdata
#if 0
	/*
	* Graph View Tab 
	*/
	m_sys_graph_view = new SysGraphView (&m_viewShown, this, 
					"SysGraph", Qt::WDestructiveClose);
	RETURN_FALSE_IF_NULL (m_sys_graph_view, this);

	connect (this, SIGNAL (shownDataChange(ViewShownData*)), m_sys_graph_view, 
		SLOT (onViewChanged(ViewShownData*)));
	connect (m_sys_graph_view, SIGNAL (shownDataChange(ViewShownData*)), 
		this , SLOT (onShownColumnToggled(ViewShownData*)));

	if (!m_sys_graph_view->init (m_pProfReader))
		return false;

	addTabToNavigatorBar (m_sys_graph_view, SYS_GRAPH_STRING);
	QObject::connect (m_sys_graph_view,
		SIGNAL (moduleDblClicked (QString, TAB_TYPE, unsigned int)),
		SLOT (onModuleDblClicked (QString, TAB_TYPE, unsigned int)));
	
	QObject::connect (m_sys_graph_view,
		SIGNAL (viewSysData()),
		SLOT (onViewSystemData()));
#endif

	m_pTabWidget->setCurrentPage(0);

	bool ret = m_sys_dat_view->display (caption);

	return ret;
} // CSessionNavigator::display


QWidget * CSessionNavigator::findTab (QString caption, QString filePath)
{
	for (int i= 0; i < m_pTabWidget->count(); i++)
	{
		QWidget * pWidget = m_pTabWidget->page (i);
		if (m_pTabWidget->tabToolTip (pWidget) == caption) 
		{
			if (!filePath.isEmpty()) 
			{
				if (pWidget->iconText() == filePath)
					return pWidget;
			} else
				return pWidget;
		}
	}
	return NULL;
}

bool CSessionNavigator::checkTimeStamp(QString modName)
{
	//***********************************************************
	/* 
	* TIMESTAMP SANITY CHECK
	* Description: Here we check if the profile time in TBP 
	* is older than the modules creation time.
	*/
	// Get TBP Timestamp
	QString tbp_dt_str = wstrToQStr(m_pProfInfo->m_timeStamp);
	tbp_dt_str = tbp_dt_str.simplifyWhiteSpace();

	/* NOTE:
	* We need to do this since QT has a bug in function 
	* QDateTime::fromString() !!!!
	*/
	struct tm tmp_dt;	
	strptime(tbp_dt_str.toAscii().data(),"%a %b %d %H:%M:%S %Y",&tmp_dt);
	tmp_dt.tm_isdst = -1; // Take into account daylight saving stuff
	time_t tmp_dt2 = mktime(&tmp_dt);

	QDateTime tbp_dt ;
	tbp_dt.setTime_t(tmp_dt2);

	QString oldModuleName = "";
	QFileInfo f_info = QFileInfo(modName);

	if (!f_info.isAbsolute()) {
		//NOTE: If not absolute path, we assume that this 
		//      is Java. We won't be checking anything.
		return true;
	}

	if (!f_info.exists()) {
		QMessageBox::critical(this,"CodeAnalyst Error", 
			"File not found : " + modName + "\n" +
			"Please make sure the file exists at the specified path.\n");
		return false;
	}
	
	// Check modules
	if(f_info.created() > tbp_dt)
		oldModuleName += (modName + "\n");

	// Give warning message if module is newer than profiling time
	if (oldModuleName.length() != 0) {
		QString msg = "Module:\n\n" + oldModuleName
			+ "\nhas creation time ("+ f_info.created().toString()
			+") later than the profiling time ("+ tbp_dt.toString()
			+").\n"
			+ "Symbol information might be inaccurate. Please re-run the profile.\n\n" 
			+ "Do you want to continue?\n"; 

		if( QMessageBox::warning(this,"CodeAnalyst Warning", msg, 
				QMessageBox::Yes, QMessageBox::No)	!= QMessageBox::Yes)
		{
			return false;
		}
	} 
	return true;
}


static bool bModProcessing = false;

void CSessionNavigator::onModuleDblClicked ( const CA_Module *pMod,
				TAB_TYPE type, 
				unsigned int taskId)
{
	if (bModProcessing)
		return;

	QString module_name = wstrToQStr(pMod->getPath());

	if (!checkTimeStamp(module_name))
		return;

	bModProcessing = true;
	
	QString caption;
	TaskSampleMap *pTMap = NULL;
	JitBlockPtrVec *pJitBlkPtrVec = NULL;	

	if (TAB_DATA == type) 
	{
		caption = module_name + " - Data";
		// Add the Data Tab if not already shown
		QWidget *pOldTab = findTab (caption);
		if (NULL == pOldTab)
		{
			// Initializing Module Data Tab
			ModuleDataTab *view = 	new ModuleDataTab (pMod,
								m_pProfReader, 
								&m_viewShown, 
								this,
								MOD_DATA_WIDGET);
	
			if (!view) {
				QMessageBox::critical(this, NOMEM_HEADER, NOMEM_MSG);
				goto out;
			}

//			AddModGlobalMap(m_tbp_file_path, module_name, &pTMap, &pJitBlkPtrVec);
//			view->setDataMapPointer(pTMap, pJitBlkPtrVec);
			
			addTabToNavigatorBar (view, caption);

			if (!view->initialize (taskId)) 
			{
				m_pTabWidget->removePage ( view );
				if(view != NULL)
				{
				    delete view;
				    view = NULL;
				}
				QMessageBox::warning (this, "CodeAnalyst symbols problem",
					"The module tab for " + module_name
					+ " is unavailable.");
				goto out;
			}

			m_pTabWidget->showPage ( view );

			QObject::connect (view,
				SIGNAL (symDblClicked (VADDR, PID_T, TID_T, const CA_Module*)),
				SLOT (onSymDblClicked (VADDR, PID_T, TID_T, const CA_Module*)));

//			QObject::connect (view,
//				SIGNAL (viewGraph (QString, const CA_Module *, TAB_TYPE)),
//				SLOT (onModuleDblClicked (QString, const CA_Module * , TAB_TYPE)));

			connect (this, SIGNAL (shownDataChange(ViewShownData*)), 
				view, SLOT (onViewChanged(ViewShownData*)));
			connect (view, SIGNAL (shownDataChange(ViewShownData*)),
				this , SLOT (onShownColumnToggled(ViewShownData*)));
			connect (this, SIGNAL (pidChanged(unsigned int)),
				view , SLOT (onPidChanged(unsigned int)));

		} else {
			
			emit pidChanged (taskId);
			m_pTabWidget->showPage ( pOldTab );
		}
	} else {
// FIXME [3.0]
#if 0
		caption = module_name + " - Graph";

		// Add the Graph Tab if not already shown
		QWidget *pOldTab = findTab (caption);
		if (NULL == pOldTab)
		{
			ModGraphView *graph = new ModGraphView (&m_viewShown, this, 
							"ModGraph", Qt::WDestructiveClose);
			if (!graph) {
				QMessageBox::critical(this, NOMEM_HEADER, NOMEM_MSG);
				goto out;
			}

			AddModGlobalMap(m_tbp_file_path, module_name, &pTMap, &pJitBlkPtrVec);

			if (!graph->init (m_pProfReader, (char *) module_name.data (), 
							pTMap, pJitBlkPtrVec, taskId))
			{
				delete graph;
				QMessageBox::warning (this, "CodeAnalyst symbols problem",
					"The module graph tab for " + module_name
					+ " is unavailable.");
				goto out;
			}

			// Add the Graph Tab
			addTabToNavigatorBar (graph, caption);
			m_pTabWidget->showPage ( graph );
			graph->setFocus();

			QObject::connect (graph,
				SIGNAL (symDblClicked (VADDR, SampleMap*, ProfileAttribute)),
				SLOT (onSymDblClicked (VADDR, SampleMap*, ProfileAttribute)));

			QObject::connect (graph,
				SIGNAL (viewModuleData (QString, TAB_TYPE)),
				SLOT (onModuleDblClicked (QString, TAB_TYPE)));

			connect (this, SIGNAL (shownDataChange(ViewShownData*)), graph, 
				SLOT (onViewChanged(ViewShownData*)));
			connect (graph, SIGNAL (shownDataChange(ViewShownData*)),
				this , SLOT (onShownColumnToggled(ViewShownData*)));

		} else {
			m_pTabWidget->showPage ( pOldTab );
			m_pTabWidget->currentPage()->setFocus();
		}
#endif
	}

out:
	bModProcessing = false;
	return;
} // CSessionNavigator::OnModuleDblClicked


bool CSessionNavigator::launchSourceView (VADDR address, 
					PID_T pid, TID_T tid,
					const CA_Module *pMod)
{
	bool bRet = true;
		
	// Look for existing source tab
	QString caption;
	if (pMod->getModType() != JAVAMODULE)
		caption.sprintf ("%s - Src/Dasm", wstrToQStr(pMod->getPath()).toAscii().data());
	else {
		AddrFunctionMultMap::const_reverse_iterator rit = pMod->getFunction(address);
		if (rit != pMod->getRendFunction())
			caption.sprintf ("%s : %s - Src/Dasm",
				wstrToQStr(rit->second.getJncFileName()).toAscii().data(),
				wstrToQStr(rit->second.getFuncName()).toAscii().data());
		else
			return false;		
	}

	SourceDataTab *pOldTab = (SourceDataTab*)findTab (caption);
	if (NULL != pOldTab) {
		// Check if the current source view contain the new address
		if (pOldTab->hasSymbolForAddress(address)) {
			emit newSrcHotspot (address, pid, tid);

			m_pTabWidget->showPage ( pOldTab );
			m_pTabWidget->currentPage()->setFocus();
			return bRet;
		}
	} 

	//------------------------------------------------------
	// Setting up new source tab
	SourceDataTab *src = new SourceDataTab (&m_viewShown, 
					m_pProfReader, pMod , 
					this, SRC_VIEW_WIDGET);
	RETURN_FALSE_IF_NULL (src, this);

	src->setCaption (caption);

	if (src->init(address)) {

		QObject::connect (this, SIGNAL (newSrcHotspot (VADDR, PID_T, TID_T)),
			src, SLOT (onNewHotSpot (VADDR, PID_T, TID_T)));

		QObject::connect (this, SIGNAL (densityVisibilityChanged()),
			src, SLOT (onDensityVisibilty()));

		connect (this, SIGNAL (shownDataChange(ViewShownData*)), src, 
			SLOT (onViewChanged(ViewShownData*)));
		connect (src, SIGNAL (shownDataChange(ViewShownData*)),
			this , SLOT (onShownColumnToggled(ViewShownData*)));

		addTabToNavigatorBar (src, caption);
		emit newSrcHotspot (address, pid, tid);

	} else {
		delete src;
		src = NULL;
		bRet = false;
	}

	return bRet;
} //CSessionNavigator::launchSourceView


//It would be better if we could just connect the  ApplicationWindow object
// to the dasm object, but that pointer wasn't passed.
void CSessionNavigator::onDensityVisibilty ()
{
	emit densityVisibilityChanged ();
}


void CSessionNavigator::onSymDblClicked (VADDR address,
					PID_T pid, TID_T tid,
					const CA_Module *pMod)
{
	launchSourceView (address, pid, tid, pMod);
}


void CSessionNavigator::addTabToNavigatorBar (QWidget * main_tab_widget,
					  QString tab_caption)
{
	// adds the view to the session navigator
	QWidget  *pOldTab = findTab (tab_caption);

	// Add the Data Tab if not already shown

	if (NULL == pOldTab)
	{
		// Add Data Tab
		QString trunc;
		if (tab_caption.length() > 50)
			trunc = QString("...") + tab_caption.right(50);	
		else
			trunc = tab_caption;
			
		m_pTabWidget->insertTab (main_tab_widget, trunc);
		m_pTabWidget->setTabToolTip (main_tab_widget, tab_caption);
		m_pTabWidget->showPage (main_tab_widget);
	} else {
		m_pTabWidget->showPage (pOldTab);
	}
}                               // CSessionNavigator::AddTabToNavigatorBar


//void CSessionNavigator::onViewSystemGraph ()
//{
//	m_pTabWidget->showPage ( findTab (SYS_GRAPH_STRING));
//}

void CSessionNavigator::onViewSystemTask ()
{
	m_pTabWidget->showPage ( findTab (SYS_TASK_STRING));
}

void CSessionNavigator::onViewSystemData()
{
	m_pTabWidget->showPage ( findTab (SYS_DATA_STRING));
}


void CSessionNavigator::onCurrentTabChanged (QWidget * widget)
{
	if (NULL == widget) 
	{
		QString temp;
		emit DataViewChanged( temp, NULL );
		return;
	}
	if (NULL == m_pCloseTab)
		return;
	DataTab *pDataTab = (DataTab *)widget;

	//Disable the close tab button if not on closable tab
	m_pCloseTab->setEnabled (pDataTab->CanUserClose());

	// Notify the Application to change menu options accordingly
	emit DataViewChanged( pDataTab->GetExportString(), 
		pDataTab->GetExportList() );
}


QWidget *CSessionNavigator::getCurrentView ()
{
	if (NULL != m_pTabWidget)
		return m_pTabWidget->currentPage ();
	else 
		return NULL;
}

//called when the close tab button (The x on the far upper right of the tab)
//	is pressed
void CSessionNavigator::OnCloseTab ()
{
	DataTab *pDataTab = (DataTab *)m_pTabWidget->currentPage();

	//Just in case it was enabled
	if (!pDataTab->CanUserClose())
		return;

	m_pTabWidget->removePage (pDataTab);

	// Suravee: removePage doesn't actually close the tab
	pDataTab->close();

	// [Suravee] QT does not delete for us. This must be done!!
	delete pDataTab;
}

void CSessionNavigator::OnTabMessageForStatus (const QString & message, int mS)
{
	Q_UNUSED (message);
	Q_UNUSED (mS);
}

// This slot handle signal from application.cpp for when modifying
// view configuration from global view management.
void CSessionNavigator::onGlobalShownChanged (QString viewName)
{
	unsigned long model, stepping;
	unsigned long family = m_pProfInfo->m_cpuFamily;
	model = m_pProfInfo->m_cpuModel;
	if (family == 0)
		getDefaultCpuFamilyModel(&family, &model, &stepping);

	// Re-read available view since it might be changed.
	m_views.updateView(viewName,family);

	onViewCfgChanged(m_pViewBox->currentText());
}


// Session specific view management
void CSessionNavigator::onViewCfgChanged (const QString & view)
{
	unsigned long model, stepping;
	unsigned long family = m_pProfInfo->m_cpuFamily;
	model = m_pProfInfo->m_cpuModel;
	if (family == 0)
		getDefaultCpuFamilyModel(&family, &model, &stepping);
	
	if (m_views.getViewConfig (view, &m_viewShown, 
				m_pProfInfo->m_numCpus, 
				family,model,	
				&m_normMap))
	{
		//if it hasn't been initialized yet, not needed.
		if ((NULL != m_pViewBox) && (NULL != m_pViewCfg))
		{
			m_pViewCfg->setEnabled (false);
			m_pViewBox->setEnabled (false);
			qApp->setOverrideCursor (Qt::WaitCursor);
		}

		//reload data for all tabs from file
		emit shownDataChange (&m_viewShown);

		if ((NULL != m_pViewBox) && (NULL != m_pViewCfg))
		{
			qApp->restoreOverrideCursor ();
			m_pViewBox->setEnabled (true);
			m_pViewCfg->setEnabled (true);
		}
	}
	
	updateViewButtons();
}

void CSessionNavigator::onShownColumnToggled(ViewShownData* viewShown)
{
	//reload data for all tabs from file
	emit shownDataChange (viewShown);
}


void CSessionNavigator::onViewMaybeTip (const QString & view)
{
	m_pViewBox->setToolTip("");
	QString newTip;

	if (m_views.getViewTexts (view, &newTip, NULL))
		m_pViewBox->setToolTip(newTip);
}


void CSessionNavigator::onConfigureView ()
{
	//create new ViewCfgDlg
	ViewCfgDlg *pViewCfgDlg = new ViewCfgDlg (this);
	RETURN_IF_NULL (pViewCfgDlg, this);

	//fill view data
	unsigned long model, stepping;
	unsigned long family = m_pProfInfo->m_cpuFamily;
	model = m_pProfInfo->m_cpuModel;
	if (family == 0)
		getDefaultCpuFamilyModel(&family, &model, &stepping);

	pViewCfgDlg->setViews (m_pViewBox->currentText(), family, model, false, &m_views);

	if (QDialog::Accepted == pViewCfgDlg->exec())
	{
		//reload viewshown from view
		onViewCfgChanged (m_pViewBox->currentText());
	}
}



/* 
* NOTE: (Suravee)
* Instead of using the norm from TBP file, we just calculate them from
* the session setting when viewing it.
*
* -----------------
* Formulas
* -----------------
*
*    active fraction [e]  = (number of occurences [e]) / (number of event groups [e])
*                   
*    norm factor [e]      = (sampling period [e]) / (active fraction [e])
*
*    norm samples [e]     = (norm factor [e]) * (number of samples [e])
*
*    DC access rate = norm samples          /  norm samples
*                                 DC access                ret inst
*
*/

void CSessionNavigator::calculateEventNorms (QString eventSessionName)
{
	if (eventSessionName.isEmpty())
		return ;

	EBP_OPTIONS opts;
	m_pProject->getSessionDetails (eventSessionName, &opts);
	
	/*
	* For each event 
	*/
	PerfEventList::iterator it     = opts.getEventContainer()->getPerfEventListBegin();
	PerfEventList::iterator it_end = opts.getEventContainer()->getPerfEventListEnd();

	for (; it != it_end ; it++) {

		EVMASK_T tempEvent;
		// NOTE: We do not concern the mask for IBS events 
		if (PerfEvent::isIbsEvent((*it).select()))
			tempEvent = EncodeEventMask((*it).select(), 0);
		else
			tempEvent = EncodeEventMask((*it).select(), (*it).umask());
		if (m_normMap.contains (tempEvent))
			continue;
		m_normMap[tempEvent] =  (*it).count;
	} // End for each event 

	return;
}


void CSessionNavigator::getDefaultCpuFamilyModel (unsigned long * cpuFamily, 
						unsigned long * model, 
						unsigned long * stepping) 
{
	char VenderId[15];
	CpuId(VenderId, cpuFamily, model, stepping);
}

//////////////////////////////////////////////////////
// DataTab Class
//////////////////////////////////////////////////////

//for each item in the list, update the text
/*
void DataTab::onShownChanged () 
{
	if (NULL == m_pList)
		return;

	//remove all columns
	clearShownColumns ();
	m_pList->setUpdatesEnabled (false);

	//update shown columns
	addShownColumns();

	QListViewItemIterator it( m_pList );
	while ( it.current() ) 
	{
		DataListItem *pDataItem = static_cast<DataListItem *>(it.current());
		pDataItem->updateShown();
		++it;
	}
	m_pList->setUpdatesEnabled (true);
}
*/

bool DataTab::isDisplayReady () 
{
	if (!m_displayMutex.tryLock()) {
		// queue event to try later
		QApplication::postEvent(this, new QEvent(QEvent::User));
		return false;
	}
	m_displayMutex.unlock();
	return true;
}


bool DataTab::clearShownColumns ()
{
	//remove all columns from the list view
	for (int col = m_pList->columns() - 1; col >= m_indexOffset; --col)
	{
		m_pList->removeColumn (col);
	}
	return true;
}

bool DataTab::initMenu ()
{
	if(m_pColumnMenu == NULL) 
	{
		m_pColumnMenu = new Q3PopupMenu (this);
		RETURN_FALSE_IF_NULL (m_pColumnMenu, this);

		//when the menu item is selected, the id of the column will be sent to 
		//	OnColumnToggled
		connect (m_pColumnMenu, SIGNAL (activated (int)), 
						SLOT (OnColumnToggled (int)));

	}

	m_pColumnMenu->setCheckable (true);

	m_pColumnIndex = new int[m_pViewShown->available.count()];
	RETURN_FALSE_IF_NULL (m_pColumnIndex, this);

	QStringList::Iterator it = m_pViewShown->available.begin();
	for (int i = 0; it != m_pViewShown->available.end(); ++it, ++i)
	{
		//save id of name in m_pColumnIndex[index]
		m_pColumnIndex[i] = m_pColumnMenu->insertItem( *it);
		//if index in m_pViewShown (ViewShownData::iterator itData)
		IndexVector::iterator itShown = m_pViewShown->shown.begin ();
		for (; itShown != m_pViewShown->shown.end(); ++itShown)
		{
			//if index is in m_pViewShown->shown
			if ((*itShown) == i)
			{
				//set the item to be checked.
				m_pColumnMenu->setItemChecked (m_pColumnIndex[i], true);
				break;
			}
		}
	}

	return addShownColumns();
} //DataTab::initMenu


//This will add columns to the list view for each column that shows data
bool DataTab::addShownColumns ()
{
	if (NULL == m_pList)
		return false;

	int column = m_indexOffset;
/*
	if (m_pViewShown->showPercentage)
	{
		m_pList->addColumn ("Total %", 60);
		m_pList->setColumnAlignment( column++ , AlignRight );
	}
*/
	IndexVector::iterator itShown = m_pViewShown->shown.begin ();

	for (; itShown != m_pViewShown->shown.end(); ++itShown, column++)
	{
		//add view columns
		m_pList->addColumn (m_pViewShown->available[*itShown], 100 );
		m_pList->setColumnAlignment( column, Qt::AlignRight );
	}

	if (NULL == m_pHeaderTip)
	{
		m_pHeaderTip = new HeaderTip (m_pList, m_pViewShown, m_indexOffset);
	}
	return true;
} //DataTab::addShownColumns


//This handles the right-click menu changing which columns are shown
void DataTab::OnColumnToggled (int id)
{
	int index = -1;
	//find index for menu id
	int i;
	for ( i = 0; i < (int) m_pViewShown->available.count(); i++)
	{
		if (m_pColumnIndex[i] == id)
		{
			index = i;
			break;
		}

	}
	//if not found,
	if (index < 0)
	{
		return;
	}

	//toggle the selected column
	bool notFound = true;
	IndexVector::iterator it = m_pViewShown->shown.begin ();
	for (; it != m_pViewShown->shown.end(); ++it)
	{
		//if index is in m_pViewShown, remove
		if ((*it) == index)
		{

			if(m_pViewShown->shown.size() == 1)
			{
				QMessageBox::critical (this, "CodeAnalyst Error","ERROR: Please select at least one event.\n");
				return;
			}


			m_pColumnMenu->setItemChecked (m_pColumnIndex[index], false);
			m_pViewShown->shown.erase (it); 
			//since it will be undefined...
			notFound = false;
			break;
		}
	}
	// if index was not in m_pViewShown, add
	if (notFound)
	{
		m_pColumnMenu->setItemChecked (m_pColumnIndex[index], true);
		m_pViewShown->shown.push_back (index);
	}

	emit shownDataChange(m_pViewShown);
} //DataTab::OnColumnToggled


void DataTab::onCopySelection()
{
	DataListItem *p_item = (DataListItem*) m_pList->firstChild ();
	QClipboard *clip = QApplication::clipboard ();

	if (NULL == clip) {
		return;
	}

	QString temp_str;

	int column_num = m_pList->columns ();
	int column_count;
	unsigned int *columns = NULL;

	columns = (unsigned int *) new unsigned int[column_num];
	if (NULL == columns) {
		QMessageBox::critical (this, NOMEM_HEADER, NOMEM_MSG);
		return;
	}
	memset (columns, 0, sizeof (unsigned int) * column_num);
	for (column_count = 0; column_count < column_num; column_count++) {
		temp_str = m_pList->columnText (column_count);
		if (columns[column_count] < temp_str.length () + 1)
			columns[column_count] = temp_str.length () + 1;
	}

	while (NULL != p_item) {
		if (p_item->isSelected ()) {
			for (column_count = 0; column_count < column_num; column_count++) {
				temp_str = p_item->text (column_count);
				if (columns[column_count] < temp_str.length () + 1)
					columns[column_count] = temp_str.length () + 1;
			}
		}

		p_item = (DataListItem*) p_item->itemBelow ();
	}

	QString text_str;
	QString header_str;
	for (column_count = 0; column_count < column_num; column_count++) {
		temp_str = m_pList->columnText (column_count);
		temp_str = temp_str.leftJustify (columns[column_count], ' ');
		header_str.append (temp_str);
		header_str.append ("\t");
	}
	header_str.append ("\n");

	p_item = (DataListItem*) m_pList->firstChild ();
	while (NULL != p_item) {
		if (p_item->isSelected ()) {
			for (column_count = 0; column_count < column_num; column_count++) {
				temp_str = p_item->text (column_count);
				temp_str = temp_str.leftJustify (columns[column_count], ' ');
				text_str.append (temp_str);
				text_str.append ("\t");
			}
			text_str.append ("\n");
		}
		
		p_item = (DataListItem*) p_item->itemBelow ();
	}
	
	if (!text_str.isEmpty ()) {
		clip->setText (header_str + text_str);
	}
	delete[]columns;

}


DataTab::DataTab( 
	ViewShownData *pViewShown, 
	QWidget* parent, 
	const char* name, 
	Qt::WindowFlags fl )
: Q3MainWindow (parent, name, 
	fl|
	Qt::CustomizeWindowHint |
	Qt::WindowSystemMenuHint |
	Qt::WindowMinMaxButtonsHint|
	Qt::SubWindow)
{
	m_exportString = "&Export data...";
	m_pList = NULL; 
	m_userCanClose = true;
	m_pViewShown = pViewShown;
	m_pColumnMenu = NULL;
	m_pColumnIndex = NULL;
	m_pHeaderTip = NULL;
	m_indexOffset = 0;

	setAttribute(Qt::WA_DeleteOnClose);

	CATuneOptions ao;
	ao.getPrecision ( &m_precision );
};


DataTab::~DataTab ()
{
	if (NULL != m_pHeaderTip) {
		delete m_pHeaderTip; 
		m_pHeaderTip = NULL;
	}

	if (NULL != m_pList) {
		delete m_pList;
		m_pList = NULL;
	}
};


void DataTab::keyPressEvent(QKeyEvent *e)
{
	Q_UNUSED(e);
}


//the tooltip has to be for the viewport of the listview, or it won't receive
// mouse events to call maybeTip
TabListTip::TabListTip (int tipColumn,  Q3ListView * pParent) 
: QWidget (pParent)
{
	m_tipColumn = tipColumn;
	m_pList = pParent;
	m_pList->viewport()->setMouseTracking (true);
}


//this is called when a mousemove event happens, in the viewport of the 
//	listview
//void TabListTip::maybeTip (const QPoint &pt)
bool TabListTip::event(QEvent *pEvent)
{
	if (pEvent->type() == QEvent::ToolTip) {  
		QHelpEvent *pHelpEvent = static_cast<QHelpEvent*>(pEvent);
		const QPoint &pt = pHelpEvent->pos();
		QPoint pt_local = m_pList->viewportToContents (pt );

		//If it is hovering over the notes column...
		if (m_tipColumn == m_pList->header()->sectionAt (pt_local.x())) 
		{
			//find the current list view item
			DataListItem *p_temp = (DataListItem *) m_pList->itemAt (pt);

			//If it's over an actual item...
			if (NULL != p_temp) 
			{
				QRect tempRect = m_pList->itemRect(p_temp);
				tempRect.setLeft (0);
				tempRect.setWidth (m_pList->width());
				//The tip shouldn't change while the cursor stays over the same item.
				QToolTip::showText (tempRect.topLeft(), p_temp->getTipString());
			}
		}
		return true;
	}
	return QWidget::event(pEvent);
}


//this static function will calculate a number, based on the predefined 
//	complex relationship.  Currently, only +, -, *, and \ are allowed between 
//	two basic values
//the complexcomponent defines the relationship and holds the indexes into the
//	data given.
//Note that to make the values more meaningful, the raw data has been 
//	normalized to a per-group/count value
float ViewShownData::setComplex (ComplexComponents *pComplex, DataArray *pData)
{
	float calc = 0;
	float normOp1 = (*pData)[pComplex->op1Index] * pComplex->op1NormValue;
	float normOp2 = (*pData)[pComplex->op2Index] * pComplex->op2NormValue;
	switch (pComplex->complexType) 
	{
	case ColumnRatio:
		if (0 != normOp2)
		{
			calc = normOp1 / normOp2;
		}
		break;
	case ColumnSum:
		calc = normOp1 + normOp2;
		break;
	case ColumnDifference:
		calc = normOp1 - normOp2;
		break;
	case ColumnProduct:
		calc = normOp1 * normOp2;
		break;
	default:
		break;
	}
	return calc;
}


HeaderTip::HeaderTip ( Q3ListView *pParent, ViewShownData *pViewShown,
					  int indexOffset) 
					  : QWidget(pParent)
{
	m_pList = pParent;
	m_pList->header()->setMouseTracking (true);
	m_pViewShown = pViewShown;
	m_indexOffset = indexOffset;
}

//void HeaderTip::maybeTip (const QPoint &pt)
bool HeaderTip::event (QEvent *pEvent)
{

	QHelpEvent *pHelpEvent = static_cast<QHelpEvent*>(pEvent);

	Q3Header *pHeader = m_pList->header();
	const QPoint &pt = pHelpEvent->pos(); 
	QPoint pt_local = m_pList->viewportToContents (pt );

	int section = pHeader->sectionAt( pt_local.x() );
	section -= m_indexOffset;
	if (m_pViewShown->showPercentage)
		--section;
	if (section >= 0)
	{
		IndexVector::iterator it = m_pViewShown->shown.begin(); ;
		for (int i = 0; i < section; ++i)
			++it;
		if (m_pViewShown->showPercentage)
			++section;
		
		// the iterator (*it) is used as an index, so we have to
		// check against the size of the ViewShownData::tips
		if (  (m_pViewShown->shown.end() != it)
		   && ((*it) < m_pViewShown->tips.size()) )
		{
			QToolTip::showText( pHeader->sectionRect( section + m_indexOffset ).topLeft(), 
					    m_pViewShown->tips[(*it)]);
		}
	}
}


