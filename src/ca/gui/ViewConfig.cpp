// ViewConfig.cpp: View configuration class (including reader/writer)

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
#include "ViewConfigAPI.h"
#include <qstring.h>
#include <QTextStream>
#include <assert.h>
#include <iostream>

#include "EventMaskEncoding.h"
#include "PerfEvent.h"

///////////////////////////////////////////////////////////////////////
// CViewConfig class definition
///////////////////////////////////////////////////////////////////////

// Constants for initializing private data members
#define DefaultConfigName  "No name"
#define DefaultColumnTitle "No title"
#define DefaultEventConfig 0x000000000ULL

//
// Simple class constructor
//
VIEWCONFIGURE_API CViewConfig::CViewConfig()
{
	m_configName = DefaultConfigName ;
	m_toolTip = "" ;
	m_description = "" ;
	m_separateCPUs = false ;
	m_separateProcesses = false ;
	m_separateThreads = false ;
	m_defaultView = false ;
	m_showPercentage = false ;
	m_numberOfColumns = 0 ;
	m_columnSpecs = NULL ;
	m_assocList.clear() ;
	m_columnList.clear() ;
}

//
// Class copy constructor
//
VIEWCONFIGURE_API CViewConfig::CViewConfig(const CViewConfig& original)
{
	m_configName = original.m_configName ;
	m_toolTip = original.m_toolTip ;
	m_description = original.m_description ;
	m_separateCPUs = original.m_separateCPUs ;
	m_separateProcesses = original.m_separateProcesses ;
	m_separateThreads = original.m_separateThreads ;
	m_defaultView = original.m_defaultView ;
	m_showPercentage = original.m_showPercentage ;
	// Copy the array of column specifications
	m_columnSpecs = NULL ;
	m_numberOfColumns = original.m_numberOfColumns ;
	if (NULL != original.m_columnSpecs) {
		m_columnSpecs = new ColumnSpec[m_numberOfColumns] ;
		CopyColumnSpecs(original.m_columnSpecs, m_columnSpecs, m_numberOfColumns) ;
	}
	// TODO: The following list assign probably does not deep copy QString elements
	m_assocList.assign( original.m_assocList.begin( ), original.m_assocList.end( ) );
	// This copy constructor does *not* copy the XML handling state
	// variables. A copy should not occur when a file is being read.
}

//
// Class destructor
//
VIEWCONFIGURE_API CViewConfig::~CViewConfig()
{
	m_assocList.clear() ;
	m_columnList.clear() ;
	if (m_columnSpecs != NULL) delete [] m_columnSpecs ;
	m_columnSpecs = NULL ;
}

//
// Class assignment operator
//
VIEWCONFIGURE_API const CViewConfig& CViewConfig::operator =(const CViewConfig& rhs)
{
	if (this != &rhs) {
		m_configName = rhs.m_configName ;
		m_toolTip = rhs.m_toolTip ;
		m_description = rhs.m_description ;
		m_separateCPUs = rhs.m_separateCPUs ;
		m_separateProcesses = rhs.m_separateProcesses ;
		m_separateThreads = rhs.m_separateThreads ;
		m_defaultView = rhs.m_defaultView ;
		m_showPercentage = rhs.m_showPercentage ;

		// Deallocate old column specs array if it exists
		if (m_columnSpecs != NULL) {
			delete [] m_columnSpecs ;
			m_columnSpecs = NULL ;
		}
		// Copy the array of column specifications
		m_numberOfColumns = rhs.m_numberOfColumns ;
		if (NULL != rhs.m_columnSpecs) {
			m_columnSpecs = new ColumnSpec[m_numberOfColumns] ;
			CopyColumnSpecs(rhs.m_columnSpecs, m_columnSpecs, m_numberOfColumns) ;
		}
		// TODO: The following list assign probably does not deep copy QString elements
		m_assocList.assign( rhs.m_assocList.begin( ), rhs.m_assocList.end( ) );
		// This assignment operator does *not* copy the XML handling state
		// variables. An assignment should not occur when a file is being read.
	}
	return( *this ) ;
}

//
// Set the view configuration name
//
VIEWCONFIGURE_API void CViewConfig::SetConfigName(const QString name)
{
	m_configName = name ;
}

//
// Return view configuration name through reference argument name
//
VIEWCONFIGURE_API void CViewConfig::GetConfigName(QString& name)
{
	name = m_configName ;
}

VIEWCONFIGURE_API void CViewConfig::SetToolTip(const QString toolTip)
{
	m_toolTip = toolTip ;
}

