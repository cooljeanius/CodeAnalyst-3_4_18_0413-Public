
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

#include <math.h>
#include "DiffDataListItem.h"
#include "EventMaskEncoding.h"
#include "dataagg.h"

///////////////////////////////////////////////////////////////////////////////////////

DiffDataListItem::DiffDataListItem( DiffViewShownData *pViewShown,
				QListView * pParent)
		: QListViewItem (pParent)
{
	m_pViewShown = pViewShown;
	m_checkColumn = -1;
	m_pSampMap = NULL;
	m_precision = DEFAULT_FLOAT_PRECISION;
	initDiffColor();
}

DiffDataListItem::DiffDataListItem( DiffViewShownData *pViewShown,
				QListView * pParent, 
				QListViewItem * pAfter) 
		: QListViewItem (pParent, pAfter)
{
	m_pViewShown = pViewShown;
	m_checkColumn = -1;
	m_total = 0;
	m_pSampMap = NULL;
	m_precision = DEFAULT_FLOAT_PRECISION;
	initDiffColor();
}

DiffDataListItem::DiffDataListItem( DiffViewShownData *pViewShown,
				QListViewItem * pItem)
		: QListViewItem (pItem)
{
	m_pViewShown = pViewShown;
	m_checkColumn = -1;
	m_total = 0;
	m_pSampMap = NULL;
	m_precision = DEFAULT_FLOAT_PRECISION;
	initDiffColor();
}

DiffDataListItem::DiffDataListItem( DiffViewShownData *pViewShown,
				QListViewItem * pItem, 
				QListViewItem * pAfter) 
		: QListViewItem (pItem, pAfter)
{
	m_pViewShown = pViewShown;
	m_checkColumn = -1;
	m_total = 0;
	m_pSampMap = NULL;
	m_precision = DEFAULT_FLOAT_PRECISION;
	initDiffColor();
}

DiffDataListItem::~DiffDataListItem()
{
	m_pSampMap = NULL;
}

void DiffDataListItem::initDiffColor()
{
	m_dataColor[0] = QColor(Qt::gray);
	m_dataColor[1] = QColor(Qt::white);
	m_addrColor[0] = QColor(Qt::darkCyan);
	m_addrColor[1] = QColor(Qt::darkCyan);
}

void DiffDataListItem::setPrecision (int precision)
{
	m_precision = precision;
}

