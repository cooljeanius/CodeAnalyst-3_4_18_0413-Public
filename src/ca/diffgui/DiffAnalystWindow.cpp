
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

#include <qpopupmenu.h>
#include <qapplication.h>
#include <qstatusbar.h>
#include <qregexp.h>
#include <qpixmap.h>
#include <qiconset.h>
#include <qfile.h>
#include <qcheckbox.h>

#define _DIFFANALYST_ICONS_
#include "../gui/xpm.h"
#undef  _DIFFANALYST_ICONS_

#include "diffStruct.h"
#include "DiffAnalystWindow.h"
#include "DiffSession.h"
#include "ViewManageDlg.h"
#include "DasmDockView.h"
#include "PropDockView.h"
#include "NewSessionWizard.h"
#include "AboutDlg.h"
#include "helperAPI.h"


DiffAnalystWindow::DiffAnalystWindow() 
	: QMainWindow(0,"DiffAnalyst Window",WType_TopLevel | WStyle_SysMenu | WDestructiveClose)
{
	m_pWs = NULL;
	m_pSplitter = NULL;
	m_pVBox	= NULL;
	m_pNewSessWzd = NULL;
	m_pViewManageDlg = NULL;
	m_isDasmToolShown = true;
	m_isPropToolShown = true;

	setIcon(QPixmap(amdArrowLogo));
	setDockMenuEnabled(false);
	
}

DiffAnalystWindow::~DiffAnalystWindow()
{
	if(m_pNewSessWzd)
	{
		m_pNewSessWzd = NULL;
	}	

	if(m_pViewManageDlg)
	{
		m_pViewManageDlg = NULL;
	}	

}

void DiffAnalystWindow::onFileOpen()
{
	QString str;
	str = QDir::home ().path();
	str.replace ('\\', '/');

	QString dawName = QFileDialog::getOpenFileName( str, 
		"DAW File (*.daw)", this, "Open",  
		"Select File Name to Open");
	if(dawName.isEmpty())
		return;	
	init(dawName);
		
}

bool DiffAnalystWindow::readFile(QString fileName,
			QString *sess0,
			QString *sess1)
{
	bool ret = false;

	if(!sess0 || !sess1)
		return ret;

	QFile dawFile(fileName);
	if(!dawFile.open(IO_ReadOnly))
		return ret;

	QTextStream dawStream(&dawFile);
	while(!dawStream.eof())
	{
		QString line = dawStream.readLine();
		QString tmp = line.section("=",0,0);
		if(tmp == "SESSION_0")
			*sess0= line.section("=",-1);
		else if(tmp == "SESSION_1")
			*sess1= line.section("=",-1);
	}
	dawFile.close();
	ret = true;
	return ret;
}

bool DiffAnalystWindow::init(QString dawFile)
{
	QString sess0, sess1;
	if(!readFile(dawFile, &sess0, &sess1))
		return false;

	// GUI not yet setup
	if (!m_pSplitter)
		return init(sess0, sess1);

	startCAProfDiffSession(sess0, sess1,dawFile);
	DiffSession* curWin = (DiffSession*) m_pWs->activeWindow();
	if (curWin) {
		curWin->setDawName(dawFile);
	}
	return true;
}

bool DiffAnalystWindow::init(QString session0, 
			QString session1) 
{
	QString diffSessName;
	bool ret = false;
	// ////////////////////////////////////////////////////////////////
	// Instantiate the workspace
	// ////////////////////////////////////////////////////////////////

	m_pSplitter	= new QSplitter (this);
	if(m_pSplitter == NULL) return ret;
	m_pVBox		= new QVBox (m_pSplitter);
	if(m_pVBox == NULL) return ret;
	m_pWs		= new QWorkspace (m_pVBox);
	if(m_pWs == NULL) return ret;

	// When we cascade the sessions...
	m_pWs->setScrollBarsEnabled (true);
	m_pVBox->setFrameStyle (QFrame::StyledPanel | QFrame::Sunken);

	QObject::connect (m_pWs,
		SIGNAL (windowActivated (QWidget *)),
		SLOT (onWindowActivated (QWidget *)));

	setCentralWidget (m_pSplitter);

	// /////////////////////////////////////////////////////////////////////
	// Setup Initial Caption
	setDiffAnalystCaption(diffSessName);

	ret = true;
	ret = ret && initMenuBar();
	ret = ret && initViewToolBar();
	ret = ret && initDasmDockView();
	ret = ret && initPropDockView();
	
	// /////////////////////////////////////////////////////////////////////
	// Initialize Status Bar
	ret = ret && initStatusBar();

	// /////////////////////////////////////////////////////////////////////
	if( !session0.isEmpty() && !session1.isEmpty())
	{
		ret = ret && startCAProfDiffSession(session0, session1,diffSessName);
	}
	
	return ret;
}//init()


