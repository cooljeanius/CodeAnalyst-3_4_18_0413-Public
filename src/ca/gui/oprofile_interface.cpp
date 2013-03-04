//$Id: oprofile_interface.cpp,v 1.21 2006/05/15 22:09:22 jyeh Exp $

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <qfile.h>
#include <q3textstream.h>
#include <qdir.h>
#include <qevent.h>
#include <q3process.h>
#include <qstringlist.h>
#include <qthread.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <qmessagebox.h>

#include "config.h"
#include "oprofile_interface.h"
#include "helperAPI.h"
#include "symbolengine.h"


namespace OPROFILE_INTERFACE {

	const int MAX_CHAR_PER_LINE = 256;

	QString OP_MOD_NAME = "oprofile";
	QString OP_FS_NAME = "oprofilefs";
	QString OP_MOUNT = "/dev/" + OP_MOD_NAME;

	QString DIR = OP_BASE_DIR;
	QString MOUNT = OP_MOUNT;
	QString LOCK_FILE = DIR + "lock";
	QString CA_OPROFILE_CONTROLLER  = QString(CA_SBIN_DIR) + "/ca_oprofile_controller";
};

using namespace OPROFILE_INTERFACE;
using namespace std;


// Constructor
oprofile_interface::oprofile_interface ()
{
	m_isDaemonStarted = false;
	m_daemon_pid = 0;
	m_ibsVersion = 0;
	m_hasDispatchOpInDaemon = 0;
	m_hasDCMissInfoInDaemon = 0;
	m_pEventContainer = NULL;
	
	CA_OPROFILE_CONTROLLER  = simplifyPath(CA_OPROFILE_CONTROLLER);
}


oprofile_interface::~oprofile_interface ()
{

}


int oprofile_interface::do_reset ()
{
	QStringList cmd;
	cmd.append(CA_OPROFILE_CONTROLLER);
	cmd.append("--reset");

	return exec_sudo_command (cmd);
}


/* Must be called after driver is loaded */
bool oprofile_interface::do_setup ()
{
	QDir dir;
	bool rt = true;

	/* Clean the OProfile sample directory */
	if(do_reset() != 0)
		rt = false;

	return rt;
}


bool oprofile_interface::set_op_events (PerfEventContainer * peContainer)
{
	if (!peContainer)
		return false;

	m_pEventContainer = peContainer;
	return true;
}

int oprofile_interface::exec_sudo_command (QStringList & args)
{
	return ca_exec_sudo_command(args, m_stdout, m_stderr, m_status);
}


int oprofile_interface::exec_command (QStringList & args)
{
	return ca_exec_command(args, m_stdout, m_stderr, m_status);
}


void oprofile_interface::generate_op_event_arg (QString & arg)
{
	QString buf, tmp;
	arg = "";

	PerfEventList::iterator it = m_pEventContainer->getPerfEventListBegin();
	PerfEventList::iterator it_end = m_pEventContainer->getPerfEventListEnd();
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::Pmc) {
			tmp.sprintf ("%s:%u:%d:%lu:%u:%u:%u,",
				(*it).opName.toAscii().data(),
				(*it).select(),
				(*it).counter,
				(*it).count,
				(*it).umask(),
				(*it).usr()? 1: 0, 
				(*it).os()? 1: 0);
			buf += tmp;
		}
	}

	if (!buf.isEmpty())
		arg = QString("--events=")+ buf;

}


