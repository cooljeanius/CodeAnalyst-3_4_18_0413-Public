//$Id: eventsfile.cpp,v 1.4 2005/02/14 16:52:29 jyeh Exp $
// implementation of the CEventsFile class.

/*
// CodeAnalyst for Open Source
// Copyright 2002-2010 Advanced Micro Devices, Inc.
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


//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eventsfile.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEventsFile::CEventsFile()
{
	m_CurrentCounters = DEFAULT_COUNTER_MASK ;
}

CEventsFile::~CEventsFile()
{

}

/*==================================================================
* CEventsFile::Open()
*/
bool
CEventsFile::open( const char *events_file_path )
{

	QFile xmlFile( events_file_path );
	QXmlInputSource	source( xmlFile );
	QXmlSimpleReader reader;

	/* set our xml handler to do work on the data */
	reader.setContentHandler( this );

	return reader.parse( source );
}

/*==================================================================
* CEventsFile::Close()
*/
void
CEventsFile::close()
{
	m_EventList.clear();
}

/*==================================================================
* CEventsFile::printEvents()
* Used primarily for debuggin purposes.  Clients will typically not 
* need to call this.
*/
void CEventsFile::printEvents()
{
#ifdef _DEBUG_
	EventList::iterator elit;

	for( elit = m_EventList.begin(); elit != m_EventList.end(); ++elit) {
	qDebug("%02X '%s'\n\tAbbrev: '%s'\n\tMin model: %d Source: %s Counters: %X\n", 
		elit->value, 
		elit->name.toAscii().data(),
		elit->abbrev.toAscii().data(),
		elit->m_minValidModel,
		elit->source.toAscii().data(), 
		elit->counters);

		UnitMaskList::iterator umit;
		for( umit = elit->m_UnitMaskList.begin(); 
			umit != elit->m_UnitMaskList.end(); ++umit ) {
				qDebug( "\t%02x %s", umit->value, 
					umit->name.toAscii().data() );
		}
	}
#endif
}


/*==================================================================
* CEventsFile::startDocument()
*/
bool
CEventsFile::startDocument()
{
	m_CurrentSourceUnit = "NONE";
	return true;
}

/*==================================================================
* CEventsFile::startElement()
*/
bool
CEventsFile::startElement(const QString & namespaceURI, 
			  const QString & localName, 
			  const QString & qName, 
			  const QXmlAttributes & atts )
{
	Q_UNUSED (namespaceURI);
	Q_UNUSED (localName);

	/* These are the main elements in our xml file.
	* 1) <cpu_events> -> Begins the document
	* 2) <source>	->contains events pertaining to a 
	*		  specific processor unit
	* 3) <event>	->describes an event
	* 4) <mask>	->located within <event> element and 
	*		contains info for valid masks
	*/
	if( qName == "source" ) {
		// all events within this element will have the 
		// following source unit
		m_CurrentSourceUnit	= atts.value( "unit" );
		// All events within this element will have the following
		// permitted counter mask (unless overridden)
		if (! atts.value("counters").isNull()) {
			m_CurrentCounters = atts.value("counters").toInt(0, 16) ;
		} else {
			m_CurrentCounters = DEFAULT_COUNTER_MASK ;
		}
	} else if( qName == "event" ) {
		CpuEvent event;

		// Add this event to our event list
		event.name = atts.value( "name" );
		event.abbrev = atts.value ("abbreviation");
		event.source = m_CurrentSourceUnit;
		// Return 0 if attribute not given, exactly what we want
		event.m_minValidModel = atts.value("minimumModel").toInt();
		event.value = atts.value( "value" ).toUInt(0, 16);
		// Override the source permitted counters mask when the
		// counters attribute is specified
		if (! atts.value("counters").isNull()) {
			event.counters = atts.value("counters").toInt(0, 16) ;
		} else {
			event.counters = m_CurrentCounters ;
		}
		m_EventList.push_back( event );
	} else if( qName == "mask" ) {
		// these upcoming masks will apply to the last 
		// event we pushed on the list stack
		CpuEvent& lastEvent = m_EventList.back();
		UnitMask unitMask;

		unitMask.name	= atts.value( "name" );
		unitMask.value	= atts.value( "value" ).toUInt();

		lastEvent.m_UnitMaskList.push_back( unitMask );

	} else if( qName == "op_name" ) {
		CpuEvent& lastEvent = m_EventList.back();

		QString op_name= atts.value( "value" );
		if (!op_name.isEmpty())
		{
			lastEvent.op_name = op_name; 
		}
	}

	return true;
}