bool DiffAnalystWindow::initStatusBar()
{

	m_pStatusMsg = new QLabel (this,"Status Bar");
	if(m_pStatusMsg == NULL) return false;
	
	// stretch is used to compute a suitable size for widget as the status
	// bar grows and shrinks.
	int stretch = 4;
	statusBar()->addWidget (m_pStatusMsg, stretch);
	return true;
}

bool DiffAnalystWindow::initDasmDockView()
{
	bool ret=false;
	m_pDasmDockView = new DasmDockView(this, "Dasm View");
	if(m_pDasmDockView == NULL) return false;

	m_pDasmDockView->init();	

	connect(this, SIGNAL(dasmDockViewShowChanged(bool)),
		m_pDasmDockView, SLOT(onDasmDockViewShowChanged(bool)));

	connect(m_pDasmDockView, SIGNAL(dasmDockViewNoShow()),
		this, SLOT(onDasmDockViewNoShow()));

	// Connect QListView selection signal to DasmDockView
	connect(this, SIGNAL(updateDasmDockView(QListViewItem*)),
		m_pDasmDockView, SLOT(onUpdateDasmSymbol(QListViewItem*)));

	moveDockWindow (m_pDasmDockView, Qt::DockBottom);

	ret = true;
	return ret;

}

bool DiffAnalystWindow::initPropDockView()
{
	bool ret=false;
	m_pPropDockView = new PropDockView(this, "Property");
	if(m_pPropDockView == NULL) return false;

	m_pPropDockView->init();	

	connect(this, SIGNAL(propDockViewShowChanged(bool)),
		m_pPropDockView, SLOT(onPropDockViewShowChanged(bool)));

	connect(m_pPropDockView, SIGNAL(propDockViewNoShow()),
		this, SLOT(onPropDockViewNoShow()));
	
	connect(this, SIGNAL(updateDasmDockView(QListViewItem*)),
		this, SLOT(onUpdateProp()));

	moveDockWindow (m_pPropDockView, Qt::DockBottom);

	ret = true;
	return ret;
}

void DiffAnalystWindow::onUpdateProp()
{
	DiffSession* curWin = (DiffSession*) m_pWs->activeWindow();
	m_pPropDockView->onChangeSession(curWin);
}

bool DiffAnalystWindow::initViewToolBar()
{
	QIconSet icon;
	bool ret=false;
	
	// ////////////////////////////////////////////////////////////////////
	// FILE TOOLBAR
	m_pFileTools = new QToolBar (this, "File Toolbar");
	if(m_pFileTools == NULL) return false;

	icon = QIconSet (QPixmap(XpmFileNew));
	m_ToolsToolId [TM_FILENEW] =  new QToolButton( icon, "New Diff Session", QString::null, this,
		SLOT(onFileNew()), m_pFileTools);
	
	icon = QIconSet (QPixmap(XpmFileOpen));
	m_ToolsToolId [TM_FILEOPEN] =  new QToolButton( icon, "Open Diff Session", QString::null, this,
		SLOT(onFileOpen()), m_pFileTools);

	icon = QIconSet (QPixmap(XpmFileSave));
	m_ToolsToolId [TM_FILESAVE] =  new QToolButton( icon, "Save Diff Session", QString::null, this,
		SLOT(onFileSave()), m_pFileTools);
	m_ToolsToolId [TM_FILESAVE]->setEnabled(false);

	// ////////////////////////////////////////////////////////////////////
	// VIEW TOOLBAR
	m_pViewTools = new QToolBar (this, "View Toolbar");
	if(m_pViewTools == NULL) return false;

	icon = QIconSet (QPixmap(viewconfig));
	m_ToolsToolId [TM_EDITVIEWS] =  new QToolButton( icon, "View Management", QString::null, this,
		SLOT(onViewManage()), m_pViewTools);

	// ////////////////////////////////////////////////////////////////////
	// TOOLS TOOLBAR
	m_pToolTools = new QToolBar (this, "Tools Toolbar");
	if(m_pToolTools == NULL) return false;

	icon = QIconSet (QPixmap((const char**)disassemblyIcon));
	m_ToolsToolId [TM_TOOLDASM] =  new QToolButton( icon, "Show/Hide Disassembly", QString::null, this,
		SLOT(onToggleDasmView()), m_pToolTools);
	
	icon = QIconSet (QPixmap((const char**)propertyIcon));
	m_ToolsToolId [TM_TOOLPROP] =  new QToolButton( icon,"Show/Hide Property", QString::null, this,
		SLOT(onToggleProp()), m_pToolTools);

	// ////////////////////////////////////////////////////////////////////
	moveDockWindow (m_pViewTools, Qt::DockTop);
	ret = true;
	return ret;
}