int oprofile_interface::help_start_daemon (bool vmlinux, 
					   QString & vmlinux_dir)
{
	int rt = OIE_OK;
	m_isDaemonStarted = false; 

	QStringList cmd_OPROFILED_START;
	cmd_OPROFILED_START.append (CA_OPROFILE_CONTROLLER);
	
	// BEGIN CMD LINE APPENDING.
	QString opd_args;
	
	// Note: this is a string, we don't need the " for arguments.	
	opd_args.append ("--start=");
	opd_args.append ("--session-dir=/var/lib/oprofile ");
	opd_args.append ("--separate-lib=1 ");
	opd_args.append ("--separate-kernel=0 ");
	opd_args.append ("--separate-thread=1 ");
	opd_args.append ("--separate-cpu=1 ");

	//********************************************************
	// VMLINUX stuff	
	QString kernel_range;
	QString buf;
	if (vmlinux) {
		kernel_range = get_kernel_range (vmlinux_dir);
		buf = "--vmlinux=";
		buf += vmlinux_dir + " ";
		opd_args.append (buf);

		buf = "--kernel-range=";
		buf += kernel_range + " ";
		opd_args.append (buf);
	} else {
		opd_args.append ("--no-vmlinux");
	}
	opd_args.append(" ");

	//********************************************************
	// PMC stuff

	// TODO: Suravee: How do we handle OProfile timer event???
	// TODO: Is the following statement still true???
	/* The daemon currently needs "--events" args even when no 
	* event  is selected. */
	QString event_args;
	generate_op_event_arg (event_args);
	opd_args.append (event_args);

	//********************************************************
	// IBS stuff
	QString ibs_args;
	if (checkIbsSupportInDaemon() == 2) {
		ibs_args = QString(" ") + getIbs2OprofiledArgString();
		opd_args.append (ibs_args);
	}
//TODO: Suravee: Do we still support this?
#if 0
	else if (checkIbsSupportInDaemon() == 1) {
		// IBS1 stuff
		buf = "";
		if( (active_profiling & OP_IBS_FETCH)
		||  (active_profiling & OP_IBS_OP))
			opd_args.append("--events= ");

		if (active_profiling & OP_IBS_FETCH) {
			buf.sprintf("--ibs-fetch=%ld ", m_pIbsProperties->fetch_count);
			opd_args.append (buf);
		} 

		if (active_profiling & OP_IBS_OP) {
			buf.sprintf("--ibs-op=%ld ", m_pIbsProperties->op_count);
			opd_args.append (buf);

		if (checkDispatchOpInDaemon() > 0) {
			if(m_pIbsProperties->dispatched_ops)
				opd_args.append ("--ibs-op-dispatch-op ");
		}
			
		if (checkDCMissInfoInDaemon() > 0) {
			if(m_pIbsProperties->dcMissInfoEnabled)
				opd_args.append ("--ibs-op-dcmissinfo-enable ");
		}

		}
	}
#endif

	// END CMD LINE APPENDING.

	cmd_OPROFILED_START.append (opd_args);

#ifdef _DEBUG_
	fprintf(stderr,"DEBUG 0: %s\n",cmd_OPROFILED_START.join(" ").toAscii().data());
#endif
	
	//---------------------------------------------------------
	/* NOTE:
	* Check if daemon is currently running by check 
	* "pidof oprofiled"
	*/
	QStringList cmd_pidof;
	cmd_pidof.append(PIDOF);
	cmd_pidof.append("oprofiled");

	if (exec_command(cmd_pidof) == 0) {
		rt = -OIE_DAEMON_RUNNING;
	}

	//---------------------------------------------------------
	/* NOTE:
	* Check if /var/lib/oprofile/lock exists. If not, start daemon. 
	*/
	if (rt == OIE_OK) 
	{
		QFile lockFile (LOCK_FILE);
		if( !lockFile.exists ()) 
		{
			// Starting oprofile daemon 
			if(exec_sudo_command (cmd_OPROFILED_START) == 0) 
			{
#if _DEBUG_
				fprintf(stderr,"help_start_daemon: stdout = %s\n",m_stdout.toAscii().data());
				fprintf(stderr,"help_start_daemon: stderr = %s\n",m_stderr.toAscii().data());
#endif 
				
				m_isDaemonStarted = true;

				m_stderr = QString("Failed to check ") + LOCK_FILE 
					+ "\n\nPlease make sure oprofile daemon is not "
					+ "currently running.\n"; 
				rt = -OIE_DAEMON_NOTSTART;

				cmd_OPROFILED_START.clear ();
				qApp->setOverrideCursor( QCursor(Qt::WaitCursor) );

				// Wait for 5 second for the lock file to be created by daemon
				for (int i = 0; i < 10; i++) 
				{
					if (QFile::exists (LOCK_FILE)) 
					{
						m_stderr = ""; 
						rt = OIE_OK;
						break;
					} else {
						timespec temp;
						temp.tv_sec = 0;
						temp.tv_nsec = FIVE_HUNDRED_MS;
						nanosleep (&temp, NULL);
					}
				}
				qApp->restoreOverrideCursor();

			}else{
#if _DEBUG_
				fprintf(stderr,"help_start_daemon: stdout = %s\n",m_stdout.toAscii().data());
				fprintf(stderr,"help_start_daemon: stderr = %s\n",m_stderr.toAscii().data());
				fprintf(stderr,"help_start_daemon: Failed to start daemon\n");
#endif 
				m_stderr = "Failed to start daemon with OProfile Command:\n\n"; 
				m_stderr += cmd_OPROFILED_START.join(" ").
						replace(" ", "\n\t").
						replace(",","\n\t\t");
				rt = -OIE_DAEMON_NOTSTART;
			}
		}else{
			m_stderr = QString("File /var/lib/oprofile/lock exists.\n")
				+ "Please remove this file and make sure oprofile "
				+ "daemon is not currently running.\n";
			rt = -OIE_LOCK_EXIST;
		}
	}

	//---------------------------------------------------------
	/* NOTE:
	* If the daemon is created successfully,
	* enable the driver by writting 1 to /dev/oprofile/enable
	* and read back to confirm.
	*/ 
	if (rt == OIE_OK) 
	{
		// Write 1 to enable
		if (!set_param (QString("enable"),1)) {
			do_kill_daemon();
			m_stderr = QString("Oprofile Driver error.\n")
				+ "There was a problem writing to the /dev/oprofile/enable.";
			rt = -OIE_DRV;
		}

		QFile enable_file(MOUNT + "/enable");
		if (enable_file.open (QIODevice::ReadOnly)) {
			char rdBuf = '\0';
			// Read to confirm
			if ( rt == OIE_OK
				&& -1 == enable_file.readBlock (&rdBuf, sizeof (rdBuf))) {
					do_kill_daemon();
					m_stderr = QString("Oprofile Driver error.\n")
					+ "There was a problem reading from the /dev/oprofile/enable.";
					rt = -OIE_DRV;
			}
			// Compare Read to Write
			if( rt == OIE_OK
				&& rdBuf != '1' ) {
					do_kill_daemon();
					m_stderr = QString("Oprofile Driver error.\n")
					+ "There was a problem enabling driver.";
					rt = -OIE_DRV;
			}

			enable_file.close ();
		}else{
			do_kill_daemon();
			m_stderr = QString("Oprofile Driver error.\n")
				+ "There was a problem openning /dev/oprofile/enable.";
			rt = -OIE_DRV;
		}
	}

	if (rt == OIE_OK) {
		m_isDaemonStarted = true;
		get_daemon_pid(m_daemon_pid);
	} else {
		m_isDaemonStarted = false;
		m_daemon_pid = 0;
	}

	return rt;
}