VIEWCONFIGURE_API void CViewConfig::GetToolTip(QString& toolTip)
{
	toolTip = m_toolTip ;
}

VIEWCONFIGURE_API void CViewConfig::SetDescription(const QString description)
{
	m_description = description ;
}

VIEWCONFIGURE_API void CViewConfig::GetDescription(QString& description)
{
	description = m_description ;
}

///////////////////////////////////////////////////////////////////////
// Functions to copy, set and get the view column specifications
///////////////////////////////////////////////////////////////////////

//
// Copy an array of ColumnSpec -- required to perform deep copy of the
// non-primitive elements like QString. This function is used within
// CViewConfig and could be useful outside the class.
//
VIEWCONFIGURE_API void CViewConfig::CopyColumnSpecs(
	ColumnSpec* fromArray, ColumnSpec* toArray, int size)
{
	for (int i = 0 ; i < size ; i++)
	{
		toArray[i] = fromArray[i] ;
	}
}

//
// Create data associations from column specifications.
// Generate ID/event associations for all events; This option
// eases the burden on the GUI until it can request IDs from the user
//
void CViewConfig::GenerateDataAssocs(ColumnSpec* columnSpecArray, 
									 int numberOfColumns)
{
	DataAssoc da ;
	QString id ;

#ifdef DEBUG
	assert( numberOfColumns > 0 ) ;
	assert( columnSpecArray != NULL ) ;
#endif

	m_assocList.clear() ;
	for (int i = 0 ; i < m_numberOfColumns ; i++)
	{
		switch( m_columnSpecs[i].type )
		{
		case ColumnSum:
		case ColumnDifference:
		case ColumnProduct:
		case ColumnRatio:
			if (! FindAssocByConfig(m_columnSpecs[i].dataSelectRight, da))
			{
				id = "e" ;
				id.append(QString::number(m_columnSpecs[i].dataSelectRight, 16)) ;
				AddAssoc(id, m_columnSpecs[i].dataSelectRight) ;
			}
			// Fall through is intended! Must handle *both* right and left
		case ColumnValue:
			if (! FindAssocByConfig(m_columnSpecs[i].dataSelectLeft, da))
			{
				id = "e" ;
				id.append(QString::number(m_columnSpecs[i].dataSelectLeft, 16)) ;
				AddAssoc(id, m_columnSpecs[i].dataSelectLeft) ;
			}
			break ;
		default:
			break;
		}
	}
}

//
// Define column specifications for the view. The caller provides a
// one-dimensional array of ColumnSpec structs where each ColumnSpec
// defines a column. Order of the ColumnSpecs is important and must
// be preserved; columns will be displayed in order from first to last.
//
VIEWCONFIGURE_API void CViewConfig::SetColumnSpecs(
	ColumnSpec* columnSpecArray, int numberOfColumns,
	const int generateDataAssocs)
{
#ifdef DEBUG
	assert( numberOfColumns > 0 ) ;
	assert( columnSpecArray != NULL ) ;
#endif

	// Delete existing column spec array to avoid memory leak
	if (m_columnSpecs != NULL) delete [] m_columnSpecs ;
	m_columnSpecs = NULL ;

	m_numberOfColumns = numberOfColumns ;
	if (m_numberOfColumns > 0) {
		m_columnSpecs = new ColumnSpec[m_numberOfColumns] ;
#ifdef _DEBUG
		assert( m_columnSpecs != NULL ) ;
#endif
		// Element-by-element copy needed to copy QString
		CopyColumnSpecs(columnSpecArray, m_columnSpecs, m_numberOfColumns) ;
	} else {
		m_numberOfColumns = 0 ;
		return ;
	}

	if (generateDataAssocs)
	{
		GenerateDataAssocs(columnSpecArray, numberOfColumns) ;
	}
}

