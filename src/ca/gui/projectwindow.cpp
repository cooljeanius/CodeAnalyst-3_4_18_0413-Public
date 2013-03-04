//$Id: projectwindow.cpp,v 1.3 2005/02/14 16:52:29 jyeh Exp $
// implementation for the ProjectWindow class.

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

#include <qtooltip.h>
#include <QString>
#include <QResizeEvent>
#include <Q3PopupMenu>
#include <QKeyEvent>
#include "projectwindow.h"

ProjectWindow::ProjectWindow ( QWidget * pParentWnd ) 
: QWidget ( pParentWnd, "ProjectWindow" )
{
	m_current_item = NULL;
	m_tbp = NULL;
	m_ebp = NULL;
	m_tree = NULL;
	m_current_item  = NULL;	

	// Initialize the Tree View with Items
	m_tree = new Q3ListView( this );
	m_tree->setVScrollBarMode ( Q3ScrollView::AlwaysOn  );
	m_tree->setSorting (-1);
	m_tree->addColumn ( NO_PROJECTS_OPENED );
        m_tree->setResizeMode(Q3ListView::AllColumns);
	m_tree->setRootIsDecorated ( TRUE );

	m_tabListTip = new ProjWndListTip(m_tree);

	// Connect the signals to slots
	QObject::connect (m_tree, SIGNAL (doubleClicked (Q3ListViewItem *) ),
		SLOT ( onDblClicked (Q3ListViewItem *) ));
	QObject::connect (m_tree, SIGNAL (selectionChanged (Q3ListViewItem *)),
		SLOT ( onSelChange (Q3ListViewItem *) ));
	QObject::connect (m_tree, SIGNAL (expanded (Q3ListViewItem *) ),
		SLOT ( onExpanded  (Q3ListViewItem *) ));
	QObject::connect(m_tree, SIGNAL (rightButtonClicked (Q3ListViewItem *, const QPoint &, int)),
		SLOT (onRightButtonClicked (Q3ListViewItem *, const QPoint &, int)) );
	QObject::connect(m_tree, SIGNAL (contextMenuRequested (Q3ListViewItem *, const QPoint &, int)),
		SLOT (onRightButtonClicked (Q3ListViewItem *, const QPoint &, int)));

	m_menu = new Q3PopupMenu ( m_tree );

	if (NULL != m_menu) {
		m_menu->insertItem ("&Properties", this, 
			SLOT (onPropertiesSession()));
		m_menu->insertItem( "&Copy configuration to current", this, 
			SLOT (OnCopyToCurrent()) );
		m_menu->insertItem ("&Rename", this, SLOT (onRenameSession()) );
// FIXME [3.1] : Postpone for now
//		m_menu->insertItem ("Create D&iff Session", this, 
//			SLOT (onOpenSessionInDiffAnalyst()));
		m_menu->insertItem ("View CSS in kcachegrind", this, 
			SLOT (onOpenKcachegrind()));
		m_menu->insertSeparator();
		m_menu->insertItem ("&Delete", this, 
			SLOT (deleteCurrentSessionItem()));
	}

	m_menu_parent = new Q3PopupMenu ( m_tree );

	if (NULL != m_menu_parent) {
	    m_menu_parent->insertItem ("&Delete all Sessions", this, 
				SLOT (deleteAllSessionItem()));
	}
} //ProjectWindow::ProjectWindow


ProjectWindow::~ProjectWindow()
{
	if (NULL != m_menu)
		delete m_menu;
}


void ProjectWindow::onExpanded ( Q3ListViewItem * item )
{
	if ((TBP_SESSION_STR == item->text (0)) || (PERF_TBP_SESSION_STR == item->text (0)))   {
		QString TipString;
		QRect	 ItemRect;
		ProjWndListItem * p;

		for (p = (ProjWndListItem *) item->firstChild() ;
			NULL != p;
			p = (ProjWndListItem *) p->nextSibling()) {
			m_tree->ensureItemVisible ( p );
			ItemRect = m_tree->itemRect ( p );

			TipString.sprintf ( "%s", item->text (0).data() ); 
		}
	}
}


// Delete all items in the list view
//
void ProjectWindow::deleteAllItems()
{
	m_tree->clear();
	m_tree->setColumnText ( 0, NO_PROJECTS_OPENED );
}


