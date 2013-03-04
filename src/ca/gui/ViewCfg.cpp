//$Id: ViewCfg.cpp 20178 2011-08-25 07:40:17Z sjeganat $

/*
// CodeAnalyst for Open Source
// Copyright 2006-2007 Advanced Micro Devices, Inc.
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

#include <q3textedit.h>
#include <q3listbox.h>
#include <qcombobox.h>
#include "ViewCfg.h"
#include "atuneoptions.h"

#define K7_PLATFORM_STR		"CPU Family 0x6 (K7)"
#define K8_PLATFORM_STR		"CPU Family 0xF (K8)"
#define FAMILY10H_PLATFORM_STR	"CPU Family 0x10"
#define FAMILY11H_PLATFORM_STR	"CPU Family 0x11"
#define FAMILY12H_PLATFORM_STR	"CPU Family 0x12"
#define FAMILY14H_PLATFORM_STR	"CPU Family 0x14"
#define FAMILY15H_PLATFORM_STR	"CPU Family 0x15"
#define FAMILY15H_1XH_PLATFORM_STR	"CPU Family 0x15 Model 0x10-1F"

ShownListBoxItem::ShownListBoxItem ( Q3ListBox* listbox, const QString & text, int index)
: Q3ListBoxText( listbox, text)
{
	m_index = index;
}


ViewCfgDlg::ViewCfgDlg ( 
	QWidget* parent, 
	Qt::WFlags fl) 
: QDialog(parent, fl|CA_DLG_FLAGS)
{
	setupUi(this);
	m_modified 	= false;
	m_pViews 	= NULL;
	m_globalView 	= false;
	m_pLastView	= NULL;

	m_pSeparateThreads->hide();
	m_pSeparateTasks->hide();

	connect (m_pAddShown, SIGNAL (clicked()), SLOT (onModified()));
	connect (m_pRemoveShown, SIGNAL (clicked()), SLOT (onModified()));
	connect (m_pSeparateCpus, SIGNAL (clicked()), SLOT (onModified()));
	connect (m_pSeparateTasks, SIGNAL (clicked()), SLOT (onModified()));
	connect (m_pSeparateThreads, SIGNAL (clicked()), SLOT (onModified()));
	connect (m_pShowPercentage, SIGNAL (clicked()), SLOT (onModified()));

	connect (m_pPlatformName, SIGNAL (activated (const QString &)), 
		SLOT (onChangePlatform (const QString &)));

	connect (m_pViewName, SIGNAL (activated (const QString &)), 
		SLOT (onChangeView (const QString &)));

	connect (m_pAddShown, SIGNAL (clicked()), SLOT (onAddShown()));
	connect (m_pRemoveShown, SIGNAL (clicked()), SLOT (onRemoveShown()));

	connect (buttonOk, SIGNAL (clicked()), SLOT (onOk ()));
	connect (buttonApply, SIGNAL (clicked()), SLOT (onApply ()));
}


ViewCfgDlg::~ViewCfgDlg()
{
	if(m_pViews != NULL)
	{
		m_pViews = NULL;	
	}
}


void ViewCfgDlg::setViews (QString current, 
			   unsigned long cpuFamily,
			   unsigned long cpuModel,
			   bool globalView,
			   ViewCollection* pViews)
{
	m_globalView = globalView;
	m_lastView = current;
	m_cpuFamily = cpuFamily;
	m_cpuModel= cpuModel;
	QString platformName = "";
	QStringList views; 
	m_pPlatformName->clear();
	m_pViewName->clear();

	/*
	* Note: When doing non-global view configuration, we pass in pViews
	* 	 with "All Data" view already added. Then, add the rest of the views
	*	 based on the CPU family.  The "All Data" view doesn't exist
	* 	 in global view configuration.
	*/
	if(pViews != NULL)
	{
		m_pViews = pViews;
	}
	else
	{
		m_pViews = &m_Views;
		// Set current text to the cpuFamily
		m_pViews->readAvailableViews(cpuFamily);
	}

	views = m_pViews->getListOfViews ();

	// For global view management, we list all the supported 
	// platforms
	if(m_globalView)
	{
		m_pPlatformName->setEnabled(true);

		// K7
		m_pPlatformName->addItem(QString(K7_PLATFORM_STR));

		// K8
		m_pPlatformName->addItem(QString(K8_PLATFORM_STR));

		// FAMILY10H
		m_pPlatformName->addItem(QString(FAMILY10H_PLATFORM_STR));
		
		// FAMILY11H
		m_pPlatformName->addItem(QString(FAMILY11H_PLATFORM_STR));

		// FAMILY12H
		m_pPlatformName->addItem(QString(FAMILY12H_PLATFORM_STR));

		// FAMILY14H
		m_pPlatformName->addItem(QString(FAMILY14H_PLATFORM_STR));

		// FAMILY15H
		m_pPlatformName->addItem(QString(FAMILY15H_PLATFORM_STR));
		
		// FAMILY15H_1XH
		m_pPlatformName->addItem(QString(FAMILY15H_1XH_PLATFORM_STR));

		buttonOk->setText("&Save and Close");
	}else{
		// We restrict the platform setting in the session view.
		m_pPlatformName->setEnabled(false);
	}

	switch(cpuFamily)
	{
	case ATHLON_FAMILY:
		if(!m_globalView)
			m_pPlatformName->addItem (QString(K7_PLATFORM_STR));
		m_pPlatformName->setCurrentText(K7_PLATFORM_STR);
		break;
	case OPTERON_FAMILY:
		if(!m_globalView)
			m_pPlatformName->addItem (QString(K8_PLATFORM_STR));
		m_pPlatformName->setCurrentText(K8_PLATFORM_STR);
		break;
	case GREYHOUND_FAMILY:
		if(!m_globalView)
			m_pPlatformName->addItem (QString(FAMILY10H_PLATFORM_STR));
		m_pPlatformName->setCurrentText(FAMILY10H_PLATFORM_STR);
		break;
	case GRIFFIN_FAMILY:
		if(!m_globalView)
			m_pPlatformName->addItem (QString(FAMILY11H_PLATFORM_STR));
		m_pPlatformName->setCurrentText(FAMILY11H_PLATFORM_STR);
		break;
	case FAMILY12H_FAMILY:
		if(!m_globalView)
			m_pPlatformName->addItem (QString(FAMILY12H_PLATFORM_STR));
		m_pPlatformName->setCurrentText(FAMILY12H_PLATFORM_STR);
		break;
	case FAMILY14H_FAMILY:
		if(!m_globalView)
			m_pPlatformName->addItem (QString(FAMILY14H_PLATFORM_STR));
		m_pPlatformName->setCurrentText(FAMILY14H_PLATFORM_STR);
		break;
	case FAMILY15H_FAMILY:
		if (cpuModel < 0x10) {
			if(!m_globalView)
				m_pPlatformName->addItem (QString(FAMILY15H_PLATFORM_STR));
			m_pPlatformName->setCurrentText(FAMILY15H_PLATFORM_STR);
		} else if (cpuModel < 0x20) {
			if(!m_globalView)
				m_pPlatformName->addItem (QString(FAMILY15H_1XH_PLATFORM_STR));
			m_pPlatformName->setCurrentText(FAMILY15H_1XH_PLATFORM_STR);
		}
		break;
	default:
		break;
	}

	// Check current view
	if (current.isEmpty ())
	{
		current = views[0];
	}

	m_pViewName->addItems(views);
	m_pViewName->setCurrentText (current);


	if(!m_globalView)
		buttonReset->hide();

	updateUi (current);
}