//
// Make column specifications from an array of EventConfig values. Use
// the list of title strings to initialize the column titles. Create a
// column spec for each event (show value, no sorting.) Generate data
// associations, if enabled.
//
VIEWCONFIGURE_API void CViewConfig::MakeColumnSpecs(EventEncodeVec * pEvents, 
						QStringList titles,
						const int generateDataAssocs)
{
	// Delete existing column spec array to avoid memory leak
	if (m_columnSpecs != NULL) delete [] m_columnSpecs ;
	m_columnSpecs = NULL ;

	// Create a new column spec array
	m_numberOfColumns = pEvents->size();
	if (m_numberOfColumns > 0) {
		m_columnSpecs = new ColumnSpec[m_numberOfColumns] ;
#ifdef _DEBUG
		assert( m_columnSpecs != NULL ) ;
#endif
	} else {
		m_numberOfColumns = 0 ;
		return ;
	}

	// Initialize each column spec for an event
	QStringList::iterator qslit = titles.begin() ;

	EventEncodeVec::iterator it = pEvents->begin();		
	EventEncodeVec::iterator itEnd = pEvents->end();		
	for (int i = 0 ; it != itEnd; i++, it++ , qslit++)
	{
		m_columnSpecs[i].type = ColumnValue ;
		m_columnSpecs[i].sorting = NoSort ;
		m_columnSpecs[i].visible = true ;
		//m_columnSpecs[i].dataSelectLeft = (*it).eventMask ;

		/* Note: For IBS events, we don't care about unitmask */
		unsigned int ev = 0;
		DecodeEventMask((*it).eventMask, &ev, NULL);
		if (PerfEvent::isIbsEvent(ev)) {
			m_columnSpecs[i].dataSelectLeft = ev;
		} else {
			m_columnSpecs[i].dataSelectLeft = (*it).eventMask ;
		}

		m_columnSpecs[i].dataSelectRight = DefaultEventConfig ;
		m_columnSpecs[i].title = (*qslit) ;
	}

	if (generateDataAssocs)
	{
		GenerateDataAssocs(m_columnSpecs, m_numberOfColumns) ;
	}
}


//
// Return column specifications in the array allocated by the caller.
// The array *must* be large enough to hold all of the view's column
// specifications. GetNumberOfColumns() can be called to determine the
// size of the array to be allocated.
//
VIEWCONFIGURE_API void CViewConfig::GetColumnSpecs(ColumnSpec* columnSpecArray)
{
#ifdef DEBUG
//	assert( columnSpecArray != NULL ) ;
//	assert( (m_numberOfColumns > 0) && (m_columnSpecs != NULL) ) ;
#endif
	if ((m_numberOfColumns > 0) && (m_columnSpecs != NULL))
	{
		CopyColumnSpecs(m_columnSpecs, columnSpecArray, m_numberOfColumns) ;
	}
}

///////////////////////////////////////////////////////////////////////
// Functions to manipulate event configuration
///////////////////////////////////////////////////////////////////////

//
// Create an event configuration value from a 16-bit select and unit
// mask
//
VIEWCONFIGURE_API EventConfig CViewConfig::EncodeConfig(unsigned int select,
														unsigned int unitMask)
{
	/*
	UINT64 selectHi = (select >> 8) & 0xFF ;
	UINT64 selectLo = select & 0xFF ;
	UINT64 mask = unitMask & 0xFF ;

	return( (selectHi << 32) | (mask << 8) | selectLo ) ;
	*/
	return EncodeEventMask(select, unitMask);
}

//
// Return a 16-bit event select from an event configuration value
//
VIEWCONFIGURE_API unsigned int CViewConfig::ExtractSelect(EventConfig config)
{
	/*
	unsigned int selectLo = config & 0xFF ;
	unsigned int selectHi = (config >> 32) & 0xFF ;

	return( (selectHi << 8) | selectLo ) ;
	*/
	unsigned int event;
	DecodeEventMask(config, &event,NULL);
	return (unsigned int) event;

}

//
// Return the unit mask from an event configuration value
//
VIEWCONFIGURE_API unsigned int CViewConfig::ExtractUnitMask(EventConfig config)
{
	//return( (config >> 8) & 0xFF ) ;

	unsigned int umask = 0;
	DecodeEventMask(config,NULL,(unsigned char*)&umask);
	return umask;
}

///////////////////////////////////////////////////////////////////////
// Id/event configuration association
///////////////////////////////////////////////////////////////////////

//
// Define new id/event configuration association
//
VIEWCONFIGURE_API void CViewConfig::AddAssoc(const QString id,
											 EventConfig config)
{
	// This function does not check for duplicates
	DataAssoc da ;
	da.eventId = id ;
	da.eventConfig = config ;
	m_assocList.push_back(da) ;
}

VIEWCONFIGURE_API void CViewConfig::AddAssoc(const QString id, 
											 unsigned int select, unsigned int unitMask)
{
	// This function does not check for duplicates
	DataAssoc da ;
	da.eventId = id ;
	da.eventConfig = EncodeConfig(select, unitMask) ;
	m_assocList.push_back(da) ;
}

//
// Find data/event configuration association by id or value
//
VIEWCONFIGURE_API bool CViewConfig::FindAssocById(const QString id,
												  DataAssoc& assoc)
{
	DataAssocList::const_iterator dalit ;

	for (dalit=m_assocList.begin() ; dalit!=m_assocList.end() ; dalit++)
	{
		if (dalit->eventId == id)
		{
			assoc = (*dalit) ;
			return( true ) ;
		}
	}
	return( false ) ;
}

