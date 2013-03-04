// interface for the GraphView class.
//
// CodeAnalyst for Open Source
// Copyright 2002 . 2007 Advanced Micro Devices, Inc.
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


#ifndef _GRAPHVIEW_H
#define _GRAPHVIEW_H

#include <qrect.h>
#include <q3valuelist.h>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <Q3PopupMenu>

#include "stdafx.h"
#include "atuneoptions.h"
#include "tbsreader.h"
#include "symbolengine.h"
#include "sessionnav.h"

// Structures & Defines
enum Graph_Enum_Type
{
	MAX_ELEMENTS =	1000,
	GM_COLORS = 0,
	GM_ALL,
	GM_DATA
};


//A little more than a struct, but less than a class.
class GraphItem
{
public:
	GraphItem();

	// QSortList mandates that the < operator be defined
	bool operator< ( GraphItem item );

	// QSortList mandates that the == operator be defined
	bool operator== ( GraphItem item );

public:
	QString	label;
	VADDR		address;
	SampleDataMap *pCpuSamples;	// assigned from caller, don't release;
	QRect		rect;
	unsigned long dataToSort;
};

/////////////////////////////////////////////////////////////////////////////
// GraphArea class

class GraphArea : public QWidget
{
	Q_OBJECT
public:
	GraphArea (ViewShownData *pViewShown,  QWidget * parent );
	virtual ~GraphArea();
	bool initialize (QString title);
	
	void addGraphItem(GraphItem *gItem);
	void clearData();
	void scaleArea();

	int getMinWidth();
	int recalculateHeight (int width);
	void setTotal(SampleDataMap* pTotal) {
		m_pTotalSampleDataMap = pTotal;
	}
	void setNumCpus(unsigned int numCpus) {
		m_numCpus = numCpus;	
	}

private:
	void drawTitle ( QPainter * painter );
	int helpDrawBar (QPainter * painter, QRect & rect, 
				int w, int x, int y, int index, QColor color);
	void drawDataBars ( QPainter * painter);
	void drawTickMarks ( QPainter * painter );
	void drawAxes ( QPainter * painter );
	void drawLegend ( QPainter * painter );
	void draw ( QPaintEvent * event );
	int calculateLegendRows (int width);
	void calculateLabelWidth();

	void attributeTotalToColumns(SampleDataMap *pDataMap, DataArray &data);
	void dataComplexCalc(SampleDataMap *pDataMap, DataArray &data);

public slots:
	void onDisplay ( int id );
	void onSwitchToDataView();
	void OnManageColors ();
	void onViewChanged (ViewShownData* pShownData);

protected:
	void getComplexColumnsMap();
	void paintEvent ( QPaintEvent * pEvent );
	void mousePressEvent ( QMouseEvent * e );
	void mouseDoubleClickEvent ( QMouseEvent * e );

signals:
	void graphItemDblClicked (QString, VADDR);
	void viewDataTab();

private:
	unsigned int		m_max_cum_val;
	int			m_max_txt_width;
	int			m_legend_height;
	int			m_one_legend_width;
	int			m_start_index;
	int			m_stop_index;
	int			m_num_displayed;
	int			m_num_elements;
	UINT			m_element_height;
	QString			m_title;
	QRect			m_title_rect;

	Q3PopupMenu		*m_popup;
	int			m_popup_id[GM_DATA + 1];
	int			m_prev_id;
	Q3ScrollView 		*m_scroll_view;
	QPainter 		*m_painter;
	ColorList		m_colorList;
	Q3SortedList<GraphItem>	m_data;
	ViewShownData		*m_pViewShown;
	bool			bSeparateTasks;
	
	SampleDataMap		*m_pTotalSampleDataMap;
	DataArray		m_totalData;
	Q3ValueList<int>		m_ComplexColumnIndexList;
	unsigned int 		m_numCpus;
}; //class GraphArea

/////////////////////////////////////////////////////////////////////////////
class SysGraphView : public DataTab
{
	Q_OBJECT
public:
	SysGraphView (ViewShownData *pViewShown, QWidget * parent, 
		const char* name, int wflags );
	virtual ~SysGraphView() ;

	bool init (TbsReader * tbp_file);

protected:
	void resizeEvent ( QResizeEvent * event );

private:
	void helpResizeArea();
	bool readSysDataSection();

public slots:
	void onModuleDblClicked ( QString label, VADDR );

	void onViewChanged (ViewShownData* pShownData);

	void onViewSysData();

signals:
	void viewSysData    ( );
	void moduleDblClicked ( QString module_name, TAB_TYPE type, 
					unsigned int taskId );

private:
	Q3ScrollView * m_scroll_view;
	GraphArea   * m_graph_area;
	TbsReader 	* m_tbp_file;

	Q3ValueList<SYS_LV_ITEM> m_mod_list;
	map<unsigned int, QString> m_taskId_name_map;
	SampleDataMap m_totalSampleDataMap;
};


/////////////////////////////////////////////////////////////////////////////
class ModGraphView : public DataTab
{
	Q_OBJECT
public:
	ModGraphView (ViewShownData *pViewShown, QWidget * parent, 
		const char* name, int wflags );
	~ModGraphView() ;

	bool init (TbsReader * tbp_file, 
		char * module_name,
		TaskSampleMap * pTaskSampMap, 
		JitBlockPtrVec * pJitBlkPtrVec,	
		unsigned int taskId = SHOW_ALL_TASKS);

	bool tbsDisplayProgress_callback(const char * msg);

private:
	bool readTaskInfo(TbsReader * tbp_file);
	bool readInstSamples(TbsReader *tbp_file);
	bool AddSamplesIntoGraph();
	bool drawElfGraphItems();
	bool drawJavaGraphItems();

	bool initModuleToolBar (TbsReader * tbp_file, char * pModName, 
					unsigned int taskId);

	void resizeEvent ( QResizeEvent * event );

	void helpResizeArea();


public slots:
	void onSymDblClicked ( QString label, VADDR address);

	void onViewModuleData ();

	void onViewChanged (ViewShownData* pShownData);
	void OnSelectTask (int index);

signals:
	void symDblClicked ( VADDR address, 
			SampleMap *Samp_map, ProfileAttribute prof_att);

	void viewModuleData ( QString module_name, TAB_TYPE type );

private:
	Q3ScrollView		*m_scroll_view;
	GraphArea		*m_graph_area;
	Q3ToolBar		*m_pTaskToolbar;
	QComboBox		*m_pTaskIdBox;
	TaskSampleMap		*m_pTaskSampMap;
	JitBlockPtrVec 		*m_pJitBlkPtrVec;	
	ProfileAttribute	m_prof_att;
	unsigned int		m_taskId;
	QString			m_moduleName;
	TbsReader		*m_tbp_file;

	map<QString,unsigned int> m_taskName_id_map;
	map<unsigned int, QString> m_taskId_name_map;

	Q3ProgressDialog		*m_pProgressDlg;
	/* Number of the next threshold to actually update the gui */
	unsigned int 		m_display_progress_threshold;
	/* Number of times the callback has been called */
	unsigned int 		m_display_progress_calls;
	SampleDataMap 		m_totalSampleDataMap;
};



#endif // not _GRAPHVIEW_H