/*
* NOTE: This function should only be called from
* 	 the Global View Configuration
*/ 
void ViewCfgDlg::onChangePlatform(const QString & platformName)
{
	QStringList views; 

	if(platformName == K7_PLATFORM_STR)
		m_cpuFamily = ATHLON_FAMILY;
	else if(platformName == K8_PLATFORM_STR)
		m_cpuFamily = OPTERON_FAMILY;
	else if(platformName == FAMILY10H_PLATFORM_STR)
		m_cpuFamily = GREYHOUND_FAMILY;
	else if(platformName == FAMILY11H_PLATFORM_STR)
		m_cpuFamily = GRIFFIN_FAMILY;
	else if(platformName == FAMILY12H_PLATFORM_STR)
		m_cpuFamily = FAMILY12H_FAMILY;
	else if(platformName == FAMILY14H_PLATFORM_STR)
		m_cpuFamily = FAMILY14H_FAMILY;
	else if(platformName == FAMILY15H_PLATFORM_STR)
		m_cpuFamily = FAMILY15H_FAMILY;
	else if(platformName == FAMILY15H_1XH_PLATFORM_STR) {
		m_cpuFamily = FAMILY15H_FAMILY;
		m_cpuModel = 0x10;
	}

	// Save the current selected view
	QString oldView = m_pViewName->currentText();

	m_Views.removeAllView();
	m_Views.readAvailableViews(m_cpuFamily, m_cpuModel);
	views = m_Views.getListOfViews ();
	m_pViewName->clear();
	m_pViewName->addItems(views);

	// Show the previously selected view if exist
	if (!views.contains(oldView))
	{
		oldView = views[0];	
	}
	m_pViewName->setCurrentText(oldView);
	updateUi (oldView);	

}

