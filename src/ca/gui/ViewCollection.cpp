//$Id: ViewCollection.cpp 19802 2011-06-01 05:08:13Z ssuthiku $
//This file defines the ViewCollection implementation

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
#include "ViewCollection.h"
#include "atuneoptions.h"
#include "stdafx.h"
#include "eventsfile.h"
#include "EventMaskEncoding.h"
#include "helperAPI.h"
#include <qmessagebox.h>


ViewCollection::ViewCollection ()
{
}

ViewCollection::~ViewCollection ()
{
	m_configs.clear();
}

void ViewCollection::helpReadXmlFile(QString path)
{
	QDir dir (path);

	// Sanity check
	if (!dir.exists())
	{
		return ;
	}

	QFileInfoList f_list = dir.entryInfoList ("*.xml");

	for (QFileInfoList::iterator f_it = f_list.begin(); 
		f_it != f_list.end(); f_it++) 
	{
		CViewConfig  * pTemp = new CViewConfig ();
		RETURN_IF_NULL (pTemp, NULL);

		QString filePath = path + "/" + (*f_it).fileName();
		//If it's not a view configuration...
		if (!pTemp->ReadConfigFile ((wchar_t *)filePath.ucs2()))
		{
			delete pTemp;
			continue;
		}

		QString profileName;
		pTemp->GetConfigName (profileName);
		if (m_configs.contains (profileName))
		{
			delete pTemp;
			continue;
		}

		m_configs[profileName].pView = pTemp;
		m_configs[profileName].path = filePath;
		m_configs[profileName].modifiable = false;
	}

	return ;
}

void ViewCollection::readAvailableViews(unsigned long cpuFamily, unsigned long cpuModel)
{
	QString path;

	// **************************************************************
	// Read from user home/.CodeAnalyst/Configs/ViewConfig/<platform> directory
	path = QDir::home ().path() 
		+ QString("/.CodeAnalyst/Configs/ViewConfig/") 
		+ helpGetFamilyDir(cpuFamily, cpuModel);

	helpReadXmlFile(path);
	
	// **************************************************************
	//read from user home/Configs/ViewConfig/ directory
	path = QDir::home ().path() + "/.CodeAnalyst/Configs/ViewConfig";

	helpReadXmlFile(path);

	// **************************************************************
	//read from install/Configs directory
	//read from install/Configs/ViewConfig/<platform> directory
	path = CA_DATA_DIR 
		+ QString("/Configs/ViewConfig/") 
		+ helpGetFamilyDir(cpuFamily, cpuModel);

	helpReadXmlFile(path);

	// **************************************************************
	// Read from install Configs/ViewConfigs/
	path = QString(CA_DATA_DIR) + "/Configs/ViewConfig/";

	helpReadXmlFile(path);

	// **************************************************************
} //ViewCollection::readAvailableProfiles

void ViewCollection::addAllDataView (EventEncodeVec * pAvailable, 
					unsigned long cpuFamily,
					unsigned long model)
{
	CEventsFile eventFile;
	
	if (!pAvailable)
		return;
	
	const static EventConfig timerTest = CViewConfig::EncodeConfig (0xFFF, 0);
	QString file = helpGetEventFile(cpuFamily, model);

	if (!eventFile.open (file))
	{
		QMessageBox::information(qApp->activeWindow(), "Error", 
			"Unable to open the events file: " + file);
		return;
	}

	CViewConfig  * pTemp = new CViewConfig ();
	RETURN_IF_NULL (pTemp, NULL);

	pTemp->SetConfigName (ALL_DATA_VIEW);
	QStringList eventList;

	EventEncodeVec::iterator eit  = pAvailable->begin();
	EventEncodeVec::iterator eend = pAvailable->end();
	for (; eit != eend ; eit++)
	{
		CpuEvent event;

		// NOTE: We have to use the original 16-bit value to do this 
		//       and not the EventConfig encoded value
		if (eventFile.findEventByValue( CViewConfig::ExtractSelect((*eit).eventMask), event))
		{
			// Appending unitmask to the end of name
			unsigned char umask;
			DecodeEventMask((*eit).eventMask, NULL, &umask);
			QString tmpName = event.name + ":" + QString::number(umask);
			eventList += tmpName;

		} else {
			if (timerTest == (*eit).eventMask)
				eventList += "Timer";
			else
				{
					//FIXME //This must be decoded properly, shouldn't be hardcoding like this
					eventList += "Milliseconds";
				}
		}
	}

	pTemp->MakeColumnSpecs (pAvailable, eventList);
	pTemp->SetDescription("This special view has all of the data from the profile available.") ;
	m_configs[ALL_DATA_VIEW].pView = pTemp;
	m_configs[ALL_DATA_VIEW].modifiable = false;
} //ViewCollection::addAllDataView