VIEWCONFIGURE_API bool CViewConfig::FindAssocByConfig(EventConfig config,
													  DataAssoc& assoc)
{
	DataAssocList::const_iterator dalit ;

	for (dalit=m_assocList.begin() ; dalit!=m_assocList.end() ; dalit++)
	{
		if (dalit->eventConfig == config)
		{
			assoc = (*dalit) ;
			return( true ) ;
		}
	}
	return( false ) ;
}

VIEWCONFIGURE_API bool CViewConfig::FindAssocByValue(unsigned int select,
													 unsigned int unitMask, DataAssoc& assoc)
{
	DataAssocList::const_iterator dalit ;
	EventConfig config = EncodeConfig(select, unitMask) ;

	for (dalit=m_assocList.begin() ; dalit!=m_assocList.end() ; dalit++)
	{
		if (dalit->eventConfig == config)
		{
			assoc = (*dalit) ;
			return( true ) ;
		}
	}
	return( false ) ;
}

//
// Return the EventConfig value associated with an event ID. If the
// ID is not found, then return DefaultEventConfig.
//
VIEWCONFIGURE_API EventConfig CViewConfig::GetConfigById(const QString id)
{
	DataAssoc da ;

	if ((! id.isNull()) && (FindAssocById(id, da)))
	{
		return( da.eventConfig ) ;
	}
	return( DefaultEventConfig ) ;
}

//
// Find the specified event configuration in an array of event
// configurations. Return true if found.
//
bool CViewConfig::FindEventConfig(EventConfig config, EventEncodeVec * pVec, bool testUnitMask)
{
	EventEncodeVec::iterator it = pVec->begin();
	EventEncodeVec::iterator it_end = pVec->end();
	for ( ; it != it_end ; it++)
	{
		if (ExtractSelect(config) == ExtractSelect((*it).eventMask))
		{
			// Event select values match
			if (testUnitMask)
			{
				if (ExtractUnitMask(config) == ExtractUnitMask((*it).eventMask))
				{
					// Found it, return true
					return( true ) ;
				}
			} else {
				// Found it, return true
				return( true ) ;
			}
		}
	}
	// Not in the array, return false
	return( false ) ;

}

//
// Determine if this view (configuration) is displayable given a
// set of available events. The available events are supplied in a
// one dimensional array of EventConfig values. The argument
// numberOfEvents specifies the number of elements in the array.
// Walk the list of data associations (id/event config pairs) and
// try to find each event configuration in the array of available
// events. If all required events can be found in the array of
// available events, then the view is displayable and true is returned.
//
VIEWCONFIGURE_API bool CViewConfig::isViewDisplayable(EventEncodeVec * pAvailable, bool testUnitMask)
{
	DataAssocList::const_iterator dalit ;

	for (dalit=m_assocList.begin() ; dalit!=m_assocList.end() ; dalit++)
	{
		if (! FindEventConfig(dalit->eventConfig, pAvailable, testUnitMask))
		{
			return( false ) ;
		}
	}
	return( true ) ;
}

//
// Write the tool tip element to the XML output stream.
//
void CViewConfig::WriteTooltip(QTextStream& xmlStream, QString& text)
{
	xmlStream << "    <tool_tip>" << text << "</tool_tip>\n" ;
}

//
// Write the description element to the XML output stream.
//
void CViewConfig::WriteDescription(QTextStream& xmlStream, QString& text)
{
	xmlStream << "    <description>" << text << "</description>\n" ;
}

//
// Write a string-valued attribute to the XML output stream.
//
void CViewConfig::WriteStringAttr(QTextStream& xmlStream, const char* attr, const QString& value)
{
	xmlStream << ' ' << attr << "=\"" << value << "\"" ;
}

//
// Write an integer attribute in decimal to the XML output stream.
//
void CViewConfig::WriteDecimalAttr(QTextStream& xmlStream, const char* attr, unsigned long long int value)
{
	QString decimal_value ;
	decimal_value.setNum(value, 10) ;
	xmlStream << ' ' << attr << "=\"" ;
	xmlStream << decimal_value << "\"" ;
}

//
// Write an integer attribute in hexdecimal to the XML output stream.
//
void CViewConfig::WriteHexAttr(QTextStream& xmlStream, const char* attr, unsigned long long int value)
{
	QString hex_value ;
	hex_value.setNum(value, 16) ;
	xmlStream << ' ' << attr << "=\"" ;
	xmlStream << hex_value << "\"" ;
}