void DiffDataListItem::drawData (SYMBOL_VIEW_INFO* pSymViewInfo, bool isSeparateCpu, MAX_ENTRY_MAP *pMaxEntMap )
{
	m_pSymViewInfo = pSymViewInfo;
	DiffViewColumnInfoMap *pDataColMap = NULL;
	ComplexColumnInfoMap *pComplexColMap = NULL;
	int numCol = m_pViewShown->getNumColumn();
	
	m_pViewShown->getSampleColumnMap(&pDataColMap);
	m_pViewShown->getComplexColumnMap(&pComplexColMap);

	// Setup a buffer to hold calculated value
	DataArray computedBuffer(numCol);

	// Fill-in symbol name 
	DiffCpuEventType key(-1,0,0,-1);
	DiffViewColumnInfoMap::iterator it_v = pDataColMap->find(key);
	setText((it_v.data()).colIndex, m_pSymViewInfo->symName);

	////////////////////////////////////////////////////////////	
	// Showing delta, we show one column 
	if(m_pViewShown->getDiffViewOptions()->showDelta)
	{
		// For each diff module
		for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
		{
			// Fill-in address
			DiffCpuEventType key(-1,0,0,i);
			DiffViewColumnInfoMap::iterator it_v = pDataColMap->find(key);

			QString addrStr = "0x";
			if(m_pSymViewInfo->startAddr[i] == 0)
			{
				addrStr = "";
			}else{
				addrStr += QString::number(m_pSymViewInfo->startAddr[i],16);
			}

			setText((it_v.data()).colIndex, addrStr);

			// TODO: Clean up this code
			if(m_pViewShown->getDiffViewOptions()->showPercentage)
			{
				SamplePercentMap::iterator sdmIt, sdmEnd;
				sdmIt  = (m_pSymViewInfo->samplePercentMap[i]).begin();	
				sdmEnd = (m_pSymViewInfo->samplePercentMap[i]).end();	
				// For each perfevent,unitmask, and cpu
				for(; sdmIt != sdmEnd ;sdmIt++)
				{
					// Separate CPU
					if(isSeparateCpu)
					{
						if( sdmIt->first.cpu == -1 )
							continue;
					}else{
						if( sdmIt->first.cpu != -1 )
							continue;
					}

					unsigned long long deEvent;
					unsigned char deUMask;
					DecodeEventMask(sdmIt->first.event,&deEvent, &deUMask);	
				
					// Get the column index	
					DiffCpuEventType key(sdmIt->first.cpu,deEvent,deUMask,-1);
					//DiffCpuEventType key(-1,deEvent,deUMask,-1);
					DiffViewColumnInfoMap::iterator it_v = pDataColMap->find(key);

					// Calculate Delta
					if( it_v != pDataColMap->end()
					&&  (it_v.data().colIndex) >= 0)
					{
						if(i == 0)
						{
							computedBuffer[(it_v.data().colIndex)] += sdmIt->second;
						}else{
							computedBuffer[(it_v.data().colIndex)] -= sdmIt->second;
						}
					}
				}
			}else{	
				SampleDataMap::iterator sdmIt, sdmEnd;
				sdmIt  = (m_pSymViewInfo->sampleDataMap[i]).begin();	
				sdmEnd = (m_pSymViewInfo->sampleDataMap[i]).end();	
				// For each perfevent,unitmask, and cpu
				for(; sdmIt != sdmEnd ;sdmIt++)
				{
					// Separate CPU
					if(isSeparateCpu)
					{
						if( sdmIt->first.cpu == -1 )
							continue;
					}else{
						if( sdmIt->first.cpu != -1 )
							continue;
					}

					unsigned long long deEvent;
					unsigned char deUMask;
					DecodeEventMask(sdmIt->first.event,&deEvent, &deUMask);	
				
					// Get the column index	
					DiffCpuEventType key(sdmIt->first.cpu,deEvent,deUMask,-1);
					//DiffCpuEventType key(-1,deEvent,deUMask,-1);
					DiffViewColumnInfoMap::iterator it_v = pDataColMap->find(key);

					// Calculate Delta
					if( it_v != pDataColMap->end()
					&&  (it_v.data().colIndex) >= 0)
					{
						if(i == 0)
						{
							computedBuffer[(it_v.data().colIndex)] += sdmIt->second;
						}else{
							computedBuffer[(it_v.data().colIndex)] -= sdmIt->second;
						}
					}
				}
			}
		}	
	// TODO: We do not show delta complex value at this point.
	//       Although we should :)	

	}
	////////////////////////////////////////////////////////////	
	// Showing non-delta,
	else{
		// For each diff module
		for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
		{
			//----------------------------
			// Fill-in address
			DiffCpuEventType key(-1,0,0,i);
			DiffViewColumnInfoMap::iterator it_v = pDataColMap->find(key);
			QString addrStr = "0x";
			if(m_pSymViewInfo->startAddr[i] == 0)
			{
				addrStr = "";
			}else{
				addrStr += QString::number(m_pSymViewInfo->startAddr[i],16);
			}

			setText((it_v.data()).colIndex, addrStr);

			//----------------------------

			SampleDataMap::iterator sdmIt, sdmEnd;
			sdmIt  = (m_pSymViewInfo->sampleDataMap[i]).begin();	
			sdmEnd = (m_pSymViewInfo->sampleDataMap[i]).end();	
			// For each perfevent,cpu
			for(; sdmIt != sdmEnd  ;sdmIt++)
			{
				// Separate CPU
				if(isSeparateCpu)
				{
					if( sdmIt->first.cpu == -1 )
						continue;
				}else{
					if( sdmIt->first.cpu != -1 )
						continue;
				}

				// Decode Event/Mask
				unsigned long long deEvent;
				unsigned char deUMask;
				DecodeEventMask(sdmIt->first.event,&deEvent, &deUMask);				
				
				DiffCpuEventType key(sdmIt->first.cpu,deEvent,deUMask,i);
				//DiffCpuEventType key(-1,deEvent,deUMask,i);
				DiffViewColumnInfoMap::iterator it_v = pDataColMap->find(key);
			
				// Write Value
				if((it_v.data().colIndex) >= 0)
				{
					computedBuffer[(it_v.data().colIndex)] = 
						computedBuffer[(it_v.data().colIndex)] + sdmIt->second; 
				}
			}
		} // Aggregate data

		// Calculate Complex data
		ComplexColumnInfoMap::iterator c_it = pComplexColMap->begin();
		ComplexColumnInfoMap::iterator c_end = pComplexColMap->end();
		for(; c_it != c_end ; c_it++)
		{
			computedBuffer[c_it.data().colIndex]
				= computeComplex( computedBuffer[c_it.key().LopIndex],	// Left op float
					computedBuffer[c_it.key().RopIndex],	// Right op float
					c_it.key().opType );			// Ops	
		}	
		
		// Calculate Percentage 
		if(m_pViewShown->getDiffViewOptions()->showPercentage)
		{
			for(int i = 0 ; i < MAX_NUM_DIFF ; i++)
			{
				SamplePercentMap::iterator sdmIt, sdmEnd;
				sdmIt  = (m_pSymViewInfo->samplePercentMap[i]).begin();	
				sdmEnd = (m_pSymViewInfo->samplePercentMap[i]).end();	
				// For each perfevent,cpu
				for(; sdmIt != sdmEnd  ;sdmIt++)
				{
					// Separate CPU
					if(isSeparateCpu)
					{
						if( sdmIt->first.cpu == -1 )
							continue;
					}else{
						if( sdmIt->first.cpu != -1 )
							continue;
					}

					// Decode Event/Mask
					unsigned long long deEvent;
					unsigned char deUMask;
					DecodeEventMask(sdmIt->first.event,&deEvent, &deUMask);				
					
					DiffCpuEventType key(sdmIt->first.cpu,deEvent,deUMask,i);
					//DiffCpuEventType key(-1,deEvent,deUMask,i);
					DiffViewColumnInfoMap::iterator it_v = pDataColMap->find(key);
				
					// Write Value
					if((it_v.data().colIndex) >= 0)
					{
						computedBuffer[(it_v.data().colIndex)] = sdmIt->second; 
					}
				}
			}
		}

	}

	////////////////////////////////////////////////////////////////////////
	// Writting Sample Data into column
	DiffViewColumnInfoMap::iterator it = pDataColMap->begin();
	DiffViewColumnInfoMap::iterator it_end = pDataColMap->end();
	for(; it != it_end ; it++)
	{
		if((it.data()).isShown == false)
			continue;

		if((it.data()).colType == DATA_COL)
		{
			float val = 0;
			QString tmp;
			if((it.data().colIndex) < 0)
				continue;

			val = fabsf(computedBuffer[(it.data()).colIndex]);
			if( val == 0)
			{	
				setText((it.data()).colIndex,"");
			}else{
				if (fmod (val, (float)1.0) == 0.0)
					m_precision = 0;

				if(m_pViewShown->getDiffViewOptions()->showPercentage)
				{
					setText((it.data()).colIndex, 
						tmp.setNum(val,'f',m_precision)+ "%");
				}else{
					setText((it.data()).colIndex, 
						tmp.setNum(val,'f',m_precision));

				}
			}

			//------------------------------------------------------
			// Check if this entry has the max value of the column. 
			MAX_ENTRY_MAP::iterator mit = pMaxEntMap->find(it.data().colIndex);
			if(mit == pMaxEntMap->end())
			{
				pMaxEntMap->insert(it.data().colIndex,this);
			}else{
				DiffDataListItem *pItem = (DiffDataListItem*)mit.data();
				if(pItem != NULL)
				{
					float maxVal = pItem->text(it.data().colIndex)
							.replace(QChar('%'),"")
							.toFloat();
					if(maxVal < val)
					{
						mit.data() = this;
					}
				}
			}
		}
	}

	// Writting Complex Data into column
	ComplexColumnInfoMap::iterator c_it = pComplexColMap->begin();
	ComplexColumnInfoMap::iterator c_end = pComplexColMap->end();
	for(; c_it != c_end ; c_it++)
	{
		if((c_it.data()).isShown == false)
			continue;

		if((c_it.data()).colType == COMPLEX_COL)
		{
			QString tmp;
			if((c_it.data().colIndex) < 0)
				continue;

			float val = computedBuffer[(c_it.data()).colIndex];
			if( val == 0)
			{
				setText((c_it.data()).colIndex,"");
			}else{
				setText((c_it.data()).colIndex, tmp.setNum(val));
			}
			
			//------------------------------------------------------
			// Check if this entry has the max value of the column. 
			MAX_ENTRY_MAP::iterator mit = pMaxEntMap->find(c_it.data().colIndex);
			if(mit == pMaxEntMap->end())
			{
				pMaxEntMap->insert(c_it.data().colIndex,this);
			}else{
				DiffDataListItem *pItem = (DiffDataListItem*)mit.data();
				if(pItem != NULL)
				{
					float maxVal = pItem->text(c_it.data().colIndex)
							.replace(QChar('%'),"")
							.toFloat();
					if(maxVal < val)
					{
						mit.data() = this;
					}
				}
			}
		}
	}
	computedBuffer.clear();
} //DiffDataListItem::drawData


