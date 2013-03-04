//$Id: atuneoptions.cpp,v 1.4 2006/05/15 22:09:22 jyeh Exp $
// Implementation for CATuneOptions class, which encapsulates dealings with 
//  the application settings.

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

#include <stdlib.h>

#include "stdafx.h"
#include "atuneoptions.h"
#include <qapplication.h>

//#include "op_config.h"
#ifndef OP_BASE_DIR
#define OP_BASE_DIR "/var/lib/oprofile/"
#endif 

#ifndef OP_SAMPLES_DIR
#define OP_SAMPLES_DIR OP_BASE_DIR "samples/"
#endif

#ifndef OP_SAMPLES_CURRENT_DIR
#define OP_SAMPLES_CURRENT_DIR OP_SAMPLES_DIR "current/"
#endif



// Baskar: BUG335204, In Qt 4.8, QSettings::setPath does not seem to set
// proper file format. We need to use the appropriate constructor instead.
// This solves the crash issue Qt4.8.1.
// Also, QSettings::insertSearchPath is a no-op in Qt4.8
//
// Note: QSettings will use $HOME/.config/AMD/CodeAnalyst.conf for storing the
// application settings.
//
CATuneOptions::CATuneOptions()
	: m_qsetting(QSettings::NativeFormat,
		     QSettings::UserScope,
		     "AMD",
		     "CodeAnalyst")
{
}


CATuneOptions::~CATuneOptions()
{
}

bool CATuneOptions::getShowAggregationController(uint * show)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry (QString (OPT_SHOW_AGG_CONTROLLER),
					 0, &ok);
	if (ok)
		*show= temp;
	return ok;
}

bool CATuneOptions::setShowAggregationController(uint show)
{
	return m_qsetting.writeEntry (QString (OPT_SHOW_AGG_CONTROLLER),
		static_cast<int>(show));
}

bool CATuneOptions::getDataAggregation(uint * data_agg)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry(QString (OPT_DATA_AGGREGATION),0, &ok);
	if (ok)
		*data_agg = temp;
	else
		*data_agg = AGG_INLINEINSTANCE;

	return ok;
}

bool CATuneOptions::setDataAggregation(uint data_agg)
{
	return m_qsetting.writeEntry (QString (OPT_DATA_AGGREGATION),
		static_cast<int>(data_agg));
}

bool CATuneOptions::getBypassSource (uint * bypass_source)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry (QString (OPT_BYPASS_SOURCE),
		0, &ok);
	if (ok)
		*bypass_source = temp;
	else
		*bypass_source = NO_BYPASS_SOURCE;

	return ok;
}


bool CATuneOptions::setBypassSource (uint bypass_source)
{
	return m_qsetting.writeEntry (QString (OPT_BYPASS_SOURCE),
		static_cast<int>(bypass_source));
}


bool CATuneOptions::getReplaceSysDir ( uint * replace )
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry (QString (OPT_REPLACESYSDIR), 0,
		&ok);
	if (ok)
		*replace  = temp;
	return ok;
}


bool CATuneOptions::setReplaceSysDir (uint replace)
{
	return m_qsetting.writeEntry (QString (OPT_REPLACESYSDIR),
		static_cast<int>(replace));
}


bool CATuneOptions::getAlertNoSource (uint * alert_no_source)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry (QString (OPT_SOURCE_ALERT),
		0, &ok);
	if (ok)
		*alert_no_source = temp;
	else
		*alert_no_source = SOURCE_ALERT;
	return ok;
}


bool CATuneOptions::setAlertNoSource ( uint alert_no_source )
{
	return m_qsetting.writeEntry (QString (OPT_SOURCE_ALERT),
		static_cast<int>(alert_no_source));
}


bool CATuneOptions::getDefaultProjectDir (QString & pro_dir)
{
	bool ok;
	QString temp;
	temp = m_qsetting.readEntry (QString (OPT_PROJDIR), NULL, &ok );
	if (ok)
		pro_dir = temp;
	return ok;
}


bool CATuneOptions::setDefaultProjectDir (QString & pro_dir)
{
	return m_qsetting.writeEntry (QString (OPT_PROJDIR), pro_dir);
}


bool CATuneOptions::getDebugSearchPaths (QString & search_path)
{
	bool ok;
	QString temp;
	temp = m_qsetting.readEntry (QString (OPT_SEARCH_PATHS), NULL,
		&ok );
	if (ok)
		search_path = temp;
	return ok;
}


bool CATuneOptions::setDebugSearchPaths (QString & search_path)
{
	return m_qsetting.writeEntry (QString (OPT_SEARCH_PATHS), search_path);
}


bool CATuneOptions::getUseHotKeys (uint * use_hot_keys)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry (QString (OPT_USE_HOTKEYS), 0,
		&ok);
	if (ok)
		*use_hot_keys = temp;
	else
		*use_hot_keys = NO_HK_USE;
	return ok;
}