bool oprofile_interface::get_daemon_pid(unsigned int &pid) 
{
	QFile lock (LOCK_FILE);
	char tmp[MAX_CHAR_PER_LINE];
	if (lock.exists() && lock.open(QIODevice::ReadOnly)) {
		lock.readLine(tmp,MAX_CHAR_PER_LINE);
		lock.close();
		QString line(tmp);
		pid = line.toUInt();
		return true;
	}
	return false;		
}


int oprofile_interface::do_start_daemon (bool vmlinux, 
					QString & vmlinux_dir,
					unsigned int buffer_size,
					unsigned int watershed_size, 
					unsigned int cpu_buf_size,
					unsigned int multiplex_interval)
{
	m_stderr = "";
	m_availMask = 0;

	// Get counter allocation for PMC event
	if (!mapPmcEvents())
		return -OIE_ERR;

	if(!do_sysctl_setup(buffer_size, 
			watershed_size, 
			cpu_buf_size, 
			multiplex_interval))
		return -OIE_ERR;

	return help_start_daemon(vmlinux, vmlinux_dir);
}


bool oprofile_interface::set_ctr_param (int event, QString field, unsigned long int value)
{
	QString fname = QString::number(event) + "/" + field;

	return set_param(fname, value);
}


bool oprofile_interface::set_param (QString field, unsigned long int value)
{
	QStringList cmd_DEVCONFIG;
	cmd_DEVCONFIG.append (CA_OPROFILE_CONTROLLER);
	
	// BEGIN CMD LINE APPENDING.
	QString args;
	
	// Note: this is a string, we don't need the " for arguments.	
	args.append ("--dev-config=");
	args.append (field);
	args.append (":");
	args.append (QString::number(value));

	cmd_DEVCONFIG.append(args);	

	if (exec_sudo_command (cmd_DEVCONFIG) != 0) {
		QMessageBox::critical(qApp->activeWindow(), "Oprofile Device File Error",
			QString("Error:\nca_oprofile_controller could not configure file\n") 
			+ "/dev/oprofile/" + field + " with value " + QString::number(value));
		return false;
	}
	return true;
}


bool oprofile_interface::do_mux_setup(unsigned int muxInterval)
{
	/* Setting up multiplexing we need:
 	 * 	- MUX interval > 0 
 	 * 	- /dev/oprofile/time_slice present	
 	 */
	QFile mux_ctrl(QString("/dev/oprofile/")+ MUX_DRIVER_INTERFACE);

	if (!mux_ctrl.exists())
		return true;
	
	if (muxInterval == 0) {
		muxInterval = DEFAULT_MUX_INTERVAL_MS;
	}

	if(!set_param (MUX_DRIVER_INTERFACE, muxInterval)) {
		/* This usually means driver error */
		m_stderr = QString("Failed to setup driver for event Multiplexing:\n")
			+ QString("Cannot configure /dev/oprofile/") 
			+ MUX_DRIVER_INTERFACE + ".\n";
		return false;
	}
	return true;
}


bool oprofile_interface::mapPmcEvents()
{
	if (m_availMask != 0)
		return true; 

	if (!(m_availMask = getCounterAvailMask())) {
		m_stderr = "Could not determine available counters.\n";
		m_stderr += 
			"Please try unload driver by running\n"
			"    opcontrol --deinit\n"
			"and try running profile again.\n"
			"(NOTE: root permission required)\n";
		return false;
	}

	if (!m_pEventContainer->getCounterAllocation(m_availMask)) {
		m_stderr = "Could not allocate counter to current event configuration.";
		return false;
	}

	return true;
}

bool oprofile_interface::do_ebs_reset()
{
	QStringList cmd_DEVRESET;
	cmd_DEVRESET.append (CA_OPROFILE_CONTROLLER);
	
	// BEGIN CMD LINE APPENDING.
	QString args("--pmc-reset");

	cmd_DEVRESET.append(args);	

	if (exec_sudo_command (cmd_DEVRESET) != 0) {
		QMessageBox::critical(qApp->activeWindow(), "Oprofile Device File Error",
			QString("Error:\nca_oprofile_controller could not reset /dev/oprofile."));
		return false;
	}
	return true;
}

