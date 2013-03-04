
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

#ifndef _DIFFSESSION_H_
#define _DIFFSESSION_H_

#include <qlistview.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qframe.h>
#include <qmap.h>
#include <qtoolbutton.h>
#include <qtoolbar.h>

#include "stdafx.h"
#include "diffStruct.h"
#include "tbsreader.h"
#include "symbolengine.h"
#include "DiffDataListItem.h"
#include "DiffViewShownData.h"

class DiffAnalystWindow;
///////////////////////////////////
// Menu ID's under the Tools Menu

typedef struct _INLINE_BLOCK
{
	QString name;
	bool 	isInline;
	_INLINE_BLOCK(QString n = "", bool b = false)
	{
		name = n;
		isInline = b;	
	};
} INLINE_BLOCK;

typedef map<bfd_vma, INLINE_BLOCK> 	INLINE_BLOCK_MAP;
typedef QValueList<SYS_LV_ITEM>	PROC_ID_LIST;	

class DiffSession : public QMainWindow
{
	Q_OBJECT;
public:

	enum {
	    TM_LEFTRIGHT= 0,
	    TM_SIDEBYSIDE,
	    TM_DELTA,
	    TM_SEPARATECPU,
	    TM_PERCENT,
	    TM_INLINE,
	    TM_LAST
	};

	DiffSession(QWidget * parent = 0, 
			const char * name = 0, 
			WFlags f = WDestructiveClose);
	~DiffSession();
	bool init(SESSION_DIFF_INFO_VEC &diffInfoVec);
	
	SESSION_DIFF_INFO_VEC* getSessionDiffInfo()
	{ return &m_diffInfoVec; };

	DIFF_VIEW_OPTIONS* getSessionViewOptions()
	{ return &m_diffViewOptions; };

	void setSessionViewOptions(DIFF_VIEW_OPTIONS *ops)
	{ m_diffViewOptions = *ops; };

	bool populateDataToSymbolView();
		
	ViewShownData* getViewShown(unsigned int index)
	{ return &(m_viewShown[index]);};

	SymbolEngine* getSymbolEngine(unsigned int index)
	{ return &(m_symEng[index]);};

	SampleMap* getSampleMap(unsigned int index)
	{ return &((m_taskSampleMap[index])[m_taskId[index]]);};

	QListViewItem* getCurrentSymbol()
	{ 
		if(m_pSymbolLV)
			return m_pSymbolLV->currentItem();
		else
			return NULL;	
	};

	QString getSessionStr(unsigned int index);

	bool isSameModule();

	unsigned int getNumDiffSession()
	{
		return m_diffInfoVec.count();
	};
	
	void setDawName(QString dawName)
	{ m_dawName = dawName; };

	QString getDawName()
	{ return m_dawName; };

	bool isAggregateInline()
	{ return m_aggregateInline; };

protected:	
	virtual void closeEvent(QCloseEvent *e);

private slots:
	void onPrepareToUpdateDasmDockView(QListViewItem *i)
	{ emit updateDasmDockView(i); };

	void onRightClicked(QListViewItem* item,
				const QPoint &pt,
				int col);
	
	void onMergeRow();

	void onClickedColumnArrangement();
	void onClickedLeftRight();
	void onClickedSideBySide();
	void onClickedDelta();
	void onTogglePercent(bool b);
	void onSeparateCpu(bool b);
	void onAggregateInline(bool b);

private:
	bool initViewToolBar();

	bool getInfoFromEbpFile ( TaskSampleMap* pTaskSampleMap,
				QString sessionFile, 
				QString task,
				QString module,
				unsigned int diffIndex);
	
	bool aggregateSamplesForModule( unsigned int diffIndex,
				TaskSampleMap* pTaskSampleMap,
				QString module);
	
	void aggregateSamplesForSymbol( SampleDataMap& pSampleDataMap,
					SampleDataMap& pSampleDataMapDest);

	void aggregateTotalSamples( SampleDataMap& pSampleDataMap, 
				unsigned int diffIndex);

	void sumAllCpusInSampleDataMap(SampleDataMap* pSdm);
	

	bool initViewCfg(unsigned int diffIndex);
	
	void buildInlineBlockMap(int diffIndex, bfd_vma funcStartAddr);
	void getInlineBlockMap(int diffIndex,
				bfd_vma addr,
				QString &name, 
				bool &flag, 
				bfd_vma &prev, 
				bfd_vma &next);
	
	void calculateSamplePercentMap(SamplePercentMap* percent, 
				SampleDataMap* samples,
				SampleDataMap* total);

	DiffAnalystWindow	*m_pDiffAnalystWindow;
	// SessionDiffInfo
	QString			m_dawName;
	SESSION_DIFF_INFO_VEC 	m_diffInfoVec;
	DIFF_VIEW_OPTIONS	m_diffViewOptions;
	MAX_ENTRY_MAP		m_maxEntMap;

	// Symbol stuff
	PROC_ID_LIST		m_procIdList[MAX_NUM_DIFF];
	int			m_taskId[MAX_NUM_DIFF];
	SYMBOL_VIEW_INFO_MAP 	m_symViewInfoMap;
	SymbolEngine		m_symEng[MAX_NUM_DIFF];
	QListView 			*m_pSymbolLV;
	unsigned int 		m_numDiff;
	TaskSampleMap		m_taskSampleMap[MAX_NUM_DIFF];
	SampleDataMap		m_moduleTotalMap[MAX_NUM_DIFF];
	
	TbsReader		*m_pTbsReader[MAX_NUM_DIFF];
	INLINE_BLOCK_MAP	m_inlineBlockMap[MAX_NUM_DIFF];
	bool			m_aggregateInline;

	// View stuff
	bool 			m_alreadyCheckedTime[MAX_NUM_DIFF];
	DiffViewShownData 	*m_pDiffViewShown;
	ViewShownData 		m_viewShown[MAX_NUM_DIFF];

	// Toolbar
	QToolBar		*m_pSymViewToolbar;	
	QPushButton 		*m_pToolId[TM_LAST];
	
	
signals:
	void updateDasmDockView(QListViewItem *i);

};


#endif //_DIFFSESSION_H_