//
// Write a float attribute in decimal to the XML output stream.
//
void CViewConfig::WriteFloatAttr(QTextStream& xmlStream, const char* attr, float value)
{
	QString decimal_value ;
	decimal_value.setNum(value, 'f', 4) ;
	xmlStream << ' ' << attr << "=\"" ;
	xmlStream << decimal_value << "\"" ;
}

//
// Write an Boolean attribute as "T" or "F" to the XML output stream.
//
void CViewConfig::WriteBoolAttr(QTextStream& xmlStream, const char* attr, bool value)
{
	xmlStream << ' ' << attr ;
	if (value) {
		xmlStream << "=\"T\"" ;
	} else {
		xmlStream << "=\"F\"" ;
	}
}

//
// Write the <data> element to the XML output stream.
//
void CViewConfig::WriteData(QTextStream& xmlStream)
{
	DataAssocList::iterator dalit ;

	xmlStream << "    <data>\n" ;
	for (dalit=m_assocList.begin() ; dalit!=m_assocList.end() ; dalit++)
	{
		xmlStream << "      <event" ;
		WriteStringAttr(xmlStream, "id", dalit->eventId) ;
		WriteHexAttr(xmlStream, "select", ExtractSelect(dalit->eventConfig)) ;
		WriteHexAttr(xmlStream, "mask", ExtractUnitMask(dalit->eventConfig)) ;
		xmlStream << " />\n" ;
	}
	xmlStream << "    </data>\n" ;
}

//
// Write an event ID to the XML output stream.
//
void CViewConfig::WriteEventId(QTextStream& xmlStream, const char* attr, const EventConfig& config)
{
	QString id ;
	DataAssoc da ;

	if (FindAssocByConfig(config, da))
	{
		id = da.eventId ;
	} else {
		// This case is an internal error. The data section has
		// already been written -- too late to generate an ID!
		xmlStream << "<!-- Missing event ID --> " ;
		id = "ERROR" ;
	}
	WriteStringAttr(xmlStream, attr, id) ;
}

//
// Write an arithmetic relationship to the XML output stream.
//
void CViewConfig::WriteArith(QTextStream& xmlStream, const ColumnSpec& column)
{
	switch ( column.type )
	{
	case ColumnSum:
		{
			xmlStream << "        <sum" ;
			break ;
		}
	case ColumnDifference:
		{
			xmlStream << "        <difference" ;
			break ;
		}
	case ColumnProduct:
		{
			xmlStream << "        <product" ;
			break ;
		}
	case ColumnRatio:
		{
			xmlStream << "        <ratio" ;
			break ;
		}
	default:
		{
			// Internal error
			xmlStream << "        <!-- Invalid column type -->\n" ;
			xmlStream << "        <ERROR" ;
			break ;
		}
	}
	WriteEventId(xmlStream, "left", column.dataSelectLeft) ;
	WriteEventId(xmlStream, "right", column.dataSelectRight) ;
	xmlStream << " />\n" ;
}

//
// Write a <column> element to the XML output stream.
//
void CViewConfig::WriteColumn(QTextStream& xmlStream, const ColumnSpec& column)
{
	QString sort ;
	xmlStream << "      <column" ;
	WriteStringAttr(xmlStream, "title", column.title) ;
	// All this to write the sort attribute!
	switch( column.sorting )
	{
	case NoSort:
		{
			sort = "none" ;
			break ;
		}
	case AscendingSort:
		{
			sort = "ascending" ;
			break ;
		}
	case DescendingSort:
		{
			sort = "descending" ;
			break ;
		}
	default:
		{
			// Internal error
			xmlStream << "<!-- Invalid sort --> " ;
			sort = "ERROR" ;
		}
	}
	WriteStringAttr(xmlStream, "sort", sort) ;
	WriteBoolAttr(xmlStream, "visible", column.visible) ;
	xmlStream << ">\n" ;

	if ( column.type == ColumnValue )
	{
		// Handle the simple case of a raw value
		xmlStream << "        <value" ;
		WriteEventId(xmlStream, "id", column.dataSelectLeft) ;
		xmlStream << " />\n" ;
	} else {
		// Handle arithmetic relationships (sum, ratio, etc.)
		WriteArith(xmlStream, column) ;
	}

	xmlStream << "      </column>\n" ;
}