void DiffDataListItem::setChecked (int column)
{
	m_checkColumn = column;
}


//We needed to customize this so we can show checks in a column
void DiffDataListItem::paintCell (QPainter * p, const QColorGroup & cg, 
					  int column, int width, int align)
{
	//Only items that should be checked have a valid m_checkColumn
	if (m_checkColumn == column)
	{
		//Similar to qlistview.cpp, for the check in the list view item column
		QListView *lv = listView();
		if ( !lv )
			return;

		const BackgroundMode bgmode = lv->viewport()->backgroundMode();
		QColorGroup::ColorRole crole = QPalette::backgroundRoleFromMode( bgmode );
		if (isSelected ())
			crole = QColorGroup::Highlight;
		p->fillRect( 0, 0, width, height(), cg.brush( crole ) );

		QFontMetrics fm( lv->fontMetrics() );
		int boxsize = lv->style().pixelMetric( QStyle::PM_CheckListButtonSize,
			lv);
		int margin = lv->itemMargin();

		int styleflags = QStyle::Style_Default | QStyle::Style_On 
			| QStyle::Style_Enabled;
		if ( isSelected() )
			styleflags |= QStyle::Style_Selected;

		int x = ((width - boxsize) / 2) + margin;
		int y = ( ( height() - boxsize ) / 2 ) + margin;

		lv->style().drawPrimitive (QStyle::PE_CheckMark, p,
			QRect (x, y, boxsize, boxsize), cg, styleflags, 
			QStyleOption(this));
	} else {
		QColorGroup cg1(cg);
		int diffIndex = -1;
		bool isAddr = false;	
		// Symbol Column
		if(column == 0)
		{
			cg1.setColor(QColorGroup::Text, QColor(Qt::blue));		

		}else if((diffIndex = m_pViewShown->getDiffIndex(column,&isAddr)) >= 0)
		{
			if(isAddr)
			{
				cg1.setColor(QColorGroup::Text, QColor(Qt::white));
				cg1.setColor(QColorGroup::Base, m_addrColor[diffIndex]);
			}else{
				cg1.setColor(QColorGroup::Base, m_dataColor[diffIndex]);
			}
		}

		// Highlight Max entry
		if(m_maxCol[column])
		{
			// Do no highlight if 0
			if(!text(column).isEmpty())
			{
				cg1.setColor(QColorGroup::Base, QColor(Qt::red));
				cg1.setColor(QColorGroup::Text, QColor(Qt::white));
			}
		}

		QListViewItem::paintCell (p, cg1, column, width, align);
	}
} //DiffDataListItem::paintCell


