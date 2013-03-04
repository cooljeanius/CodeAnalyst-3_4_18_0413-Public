// disassembly view 
//
/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2006 Advanced Micro Devices, Inc.
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


#ifndef DASMVIEW_H
#define DASMVIEW_H

#include "stdafx.h"
#include "symbolengine.h"
#include "auxil.h"
#include "codeDensity.h"
#include "sessionnav.h"

#include "bbanalysis.h"

#include <map>
#include <string>
#include <QShowEvent>
#include <Q3ValueList>
#include <Q3PopupMenu>

using namespace std;

// constants for the list columns

enum DASM_COLUMNS {
	_ASM_ADDRESS_COLUMN = 0,
	_ASM_CODE_BYTES_COLUMN,
	_ASM_INSTRUCTION_COLUMN,
	_ASM_SYMBOL_COLUMN,
	_ASM_OFFSET_INDEX,
	_ASM_INVALID
};

enum DASM_CODE_DENSITY_Enum {
	DASM_ZOOM_MODULE = 0,
	DASM_ZOOM_SECTION,
	DASM_ZOOM_USER,
	DASM_ZOOM_SHOWN,
};

enum DASM_POPUPS {
	ASM_POP_COPY = 0,
	ASM_POP_UP,
	ASM_POP_DOWN,
	ASM_POP_SHOWN
};

typedef struct {
	VADDR addr;
	DataArray samples;
	QString function;
} DasmChartSample ;

typedef Q3ValueList < DasmChartSample > DasmChartSampleList;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// class used to sort the list view, if needed
///////////////////////////////////////////////////////////////////////////////
class DasmTabItem : public DataListItem
{
public:
	int *m_pPrecision;
	DasmTabItem (Q3ListView * pParent, DASM_LV_ITEM item,
		ViewShownData *pViewShown, int indexOffset);
protected:
	QString key (int column, bool ascending) const;
};


//DensityChartArea defined in codeDensity.h
class DasmDensityChart : public DensityChartArea {
	Q_OBJECT
public:
	DasmDensityChart (ViewShownData *pViewShown, QWidget * parent, 
		ProfileAttribute profile);
	virtual ~DasmDensityChart ();

	bool initialize (SampleMap * samp_map);
	void showCurrent (VADDR start, VADDR end);
	void setInterestAddr (VADDR interesting);

	//overwriting pure virtual function
	virtual void zoomChanged (int zoom_level);

private:
	void groupSections ();
	void storeSessionData (SampleMap * samp_map);
	bool readModuleData ();
	bool initialize_symbolengine();

protected:
	//overwriting pure virtual functions!
	virtual bool groupData ();
	virtual void tipGroups ();

public slots:
	void onViewChanged (SampleMap *pSamples);

private slots:
	void onGroupDoubleClicked (UINT64 group_data);

signals: 
	void doubleClicked (VADDR interesting_addr);

private:
	VADDR m_mod_start;
	VADDR m_mod_end;

	ProfileAttribute m_profile;
	QString m_module_name;

	DasmChartSampleList m_sample_list;
	SymbolEngine  * m_symbols_engine;
};


//ZoomDock defined in codeDensity.h
class DasmDensityView : public ZoomDock {
	Q_OBJECT 
public:
	DasmDensityView (QWidget * parent);
	~DasmDensityView ();

	bool initialize (ViewShownData *pViewShown, ProfileAttribute profile, 
		SampleMap * samp_map, VADDR hot_spot);
	void shownDataChanged (VADDR start, VADDR end);
	void setInterestPoint (VADDR interesting);
	void setShowCurrentData (bool show_current);
	void setClickedCaption (QString caption);

public slots: 
	void onDoubleClicked (VADDR hot_spot);

signals: 
		void newHotSpot (VADDR hot_spot, QString caption, SampleMap *pMap);

private:
	DasmDensityChart * m_dasm_chart;
};

typedef multimap<VADDR, VADDR> BR_DEST_MAP;

// key is pop-up menu id, data is branch dest or source address;
typedef map<int, VADDR> POPUPMENU_MAP;

///////////////////////////////////////////////////////////////////////////////
class DasmTab : public DataTab {
	Q_OBJECT
		// functions
public:
	DasmTab (VADDR Address, SampleMap *SampMap, ViewShownData *pViewShown,
		ProfileAttribute profAtt, QWidget * parent,
		const char * name, int wflags);
	virtual ~DasmTab ();


	bool Init ();
	bool Dasm ();
	void Tag (QString caption);
	bool displayProgress_callback();

protected:
	void showEvent (QShowEvent * e);

private:
	bool initialize_symbolengine();
	VADDR FindStartAddress ();
	VADDR FindStartShowAddress ();
	bool GetSectionMap();
	void AddSampleToLabelMap ();
	VADDR FindBlockStartAddr (VADDR centerItemAddr);
	bool GetSectionAddress (VADDR address, VADDR * pStartVAddr,
		VADDR * pEndVAddr);
	void BuildUpWorkflowMap();
	void ChangeInterestItem(VADDR focus);

private slots:
	void OnNewHotspot (VADDR addr, QString caption, SampleMap *pMap);
	void OnRightClick (Q3ListViewItem * pItem, const QPoint & pt, int i);
//	void OnDoubleClick (QListViewItem * pItem, const QPoint & pt, int i);
	void OnPageUp ();
	void OnMenuItemClick(int menuid);
	void OnPageDown ();
	void OnSelectionChange ();
	void OnListRedrawn ();

public slots:
	void OnDensityVisibilty ();
	virtual void onViewChanged (ViewShownData* pShownData);

signals:
	void viewChange (SampleMap *pSamples);

private:
	bool m_oneAtATime;
	QString m_ModuleName;
	QString m_file_path;
	VADDR m_Address;
	VADDR m_vaImageBase;
	QFile * m_pFile;
	QString m_Caption;
	ChartListView * m_pAsmList;
	Q3ListViewItem * m_pItemOfInterest;
	SampleMap *m_SampMap;
	
	// this is used for current view items
	SampleMap m_ViewSampMap;

	Q3PopupMenu * m_pMenu;
	SymbolEngine  * m_dasmtab_symengine;

	section_map_type m_SectionMap;
	VADDR m_FirstInstVAddr;
	VADDR m_LastInstVAddr;
	QScrollBar * m_pScrollBar;
	ProfileAttribute m_ProfAtt;
	unsigned int m_totalModSamples;

	DasmDensityView * m_densityView;


	Q3ProgressDialog * m_prog_dlg;

	/* Number of the next threshold to actually update the gui */
	unsigned int m_display_progress_threshold;
	/* Number of times the callback has been called */
	unsigned int m_display_progress_calls;

	BBAnalysis m_bba;
	BR_DEST_MAP m_ViewBrDestMap;
	BR_DEST_MAP m_ViewBrSrcMap;
	POPUPMENU_MAP m_PopMenuMap;
};

#endif // #ifndef DASMVIEW_H
