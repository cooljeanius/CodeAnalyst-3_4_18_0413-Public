#if ( defined (_WIN32) || defined (_WIN64) )
#include "stdafx.h"
#endif

#include "CaDataWriter.h"

using namespace std;

CaDataWriter::CaDataWriter()
{
	m_path.clear();
	m_pFile = NULL;
	m_stage = evOut_OK;
}


CaDataWriter::~CaDataWriter()
{
	close();

}


bool CaDataWriter::open(wstring path)
{
	bool ret = true;
	
#if ( defined (_WIN32) || defined (_WIN64) )
	if (0 != _wfopen_s(&m_pFile, path.c_str(), L"w, ccs=UTF-8")) {
#else
	string str(path.begin(), path.end());
	if ((m_pFile = fopen(str.c_str(), "wb")) == NULL) {
	//if ((m_pFile = fopen(str.c_str(), "w, css=UTF-8")) == NULL) {
#endif
		m_pFile = NULL;
		ret = false;
	} else 
		m_path = path;

	return ret;
}


void CaDataWriter::close()
{
	if (m_pFile) {
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