bool DiffAnalystWindow::initMenuBar()
{
	bool ret=false;
	// ////////////////////////////////////////////////////////////////////
	// FILE
	m_pFileMenu = new QPopupMenu (this);
	if(m_pFileMenu == NULL) return false;
	menuBar ()->insertItem ("&File", m_pFileMenu);

	// New
	m_pFileMenu->insertItem (QIconSet (QPixmap (XpmFileNew)), "&New Session Wizard", this,
		SLOT (onFileNew ()), CTRL + Key_N);
	// Open
	m_pFileMenu->insertItem (QIconSet (QPixmap (XpmFileOpen)), "&Open",
		this, SLOT (onFileOpen ()), CTRL + Key_O);
	
	// -------------------
	m_pFileMenu->insertSeparator ();
	
	// Save
	m_SaveId = m_pFileMenu->insertItem (QIconSet (QPixmap (XpmFileSave)),
		"&Save", this, SLOT (onFileSave ()), CTRL + Key_S);
	m_pFileMenu->setItemEnabled (m_SaveId, false);

	// Save As
	m_SaveAsId = m_pFileMenu->insertItem (QIconSet(QPixmap()),
		"Save &As", this, SLOT (onFileSaveAs ()), CTRL + Key_A);
	m_pFileMenu->setItemEnabled (m_SaveAsId, false);

	// -------------------
//	m_pFileMenu->insertSeparator ();

	// export data
//	m_ExportDataId = m_pFileMenu->insertItem ("&Export...", this,
//		SLOT (onExportData ()));
//	m_pFileMenu->setItemEnabled (m_ExportDataId, false);

	// -------------------
	m_pFileMenu->insertSeparator ();

	// Close
	m_CloseId 	= m_pFileMenu->insertItem ("&Close", this, SLOT (onFileClose ()), CTRL + Key_W);
	m_CloseAllId 	= m_pFileMenu->insertItem ("Close Al&l", this, SLOT (onCloseAllWindows ()));

	// Quit
	m_pFileMenu->insertItem ("&Quit", this, SLOT (close ()), CTRL + Key_Q);
	QObject::connect (qApp, SIGNAL (aboutToQuit ()), this,
		SLOT (onAboutToQuit ()));
	
	// ////////////////////////////////////////////////////////////////////
	// SETTINGS
//	m_pSettingsMenu = new QPopupMenu (this);
//	if(m_pSettingsMenu == NULL) return false;
//	menuBar ()->insertItem ("&Settings", m_pSettingsMenu);

	// /////////////////////////////////////////////////////////////////////
	// TOOLS
	m_pToolsMenu = new QPopupMenu (this);
	if(m_pToolsMenu == NULL) return false;

	m_pToolsMenu->setCheckable (TRUE);
        connect (m_pToolsMenu, SIGNAL (aboutToShow ()), this,
                SLOT (toolsMenuAboutToShow ()));

	menuBar ()->insertItem ("&Tools", m_pToolsMenu);

	// /////////////////////////////////////////////////////////////////////
	// VIEW 
	m_pViewMenu = new QPopupMenu (this);
	if(m_pViewMenu == NULL) return false;
	menuBar ()->insertItem ("&View", m_pViewMenu);

	// View Management
	m_ViewManageId = m_pViewMenu->insertItem (QPixmap(viewconfig),"&View Management", this,
		SLOT (onViewManage ()));

	// /////////////////////////////////////////////////////////////////////
	// WINDOWS
	m_pWindowsMenu = new QPopupMenu (this);
	if(m_pWindowsMenu == NULL) return false;
        connect (m_pWindowsMenu, SIGNAL (aboutToShow ()), this,
                SLOT (onWindowsMenuAboutToShow ()));
	
	menuBar ()->insertItem ("&Windows", m_pWindowsMenu);

	// /////////////////////////////////////////////////////////////////////
	// HELPS
	m_pHelpMenu = new QPopupMenu (this);
	if(m_pHelpMenu == NULL) return false;
	menuBar ()->insertItem ("&Help", m_pHelpMenu);

	m_pHelpMenu->insertItem ("&About", this, SLOT (onHelpAbout ()), Key_F1);
	m_pHelpMenu->insertItem ("&Help", this, SLOT (onHelpContents ()));
	
	ret = true;
	return ret;
}// DiffAnalystWindow::initMenuBar