QStringList ViewCollection::getListOfViews (EventEncodeVec * pAvailable, int * pDefault)
{
	//pAvailable is NULL if we want a list of all views
	QStringList configList;
	ConfigMap::Iterator it = m_configs.begin();
	for( ; it != m_configs.end(); ++it )
	{
		//If the profile fits the available view
		if ((NULL == pAvailable) 
		||  ((*it).pView->isViewDisplayable (pAvailable, false)))
		{
			//configList += it.key();
			configList.append(it.key());
			if ((NULL != pDefault) && ((*it).pView->GetDefaultView()))
			{
				*pDefault = configList.size() - 1;
			}
		}
	}
	return configList;	
}


bool ViewCollection::getViewTexts (QString name, QString *pToolTip, QString *pDescrip)
{
	if (!m_configs.contains (name))
		return false;

	if (NULL != pToolTip)
	{
		m_configs[name].pView->GetToolTip (*pToolTip);
	}
	if (NULL != pDescrip)
	{
		m_configs[name].pView->GetDescription (*pDescrip);
	}
	return true;
}

typedef QMap<EventConfig, int> TempEventIndexMap;

//If cpuCount == 0, then we're getting the settings for visual
bool ViewCollection::getViewConfig (QString name, ViewShownData *pViewShown,
				int cpuCount, unsigned long cpuFamily,
				unsigned long model,
				EventNormValueMap *pNorms)
{
	if (!m_configs.contains (name))
		return false;
	if (NULL == pViewShown)
		return false;

	pViewShown->clear();

	pViewShown->separateCpus = m_configs[name].pView->GetSeparateCPUs ();
	pViewShown->showPercentage = m_configs[name].pView->GetShowPercentage();
	pViewShown->separateTasks = m_configs[name].pView->GetSeparateProcesses ();
	pViewShown->separateThreads = m_configs[name].pView->GetSeparateThreads ();
	
	CEventsFile eventFile;
	QString file = helpGetEventFile(cpuFamily, model);

	if (!eventFile.open (file))
	{
		QMessageBox::information( qApp->activeWindow(), "Error", 
			"Unable to open the events file: " + file);
		return false;
	}

	CpuEvent event;

	//get columns

	int availableCount = m_configs[name].pView->GetNumberOfColumns ();
	ColumnSpec * columnArray = new ColumnSpec[availableCount];
	RETURN_FALSE_IF_NULL (columnArray, NULL);

	m_configs[name].pView->GetColumnSpecs (columnArray) ;

	int index = 0;
	TempEventIndexMap complexMap;
	//for each column
	for (int col = 0; col < availableCount; ++col, ++index)
	{
		unsigned long long encodedColumnEvent = columnArray[col].dataSelectLeft;
		unsigned long long columnEvent   = CViewConfig::ExtractSelect(encodedColumnEvent);
		unsigned char columnMask         = CViewConfig::ExtractUnitMask(encodedColumnEvent);
		//fprintf(stderr,"DEBUG Encoded     = %llx\n",encodedColumnEvent );
		//fprintf(stderr,"DEBUG columnEvent = %llx\n",columnEvent);
		//fprintf(stderr,"DEBUG columnMask  = %x\n",columnMask);

		//store the index offset for the given event, if cpus are separate, use
		//	cpu offset to adjust
		if (ColumnValue == columnArray[col].type ) 
		{
			complexMap[encodedColumnEvent] = index;
		}

		eventFile.findEventByValue( columnEvent, event );

		//if cpus separated
		if ((cpuCount > 1) && (pViewShown->separateCpus))
		{
			//for each cpu
			for (int cpu = 0; cpu < cpuCount; ++cpu, ++index)
			{
				//add viewShown available
				QString title = columnArray[col].title + ":0x" 
					+ QString::number(columnMask, 16) 
					+ " - C" + QString::number (cpu);
				pViewShown->tips += title;

				//add viewShown indexes
				if (ColumnValue == columnArray[col].type )
				{
					if (!event.abbrev.isEmpty())
					{
						title = event.abbrev + " - C" +	QString::number (cpu);
					}
					CpuEventType key;
					key.event	= columnEvent;
					key.cpu	= cpu;
					key.umask	= columnMask;
					pViewShown->indexes[key] = index;
				} else {
					ComplexComponents complex;
					complex.columnIndex = index;
					complex.complexType = columnArray[col].type;
					complex.op1Index = complexMap[encodedColumnEvent] + cpu;

					unsigned long long encodedOtherEvent = columnArray[col].dataSelectRight;
					complex.op2Index = complexMap[encodedOtherEvent] + cpu;
					if (NULL != pNorms)
					{
						complex.op1NormValue = (*pNorms)[encodedColumnEvent];
						complex.op2NormValue = (*pNorms)[encodedOtherEvent];

						//fprintf(stderr,"encodedColumnEvent %llx, norm = %f\n",encodedColumnEvent,complex.op1NormValue);
						//fprintf(stderr,"encodedOtherEvent  %x, norm = %f\n",encodedOtherEvent,complex.op2NormValue);
					}
					pViewShown->calculated[complex.op1Index].append (complex);
					pViewShown->calculated[complex.op2Index].append (complex);
				}
				pViewShown->available += title;

				//persist shown
				if (columnArray[col].visible)
					pViewShown->shown.push_back (index);
			}
			//so that the other for loop won't advance it too far
			--index;
		} else {
			//CPU data is aggregated...

			//add viewShown available
			pViewShown->tips += columnArray[col].title + ":0x" + QString::number(columnMask, 16);

			//for each cpu
			if (ColumnValue == columnArray[col].type )
			{
				if (!event.abbrev.isEmpty())
				{
					pViewShown->available += event.abbrev;
				} else {
					pViewShown->available += columnArray[col].title;
				}
				for (int cpu = 0; cpu < cpuCount; ++cpu)
				{
					//add viewShown indexes
					CpuEventType key;
					key.event 	= columnEvent;
					key.cpu 	= cpu;
					key.umask 	= columnMask;
					pViewShown->indexes[key] = index;
				}
			} else {
				pViewShown->available += columnArray[col].title;
				ComplexComponents complex;
				complex.columnIndex = index;
				complex.complexType = columnArray[col].type;
				complex.op1Index = complexMap[encodedColumnEvent];

				unsigned long long encodedOtherEvent = columnArray[col].dataSelectRight;
				complex.op2Index = complexMap[encodedOtherEvent];
				if (NULL != pNorms)
				{
					complex.op1NormValue = (*pNorms)[encodedColumnEvent];
					complex.op2NormValue = (*pNorms)[encodedOtherEvent];
					//fprintf(stderr,"encodedColumnEvent %llx, norm = %f\n",encodedColumnEvent,complex.op1NormValue);
					//fprintf(stderr,"encodedOtherEvent  %x, norm = %f\n",encodedOtherEvent,complex.op2NormValue);
				} else {
					complex.op1NormValue = complex.op2NormValue = 1;
				}

				pViewShown->calculated[complex.op1Index].append (complex);
				pViewShown->calculated[complex.op2Index].append (complex);
			}

			//persist shown
			if (columnArray[col].visible)
				pViewShown->shown.push_back (index);
		}
	}
	delete [] columnArray;
	complexMap.clear();
	eventFile.close ();

	return true;
} //ViewCollection::getViewConfig