// Set the title of the list view
//
void ProjectWindow::setCaption ( QString str )
{
	m_tree->setColumnText (0, str);
}


void ProjectWindow::onDblClicked ( Q3ListViewItem * item )
{
	if ( NULL == item ) return;
	if ( NULL ==  item->parent() ) return;

	if( item->parent()->text(0) == TBP_SESSION_STR )
	{
		emit SessionDblClicked (TIMER_TRIGGER, item->text(0) );
	} 
	else if( item->parent()->text(0) == PERF_TBP_SESSION_STR )
	{
		emit SessionDblClicked (PERF_TIMER_TRIGGER, item->text(0) );
	} 
	else if( item->parent()->text(0) == EBP_SESSION_STR )
	{
		emit SessionDblClicked (EVENT_TRIGGER, item->text(0) );
	} 
}


// Called when the selection has changed in the tree view
// We want to emit a signal with the sessions name to the app window
//
void ProjectWindow::onSelChange ( Q3ListViewItem * item )
{
	if ( NULL == item->parent() ) return;

	// Track the current item
	m_current_item = (ProjWndListItem*) item;

	if ( (TBP_SESSION_STR == item->parent()->text (0)) || (PERF_TBP_SESSION_STR == item->parent()->text (0) ))  {
		emit newSessionSelected ( item->text (0) );
	}
}


void ProjectWindow::resizeEvent ( QResizeEvent * e )
{
	Q_UNUSED (e);
	m_tree->resize ( width(), parentWidget()->height() );
}


void ProjectWindow::keyReleaseEvent ( QKeyEvent * e )
{
	if ( Qt::Key_Delete == e->key() ) {
		deleteCurrentSessionItem();
	}
}


void ProjectWindow::insertTbpSamplingSession ( QString session_name, QString session_note )
{	
	static ProjWndListItem * last_item = NULL;
	ProjWndListItem *item;
	if (NULL != last_item)
		item = new ProjWndListItem( m_tbp, last_item, session_name );
	else
		item = new ProjWndListItem( m_tbp, session_name );
	
	item->setTip(session_note);

	last_item = item;
}

void ProjectWindow::insertPerfTbpSamplingSession ( QString session_name, QString session_note )
{	
	static ProjWndListItem * last_item = NULL;
	ProjWndListItem *item;
	if (NULL != last_item)
		item = new ProjWndListItem( m_perftbp, last_item, session_name );
	else
		item = new ProjWndListItem( m_perftbp, session_name );
	
	item->setTip(session_note);

	last_item = item;
}


void ProjectWindow::insertEbpSamplingSession ( QString session_name, QString session_note )
{	
	static ProjWndListItem* last_item = NULL;
	ProjWndListItem *item;
	if (NULL != last_item)
		item = new ProjWndListItem( m_ebp, last_item, session_name );
	else
		item = new ProjWndListItem( m_ebp, session_name );
	
	item->setTip(session_note);

	last_item = item;
}


void ProjectWindow::initialize()
{

	/* 
	* NOTE: Put the initialization in this order so that TBP is on top 
	*/
	m_ebp = new ProjWndListItem( m_tree, EBP_SESSION_STR );
	m_tbp = new ProjWndListItem( m_tree, TBP_SESSION_STR );
#ifdef CAPERF_SUPPORT_ENABLED
	m_perftbp = new ProjWndListItem( m_tree, PERF_TBP_SESSION_STR );
#endif //CAPERF_SUPPORT_ENABLED
}


void ProjectWindow::expandTbpPerf()
{
	m_perftbp->setOpen ( TRUE );
}

void ProjectWindow::expandTbp()
{
	m_tbp->setOpen ( TRUE );
}


void ProjectWindow::expandEbp()
{
	m_ebp->setOpen ( TRUE );
}

//Note that this seems to have an issue on items in 64-bit Linux
//
void ProjectWindow::onRightButtonClicked ( Q3ListViewItem * item, 
					  const QPoint &pt, int col )
{
	Q_UNUSED (col);
	// ONly show the menu for children, not parent itmes
	if ( NULL == item ) return;
	if ( NULL == item->parent() ) 
	{
		m_menu_parent->popup(pt);
		return;
	}

	m_menu->popup ( pt );
}