//
// Write the <output> element to the XML output stream.
//
void CViewConfig::WriteOutput(QTextStream& xmlStream)
{
	QString sort, left, right ;
	xmlStream << "    <output>\n" ;
	for (int i = 0 ; i < m_numberOfColumns ; i++)
	{
		// Write <column> element for each column in the view
		WriteColumn(xmlStream, m_columnSpecs[i]) ;
	}
	xmlStream << "    </output>\n" ;
}

//
// Write the <view> element to the XML output stream.
//
void CViewConfig::WriteView(QTextStream& xmlStream)
{
	xmlStream << "  <view" ;
	WriteStringAttr(xmlStream, "name", m_configName) ;
	WriteBoolAttr(xmlStream, "separate_cpus", m_separateCPUs) ;
	WriteBoolAttr(xmlStream, "separate_processes", m_separateProcesses) ;
	WriteBoolAttr(xmlStream, "separate_threads", m_separateThreads) ;
	WriteBoolAttr(xmlStream, "default_view", m_defaultView) ;
	WriteBoolAttr(xmlStream, "show_percentage", m_showPercentage) ;
	xmlStream << ">\n" ;
	WriteData(xmlStream) ;
	WriteOutput(xmlStream) ;
	WriteTooltip(xmlStream, m_toolTip) ;
	WriteDescription(xmlStream, m_description) ;
	xmlStream << "  </view>\n" ;
}

//
// Write the view configuration element to the XML output stream.
//
VIEWCONFIGURE_API bool CViewConfig::WriteConfigFile(wchar_t *configFileName)
{
	QFile* pFile = NULL ;
	QTextStream xmlStream ;

	pFile = new QFile( QString::fromUcs2 ((const short unsigned int *)configFileName) ) ;
	if (! pFile->open( QIODevice::WriteOnly )) {
		// Fill open error
		return( false ) ;
	}
	xmlStream.setDevice( pFile ) ;
	// Set XML file character encoding
	xmlStream.setEncoding(QTextStream::UnicodeUTF8) ;
	//	xmlStream.setEncoding(QTextStream::Unicode) ;

	xmlStream << "<view_configuration>\n" ;
	WriteView(xmlStream) ;
	xmlStream << "</view_configuration>\n" ;

	// Close file and deallocate QFile explicitly
	xmlStream.unsetDevice() ;
	pFile->close() ;
	delete pFile ;
	return( true ) ;
}

///////////////////////////////////////////////////////////////////////
// Read view confguration from XML file
///////////////////////////////////////////////////////////////////////

//
// Set up the XML file for reading, then start the parse. The XML parse
// proceeds and calls the callback functions below. Return false if the
// parser detected an error.
//
VIEWCONFIGURE_API bool CViewConfig::ReadConfigFile(wchar_t *configFileName)
{
	// Create a file, an XML input source and a simple reader
	QFile xmlFile( QString::fromUcs2((const short unsigned int *)configFileName) ) ;
	QXmlInputSource source( xmlFile ) ;
	QXmlSimpleReader reader ;
	// Connect this object's handler interface to the XML reader
	reader.setContentHandler( this ) ;
	// Return true if the parse succeeds and no XML semantic errors
	return( reader.parse( source ) && (!m_xmlSemanticError) ) ;
}

///////////////////////////////////////////////////////////////////////
// Callback functions for reading a view configuration in XML
///////////////////////////////////////////////////////////////////////

//
// startDocument() is called at the beginning of an XML document.
// Prepare to read a data collection configuration by clearing the
// current configuration.
//
VIEWCONFIGURE_API bool CViewConfig::startDocument()
{
	m_configName = DefaultConfigName ;
	m_toolTip = "" ;
	m_description = "" ;
	m_separateCPUs = false ;
	m_separateProcesses = false ;
	m_separateThreads = false ;
	m_defaultView = false ;
	m_showPercentage = false ;
	// Discard any column specifications
	if (m_columnSpecs != NULL) delete [] m_columnSpecs ;
	m_columnSpecs = NULL ;
	m_numberOfColumns = 0 ;
	// Discard any event ID/config associations
	m_assocList.clear() ;

	// Initialize all XML-related working variables
	m_viewIsOpen = false ;
	m_dataIsOpen = false ;
	m_outputIsOpen = false ;
	m_columnIsOpen = false ;
	m_toolTipIsOpen = false ;
	m_descriptionIsOpen = false ;
	m_xmlSemanticError = false ;
	m_columnList.clear() ;

	return( true ) ;
}

//
// endDocument() is called at the beginning of an XML document.
//
VIEWCONFIGURE_API bool CViewConfig::endDocument()
{
	return( true ) ;
}

