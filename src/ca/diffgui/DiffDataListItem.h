
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

#ifndef _DIFFDATALISTITEM_H_
#define _DIFFDATALISTITEM_H_


#include <qlistview.h>
#include <qcolor.h>

#include "stdafx.h"
#include "DiffViewShownData.h"
#include "diffStruct.h"

class DiffDataListItem : public QListViewItem
{
public:
	DiffDataListItem( DiffViewShownData *pViewShown,
					QListView * pParent);
	DiffDataListItem( DiffViewShownData *pViewShown,
					QListView * pParent, 
					QListViewItem * pAfter);
	DiffDataListItem( DiffViewShownData *pViewShown,
					QListViewItem * pItem);
	DiffDataListItem( DiffViewShownData *pViewShown,
					QListViewItem * pItem, 
					QListViewItem * pAfter);

	~DiffDataListItem();

	void addMaxCol(int c) { m_maxCol[c] = true;};
	void setItemColor(QColor color) { m_textColor = color;};
	void setBackgroundColor (QColor color) {m_backColor = color;};
	void setPrecision (int precision);
	void drawData (SYMBOL_VIEW_INFO* pSymViewInfo, 
			bool isSeparateCpu = false ,
			MAX_ENTRY_MAP *pMaxEntMap = NULL);

	QString getTipString() {  return m_tip;	};
	UINT64 getItemTotal() {return m_total;};
	void setItemTotal(UINT64 total) {m_total = total;};
	void addItemTotal(UINT64 count) {m_total += count;};
	// The following functino should be removed.
	QStringList m_dataList;
	//this would be better as a slot.
	void appendData (float data, int precision = 2);
	void setChecked (int column);
	
	virtual void paintCell( QPainter * p, const QColorGroup & cg, 
			int column, int width, int align );

	virtual int compare( QListViewItem *i, int col, bool ascending) const;

	SYMBOL_VIEW_INFO* getSymbolViewInfo()
	{ return m_pSymViewInfo; };	
	
private:

	void initDiffColor();
	float computeComplex (float lop, float rop, int ops);


	DiffViewShownData 	*m_pViewShown;
	int 			m_precision;
	SYMBOL_VIEW_INFO	*m_pSymViewInfo;
	QString 		m_tip;
	UINT64 			m_total;
	int 			m_checkColumn;
	QColor 			m_textColor;
	QColor 			m_backColor;
	QColor 			m_dataColor[MAX_NUM_DIFF];
	QColor 			m_addrColor[MAX_NUM_DIFF];
	SampleDataMap 		*m_pSampMap;
	QMap<int,bool>		m_maxCol;
};



#endif ///_DIFFDATALISTITEM_H_
