
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

#include <qmessagebox.h>
#include "DasmDockView.h"
#include "DiffDataListItem.h"
#include "bfd.h"
#include "DasmDataListItem.h"
#include "stdafx.h"
#include <qiconset.h>
#include <qpixmap.h>
#include <qtooltip.h>

#define _DIFFANALYST_ICONS_
#include "../gui/xpm.h"
#undef  _DIFFANALYST_ICONS_

#define DADSM_DEBUG 0

DasmDockView::DasmDockView(QWidget * parent, 
			const char * name , 
			WFlags f)
: QDockWindow(parent,name,f)
{
	m_pBLayout = NULL;
	m_pSplitter = NULL;
	m_pDiffSession = NULL;
	m_pDasmToolbar = NULL;
	m_pMainWindow = NULL;
	m_syncDasmView = false;
	m_isMoving = false;
	m_pPercentCombo = NULL;

	//Dasm ToolButtons
	m_pCodeByte = NULL;
	m_pLoadStore = NULL;
	m_pBBDiff   = NULL;
	
	for(int i=0 ; i<MAX_NUM_DIFF; i++)
	{
		m_pView[i] = NULL;
		m_pBBMap[i] = NULL;
		m_pMemMap[i] = NULL;
		m_pTotalSampleDataMap[i] = NULL;
		m_pCurHighlightedBB[i] = NULL;
	}
	
	setNewLine(true);
	setResizeEnabled(true);
	setCaption(QString("Dasm Diff : [ ]"));
}

DasmDockView::~DasmDockView()
{
	// Do no delete this.
	m_pDiffSession = NULL;

	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
		m_bba[i].bba_cleanup();
}

bool DasmDockView::init()
{
	// Setup docking property
	bool ret=false;

	// Setup size when doc
	setFixedExtentHeight(200);

	// Get layout
	m_pBLayout = boxLayout();

	//Setup MainWindow
	m_pMainWindow = new QMainWindow(this,"", Qt::WStyle_Customize | Qt::WStyle_NoBorder );
	if(!m_pMainWindow) return ret;	
	m_pMainWindow->setDockMenuEnabled(false);
	m_pBLayout->addWidget(m_pMainWindow);

	//Set up Toolbar
	initDasmToolbar();

	//Set up Splitter
	m_pSplitter = new QSplitter(m_pMainWindow);
	if(m_pSplitter == NULL) return false;

	m_pMainWindow->setCentralWidget(m_pSplitter);

	// Setting up ListView
	for(int i = 0; i < MAX_NUM_DIFF; i++)
	{
		m_pView[i] = new QListView(m_pSplitter);
		if(m_pView[i] == NULL) return false;

		m_pView[i]->setRootIsDecorated(true);
		m_pView[i]->setAllColumnsShowFocus(true);
		m_pView[i]->setShowSortIndicator(true);

		//SYNC
		connect(m_pView[i], SIGNAL(selectionChanged(QListViewItem*)),
			this, SLOT(onSelectionChanged(QListViewItem*)));
		connect(m_pView[i], SIGNAL(expanded(QListViewItem*)),
			this, SLOT(onExpanded(QListViewItem*)));
		connect(m_pView[i], SIGNAL(collapsed(QListViewItem*)),
			this, SLOT(onCollapsed(QListViewItem*)));
	}

	// TODO: This is hardcoded. Need to write new algorithm 
	connect(m_pView[0], SIGNAL(contentsMoving(int,int)),
		this, SLOT(onContentsLeftMoving(int,int)));
	connect(m_pView[1], SIGNAL(contentsMoving(int,int)),
		this, SLOT(onContentsRightMoving(int,int)));

	ret = true;
	return ret;
}

