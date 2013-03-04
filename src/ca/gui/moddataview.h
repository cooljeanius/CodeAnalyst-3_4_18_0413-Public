//interface for the ModuleDataView class.

/*
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
*/

#ifndef _MODDATAVIEW_H
#define _MODDATAVIEW_H

#include <pthread.h>

#include "stdafx.h"
#include "CaProfileReader.h"
#include "symbolengine.h"
#include "sessionnav.h"
#include "bbanalysis.h"

#include <Q3PopupMenu>

struct InlineData {
	QString inlineFuncName;	// this is the name of inline function
	QString functionName;	// this is the fucntion called inline function;
	VADDR functionAddr;		// this is the fucntion address 
	VADDR inlineInstAddr;		// this is the inline instance address 
	VADDR inlineInstEndAddr;	// this is the inline instance address 
	VADDR sampleAddr;
	AggregatedSample samp;
	PID_T pid;
	TID_T tid;
};

typedef map<VADDR, InlineData> InlineDataMap;

class ModuleItem;

//////////////////////////////////////////////////////////////////////////////
enum MODULE_COLUMNS {
	MOD_ADDR = 0,
	MOD_SYMBOL,
	MOD_PID,
	MOD_TID,
	MOD_OFFSET_INDEX,
	MOD_INVALID
};

enum MOD_POPUPS {
	MOD_POP_DATA = 1,
	MOD_POP_SHOWN
};


class ModuleItem : public DataListItem {
public:

	enum ModItemType{
		Mod_UnknownItem = 0,
		Mod_SampItem = 1,
		Mod_FuncItem = 2,
		Mod_IlItem = 4,
		Mod_InstItem = 8,
		Mod_DummyItem = 0x10,
		Mod_BbItem = 0x20,
		Mod_hasDummy = 0x40,
	};

	ModuleItem (ViewShownData *pViewShown, int indexOffset, 
		Q3ListView * parent, Q3ListViewItem * after);
	ModuleItem (ViewShownData *pViewShown, int indexOffset, 
		Q3ListViewItem * item, Q3ListViewItem * after);
	virtual ~ModuleItem() {};

protected:
	QString key (int column, bool ascending) const;

public:
	unsigned int type;
	sym_info sym;
};

//////////////////////////////////////////////////////////////////////////////
// ModuleDataTab Class
//

class ModuleDataTab:public DataTab {
	Q_OBJECT
public:
	ModuleDataTab (const CA_Module * pMod, 
		CaProfileReader * pProfReader,
		ViewShownData *pViewShown, 
		QWidget * parent,
		const char *name, 
		Qt::WindowFlags wflags = 0);

	virtual ~ModuleDataTab ();
	
	bool initialize (unsigned int pid);

private:
	void calculateTotalData(int type);
	void calculateTotalForSystem();
	void calculateTotalForModule();
	bool addItemsAndDisplayData();
	void setColumns();
	bool readAndAggregateData();
	int initSymbolEngine(QString moduleName);
	int initBba();

	ModuleItem* AddFirstLvFnItem(ModuleItem *pAfter);

	ModuleItem* AddFirstLvBBFnItem(
		VADDR addr, 
		ModuleItem *pAfter,
		BLOCKMAP::iterator &bbIter, 
		BLOCKMAP::iterator bbEnd,
		bfd_vma imageBase);

	bool DisplayData ();
	bool DisplayFunction();
	bool DisplayIL();
	bool DisplayBasicBlocks();

	void setupPidComboBox();
	void setupTidComboBox();
	void setupAggregationComboBox();
	void setupModuleToolbar();
	void setupPercentageToolbar();
	void setupListView();
	void showPidTidCol(bool b);
	bool isSeparatedPidTid() { return (m_pSepPidTid->isOn()); };
	QString UpdateMemoryAccess(sym_info &secondLvSym, 
			MEMACCESSMAP::iterator &memAccIter, 
			MEMACCESSMAP::iterator memAccEnd,
			bfd_vma imageBase);

	void helpDisplayInlineFunction();
	void helpFillSampleItems(ModuleItem * pFirstLvItem);
	void helpRemoveDummyItem(ModuleItem* pItem);
	void helpAggSamplesFromLv2(ModuleItem* pItem, AggregatedSample &data);
	ModuleItem * helpFindItemForFunction(QString funcName); 



	QString 	m_module_name;
	Q3PopupMenu 	*m_popup;
	SymbolEngine 	m_symbol_engine;

	CaProfileReader *m_pProfReader;
	CaProfileInfo	*m_pProfInfo;
	const CA_Module	*m_pMod;

	PID_T		m_pid;
	TID_T		m_tid;
	unsigned int	m_pidColWidth;
	unsigned int	m_tidColWidth;
	QPushButton 	*m_pExpandCollapse;
	Q3ToolBar 	*m_pToolbar;
	Q3ToolBar 	*m_pModAggregationToolbar;
	Q3ToolBar 	*m_pPercentToolbar;
	QComboBox 	*m_pPidSel;
	QComboBox 	*m_pTidSel;
	QComboBox 	*m_pAggregationId;
	QComboBox 	*m_pPercentSel;
	QPushButton 	*m_pSepPidTid;

	UINT		m_dataAggType;
	UINT		m_showAggCtrl;
	BBAnalysis 	m_bba;
	bool 		m_bbaInit;
	InlineDataMap	m_instMap;

	/* Number of the next threshold to actually update the gui */
	unsigned int 	m_display_progress_threshold;
	/* Number of times the callback has been called */
	unsigned int 	m_display_progress_calls;

private slots:
	void onDblClicked (Q3ListViewItem * item);
	void onRightClick (Q3ListViewItem * item, const QPoint & pt, int col);
	void onSelectionChange ();
	void onSelectPid (int index);
	void onSelectTid (int index);
	void onSeparatePidTidToggled(bool b);
	void onExpandCollapseToggled(bool b);
	void onAggregationChanged(int aggregationType);
	void onPercentageChanged(int type);
	void onExpandFirstLvItemBB(Q3ListViewItem * pItem);
	void onExpandFirstLvItemFunction(Q3ListViewItem * pItem);
	void onExpandItemForIL(Q3ListViewItem * pItem);

public slots:
	virtual void onViewChanged (ViewShownData * pShownData);
	void onPidChanged(unsigned int pid);

signals:
	void symDblClicked (VADDR address,
			PID_T pid, TID_T tid,
			const CA_Module * pMod);
};

#endif // #ifndef _MODDATAVIEW_H
