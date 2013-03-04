#include <qapplication.h>
#include <q3progressdialog.h>
#include <QEvent>
#include <QTextStream>

#include "../../include/stdafx.h"
#include "catranslate_gui_display.h"

#include "CAQtEvent.h"

//#include "op_config.h"

#ifndef OP_BASE_DIR
#define OP_BASE_DIR "/var/lib/oprofile/"
#endif

#ifndef OP_SAMPLES_DIR
#define OP_SAMPLES_DIR OP_BASE_DIR "samples/"
#endif  

#ifndef OP_LOG_FILE
#define OP_LOG_FILE OP_SAMPLES_DIR "oprofiled.log"
#endif

ca_translate_gui_display::ca_translate_gui_display()
{
    m_evHandler = NULL;
}


ca_translate_gui_display::~ca_translate_gui_display()
{
}


void ca_translate_gui_display::init()
{
}


void ca_translate_gui_display::setDlgEventHandler(QWidget * evHandler)
{
    if (NULL != evHandler) {
        m_evHandler = evHandler;
    }
}

void ca_translate_gui_display::update_display(const char * text)
{
    QString msg = text;
    QApplication::postEvent (m_evHandler, 
        new CAQtEvent ((QEvent::Type)EVENT_UPDATE_PROGRESS,
        (void *) new QString (msg)));
    qApp->processEvents (QEventLoop::ExcludeSocketNotifiers);
}

void ca_translate_gui_display::update_display_error(const char * text)
{
    QString msg = text;
    QApplication::postEvent (m_evHandler, 
        new CAQtEvent ((QEvent::Type)EVENT_UPDATE_ERROR,
        (void *) new QString (msg)));
    qApp->processEvents (QEventLoop::ExcludeSocketNotifiers);
}

void ca_translate_gui_display::translationInProgress(bool b)
{
	QEvent *custEvent = NULL;

	if(b)
		custEvent = new QEvent((QEvent::Type)EVENT_UPDATE_PROGRESS_BEGIN);
	else
		custEvent = new QEvent((QEvent::Type)EVENT_UPDATE_PROGRESS_DONE);

	QApplication::postEvent (m_evHandler, custEvent);
	qApp->processEvents (QEventLoop::ExcludeSocketNotifiers);
}

void ca_translate_gui_display::display_oprofile_log()
{
    QFile * logFile =  new QFile(OP_LOG_FILE);
    QTextStream * logStream = NULL;
    QString tmp = "";
    QString sessionString;
    QStringList sessionStringsList;
    int curCpu = -1;
    bool foundStartLog = false;

    //wait for the log file to be written to.
    //qApp->processEvents (QEventLoop::AllEvents,1000);
    qApp->processEvents (QEventLoop::ExcludeSocketNotifiers);
    if (NULL != logFile && logFile->exists()) {
        logFile->open(QIODevice::ReadOnly);

        logStream = new QTextStream(logFile);

	if (NULL != logStream) {
		QApplication::postEvent (m_evHandler, 
			new CAQtEvent((QEvent::Type)EVENT_UPDATE_PROGRESS, 
			(void *) new QString("\n========== Oprofile Log ==========\n")));

		QString line,line2;
		do
		{
			line = logStream->readLine();
			line2 = line;
			line.append("\n");    

			// "Nr. sample dumps" is the first line that we care.
			// In oprofile 0.9.3, daemon doesn't clear the old log in the file.  
			// Here, we are looking for the last log in the file.
			if (!foundStartLog
			&&  (line.startsWith("Nr. sample dumps") 
			||   line.startsWith("-- OProfile" )))
			{
				foundStartLog = true;
				sessionString = line;
			} else 
				sessionString.append(line);
		} while(!line2.isNull());
        }

        QStringList finalLogList = QStringList::split("\n", sessionString, true);
       
        QStringList::iterator begin = finalLogList.begin(); 
        for (; begin != finalLogList.end(); begin++) {
            QString line = *begin;
            QEvent::Type message = (QEvent::Type) EVENT_UPDATE_PROGRESS;
		
		// Highlight lost information in Red	 
		if (line.contains("lost")) 
		{
			//QString tmp = line.right(line.length() - line.findRev(":") - 2); 
			//if ("0" != tmp) {
				message = (QEvent::Type) EVENT_UPDATE_ERROR;
			//}
		}

		// Add restore new line
		if (line.isEmpty())
			line = QString("\n");

		QApplication::postEvent (m_evHandler, 
                                     new CAQtEvent (message, 
                                         (void *) new QString (line)));
        }

        logFile->close();

        if (NULL != logFile) {
            delete logFile;
            logFile = NULL;
        }

        if (NULL != logStream) {
            delete logStream;
            logStream = NULL;
        }
    }
}