/*==================================================================
* CEventsFile::endElement()
*/
bool
CEventsFile::endElement(const QString & namespaceURI, 
			const QString & localName, 
			const QString & qName )
{
	Q_UNUSED (namespaceURI);
	Q_UNUSED (localName);
	if( qName == "source" )	{
		m_CurrentSourceUnit = "NONE";
	}

	return true;
}

/*==================================================================
* CEventsXmlHandler::characters()
*/
bool
CEventsFile::characters( const QString & ch )
{
	Q_UNUSED (ch);
	return true;
}

EventList::const_iterator CEventsFile::FirstEvent()
{
	return m_EventList.begin();
}

EventList::const_iterator CEventsFile::EndOfEvents()
{
	return m_EventList.end();
}


EventList::const_iterator CEventsFile::FirstPmcEvent()
{
	EventList::const_iterator it = FirstEvent();
	EventList::const_iterator it_end = EndOfEvents();
	for (; it != it_end; it++) {
		if (PerfEvent::isPmcEvent((*it).value))
			return  it;
	}
	return it_end;
}


EventList::const_iterator CEventsFile::FirstIbsFetchEvent()
{
	EventList::const_iterator it = FirstEvent();
	EventList::const_iterator it_end = EndOfEvents();
	for (; it != it_end; it++) {
		if (PerfEvent::isIbsFetchEvent((*it).value))
			return  it;
	}
	return it_end;
}


EventList::const_iterator CEventsFile::FirstIbsOpEvent()
{
	EventList::const_iterator it = FirstEvent();
	EventList::const_iterator it_end = EndOfEvents();
	for (; it != it_end; it++) {
		if (PerfEvent::isIbsOpEvent((*it).value))
			return  it;
	}
	return it_end;
}


UnitMaskList::const_iterator
CEventsFile::FirstUnitMask( const CpuEvent& event )
{
	return event.m_UnitMaskList.begin();
}

UnitMaskList::const_iterator
CEventsFile::EndOfUnitMasks( const CpuEvent& event )
{
	return event.m_UnitMaskList.end();
}

bool
CEventsFile::findEventByOpStr( QString str, CpuEvent& event )
{
	/* 
	* NOTE: 
	* Format of str is:
	* 	event:IBS_OP_ALL count:250000 unit-mask:0
	* We have to parse just the name
	*/
	QString tmp= str.section(" ",0,0);
	QString name= tmp.section(":",1,1);

	EventList::iterator elit;
	for( elit=m_EventList.begin(); elit!=m_EventList.end(); elit++)
	{
		if (NULL == elit->op_name.toAscii().data())
			continue;

		if(name == elit->op_name)	
		{
			event = (*elit);
			return true;
		}
	}

	return false;
}

bool
CEventsFile::findEventByOpName( QString name, CpuEvent& event )
{
	EventList::iterator elit;
	for( elit=m_EventList.begin(); elit!=m_EventList.end(); elit++)
	{
		if (NULL == elit->op_name.toAscii().data())
			continue;

		if(name == elit->op_name)	
		{
			event = (*elit);
			return true;
		}
	}

	return false;
}

bool CEventsFile::isEventExist( QString eventName )
{
	EventList::iterator elit;
	for( elit=m_EventList.begin(); elit!=m_EventList.end(); elit++)
	{
		if (NULL == elit->op_name.data())
			continue;

		if(eventName == elit->op_name)	
		{	
			return true;
		}
	}

	return false;
}

bool
CEventsFile::findEventByName( QString eventName, CpuEvent& event )
{
	EventList::iterator elit;

	for( elit=m_EventList.begin(); elit!=m_EventList.end(); elit++){
		if( elit->name == eventName ) {
			event = (*elit);
			return true;
		}
	}

	return false;
}

bool
CEventsFile::findEventByValue( unsigned int value, CpuEvent& event )
{
	EventList::iterator elit;

	for( elit=m_EventList.begin(); elit!=m_EventList.end(); elit++){
		if( elit->value == value ){ 
			event = (*elit);
			return true;
		}
	}

	return false;
}

bool
CEventsFile::findIbsEvent(EventList &evList)
{
	bool ret = false;		
	evList.clear();

	EventList::iterator el_it  = m_EventList.begin();
	EventList::iterator el_end = m_EventList.end();
	for(; el_it != el_end ; el_it++)
	{
		if( (*el_it).name.contains("IBS")){ 
			evList.push_back((*el_it));
			ret = true;
		}
	}
	return ret;	
}
