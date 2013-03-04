//$Id: CfgIbs.h 20178 2011-08-25 07:40:17Z sjeganat $
//  This is our derivative of the iIbsCfg.ui
/*
// CodeAnalyst for Open Source
// Copyright 2002 - 2007 Advanced Micro Devices, Inc.
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

#ifndef __CFGIBS_H__ 
#define __CFGIBS_H__ 
#include <qstring.h>
#include <qradiobutton.h>
#include <q3listview.h>
#include "IIbsCfg.h"
#include "ProfileCollection.h"
#include "eventsfile.h"
#include "cawfile.h"


#define CA_IBS_FETCH_MAX 	1048575
#define CA_IBS_FETCH_MIN	25000
#define CA_IBS_OP_MAX 		1048575
#define CA_IBS_OP_MIN		25000

enum IBSListEventColumn 
{
	IBS_VALUE = 0,
	IBS_SRC,
	IBS_NAME
};


class IbsEventListViewItem : public Q3ListViewItem
{

public:
	IbsEventListViewItem(Q3ListView *parent) 
	: Q3ListViewItem(parent)
	{
		this->value 	= 0;
	};

	IbsEventListViewItem(Q3ListView *parent, 
			QString name, 
			unsigned int value, 
			QString source)
	: Q3ListViewItem(parent)
	{
		this->setText(0,QString("0x")+QString::number(value,16));
		this->setText(1,source);
		this->setText(2,name);
		this->name = name;
		this->source = source;
		this->value = value;
	};

	~IbsEventListViewItem()
	{

	};

	QString 	name;
	QString 	source;	
	unsigned int 	value;
};

//=================================================================
class IbsCfgDlg : public IIbsCfg
{ 
	Q_OBJECT

public:
	IbsCfgDlg ( QWidget* parent = 0, const char* name = 0, bool modal = true,
		Qt::WFlags fl = Qt::WStyle_Customize | Qt::WStyle_DialogBorder | Qt::WStyle_Title );

	~IbsCfgDlg ();

	void setProperties (IBS_OPTIONS *pSession);
	void setProfile (ProfileCollection *pProfiles, QString name);
	bool wasModified ();

public slots:
	virtual void onOk ();
	void onModified ();
	void onOpEnabled (bool enabled);
	void onSelectAllFetch ();
	void onSelectAllOp ();

private:
	void addIbsEventFromFileToList(int model);
	void addIbsEventFromProfileToList(int model, IBS_OPTIONS *pSession);
	void selectIbsEvents( IBS_OPTIONS * ibsOptions);

	QString m_profileName;
	ProfileCollection *m_pProfiles;
	bool m_modified;
	bool m_dispatchAvail;
	CEventsFile m_eventsFile;
};
#endif /*__CFGIBS_H__  */
