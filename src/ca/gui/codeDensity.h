//$Id: codeDensity.h,v 1.18 2005/02/14 16:52:40 jyeh Exp $
// interface for the CodeDensityChart class

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2006 Advanced Micro Devices, Inc.
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

#ifndef _CODE_DENSITY_H
#define _CODE_DENSITY_H

#include <qtooltip.h>
#include <q3dockwindow.h>
#include <QComboBox>
#include <Q3GridLayout>
#include <QPaintEvent>
#include <QResizeEvent>
#include <Q3ValueList>
#include <QMouseEvent>
#include <Q3PopupMenu>
#include "stdafx.h"
#include "atuneoptions.h"
#include "tbsStruct.h"
#include "symbolengine.h"


//only keep track of non-executing sections
typedef struct _ChartSectionType{
	_ChartSectionType(){
	shown = false;
	start = 0;
	end = 0;
	group_start = 0;
	group_end = 0;

	};
	
	bool shown;      //whether the section can be shown at this time
	UINT64 start;    //the start of the section, same units as x axis
	UINT64 end;      //the end of the section, same units as x axis
	int group_start; //which group the section starts in
	int group_end;   //which group the section ends in
} ChartSectionType; 


//Each group will cover a range in the x axis
class ChartGroupType {
public:
	UINT64 start;  //the start of the group range, same units as x axis
	UINT64 end;  //the end of the group range, same units as x axis

	//in the design discussions, the customers said they don't want the data 
	// seperated by cpu
	Q3ValueVector <long> samples;

	//This could be an address, or whatever should be returned when the group 
	// is double-clicked
	UINT64 data;  

	QRect tip_rect; //the rectangle for the group's tool tip
	QString label;  //The label for the group, assigned when the data is added
	ChartGroupType () {};
	ChartGroupType (int sizeOfData) {samples.resize (sizeOfData, 0);};
};


typedef Q3ValueList < ChartSectionType > ChartSectionList;


//This parent class will handle drawing groups, user interactions with the
// chart, and all scale calculations EXCEPT putting the data in groups and
// determining the maximum sample for all the groups and adding tool tips to
// the groups.
//Please note that this is a pure virtual class, current implementations are in
// the dasmview.h/cpp and sourceview.h/cpp files.
class DensityChartArea : public QWidget {
	Q_OBJECT
public:
	DensityChartArea (ViewShownData *pViewShown, QWidget * parent);
	virtual ~DensityChartArea ();

	bool initialize (/*CA_Module * pMod*/);
	void showMarkedRange (bool show_current);
	void setCurrentRange (UINT64 start, UINT64 end, QString start_label,
		QString end_label);
	void markRange (UINT64 start, UINT64 end);
	virtual void zoomChanged (int zoom_level) = 0;

private:
	void drawAxis (QPainter * painter);
	void drawGroups (QPainter * painter);
	void drawTicks (QPainter * painter);
	void drawInterestLines (QPainter * painter);
	void drawCurrentMarkers (QPainter * painter);

	bool calculateGroups ();
	int findGroup (int x);

protected:
	//note pure virtual functions!
	virtual void tipGroups () = 0;
	//groupData should be able to handle case when we're out of memory and
	// m_groups is NULL
	virtual bool groupData () = 0;

	QRect groupTipRect (int group, unsigned int samples);
	void calculateScale ();

	//overwriting qwidget functions
	virtual void paintEvent (QPaintEvent * paint_event);
	virtual void mousePressEvent (QMouseEvent * e);
	virtual void mouseReleaseEvent (QMouseEvent * e);
	virtual void mouseMoveEvent (QMouseEvent * e);
	virtual void mouseDoubleClickEvent (QMouseEvent * e);
	virtual void resizeEvent (QResizeEvent * e);

public slots: 
	void onShownChanged ();
	void OnManageColors (); 

signals:
	void groupDoubleClicked (UINT64 group_data);

private:
	//chart helpers 
	int m_font_height;          //in pixels
	int m_xaxis_at_y;           //in pixels
	int m_yaxis_at_x;           //in pixels
	float m_yscale;             //in samples per pixel
	float m_one_group_width;    //in pixels
	float m_one_bar_width;      //in pixels

	QString m_start_label;
	QString m_end_label;

	bool m_show_marks;

	//events data
	ColorList m_colorList;
	Q3PopupMenu 	*m_popup;

	//the first group of interest, when the user selects a range
	int m_user_group;
	bool m_user_dragging;

protected:
	//edges of chart
	UINT64 m_cur_start;
	UINT64 m_cur_end;

	UINT64 m_mark_start;
	UINT64 m_mark_end;

	UINT64 m_start_interest;
	UINT64 m_end_interest;

	//We want random access, so not a list
	int m_group_count;
	ChartGroupType * m_groups;
	//how much of m_cur_start through m_cur_end is covered by one group
	UINT64 m_group_range;

	//The maximum samples in all groups, for y scale
	unsigned int m_max_group_sample;

	ViewShownData *m_pViewShown;

	//used to prevent errors when trying to draw before initialization finished
	bool m_initialized;

	ChartSectionList m_sections;
};


//This is the parent class of the view that other widgets will interact with
//This handles all the user zoom interactions
class ZoomDock:public Q3DockWindow {
	Q_OBJECT
public:
	ZoomDock (QWidget * parent);
	virtual ~ZoomDock ();

	bool initialize ();
	void checkVisibility ();
	void setChartArea (DensityChartArea * m_area);
	int addZoomLevel (QString description, int index = -1);
	void removeZoomLevel (int index);
	void setZoomLevel (int index);
	int getCurrentZoom() {return m_zoom_level; };

protected:
	void setShowMarkedData (bool show_mark);
	int zoomLevelMinimum ();

public slots:
	void onDockChange (Q3DockWindow::Place p);
	void onZoomIn ();
	void onZoomOut ();
	void onZoomBox (int i);

private:
	DensityChartArea * m_area;

	int m_min_zoom_level;
	Q3GridLayout *m_layout;
	QComboBox *m_zoom_box;
	QPushButton *m_zoom_in;
	QPushButton *m_zoom_out;

protected:
	int m_zoom_level;
	QString m_parent_caption;
};


//We have to subclass this, to be able to check when the visible items may have
// changed.
class ChartListView:public Q3ListView {
	Q_OBJECT 
public:
	ChartListView (QWidget * parent):Q3ListView (parent) {};
	virtual ~ChartListView () {};

signals:
	void contentsRedrawn ();

protected:
	virtual void drawContentsOffset (QPainter * p, int ox, int oy,
		int cx, int cy, int cw, int ch) {
			Q3ListView::drawContentsOffset (p, ox, oy, cx, cy, cw, ch);
			emit contentsRedrawn ();
	};
	virtual void resizeEvent (QResizeEvent * e) {
		Q3ListView::resizeEvent (e);
		emit contentsRedrawn ();
	};

};
#endif //  _CODE_DENSITY_H
