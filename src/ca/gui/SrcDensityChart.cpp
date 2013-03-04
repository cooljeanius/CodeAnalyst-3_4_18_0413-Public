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

#include <qstring.h>
#include "SrcDensityChart.h"

SrcDensityChart::SrcDensityChart (ViewShownData *pViewShown, 
				QWidget * parent)
:  DensityChartArea (pViewShown, parent)
{
	m_funcBegin = 0;
	m_funcEnd = 0;
	m_type = CODE_DENSITY_SRC;
}


SrcDensityChart::~SrcDensityChart ()
{
	m_sample_map.clear ();
}


void SrcDensityChart::setSamples (SrcChartSampMap samp_map)
{
	m_sample_map = samp_map;
}


//If this returns false, the initialization failed, and the chart area
// shouldn't be used.
//SampleMap is defined in stdafx.h
bool SrcDensityChart::initialize ()
{
	if (!DensityChartArea::initialize ())
		return false;

	connect (this, SIGNAL (groupDoubleClicked (UINT64)),
		this, SLOT (onGroupDoubleClicked (UINT64)));

	return true;
}

//recalculates shown stuff from current center, and current start/end
void SrcDensityChart::zoomChanged (int zoom_level)
{
	sym_info temp_sym;
	bool currentZoomFailed = false;

	if (!m_initialized)
		return;

	switch (zoom_level) {
		case SRC_ZOOM_SECTION:
			{
			m_cur_start = m_funcBegin;
			m_cur_end = m_funcEnd;
#if 0
				SrcChartSampMap::Iterator it;

				it = m_sample_map.find (m_start_interest);

				fprintf(stderr,"DEBUG0 func = \n", it.data().function.toAscii().data());
				//if m_start_interest is in a function, show function!
				if ((m_sample_map.end () != it)
				&&  (!it.data().function.isEmpty())) 
				{
				fprintf(stderr,"DEBUG1\n");
					SrcFunctionLineRange & function = 
						m_function_map[it.data ().function];

					if (-1 == function.start) {
						/* If the double clicked line falls on an inline
						* functions, function.start is -1. In this case change
						* the zoom to entire file.
						*/
						currentZoomFailed = true;
					} else {
						m_cur_start = function.start;
						// we want the line after the end, 
						// so it will show the data of the last line
						m_cur_end = function.end + 1;
					}
				} else {
				fprintf(stderr,"DEBUG2\n");
					//otherwise a 500 line region around interest
					if (m_start_interest > 500)
						m_cur_start = m_start_interest - 500;
					else
						m_cur_start = 1;
					m_cur_end = m_start_interest + 500;
				}
#endif
			
#if 0
				if(!m_currentFunction.isEmpty())
				{
					SrcFunctionLineRange & function = 
						m_function_map[m_currentFunction];

					if (~0U == function.start) {
						/* If the double clicked line falls on an inline
						* functions, function.start is -1. In this case change
						* the zoom to entire file.
						*/
						currentZoomFailed = true;
					} else {
						m_cur_start = function.start;
						// we want the line after the end, 
						// so it will show the data of the last line
						m_cur_end = function.end + 1;
					}
				}else{
					//otherwise a 500 line region around interest
					if (m_start_interest > 500)
						m_cur_start = m_start_interest - 500;
					else
						m_cur_start = 1;
					m_cur_end = m_start_interest + 500;
				}
#endif
			}
			break;

		case SRC_ZOOM_USER:
			//if user specified region
			if (0 != m_end_interest) {
				m_cur_start = m_start_interest;
				m_cur_end = m_end_interest + 1;
			} else {
				m_cur_start = m_funcBegin;
				m_cur_end = m_funcEnd;
			}
			break;
	}

	if (currentZoomFailed) { 
		QString msg =  "ERROR: No relevant symbol found.\n";
		msg +=  "Source Density Chart failed to draw. The function maybe an inline function.\n";

		QMessageBox::critical (this, "CodeAnalyst Function Zoom", msg);
	} else {
		if (m_type == CODE_DENSITY_SRC) {
			setCurrentRange (m_cur_start, m_cur_end,
				QString::number ((unsigned int) m_cur_start),
				QString::number ((unsigned int) m_cur_end));
		} else {
			setCurrentRange (m_cur_start, m_cur_end,
				QString("0x") + QString::number ((unsigned int) m_cur_start, 16),
				QString("0x") + QString::number ((unsigned int) m_cur_end, 16));
		}

		calculateScale ();

	}
} //SrcDensityChart::zoomChanged