void DiffAnalystWindow::onFileNew()
{

	if(m_pNewSessWzd)
	{
		delete m_pNewSessWzd;
		m_pNewSessWzd = NULL;
	}

	m_pNewSessWzd = new NewSessionWizard(this, getNextDiffSessionName());
	if(m_pNewSessWzd == NULL) return;

	m_pNewSessWzd->init();
	if (QDialog::Rejected == m_pNewSessWzd->exec())
                return;

	startCAProfDiffSession();	
}

void DiffAnalystWindow::onFileSave()
{
	DiffSession* curWin = (DiffSession*) m_pWs->activeWindow();
	if (curWin)
	{
		QString dawName = curWin->getDawName();
		if(!dawName.isEmpty())
		{
			generateDAWFile(dawName,curWin);
		}else{
			onFileSaveAs();
		}
	}
}

void DiffAnalystWindow::onFileSaveAs()
{

	DiffSession* curWin = (DiffSession*) m_pWs->activeWindow();
	if (curWin)
	{

		QString str;
		str = QDir::home ().path();
		str.replace ('\\', '/');
		str = str + "/" +curWin->name();

		QString dawName = QFileDialog::getSaveFileName( str, 
			"DAW File (*.daw)", this, "SaveAs",  
			"Select File Name to Save");

		// Check if file name is empty
		if(dawName.isEmpty())
			return;	

		if(!dawName.endsWith(".daw"))
		{
			dawName = dawName + ".daw";
		}

		// Check if file already exists
		if(QFile::exists(dawName))
		{
			if(QMessageBox::warning(this,"DiffAnalyst Warning",
				QString("WARNING:\n\nFile \"")
				+ dawName + "\" already exists.\n\n"
				+ "Do you want to overwrite?\n"	,
				QMessageBox::Yes,QMessageBox::No) == QMessageBox::No)
				return;	
		}

		generateDAWFile(dawName,curWin);
		curWin->setDawName(dawName);
		curWin->setCaption(dawName);
	}
}

bool DiffAnalystWindow::generateDAWFile(QString dawName, 
				DiffSession *diffSession)
{
	bool ret = false;
	
	if(!dawName.endsWith(".daw"))
	{
		dawName = dawName + ".daw";
	}

	QFile dawFile(dawName);
	if(!dawFile.open(IO_ReadWrite|IO_Truncate))
	{
		return ret;	
	}
	
	//Start output stream
	QTextStream dawFStream(&dawFile);
	
	//Write File
	for(int i = 0 ; i < diffSession->getNumDiffSession() ; i++)
	{
		dawFStream << "SESSION_" << i << "=" << diffSession->getSessionStr(i) + "\n" ;
	}
	dawFStream << "\n" ;

	//Close File
	dawFile.close();

	ret = true;
	return ret;
}