//
// startElement() is called at the beginning of an XML element.
// Dispatch control using the element name. Handle an element-specific
// attributes. Set-up state variables to handle the contents of the
// element as the XML parse proceeds.
//
VIEWCONFIGURE_API bool CViewConfig::startElement(
	const QString &namespaceURI, const QString &localName,
	const QString &qName, const QXmlAttributes &atts)
{
	Q_UNUSED (namespaceURI);
	Q_UNUSED (localName);
	QString boolValue ;
	QString elementName = qName.lower() ;

	if ( elementName == "view" ) {
		/////////////////////////////////////////////////////////
		// <view> element
		/////////////////////////////////////////////////////////
#ifdef _DEBUG
		assert( !atts.value("name").isNull() ) ;
#endif
		// Use the default name when configuration name is missing
		if (! atts.value("name").isNull()) 
		{
			m_configName = atts.value("name") ;
		}
		if (! atts.value("separate_cpus").isNull()) 
		{
			boolValue = atts.value("separate_cpus").upper() ;
			m_separateCPUs = (boolValue == "T") ;
		}
		if (! atts.value("separate_processes").isNull()) 
		{
			boolValue = atts.value("separate_processes").upper() ;
			m_separateProcesses = (boolValue == "T") ;
		}
		if (! atts.value("separate_threads").isNull()) 
		{
			boolValue = atts.value("separate_threads").upper() ;
			m_separateThreads = (boolValue == "T") ;
		}
		if (! atts.value("default_view").isNull()) 
		{
			boolValue = atts.value("default_view").upper() ;
			m_defaultView = (boolValue == "T") ;
		}
		if (! atts.value("show_percentage").isNull()) 
		{
			boolValue = atts.value("show_percentage").upper() ;
			m_showPercentage = (boolValue == "T") ;
		}
		m_viewIsOpen = true ;
	} else if ( elementName == "data" ) {
		/////////////////////////////////////////////////////////
		// <data> element
		/////////////////////////////////////////////////////////
		m_assocList.clear() ;
		m_dataIsOpen = true ;
	} else if ( elementName == "event" ) {
		/////////////////////////////////////////////////////////
		// <event> element
		/////////////////////////////////////////////////////////
		DataAssoc da ;
		unsigned int select, unitMask ;
#ifdef _DEBUG
		assert( !atts.value("id").isNull() ) ;
		assert( !atts.value("select").isNull() ) ;
		assert( !atts.value("mask").isNull() ) ;
#endif
		select = atts.value("select").toInt(0, 16) ;
		unitMask = atts.value("mask").toInt(0, 16) ;
		da.eventId = atts.value("id") ;
		da.eventConfig = EncodeConfig(select, unitMask) ;
		m_assocList.push_back(da) ;
	} else if ( elementName == "output" ) {
		/////////////////////////////////////////////////////////
		// <output> element
		/////////////////////////////////////////////////////////
		// Discard an pre-existing column specifications
		if (m_columnSpecs != NULL) delete [] m_columnSpecs ;
		m_columnSpecs = NULL ;
		m_numberOfColumns = 0 ;
		m_columnList.clear() ;
		m_outputIsOpen = true ;
	} else if ( elementName == "column" ) {
		/////////////////////////////////////////////////////////
		// <column> element
		/////////////////////////////////////////////////////////
		ColumnSpec cs ;
		cs.type = ColumnInvalid ;
		cs.sorting = NoSort ;
		cs.visible = true ;
		cs.dataSelectLeft = DefaultEventConfig ;
		cs.dataSelectRight = DefaultEventConfig ;
		if (! atts.value("title").isNull())
		{
			cs.title = atts.value("title") ;
		} else {
			cs.title = DefaultColumnTitle ;
		}
		if (atts.value("sort").lower() == "ascending")
		{
			cs.sorting = AscendingSort ;
		} else if (atts.value("sort").lower() == "descending")
		{
			cs.sorting = DescendingSort ;
		}
		if (! atts.value("visible").isNull()) 
		{
			boolValue = atts.value("visible").upper() ;
			cs.visible = (boolValue == "T") ;
		}
		m_columnList.push_back(cs) ;
		m_numberOfColumns++ ;
		m_columnIsOpen = true ;
	} else if ( elementName == "value" ) {
		/////////////////////////////////////////////////////////
		// <value> element
		/////////////////////////////////////////////////////////
		if (m_columnIsOpen && (! m_columnList.empty()) )
		{
			ColumnSpec& cs = m_columnList.back() ;
			cs.type = ColumnValue ;
			cs.dataSelectLeft = GetConfigById(atts.value("id")) ;
		}
	} else if ( elementName == "ratio" ) {
		/////////////////////////////////////////////////////////
		// <ratio> element
		/////////////////////////////////////////////////////////
		if (m_columnIsOpen && (! m_columnList.empty()) )
		{
			ColumnSpec& cs = m_columnList.back() ;
			cs.type = ColumnRatio ;
			cs.dataSelectLeft = GetConfigById(atts.value("left")) ;
			cs.dataSelectRight = GetConfigById(atts.value("right")) ;
		}
	} else if ( elementName == "sum" ) {
		/////////////////////////////////////////////////////////
		// <sum> element
		/////////////////////////////////////////////////////////
		if (m_columnIsOpen && (! m_columnList.empty()) )
		{
			ColumnSpec& cs = m_columnList.back() ;
			cs.type = ColumnSum ;
			cs.dataSelectLeft = GetConfigById(atts.value("left")) ;
			cs.dataSelectRight = GetConfigById(atts.value("right")) ;
		}
	} else if ( elementName == "difference" ) {
		/////////////////////////////////////////////////////////
		// <difference> element
		/////////////////////////////////////////////////////////
		if (m_columnIsOpen && (! m_columnList.empty()) )
		{
			ColumnSpec& cs = m_columnList.back() ;
			cs.type = ColumnDifference ;
			cs.dataSelectLeft = GetConfigById(atts.value("left")) ;
			cs.dataSelectRight = GetConfigById(atts.value("right")) ;
		}
	} else if ( elementName == "product" ) {
		/////////////////////////////////////////////////////////
		// <product> element
		/////////////////////////////////////////////////////////
		if (m_columnIsOpen && (! m_columnList.empty()) )
		{
			ColumnSpec& cs = m_columnList.back() ;
			cs.type = ColumnProduct ;
			cs.dataSelectLeft = GetConfigById(atts.value("left")) ;
			cs.dataSelectRight = GetConfigById(atts.value("right")) ;
		}
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
	} else if ( elementName == "view_configuration" ) {
		/////////////////////////////////////////////////////////
		// <view_configuration> 
		/////////////////////////////////////////////////////////
	} else {
		/////////////////////////////////////////////////////////
		// Unrecognized element name 
		/////////////////////////////////////////////////////////
		return( false ) ;
	}
	return( true ) ;
}