//will display text with no decimal point if unneeded
void DiffDataListItem::appendData (float data, int precision)
{
	if (fmod (data, (float)1.0) == 0.0)
		precision = 0;

	m_dataList.append (QString::number (data, 'f', 
		precision));
}

int DiffDataListItem::compare( QListViewItem *i, int col, bool ascending) const
{
	Q_UNUSED(ascending);
	bool isOkL, isOkR;
	int ret = 0;

	if (i == NULL)	
		return ret;

	QString txtL = text(col);
	QString txtR = i->text(col);

	//--------------------------------------
	// Try converting to Int
	int left = 0;
	int right = 0;

	// Get Text Left
	if(!txtL.isEmpty())
		left = txtL.toInt(&isOkL,0);
	else{
		left = 0;
		isOkL = true;
	}

	// Get Text Right
	if(!txtR.isEmpty())
		right = txtR.toInt(&isOkR,0);
	else{
		right = 0;
		isOkR = true;
	}

	// Check if can convert to int
	if(isOkL && isOkR)	
	{
		ret = left - right; 
		//ret = right - left; // This is the reverse
		return ret ;
	}

	//--------------------------------------
	// Try converting to Float 
	// Get Text Left
	float leftF = 0;
	float rightF = 0;

	if(!txtL.isEmpty())
		leftF = txtL.toFloat(&isOkL);
	else{
		leftF = 0;
		isOkL = true;	
	}

	// Get Text Right
	if(!txtR.isEmpty())
		rightF = txtR.toFloat(&isOkR);
	else{
		rightF = 0;
		isOkR = true;	
	}

	// Check if can convert to float
	if(isOkL && isOkR)	
	{
		// NOTE: HACK. This is the reverse
		if(leftF < rightF)
			ret = -1;	
		else if(leftF == rightF)
			ret = 0;	
		if(leftF > rightF)
			ret = 1;	
		return ret ;
	}

	//--------------------------------------

	// Do Txt comparison
	ret = txtL.compare(txtR);	
	return ret;

}

float DiffDataListItem::computeComplex (float lop, float rop, int ops)
{
	float calc = 0;
	float normOp1 = lop;
	float normOp2 = rop;
//	float normOp1 = (*pData)[pComplex->op1Index] * pComplex->op1NormValue;
//	float normOp2 = (*pData)[pComplex->op2Index] * pComplex->op2NormValue;
	switch (ops) 
	{
	case ColumnRatio:
		if (0 != normOp2)
		{
			calc = normOp1 / normOp2;
		}
		break;
	case ColumnSum:
		calc = normOp1 + normOp2;
		break;
	case ColumnDifference:
		calc = normOp1 - normOp2;
		break;
	case ColumnProduct:
		calc = normOp1 * normOp2;
		break;
	default:
		break;
	}
	return calc;
}

