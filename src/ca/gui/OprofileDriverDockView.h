
/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2009 Advanced Micro Devices, Inc.
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

#ifndef _OPROFILEDRIVERDOCKVIEW_H_
#define _OPROFILEDRIVERDOCKVIEW_H_

#include <q3dockwindow.h>
#include <qtabwidget.h>
#include <qthread.h>
#include <qlayout.h>
#include <qmutex.h>
#include <Q3BoxLayout>

#include "MonitorDockView.h"
#include "DriverMonitorTool.h"

class OprofileDriverDockView : public MonitorDockView
{
	Q_OBJECT;
public:
	OprofileDriverDockView(
		QWidget * parent, 
		const char * name = NULL, 
		Qt::WindowFlags f = 0);
	virtual ~OprofileDriverDockView();
	virtual void updateWithSnapshot();

public slots:

protected:
	virtual bool init();

	DriverMonitorTool 		* m_pDriverMonitorTool;
	DriverMonitorSnapshot 		* m_pSnapDriver;
	QBoxLayout			* m_pBLayout;
};

#endif //_OPROFILEDOCKVIEW_H_


