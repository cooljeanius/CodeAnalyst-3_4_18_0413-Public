#ifndef _CADATAWRITER_H_
#define _CADATAWRITER_H_

#include <string>
#include <stdio.h>
#include "libCAdata.h"

using namespace std;

/****************************
 * class CaDataWriter
 * 
 * Description:
 * This is the base class for all writer.
 */
class CaDataWriter
{
public:
	enum output_stage {
		evOut_OK = 0
	};

	CaDataWriter();

	virtual ~CaDataWriter();

	virtual bool open(wstring path);
	
	void close ();

protected:
	wstring			m_path;
	FILE			*m_pFile;
	unsigned int	m_stage;
};


#endif //ifndef _CADATAWRITER_H_
