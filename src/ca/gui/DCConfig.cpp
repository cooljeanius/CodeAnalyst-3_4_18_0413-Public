// DCConfig.cpp: Data collection configuration reader/writer

/*
// CodeAnalyst for Open Source
// Copyright 2006 Advanced Micro Devices, Inc.
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

#include "stdafx.h"
#include <qstringlist.h>
#include <qfile.h>
#include <QTextStream>
#include "DCConfigAPI.h"
#include "eventsfile.h"
#include "helperAPI.h"

#ifdef _DEBUG
#include <assert.h>
#include <iostream>
#endif


////////////////////////////////////////////////////////////////////////////////////
// CDCConfig class definition
////////////////////////////////////////////////////////////////////////////////////

// Constants for initializing private data members
#define DefaultConfigName      "No name"
#define DefaultInterval        1.0
#define DefaultMultiplexPeriod 1
#define DefaultProcCore        "Opteron"
#define DefaultMultiplier      10
#define DefaultMaxToTrace      10
#define DefaultMaxCount        0

//
// Simple class constructor
//
CDCConfig::CDCConfig()
{
	m_configType = DCConfigInvalid ;
	m_configName = DefaultConfigName ;
	m_toolTip = "" ;
	m_description = "" ;
	// Time-based profiling configuration attributes
	m_interval = DefaultInterval ;
	// Event-based profiling configuration attributes
	m_multiplexPeriod = 0 ;
//	m_pEventsFile = NULL;
}

//
// Class copy constructor
//
CDCConfig::CDCConfig(const CDCConfig& original)
{
	m_configType = original.m_configType ;
	m_configName = original.m_configName ;
	m_toolTip = original.m_toolTip ;
	m_description = original.m_description ;
	// Time-based profiling configuration attributes
	m_interval = original.m_interval ;
	// Event-based profiling configuration attributes
	m_multiplexPeriod = original.m_multiplexPeriod ;

	// This copy constructor does *not* copy the XML handling state
	// variables. A copy should not occur when a file is being read.
}

//
// Class destructor
//
CDCConfig::~CDCConfig()
{
/*
	if (m_pEventsFile) {
		m_pEventsFile->close();
		delete m_pEventsFile;
		m_pEventsFile = NULL;
	}
*/
}

//
// Class assignment operator
//
const CDCConfig& CDCConfig::operator =(const CDCConfig& rhs)
{
	if (this != &rhs) {
		m_configType = rhs.m_configType ;
		m_configName = rhs.m_configName ;
		m_toolTip = rhs.m_toolTip ;
		m_description = rhs.m_description ;
		// Time-based profiling configuration attributes
		m_interval = rhs.m_interval ;
		// Event-based profiling configuration attributes
		m_multiplexPeriod = rhs.m_multiplexPeriod ;
	}
	return( *this ) ;
}

////////////////////////////////////////////////////////////////////////////////////
// DCConfig mutator functions
////////////////////////////////////////////////////////////////////////////////////

//
// Set the data collection configuration type.
//
void CDCConfig::SetConfigType(DCConfigType configType)
{
	m_configType = configType ;
}

//
// Set the data collection configuration name.
//
void CDCConfig::SetConfigName(const QString configName)
{
	m_configName = configName ;
}

//
// Set the data collection configuration tool tip.
//
void CDCConfig::SetToolTip(const QString toolTip)
{
	m_toolTip = toolTip ;
}

//
// Set the data collection configuration description.
//
void CDCConfig::SetDescription(const QString description)
{
	m_description = description ;
}

//
// Set the data collection cpu type.
//
void CDCConfig::SetCpuType(const QString cpuType)
{
	m_cpuType = cpuType;
}


//
// Set the timer interval. TBP only.
//
void CDCConfig::SetTimerInterval(float interval)
{
	m_interval = interval ;
}

//
// Set the event information. EBP only.
//
void CDCConfig::SetEventInfo(PerfEventContainer * pEventContainer,
				UINT64 multiplexPeriod)
{
	m_multiplexPeriod = multiplexPeriod ;

	m_eventContainer = *pEventContainer;
}


//
// add single event . EBP only.
//
bool CDCConfig::AddEventInfo(PerfEvent & pEvent)
{
	return m_eventContainer.add(pEvent);
}


