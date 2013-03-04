// $Id: main.cpp,v 1.4 2006/05/15 22:09:22 jyeh Exp $
// holds the main()

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

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <popt.h>
#include <dlfcn.h>

#include <qsplashscreen.h>
#include <QPixmap>

#include "stdafx.h"
#include "helperAPI.h"
#include "application.h"
#include "helperAPI.h"

#include "catranslate_console_display.h"

#define CODEANALYST_SPLASH          "CASplash.png"

static const char * import = NULL;
static const char * proj_dir = NULL;
static const char * proj_name = NULL;
static const char * session_name = NULL;
static const char * session_note = NULL;
static const char * cpuinfo = "/proc/cpuinfo";
static bool quiet = false;

QString importErrorStr;
QString cawStr;

enum {
	OPT_IMPORT = 1,
	OPT_PROJ_NAME,
	OPT_PROJ_DIR,
	OPT_SESSION_NAME,
	OPT_SESSION_NOTE,
	OPT_CPUINFO,
	OPT_QUIET,
	OPT_VERSION,
	OPT_HELP
};

enum {
	LOCAL_IMPORT = 1,
	REMOTE_IMPORT,
	XML_IMPORT
};


static struct poptOption options[] = {
{ "import"        , 'i' , POPT_ARG_STRING, &import    , OPT_IMPORT , "" , NULL },
{ "proj-name"     , 'p' , POPT_ARG_STRING, &proj_name , OPT_PROJ_NAME, "" , NULL },
{ "proj-dir"      , 'd' , POPT_ARG_STRING, &proj_dir  , OPT_PROJ_DIR , "" , NULL },
{ "session-name"  , 'n' , POPT_ARG_STRING, &session_name , OPT_SESSION_NAME, "" , NULL },
{ "session-note"  , 's' , POPT_ARG_STRING, &session_note , OPT_SESSION_NOTE, "" , NULL },
{ "cpuinfo"       , 'c' , POPT_ARG_STRING, &cpuinfo   , OPT_CPUINFO, "" , NULL },
{ "quiet"         , 'q' , POPT_ARG_NONE  , NULL, OPT_QUIET, "" , NULL },
{ "version" , 'v' , POPT_ARG_NONE, NULL, OPT_VERSION , "", NULL },
{ "help"    , 'h' , POPT_ARG_NONE, NULL, OPT_HELP    , "", NULL },
{ NULL, 0, 0, NULL, 0, NULL, NULL },
};


void showVersion()
{
	fprintf(stdout,"%s\n",PACKAGE_STRING);	
	fprintf(stdout,"  Built on : %s %s\n",__DATE__,__TIME__);	
	fprintf(stdout,"  Contacts : %s \n",PACKAGE_BUGREPORT);	
	fprintf(stdout,"  Configure detail :\n  %s\n",CA_CONFIG_ARG);	
}


void show_usage() 
{
	printf( " Usage:\n"
		" 1. Running GUI:\n"
		"       CodeAnalyst -[v|h]\n"
		"    -v|--version   Show version number.\n"
		"\n"
		"    -h|--help      Show this message.\n"
		"\n"
		" 2. Import profiles:\n"
		"       CodeAnalyst -[i|p|d|n|s|c]\n"
		"\n"
		"    -i|--import=<input>\n"
		"                   Import profile session. Input can be one of the following:\n"
		"                   1. Path to OProfile profile directory (i.e. /var/lib/oprofile/samples/current).\n"
		"                   2. Path to capackage.tar.gz\n" 
		"                   3. Path to OProfile XML output from opreport.\n"
		"                   4. Path to CAPerf profile datafile (i.e. /home/amd/AMD/CodeAnalyst/test/test.tbp.dir/test.data).\n"
		"\n"
		"    -p|--proj-name=<name>\n"
		"                   Specify project name.\n"
		"\n"
		"    -d|--proj-dir=<dir> (Default: /<home>/AMD/CodeAnalyst/)\n"
		"                   Specify project directory.\n"
		"\n"
		"    -n|--session-name=<name> (Default:Session)\n"
		"                   Specify session name.\n"
		"\n"
		"    -s|--session-note=<note>\n"
		"                   Specify session note.\n"
		"\n"
		"    -c|--cpuinfo=<Path to cpuinfo> (Default:/proc/cpuinfo)\n"
		"                   Specify cpuinfo file.\n"
		"\n"
		"    -q|--quiet\n"
		"                   Suppress verbose output to standard out.\n"
		"\n"
		);
}