bool oprofile_interface::do_ebs_setup()
{
	bool rt = false;

	m_stderr = QString("Cannot access /dev/oprofile/0,1,2,3...\n");

	unsigned long long curAvailMask = m_availMask;

	// Reset PMC counters
	if (!do_ebs_reset()) {
		m_stderr = QString("Cannot reset /dev/oprofile/0,1,2,3...\n");
		return rt;	
	}

	// Program counters with event
	PerfEventList::iterator it = m_pEventContainer->getPerfEventListBegin();
	PerfEventList::iterator it_end = m_pEventContainer->getPerfEventListEnd();
	for (; it != it_end; it++) {
		if (((*it).type() == PerfEvent::Pmc || (*it).type() == PerfEvent::OProfile)
		&&  (*it).counter >= 0) 
		{
			int cntr = (*it).counter;
			if(!set_ctr_param (cntr, QString ("enabled"), 1)) goto errOut;
			if(!set_ctr_param (cntr, QString ("event"), (*it).select())) goto errOut;
			if(!set_ctr_param (cntr, QString ("count"), (*it).count)) goto errOut;
			if(!set_ctr_param (cntr, QString ("unit_mask"), (*it).umask())) goto errOut;
			if(!set_ctr_param (cntr, QString ("kernel"), (*it).os()? 1: 0)) goto errOut;
			if(!set_ctr_param (cntr, QString ("user"), (*it).usr()? 1: 0)) goto errOut;
			curAvailMask &= ~(1ULL << cntr);
		}
	}

	rt = true;
errOut:
	return rt;
}


bool oprofile_interface::do_ibs_setup(QString * errStr)
{
	// Setup IBS Stuff if exist
	QFile ibs_fetch_dir("/dev/oprofile/ibs_fetch");
	QFile ibs_op_dir("/dev/oprofile/ibs_op");
	bool rt = false;
	unsigned long count = 0;
	unsigned int  umask = 0;

	if (ibs_fetch_dir.exists()) {
		// Setup IBS Fetch parameters
		if(!m_pEventContainer->getIbsFetchCountUmask(count, umask)) goto errOut;
		
		if (count > 0) {
			if(!set_param ("ibs_fetch/max_count", count )) goto errOut;
			// Force always
			if(!set_param ("ibs_fetch/rand_enable", 1)) goto errOut;
			if(!set_param ("ibs_fetch/enable", (count != 0)? 1 : 0)) goto errOut;
		} else {
			// Disable IBS Fetch
			if(!set_param ("ibs_fetch/enable", 0)) goto errOut;
		}
	}

	if (ibs_op_dir.exists()) {
		// Setup IBS Op parameters
		if(!m_pEventContainer->getIbsOpCountUmask(count, umask)) goto errOut;
		if (count > 0) {
			if(!set_param ("ibs_op/max_count", count )) goto errOut;
			if(!set_param ("ibs_op/enable", (count != 0)? 1 : 0)) goto errOut;
		
			// Check if dispatched_ops is available
			if (isDriverIbsOpDispatchOpOk()) {
				if (!set_param ("ibs_op/dispatched_ops", 
				    ((umask & IbsOpEvent::DispatchCount) > 0)? 1 : 0 )) 
					goto errOut;
			} else if ((umask & IbsOpEvent::DispatchCount)) {
				/* Error: Want DispatchCount but not available */
				if (errStr) *errStr = "Driver does not support IBS Op Dispatch Count mode.";
				goto errOut;
			}
			
			// Check if branch target addr is available
			if (isDriverIbsOpBranchTargetAddrOk()) {
				if (!set_param ("ibs_op/branch_target", 
				    ((umask & IbsOpEvent::BranchTargetAddr) > 0)? 1 : 0 )) 
					goto errOut;
			}  else if (umask & IbsOpEvent::BranchTargetAddr) {
				/* Error: Want BranchTargetAddr but not available */
				if (errStr) *errStr = "Driver does not support Branch Target Address logging.";
				goto errOut;
			}
		} else {
			// Disable IBS Op
			if(!set_param ("ibs_op/enable" , 0)) goto errOut;
		}
	}
	rt = true;
errOut:
	return rt;
}