bool DasmDockView::initDasmToolbar()
{
	m_pDasmToolbar = new QToolBar(m_pMainWindow);
	m_pDasmToolbar->setMovingEnabled(false);
	m_pDasmToolbar->setLabel("Dasm View Control Toolbar");
	m_pDasmToolbar->boxLayout()->setSpacing(5);

	QIconSet icon;

	// CodeByte
	icon = QIconSet (QPixmap((const char**)codeByteIcon));
	m_pCodeByte = new QPushButton(icon, QString(""),m_pDasmToolbar);
	m_pCodeByte->setMaximumHeight(20);
	m_pCodeByte->setMaximumWidth(20);
	m_pCodeByte->setToggleButton(true);
	connect(m_pCodeByte,SIGNAL(toggled(bool)), 
		this,SLOT(onCodeByteToggled(bool)));
	m_pCodeByte->setToolTip(QString("Show code byte"));
	
	
	// Inline
	icon = QIconSet (QPixmap((const char**)inlineIcon));
	m_pInline= new QPushButton(icon, QString(""),m_pDasmToolbar);
	m_pInline->setMaximumHeight(20);
	m_pInline->setMaximumWidth(20);
	m_pInline->setToggleButton(true);
	connect(m_pInline,SIGNAL(toggled(bool)), 
		this,SLOT(onInlineToggled(bool)));
	m_pInline->setToolTip(QString("Show inline instruction"));
	
	
	// Load/Store
	icon = QIconSet (QPixmap((const char**)loadStoreIcon));
	m_pLoadStore = new QPushButton(icon, QString(""),m_pDasmToolbar);
	m_pLoadStore->setMaximumHeight(20);
	m_pLoadStore->setMaximumWidth(20);
	m_pLoadStore->setToggleButton(true);
	connect(m_pLoadStore,SIGNAL(toggled(bool)), 
		this,SLOT(onLoadStoreToggled(bool)));
	m_pLoadStore->setToolTip(QString("Show load/store"));
	
	// Expand/Collapse
	icon = QIconSet (QPixmap((const char**)expandCollapseIcon));
	m_pExpandCollapse = new QPushButton(icon, "",m_pDasmToolbar);
	m_pExpandCollapse->setMaximumHeight(20);
	m_pExpandCollapse->setMaximumWidth(20);
	m_pExpandCollapse->setToggleButton(true);
	connect(m_pExpandCollapse,SIGNAL(toggled(bool)), 
		this,SLOT(onExpandCollapseToggled(bool)));
	m_pExpandCollapse->setToolTip(QString("Expand / Collapse all basic-block"));
	

	// Sync
	icon = QIconSet (QPixmap((const char**)lockIcon));
	m_pSyncDasmView = new QPushButton(icon,"",m_pDasmToolbar);
	m_pSyncDasmView->setMaximumHeight(20);
	m_pSyncDasmView->setMaximumWidth(20);
	m_pSyncDasmView->setToggleButton(true);
	connect(m_pSyncDasmView,SIGNAL(toggled(bool)), 
		this,SLOT(onSyncDasmViewToggled(bool)));
	m_pSyncDasmView->setToolTip(QString("Synchronize diassembly view"));

	
	// Block Diff
	icon = QIconSet (QPixmap((const char**)BBDiffIcon));
	m_pBBDiff = new QPushButton(icon, "",m_pDasmToolbar);
	m_pBBDiff->setMaximumHeight(20);
	m_pBBDiff->setMaximumWidth(20);
	m_pBBDiff->setToggleButton(true);
	connect(m_pBBDiff,SIGNAL(toggled(bool)), 
		this,SLOT(onBBDiffToggled(bool)));
	m_pBBDiff->setToolTip(QString("Highlight different instructions in basic-block"));
	
//	m_pDasmToolbar->addSeparator();
	
	// Percent
	m_pPercentCombo = new QComboBox(m_pDasmToolbar);
	m_pPercentCombo->setMaximumHeight(20);
	m_pPercentCombo->insertItem(" Raw Samples ");
	m_pPercentCombo->insertItem(" Function Percentage ");
	m_pPercentCombo->insertItem(" Basic Block Percentage ");
	connect(m_pPercentCombo,SIGNAL(activated(int)), 
		this,SLOT(onPercentChanged(int)));
	
	return true;	
}

void DasmDockView::resetToolbar()
{
//	m_pCodeByte->setOn(false);
//	m_pInline->setOn(false);
//	m_pLoadStore->setOn(false);
	m_pExpandCollapse->setOn(false);
	m_pSyncDasmView->setOn(false);
	m_pBBDiff->setOn(false);
//	m_pPercentCombo->setCurrentItem(0);
}

void DasmDockView::onDasmDockViewShowChanged(bool b)
{
//	fprintf(stderr,"DEBUG: DasmDockView::onDasmDockViewShowChanged: b = %s\n",((b)?"true":"false"));

	if(b)
	{
		this->show();
	}else{
		this->hide();
	}

}

bool DasmDockView::close(bool alsoDelete)
{
	Q_UNUSED(alsoDelete);
	// DO NOT CLOSE, just hide it
//	fprintf(stderr,"DEBUG: DasmDockView::close\n");
	this->hide();
	emit dasmDockViewNoShow();
	return false;
}

