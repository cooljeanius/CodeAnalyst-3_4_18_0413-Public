#include <qtabbar.h>
#include <qtabwidget.h>
#include <q3listview.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qstringlist.h>

#include "MonitorDockView.h"

#ifndef __DRIVERMONITORTOOL_H_
#define __DRIVERMONITORTOOL_H_

class DriverMonitorTool;

class DriverMonitorSnapshot : public MonitorSnapshot
{
public:
	DriverMonitorSnapshot();

	virtual ~DriverMonitorSnapshot();
	
	virtual bool takeSnapshot();
	
	virtual void signalSnapshotReady(QObject * obj);

	QStringList getSnapGeneral();
	QStringList getSnapStats();
	QStringList getSnapCpu();
	QStringList getSnapIbs();
	QStringList getSnapPmc();

	bool takeSnapGeneral();
	bool takeSnapStats();
	bool takeSnapCpu();
	bool takeSnapIbs();
	bool takeSnapPmc();

private:
	QString helpReadFile(QString path);

	QDir  m_devOprofileDir;
	QDir  m_devOprofileStatDir;

	QString m_devOprofile;
	QString m_devOprofileStat;

	QStringList m_generalData;
	QStringList m_statsData;
	QStringList m_cpuData;
	QStringList m_ibsData;
	QStringList m_pmcData;
};


///////////////////////////////////////////////////////////////////////////////
class DriverMonitorTool : QTabWidget
{
public:
	DriverMonitorTool(QWidget * parent);
	~DriverMonitorTool();
	
	Q3ListView	* m_pGeneralTab;
	Q3ListView	* m_pStatsTab;
	Q3ListView	* m_pCpuTab;
	Q3ListView	* m_pIbsTab;
	Q3ListView	* m_pPmcTab;

	bool init();
	bool initGeneralTab(DriverMonitorSnapshot * snap);
	bool initStatsTab(DriverMonitorSnapshot * snap);
	bool initCpuTab(DriverMonitorSnapshot * snap);
	bool initIbsTab(DriverMonitorSnapshot * snap);
	bool initPmcTab(DriverMonitorSnapshot * snap);
	void initCommon(Q3ListView * pList);
};

#endif // __DRIVERMONITORTOOL_H_