int doOptions(int argc, char ** argv)
{
	poptContext optcon;
	int c = 0, ret = 0;

	optcon = poptGetContext(NULL, argc, (const char**)argv, options, 0);

	if (argc < 2) {
		poptPrintUsage(optcon, stdout, 0);
		ret = -1;
		goto out;
	}

	while ( (c = poptGetNextOpt(optcon)) != -1  ) {
		switch (c) {
		case OPT_IMPORT:	
		case OPT_PROJ_DIR:
		case OPT_PROJ_NAME:
		case OPT_SESSION_NAME:
		case OPT_SESSION_NOTE:
		case OPT_CPUINFO:
			break;	
		case OPT_QUIET:
			quiet = true;
			break;
		case OPT_HELP:
			show_usage();
			exit(0);
			break;	
		case OPT_VERSION:
			showVersion();
			exit(0);
			break;	
		default:
			fprintf(stderr,"Error: Invalid arguments. %s\n", poptBadOption(optcon,0));	
			show_usage();
			exit(1);
			break;
		}
	}

	poptFreeContext(optcon);
out:
	return ret;
}


void startGUI(int argc, char ** argv)
{
	int ret = 0;

	QApplication a (argc, argv);
	a.setQuitOnLastWindowClosed(false);

	QString BinDirectory = QString(CA_DATA_DIR) + "/" + CODEANALYST_SPLASH;
	QSplashScreen splash ( QPixmap(BinDirectory), Qt::WStyle_StaysOnTop);
	splash.show();

	ApplicationWindow * mw = new ApplicationWindow();
	splash.finish (mw);
	mw->setCaption( "Advanced Micro Devices - CodeAnalyst Performance Analyzer" );
	mw->show();

// NOTE: [Suravee] This cause problem in Qt-4.2 due to the QT bug 
//       which emit the lastWindowClosed signal when QMainWindow is closed.
//	a.connect ( &a, SIGNAL (lastWindowClosed()), mw, SLOT (onAboutToQuit()) );

	ret =  a.exec();
}


bool prepareCAWFile(CawFile ** pCawFile) 
{
	QString projDirString;

	if (!proj_name) {
		importErrorStr = "Please specify CodeAnalyst project name to import the session.\n";
		return false;
	}

	if (!proj_dir) {
		// Use the user's home directory
		projDirString = QDir::home ().path();
		projDirString.replace ('\\', '/');
		projDirString += QString("/AMD/CodeAnalyst/") + proj_name;
	} else {
		projDirString = QString(proj_dir);
	}

	// Check proj_dir exists
	QDir projDir(projDirString);
	//Fix for BUG300636
	//Replaced QDir::mkdir with QDir::mkpath to ensure nested directories are created even when the top-level
	//directory doesn't exist in the first place.
	//As per Qt documentation, QDir::isReadable seems to return false even if the path is readable, which is the root cause
	//of BUG300636
	//Extract of Qt doc goes like this "Warning: A false value from this function is not a guarantee that files
	//in the directory are not accessible".  Based on this looping is done to avoid false alarm.
	int attempt = 0;
	do
	{
		++attempt;
		if (!projDir.exists() && !projDir.mkpath(projDirString)) {
			projDir.refresh();
			importErrorStr = QString("Fail to create project directory (") + projDirString + ")\n.";
			//If this fails even after 3 attempts it is an issue really.
			if (attempt == 3)
				return false;
		}
	} while (attempt < 4); 

	
	// Check proj_dir accessible 
	if (!projDir.isReadable()) {
		importErrorStr = QString("Fail to access project directory (") + projDirString + ")\n.";
		return false;
	}
	
	cawStr = projDirString + "/" + proj_name + ".caw";

	*pCawFile = new CawFile (cawStr);
	if (*pCawFile == NULL) {
		importErrorStr = QString("Fail to create CawFile object\n.");
		return false;
	}

	if (!QFile::exists(cawStr)) {
		if (!(*pCawFile)->create()) {
			importErrorStr = QString("Fail to create CAW file ") + cawStr + "\n.";
			return false;
		}
	}

	if (!(*pCawFile)->openCawFile()) {
		importErrorStr = QString("Fail to access CAW file ") + cawStr + "\n.";
		return false;
	}
	return true;
}


