//$Id: OpreportXMLHandler.h $
// Class for handling Opreport XML output.

/*
// CodeAnalyst for Open Source
// Copyright 2009 Advanced Micro Devices, Inc.
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

#ifndef _OPREPORTXMLHANDLER_H_
#define _OPREPORTXMLHANDLER_H_

#include <qxml.h>
#include <list>
#include <vector>
#include <map>

#include "OpreportXMLStruct.h"

using namespace std;

class OpreportXMLHandler : public QXmlDefaultHandler 
{
private:
	bool		m_countSample;
	bool		m_getCount;
	bool		m_isInBinary;
	op_event	m_event;
	op_class	m_class;
	op_binary 	*m_binary;
	op_process 	*m_process;
	op_thread 	*m_thread;
	op_module	m_module;
	op_symbol	m_symbol;
	op_symboldata	m_symboldata;
	op_symboldetails m_symboldetails;
	op_detaildata	m_detaildata;
	unsigned int	m_symbolCount;
	unsigned long	m_vmaOffset;
	unsigned long 	m_count;
	std::string	m_curTaskName;
	std::string	m_curModName;
	unsigned int	m_curSymbol;
	unsigned int	m_numSymbol;

	BINARY_LIST::iterator	bit;
	MODULE_LIST::iterator	mit;

	std::vector<op_event>		*m_eventVec;
	CLASS_MAP			*m_classMap;
	BINARY_LIST			*m_binaryList;
	PROCESS_LIST			*m_processList;
	std::map<unsigned int, op_symboldata>	*m_symbolMap;
	std::vector<op_symboldetails>	*m_symbolDetailsVec;

public:
	OpreportXMLHandler();

	virtual ~OpreportXMLHandler();

	void init(
		std::vector<op_event>		* eventVec,
		CLASS_MAP			* classMap,
		BINARY_LIST			* binaryList,
		PROCESS_LIST			* processList,
		std::vector<op_symboldetails>	* symbolDetailsVec,
		std::map<unsigned int ,op_symboldata>	* symbolMap = NULL);

	bool open(const char* event_file_path);

	void close();

	void attribute();

	///////////////////////////////////////////////////////////////////////
	// Method overrides for QXmlDefaultHandler
	///////////////////////////////////////////////////////////////////////

	bool startElement(const QString & namespaceURI, 
			  const QString & localName, 
			  const QString & qName, 
			  const QXmlAttributes & atts );

	bool endElement( const QString & namespaceURI, 
			const QString & localName, 
			const QString & qName );

	bool characters ( const QString & ch );

};

#endif /* _OPREPORTXMLHANDLER_H_ */