void DasmDockView::onChangeSession(DiffSession *s)
{
	m_pDiffSession = s;
	if(m_pDiffSession == NULL)
		return;

	//------------------------------------------------

	// Set Caption
	setCaption(QString("Dasm Diff : [") + m_pDiffSession->name() + "]");

	// Init Basic-Block Analysis module
	SESSION_DIFF_INFO_VEC* vec = m_pDiffSession->getSessionDiffInfo();
	SESSION_DIFF_INFO_VEC::iterator it = vec->begin();
	SESSION_DIFF_INFO_VEC::iterator end = vec->end();
	for(int i = 0; it != end && i < MAX_NUM_DIFF ; i++, it++)
	{
		// Initialize BBA
		m_bba[i].bba_cleanup();

//		fprintf(stderr,"Module = %s\n",	(*it).module.data());
		if(m_bba[i].bba_init((*it).module.data()) == OK)
		{
			if(m_bba[i].bba_analyze_all() == OK)
			{
				m_pBBMap[i] = m_bba[i].bba_get_block_map();
				m_pMemMap[i] = m_bba[i].bba_get_mem_map();
/*
{//DEBUG
			BLOCKMAP::iterator b_it = m_pBBMap[i]->begin();
			BLOCKMAP::iterator b_end = m_pBBMap[i]->end();
			for(; b_it != b_end ; b_it++)
			{
				fprintf(stderr,"DEBUG: BBA start offset= %llx\n",b_it->first.bb_start);
			}
}
*/
			}
		}
	}

	//------------------------------------------------
	// If diff modules are the same, allow dasm sync
//	m_pSyncDasmView->setEnabled( m_pDiffSession->isSameModule() );

	//------------------------------------------------
	// Update DasmDockView
	onUpdateDasmSymbol(m_pDiffSession->getCurrentSymbol());
}

void DasmDockView::addColumn(int diffIndex)
{
	// Col 0
	m_pView[diffIndex]->addColumn("Address",100);

	// Col 1
	m_pView[diffIndex]->addColumn("Basic Block / Disassembly", 200);

	// Col 2
	m_pView[diffIndex]->addColumn("Code Byte", 100);
	m_pView[diffIndex]->setColumnAlignment(DASM_CODEBYTE_COL, Qt::AlignRight);

	// Col 3
	m_pView[diffIndex]->addColumn("In-line Function", 100);
	m_pView[diffIndex]->setColumnAlignment(DASM_INLINE_COL, Qt::AlignRight);
	
	// Col 4
	m_pView[diffIndex]->addColumn("Load",50);
	m_pView[diffIndex]->setColumnAlignment(DASM_LOAD_COL, Qt::AlignRight);
	
	// Col 5
	m_pView[diffIndex]->addColumn("Store",50);
	m_pView[diffIndex]->setColumnAlignment(DASM_STORE_COL, Qt::AlignRight);

	// DataColumn
	ViewShownData * pViewShownData = m_pDiffSession->getViewShown(diffIndex);
	QStringList::iterator it = pViewShownData->available.begin();
	QStringList::iterator end = pViewShownData->available.end();
	for(int colCount=0,j=0; it != end; j++, it++)
	{

		// Check if event should be shown
		bool isColShown = false;
		IndexVector::iterator s_it = pViewShownData->shown.begin();
		IndexVector::iterator s_end = pViewShownData->shown.end();
		for(; s_it != s_end ; s_it++)
		{
			if((*s_it) == j)
			{
				isColShown = true;
				break;
			}
		}

		if(isColShown == false)
		{
			continue;
		}

		m_pView[diffIndex]->addColumn(*it, 100);
		m_pView[diffIndex]->setColumnAlignment(colCount + DASM_OFFSET_COL, Qt::AlignRight);
		colCount++; 
		
	}

	setHiddenColumn();
}