void ProjectWindow::onPropertiesSession()
{
	if (NULL == m_current_item) return;

	QString current_name = m_current_item->text (0);
	if (TBP_SESSION_STR == m_current_item->parent()->text (0)) {
		emit tbpSessionProperties ( current_name );
	} else if ( PERF_TBP_SESSION_STR == m_current_item->parent()->text (0) ) {
		emit perftbpSessionProperties ( current_name );
	} else if ( EBP_SESSION_STR == m_current_item->parent()->text (0) ) {
		emit ebpSessionProperties ( current_name );
	}
}


void ProjectWindow::OnCopyToCurrent()
{
	ProjWndListItem * item = (ProjWndListItem *) m_tree->selectedItem();
	if (item == NULL)
		return;

	QString CurrentName = item->text(0);
	QString type = item->parent()->text(0);
	if( TBP_SESSION_STR == type)
	{
		emit CopySessionToCurrent (TIMER_TRIGGER, CurrentName );
	} 
	else if (PERF_TBP_SESSION_STR == type )
	{
		emit CopySessionToCurrent (PERF_TIMER_TRIGGER, CurrentName );
	}
	else if (EBP_SESSION_STR == type )
	{
		emit CopySessionToCurrent (EVENT_TRIGGER, CurrentName );
	}
}

void ProjectWindow::onRenameSession()
{
	ProjWndListItem * item = (ProjWndListItem *) m_tree->selectedItem();
	if (item == NULL)
		return;
	QString CurrentName = item->text(0);

	// Singal the name change
	if( item->parent()->text(0) == TBP_SESSION_STR ) {
		emit SessionRenamed (TIMER_TRIGGER, CurrentName);
	} else if( item->parent()->text(0) == PERF_TBP_SESSION_STR ) {
		emit SessionRenamed (PERF_TIMER_TRIGGER, CurrentName);
	} else if( item->parent()->text(0) == EBP_SESSION_STR ) {
		emit SessionRenamed (EVENT_TRIGGER, CurrentName);
	}
}

void ProjectWindow::deleteCurrentSessionItem()
{
	ProjWndListItem *pItem = (ProjWndListItem *) m_tree->selectedItem();

	//Don't delete a null selection or any of the headers!
	if( (NULL != pItem) && (pItem->parent() != NULL))
	{
		// NOTE: 
		// We need to capture this before poping the dialog
		// since the current item could have been changed 
		// during the time when the message is shown.
		QString key = pItem->key(0, TRUE);
		QString parent = pItem->parent()->text(0);

		int answer = QMessageBox::information( this, "Delete?", 
			"Delete session '" + pItem->text(0) + "'?",
			QMessageBox::Yes, QMessageBox::No );

		if( QMessageBox::Yes == answer  )
		{
			TRIGGER trigger = NO_TRIGGER;
			if( parent == TBP_SESSION_STR ) {
				trigger = TIMER_TRIGGER;
			} else if( parent == PERF_TBP_SESSION_STR ) {
				trigger = PERF_TIMER_TRIGGER;
			} else if( parent == EBP_SESSION_STR ) {
				trigger = EVENT_TRIGGER;
			}
			emit SessionDeletedAndReinit (trigger, key);
		}
    }
}

void ProjectWindow::deleteAllSessionItem()
{
	ProjWndListItem *pItem = (ProjWndListItem *) m_tree->selectedItem();
	if(!pItem) return;
	
	ProjWndListItem *pChild = (ProjWndListItem *) pItem->firstChild();
	if(!pChild) return;

	// NOTE: 
	// We need to capture this before poping the dialog
	// since the current item could have been changed 
	// during the time when the message is shown.
	QString parent = pItem->text(0);

	int answer = QMessageBox::information( this, "Delete All Sessions?", 
		       "Delete all sessions in '" + pItem->text(0) + "'?",
		       QMessageBox::Yes, QMessageBox::No );

	if( QMessageBox::Yes == answer  )
	{
		// NOTE: 
		// Current item could have changed. 
		// We need to capture the first child here
		pChild = (ProjWndListItem *) m_tree->findItem(parent,0)->firstChild();

		TRIGGER trigger = TIMER_TRIGGER;
		if( parent == TBP_SESSION_STR ) {
			trigger = TIMER_TRIGGER;
		} else if( parent == PERF_TBP_SESSION_STR ) {
			trigger = PERF_TIMER_TRIGGER;
		} else if( parent == EBP_SESSION_STR ) {
			trigger = EVENT_TRIGGER;
		}

		ProjWndListItem *pNextChild = NULL;
		do {
			pNextChild = (ProjWndListItem *) pChild->nextSibling();
			emit SessionDeletedOnly (trigger, pChild->key(0, TRUE) );
			pChild = pNextChild;
		} while(pChild);

		emit reinitProjectWindow();
	}
}

