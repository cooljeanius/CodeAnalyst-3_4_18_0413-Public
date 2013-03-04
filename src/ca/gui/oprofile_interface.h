//$Id: oprofile_interface.h,v 1.4 2005/02/14 16:52:40 jyeh Exp $

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2005 Advanced Micro Devices, Inc.
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

#ifndef OPROFILE_INTERFACE_H_
#define OPROFILE_INTERFACE_H_

#include "stdafx.h"
#include <qstring.h>
#include <qthread.h>
#include <q3process.h>
#include <QEvent>
#include <list>
#include "PerfEventContainer.h"

#ifndef OP_BASE_DIR
#define OP_BASE_DIR "/var/lib/oprofile/"
#endif

#ifndef OP_SAMPLES_DIR
#define OP_SAMPLES_DIR OP_BASE_DIR "samples/"
#endif

#ifndef OP_SAMPLES_CURRENT_DIR
#define OP_SAMPLES_CURRENT_DIR OP_SAMPLES_DIR "current/"
#endif

#ifndef OP_LOCK_FILE
#define OP_LOCK_FILE OP_BASE_DIR "lock"
#endif

#ifndef OP_LOG_FILE
#define OP_LOG_FILE OP_SAMPLES_DIR "oprofiled.log"
#endif

#ifndef OP_DUMP_STATUS
#define OP_DUMP_STATUS OP_BASE_DIR "complete_dump"
#endif

#ifndef OP_JAVA_DIR
#define OP_JAVA_DIR  OP_BASE_DIR "Java/"
#endif

#ifndef OP_JIT_DIR
#define OP_JIT_DIR  OP_BASE_DIR "jit/"
#endif


#define MUX_DRIVER_INTERFACE 	"time_slice"

namespace oprofile_interface_ns {

	enum oprofile_interface_errorno {
		OIE_OK=0,
		OIE_LOCK_EXIST,
		OIE_DAEMON_NOTSTART,
		OIE_DAEMON_RUNNING,
		OIE_DRV,
		OIE_ERR
	};

};

using namespace oprofile_interface_ns;
using namespace std;


//typedef std::list<unsigned int>		CntrList;

/**
* Interface between CodeAnalyst and Oprofile. 
* The class essentially does the same thing as opcontrol. 
*
* Note: The function names do not follow the QT coding
* standard to reflect the equivalent name in opcontrol 
* script.
*/
class oprofile_interface
{
public:

	oprofile_interface ();
	
	virtual ~oprofile_interface (); 

	int do_start_daemon (bool vmlinux, 
			QString & vmlinux_dir,
			unsigned int buffer_size, 
			unsigned int buffer_watershed_size, 
			unsigned int cpu_buf_size,
			unsigned int multiplex_interval);

	bool set_op_events (PerfEventContainer * peContainer);
	bool set_css(unsigned int cssDepth, 
			unsigned int cssInterval,
			unsigned int cssTgid,
			unsigned int cssBitness);
	void do_dump ();
	void do_kill_daemon ();
	void do_pause_daemon ();
	void do_resume_daemon();


	QString get_kernel_range (QString vmlinux_dir);

	QString get_std_err() {return m_stderr;};

	QString get_std_out() {return m_stdout;};

	int get_status() {return m_status;};

	bool do_setup ();

	bool remove_lock_file();
	
	bool havePermissionToProfile();

	bool checkOprofileDriverStatus();

	int checkIbsSupportInDaemon();

	bool checkIbsSupportInDriverAndDaemonOk();

	int checkDispatchOpInDaemon();

	int checkDCMissInfoInDaemon();
	
	bool checkMuxSupportInDaemon();

	bool checkMuxSupportInDriver();

	bool checkCssSupportInDaemon();

	bool checkCssSupportInDriver();
	
	bool checkWatchdogEnabled();
	
	bool loadOprofileDriver();

	bool set_param (QString field, unsigned long int value);

	bool isDriverEnabled();

private:
	
	int exec_command (QStringList & args);
	
	int exec_sudo_command (QStringList & args);

	int do_reset();
	
	int  help_start_daemon (bool vmlinux, 
			QString & vmlinux_dir);

	bool get_daemon_pid(unsigned int &pid);


	bool do_sysctl_setup (unsigned int buffer_size, 
				unsigned int buffer_watershed_size, 
				unsigned int cpu_buf_size,
				unsigned int multiplex_interval);

	void customEvent (QEvent * pCustEvent);

	void generate_op_event_arg (QString & arg);

	bool set_ctr_param (int event, QString field, unsigned long int value);

	bool do_mux_setup(unsigned int muxInterval);

	bool do_ebs_setup();
	
	bool do_ebs_reset();
	
	bool do_ibs_setup(QString * errStr);

	QString simplifyPath(QString path);
	
	bool is_driver_loaded();
	
	bool is_oprofilefs_mounted();

	QString getIbs2OprofiledArgString();

	bool isCntrAvailable(unsigned int value);

	bool mapPmcEvents();
private:
	PerfEventContainer	*m_pEventContainer;
	QString 		m_stderr;
	QString 		m_stdout;
	int 			m_status;
	bool 			m_isDaemonStarted;
	unsigned int 		m_daemon_pid;
	int			m_ibsVersion;
	int			m_hasDispatchOpInDaemon;
	int			m_hasDCMissInfoInDaemon;
	unsigned long long 	m_availMask;
	
};

#endif