void DasmDockView::onUpdateDasmSymbol(QListViewItem* i)
{
	resetToolbar();

	DiffDataListItem *item = (DiffDataListItem*) i;

//	fprintf(stderr,"DEBUG: onUpdateDasm: symbol = %s\n",item->getSymbolViewInfo()->symName.data()); 
	
	// Get Basic-Block and Dasm Information
	for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
	{
		// Sanity check
		if(m_pBBMap[i] == NULL)
			break;

		// Clear old stuff
		m_bbViewInfoMap[i].clear();
		m_dasmViewInfoMap[i].clear();
		m_pView[i]->clear();
		int numCol = m_pView[i]->columns();
		for (int j = numCol ; j >= 0; j--)
		{
			m_pView[i]->removeColumn(j);
		}
		
		if(item == NULL)
		{
			continue;
		}

		//------------------------------------------------
		//Get TaskSampleMap
		SampleMap *pSm = m_pDiffSession->getSampleMap(i);
		SampleMap::reverse_iterator smRit  = pSm->rbegin();
		SampleMap::reverse_iterator smRend = pSm->rend();

		//------------------------------------------------
		// Set new Column
		addColumn(i);

		//------------------------------------------------
		// Get SYMBOL_VIEW_INFO Ptr
		SYMBOL_VIEW_INFO* pSymViewInfo = item->getSymbolViewInfo();

		//------------------------------------------------
		// Get Symbol Total Sample Data Map Ptr
		if(pSymViewInfo->isInlineInstance)
			m_pTotalSampleDataMap[i] = &(pSymViewInfo->pCallerFunc->sampleDataMapTotal[i]);
		else
			m_pTotalSampleDataMap[i] = &(pSymViewInfo->sampleDataMap[i]);
		
		//------------------------------------------------
		// Get SymbolEngine Ptr
		SymbolEngine * pSymEng = m_pDiffSession->getSymbolEngine(i);

		//------------------------------------------------
		// Get Inlines
		m_inlineInfoVec[i].clear();
		pSymEng->getInlinesForFunctionDwarf(pSymViewInfo->startAddr[i],m_inlineInfoVec[i]);	
		SYM_INFO_VECTOR::iterator it_inline = m_inlineInfoVec[i].begin();
		SYM_INFO_VECTOR::iterator end_inline = m_inlineInfoVec[i].end();

#if DADSM_DEBUG
		for(; it_inline != end_inline; it_inline++)
		{
			fprintf(stderr,"DiffIndex = %d, inline startaddr = %lx, stopaddr = %lx, function = %s\n",
					i,
					(*it_inline)->sym_start,
					(*it_inline)->possible_end,
					(*it_inline)->name.data());
			
		}
		it_inline = m_inlineInfoVec[i].begin();
#endif
		//------------------------------------------------
		// PROCESSING BASICBLOCK	
		// Build BB key
		BASIC_BLOCK_KEY beginKey;
		BASIC_BLOCK_KEY endKey;
		beginKey.bb_start =  pSymViewInfo->startAddr[i] - m_bba[i].bba_get_image_base();
		endKey.bb_start   =  pSymViewInfo->stopAddr[i]  - m_bba[i].bba_get_image_base();

		// Search for ending basic-block of this symbol
		BLOCKMAP::iterator b_it = m_pBBMap[i]->upper_bound(endKey);
		if( b_it == m_pBBMap[i]->end())
		{
			fprintf(stderr,"ERROR: END BBA with address %lx not found\n",
				pSymViewInfo->stopAddr[i]);	
			return;
		}

		int bbIndex = 0;
		bfd_vma bAddr = 0;
		bfd_vma eAddr = pSymViewInfo->stopAddr[i];
			
		//-----------------------------------------------------		
		// We have the ending BB, now search for beginning BB
		while(b_it->first.bb_start >= beginKey.bb_start)	
		{
			bAddr = b_it->first.bb_start + m_bba[i].bba_get_image_base();

			//---------------------------------------------------
			// Generate map data
			QString bbName = QString("[ 0x") + QString::number(bAddr,16)
						+ " , 0x" + QString::number(eAddr,16)
						+ " )"; 
//			fprintf(stderr,"BBA %3d :offset= %lx, bbName = %s\n", 
//				bbIndex, b_it->first.bb_start, bbName.data());

			DASM_VIEW_INFO bbViewInfo;
			bbViewInfo.name 	= bbName;
			bbViewInfo.addr 	= bAddr;
			bbViewInfo.rowType 	= DASM_VIEW_INFO::BB_ROW;
			bbViewInfo.dataListItem = (DataListItem*) new BBDataListItem(
								m_pDiffSession->getViewShown(i),
								DASM_OFFSET_COL,
								m_pView[i]);

			// Insert basic block level to DASM_VIEW_INFO_MAP
			m_bbViewInfoMap[i].insert( DASM_VIEW_INFO_MAP::value_type(bAddr, bbViewInfo) );	
			
			//---------------------------------------------------
			// Get next BasicBlock
			if(b_it != m_pBBMap[i]->end())
			{
				eAddr = bAddr;
				bbIndex++;
				b_it++;
			} else {
				fprintf(stderr,"ERROR: BEGINNING BBA with address %lx not found\n",
							pSymViewInfo->startAddr[i]);	
				return;
			}
		}
		//---------------------------------------------------
		// PROCESSING DISASSEMBLY
		// Query disassmbly
		line_list_type dasmLineList;
		
		if( pSymViewInfo->startAddr[i] != 0 
		&&  pSymViewInfo->stopAddr[i]  != 0)
		{
			int ret = pSymEng->disassembleRange(pSymViewInfo->startAddr[i],
							pSymViewInfo->stopAddr[i],
							&dasmLineList);

			if ( SymbolEngine::OKAY != ret
			||   dasmLineList.count() == 0 ) 
			{
				//May have failed because of cancel
				QString temp = QString("Failed to disassemble range [ 0x") +
					QString::number(pSymViewInfo->startAddr[i],16) + " , 0x" +
					QString::number(pSymViewInfo->stopAddr[i],16) + " )";
		
				QMessageBox::information (this, "Disassembly error", temp);
				//return ;
			}
		}

		//---------------------------------------------------
		// For each line of disassembly in the function
		// NOTE: We traverse the dasmLineList backward (large address to small)
		//       since we also travese the SampleDataMap backward.
		//       This is a better algorithm for attribute samples. 
		//       This loop should behave like reverse_iterator 
		//       (not available in Qt)
		line_list_type::iterator bw_dasmIter = dasmLineList.end();
		line_list_type::iterator dasmBegin  = dasmLineList.begin();
		bw_dasmIter--;
		if(bw_dasmIter == dasmBegin)
			continue;
		
		while (1) 
		{
			//--------------------------------------------
			// Get instruction address
			VADDR tAddr = (VADDR) (*bw_dasmIter).address;
		
			//--------------------------------------------
			// Find BB with this address
			DASM_VIEW_INFO_MAP::iterator d_it = m_bbViewInfoMap[i].upper_bound(tAddr);
			if(d_it != m_bbViewInfoMap[i].begin())
				d_it--;

			if(d_it == m_bbViewInfoMap[i].end())
			{
				fprintf(stderr,"ERROR, Failed to map instruction: addr = %llx\n",tAddr);
				if(bw_dasmIter != dasmBegin) 
				{
					bw_dasmIter--;	
					continue;
				}else{
					break;
				}
			}

			//--------------------------------------------
			
			DASM_VIEW_INFO *pBBViewInfo = &(d_it->second);
			BBDataListItem *pBB = (BBDataListItem*)(pBBViewInfo->dataListItem);

			DASM_VIEW_INFO dasmViewInfo;
			dasmViewInfo.name = (*bw_dasmIter).disassembly;
			dasmViewInfo.addr = tAddr;
			dasmViewInfo.rowType = DASM_VIEW_INFO::DASM_ROW;
			dasmViewInfo.codeByte = (*bw_dasmIter).codebytes;
			dasmViewInfo.pParent = pBBViewInfo;
			dasmViewInfo.dataListItem = (DataListItem*) new DasmDataListItem(
								m_pDiffSession->getViewShown(i),
								DASM_OFFSET_COL,
								pBB);
		
			//--------------------------------------------
			// Query Memory access
			VADDR offSet = tAddr - m_bba[i].bba_get_image_base();
			MEMACCESSMAP::iterator m_it = m_pMemMap[i]->find(offSet);
			if(m_it != m_pMemMap[i]->end())
			{
				switch(m_it->second)
				{
					case MA_READ:
						dasmViewInfo.loadCnt++;
						pBBViewInfo->loadCnt++;
						break;
					case MA_WRITE:
						dasmViewInfo.storeCnt++;
						pBBViewInfo->storeCnt++;
						break;
					case MA_READWRITE:
						dasmViewInfo.loadCnt++;
						dasmViewInfo.storeCnt++;
						pBBViewInfo->loadCnt++;
						pBBViewInfo->storeCnt++;
						break;
					default:
						break;
					
				};
			}

			//---------------------------------------------
			// Check inline function
			it_inline = m_inlineInfoVec[i].begin();
			while( it_inline != end_inline )	
			{
				if( tAddr < (*it_inline)->sym_start )
				{
					break;
				} else if ((tAddr >= (*it_inline)->sym_start)
					&&  (tAddr <  (*it_inline)->possible_end))  
				{
						// Found the inline function
						dasmViewInfo.inlineName = (*it_inline)->name;
						pBBViewInfo->inlineName = (*it_inline)->name;
						break;	
				}else{
						// Increment the iterator and recheck
						it_inline++;	
				}	
			}

			//------------------------------------------------------
			// Attribute samples into each disassembly line.
			// Note: We need to account for the IBS-Fetch special case when
			//       sample address is not align with the instruction address.
			if(smRit != smRend)
			{
				while(smRit->first >= tAddr)
				{

					if(smRit->first <= pSymViewInfo->stopAddr[i])
					{
						aggregateSampleDataMap(&(dasmViewInfo.aggSampleDataMap),&(smRit->second));
					}
					
					smRit++;

					if(smRit == smRend)
					{
						break;	
					}
				}
			}

			//------------------------------------------------------
			// Store disassembly view info in map
			m_dasmViewInfoMap[i].insert(DASM_VIEW_INFO_MAP::value_type(tAddr,dasmViewInfo));
		
			// Aggregate samples from dasm into BB
			aggregateSampleDataMap( &(pBBViewInfo->aggSampleDataMap), 
						&(dasmViewInfo.aggSampleDataMap));

			if(bw_dasmIter != dasmBegin) 
			{
				bw_dasmIter--;	
				continue;
			}else{
				break;
			}
		}// end for each line of disasmbly in the function.
	}

	updateListViews();

	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{	
		m_pView[i]->setSorting(0);
		m_pView[i]->sort();
	}

} //DasmDockView::onUpdateDasmSymbol