bool ViewCollection::setViewConfig (QString name, ViewShownData *pViewShown, bool saveToFile)
{
	if (!m_configs.contains (name))
		return false;
	if (NULL == pViewShown)
		return false;

	m_configs[name].pView->SetSeparateCPUs (pViewShown->separateCpus);
	m_configs[name].pView->SetSeparateProcesses (pViewShown->separateTasks);
	m_configs[name].pView->SetSeparateThreads (pViewShown->separateThreads);
	m_configs[name].pView->SetShowPercentage (pViewShown->showPercentage);

	//get columns
	int availableCount = m_configs[name].pView->GetNumberOfColumns ();
	ColumnSpec * columnArray = new ColumnSpec[availableCount];
	RETURN_FALSE_IF_NULL (columnArray, NULL);

	m_configs[name].pView->GetColumnSpecs (columnArray) ;

	for (int col = 0; col < availableCount; ++col)
	{
		columnArray[col].visible = false;
	}

	IndexVector::iterator it = pViewShown->shown.begin ();
	for (; it != pViewShown->shown.end(); ++it)
	{
		columnArray[*it].visible = true;
	}

	m_configs[name].pView->SetColumnSpecs (columnArray, availableCount, false) ;
	delete [] columnArray;

	if (!m_configs[name].path.isEmpty()
	&&  saveToFile)
	{
		QString path = m_configs[name].path;
		QString newPath;
		if(path.startsWith(CA_DATA_DIR))
		{
			newPath = QDir::home ().path();
			newPath += "/.CodeAnalyst/";
			newPath += path.section(CA_DATA_DIR,-1);
		}else{
			newPath = path;
		}
			
		
		QString dirPath = newPath.section("/",0,-2);
		QDir dir (dirPath);
		if (!dir.exists())
		{
			//recursively create directories
			int sections = dirPath.count('/');
			for (int sects = 1; sects <= sections; sects ++)
			{
				QDir tempDir;
				tempDir.setPath( dirPath.section ("/", 0, sects) );
				if( !tempDir.exists() )
				{
					tempDir.mkdir (tempDir.path());
				}
			}
		}
		
		m_configs[name].pView->WriteConfigFile ((wchar_t*)newPath.ucs2());
	}
	return true;
} //ViewCollection::setViewConfig


