//$Id: projectfile.cpp,v 1.2 2005/02/14 16:52:29 jyeh Exp $
// Implementation for the CProjectFile class.

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


//Revision history
//$Log: projectfile.cpp,v $
//Revision 1.2  2005/02/14 16:52:29  jyeh
//Updated Header.
//
//Revision 1.1.1.1  2004/10/05 17:03:23  jyeh
//initial import into CVS
//
//Revision 1.6  2004/01/06 23:26:30  franksw
//Removed debugging prints
//
//Revision 1.5  2003/12/15 19:28:21  leiy
//Code clean up after code review
//
//Revision 1.4  2003/12/12 23:04:00  franksw
//Code Review
//
//Revision 1.3  2003/11/17 23:24:40  franksw
//code cleanup
//

#include <qmessagebox.h>
#include <Q3TextStream>

#include "stdafx.h"
#include "projectfile.h"


CProjectFile::CProjectFile()
{
}


CProjectFile::~CProjectFile()
{
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::Open()
* ~~~~~~~~~~~~~~~~~~~~
*/
bool CProjectFile::Open ( char * file_path )
{
	QFile xmlFile ( file_path );
	QXmlInputSource source ( xmlFile );
	QXmlSimpleReader reader;

	m_filePath = file_path;

	/* set our xml handler to do work on the data */
	reader.setContentHandler ( this );

	return reader.parse ( source );
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::startDocument()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
bool CProjectFile::startDocument()
{
	m_elementStack.clear();
	return true;
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::startElement()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
bool CProjectFile::startElement ( const QString & namespaceURI,
								 const QString & localName,
								 const QString & qName, 
								 const QXmlAttributes & atts )
{
	/* There are three main elements in our xml file.
	* 1) <project> -> Begins the document and contains descriptions 
	attributes for the project.
	* 2) <values>	-> contains values for general properties of the 
	project
	* 3) <timer>	-> contains values regarding the timer properties
	* 4) <eventX>	-> Containts values for event0-3
	* 5) <tbs_sessions>
	* 6) <ebs_sessions>
	* 7) <session>
	*/
	Q_UNUSED (namespaceURI);
	Q_UNUSED (localName);

	m_elementStack.push( qName );

	if ("project" == qName) {
		//Do nothing,
	} else if ("values" == qName) {
		m_profileValues.name = atts.value ("name");
		m_profileValues.duration = atts.value ("duration").toULong();
		m_profileValues.bufferSize 
			= atts.value ("bufferSize").toULong();
		m_profileValues.cpus = atts.value( "cpus").toULong();
		m_profileValues.program = atts.value ("program");
		m_profileValues.delay = atts.value ("delay").toULong();
	} else if ("timer" ==  qName) {
		if ("values" == CurrentElement() ) {
			m_profileValues.timerValues.interval 
				= atts.value ("interval").toFloat();
		} else if ("tbs_session" == CurrentElement()) {
			TBS_SESSION_VALUES tsv = m_tbsSessionList.last();
			tsv.timerValues.interval 
				= atts.value("interval").toFloat();
		}
	} else if ("event" == qName  ) {
		DWORD n = atts.value ("num").toULong();
		if ("values" == CurrentElement()) {
			m_profileValues.eventValues[n].comboIndex 
				= atts.value ("combo").toULong();
			m_profileValues.eventValues[n].event
				= atts.value ("event").toULong();
			m_profileValues.eventValues[n].unit
				= atts.value ("unit").toULong();
			m_profileValues.eventValues[n].sourceUnit
				= atts.value ("srcunit");
			m_profileValues.eventValues[n].count
				= atts.value ("count").toULong();
			m_profileValues.eventValues[n].os
				= atts.value ("os").toUInt();
			m_profileValues.eventValues[n].usr
				= atts.value("usr").toUInt();
		} else if ("ebs_session" == CurrentElement()) {
			EBS_SESSION_VALUES esv = m_ebsSessionList.last();
			esv.eventValues[n].comboIndex 
				= atts.value ("combo").toULong();
			esv.eventValues[n].event 
				= atts.value ("event").toULong();
			esv.eventValues[n].unit 
				= atts.value ("unit").toULong();
			esv.eventValues[n].sourceUnit
				= atts.value ("srcunit");
			esv.eventValues[n].count
				= atts.value ("count").toULong();
			esv.eventValues[n].os = atts.value ("os").toUInt();
			esv.eventValues[n].usr = atts.value ("usr").toUInt();
		}
	} else if ( "tbs_sessions" == qName) {
		TBS_SESSION_VALUES tsv;

		tsv.fileName = atts.value ("file");
		tsv.name = atts.value ("name");

		m_tbsSessionList.append( tsv );
	} else if ("ebs_sessions" == qName  ) {
		EBS_SESSION_VALUES esv;

		esv.fileName = atts.value ("file");
		esv.name = atts.value ("name");

		m_ebsSessionList.append( esv );
	} else {
		// unknown element
	}
	return true;
} //CProjectFile::startElement


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::endElement()
*/
bool CProjectFile::endElement ( const QString & namespaceURI, 
							   const QString & localName, 
							   const QString & qName )
{
	Q_UNUSED (namespaceURI);
	Q_UNUSED (localName);
	Q_UNUSED (qName);

	// pop off the last element
	m_elementStack.pop();
	return true;
}


QString CProjectFile::CurrentElement()
{
	return m_elementStack.top();
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::Create()
* ~~~~~~~~~~~~~~~~~~~~~~
* Is is the callees responsibility to fill in the pv structure.
* Once we have it, just do a save
*/
bool CProjectFile::Create ( char * file_path, PROFILE_VALUES pv )
{
	m_profileValues = pv;
	m_filePath = file_path;

	return Save();
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::Save()
* ~~~~~~~~~~~~~~~~~~~~
* Saving a project file involves a series of steps:
*	1) Writing out the persistent project values
*	2) Saving the tbs sessions that have been performed
*	3) Saving the ebs sessions that have been performed
*/
bool CProjectFile::Save()
{
	QFile file ( m_filePath );
	Q3TextStream stream;
	bool bSuccess;

	bSuccess = file.open (QIODevice::Truncate | QIODevice::Text);
	if (bSuccess) {
		stream.setDevice ( &file );

		stream << "<project>" << endl << endl;

		WriteValues ( stream );
		WriteTbsSessions ( stream );
		WriteEbsSessions ( stream );

		stream << "</project>";
	}

	return bSuccess;
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::WriteValues()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~
* Write out the persistent project values
*/
void CProjectFile::WriteValues ( Q3TextStream & stream )
{
	stream << "<values name=\"" << m_profileValues.name.data()
		<< "\"duration=\"" << m_profileValues.duration
		<< "\"buffsize=\"" << m_profileValues.bufferSize	
		<< "\"cpus=\"" << m_profileValues.cpus
		<< "\"program=\"" << m_profileValues.program
		<< "\"delay=\"" << m_profileValues.delay
		<< "\"trigger=\"" << m_profileValues.trigger
		<< ">" << endl << endl;

	stream << "\t<timer interval=\"" 
		<< m_profileValues.timerValues.interval << "/>" << endl << endl;

	WriteEventValues ( stream, m_profileValues.eventValues );

	stream << "</values>" << endl << endl;
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::WriteEventValues()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Write out the event properties for an object
* The object can be from project values, or an ebs session.
*/
void CProjectFile::WriteEventValues ( Q3TextStream &stream, 
									 EVENT_VALUES eventValues[4] )
{
	for ( int n = 0; n < 4; ++n ) {
		stream	<< "<event num=\"" << n 
			<< "\"combo=\"" << eventValues[n].comboIndex 
			<< "\"event=\"" << eventValues[n].event		 
			<< "\"unit=\"" << eventValues[n].unit		 
			<< "\"srcunit=\"" << eventValues[n].sourceUnit 
			<< "\"count=\"" << eventValues[n].count		 
			<< "\"os=\"" << eventValues[n].os		
			<< "\"usr=\"" << eventValues[n].usr		
			<< "/>" << endl;
	}
}


void CProjectFile::WriteTbsSessions ( Q3TextStream & stream )
{
	TbsSessions::Iterator it;

	for ( it = m_tbsSessionList.begin(); it != m_tbsSessionList.end(); 
		++it ) {
			stream << "<tbs_session file=\"" << (*it).fileName 
				<< "\"name=\"" << (*it).name << "/>" << endl 
				<< "\t<timer interval=\"" << (*it).timerValues.interval 
				<< "/>" << endl << "</tbs_session" << endl;
	}

	stream << endl;
}


void CProjectFile::WriteEbsSessions ( Q3TextStream & stream )
{
	EbsSessions::Iterator it;

	for ( it = m_ebsSessionList.begin(); it != m_ebsSessionList.end(); 
		++it ) {
			stream	<< "<ebs_session file=\"" << (*it).fileName 
				<< "\"name=\"" << (*it).name << "/>" << endl;
			stream	<< "\t";

			WriteEventValues ( stream, (*it).eventValues );

			stream	<< "</ebs_session" << endl;
	}
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::DeleteTbsSession()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void CProjectFile::DeleteTbsSession ( QString SessionFileName )
{
	TbsSessions::Iterator it;

	for (it = m_tbsSessionList.begin(); it != m_tbsSessionList.end(); 
		++it) {
			if ( (*it).fileName == SessionFileName ) {
				m_tbsSessionList.remove ( it );
				break;
			}
	}
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::DeleteEbsSession()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void CProjectFile::DeleteEbsSession ( QString SessionFileName )
{
	EbsSessions::Iterator it;

	for ( it = m_ebsSessionList.begin(); it != m_ebsSessionList.end(); 
		++it ) {
			if ( (*it).fileName == SessionFileName ) {
				m_ebsSessionList.remove ( it );
				break;
			}
	}
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::RenameTbsSession()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void CProjectFile::RenameTbsSession ( QString oldname, QString newname )
{
	TbsSessions::Iterator it;

	for ( it = m_tbsSessionList.begin(); it != m_tbsSessionList.end(); 
		++it ) {
			if ( (*it).name == oldname ) {
				(*it).name = newname;
				break;
			}
	}
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::RenameEbsSession()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void CProjectFile::RenameEbsSession ( QString oldname, QString newname )
{
	EbsSessions::Iterator it;

	for ( it = m_ebsSessionList.begin(); it != m_ebsSessionList.end(); 
		++it ) {
			if ( (*it).name == oldname ) {
				(*it).name = newname;
				break;
			}
	}
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::GetPath()
* ~~~~~~~~~~~~~~~~~~~~~~~
* Returns the full path of the project
*/
QString CProjectFile::GetPath()
{
	return m_filePath;
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::GetDir()
* ~~~~~~~~~~~~~~~~~~~~~~
* Returns the directory of the project
*/
QString CProjectFile::GetDir()
{
	QString dir = m_filePath;

	dir.truncate ( m_filePath.findRev ("/") );

	return dir;
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::GetName()
* ~~~~~~~~~~~~~~~~~~~~~~
* Returns the name of the project
*/
QString CProjectFile::GetName()
{
	QString name = m_filePath;

	name.right ( name.length() - name.findRev ("/") );

	return name;
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::GetTbsSessionNames()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Returns the names of all the tbs sessions in the project
*/
QStringList CProjectFile::GetTbsSessionNames()
{
	QStringList list;
	TbsSessions::Iterator it;

	for ( it = m_tbsSessionList.begin(); it != m_tbsSessionList.end(); 
		++it ) {
			list.append ( (*it).name );
	}

	return list;
}


/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* CProjectFile::GetEbsSessionNames()
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Returns the names of all the ebs sessions in the project
*/
QStringList CProjectFile::GetEbsSessionNames()
{
	QStringList list;
	EbsSessions::Iterator it;

	for ( it = m_ebsSessionList.begin(); it != m_ebsSessionList.end(); 
		++it ) {
			list.append ( (*it).name );
	}

	return list;
}


UINT32 CProjectFile::GetNumEbsSessions()
{
	return m_ebsSessionList.count();
}


UINT32 CProjectFile::GetNumTbsSessions()
{
	return m_tbsSessionList.count();
}


void CProjectFile::AddNewEbsSession ( EBS_SESSION_VALUES esv )
{
	m_ebsSessionList.append ( esv );
}


void CProjectFile::AddNewTbsSession ( TBS_SESSION_VALUES tsv )
{
	m_tbsSessionList.append ( tsv );
}


void CProjectFile::GetGeneralOptions ( PPROFILE_VALUES pValues )
{
	(*pValues) = m_profileValues;
}


void CProjectFile::SetGeneralOptions ( PROFILE_VALUES values )
{
	m_profileValues = values;
}