void DasmDockView::updateListViewsWithFunctionPercent()
{
	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{
		DASM_VIEW_INFO_MAP::iterator bbIt = m_bbViewInfoMap[i].begin();
		DASM_VIEW_INFO_MAP::iterator bbEnd = m_bbViewInfoMap[i].end();
		for(; bbIt != bbEnd ; bbIt++)
		{
			SamplePercentMap percentMap;
			calculateSamplePercentMap(&percentMap, 
						&(bbIt->second.aggSampleDataMap),
						m_pTotalSampleDataMap[i]);
		
			bbIt->second.pSamplePercentMap = &percentMap;
			BBDataListItem *pBB = (BBDataListItem*) bbIt->second.dataListItem;
			pBB->drawData(&(bbIt->second),true);
		}

		DASM_VIEW_INFO_MAP::iterator dasmIt = m_dasmViewInfoMap[i].begin();
		DASM_VIEW_INFO_MAP::iterator dasmEnd = m_dasmViewInfoMap[i].end();
		for(; dasmIt != dasmEnd ; dasmIt++)
		{
			SamplePercentMap percentMap;
			calculateSamplePercentMap(&percentMap, 
						&(dasmIt->second.aggSampleDataMap),
						m_pTotalSampleDataMap[i]);
		
			dasmIt->second.pSamplePercentMap = &percentMap;
			DasmDataListItem *pDasm = (DasmDataListItem*) dasmIt->second.dataListItem;
			pDasm->drawData(&(dasmIt->second),true);
		}

	}
}

