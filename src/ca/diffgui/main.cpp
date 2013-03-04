
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

#include "unistd.h"
#include <qapplication.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qtextstream.h>
#include "DiffAnalystWindow.h"

void showUsage()
{
	QString usage = "DiffAnalyst Usage :\n"
			"\n"
			"    DiffAnalyst [-h] -[0|1] <CA Session File>:<Task>:<Module>\n"
			"\n"
			"    -o <DiffAnalyst Session file (*.DAW)>\n"
			"        Specify DiffAnalyst Session file.\n"
			"\n"
			"    -0 <CA Session File>:<Task>:<Module\n"
			"        Specify CA session file, task name, and module name\n"
			"        for session 0.\n"
			"\n"
			"    -1 <CA Session File>:<Task>:<Module\n"
			"        Specify CA session file, task name, and module name\n"
			"        for session 1.\n"
			"\n"
			"    -h	Show this message\n";
	fprintf(stderr,"%s\n",usage.data());
}

int main(int argc, char* argv[])
{
	int ret_value = 0;
	QString dawFileName;
	QString diffSessName, session0, session1;

	int c;
	while ((c = getopt (argc, argv, "ho:0:1:")) != -1)
	{
		switch (c) {
		case 'o':
			dawFileName = QString(optarg);
			break;
		case '0':
			session0 = QString(optarg);
			break;
		case '1':
			session1 = QString(optarg);
			break;
		case 'h':
			showUsage();
			return 0;
		default:
			showUsage();
			return 1;
		}
	}

	// Verify session name
	if ((session0.isEmpty() && !session1.isEmpty())
	||  (!session0.isEmpty() && session1.isEmpty()))
	{
		showUsage();
		return 1;
	}
	
	// Verify file existance
	if (!dawFileName.isEmpty() && !QFile::exists(dawFileName)) {
		fprintf(stderr,"DiffAnalyst Error: File \" %s \" does not exist.\n", dawFileName.toAscii().data());
		return 1;
	}

	QApplication a(argc, argv);

	DiffAnalystWindow * mw = new DiffAnalystWindow();

	if(!dawFileName.isEmpty())
		mw->init(dawFileName);
	else
		mw->init(session0, session1);
	
	mw->show();

	a.connect ( &a, SIGNAL (lastWindowClosed()), &a, SLOT (quit()) );
	ret_value =  a.exec();

	return ret_value;

}
