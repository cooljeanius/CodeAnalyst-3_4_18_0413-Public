// CodeAnayst User Manager to manage group amdca

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2008 Advanced Micro Devices, Inc.
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

#include <iostream>

#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdatetime.h>
#include <QFileInfo>
#include <QProcess>

#include <popt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>


#include "config.h"

using namespace std;

static char* add	= NULL;
static char* del	= NULL;
static int removeAll 	= 0;

namespace CA_USER_MANAGER {
	string CA_OPROFILE_CONTROLLER = 
		string(CA_SBIN_DIR) + string("/ca_oprofile_controller");
	string ETC_GROUP("/etc/group");
	string ETC_GROUP_TMP("/etc/group.tmp");
	
	QString amdca_old;
	QString amdca_new;
	QStringList amdcaList;

	QFile group_file(ETC_GROUP.c_str());
	QFile group_tmp_file(ETC_GROUP_TMP.c_str());
	
	QTextStream group_stream;
	QTextStream group_tmp_stream;

	QDateTime group_lastmod;
};

using namespace CA_USER_MANAGER;


enum {
	OPT_ADD = 1,
	OPT_DEL,
	OPT_REM,
	OPT_VERSION,
	OPT_HELP
		
};

static struct poptOption options[] = {
{ "add"     , 'a', POPT_ARG_STRING, &add , OPT_ADD, "", NULL },
{ "delete"  , 'd', POPT_ARG_STRING, &del , OPT_DEL, "", NULL },
{ "remove-all",'r' , POPT_ARG_NONE , &removeAll, OPT_REM, "", NULL },
{ "version" , 'v', POPT_ARG_NONE, NULL, OPT_VERSION, "", NULL },
{ "help"    , 'h', POPT_ARG_NONE, NULL, OPT_HELP, "", NULL },
{ NULL, 0, 0, NULL, 0, NULL, NULL },
};

void show_version() 
{
        fprintf(stdout,"ca_user_manager for %s\n",PACKAGE_STRING);
        fprintf(stdout,"  Built on : %s %s\n",__DATE__,__TIME__);
        fprintf(stdout,"  Contacts : %s \n",PACKAGE_BUGREPORT);
        fprintf(stdout,"  Configure detail :\n  %s\n",CA_CONFIG_ARG);
}

void show_usage() 
{
	printf( " Usage:\n"
		"       ca_user_manager [-a|--add|-d|--delete] | [-r|-remove-all]\n"
		"                       [-v|--version] | [-h|--help]\n"
		"\n"
		"    -a|--add=<list of users>      Add comma-separated list of users to amdca group.\n"
		"    -d|--delete=<list of users>   Delete comma-separated list of users from amdca group.\n"
		"    -r|--remove-all               Delete all users from amdca group.\n"
		"    -v|--version                  Show version number.\n"
		"    -h|--help                     Show this message.\n"
		"\n"
		" Description :\n"
		"     Use this utility to specify a set of users that allowed to use CodeAnalyst.\n"
		"\n"
		" NOTE :\n"
		"     - Please run as root\n"
		"     - This utility will create group \"amdca\", and add users to it.\n"
		"     - This utility will modify /etc/group and /etc/sudoers.\n");
}

int do_options(int argc, char const * argv[])
{
	poptContext optcon;
	int c = 0, ret = 0;

	optcon = poptGetContext(NULL, argc, argv, options, 0);

	if (argc < 2) {
		show_usage();
		ret = -1;
		goto out;
	}

	while ( (c = poptGetNextOpt(optcon)) != -1  ) {
		switch (c) {
		case OPT_ADD:	
		case OPT_DEL:
			if (removeAll != 0) {
				fprintf(stderr,
					"Error: Invalid arguments. "
					"--remove-all cannot be used with other options.\n");	
				exit(1);	
			}
			break;	
		case OPT_REM:	
			if (add || del) {
				fprintf(stderr,
					"Error: Invalid arguments. "
					"--remove-all cannot be used with other options.\n");	
				exit(1);	
			}
			break;	
		case OPT_HELP:
			show_usage();
			exit(0);
			break;	
		case OPT_VERSION:
			show_version();
			exit(0);
			break;	
		default:
			fprintf(stderr,"Error: Invalid arguments.\n");	
			show_usage();
			exit(1);
			break;
		}
	}
	poptFreeContext(optcon);
out:
	return ret;
}


void print_finish()
{
	printf( "\n"
		"* ca_user_manager: Successfully updated CodeAnalyst users.\n" 
		"*                  Users need to re-login for the permission to take effect.\n\n");
}