bool oprofile_interface::do_sysctl_setup (unsigned int buffer_size,
				unsigned int watershed_size, 
				unsigned int cpu_buf_size, 
				unsigned int multiplex_interval)
{
	QStringList args;
	QString errStr;
	bool rt = true;
	
	/* Setting /dev/oprofile/buffer_size */
	if(buffer_size != 0 && !set_param("buffer_size", buffer_size)) {
		m_stderr = QString("Cannot configure /dev/oprofile/buffer_size.\n");
		goto errOut;
	}

	/* Setting /dev/oprofile/buffer_watershed */
	if(buffer_size != 0 && !set_param("buffer_watershed", watershed_size)) {
		m_stderr = QString("Cannot configure /dev/oprofile/buffer_watershed.\n");
		goto errOut;
	}

	/* Setting /dev/oprofile/cpu_buffer_size */
	if(cpu_buf_size != 0 && !set_param ("cpu_buffer_size", cpu_buf_size)) {
		m_stderr = QString("Cannot configure /dev/oprofile/cpu_buffer_size.\n");
		goto errOut;
	}

	if (!do_mux_setup(multiplex_interval)) {
		goto errOut;
	}

	if (!do_ebs_setup()) {
		m_stderr = QString("Failed to setup driver for EBP.\n");
		goto errOut;
	}

	if (!do_ibs_setup(&errStr)) {
		m_stderr = QString("Failed to setup driver for IBS.\n") + errStr + "\n";
		goto errOut;
	}

	m_stderr = "";
	return rt;

errOut:	
	m_stderr += QString("\nPlease also make sure that :\n")
		+ "    1. /dev/oprofile/... exists.\n"
		+ "    2. User is in the \"amdca\" group.\n\n"
		+ "Otherwise, please do one of the followings:\n"
		+ "    1. Run \"opcontrol --init\" as root.\n"
		+ "    2. Or reboot the system.\n";
	rt = false;
	return rt;

}


QString oprofile_interface::get_kernel_range (QString vmlinux_dir)
{
	char buf[LONG_STR_MAX * 2 + 1];
	char buf2[LONG_STR_MAX];
	char buf3[LONG_STR_MAX];
	QString range = "";
	SymbolEngine symengine;
	string line;
	bfd_size_type size = 0;
	bfd_vma str_addr;

	int symengineCode = symengine.open(vmlinux_dir.toAscii().data());

	if ((SymbolEngine::OKAY == symengineCode) ||
			(SymbolEngine::NO_SYMBOLS == symengineCode) ||
			SymbolEngine::NO_SYMBOL_TABLE == symengineCode) {

		symengine.getSecBase (&str_addr, ".text");
		symengine.getSecSize (&size, ".text"); 
		bfd_vma end_addr = str_addr + size;
		snprintf(buf2, (LONG_STR_MAX-1), LONG_FORMAT, (long unsigned int)str_addr);
		snprintf(buf3, (LONG_STR_MAX-1), LONG_FORMAT, (long unsigned int)end_addr);
		snprintf(buf, (LONG_STR_MAX * 2), "%s,%s", buf2, buf3);
	}

	range = buf;
	symengine.closeEngine();
	return range;
}


/* NOTE:
* Here, we write 1 to /dev/oprofile/dump, and then we MUST check
* if the dump is completed by checking if daemon write 1 to
* /var/lib/oprofile/complete_dump
*/
void oprofile_interface::do_dump ()
{
	QString complete_dump = "/var/lib/oprofile/complete_dump";
	QFileInfo complete_dump_info (complete_dump);
	QDateTime last_dump;
	QDateTime cur_dump;

	timespec temp;
	temp.tv_sec = 0;
	temp.tv_nsec = FIVE_HUNDRED_MS;

	//---------------------------
	// Get previous complete_dump time
	if(complete_dump_info.exists())
		last_dump = complete_dump_info.lastModified();
	else
		last_dump = QDateTime::currentDateTime();

	//---------------------------
	// Write 1 to /dev/oprofile/dump
	if (! set_param ("dump", 1)) 
		QMessageBox::critical(qApp->activeWindow(), "Writing error", "There was a problem "
		"writing a block to the dump");
	//---------------------------
	// Wait for complete_dump to be refresh.

	// Max wait time = 3 sec	
	int i = 6; 
	while(1)
	{
		complete_dump_info.refresh();
		cur_dump = complete_dump_info.lastModified();
		if (cur_dump.isValid() && (cur_dump > last_dump))
			break;
		else
			if(i != 0) {	
				nanosleep (&temp, NULL);
				i--;
			} else {
				break;	
			}		
	}
}


void oprofile_interface::do_resume_daemon()
{
	/* NOTE:
	* To Resume, write 1 to /dev/oprofile/enable file
	*/
	set_param("enable", 1);
}


/* Pause the daemon */
void oprofile_interface::do_pause_daemon ()
{
	/* NOTE:
	* To Resume, write 0 to /dev/oprofile/enable file
	*/
	set_param("enable", 0);
}


bool oprofile_interface::remove_lock_file()
{
	bool rt = false;

	QStringList cmd_OPROFILE_RMLOCK;
	cmd_OPROFILE_RMLOCK.append(CA_OPROFILE_CONTROLLER);
	cmd_OPROFILE_RMLOCK.append("--rmlock");
	if (exec_sudo_command(cmd_OPROFILE_RMLOCK) == 0)	
	{
		rt = true;
	}
	return rt;
}