void DasmDockView::updateListViewsWithBBPercent()
{
	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{
		DASM_VIEW_INFO_MAP::iterator bbIt = m_bbViewInfoMap[i].begin();
		DASM_VIEW_INFO_MAP::iterator bbEnd = m_bbViewInfoMap[i].end();
		for(; bbIt != bbEnd ; bbIt++)
		{
			BBDataListItem *pBB = (BBDataListItem*) bbIt->second.dataListItem;
			pBB->drawData(&(bbIt->second),0);
		}

		DASM_VIEW_INFO_MAP::iterator dasmIt = m_dasmViewInfoMap[i].begin();
		DASM_VIEW_INFO_MAP::iterator dasmEnd = m_dasmViewInfoMap[i].end();
		for(; dasmIt != dasmEnd ; dasmIt++)
		{
			SamplePercentMap percentMap;
			calculateSamplePercentMap(&percentMap, 
						&(dasmIt->second.aggSampleDataMap),
						&(dasmIt->second.pParent->aggSampleDataMap));
		
			dasmIt->second.pSamplePercentMap = &percentMap;
			DasmDataListItem *pDasm = (DasmDataListItem*) dasmIt->second.dataListItem;
			pDasm->drawData(&(dasmIt->second),true);
		}

	}
}

void DasmDockView::updateListViewsWithRawData()
{
	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{
		DASM_VIEW_INFO_MAP::iterator bbIt = m_bbViewInfoMap[i].begin();
		DASM_VIEW_INFO_MAP::iterator bbEnd = m_bbViewInfoMap[i].end();
		for(; bbIt != bbEnd ; bbIt++)
		{
			BBDataListItem *pBB = (BBDataListItem*) bbIt->second.dataListItem;
			pBB->drawData(&(bbIt->second),0);
		}
		
		DASM_VIEW_INFO_MAP::iterator dasmIt = m_dasmViewInfoMap[i].begin();
		DASM_VIEW_INFO_MAP::iterator dasmEnd = m_dasmViewInfoMap[i].end();
		for(; dasmIt != dasmEnd ; dasmIt++)
		{
			DasmDataListItem *pDasm = (DasmDataListItem*) dasmIt->second.dataListItem;
			pDasm->drawData(&(dasmIt->second),0);
		}
	}
}

void DasmDockView::updateListViews()
{
	setHiddenColumn();

	switch (m_pPercentCombo->currentItem())
	{
	case 0:
		updateListViewsWithRawData();
		break;
	case 1:
		updateListViewsWithFunctionPercent();
		break;
	case 2:
		updateListViewsWithBBPercent();
		break;	
	default:
		updateListViewsWithRawData();
		break;	
	};
}

void DasmDockView::setHiddenColumn()
{
	// Hide Columns
	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{
		if(!m_pCodeByte->isOn())
		{
			m_pView[i]->hideColumn(DASM_CODEBYTE_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_CODEBYTE_COL);
		}
		
		if(!m_pInline->isOn())
		{
			m_pView[i]->hideColumn(DASM_INLINE_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_INLINE_COL);
		}
		
		if(!m_pLoadStore->isOn())
		{
			m_pView[i]->hideColumn(DASM_LOAD_COL);
			m_pView[i]->hideColumn(DASM_STORE_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_LOAD_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_STORE_COL);
		}
	
	}
	
}