void ProjectWindow::onOpenKcachegrind()
{
	ProjWndListItem * item   = (ProjWndListItem *) m_tree->selectedItem();
	if (item == NULL)
		return;
	ProjWndListItem * parent = (ProjWndListItem *) item->parent();
	if (parent == NULL)
		return;

	QString sessionDirName = item->text(0);
	QString parentStr = parent->text(0);

	if( (parentStr == TBP_SESSION_STR) || (parentStr == PERF_TBP_SESSION_STR) )
		sessionDirName += ".tbp.dir";
	else if( parentStr == EBP_SESSION_STR )
		sessionDirName += ".ebp.dir";

	emit openKcachegrind(sessionDirName);
}

#if 0
void ProjectWindow::onOpenSessionInDiffAnalyst()
{
	DiffSessionInfo sessInfo;
 
	ProjWndListItem * item = (ProjWndListItem *) m_tree->selectedItem();
	if (item == NULL)
		return;

	sessInfo.sessionFile = item->text(0);
	QString type = item->parent()->text(0);
	if( TBP_SESSION_STR == type) {
		sessInfo.type = DiffSessionInfo::TBP_SESS;	
	} else if (EBP_SESSION_STR == type ) {
		sessInfo.type = DiffSessionInfo::EBP_SESS;	
	}

	emit openSessionInDiffAnalyst(&sessInfo);
}
#endif //0

//----------------------------------------

//the tooltip has to be for the viewport of the listview, or it won't receive
// mouse events to call maybeTip
//ProjWndListTip::ProjWndListTip ( tipColumn,  Q3ListView * pParent) 
ProjWndListTip::ProjWndListTip ( Q3ListView * pParent) 
: QWidget(pParent), m_pList(pParent)
{
	//m_tipColumn = tipColumn;
	//m_pList = pParent;
	m_pList->viewport()->setMouseTracking (true);
}

bool ProjWndListTip::event( QEvent *pEvent)
{
	if (QEvent::ToolTip == pEvent->type())
	{
		QHelpEvent *pHelpEvent = static_cast<QHelpEvent *>(pEvent);
		
		const QPoint &pt = pHelpEvent->pos();

		QPoint pt_local = m_pList->viewportToContents (pt );

		if (m_tipColumn == m_pList->header()->sectionAt (pt_local.x())) 
		{
			//find the current list view item
			//DataListItem *p_temp = (DataListItem *) m_pList->itemAt (pt);
			ProjWndListItem *p_temp = (ProjWndListItem*) m_pList->itemAt (pt);

			//If it's over an actual item...
			if (NULL != p_temp) 
			{
				QRect tempRect = m_pList->itemRect(p_temp);
				tempRect.setLeft (0);
				tempRect.setWidth (m_pList->width());
				//The tip shouldn't change while the cursor stays over the same item.
				QToolTip::showText (tempRect.topLeft(), p_temp->getTip());
			}
		}
		return true;
	}
	return QWidget::event(pEvent);		
}		

//----------------------------------------

ProjWndListItem::ProjWndListItem(Q3ListView * parent,
				ProjWndListItem * after,
				QString name):
Q3ListViewItem(parent, after, name)
{
} 


ProjWndListItem::ProjWndListItem(ProjWndListItem * parent,
				ProjWndListItem * after,
				QString name):
Q3ListViewItem(parent, after, name)
{
} 


ProjWndListItem::ProjWndListItem(Q3ListView * parent,
				QString name):
Q3ListViewItem(parent, name)
{
} 

ProjWndListItem::ProjWndListItem(ProjWndListItem * parent,
				QString name):
Q3ListViewItem(parent, name)
{
}

ProjWndListItem::~ProjWndListItem()
{
} 
