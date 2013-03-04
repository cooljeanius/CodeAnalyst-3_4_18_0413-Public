/*
 * SrcDensityChart.cpp
 * Source Density Chart
 */

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2012 Advanced Micro Devices, Inc.
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

#ifndef _SRCDENSITYCHART_H_
#define _SRCDENSITYCHART_H_

#include "stdafx.h"
#include "libCAdata.h"
#include "Function.h"
#include "codeDensity.h"

enum SRC_CODE_DENSITY_Enum {
	SRC_ZOOM_SECTION,
	SRC_ZOOM_USER
};

enum CODE_DENSITY_TYPE_Enum {
	CODE_DENSITY_SRC,	
	CODE_DENSITY_DASM,
};

typedef struct _SrcChartSample{
	VADDR first_addr;
	VADDR first_line;
	DataArray samples;

	_SrcChartSample () {
		first_addr = 0;
		first_line = 0;
	};
} SrcChartSample ;

//maps line number to samples
typedef QMap < VADDR, SrcChartSample > SrcChartSampMap;

//DensityChartArea defined in codeDensity.h
class SrcDensityChart : public DensityChartArea 
{
	Q_OBJECT
public:
	SrcDensityChart (ViewShownData *pViewShown, QWidget * parent);
	virtual ~SrcDensityChart ();

	void setFuncRange (VADDR begin, VADDR end);
	void setSamples (SrcChartSampMap samp_map);
	bool initialize ();
	void showCurrent (unsigned int start, unsigned int end);
	void setInterestLine (unsigned long int interesting);
	void setMaxLine (unsigned int max);
	void setDensityType (unsigned int type) { m_type = type; };

	//overwriting pure virtual function
	virtual void zoomChanged (int zoom_level);
	void setCurrentFunction(QString name)
	{ m_currentFunction = name; };

protected:
	//overwriting pure virtual functions!
	virtual bool groupData ();
	virtual void tipGroups ();

private slots:
	void onGroupDoubleClicked (UINT64 group_data);

signals:
	void doubleClicked (UINT64 interesting);

private:
	unsigned int m_total_src_lines;
	QString m_currentFunction;

	SrcChartSampMap m_sample_map;
	VADDR m_funcBegin;
	VADDR m_funcEnd;
	unsigned int m_type;
}; //class SrcDensityChart


#endif //_SRCDENSITYCHART_H_