void DiffAnalystWindow::onFileClose()
{
	QWidget * curWin = m_pWs->activeWindow();
	if (curWin)
	{
		curWin->close(true);
	}
}

void DiffAnalystWindow::onExportData()
{
}

void DiffAnalystWindow::onMruItemSelected(int id){Q_UNUSED(id);}

void DiffAnalystWindow::onHelpAbout()
{
        AboutDlg *p_dlg = new AboutDlg (this, "About AMD DiffAnalyst", TRUE);

        if (NULL != p_dlg)
        {
                p_dlg->show ();
        } else
		QMessageBox::critical (this, "Error", "Could not allocate memory for the dialog.");
}

void DiffAnalystWindow::onHelpContents()
{
	QString invoke = getDefaultPdfReader();

	if (invoke.isEmpty() || !QFile::exists(invoke)) {
		QMessageBox::critical (this, "Error", 
			"Could not locate PDF Reader.");
		return;
	}

	invoke += QString(" ") + CA_DATA_DIR "/doc/dadoc/DiffAnalyst_linux_users_guide.pdf";

	qApp->setOverrideCursor (QCursor (Qt::WaitCursor));
	system (invoke.toAscii().data());
	qApp->restoreOverrideCursor ();
}

void DiffAnalystWindow::toolsMenuAboutToShow()
{
	m_pToolsMenu->clear ();

	m_dasmToolId = m_pToolsMenu->insertItem ("Show &Dasm Diff View", 
				this, SLOT (onToggleDasmView ()));
	m_pToolsMenu->setItemChecked (m_dasmToolId, m_isDasmToolShown);
	
	m_propToolId = m_pToolsMenu->insertItem ("Show &Property View", 
				this, SLOT (onToggleProp ()));
	m_pToolsMenu->setItemChecked (m_propToolId, m_isPropToolShown);
}

void DiffAnalystWindow::onDasmDockViewNoShow()
{
	m_isDasmToolShown = false;
}

void DiffAnalystWindow::onPropDockViewNoShow()
{
	m_isPropToolShown = false;
}

void DiffAnalystWindow::onToggleDasmView()
{
	QString toggle_text;
/*
	bool showDasmTool = true;
	//Toggle here
	if (m_pToolsMenu->isItemChecked (m_dasmToolId))
		showDasmTool = true;		
	else
		showDasmTool = false;		

	m_isDasmToolShown = !showDasmTool;
*/
	m_isDasmToolShown = !m_isDasmToolShown;
        m_pToolsMenu->setItemChecked (m_dasmToolId,m_isDasmToolShown);
	emit dasmDockViewShowChanged (m_isDasmToolShown);
}

void DiffAnalystWindow::onToggleProp()
{
	QString toggle_text;
	
	m_isPropToolShown = !m_isPropToolShown;
        m_pToolsMenu->setItemChecked (m_propToolId,m_isPropToolShown);
	emit propDockViewShowChanged (m_isPropToolShown);
}

void DiffAnalystWindow::onViewManage()
{
	// Get active window	
	DiffSession* curWin = (DiffSession*) m_pWs->activeWindow();

	// New View Manage Dialog
	m_pViewManageDlg = new ViewManageDlg(this,curWin);
	if(m_pViewManageDlg == NULL) return;

	if(curWin != NULL)
	{
		m_pViewManageDlg->setViews (QString::null,0x10,2);
		m_pViewManageDlg->setTitle(curWin->name());
	
		m_pViewManageDlg->setCurrentTab(SESSION_VIEW_OPTION_TAB);
		
		// Show
		if (QDialog::Rejected == m_pViewManageDlg->exec())
			return;

		// Get the current configuration
		curWin->populateDataToSymbolView();

	}else{
		m_pViewManageDlg->setViews (QString::null,0x10,2);
		m_pViewManageDlg->setCurrentTab(VIEWCFG_MANAGE_TAB);
		m_pViewManageDlg->setTabEnabled(SESSION_VIEW_OPTION_TAB, false);
		
		// Show
		if (QDialog::Rejected == m_pViewManageDlg->exec())
			return;

	}

	// Clear DamsDockView
	emit updateDasmDockView(NULL);
}