//Bascially copies file to specified file
bool ViewCollection::exportView (QString name, QString fileName)
{
	if (!m_configs.contains (name))
		return false;

	m_configs[name].pView->WriteConfigFile ((wchar_t *)fileName.ucs2());
	return true;
}


//Bascially reads settings and copies file to user's directory
bool ViewCollection::importView (QString fileName)
{
	//Read in profile to get name
	CViewConfig *pTemp = new CViewConfig();
	RETURN_FALSE_IF_NULL (pTemp, NULL);
	pTemp->ReadConfigFile ((wchar_t *)fileName.ucs2());

	QString importName;
	pTemp->GetConfigName (importName);

	//If it's already available, stop
	if (m_configs.contains (importName))
	{
		delete pTemp;
		return false;
	}
	
	m_configs[importName].pView = pTemp;
	m_configs[importName].path = fileName;
//	m_configs[importName].modifiable = true;
	return true;
} //ViewCollection::importProfile


bool ViewCollection::removeView (QString name)
{
	if (!m_configs.contains (name))
	{
		return false;
	}
	//if file in install directory, return false
	if (!m_configs[name].modifiable)
	{
		return false;
	}

	//delete file
	QFile::remove (m_configs[name].path);

	m_configs.remove  (name);
	return true;
}

void ViewCollection::resetView(QString name, unsigned long cpuFamily, unsigned long cpuModel)
{	
	bool isOk = false;
	ConfigMap::iterator it = m_configs.find(name);
	if(it == m_configs.end())
		return;

	QString org_path = it.data().path;
	QString filename = org_path.section("/",-1);

	// Remove the existing user-specific settings
	if(!org_path.startsWith(CA_DATA_DIR))
	{
		m_configs.erase(it);
		QFile::remove(org_path);
	}

	isOk = true;
	//read from original ViewConfig Directory.
	QString path = QString(CA_DATA_DIR) 
		+ "/Configs/ViewConfig/"
		+ helpGetFamilyDir(cpuFamily, cpuModel) + filename;
	if(!QFile::exists(path))
	{
		// Use non-arch-specific default setting
		path =  QString(CA_DATA_DIR) + "/Configs/ViewConfig/" 
			+ filename;
		if(!QFile::exists(path))
		{
			QMessageBox::critical(qApp->activeWindow(), QString("CodeAnalyst Error"),
				QString("ERROR:\n")
				+ "View Configureation " + name 
				+ " does not exist.");
			isOk = false;
		}
	}

	if(isOk)
	{
		importView(path);
	}
		
}

void ViewCollection::updateView(QString name, unsigned long cpuFamily, unsigned long cpuModel)
{
	bool isOk = false;	
	ConfigMap::iterator it = m_configs.find(name);
	if(it == m_configs.end())
		return;
	
	QString org_path = it.data().path;
	QString filename = org_path.section("/",-1);

	//delete view
	m_configs.remove  (name);

	//Reimporting the view
	QString familyStr = helpGetFamilyDir(cpuFamily, cpuModel);
 
	isOk = true;
	//Check user home/.CodeAnalyst/Configs/ViewConfig/ directory
	QString path = QDir::home ().path() 
		+ "/.CodeAnalyst/Configs/ViewConfig/" 
		+ familyStr + filename;
	if(!QFile::exists(path))
	{
		// Check non-arch-specific dir
		path = QDir::home ().path() 
			+ "/.CodeAnalyst/Configs/ViewConfig/" 
			+ filename;
		if(!QFile::exists(path))
		{
			// Use default setting
			path =  QString(CA_DATA_DIR) + "/Configs/ViewConfig/" 
				+ familyStr + filename;
			if(!QFile::exists(path))
			{
				// Use non-arch-specific default setting
				path =  QString(CA_DATA_DIR) + "/Configs/ViewConfig/" 
					+ filename;
				if(!QFile::exists(path))
				{
					QMessageBox::critical(qApp->activeWindow(), QString("CodeAnalyst Error"),
						QString("ERROR:\n")
						+ "View Configureation " + name 
						+ " does not exist.");
					isOk = false;
				}
			}
		}
	}

	if(isOk)
	{
		importView(path);
	}
}
