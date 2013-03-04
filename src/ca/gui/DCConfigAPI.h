// DCConfigAPI.h: API interface for DC configuration reader/write classes

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
#ifndef _DCCONFIGAPI_H_
#define _DCCONFIGAPI_H_

// Include Qt-related definitions
#include <qstring.h>
#include <qfile.h>
#include <QTextStream>
#include <qxml.h>
#include <q3valuelist.h>

#include <list>

#include "PerfEventContainer.h"
using namespace std ;

//
// Number of event counters per group. This is a platform-dependent
// constant which may somebody be defined in a central location!
//
static const int NumberOfCounters = 4 ;


typedef enum
{
	DCConfigInvalid,           // No or invalid DC configuration
	DCConfigTBP,               // Time-based profiling (TBP)
	DCConfigEBP,               // Event-based profiling (EBP)
	DCConfigTBPPerf            // PERF based Time-Based profiling
} DCConfigType ;


////////////////////////////////////////////////////////////////////////////////////
// CDCConfig class declaration
////////////////////////////////////////////////////////////////////////////////////

class CDCConfig : public QXmlDefaultHandler
{
public:
	///////////////////////////////////////////////////////////////////////////////
	// Constructors, destructors, assignment
	///////////////////////////////////////////////////////////////////////////////
	CDCConfig() ;
	CDCConfig(const CDCConfig& original) ;
	~CDCConfig() ;
	const CDCConfig& operator = (const CDCConfig& rhs) ;

	///////////////////////////////////////////////////////////////////////////////
	// Client functions for all DC configuration types
	///////////////////////////////////////////////////////////////////////////////
	// Return the configuration type (TBP, EBP, ...)
	DCConfigType GetConfigType() ;
	// Return the configuration name in configName
	void GetConfigName(QString& configName) ;
	// Return the configuration tool type in toolTip
	void GetToolTip(QString& toolTip) ;
	// Return the configuration description in description
	void GetDescription(QString& description) ;

	// Set the configuration type
	void SetConfigType(DCConfigType configType) ;
	// Set the configuration name
	void SetConfigName(const QString configName) ;
	// Set the configuration tool tip
	void SetToolTip(const QString toolTip) ;
	// Set the configuration description
	void SetDescription(const QString description) ;
	// Set the cpu_type 
	void SetCpuType(const QString cpuType = QString()) ;
	

	// Read a DC configuration in XML from the specified file
	bool ReadConfigFile(wchar_t* configFileName) ;
	// Write the current configuration in XML to the specified file
	bool WriteConfigFile(wchar_t* configFileName) ;

	bool validateEvents(CEventsFile * pEventFile, QString & errorStr);

	///////////////////////////////////////////////////////////////////////////////
	// Client functions for TBP DC configurations
	///////////////////////////////////////////////////////////////////////////////
	// Set the TBP timer interval (sample period)
	void SetTimerInterval(float interval) ;
	// Get the TBP timer interval
	float GetTimerInterval() ;

	///////////////////////////////////////////////////////////////////////////////
	// Client functions for EBP DC configurations
	///////////////////////////////////////////////////////////////////////////////
	// Add event to the current container 
	bool AddEventInfo(PerfEvent & pEvent) ;
	// Set event groups and multiplex period for the configuration
	void SetEventInfo(PerfEventContainer * pEventContainer, UINT64 multiplexPeriod = 0) ;
	// Return the number of event groups in the configuration
	void GetEventInfo(PerfEventContainer * pEventContainer) ;
	// Get the multiplexing period; Multiplexing period is zero when disabled
	UINT64 GetMultiplexPeriod() ;
	// Set the multiplexing period; Multiplexing period is zero when disabled
	void SetMultiplexPeriod(UINT64 mux) ;

	///////////////////////////////////////////////////////////////////////////////
	// Callback functions which override QXmlDefaultHandler
	///////////////////////////////////////////////////////////////////////////////
	bool startDocument() ;
	bool endDocument() ;
	bool startElement( const QString& namespaceURI, 
			const QString& localName,
			const QString& qName, 
			const QXmlAttributes& atts) ;
	bool endElement( const QString& namespaceURI, 
			const QString& localName,
			const QString& qName) ;
	bool characters(const QString& ch) ;

private:
	// Common configuration attributes
	DCConfigType     m_configType ;        // Kind of configuration (TBP, EBP, ...)
	QString          m_configName ;        // Configuration name
	QString          m_toolTip ;           // Configuration's tool tip
	QString          m_description ;       // Description of the configuration
	// Time-based profiling configuration attributes
	float            m_interval ;          // Timer interval (sample period)
	// Event-based profiling configuration attributes
	UINT64           m_multiplexPeriod ;   // Multiplexing period (0: no multiplexing)
	bool             m_toolTipIsOpen ;     // Tooltip is currently open when true
	bool             m_descriptionIsOpen ; // Description is currently open
	bool             m_groupIsOpen ;       // Event group is currently open
	bool             m_xmlSemanticError ;  // Is true on XML semantic error
	QString		 m_cpuType;
	PerfEventContainer 	m_eventContainer;
//	CEventsFile	 * m_pEventsFile;

	///////////////////////////////////////////////////////////////////////////////
	// Private functions to assist configuration writing
	///////////////////////////////////////////////////////////////////////////////
	// Write tool tip element to the XML stream
	void WriteTooltip(QTextStream& xmlStream, QString& text) ;
	// Write description element to the XML stream
	void WriteDescription(QTextStream& xmlStream, QString& text) ;
	// Write string-valued attribute to the XML stream
	void WriteStringAttr(QTextStream& xmlStream, const char* attr, QString& value) ;
	// Write integer-valued attribute (decimal format) to the XML stream
	void WriteDecimalAttr(QTextStream& xmlStream, const char* attr,
		unsigned long long int value) ;
	// Write integer-valued attribute (hexadecimal format) to the XML stream
	void WriteHexAttr(QTextStream& xmlStream, const char* attr,
		unsigned long long int value) ;
	// Write float-valued attribute (decimal format) to the XML stream
	void WriteFloatAttr(QTextStream& xmlStream, const char* attr, float value) ;
	// Write Boolean-valued attribute to the XML stream
	void WriteBoolAttr(QTextStream& xmlStream, const char* attr, bool value) ;
	// Write TBP configuration to the XML stream
	void WriteTBP(QTextStream& xmlStream) ;
	// Write PERF TBP configuration to the XML stream
	void WriteTBPPerf(QTextStream& xmlStream) ;
	// Write EBP configuration to the XML stream
	void WriteEBP(QTextStream& xmlStream) ;
	// Write event element to the XML stream
	void WriteEvent(QTextStream& xmlStream, PerfEvent * e) ;
	// Setup events file.	
//	bool SetupEventsFile(QString cpuType)
} ;

#endif