//
// endElement() is called at the end of an XML element. Dispatch
// control using the element name to perform element-specific
// finalization of the element's contents.
//
VIEWCONFIGURE_API bool CViewConfig::endElement(
	const QString &namespaceURI, const QString &localName,
	const QString &qName)
{
	Q_UNUSED (namespaceURI);
	Q_UNUSED (localName);
	QString elementName = qName.lower() ;
	if ( elementName == "data" ) {
		/////////////////////////////////////////////////////////
		// </data>
		/////////////////////////////////////////////////////////
		m_dataIsOpen = false ;
	} else if ( elementName == "output" ) {
		/////////////////////////////////////////////////////////
		// </data>
		/////////////////////////////////////////////////////////
		ColumnSpecList::const_iterator cs ;
		ColumnSpec *pCS ;
#ifdef _DEBUG
		assert( m_numberOfColumns >= 1 ) ;
		assert( m_numberOfColumns == m_columnList.size() ) ;
#endif
		// Allocate array to hold column specifications
		m_columnSpecs = new ColumnSpec[m_numberOfColumns] ;
		// Copy event groups from list to the new array
		pCS = m_columnSpecs ;
		for (cs = m_columnList.begin() ; cs != m_columnList.end() ; cs++) {
			pCS->type = cs->type ;
			pCS->sorting = cs->sorting ;
			pCS->visible = cs->visible ;
			pCS->title = cs->title ;
			pCS->dataSelectLeft = cs->dataSelectLeft ;
			pCS->dataSelectRight = cs->dataSelectRight ;
			pCS++ ;
		}
		m_columnList.clear() ;
		m_outputIsOpen = false ;
	} else if ( elementName == "column" ) {
		/////////////////////////////////////////////////////////
		// </data>
		/////////////////////////////////////////////////////////
		m_columnIsOpen = false ;
	} else if ( elementName == "tool_tip" ) {
		/////////////////////////////////////////////////////////
		// </data>
		/////////////////////////////////////////////////////////
		m_toolTipIsOpen = false ;
	} else if ( elementName == "description" ) {
		/////////////////////////////////////////////////////////
		// </data>
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
VIEWCONFIGURE_API bool CViewConfig::characters(const QString &ch)
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