void ViewCfgDlg::onApply ()
{
	if (m_modified)
		updateView (m_pViewName->currentText());

	if(m_pLastView)	
		*m_pLastView = m_pViewName->currentText();
}


void ViewCfgDlg::onOk ()
{
	onApply();
	m_pShown->clear();
	accept();
}

void ViewCfgDlg::onChangeView (const QString & viewName)
{
	m_pShown->clear();
	m_modified = false;
	m_lastView = viewName;
	updateUi (viewName);
}


void ViewCfgDlg::updateUi (QString viewName)
{
	QString description;
	m_pViews->getViewTexts (viewName, NULL, &description);
	m_pDescription->setText (description);

	ViewShownData viewData;

	//The 0 is to get the raw data columns
	m_pViews->getViewConfig (viewName, &viewData, 0, m_cpuFamily, m_cpuModel);
	m_pAvailable->clear();
	m_pShown->clear();
	//This algorithm is potentially nlogn, but since it handles such small 
	//	sizes of n, it doesn't matter
	for (int i = 0; i < (int) viewData.tips.size(); i++)
	{
		IndexVector::iterator it;
		bool found = false;
		for (it = viewData.shown.begin(); it != viewData.shown.end(); ++it)
		{
			//if the index is shown
			if ((*it) == i)
			{
				new ShownListBoxItem (m_pShown, viewData.tips[i], i);
				found = true;
				break;
			}
		}
		if (!found)
		{
			new ShownListBoxItem (m_pAvailable, viewData.tips[i], i);
		}
	}

	m_pShowPercentage->setChecked (viewData.showPercentage);
	m_pSeparateCpus->setChecked ( viewData.separateCpus);
	m_pSeparateTasks->setChecked (viewData.separateTasks);
	m_pSeparateThreads->setChecked (viewData.separateThreads);
}  //ViewCfgDlg::updateUi


void ViewCfgDlg::updateView (QString viewName)
{
	if (0 == m_pShown->count())
	{
		QMessageBox::critical (this, "CodeAnalyst Error",
			"At least one piece of data should be shown.");
		return;
	}
	ViewShownData viewData;

	//The 0 is to get the raw data columns
	m_pViews->getViewConfig (viewName, &viewData, 0, m_cpuFamily, m_cpuModel);

	viewData.shown.clear();
	for (UINT i = 0; i < m_pShown->count(); i++)
	{
		ShownListBoxItem *pItem = (ShownListBoxItem *)m_pShown->item (i);
		viewData.shown.push_back (pItem->m_index);
	}

	viewData.showPercentage = m_pShowPercentage->isChecked ();
	viewData.separateCpus = m_pSeparateCpus->isChecked ();
	viewData.separateTasks = m_pSeparateTasks->isChecked ();
	viewData.separateThreads = m_pSeparateThreads->isChecked ();

	// Save to file if m_globalView is true
	m_pViews->setViewConfig (viewName, &viewData, m_globalView);

} //ViewCfgDlg::updateView

void ViewCfgDlg::onAddShown ()
{
	int item = m_pAvailable->currentItem();
	if (-1 == item)
		return;

	ShownListBoxItem *pItem = (ShownListBoxItem *)m_pAvailable->item (item);

	new ShownListBoxItem (m_pShown, m_pAvailable->currentText(), pItem->m_index);
	m_pAvailable->removeItem (item);  //or delete pItem
}


void ViewCfgDlg::onRemoveShown ()
{
	int item = m_pShown->currentItem();
	if (-1 == item)
		return;

	ShownListBoxItem *pItem = (ShownListBoxItem *)m_pShown->item (item);

	new ShownListBoxItem (m_pAvailable, m_pShown->currentText(), pItem->m_index);
	m_pShown->removeItem (item);  //or delete pItem
}


void ViewCfgDlg::onModified ()
{
	m_modified = true;
}

void ViewCfgDlg::onResetCurrentView()
{

	if(QMessageBox::warning(this,"Reset View Confirmation",
			QString("CodeAnalyst is resetting this view configuration to the original value.\"")
				+ m_pViewName->currentText() + "\"\n\n"
				+ "Confirm resetting?\n",
				QMessageBox::Yes,QMessageBox::No)
		!= QMessageBox::Yes)
	{
		return;
	}

	m_pViews->resetView(m_pViewName->currentText(),m_cpuFamily);
	updateUi(m_pViewName->currentText());	
}