/* Note: The logic in this function should be similar to opcontrol script*/
void oprofile_interface::do_kill_daemon ()
{
	QFile lock (LOCK_FILE);
	QStringList cmd_pidof;
	cmd_pidof.append(PIDOF);
	cmd_pidof.append("oprofiled");
	bool isDaemonRunning = false;
	QString daemonPid;

	// Sanity check if daemon is running.
	if (exec_command(cmd_pidof) == 0)
	{
		isDaemonRunning = true;
		daemonPid = m_stdout.simplifyWhiteSpace();
	}

	/* NOTE:
	* We have to handle these scenario
	* 1.1 No Daemon, No Lock, No Daemon Started
	* 1.2 No Daemon, No Lock, Daemon Started
	* 2.  No Daemon, Have Lock
	* 3.  Have Daemon, Have different lock, Daemon Started
	* 4.  Have Daemon, No Lock
	* 5.  Have Daemon, Have Lock
	*/ 

	// 1.2 No Daemon, No Lock, No Daemon Started
	if (!isDaemonRunning && !lock.exists() && !m_isDaemonStarted)
		return;

	// 1.2 No Daemon, No Lock, Daemon Started
	if (!isDaemonRunning && !lock.exists() && m_isDaemonStarted)
	{
		QMessageBox::warning(qApp->activeWindow(), "Oprofile Daemon Warning",
			QString("Warning:\n") +
			"Oprofile daemon died unexpectedly.\n" +
			"Profile result might be inaccurate.\n" +
			"Please rerun the profile");
		m_isDaemonStarted = false;
		return; 
	}

	// 2. No Daemon, Have Lock 
	if (!isDaemonRunning && lock.exists()) {
		QMessageBox::warning(qApp->activeWindow(), "Oprofile Daemon Warning",
			QString("Warning:\n") +
			"Oprofile daemon died unexpectedly.\n" +
			"Profile result might be inaccurate.\n" +
			"Please rerun the profile");

		remove_lock_file();
		m_isDaemonStarted = false;
		return;
	}

	// 3 Have Daemon, Have different lock, Daemon Started 
	unsigned int cur_daemon_pid;
	if (m_isDaemonStarted
	&&  get_daemon_pid(cur_daemon_pid)
	&&  cur_daemon_pid != m_daemon_pid) {
		QMessageBox::warning(qApp->activeWindow(), "Oprofile Daemon Warning",
			QString("Warning:\n") +
			"A different Oprofile daemon is running.\n" +
			"Profile result might be inaccurate.\n" +
			"Please rerun the profile ("+
			QString::number(cur_daemon_pid)+","+
			QString::number(m_daemon_pid)+")");
		return;
	}
	
	// 4. Have Daemon, No Lock. This is a bad case, we can't kill it.
	if (!lock.exists()) {
		QMessageBox::warning(qApp->activeWindow(), "Oprofile Daemon Warning",
			"Warning:\nLock file does not exist but Oprofile daemon is running.");
	}

	// START KILLING DAEMON

	m_isDaemonStarted = false;

	// First, dump samples
	if (isDaemonRunning)
		do_dump(); 

	// Second, stop daemon and driver
	if (isDaemonRunning)
		do_pause_daemon(); 

	QStringList cmd_OPROFILED_TERM;
	cmd_OPROFILED_TERM.append(CA_OPROFILE_CONTROLLER);
	cmd_OPROFILED_TERM.append("--term");

	QStringList cmd_OPROFILED_KILL;
	cmd_OPROFILED_KILL.append(CA_OPROFILE_CONTROLLER);
	cmd_OPROFILED_KILL.append("--kill");

	if(exec_sudo_command (cmd_OPROFILED_TERM) == -1)
	{
		QMessageBox::warning(qApp->activeWindow(), "Oprofile Daemon Warning",
			"Warning:\nca_oprofile_controller return error. "
			"Oprofile daemon might have exitted unexpectedly, "
			"please rerun the profile.");
		return;
	}

	/* NOTE: 
	* Give oprofile daemon 20 seconds to exit before killing.
	* IBS generates much more data and 20 seems to be appropriate.
	* The number may need adustment */

	int i = 20;
	timespec temp;

	temp.tv_sec = 1;
	temp.tv_nsec = 0;
	while (exec_command(cmd_pidof) != -1) {
		if (0 == i) {
			exec_sudo_command(cmd_OPROFILED_KILL);
			break;
		}
		exec_sudo_command(cmd_OPROFILED_TERM);
		//		fprintf(stderr,"DEBUG: Stopping Daemon count down %d\n",i);
		nanosleep (&temp, NULL);
		i--;
	}

	/* NOTE:
	* At this point there should be no daemon running, we might still have
	* stale lock.  MUST REMOVE IT.
	*/
	if(lock.exists())
	{
		if(!remove_lock_file())
			QMessageBox::warning(qApp->activeWindow(), "CodeAnalyst Warning",
			"Warning:\n"
			"Cannot remove /var/lib/oprofile/lock");
	}
}


void oprofile_interface::customEvent (QEvent * pCustEvent)
{
	Q_UNUSED (pCustEvent);
}