void DiffAnalystWindow::onWindowActivated(QWidget* s)
{
	DiffSession* curWin = (DiffSession*) s;
	if (curWin)
	{
		QString dawName = curWin->getDawName();
	}else{
		m_pFileMenu->setItemEnabled (m_SaveId, false);
		m_pFileMenu->setItemEnabled (m_SaveAsId, false);
		m_ToolsToolId [TM_FILESAVE]->setEnabled(false);
	}
	m_pDasmDockView->onChangeSession((DiffSession*)s);
	m_pPropDockView->onChangeSession((DiffSession*)s);
}

bool DiffAnalystWindow::startCAProfDiffSession(QString session0, 
				QString session1, 
				QString diffSessName)
{
	SESSION_DIFF_INFO_VEC sessionInfoVec;
	QFileInfo fInfo;

	if(!session0.isEmpty() && !session1.isEmpty())
	{
		sessionInfoVec.clear();
		SESSION_DIFF_INFO info0,info1;
		bool isOk = false;

		while(1)
		{
			info0.sessionFile 	= session0.section(":",0,0);
			fInfo.setFile(info0.sessionFile);
			if(!fInfo.exists()) break;
		
			info0.task		= session0.section(":",1,1);
			if(!info0.task.isEmpty()
			&& info0.task.compare("All Tasks"))
			{
				fInfo.setFile(info0.task);
				if(!fInfo.exists()) break;
			}

			info0.module		= session0.section(":",2,2);
			fInfo.setFile(info0.module);
			if(!fInfo.exists()) break; 

			sessionInfoVec.push_back(info0);

			info1.sessionFile 	= session1.section(":",0,0);
			fInfo.setFile(info1.sessionFile);
			if(!fInfo.exists()) break;
		
			info1.task		= session1.section(":",1,1);
			if(!info1.task.isEmpty()
			&& info1.task.compare("All Tasks"))
			{
				fInfo.setFile(info1.task);
				if(!fInfo.exists()) break;
			}
		
			info1.module		= session1.section(":",2,2);
			fInfo.setFile(info1.module);
			if(!fInfo.exists()) break;

			isOk = true;
			break;
		}
	
		if(!isOk)
		{
			QMessageBox::critical(this,"DiffAnalyst Error",QString() +
				"Path not found: " + fInfo.absFilePath() + "\n\n" +
				"Please check command-line options, or use New Session Wizard.\n" );
			return false;
		}
	
		sessionInfoVec.push_back(info1);

		if(diffSessName.isEmpty())	
			diffSessName = "Untitle-1";
	}else{
		int sessionCnt = 0;

		/////////////////////////////////////////
		// Query NewSessionWizard
		if(m_pNewSessWzd == NULL)
			return false;
		
		if((sessionCnt = m_pNewSessWzd->getSessionDiffInfo(&sessionInfoVec)) <= 0)
		{
			return false;
		}


		/////////////////////////////////////////
		// Setup DiffAnalystWindowName
		diffSessName = m_pNewSessWzd->getSessionDiffName();

	}

	/////////////////////////////////////////
	// Setup DiffSession
	DiffSession *diffSess = new DiffSession(m_pWs,diffSessName.toAscii().data());
	if(diffSess == NULL) return false;
	if(!diffSess->init(sessionInfoVec)) return false;

	diffSess->showMaximized();
	
	m_pFileMenu->setItemEnabled (m_SaveId, true);
	m_pFileMenu->setItemEnabled (m_SaveAsId, true);
	m_ToolsToolId [TM_FILESAVE]->setEnabled(true);

	return true;

} //DiffAnalystWindow::startCAProfDiffSession


///////////////////////////////////////////////////////////
// Quit Application
///////////////////////////////////////////////////////////
void DiffAnalystWindow::closeEvent(QCloseEvent *e)
{
	e->accept();
}

void DiffAnalystWindow::onAboutToQuit()
{
        onCloseAllWindows ();
}

void DiffAnalystWindow::onCloseAllWindows()
{
        // Close all open windows
        QWidgetList windows = m_pWs->windowList ();

        for (int i = 0; i < int (windows.count ()); ++i)
        {
                windows.at (i)->close ();
        }
}