void cleanup_exit_error(int ret)
{
	if(group_tmp_file.isOpen()) group_tmp_file.remove();	
	if(group_file.isOpen()) group_file.close();

	exit(ret);
}


int create_etc_group_amdca_backup()
{
	bool ret = false;
	fprintf(stdout,".... Creating /etc/group.amdca.bak : ");

	string cmd("cp /etc/group /etc/group.amdca.bak");

	
	if (0 == system(cmd.c_str())) {
		fprintf(stdout," OK\n");
		ret = true;
	} else	
		fprintf(stdout," NO\n");
	return ret;	
}


bool check_amdca_group()
{
	bool ret = false;
	char cmd[] = {"/usr/bin/getent group amdca > /dev/null"};
	
	fprintf(stdout, ".... Check group \"amdca\" : ");
	if ( system(cmd) == 0 ) {
		fprintf(stdout, "OK");
		ret = true;
	} else {
		fprintf(stdout, "NO");	
		ret = false;
	}

	fprintf(stdout, "\n");
	return ret;
}


bool create_amdca_group()
{
	bool ret = false;
	char cmd[] = {"/usr/sbin/groupadd -r amdca > /dev/null"};
	
	fprintf(stdout, ".... Adding group \"amdca\" : ");
	if ( system(cmd) == 0 ) {
		fprintf(stdout, "OK");
		ret = true;
	} else {
		fprintf(stdout, "FAILED");	
		ret = false;
	}

	fprintf(stdout, "\n");
	return ret;
}

#if 0
int verify_user_sudoers(const char* user)
{
	int ret = 1;
	fprintf(stdout,".... Verifying sudoer for user %s : ",user);
	string cmd("sudo -u ");
	cmd.append(user);
	cmd.append(" sudo -S ");
	cmd.append(CA_OPROFILE_CONTROLLER.toAscii().data());
	cmd.append(" --help 0> /dev/null > /dev/null 2>&1");

	//fprintf(stderr,"DEGUG cmd = %s\n",cmd.c_str());

	ret = system(cmd.c_str());
	if (ret == 0)
		fprintf(stdout," OK\n");
	else	
		fprintf(stdout," NO\n");
	return ret;	
}
#endif

int verify_amdca_sudoers()
{
	int ret = 1;
	fprintf(stdout,".... Check amdca in /etc/sudoer : ");

	string cmd("fgrep \"%amdca ALL= NOPASSWD: ");
	cmd.append(CA_OPROFILE_CONTROLLER.c_str());
	cmd.append("\" /etc/sudoers > /dev/null 2>&1");

	//fprintf(stderr,"DEGUG cmd = %s\n",cmd.c_str());

	ret = system(cmd.c_str());
	if (ret == 0)
		fprintf(stdout," OK\n");
	else	
		fprintf(stdout," NO\n");
	return ret;	
}

int create_amdca_sudoer()
{
	int count = 0;
	int ret = 1;
	fprintf(stdout, ".... Updating /etc/sudoers file : ");
	fflush(stdout);
	while(QFile::exists("/etc/sudoers.tmp")) {
		fprintf(stdout,".");
		fflush(stdout);
		sleep(1);	
		if(count++ >= 10) {
			fprintf(stderr,"\n\nError: Failed to write to /etc/sudoers file.\n\n");
			cleanup_exit_error(1);
		}
	}

	string cmd("echo \"%amdca ALL= NOPASSWD: ");
	cmd.append(CA_OPROFILE_CONTROLLER.c_str());
	cmd.append("\" >> /etc/sudoers");

	ret = system(cmd.c_str());
	if(ret == 0)
		fprintf(stdout,"OK\n");
	else
		fprintf(stdout,"NO\n");
	return ret;
}

bool user_exists(const char* user)
{
	bool ret = false;
	string cmd("getent passwd ");
	cmd.append(user);
	cmd.append(" > /dev/null");

	if(system(cmd.c_str()) == 0)
		ret = true;
	return ret;	
}

int check_user_in_amdca(const char* user)
{
	string cmd("groups ");
	cmd.append(user);
	cmd.append(" | grep amdca > /dev/null 2>&1");

	return system(cmd.c_str());
}