//Given the current space, partitions the shown region into groups
// these groups aggregate all samples within them, so the max sample changes
//Note that we do this once per resizing/recalculation/zoom
bool SrcDensityChart::groupData ()
{
	int group;
	m_max_group_sample = 1;

	//allocate new groups
	m_groups = new ChartGroupType[m_group_count];
	RETURN_FALSE_IF_NULL (m_groups, this);

	//sort raw data into groups
	for (group = 0; group < m_group_count; group++) {
		m_groups[group].samples.resize (m_pViewShown->available.size(), 0);

		//saves from recalculating this every time.
		m_groups[group].start = m_group_range * (group) + m_cur_start;
		m_groups[group].end = m_group_range * (group + 1) + m_cur_start - 1;

		//The SrcDensityChart saves the first address with samples within the
		//  range in the group.data member
		m_groups[group].data = 0;
		m_groups[group].label = QString::null;
	}

	//For each sample
	for ( SrcChartSampMap::iterator it = m_sample_map.begin ()
			; it != m_sample_map.end (); ++it) 
	{
		//if the sample is outside the current range, don't add it to a shown
		// group
		if ((m_cur_start > it.key ()) || (m_cur_end <= it.key ())) {
			continue;
		}

		group = (it.key () - m_cur_start) / m_group_range;
		if (group > m_group_count) {
			continue;
		}

		if (m_type == CODE_DENSITY_SRC) {
			//save first sample address in the group as data
			if ((m_groups[group].data > it.data ().first_line)
			||  (m_groups[group].label.isEmpty ())) {
				m_groups[group].data = it.data ().first_addr;
			}
		} else {
			if ((m_groups[group].data > it.data ().first_addr)
			||  (m_groups[group].label.isEmpty ())) {
				m_groups[group].data = it.data ().first_addr;
			}

		}

		for (UINT ev = 0; ev < m_pViewShown->shown.size(); ev++) {
			int index = m_pViewShown->shown[ev];
			//ignore events that aren't currently shown

			if ( (!it.data ().samples.empty()) && 
					(index < m_groups[group].samples.size()) && 
					(index < it.data().samples.size())) 
			{
				m_groups[group].samples[index] += (long) it.data ().samples[index];
			}

			//The max group sample sets the y scale.
			if ((m_max_group_sample < m_groups[group].samples[index]) && 
				(index < m_groups[group].samples.size())) 
			{
				m_max_group_sample = m_groups[group].samples[index];
			}
		}
	} //for each sample

	return true;
} //SrcDensityChart::groupData


void SrcDensityChart::tipGroups ()
{
	QString helper;
	QString tip;

	if (NULL == m_groups)
		return;

	for (int group = 0; group < m_group_count; group++) {
		unsigned int max = 0;

		if (m_type == CODE_DENSITY_SRC) {
			//Show the range of the group in the tool tip
			if (m_groups[group].start != m_groups[group].end) {
				tip.sprintf ("Line %u to ", (unsigned int) m_groups[group].start);
				helper.sprintf ("%u", (unsigned int) m_groups[group].end);
				tip += helper;
			} else
				tip.sprintf ("Line %u", (unsigned int) m_groups[group].start);
		} else {
			//Show the range of the group in the tool tip
			if (m_groups[group].start != m_groups[group].end) {
				tip.sprintf ("Address 0x%lx to ", (unsigned long) m_groups[group].start);
				helper.sprintf ("0x%lx", (unsigned long) m_groups[group].end);
				tip += helper;
			} else
				tip.sprintf ("Address 0x%lx", (unsigned long) m_groups[group].start);
		}

		//If there was a label for the group, show it.
		if (!m_groups[group].label.isEmpty ()) {
			tip += QString ("\n") + m_groups[group].label;
		}

		for (UINT ev = 0; ev < m_pViewShown->shown.size(); ev++) {
			int index = m_pViewShown->shown[ev];

			tip += QString ("\n\t");
			tip += QString::number (m_groups[group].samples[index]);
			tip += QString (": ");
			tip += m_pViewShown->tips[index];

			if (max < m_groups[group].samples[index])
				max = m_groups[group].samples[index];
		}

		//The tool tip for the group covers all bars in the group
		m_groups[group].tip_rect = groupTipRect (group, max);

		//If there is no chart to show, don't add a tool-tip
		if (0 == m_groups[group].tip_rect.height ()) {
			continue;
		}
		setToolTip(tip);
	} //for each group
}	// SrcDensityChart::tipGroups


//When a new hotspot is set, set the point of interest in the chart
void SrcDensityChart::setInterestLine (unsigned long int interesting)
{
	m_end_interest = 0;
	m_start_interest = interesting;
	update ();
}


void SrcDensityChart::onGroupDoubleClicked (UINT64 group_data)
{
	emit doubleClicked (group_data);
}


void SrcDensityChart::setMaxLine (unsigned int max)
{
	m_total_src_lines = max;
	update ();
}


void SrcDensityChart::setFuncRange(VADDR begin, VADDR end)
{
	m_funcBegin = begin;	
	m_funcEnd = end;	
}