void DasmDockView::onCodeByteToggled(bool b)
{
	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{
		if(b)
		{
			m_pView[i]->setColumnWidth(DASM_CODEBYTE_COL,100);
			m_pView[i]->header()->setResizeEnabled(true,DASM_CODEBYTE_COL);
		}else{
			m_pView[i]->hideColumn(DASM_CODEBYTE_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_CODEBYTE_COL);
		}
	}
}

void DasmDockView::onLoadStoreToggled(bool b)
{
	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{
		if(b)
		{
			m_pView[i]->setColumnWidth(DASM_LOAD_COL,50);
			m_pView[i]->setColumnWidth(DASM_STORE_COL,50);
			m_pView[i]->header()->setResizeEnabled(true,DASM_LOAD_COL);
			m_pView[i]->header()->setResizeEnabled(true,DASM_STORE_COL);
		}else{
			m_pView[i]->hideColumn(DASM_LOAD_COL);
			m_pView[i]->hideColumn(DASM_STORE_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_LOAD_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_STORE_COL);
		}
	}
}

void DasmDockView::onInlineToggled(bool b)
{
	for(int i = 0 ; i < MAX_NUM_DIFF; i++)
	{
		if(b)
		{
			m_pView[i]->setColumnWidth(DASM_INLINE_COL,100);
			m_pView[i]->header()->setResizeEnabled(true,DASM_INLINE_COL);
		}else{
			m_pView[i]->hideColumn(DASM_INLINE_COL);
			m_pView[i]->header()->setResizeEnabled(false,DASM_INLINE_COL);
		}
	}
}

void DasmDockView::onExpandCollapseToggled(bool b)
{
	for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
	{
		QListViewItem *pBB = m_pView[i]->firstChild();
		while(pBB != NULL)
		{
			m_pView[i]->setOpen(pBB,b);
			pBB = pBB->nextSibling();
		} 
		m_pView[i]->setContentsPos(0,0);
	}
	
}

void DasmDockView::onSyncDasmViewToggled(bool b)
{
	if(b)
	{
		//Store Difference
		m_xDiff = m_pView[1]->contentsX() - m_pView[0]->contentsX();
		m_yDiff = m_pView[1]->contentsY() - m_pView[0]->contentsY();
	}

	m_syncDasmView = b;
}

void DasmDockView::onBBDiffToggled(bool b)
{
	QListViewItem *pBB[MAX_NUM_DIFF] = {NULL};
	DasmDataListItem *pDasm[MAX_NUM_DIFF] = {NULL};
	bool doGetNextLine = true;

	// Get BB
	for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
	{
		pBB[i] = m_pView[i]->selectedItem();
		if(pBB[i] == NULL )
		{
			if(b)
				QMessageBox::critical(this,"DiffAnalyst Error", "There is no basic-block to compare.\n");
			m_pBBDiff->setOn(false);
			return;
		}
		pDasm[i] = (DasmDataListItem*) pBB[i]->firstChild();
		if(pDasm[i] == NULL && b)
		{
			QMessageBox::critical(this,"DiffAnalyst Error", "Selected items are not Basic-Block. Cannot diff.\n");
			m_pBBDiff->setOn(false);
			return;
		}
	
		// Save selected BB to be unhighlighted when unselected
		if(b)
		{
			m_pCurHighlightedBB[i] = (BBDataListItem*)pBB[i];
		}else{
			m_pCurHighlightedBB[i] = NULL;
		}
	}

	// For each row in BB	
	while( doGetNextLine)
	{
		bool isDifferent = false;
		QString curCB = "";
		// Compare each row of each session
		for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
		{
			// If no disassembly to compare, Highlight the others
			if(pDasm[i] == NULL)
			{
				isDifferent = true;
				break;
			}

			// Check if this is First session
			if(!curCB.isEmpty())
			{
				if( curCB.compare(pDasm[i]->text(DASM_CODEBYTE_COL)) != 0)
				{
					isDifferent = true;
					break;
				}
			}else{
				// First session
				curCB =	pDasm[i]->text(DASM_CODEBYTE_COL);
			}
		}
	
		doGetNextLine = false;
		// Set highlight if different
		for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
		{
			if(pDasm[i] == NULL)
				continue;
			if(isDifferent)
			{
				pDasm[i]->setHighlightDifference(b);
				pDasm[i]->repaint();
			}

			pDasm[i] = (DasmDataListItem*) pDasm[i]->nextSibling();
			// Check if we need to compare the next line.
			if(pDasm[i] != NULL)
				doGetNextLine = true;
		}
	}
}