bool get_old_amdca()
{
	bool ret = false;

	QStringList cmd;
	cmd.append ("group");
	cmd.append ("amdca");

	QProcess proc;
	proc.start(QString("getent"), cmd);
	proc.setReadChannel(QProcess::StandardOutput);
	proc.waitForReadyRead(-1);

	// Read first line of stdout	
	amdca_old = QString(proc.readAllStandardOutput()).simplified();
	if(amdca_old.length()!= 0)
		ret = true;

	QString tmp = amdca_old.section(":",3,3);
	amdcaList = tmp.split(",");

	proc.waitForFinished(-1);
	return ret;
}

int open_files_and_get_mod_time()
{
	QFileInfo fileInfo;

	if(group_tmp_file.exists()) {
		fprintf(stderr,"Error: %s exists.\n",ETC_GROUP_TMP.c_str());
		cleanup_exit_error(1);
	}
	if(!group_file.open(QIODevice::ReadOnly)) {
		fprintf(stderr,"Error: Cannot open %s .\n",ETC_GROUP.c_str());
		cleanup_exit_error(1);
	}
	fileInfo.setFile(group_file);
	group_lastmod = fileInfo.lastModified();
	
	if(!group_tmp_file.open(QIODevice::ReadWrite)) {
		fprintf(stderr,"Error: Cannot open %s .\n",ETC_GROUP_TMP.c_str());
		cleanup_exit_error(1);
	}

	group_tmp_stream.setDevice(&group_tmp_file);

	return 0;
}


int write_tmp()
{
	QString tmp;
	char buf[1024];

	while( (group_file.readLine(buf,1024) != -1) )
	{
		tmp = QString(buf);
		// Check for amdca group and replace 
		// with new group line
		if(tmp.startsWith("amdca:"))
		{
			// Form new string.
			tmp = amdca_new;
		}
		group_tmp_stream << tmp;
	}

	return 0;
}


