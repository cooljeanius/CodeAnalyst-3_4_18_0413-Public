//$Id: projectfile.h,v 1.2 2005/02/14 16:52:40 jyeh Exp $
// interface for the CProjectFile class.

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
//$Log: projectfile.h,v $
//Revision 1.2  2005/02/14 16:52:40  jyeh
//Updated header.
//
//Revision 1.1.1.1  2004/10/05 17:03:23  jyeh
//initial import into CVS
//
//Revision 1.4  2003/12/12 23:04:00  franksw
//Code Review
//
//Revision 1.3  2003/11/17 23:24:40  franksw
//code cleanup
//

#ifndef _PROJECTFILE_H_
#define _PROJECTFILE_H_

#include <qxml.h>
#include <q3valuestack.h>
#include <Q3TextStream>
#include <Q3ValueList>

//////////////////////////////////////////////////////////////////////////
// TIMER_VALUES
//
struct TIMER_VALUES
{
	float	interval;
};

//////////////////////////////////////////////////////////////////////////
// EVENT_VALUES
//
struct EVENT_VALUES
{
	DWORD	comboIndex;
	DWORD	event;
	DWORD	unit;
	QString	sourceUnit;
	DWORD	count;
	bool	os;
	bool	usr;
};

//////////////////////////////////////////////////////////////////////////
// TBS_SESSION_VALUES
//
struct TBS_SESSION_VALUES
{
	TIMER_VALUES	timerValues;
	QString		name;
	QString		fileName;
};


//////////////////////////////////////////////////////////////////////////
// EBS_SESSION_VALUES
//
struct EBS_SESSION_VALUES
{
	EVENT_VALUES	eventValues[4];
	QString		name;
	QString		fileName;
};


//////////////////////////////////////////////////////////////////////////
// PROFILE_VALUES
//
typedef struct _PROFILE_VALUES
{
	QString		name;
	DWORD		duration;
	DWORD		bufferSize;
	DWORD		cpus;
	QString		program;
	DWORD		delay;
	TRIGGER		trigger;

	TIMER_VALUES	timerValues;
	EVENT_VALUES	eventValues[4];

} PROFILE_VALUES, *PPROFILE_VALUES;

// typedefs
typedef Q3ValueList<TBS_SESSION_VALUES>	TbsSessions;
typedef Q3ValueList<EBS_SESSION_VALUES>	EbsSessions;
typedef Q3ValueStack<QString>		ElementStack;


//////////////////////////////////////////////////////////////////////////
// CProjectFile
//
class CProjectFile : public QXmlDefaultHandler  
{

public:
	CProjectFile();
	virtual ~CProjectFile();

	bool	Open ( char * file_path );
	bool	Create ( char *file_path, PROFILE_VALUES pv );
	bool	Save();

	void DeleteTbsSession ( QString SessionFileName );
	void DeleteEbsSession ( QString SessionFileName );
	void RenameEbsSession ( QString oldname, QString newname );
	void RenameTbsSession ( QString oldname, QString newname );

	QString GetPath();
	QString GetName();
	QString GetDir ();

	QStringList GetTbsSessionNames();
	QStringList GetEbsSessionNames();

	UINT32  GetNumEbsSessions();
	UINT32  GetNumTbsSessions();

	void AddNewTbsSession( TBS_SESSION_VALUES tsv );
	void AddNewEbsSession( EBS_SESSION_VALUES esv );

	void GetGeneralOptions( PPROFILE_VALUES pValues );
	void SetGeneralOptions( PROFILE_VALUES  values   );

	/* overrides from QXmlDefaultHandler */
	bool startDocument();
	bool startElement ( const QString & namespaceURI, 
		const QString & localName, const QString & qName,
		const QXmlAttributes & atts );
	bool endElement ( const QString & namespaceURI, 
		const QString & localName, const QString & qName );

private:
	QString	CurrentElement();
	void WriteValues ( Q3TextStream &stream );
	void WriteEventValues ( Q3TextStream &stream, 
		EVENT_VALUES eventValues[4] );
	void WriteTbsSessions ( Q3TextStream &stream );
	void WriteEbsSessions ( Q3TextStream &stream );

	// data
private:
	QString		m_filePath;

	TbsSessions	m_tbsSessionList;
	EbsSessions	m_ebsSessionList;
	ElementStack	m_elementStack;

	PROFILE_VALUES	m_profileValues;
};

#endif // #ifndef _PROJECTFILE_H_