void DasmDockView::calculateSamplePercentMap(SamplePercentMap* percent, 
				SampleDataMap* samples,
				SampleDataMap* total)
{
	if(percent == NULL || samples == NULL || total == NULL)
		return;

#if DADSM_DEBUG 
	fprintf(stderr,"--------INSTR---------\n");
#endif
	SampleDataMap::iterator sit  = samples->begin();
	SampleDataMap::iterator send = samples->end();
	for(; sit != send ; sit++)
	{
	
		SampleKey key(-1,sit->first.event);	
		SampleDataMap::iterator tit  = total->find(key);
		if( tit != total->end())
		{
			float result = ((sit->second) * 100);
			result = result / (tit->second);
			percent->insert(SamplePercentMap::value_type(sit->first,result));
#if DADSM_DEBUG 
			fprintf(stderr,"cpu = %llx, event = %llx\t\t",
					sit->first.cpu, sit->first.event);
			fprintf(stderr,"result = %f, total = %d, samples = %d\n",
					result, (tit->second), (sit->second));
#endif
		}
	}
}

void DasmDockView::onSelectionChanged(QListViewItem *i)
{
	Q_UNUSED(i);	
/*
	// TODO:This logic enable the simultaneous selection on 
	// both side. It does not work currently.
	if(m_syncDasmView && i != NULL)
	{
		int vx = 0,vy = 0;
		int cy = i->itemPos();
		QListView * parent = i->listView();
		parent->contentsToViewport(0,cy,vx,vy);
	
		for(int j = 0 ; j < MAX_NUM_DIFF ; j++)
		{
				QListViewItem *pItem = m_pView[j]->itemAt(QPoint(0,vy));
				if(pItem != NULL && pItem != i)
						m_pView[j]->setSelected(pItem,true);
				
		}
	}
*/

	// Unhighlight any basic block if any
	for(int j = 0 ; j < MAX_NUM_DIFF ; j++)
	{
		if(m_pCurHighlightedBB[j] != NULL)
		{
			
			DasmDataListItem *pDasm = (DasmDataListItem*) m_pCurHighlightedBB[j]->firstChild();
			while(pDasm != NULL)
			{
				pDasm->setHighlightDifference(false);
				pDasm->repaint();
				pDasm = (DasmDataListItem*) pDasm->nextSibling();
			}
			m_pCurHighlightedBB[j] = NULL;		
		}
	}
	m_pBBDiff->setOn(false);
}

void DasmDockView::onExpanded(QListViewItem *i)
{
	if(m_syncDasmView && i != NULL)
	{
		int vx = 0,vy = 0;
		int cy = i->itemPos();	
		QListView * parent = i->listView();
		parent->contentsToViewport(0,cy,vx,vy);
		for(int j = 0 ; j < MAX_NUM_DIFF ; j++)
		{
			QListViewItem *pItem = m_pView[j]->itemAt(QPoint(0,vy));
			if(pItem != NULL && pItem != i)
				m_pView[j]->setOpen(pItem,true);
		}
	}
}

void DasmDockView::onCollapsed(QListViewItem *i)
{
	if(m_syncDasmView && i != NULL)
	{
		int vx = 0,vy = 0;
		int cy = i->itemPos();	
		QListView * parent = i->listView();
		parent->contentsToViewport(0,cy,vx,vy);
		for(int j = 0 ; j < MAX_NUM_DIFF ; j++)
		{
			QListViewItem *pItem = m_pView[j]->itemAt(QPoint(0,vy));
			if(pItem != NULL && pItem != i)
				m_pView[j]->setOpen(pItem,false);
		}
	}
}

// TODO: This is hardcoded. Need to write new algorithm 
void DasmDockView::onContentsRightMoving(int x ,int y)
{
	if(m_syncDasmView )
	{
		if(!m_isMoving)
		{
			m_isMoving = true;
			int newLX = x - m_xDiff;
			int newLY = y - m_yDiff;
			m_pView[0]->setContentsPos(newLX,newLY);
		}else{
			m_isMoving = false;
		}
	}
}

// TODO: This is hardcoded. Need to write new algorithm 
void DasmDockView::onContentsLeftMoving(int x ,int y)
{
	if(m_syncDasmView)
	{
		if(!m_isMoving)
		{
			m_isMoving = true;
			int newRX = x + m_xDiff;
			int newRY = y + m_yDiff;
			m_pView[1]->setContentsPos(newRX,newRY);
		}else{
			m_isMoving = false;
		}
	}
}

void DasmDockView::onPercentChanged(int i)
{
	Q_UNUSED(i);
	updateListViews();
}

void aggregateSampleDataMap(SampleDataMap* left, SampleDataMap* right)
{
	if(left == NULL || right == NULL)
		return;

	SampleDataMap::iterator rit  = right->begin();
	SampleDataMap::iterator rend = right->end();
	for(; rit != rend ; rit++)
	{
		SampleDataMap::iterator lit  = left->find(rit->first);
		if( lit == left->end())
		{
			left->insert(SampleDataMap::value_type(rit->first,rit->second));
		}else{
			lit->second = lit->second + rit->second;
		}
	}
}

