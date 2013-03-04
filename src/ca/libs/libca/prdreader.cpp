//$Id: prdreader.cpp,v 1.2 2006/05/15 22:09:45 jyeh Exp $

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


#include "prdreader.h"


const int MAX_COUNTERS = 4;

CPRDReader::CPRDReader()
	: m_PRDFileName("")
{
	m_IStream = NULL;
    m_nRecords = 0;
}

CPRDReader::~CPRDReader()
{
    m_FileBuf.close();
	if (NULL != m_IStream)
		delete m_IStream;
}

bool CPRDReader::openPRDFile(string fileName)
{
	bool ret = true;
	
    m_PRDFileName = fileName;
    
    if ("" != m_PRDFileName 
        && m_FileBuf.open(m_PRDFileName.c_str(), ios::in)) {

		m_IStream = new istream(&m_FileBuf);
	}

	if (NULL == m_IStream)
		ret = false;
    
	return ret;
}


void CPRDReader::closePRDFile()
{
    m_FileBuf.close();
	if (NULL != m_IStream)
		delete m_IStream;
}

bool CPRDReader::readPRDHeader(prd_header & header)
{
    bool ret = false;
	if (NULL == m_IStream)
		return ret;

	m_IStream->read((char *)&header.signature, sizeof(header.signature));
	m_IStream->read((char *)&header.version, sizeof(header.version));
	m_IStream->read((char *)&header.counter, sizeof(header.counter));
	m_IStream->read((char *)&header.count, sizeof(header.count));

    
    if (0 == strncmp(header.signature, "CAPRDLIN", 8));
        ret = true;

#ifdef _DEBUG_
    printf("sizeof CPRDReader:header.signaure %d\n", sizeof(header.signature));

    printf("sizeof CPRDReader:header.versioin %d\n", sizeof(header.version));
    printf("sizeof CPRDReader:header.counter %d\n", sizeof(header.counter));

    printf("CPRDReader:header signagure %s\n", header.signature);
    printf("CPRDReader:header version %d\n", header.version);
    printf("CPRDReader:m_nRecords %d\n", m_nRecords);
    for (int i=0; i<MAX_COUNTERS; i++)
        printf("CPRDReaer:counter[%d] = %x\n", i, header.counter[i]);
#endif

    return ret;
}

bool CPRDReader::readNextRecord(prd_record * pRecord)
{
    bool bret = false;

#ifdef _DEBUG_
    printf("size of prd_record is %d ", sizeof(prd_record)); 
    printf("size of pRecord->time_stamp is %d ",
        sizeof(pRecord->time_stamp)); 
    printf("size of pRecord->eip is %d ",
        sizeof(pRecord->eip)); 
    printf("size of pRecord->tgid is %d ",
        sizeof(pRecord->tgid)); 
    printf("size of pRecord->event_index is %d ",
        sizeof(pRecord->event_index)); 
    printf("size of pRecord->cpu_nr is %d ",
        sizeof(pRecord->cpu_nr)); 
    printf("size of pRecord->padding is %d\n",
        sizeof(pRecord->padding)); 
#endif


    if ((NULL == pRecord) || m_IStream->eof()) {
        return bret;
    } else {
        m_IStream->read((char *)pRecord, sizeof(prd_record));
        bret = true;
    }

    return bret;
}


prd_record * CPRDReader::readPRDFileBlock(unsigned int n)
{
    bool ret = true;
    prd_record * pRecords = NULL;

    m_nRecords = n;

    pRecords = new prd_record[m_nRecords];

    if (NULL != pRecords)
        m_IStream->read((char *)pRecords, sizeof(prd_record) * m_nRecords);
   
    return pRecords;
}