//
// Write the tool tip element to the XML output stream.
//
void CDCConfig::WriteTooltip(QTextStream& xmlStream, QString& text)
{
	xmlStream << "    <tool_tip>" << text << "</tool_tip>\n" ;
}

//
// Write the description element to the XML output stream.
//
void CDCConfig::WriteDescription(QTextStream& xmlStream, QString& text)
{
	xmlStream << "    <description>" << text << "</description>\n" ;
}

//
// Write a string-valued attribute to the XML output stream.
//
void CDCConfig::WriteStringAttr(QTextStream& xmlStream, const char* attr, QString& value)
{
	xmlStream << ' ' << attr << "=\"" << value << "\"" ;
}

//
// Write an integer attribute in decimal to the XML output stream.
//
void CDCConfig::WriteDecimalAttr(QTextStream& xmlStream, const char* attr, unsigned long long int value)
{
	QString decimal_value ;
	decimal_value.setNum(value, 10) ;
	xmlStream << ' ' << attr << "=\"" ;
	xmlStream << decimal_value << "\"" ;
}

//
// Write an integer attribute in hexdecimal to the XML output stream.
//
void CDCConfig::WriteHexAttr(QTextStream& xmlStream, const char* attr, unsigned long long int value)
{
	QString hex_value ;
	hex_value.setNum(value, 16) ;
	xmlStream << ' ' << attr << "=\"" ;
	xmlStream << hex_value << "\"" ;
}

//
// Write a float attribute in decimal to the XML output stream.
//
void CDCConfig::WriteFloatAttr(QTextStream& xmlStream, const char* attr, float value)
{
	QString decimal_value ;
	decimal_value.setNum(value, 'f', 4) ;
	xmlStream << ' ' << attr << "=\"" ;
	xmlStream << decimal_value << "\"" ;
}

//
// Write an Boolean attribute as "T" or "F" to the XML output stream.
//
void CDCConfig::WriteBoolAttr(QTextStream& xmlStream, const char* attr, bool value)
{
	xmlStream << ' ' << attr ;
	if (value) {
		xmlStream << "=\"T\"" ;
	} else {
		xmlStream << "=\"F\"" ;
	}
}

//
// Write a TBP configuration element to the XML output stream.
//
void CDCConfig::WriteTBP(QTextStream& xmlStream)
{
	xmlStream << "  <tbp" ;
	WriteStringAttr(xmlStream, "name", m_configName) ;
	WriteFloatAttr(xmlStream, "interval", m_interval) ;
	xmlStream << ">\n" ;
	WriteTooltip(xmlStream, m_toolTip) ;
	WriteDescription(xmlStream, m_description) ;
	xmlStream << "  </tbp>\n" ;
}

//
// Write a PERF TBP configuration element to the XML output stream.
//
void CDCConfig::WriteTBPPerf(QTextStream& xmlStream)
{
	xmlStream << "  <tbp_perf" ;
	WriteStringAttr(xmlStream, "name", m_configName) ;
	WriteFloatAttr(xmlStream, "interval", m_interval) ;
	xmlStream << ">\n" ;
	WriteTooltip(xmlStream, m_toolTip) ;
	WriteDescription(xmlStream, m_description) ;
	xmlStream << "  </tbp_perf>\n" ;
}


//
// Write an EBP configuration element to the XML output stream.
//
void CDCConfig::WriteEBP(QTextStream& xmlStream)
{
	xmlStream << "  <ebp" ;
	WriteStringAttr(xmlStream, "name", m_configName) ;
	WriteDecimalAttr(xmlStream, "mux_period", m_multiplexPeriod) ;
	// num_groups is not read. It is written as a debugging aid
	xmlStream << ">\n" ;

	PerfEventList::iterator it     = m_eventContainer.getPerfEventList()->begin();
	PerfEventList::iterator it_end = m_eventContainer.getPerfEventList()->end();
	for (; it != it_end; it++) {
		WriteEvent(xmlStream, &(*it)) ;
	}

	WriteTooltip(xmlStream, m_toolTip) ;
	WriteDescription(xmlStream, m_description) ;
	xmlStream << "  </ebp>\n" ;
}


