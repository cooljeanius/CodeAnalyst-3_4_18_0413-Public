/*
 * Source Density View
 * SrcDensityView.cpp
 */

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2010 Advanced Micro Devices, Inc.
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

#include "SrcDensityChart.h"
#include "SrcDensityView.h"

SrcDensityView::SrcDensityView (QWidget * parent)
:  ZoomDock (parent) 
{
}


SrcDensityView::~SrcDensityView ()
{
	if(m_pSrcChart)
		delete m_pSrcChart;
}


bool SrcDensityView::initialize (ViewShownData *pViewShown)
{
	if (!ZoomDock::initialize ())
		return false;

	//addZoomLevel ("Source File", SRC_ZOOM_FILE);
	addZoomLevel ("Function", SRC_ZOOM_SECTION);
	addZoomLevel ("Partial", SRC_ZOOM_USER);

//	//create the density chart  and hook it up
	m_pSrcChart = new SrcDensityChart (pViewShown, widget ());
	RETURN_FALSE_IF_NULL (m_pSrcChart, this);

	m_pSrcChart->setInterestLine (1);
	if (!m_pSrcChart->initialize ())
		return false;

	setChartArea (m_pSrcChart);
	connect (m_pSrcChart, SIGNAL (doubleClicked (UINT64)), this,
		SLOT (onDoubleClicked (UINT64)));

	setZoomLevel(SRC_ZOOM_SECTION);
	return true;
}//SrcDensityView::initialize


void SrcDensityView::shownDataChanged (unsigned long int start, unsigned long int end)
{
	m_pSrcChart->zoomChanged (m_zoom_level);
}


void SrcDensityView::setSamples (SrcChartSampMap samp_map)
{
	m_pSrcChart->setSamples (samp_map);

	setZoomLevel (m_zoom_level);
}


void SrcDensityView::setFuncRange(VADDR begin, VADDR end)
{
	m_pSrcChart->setFuncRange (begin, end);
}

//Called when a new hot spot is given to the source view
void SrcDensityView::setInterestPoint (unsigned long int interesting)
{
	m_pSrcChart->setInterestLine (interesting);
	//force it to set current reference points
	setZoomLevel (m_zoom_level);
}


//Take the chart's double clicked line number as the new hot spot
void SrcDensityView::onDoubleClicked (UINT64 hot_spot)
{
	emit newHotSpot (hot_spot, 0, 0);
}


void SrcDensityView::setShowCurrentData (bool show_current)
{
	setShowMarkedData (show_current);
}


