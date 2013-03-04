//$Id: EventCfg.h 20178 2011-08-25 07:40:17Z sjeganat $
//  This is our derivative of the iEventCfg.ui

/**
*   (c) 2006 Advanced Micro Devices, Inc.
*  YOUR USE OF THIS CODE IS SUBJECT TO THE TERMS
*  AND CONDITIONS OF THE GNU GENERAL PUBLIC
*  LICENSE FOUND IN THE "GPL.TXT" FILE THAT IS
*  INCLUDED WITH THIS FILE AND POSTED AT
*  <http://www.gnu.org/licenses/gpl.html>
*  Support: codeanalyst@amd.com
*/

#ifndef EVENT_CFG_H
#define  EVENT_CFG_H

#include <qstring.h>
#include <qcheckbox.h>
#include <q3listbox.h>
#include "iEventCfg.h"
#include "ProfileCollection.h"
#include "eventsfile.h"
#include "cawfile.h"


const unsigned int UNIT_MASK_COUNT = 8;

class UnitCheckBox : public QCheckBox
{
	Q_OBJECT
private:
	int m_index;
public:
	UnitCheckBox (QWidget *parent, int index);
	~UnitCheckBox ();
signals:
	void checked (int index);
	private slots:
		void onChecked ();
};

class EventWithProps : public Q3ListViewItem
{
public:
	EventWithProps (Q3ListViewItem *pParent);
	EventWithProps (Q3ListView *pParent, EventWithProps *pAfter);

	UINT64 m_count;
	unsigned int m_eventSelect ;   // Event select
	unsigned int m_eventUnitMask; // Event unit mask
	bool m_edgeDetect;
	bool m_eventOS;               // Event OS sampling control
	bool m_eventUser ;             // Event User sampling control
};

class EventListWithProps : public Q3ListBoxText
{
public:
	EventListWithProps (Q3ListBox *pParent, const QString & text = QString::null);
	virtual ~EventListWithProps();

	UINT64 m_count;
	unsigned int m_eventSelect ;   // Event select
	unsigned int m_eventUnitMask; // Event unit mask
	bool m_edgeDetect;
	bool m_eventOS;               // Event OS sampling control
	bool m_eventUser ;             // Event User sampling control
};


class EventCfgDlg : public IEventCfg
{ 
	Q_OBJECT
public:
	EventCfgDlg ( QWidget* parent = 0, const char* name = 0, bool modal = true,
		Qt::WFlags fl = Qt::WStyle_Customize | Qt::WStyle_NormalBorder | Qt::WStyle_Title );
	virtual ~EventCfgDlg ();

	void setProperties (EBP_OPTIONS *pSession, const char* path);
	void setProfile (ProfileCollection *pProfiles, QString name);
	bool wasModified ();

public slots:
	virtual void onOk ();
	void onModified ();
	void onConfigChanged (const QString & configName);

	void onAddConfig ();
	void onAddEvent ();
	void onNewGroup ();
	void onMoveEventUp ();
	void onMoveEventDown ();
	void onRemoveEvent ();

	void onEventChanged (Q3ListViewItem *pEvent);
	void onUnitChecked (int index);
	void onEdgeChecked (bool on);
	void onUsrChecked (bool on);
	void onOsChecked (bool on);
	void onCountChanged (const QString & count);

private:
	void buildCheckBoxes ();
	void detectCpu (const char* path);
	int getCpuInfo(const char* path,
			QString &vendorId,
			QString &name,
			QString &family,
			QString &model,
			QString &stepping,
			QString &flags);

	void readEventsFile (QString fileName, int model);
	void updateMux ();
	void insertEvents (EBP_OPTIONS *pSession);
	bool getEvents (EBP_OPTIONS *pSession);
	void resetEventProps ();
	QString makeEventName (unsigned int evSelect);
	Q3ListViewItem * ifNeedAddGroup (int minEventCount = MAX_EVENTNUM);
	unsigned int getMinCountForEvent(unsigned int event);

private:
	UnitCheckBox * m_pUnitMask[UNIT_MASK_COUNT];
	QString m_profileName;
	ProfileCollection *m_pProfiles;
	bool m_modified;
	CEventsFile m_eventsFile;
	bool m_bGeodeChip;
	bool m_bHasLocalApic;
	bool m_isViewProperty;
	QString m_oprofileEventFile;
};
#endif