void CDCConfig::WriteEvent(QTextStream& xmlStream, PerfEvent * e)
{
	xmlStream << "      <event" ;
	WriteHexAttr(xmlStream, "select", e->select()) ;
	WriteHexAttr(xmlStream, "mask", e->umask()) ;
	WriteBoolAttr(xmlStream, "os", e->os()) ;
	WriteBoolAttr(xmlStream, "user", e->usr()) ;
	WriteBoolAttr(xmlStream, "edge_detect", e->edge()) ;
	WriteBoolAttr(xmlStream, "host", e->host()) ;
	WriteBoolAttr(xmlStream, "guest", e->guest()) ;
	WriteDecimalAttr(xmlStream, "count", e->count) ;
	xmlStream << "></event>\n" ;

}

//
// Write the data configuration element to the XML output stream. Call
// the appropriate configuration type-specific private function to actually
// perform the writing.
//
bool CDCConfig::WriteConfigFile(wchar_t *configFileName)
{
	QFile* pFile = NULL ;
	QTextStream xmlStream ;

	if (m_configType == DCConfigInvalid) {
		// No valid configuration data
		return( false ) ;
	}

	pFile = new QFile( QString::fromUcs2((const short unsigned int*)configFileName) ) ;
	if (! pFile->open( QIODevice::WriteOnly )) {
		// Fill open error
		return( false ) ;
	}
	xmlStream.setDevice( pFile ) ;
	// Set XML file character encoding
	xmlStream.setEncoding(QTextStream::UnicodeUTF8) ;
	//	xmlStream.setEncoding(QTextStream::Unicode) ;

	xmlStream << "<?xml version=\"1.0\"?>\n" ;
	xmlStream << "<!DOCTYPE dc_configuration SYSTEM \"dcconfig.dtd\">\n";
	xmlStream << "<dc_configuration" ;

	if (m_cpuType.isEmpty())
		m_cpuType = getCurrentCpuType();
	WriteStringAttr(xmlStream, "cpu_type", m_cpuType) ;
	xmlStream << ">\n" ;
	switch( m_configType ) {
		case DCConfigTBP: 
			{
				WriteTBP( xmlStream ) ;
				break ;
			}
		case DCConfigEBP:
			{
				WriteEBP( xmlStream ) ;
				break ;
			}
		case DCConfigTBPPerf:
			{
				WriteTBPPerf( xmlStream ) ;
				break ;
			}
		default:
			break;
	}
	xmlStream << "</dc_configuration>\n" ;

	// Close file and deallocate QFile explicitly
	xmlStream.unsetDevice() ;
	pFile->close() ;
	delete pFile ;
	return( true ) ;
}

////////////////////////////////////////////////////////////////////////////////////
// DCConfig accessor functions
////////////////////////////////////////////////////////////////////////////////////

//
// Return the data collection configuration type to the caller.
//
DCConfigType CDCConfig::GetConfigType()
{
	return( m_configType ) ;
}

//
// Return the configuration name to the caller.
//
void CDCConfig::GetConfigName(QString& configName)
{
	configName = m_configName ; 
}

//
// Return the tool tip string to the caller.
//
void CDCConfig::GetToolTip(QString& toolTip)
{
	toolTip = m_toolTip ; 
}

//
// Return the description string to the caller.
//
void CDCConfig::GetDescription(QString& description)
{
	description = m_description ;
}

//
// Return the timer interval to the caller. TBP only.
//
float CDCConfig::GetTimerInterval()
{
	return( m_interval ) ;
}


//
// Return event configuration information to the caller. 
//
void CDCConfig::GetEventInfo(PerfEventContainer * pEventContainer)
{
	*pEventContainer = m_eventContainer;
}

//
// Return the multiplexing period to the caller. EBP only.
//
UINT64 CDCConfig::GetMultiplexPeriod()
{
	return( m_multiplexPeriod ) ;
}


//
// Set the multiplexing period to the caller. EBP only.
//
void CDCConfig::SetMultiplexPeriod(UINT64 mux)
{
	m_multiplexPeriod = mux;
}
////////////////////////////////////////////////////////////////////////////////////
// Read data collection confguration from XML file
////////////////////////////////////////////////////////////////////////////////////

//
// Set up the XML file for reading, then start the parse. The XML parse
// proceeds and calls the callback functions below. Return false if the
// parser detected an error.
//
bool CDCConfig::ReadConfigFile(wchar_t *configFileName)
{
	// Create a file, an XML input source and a simple reader
	QFile xmlFile( QString::fromUcs2((const short unsigned int*)configFileName) ) ;
	QXmlInputSource source( xmlFile ) ;
	QXmlSimpleReader reader ;
	// Connect this object's handler interface to the XML reader
	reader.setContentHandler( this ) ;
	// Return true if the parse succeeds and no XML semantic errors
	return( reader.parse( source ) && (!m_xmlSemanticError) ) ;
}