void DiffAnalystWindow::setDiffAnalystCaption(const QString& name)
{
	QString tmp = QString("AMD - DiffAnalyst") + name;
	QWidget::setCaption(tmp);
}

bool DiffAnalystWindow::diffSessionNameExists(QString name)
{
	bool ret = false;
	QStringList nameList;
	QWidgetList wl = m_pWs->windowList(QWorkspace::CreationOrder);
	QWidgetList::iterator it = wl.begin();
	QWidgetList::iterator end = wl.end();
	for( ; it != end ; it++)
	{
		nameList.push_back( (*it)->name() );
	}
		
	QString pattern = QString("^")+ name + "$";
	QRegExp regExp(pattern);
	if(nameList.grep(regExp).size() != 0)
		ret = true;
	return ret;
}
		

QString DiffAnalystWindow::getNextDiffSessionName()
{
	QString defaultName = "Untitled-";
	int counter = 1;
	QStringList nameList;
	QString ret = "";

	QWidgetList wl = m_pWs->windowList(QWorkspace::CreationOrder);
	QWidgetList::iterator it = wl.begin();
	QWidgetList::iterator end = wl.end();
	for( ; it != end ; it++)
	{
		nameList.push_back( (*it)->name() );
	}

	while(1)
	{
		QString pattern = QString("^")+ defaultName + QString::number(counter) + "$";
		QRegExp regExp(pattern);
		if(nameList.grep(regExp).size() == 0)
		{
			ret = defaultName + QString::number(counter);
			break;
		}else{
			counter++;
		}
	}
	return ret;	
}

void DiffAnalystWindow::onWindowsMenuAboutToShow()
{
	m_pWindowsMenu->clear ();

	int cascadeId = m_pWindowsMenu->insertItem ("&Cascade", this, SLOT (onCascade ()));
	int tileId    = m_pWindowsMenu->insertItem ("&Tile", this, SLOT (onTile ()));
	int close_all_id = m_pWindowsMenu->insertItem ("Close Al&l", this, SLOT (onCloseAllWindows ()));
	if (m_pWs->windowList ().isEmpty ())
	{
		m_pWindowsMenu->setItemEnabled (cascadeId, FALSE);
		m_pWindowsMenu->setItemEnabled (tileId, FALSE);
		m_pWindowsMenu->setItemEnabled (close_all_id, FALSE);
	}

	m_pWindowsMenu->insertSeparator ();

	QWidgetList windows = m_pWs->windowList ();

	for (int i = 0; i < int (windows.count ()); ++i)
	{
		int id = m_pWindowsMenu->insertItem (windows.at (i)->caption (),
				this,
				SLOT (onWindowsMenuActivated (int)));

		m_pWindowsMenu->setItemParameter (id, i);
		m_pWindowsMenu->setItemChecked (id,
		m_pWs->activeWindow () == windows.at (i));
	}
}

//===========================================================================
//@mfunc This is a custom function because the qt version of cascade() doesn't
//      work right.

//@comm Got the offset values from Qt::QWorkspace.cpp file

//===========================================================================
void DiffAnalystWindow::onCascade()
{
	// from QWorkspace.cpp
	const int xoffset = 13;
	const int yoffset = 23;
	const int width = 640;
	const int height = 480;
	int x = 0;
	int y = 0;

	m_pWs->cascade ();
	int numWnd = m_pWs->windowList ().count ();
	
	for (int i = 0; i < numWnd; ++i)
	{
		QWidget *pWnd = m_pWs->windowList ().at (i);

		if (!pWnd->isHidden ())
		{
			pWnd->showNormal ();
			pWnd->parentWidget ()->setGeometry (x, y, width, height);
			if ((y + yoffset + height) <= m_pWs->height ())
			{
				x += xoffset;
				y += yoffset;
			}
			else
			{
				x = 0;
				y = 0;
			}
		}
	}
}

void DiffAnalystWindow::onTile()
{
	m_pWs->tile();	
}

void DiffAnalystWindow::onWindowsMenuActivated(int id)
{
	QWidget *w = m_pWs->windowList ().at (id);
	if (w)
	{
		w->show ();
		w->setFocus ();

	}
}