bool CATuneOptions::setUseHotKeys (uint use_hot_keys)
{
	return m_qsetting.writeEntry (QString (OPT_USE_HOTKEYS),
		static_cast<int>(use_hot_keys));
}


bool CATuneOptions::getMruFileList (QString & mru)
{
	bool ok;
	QString temp;
	temp = m_qsetting.readEntry (QString (OPT_MRUCAWLIST), NULL, &ok );
	if (ok)
		mru = temp;
	return ok;
}


bool CATuneOptions::setMruFileList (QString & mru)
{
	return  m_qsetting.writeEntry (QString (OPT_MRUCAWLIST), mru);
}


bool CATuneOptions::getFontSize (uint * font_size)
{
	bool ok;
	uint temp;
	temp  = m_qsetting.readNumEntry (QString (OPT_FONT_SIZE), 0, &ok);
	if (ok)
		*font_size= temp;
	else
		*font_size = FONT_SIZE_DEFAULT;
	return ok;
}


bool CATuneOptions::setFontSize (uint font_size)
{
	return m_qsetting.writeEntry (QString (OPT_FONT_SIZE),
		static_cast<int>(font_size));
}


bool CATuneOptions::getSortEventByIndex (uint * sort_by_index)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry (QString (OPT_EVENT_SORT), 0,
		&ok);
	if (ok)
		*sort_by_index = temp;
	return ok;
}


bool CATuneOptions::setSortEventByIndex (uint sort_by_index)
{
	return m_qsetting.writeEntry (QString (OPT_EVENT_SORT),
		static_cast<int>(sort_by_index));
}


bool CATuneOptions::getShowChartDensity (uint *show_chart_density)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry (QString (OPT_CHART_DENSITY), 0,
		&ok);
	if (ok)
		*show_chart_density = temp;
	else 
		*show_chart_density = 1;
	return ok;
}


bool CATuneOptions::setShowChartDensity (uint show_chart_density)
{
	return m_qsetting.writeEntry (QString (OPT_CHART_DENSITY),
		static_cast<int>(show_chart_density));
}


bool CATuneOptions::getImportType(uint * import_type)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry(QString(OPT_IMPORT_TYPE), 0, &ok);

	if (ok)
		*import_type = temp;
	return ok;
}

bool CATuneOptions::setImportType(uint import_type)
{
	return m_qsetting.writeEntry(QString(OPT_IMPORT_TYPE),
		static_cast<int>(import_type));
}


bool CATuneOptions::getImportOpdir(QString & pro_dir)
{
	bool ok;
	QString temp;

	pro_dir = OP_SAMPLES_CURRENT_DIR;
	temp = m_qsetting.readEntry (QString (OPT_IMPORT_OPDIR), NULL, &ok );
	if (ok)
		pro_dir = temp;
	return ok;
}


bool CATuneOptions::setImportOpdir(QString & pro_dir)
{
	return m_qsetting.writeEntry (QString (OPT_IMPORT_OPDIR), pro_dir);
}


bool CATuneOptions::getImportTAR(QString & jnc)
{
	bool ok;
	QString temp;

	jnc = "~";
	temp = m_qsetting.readEntry (QString (OPT_IMPORT_TAR), NULL, &ok );
	if (ok)
		jnc = temp;
	return ok;
}


bool CATuneOptions::setImportTAR(QString & jnc) 
{
	return m_qsetting.writeEntry (QString (OPT_IMPORT_TAR), jnc);
}


bool CATuneOptions::setOPPlugIn(QString & name)
{
	return m_qsetting.writeEntry(QString(OPT_PLUG_IN), name);
}


bool CATuneOptions::getOPPlugIn(QString & name)
{
	bool ok;
	QString temp;

	name = "";
	temp = m_qsetting.readEntry (QString (OPT_PLUG_IN), NULL, &ok );
	if (ok)
		name = temp;
	return ok;
}


bool CATuneOptions::getJPA(uint * jpa_type)
{
	bool ok;
	uint temp;
	temp = m_qsetting.readNumEntry(QString(OPT_JAVA_JPA), 0, &ok);

	if (ok)
		*jpa_type = temp;
	return ok;

}


bool CATuneOptions::setJPA(uint jpa_type)
{    
	return m_qsetting.writeEntry(QString(OPT_JAVA_JPA),
		static_cast<int>(jpa_type));
}


bool CATuneOptions::getImportMergeByLib(uint * merge_by_lib)
{
	bool ok;
	uint temp;
	temp = 
		m_qsetting.readNumEntry(QString(OPT_OP_MERGE_LIB),0, &ok);

	if (ok)
		*merge_by_lib = temp;
	else
		*merge_by_lib = NO_MERGE_LIB;

	return ok;

}