////////////////////////////////////////////////////////////////////////////////////
// Callback functions for reading a data collection configuration in XML
////////////////////////////////////////////////////////////////////////////////////

//
// startDocument() is called at the beginning of an XML document.
// Prepare to read a data collection configuration by clearing the
// current configuration.
//
bool CDCConfig::startDocument()
{
	m_configName = DefaultConfigName ;
	m_toolTip = "" ;
	m_description = "" ;
	// Clear all DC configuration info as if this was a new object
	m_configType = DCConfigInvalid ;
	// Time-based profiling configuration attributes
	m_interval = DefaultInterval ;
	// Event-based profiling configuration attributes
	m_multiplexPeriod = DefaultMultiplexPeriod ;
	// Initialize all XML-related working variables
	m_toolTipIsOpen = false ;
	m_descriptionIsOpen = false ;
	m_groupIsOpen = false ;
	m_xmlSemanticError = false ;

	return( true ) ;
}

//
// endDocument() is called at the beginning of an XML document.
//
bool CDCConfig::endDocument()
{
	return( true ) ;
}

//
// startElement() is called at the beginning of an XML element.
// Dispatch control using the element name. Handle an element-specific
// attributes. Set-up state variables to handle the contents of the
// element as the XML parse proceeds.
//
bool CDCConfig::startElement(
	const QString &namespaceURI, const QString &localName,
	const QString &qName, const QXmlAttributes &atts)
{
	Q_UNUSED(namespaceURI);
	Q_UNUSED(localName);
	QString elementName = qName.lower() ;

	if ( elementName == "ebp" ) {
		/////////////////////////////////////////////////////////
		// <ebp> Read and store event-based profiling parameters
		/////////////////////////////////////////////////////////
#ifdef _DEBUG
		assert( !atts.value("name").isNull() ) ;
		assert( !atts.value("mux_period").isNull() ) ;
#endif
		m_configType = DCConfigEBP ;
		// Use the default name when configuration name is missing
		if (! atts.value("name").isNull()) {
			m_configName = atts.value("name") ;
		}

		m_multiplexPeriod = atts.value("mux_period").toInt(0, 10) ;
	} else if ( elementName == "event" ) {
		/////////////////////////////////////////////////////////
		// <event> Read and store
		// event parameters in the last event group on list
		/////////////////////////////////////////////////////////
#ifdef _DEBUG
		assert( !atts.value("select").isNull() ) ;
		assert( !atts.value("mask").isNull() ) ;
		assert( !atts.value("os").isNull() ) ;
		assert( !atts.value("user").isNull() ) ;
		assert( !atts.value("count").isNull() ) ;
		// "edge_detect" is an optional attribute
		// "host" is an optional attribute
		// "guest" is an optional attribute
#endif
		// Retrieve and store EBP-related attribute values
		PerfEvent e;
		e.setSelect(atts.value("select").toUInt(0, 16));
		e.setUmask(atts.value("mask").toUInt(0, 16));
		e.setOs("T" == atts.value("os").upper());
		e.setUsr("T" == atts.value("user").upper());
		if (! atts.value("edge_detect").isNull()) 
			e.setEdge("T" == atts.value("edge_detect").upper());
		if (! atts.value("host").isNull()) 
			e.setHost("T" == atts.value("host").upper());
		if (! atts.value("guest").isNull()) 
			e.setGuest("T" == atts.value("guest").upper());
		e.count = atts.value("count").toULong(0, 10) ;
/*		
		// TODO: Suravee: Need to change this to support non-AMD events
		if (m_pEventsFile) {
			CpuEvent ce;
			m_pEventsFile->findEventByValue(e.select, ce);
			e.opName = ce.op_name;
		}
*/
		m_eventContainer.add(e);
	} else if ( elementName == "tool_tip" ) {
		/////////////////////////////////////////////////////////
		// <tool_tip> Set the current tooltip string to empty
		/////////////////////////////////////////////////////////
		m_toolTip = "" ;
		m_toolTipIsOpen = true ;
	} else if ( elementName == "description" ) {
		/////////////////////////////////////////////////////////
		// <description> Set the current description string to empty
		/////////////////////////////////////////////////////////
		m_description = "" ;
		m_descriptionIsOpen = true ;
	} else if ( elementName == "tbp" ) {
		/////////////////////////////////////////////////////////
		// <tbp> time-based profiling configuration parameters
		/////////////////////////////////////////////////////////
#ifdef _DEBUG
		assert( !atts.value("name").isNull() ) ;
		assert( !atts.value("interval").isNull() ) ;
#endif
		m_configType = DCConfigTBP ;
		// if (atts.value("name").isNull()) m_xmlSemanticError = true ;
		// Use the default name when configuration name is missing
		if (! atts.value("name").isNull()) {
			m_configName = atts.value("name") ;
		}

		m_interval = atts.value("interval").toFloat(0) ;
	} else if ( elementName == "tbp_perf" ) {
		/////////////////////////////////////////////////////////
		// <tbp> PERF time-based profiling configuration parameters
		/////////////////////////////////////////////////////////
#ifdef _DEBUG
		assert( !atts.value("name").isNull() ) ;
		assert( !atts.value("interval").isNull() ) ;
#endif
		m_configType = DCConfigTBPPerf ;
		// if (atts.value("name").isNull()) m_xmlSemanticError = true ;
		// Use the default name when configuration name is missing
		if (! atts.value("name").isNull()) {
			m_configName = atts.value("name") ;
		}

		m_interval = atts.value("interval").toFloat(0) ;
	} else if ( elementName == "dc_configuration" ) {
		/////////////////////////////////////////////////////////
		// <dc_configuration>
		/////////////////////////////////////////////////////////
#ifdef _DEBUG
		assert( !atts.value("cpu_type").isNull() ) ;
#endif
		m_cpuType = atts.value("cpu_type") ;
		
		if (m_cpuType.isEmpty()) {
			// Use current cpu_type if not specified.
			m_cpuType = getCurrentCpuType();
			if (m_cpuType.isEmpty()) {
				// TODO: Should be handled better
				fprintf(stderr,"Cannot determine cpu_type for DC Configuration.\n"); 
				return false;
			}
		}

		// TODO: Suravee: Work on this.	
//		SetupEventsFile(m_cpuType);
	} else if ( elementName == "group" ) {
		// Ignore this (older DC Config file).
	} else {
		// Unrecognized element name
		return( false ) ;
	}
	return( true ) ;
}