QString oprofile_interface::simplifyPath(QString path)
{
	QString ret = path;
	while(ret.contains("//"))
		ret = ret.replace("//","/");
	return ret;	
}


bool oprofile_interface::havePermissionToProfile()
{
	bool ret = true;

	/* NOTE
	 * We use ca_oprofile_controller --help
	 * to check for permission to run profile.
	 */
	QStringList cmd;
	cmd.append (CA_OPROFILE_CONTROLLER);
	cmd.append ("--help");
	
	if(exec_sudo_command (cmd) == -1) {
		ret = false;
	}

	return ret;
}


int oprofile_interface::checkIbsSupportInDaemon()
{
	QString command;

	if (m_ibsVersion != 0)
		goto out;
	
	// CA-OProfile-0.9.3
	command = QString(OP_BINDIR) +
			"/opcontrol --help 2>&1 " +
			"| grep ibs-fetch 2> /dev/null > /dev/null";
	
	if (system(command.toAscii().data()) == 0) {
		m_ibsVersion = 1;
		goto out;
	}

	// OProfile-0.9.5
	command = QString(OP_BINDIR) +
			"/oprofiled --help 2>&1 " +
			"| grep ext-feature 2> /dev/null > /dev/null";
	if (system(command.toAscii().data()) == 0) {
		m_ibsVersion = 2;
		goto out;
	}
	
	m_ibsVersion = -1;
out:
	return m_ibsVersion;
}


int oprofile_interface::checkDispatchOpInDaemon()
{
	bool ret = false;
	QString command;

	if (m_hasDispatchOpInDaemon != 0)
		goto out;
	
	// CA-OProfile-0.9.3
	command = QString(OP_BINDIR) +
			"/opcontrol --help 2>&1 " +
			"| grep ibs-op-dispatch-op 2> /dev/null > /dev/null";
	
	ret =  (system(command.toAscii().data()) == 0)? true: false;
	if (ret)
		m_hasDispatchOpInDaemon = 1;
	else
		m_hasDispatchOpInDaemon = -1;
out:
	return m_hasDispatchOpInDaemon;
}


int oprofile_interface::checkDCMissInfoInDaemon() 
{
	bool ret = false;
	QString command;
	
	if (m_hasDCMissInfoInDaemon != 0)
		goto out;
	
	// CA-OProfile-0.9.3
	command = QString(OP_BINDIR) +
			"/opcontrol --help 2>&1 " +
			"| grep ibs-op-dcmissinfo-enable 2> /dev/null > /dev/null";
	
	ret =  (system(command.toAscii().data()) == 0)? true: false;
	if (ret)
		m_hasDCMissInfoInDaemon = 1;
	else 
		m_hasDCMissInfoInDaemon = -1;
out:
	return m_hasDCMissInfoInDaemon;
}


bool oprofile_interface::checkMuxSupportInDaemon()
{
	// NOTE: This check allows only the CodeAnalyst Oprofile
	//       to use MUX.
	QString command = QString(OP_BINDIR) +
			"opcontrol --help 2>&1 " +
			"| grep multiplexing 2> /dev/null > /dev/null";
	return ((system(command.toAscii().data()) == 0)? true: false);
}


bool oprofile_interface::checkMuxSupportInDriver()
{
	QFile mux_ctrl(QString("/dev/oprofile/")+ MUX_DRIVER_INTERFACE);
	return ((mux_ctrl.exists())? true: false);
}


bool oprofile_interface::is_driver_loaded()
{
	QString command = QString("grep oprofile /proc/modules 2> /dev/null > /dev/null");
	
	return ((system(command.toAscii().data()) == 0)? true: false);
}


bool oprofile_interface::is_oprofilefs_mounted()
{
	QString command = QString("grep oprofilefs /etc/mtab 2> /dev/null > /dev/null");
	
	return ((system(command.toAscii().data()) == 0)? true: false);
	
}


bool oprofile_interface::checkWatchdogEnabled()
{
	bool ret = false;

	QFile watchdog_file("/proc/sys/kernel/nmi_watchdog");
	if (!watchdog_file.exists())
		return false;

	if (!watchdog_file.open (QIODevice::ReadOnly)) {
		m_stderr = QString("CodeAnalyst Error!.\n")
			+ "There was a problem openning /proc/sys/kernel/nmi_watchdog.";
		return false; 
	}

	char rdBuf = '\0';
	// Read the enable bit
	if (-1 == watchdog_file.readBlock (&rdBuf, sizeof (rdBuf))) {
		m_stderr = QString("CodeAnalyst Error.\n")
		+ "There was a problem reading from the /proc/sys/kernel/nmi_watchdog.";
		watchdog_file.close ();
		return false;
	}

	// Check if enabled
	if( rdBuf == '1' )
		ret = true;
		
	watchdog_file.close ();
	return ret;
}


bool oprofile_interface::checkOprofileDriverStatus()
{
	bool ret = false;

	// Check if driver is loaded
	ret = is_driver_loaded();

	// Check if OProfile filesystem is mounted
	if (ret) ret = is_oprofilefs_mounted();

	return ret;
}