// Now we reopen the group file for writing, check modification time, 
// copy from tmp, close it, verify sudo for each user and remove the tmp.
int finalize()
{
	QString tmp;
	char buf[1024];
	QFileInfo fileInfo;

	// Close
	group_file.close();

	// Check modification time
	fileInfo.setFile(QString(ETC_GROUP.c_str()));
	if(group_lastmod != fileInfo.lastModified()) {
		fprintf(stderr,"Error: %s has been modified by other.\n",ETC_GROUP.c_str());
		cleanup_exit_error(1);
	}

	// Reopen trancate for writing
	if(!group_file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
		fprintf(stderr,"Error: Cannot open %s for writing .\n",ETC_GROUP.c_str());
		cleanup_exit_error(1);
	}

	fprintf(stdout,".... Writing to /etc/group : ");
	// Copy /etc/group.tmp to /etc/group
	group_stream.setDevice(&group_file);
	
	group_tmp_stream.flush();
	group_tmp_stream.seek(0);

	while (!group_tmp_stream.atEnd()) {
		QString line = group_tmp_file.readLine();
		group_stream << line;	
	}
	fprintf(stdout,"OK\n");

	if(group_tmp_file.isOpen()) group_tmp_file.remove();	
	if(group_tmp_file.isOpen()) group_file.close();

	if (verify_amdca_sudoers() != 0)
		create_amdca_sudoer();	

#if 0 // Some how dosen't work
	// Verify if users can run sudoers
	QStringList::iterator it  = amdcaList.begin();
	QStringList::iterator end = amdcaList.end();

	if (it != end && verify_user_sudoers((*it).toAscii().data()) != 0)
	if (verify_amdca_sudoers() != 0)
		create_amdca_sudoer();	
	else
		it++;

	for(; it != end ; it++)
	{
		if (verify_user_sudoers((*it).toAscii().data()) != 0)
		{
			fprintf(stderr,"Error: Failed to verify sudo for user %s\n",(*it).toAscii().data());
			return 1;
		}
	}
#endif 
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int add_users()
{
	QStringList addList = QString(add).split(",");

	if(addList.size() == 0)
	{
		fprintf(stderr,"Error: No user to add.\n");
		cleanup_exit_error(1);
	}

	// Form new group line
	amdca_new = amdca_old;

	/*
 	 * For each user:
 	 */
	QStringList::iterator it     = addList.begin();
	QStringList::iterator it_end = addList.end();
	for(; it != it_end ; it++)
	{
		fprintf(stdout,".... Adding user %s :",(*it).toAscii().data());
		fflush(stdout);
		// Check if user exists
		if(!user_exists((*it).toAscii().data())) {
			fprintf(stderr,"\nWarning: User %s not exists.\n",
				(*it).toAscii().data());
			continue;
		}

		/////////////////////////////////////////////////////
 	 	// 1. Add user to group amdca
		if(-1 == amdcaList.indexOf(*it)) {
			amdcaList.push_back(*it);
			if(amdca_new.endsWith(":") || amdca_new.endsWith(","))
				amdca_new += (*it); 
			else 
				amdca_new += QString(",") + (*it); 
			fprintf(stdout,"OK\n");
			fflush(stdout);
		
		} else {
			fprintf(stderr,"\nWarning: Duplicate user %s in amdca group.\n", 
				(*it).toAscii().data());
		}
	}
	amdca_new += "\n";
	return 0;
}

int delete_users(bool deleteAll)
{
	QStringList delList;

	if(deleteAll) 
		delList = amdcaList;
	else
		delList = QString(del).split(",");

	if(delList.size() == 0)
	{
		fprintf(stderr,"Error: No user to delete.\n");
		cleanup_exit_error(1);
	}

	/*
 	 * For each user:
 	 */
	QStringList::iterator it     = delList.begin();
	QStringList::iterator it_end = delList.end();
	for(; it != it_end ; it++)
	{
		fprintf(stdout,".... Deletinging user %s :",(*it).toAscii().data()); 
		fflush(stdout);
		// Check if user exists
		if(!user_exists((*it).toAscii().data())) {
			fprintf(stderr,"\nWarning: User %s not exists.\n",
				(*it).toAscii().data());
			continue;
		}

 	 	// Delete user from group amdca 
		int ind;
		if(-1 != (ind = amdcaList.indexOf((*it).toAscii())))
		{
			amdcaList.removeAt(ind);
			fprintf(stdout,"OK\n");	
			fflush(stdout);
		} else {
			fprintf(stderr,"\nWarning: Cannot delete user %s. Not found in amdca.\n", 
					(*it).toAscii().data());
		}			
	}

	// Form new group line
	amdca_new = amdca_old.section(":",0,2) + ":" + amdcaList.join(",") + "\n";

	return 0;
}

int main(int argc, const char* argv[])
{
	int ret = 1;

	// Check if root user
	if(0 != getuid()){
		fprintf(stderr,"ca_oprofile_controller: Error, must be root\n");
		goto out;
	}

	// Parse options
	if (0 > (ret = do_options(argc, argv)))
		goto out;

	//////////////////////////////////////////////////////////
	// Create /etc/group.amdca.bak if not already exist
	if(!QFile::exists("/etc/group.amdca.bak") 
	&& !create_etc_group_amdca_backup())
		goto out;
	
	// BY DEFAULT: Create amdca group
	if(!check_amdca_group() && !create_amdca_group())
	{
		goto out;	
	}

	// Open and lock files to prevent writing
	open_files_and_get_mod_time();

	// Get existing list of users in amdca group
	if(!get_old_amdca())
	{
		fprintf(stderr,"Error: amdca group not exists.\n");
		cleanup_exit_error(1);
	}

	//////////////////////////////////////////////////////////
	// Handle Add
	if(add)
	{
		ret = add_users();
		if(ret != 0)
			goto out;
	}
	
	// Handle Delete
	if(del)
	{
		ret = delete_users(false);
		if(ret != 0)
			goto out;	
	}
	
	// Handle Remove All
	if(removeAll)
	{
		ret = delete_users(true);
		if(ret != 0)
			goto out;	
	}
	
	write_tmp();
	ret = finalize();
	if(ret == 0 )
		print_finish();
out:
	exit(ret);

}

#if 0
int remove_all_users_from_sudoer()
{
	//
	// Using this one-liner command:
	// sed -e 's/.* \/opt\/CodeAnalyst\/sbin\/ca_oprofile_controller//' /tmp/sudoers
	//
	string cmd("sed -e 's/.* ");
	QString tmp(CA_OPROFILE_CONTROLLER);
	tmp.replace(QString("/"),QString("\\/"));
	cmd.append(tmp.toAscii().data());
	cmd.append("//' /etc/sudoers > /tmp/sudoers");	
	
	return system(cmd.c_str());
}

int remove_all_users_from_amdca()
{
	//
	// Using this one-liner command:
	// sed -e 's/^amdca:x:\([0-9]*\).*/amdca:x:\1:/' /etc/group > /etc/group.tmp 
	//
	string cmd("sed -e 's/^amdca:x:\\([0-9]*\\).*/amdca:x:\\1:/' /etc/group > /tmp/group");
	
	return system(cmd.c_str());
}
#endif