//
// endElement() is called at the end of an XML element. Dispatch
// control using the element name to perform element-specific
// finalization of the element's contents.
//
bool CDCConfig::endElement(
	const QString &namespaceURI, const QString &localName,
	const QString &qName)
{
	Q_UNUSED(namespaceURI);
	Q_UNUSED(localName);
	QString elementName = qName.lower() ;
	if ( elementName == "tool_tip" ) {
		/////////////////////////////////////////////////////////
		// </tool_tip> Close the open tooltip element
		/////////////////////////////////////////////////////////
		m_toolTipIsOpen = false ;
	} else if ( elementName == "description" ) {
		/////////////////////////////////////////////////////////
		// </description> Close the open description element
		/////////////////////////////////////////////////////////
		m_descriptionIsOpen = false ;
	}
	return( true ) ;
}

//
// characters() is called to return text that is the content of
// a non-empty XML element. Tooltip and description elements are
// the only DC configuration elements that are non-empty.
//
bool CDCConfig::characters(const QString &ch)
{
	if (m_descriptionIsOpen) {
		// Add characters to the end of the description text
		m_description.append( ch.simplifyWhiteSpace() ) ;
	} else if (m_toolTipIsOpen) {
		// Add characters to the end of the tooltip text
		m_toolTip.append( ch.simplifyWhiteSpace() ) ;
	}
	return( true ) ;
}
/*
bool CDCConfig::SetupEventsFile(QString cpuType)
{
	if (m_pEventsFile) {
		delete m_pEventsFile;
		m_pEventsFile = NULL;
	}

	QString path = helpGetEventEventFile(cpuType);
	if (path.isEmpty())
		return false;

	m_pEventsFile = new CEventsFile();
	if (!m_pEventsFile || !m_pEventsFile->open(path))
		return true;
}
*/
	
bool CDCConfig::validateEvents(CEventsFile * pEventFile, QString & errorStr)
{
	return m_eventContainer.validateEvents(pEventFile, errorStr);
}