bool oprofile_interface::loadOprofileDriver()
{
	bool ret = true;
	QStringList cmd_LOAD;
	cmd_LOAD.append (CA_OPROFILE_CONTROLLER);
	cmd_LOAD.append ("--load-drv");
	
	if (exec_sudo_command (cmd_LOAD) != 0) {
		m_stderr =  QString("Error:\n")
			+ "ca_oprofile_controller could not load OProfile driver.";
		QMessageBox::critical(qApp->activeWindow(), "CodeAnalyst Error",
			m_stderr);
		ret = false;
	}
	return ret;
}


bool oprofile_interface::set_css(unsigned int cssDepth, 
				unsigned int cssInterval,
				unsigned int cssTgid,
				unsigned int cssBitness)
{
	if(!set_param ("ca_css_depth"   , cssDepth)) goto errOut;
	if(!set_param ("ca_css_interval", cssInterval)) goto errOut;
	if(!set_param ("ca_css_tgid"    , cssTgid)) goto errOut;
	if(!set_param ("ca_css_bitness" , cssBitness)) goto errOut;
	return true;

errOut:
	return false;
	
}

bool oprofile_interface::checkCssSupportInDaemon()
{
	// TODO: [Suravee] This is to be defined
	return true;
}


bool oprofile_interface::checkCssSupportInDriver()
{
	QFile btDepth    (MOUNT + "/ca_css_depth");
	QFile btInterval (MOUNT + "/ca_css_interval");
	QFile btTgid     (MOUNT + "/ca_css_tgid");
	QFile btTgidBitness(MOUNT + "/ca_css_bitness");
	return ((btDepth.exists() 
		&& btInterval.exists() 
		&& btTgid.exists())? true: false);
}

QString oprofile_interface::getIbs2OprofiledArgString()
{
	//********************************************************
	// IBS2
	// --ext-feature=ibs:fetch:ev1,ev2,...,evN:fetch_count:fetch_um|op:ev1,ev2,...,evN:op_count:op_um

	QString ret	= "";
	QString fetch	= "fetch:";
	QString op	= "op:";
	unsigned long fetchCount = 0;
	unsigned int  fetchUmask = 0;
	unsigned long opCount	 = 0;
	unsigned int  opUmask	 = 0;
	
	PerfEventList::iterator it     = m_pEventContainer->getPerfEventListBegin();	
	PerfEventList::iterator it_end = m_pEventContainer->getPerfEventListEnd();	
	for (; it != it_end; it++) {
		if ((*it).type() == PerfEvent::IbsFetch) {
			if (fetchCount != 0) fetch += ",";
			fetch += (*it).opName;
			fetchCount = (*it).count;
			fetchUmask  = (*it).umask();
		} else if ((*it).type() == PerfEvent::IbsOp) {
			if (opCount != 0) op += ",";
			op += (*it).opName;
			opCount = (*it).count;
			opUmask	= (*it).umask();
		}
	}

	if (fetchCount != 0 || opCount != 0)
		ret = "--ext-feature=ibs:";

	if (fetchCount != 0) {
		ret += fetch + ":" + QString::number(fetchCount,10) + 
				":" + QString::number(fetchUmask,10);
		ret += QString("\\|");
	}

	if (opCount != 0) {
		ret += op + ":" + QString::number(opCount,10) + 
				":" + QString::number(opUmask,10);
	}

	return ret;
}


bool oprofile_interface::checkIbsSupportInDriverAndDaemonOk()
{
	if (!isCpuIbsOk ())
	{
		QMessageBox::critical (qApp->activeWindow(), "CodeAnalyst Error",
			"Your processor does not support IBS profile.");
		return false;
	}

	if (!isDriverIbsOk ())
	{
		QMessageBox::critical (qApp->activeWindow(), "CodeAnalyst Error",
			"OProfile driver does not support IBS profile.");
		return false;
	}

	if (checkIbsSupportInDaemon() < 0 )
	{
		QMessageBox::critical (qApp->activeWindow(), "CodeAnalyst Error",
			QString("Current OProfile daemon does not support IBS profile\n")
			+ "or incompatible version of IBS support with the GUI.");
		return false;
	}
	return true;
}


bool oprofile_interface::isDriverEnabled()
{
	bool ret = false;

	QFile enable_file(MOUNT + "/enable");
	if (!enable_file.open (QIODevice::ReadOnly)) {
		m_stderr = QString("Oprofile Driver error.\n")
			+ "There was a problem openning /dev/oprofile/enable.";
		return false; 
	}

	char rdBuf = '\0';
	// Read the enable bit
	if (-1 == enable_file.readBlock (&rdBuf, sizeof (rdBuf))) {
		m_stderr = QString("Oprofile Driver error.\n")
		+ "There was a problem reading from the /dev/oprofile/enable.";
		enable_file.close ();
		return false;
	}

	// Check if enabled
	if( rdBuf == '1' )
		ret = true;
		
	enable_file.close ();
	return ret;
}
