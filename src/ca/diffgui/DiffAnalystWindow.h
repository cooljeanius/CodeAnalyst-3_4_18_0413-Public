
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

#ifndef __DIFFANALYSTWINDOW_H_
#define __DIFFANALYSTWINDOW_H_

#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qmenubar.h>
#include <qtoolbutton.h>
#include <qlistview.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include <qsplitter.h>
#include <qvbox.h>
#include <qcombobox.h>
#include <qworkspace.h>

class ViewManageDlg;
class DasmDockView;
class PropDockView;
class NewSessionWizard;
class DiffSession;

///////////////////////////////////
// Menu ID's under the Tools Menu
enum {
    TM_FILENEW= 0,
    TM_FILEOPEN,
    TM_FILESAVE,
    TM_CAOPTIONS,
    TM_EDITPROFILES,
    TM_EDITVIEWS,
    TM_TOOLDASM,
    TM_TOOLPROP,
    TM_LAST
};



class DiffAnalystWindow : public QMainWindow
{
	Q_OBJECT
public:
	DiffAnalystWindow();

	~DiffAnalystWindow();

	bool init(QString dawFile);
	bool init(QString session0, 
			QString session1);
			
	bool diffSessionNameExists(QString name);

public slots:
	// This slot receive signal from DiffSession
	// and pass it to DasmDockView
	void onUpdateDasmDockView(QListViewItem* i)
	{ emit updateDasmDockView(i); };
	void onUpdatePropDockView()
	{ emit updatePropDockView(); };


protected:
	void closeEvent(QCloseEvent *e);
	bool initMenuBar();
	bool initViewToolBar();
	bool initDasmDockView();
	bool initPropDockView();
	bool initStatusBar();
	bool startCAProfDiffSession(QString session0 = "", 
				QString session1 = "",
				QString diffSessName = "");
	void setDiffAnalystCaption(const QString& name);
	QString getNextDiffSessionName();
	bool generateDAWFile(QString dawName, DiffSession *diffSession);
	bool readFile(QString fileName,
			QString *sess0,
			QString *sess1);

private slots:
	// Menu->File
        void onFileNew();
        void onFileOpen();
        void onFileSave();
        void onFileSaveAs();
        void onFileClose();
        void onExportData();
        void onMruItemSelected(int id);

	// Menu->Help
        void onHelpAbout();
        void onHelpContents();

	// Menu->Windows
        void toolsMenuAboutToShow();
        void onWindowsMenuActivated( int id );
        void onCascade();
        void onTile();
	void onWindowsMenuAboutToShow();

	// Menu->Views
        void onToggleDasmView();
        void onToggleProp();
        void onDasmDockViewNoShow();
        void onPropDockViewNoShow();
	void onUpdateProp();
	
	// Session Actions
/*
        void onSessionDeleted (TRIGGER trigger, QString SessionName);
        void onSessionDblClicked (TRIGGER trigger, QString SessionName);
        void onSessionRenamed (TRIGGER trigger, QString oldName, QString newName);
        void onDiffProfileSessionProperties( QString SessionName );
        void OnCopySessionToCurrent (TRIGGER trigger, QString SessionName);
*/
       
	// Menu->Tools
        void onViewManage ();

	// Helper Functions
        void onAboutToQuit();
        void onWindowActivated( QWidget *pWnd );
        void onCloseAllWindows(); 

private:
	int		m_ViewManageId;
	int		m_ExportDataId;
	int		m_SaveId;
	int		m_SaveAsId;
	int		m_CloseId;
	int		m_CloseAllId;
	int             m_toggleDensityId;
	QListView	*m_pTree;
	int		m_SamplingState;
	int		m_FileSepId;
	int		m_dasmToolId;
	int		m_propToolId;
	bool		m_isDasmToolShown;
	bool		m_isPropToolShown;
	bool		m_bDocumentOpen;
	bool		m_bHotKeysLoaded;
	bool		m_bFirstDialog;
	bool		m_bLegacyTimer;
	int		m_nDocNum;
	QLabel		*m_pStatusMsg;
//	ProgressMgr	*m_pProgress;
//	SESSION_OPTIONS *m_pSession;
//	TranslateDlg    *m_translate_progress_dlg;
	QProgressBar	*m_pStatusProgress;
	QSplitter	*m_pSplitter;
	QVBox		*m_pVBox;
	QListView	*m_pExportList;
//	MruList		m_MruList;
	unsigned int	m_tick_per_sec;
	unsigned int 	m_activeCounterMask; 
	uid_t 		m_euid;

	QComboBox 	*m_pProfileBox;

	// ViewToolBar
	QToolBar	*m_pFileTools;
	QToolBar	*m_pViewTools;
	QToolBar	*m_pToolTools;
	QComboBox	*m_pDiffSelect;
	QComboBox	*m_pViewCfgSelect;

	unsigned int m_novmlinux;
	QString m_vmlinux;
	unsigned int m_sample_buf_size;
	unsigned int m_note_buf_size;

//	PlugInManager * m_plugin_manager;

	QWorkspace	*m_pWs;

	// DockView
	DasmDockView	*m_pDasmDockView;
	PropDockView	*m_pPropDockView;
	
	// MenuBar	
	QPopupMenu	*m_pFileMenu;
	QPopupMenu	*m_pSettingsMenu;
	QPopupMenu	*m_pToolsMenu;
	QPopupMenu	*m_pViewMenu;
	QPopupMenu	*m_pWindowsMenu;
	QPopupMenu	*m_pHelpMenu;

	QToolButton 	* m_ToolsToolId[TM_LAST];
	QToolButton 	* m_SaveButt;
	int		m_ToolsMenuId[TM_LAST];

	NewSessionWizard *m_pNewSessWzd;
	ViewManageDlg	*m_pViewManageDlg;

signals:
	void dasmDockViewShowChanged(bool b);
	void propDockViewShowChanged(bool b);
	void updateDasmDockView(QListViewItem *i);
	void updatePropDockView();
};


#endif // __DIFFANALYSTWINDOW_H_

