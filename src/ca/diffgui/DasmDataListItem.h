
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

#ifndef _DASMDATALISTITEM_H_
#define _DASMDATALISTITEM_H_

#include "DataListItem.h"
#include "diffStruct.h"

class DasmDataListItem : public DataListItem
{
public:
	int *m_pPrecision;
	DasmDataListItem( ViewShownData *pViewShown, 
			int indexOffset, 
			QListViewItem * pParent); 

	void onChangeView (DataArray data);

	void drawData(DASM_VIEW_INFO *pDasmVI,
			bool isPercent = false );
	virtual int compare( QListViewItem *i, 
			int col, 
			bool ascending) const;

	void setHighlightDifference(bool b){ m_hlDiff = b; };

	virtual void paintCell (QPainter * p, 
			const QColorGroup & cg, 
			int column, 
			int width, 
			int align);


protected:
//	QString key (int column, bool ascending) const;

private:
	bool m_hlDiff;
	bool m_isInline;
};

#endif //_DASMDATALISTITEM_H_