bool CATuneOptions::setImportMergeByLib(uint merge_by_lib)
{
	return m_qsetting.writeEntry (QString(OPT_OP_MERGE_LIB),
		static_cast<int>(merge_by_lib));
}


bool CATuneOptions::getImportExcludeDep(uint * exclude_dep)
{
	bool ok;
	uint temp;
	temp = 
		m_qsetting.readNumEntry(QString(OPT_OP_EXCLUDE_DEP),0, &ok);

	if (ok)
		*exclude_dep = temp;
	else
		*exclude_dep = NO_EXCLUDE_DEP;

	return ok;

}


bool CATuneOptions::setImportExcludeDep(uint exclude_dep)
{
	return m_qsetting.writeEntry (QString(OPT_OP_EXCLUDE_DEP),
		static_cast<int>(exclude_dep));
}


bool CATuneOptions::getCpuColorList ( ColorList *pColorList)
{
	bool ok;
	QString temp = m_qsetting.readEntry (QString (OPT_COLOR_LIST), NULL,
		&ok);

	if (ok) 
	{
		QStringList colors = temp.split (",");
		for (QStringList::Iterator it = colors.begin(); it != colors.end(); 
			++it)
		{
			pColorList->push_back (QColor (QRgb ((*it).toUInt())));
		}
	} else {
		pColorList->push_back (Qt::blue);
		pColorList->push_back (Qt::black);
		pColorList->push_back (Qt::red);
		pColorList->push_back (Qt::darkYellow);
		pColorList->push_back (Qt::green);
		pColorList->push_back (Qt::darkGreen);
		pColorList->push_back (Qt::white);
		pColorList->push_back (Qt::darkBlue);
		pColorList->push_back (Qt::cyan);
		pColorList->push_back (Qt::darkCyan);
		pColorList->push_back (Qt::magenta);
		pColorList->push_back (Qt::darkMagenta);
		pColorList->push_back (Qt::yellow);
		pColorList->push_back (Qt::darkRed);
		pColorList->push_back (Qt::lightGray);
		pColorList->push_back (Qt::darkGray);

		//If more are needed, they should be randomly generated at the time
		//	with pColorList->push_back (QColor ( QRgb (rand())));
		setCpuColorList (pColorList);
	}
	return ok;
} //CATuneOptions::getCpuColorList


bool CATuneOptions::setCpuColorList ( const ColorList *pColorList)
{
	QString temp;

	for (unsigned int i = 0; i < pColorList->count(); i++)
		temp += QString::number ((*pColorList)[i].rgb()) + ",";
	return m_qsetting.writeEntry (QString (OPT_COLOR_LIST), temp);
}

bool CATuneOptions::getPrecision (int *pPrecision)
{
	if (NULL == pPrecision)
		return false;
	bool ok;
	int temp;
	//2 is the default precision
	temp = m_qsetting.readNumEntry (CA_PRECISION_KEY, 2, &ok);
	if (ok)
		*pPrecision = temp;
	else
		*pPrecision = 2;
	return ok;
}

bool CATuneOptions::setPrecision (int precision)
{
	return m_qsetting.writeEntry (CA_PRECISION_KEY,
		static_cast<int>(precision));
}

//depreciated functions
bool CATuneOptions::getCpuColorList ( QColor color_list[COLOR_LIST_SIZE])
{
	bool ok;
	QString temp = m_qsetting.readEntry (QString (OPT_COLOR_LIST), NULL,
		&ok);

	if (ok) {
		for (int i = 0; i < COLOR_LIST_SIZE; i++)
			color_list[i].setRgb (QRgb (temp.section (',', i, i).toUInt()));
	} else {
		color_list[0] = Qt::white;
		color_list[1] = Qt::black;
		color_list[2] = Qt::red;
		color_list[3] = Qt::darkRed;
		color_list[4] = Qt::green;
		color_list[5] = Qt::darkGreen;
		color_list[6] = Qt::blue;
		color_list[7] = Qt::darkBlue;
		color_list[8] = Qt::cyan;
		color_list[9] = Qt::darkCyan;
		color_list[10] = Qt::magenta;
		color_list[11] = Qt::darkMagenta;
		color_list[12] = Qt::yellow;
		color_list[13] = Qt::darkYellow;
		color_list[14] = Qt::lightGray;
		color_list[15] = Qt::darkGray;

		for (int i = 16; i < COLOR_LIST_SIZE; i++)
			color_list[i] = QColor ( QRgb (random()));
	}
	return ok;
}


bool CATuneOptions::setCpuColorList ( QColor color_list[COLOR_LIST_SIZE])
{
	QString temp;

	for (int i = 0; i < COLOR_LIST_SIZE; i++)
		temp += QString::number (color_list[i].rgb()) + ",";
	return m_qsetting.writeEntry (QString (OPT_COLOR_LIST), temp);
}