bool importProfile(CawFile * pCawFile)
{
	bool ret = false;
	struct stat st;
	QString importStr(import);

	// Check existing of import file
	if (stat(importStr.toAscii().data(), &st) != 0) {
		importErrorStr = QString("Import input (") + importStr + ") does not exist.\n";
		return false;
	}
		
	// Check existing of cpuinfo
	if (stat(cpuinfo, &st) != 0) {
		importErrorStr = QString(cpuinfo) + " does not exist.\n";
		return false;
	}
		

	// Import profile
	ImportController importCtrl(false);
	importCtrl.setCawFile(pCawFile);

	// Setup translation console display
	if (!quiet) {
		ca_translate_display * p_translate_console_display = NULL;
		p_translate_console_display = new ca_translate_console_display();
		importCtrl.setTranslationDisplay(p_translate_console_display);
	}
		
	if (importStr.endsWith(".xml", false)) {
		// XML Import
		ret = importCtrl.onImportXmlFile(importStr, 
						QString(cpuinfo) );
	} else if (importStr.endsWith(".tar.gz", false)) {
		// REMOTE Import
		QString tempDir;
		QString opSamplePath;
		QString cpuinfoPath;

		untar_packed_file(&importStr, &tempDir);

		rename_files(&tempDir, &opSamplePath, &cpuinfoPath);

		// Check existing of packed files 
		if (stat(opSamplePath.toAscii().data(), &st) != 0) {
			importErrorStr = opSamplePath + " does not exist.\n";
			return false;
		}
		if (stat(cpuinfoPath.toAscii().data(), &st) != 0) {
			importErrorStr = cpuinfoPath + " does not exist.\n";
			return false;
		}
	
		ret = importCtrl.onImportLocalRemoteFile(opSamplePath,
						cpuinfoPath,
						false,
						QStringList(),
						true);
	}
#ifdef CAPERF_SUPPORT_ENABLED
        else if (importStr.endsWith(".data", false)) {
                // CAPERF data import
                ret = importCtrl.onImportPerfFile(importStr);
        }
#endif // CAPERF_SUPPORT_ENABLED
        else if (importStr.startsWith("/")) {
		// LOCAL Import
		// TODO: Currently is hardcoded
		ret = importCtrl.onImportLocalRemoteFile(importStr.toAscii().data(),
						QString(cpuinfo),
						false,
						QStringList(),
						false);
	} else {
		importErrorStr = QString("Invalid import input (") + importStr + ").\n";
		return false;
	}

	if(!ret) {
		importErrorStr = importCtrl.getErrorMessage();
	}	

	return ret;	
}


void printSummary(QString name)
{
	fprintf(stdout, "CodeAnalyst Import Summary:\n");
	fprintf(stdout, "    Import  File: %s\n", import);
	fprintf(stdout, "    Project File: %s\n", cawStr.toAscii().data());
	fprintf(stdout, "    Session Name: %s\n", name.toAscii().data());
	if(session_note)
		fprintf(stdout, "    Session Note: %s\n", session_note);
	if(cpuinfo)
		fprintf(stdout, "    cpuinfo     : %s\n", cpuinfo);
		
}

void printError(const char * str)
{
	if (strlen(str) != 0)
		fprintf(stderr, "CodeAnalyst: %s\n", str);
}

int main(int argc, char ** argv) 
{
	int ret = 0;
	CawFile * pCawFile = NULL;
	QString tmpName;
	RUN_SETTING oldSetting; 

	if(argc == 1) {
		startGUI(argc, argv);
		goto out;
	}
 
	// Parse options
	if (0 != (ret = doOptions(argc, argv))) {
		importErrorStr = "Error parsing option.\n";
		ret = -1;
		goto out;
	}

	// Handle Options	
	if (import) {
		// Setting up CAW file
		if (!prepareCAWFile(&pCawFile)) {
			ret = -1;
			goto out;			
		}

		// Handle session_name and session_note
		RUN_SETTING * runSetting = new RUN_SETTING();
		if (!runSetting) {
			fprintf(stderr,"CodeAnalyst Error: Could not configure import session\n");
			return -1;
		}

		if (session_note)
			runSetting->sessionNote = QString(session_note);

		if (session_name)
			tmpName = QString(session_name);
		else
			tmpName = QString("Session");
		
		runSetting->sessionName = tmpName;

		// Find runsetting with this name
		RUN_SETTING * tmpSetting  = pCawFile->getRunSetting (tmpName);
		if (tmpSetting) {
			/* We must not change the existing run setting.
			 * So, we back up old setting with this name.
			 */
			oldSetting = *tmpSetting;
			*tmpSetting = *runSetting;
		} else {
			pCawFile->addRunSetting(tmpName, runSetting);
		}
		pCawFile->setLastRunSetting(tmpName);	
		printSummary(tmpName);

		// Import Profile
		if (!importProfile(pCawFile)) {
			ret = -1;
			goto out;
		}	
	} else {
		importErrorStr = "Error: Please specify import type.\n";
		ret = -1;
		goto out;

	}

	/* Clean up CAW file to remove the temporary setting */
	if (!oldSetting.sessionName.isEmpty()){
		 /* restore old seting */
		RUN_SETTING * tmpSetting = pCawFile->getRunSetting (tmpName);
		*tmpSetting = oldSetting;	
	} else if (!tmpName.isEmpty()) {
		 /* delete temporary seting */
		pCawFile->deleteRunSetting(tmpName);
	}
	pCawFile->saveCawFile();

out:
	if (ret != 0)
		printError (importErrorStr.toAscii().data());
	exit(ret);
}
