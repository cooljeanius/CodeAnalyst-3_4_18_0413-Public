
/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2008 Advanced Micro Devices, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "smafile.h"

SMAFileWriter::SMAFileWriter()
{
	m_file = NULL;
	m_strtab_file = NULL;
	m_scnhdrs = NULL;
	m_currScn = 0;
}


SMAFileWriter::~SMAFileWriter()
{
	cleanup();
}

void SMAFileWriter::cleanup()
{
	if (m_file) {
		fclose(m_file);
		m_file = NULL;
	}	

	if (m_strtab_file) {
		fclose (m_strtab_file);
		m_strtab_file = NULL;
	}

	if (m_scnhdrs) {
		free(m_scnhdrs);
		m_scnhdrs = NULL;
	}

	m_currScn = 0;

}

int 
SMAFileWriter::createSMAFile(const char *fullpath, 
				unsigned int section_num)
{
	int rc = BBA_OK;
	
	// open temporary file for strtab in binary read/write (w+b) mode. 
	m_strtab_file = tmpfile();	
	
	m_file = fopen(fullpath, "w+b");

	if (!m_strtab_file || !m_file) {
		cleanup();
		return CANNOT_OPEN;
	}

	// we automatically have .strtab section
	m_scnhdrs = (SMA_SHE*) malloc(sizeof(SMA_SHE) * (section_num + 1));
	if (!m_scnhdrs) {
		cleanup();
		return NO_MEMORY;
	}

	m_smahdr.scn_num = section_num + 1;

	// write file header first; we will update some content later;
	fwrite(&m_smahdr, sizeof (SMA_HEADER), 1, m_file);

	m_smahdr.sht_offset = ftell(m_file);
	// write the headers for the rest sections
	fwrite(&m_scnhdrs, sizeof(SMA_SHE), m_smahdr.scn_num, m_file);

	// write section header for string table section 	
	strcpy(m_scnhdrs[0].scn_name, ".STRTAB");
	m_scnhdrs[0].scn_type = SMA_SCN_STRTAB;
	m_currScn = 1;


	// write .STRTAB into str table section
	unsigned char tmpstr[10] = { 0x00, 0x00,
			0x2E, // . 
			0x53, // S 
			0x54, // T 
			0x52, // R
			0x54, // T
			0x41, // A
			0x42, // B
			0x00};
	fwrite((void*) tmpstr, sizeof(unsigned char), 10, m_strtab_file);

	return rc;
}

int
SMAFileWriter::createSMAFile(const char *fullpath, const char *target,
				long long imagebase, unsigned int bitness,
				long long modtime, unsigned int modsize,
				unsigned int section_num)
{
	if (!target)
		return EMPTY_TARGET_NAME;

	int len = strlen(target);
	if (len > MAX_LENGTH)
		return LONG_TARGETNAME;

	int rc = BBA_OK;
	if ((rc = createSMAFile(fullpath, section_num)) != BBA_OK)
		return rc;

	memcpy(m_smahdr.signature, SMA_FILE_SIGNATURE, SMA_STRING_SIZE);
	m_smahdr.ver_major = CURRENT_MAJOR;
	m_smahdr.ver_minor = CURRENT_MINOR;
	m_smahdr.mod_imagebase = imagebase;
	m_smahdr.mod_format = SMA_ELF;
	m_smahdr.mod_bitness = bitness;
	m_smahdr.mod_time = modtime;
	m_smahdr.mod_size = modsize;
	m_smahdr.mod_name = ftell(m_strtab_file);
	

	fwrite((void*) target, sizeof(char), len + 1, m_strtab_file);

	return rc;
}


int 
SMAFileWriter::addSection(unsigned int sectype, const char * secString)
{
	if (!m_scnhdrs || !m_file || !m_strtab_file)
		return FAILED;

	if (sectype <= SMA_SCN_INVALID || sectype >= SMA_SCN_BOUNDARY)
		return INVALID_SECTION_TYPE; 

	if (m_currScn >= m_smahdr.scn_num)
		return OVER_SECTION_LIMIT;

	if (m_scnhdrs[m_currScn].scn_type != SMA_SCN_INVALID) {
		// indicate this section had been used
		return FAILED;
	}
	
	SMA_SHE *p_she = (SMA_SHE*) &m_scnhdrs[m_currScn];

	p_she->scn_type = sectype;
	if (secString) {
		if (strlen(secString) > 7) {
			strncpy(p_she->scn_name, secString, 7);
			p_she->scn_name[7] = 0x00;
		} else {
			strcpy(p_she->scn_name, secString);
		}	
			
	} else {
		switch (sectype) {
			case SMA_SCN_STRTAB:
				strcpy(p_she->scn_name, ".STRTAB");
				break;

			case SMA_SCN_DMOD:           // Dependent Module
				strcpy(p_she->scn_name, ".DEPMOD");
				break;

			case SMA_SCN_IF:             // Import Function
				strcpy(p_she->scn_name, ".IF");
				break;

			case SMA_SCN_EF:
				strcpy(p_she->scn_name, ".EF");
				break;

			case SMA_SCN_WRAPPER:
				strcpy(p_she->scn_name, ".WRAP");
				break;

			case SMA_SCN_REP:
				strcpy(p_she->scn_name, ".REP");
				break;

			case SMA_SCN_APIMON:         // monitored APIs
				strcpy(p_she->scn_name, ".APIMON");
				break;

			case SMA_SCN_BASICBLOCK:     // basic block section
				strcpy(p_she->scn_name, ".BBLOCK");
				p_she->scn_rec_size = sizeof(BASIC_BLOCK_INFO);
				break;

			case SMA_SCN_JMP:            // jump instruction section
				strcpy(p_she->scn_name, ".JUMP");
				p_she->scn_rec_size = sizeof(BR_INSTR_INFO);
				break;

			case SMA_SCN_CALL:           // call instruction section
				strcpy(p_she->scn_name, ".CALL");
				p_she->scn_rec_size =sizeof(BR_INSTR_INFO);
				break;

			case SMA_SCN_RET:            // return instruciton
				strcpy(p_she->scn_name, ".RET");
				p_she->scn_rec_size =sizeof(BR_INSTR_INFO);
				break;

			case SMA_SCN_MEMORYACCESS:
				strcpy(p_she->scn_name, ".MEMACC");
				p_she->scn_rec_size =sizeof(MEMACC_INST);
				break;

			default:
			case SMA_SCN_USERDEFINED:
				strcpy(p_she->scn_name, ".UNKWN");
				break;
		}

	}

	// NOTE: 
	// We don't write section header to file at here.
	// Will do it at end when we complete SMA file. 
	// 
	p_she->scn_offset = ftell(m_file);
	p_she->scn_size = 0;
	p_she->scn_rec_num = 0;

	// Write a terminator into string tab for this section
	// this will make record stirng point to empty by default
	p_she->scn_strtab_offset = ftell(m_strtab_file);
	unsigned char pad = 0x00;
	fwrite(&pad, 1, 2, m_strtab_file);
	
	return BBA_OK;
}

int
SMAFileWriter::addSecRecord(void *pRecord, unsigned int size,const char *recStr)
{
	if (!m_scnhdrs || !pRecord)
		return FAILED;

	unsigned int t_size = 0;
	unsigned int strOffset = 0;
	unsigned int stringlen = 0;
	int rc = BBA_OK;
	
	SMA_SHE *p_she = (SMA_SHE*) &m_scnhdrs[m_currScn];
	switch(p_she->scn_type) {
		case SMA_SCN_BASICBLOCK:     // basic block section
			t_size = p_she->scn_rec_size;
			if (size < t_size)
				rc = DATA_SIZE_MISMATCH;
			break;

		case SMA_SCN_JMP:            // jump instruction section
		case SMA_SCN_CALL:           // call instruction section
		case SMA_SCN_RET:            // return instruciton
			t_size = p_she->scn_rec_size;
			if (size < t_size)
				rc = DATA_SIZE_MISMATCH;
			break;

		case SMA_SCN_MEMORYACCESS:
			t_size = p_she->scn_rec_size; 
			if (size < t_size)
				rc = DATA_SIZE_MISMATCH;
			break;

		default:
			t_size = size;
	}

	if (rc == BBA_OK) {
		// write record;
		p_she->scn_rec_num++;
		fwrite(pRecord, t_size, 1, m_file);

		// write string table
		if (recStr) {
			strOffset = ftell(m_strtab_file) - p_she->scn_strtab_offset;
			stringlen = strlen(recStr) + 1;

			// write string table offset and string length	
			fwrite(&strOffset, sizeof(unsigned int), 1, m_file);
			fwrite(&stringlen, sizeof(unsigned int), 1, m_file);

			// write the string into string table
			fwrite(recStr, 1, stringlen, m_strtab_file);
		} else {
			// no string, write string offset 0;
			fwrite(&strOffset, sizeof(unsigned int), 1, m_file);
			fwrite(&stringlen, sizeof(unsigned int), 1, m_file);
		}
	}

	return rc;
}



int 
SMAFileWriter::completeSection(unsigned int sectype)
{
	if (!m_scnhdrs || !m_file || !m_strtab_file)
		return FAILED;

	if (sectype <= SMA_SCN_INVALID || sectype >= SMA_SCN_BOUNDARY)
		return INVALID_SECTION_TYPE; 

	if (m_currScn >= m_smahdr.scn_num)
		return OVER_SECTION_LIMIT;

	SMA_SHE *p_she = (SMA_SHE*) &m_scnhdrs[m_currScn];

	if (p_she->scn_type != sectype)
		return SECTION_TYPE_MISMATCH;

	int rc = BBA_OK;
	unsigned char pad = 0x00;
	unsigned int curpos = ftell(m_file);
	unsigned int padlen = ALIGNMENT - (curpos % ALIGNMENT);
	if (padlen != ALIGNMENT) {
		// padding to make next section address aligned.
		fwrite(&pad, 1, padlen, m_file);
	}
printf("Lei in completeSection: scn_size = %d\n", p_she->scn_rec_num);

	p_she->scn_size = ftell(m_file) - p_she->scn_offset;
	m_currScn++;

	return rc;
}

int 
SMAFileWriter::completeSMAFile()
{
	if (!m_scnhdrs || !m_file || !m_strtab_file)
		return FAILED;

	int rc = BBA_OK;
	unsigned int leftdata = 0;

	// update section header for .STRTAB section
	SMA_SHE *p_she1 = m_scnhdrs;
	for (unsigned int i = 0; i < m_smahdr.scn_num; i++)  {
		printf("lei in completeSMAFile: %d, scn_rec_num = %d\n", 
						i, p_she1->scn_rec_num);
		p_she1++;
	}

	SMA_SHE *p_she = m_scnhdrs;
	for (unsigned int i = 0; i < m_smahdr.scn_num; i++) {

		if(p_she->scn_type != SMA_SCN_STRTAB) {
			p_she++;
			continue;
		}

		p_she->scn_offset = ftell(m_file);

		p_she->scn_size = ftell(m_strtab_file);
		leftdata = p_she->scn_size;
		break;
	}

	unsigned char buffer[ALIGNMENT];
	rewind(m_strtab_file);

	// append strtab into current sma file
	while (leftdata) {
		if (leftdata >= ALIGNMENT) {
			fread(buffer, 1, ALIGNMENT, m_strtab_file);
			fwrite(buffer, 1, ALIGNMENT, m_file);
			leftdata -= ALIGNMENT;
		} else {
			fread(buffer, 1, leftdata, m_strtab_file);
			fwrite(buffer, 1, leftdata, m_file);
			leftdata = 0;
		}
	};

	//update file header
	m_smahdr.file_time = (long long) time(NULL);
	m_smahdr.file_size = ftell(m_file);
	
	//rewind the file and re-write the file header;
	rewind(m_file);
	fwrite(&m_smahdr, sizeof(SMA_HEADER), 1, m_file);

	// re-write the section headers;
	fseek(m_file, m_smahdr.sht_offset, SEEK_SET);
	fwrite(m_scnhdrs, sizeof(SMA_SHE), m_smahdr.scn_num, m_file);
	

	// move position indicator to end of file
	fseek(m_file, m_smahdr.file_size, SEEK_SET);
	//cleanup();

	return rc;
}



SMAFileReader::SMAFileReader()
{
	m_file = NULL;
	m_scnhdrs = NULL;
	m_currScn = 0;
}



SMAFileReader::~SMAFileReader()
{
	closeSMAFile();
}

int
SMAFileReader::openSMAFile(const char *fullpath)
{
	unsigned rc = BBA_OK;
	if ((m_file = fopen(fullpath, "r+b")) == NULL)
		return CANNOT_OPEN;
	
	unsigned int filesize = 0;

	fseek(m_file, 0, SEEK_END);
	filesize = ftell(m_file);
	if (filesize <= sizeof(SMA_HEADER)) {
		rc = FORMAT_ERROR;	
		goto EXIT;
	}

	// rewind the file
	rewind(m_file);

	fread(&m_smahdr, sizeof(SMA_HEADER), 1, m_file);
	memcpy(m_smahdr.signature, SMA_FILE_SIGNATURE, SMA_STRING_SIZE);
	if (strncmp(m_smahdr.signature, SMA_FILE_SIGNATURE, SMA_STRING_SIZE) !=0) {
		rc = FORMAT_ERROR;
		goto EXIT;
	}

	if ((filesize != m_smahdr.file_size) || 
		(filesize < sizeof(SMA_HEADER) + sizeof(SMA_SHE) * m_smahdr.scn_num)){
		rc = FORMAT_ERROR;
		goto EXIT;
	}

	m_scnhdrs = (SMA_SHE*) malloc(sizeof(SMA_SHE) * (m_smahdr.scn_num));
	if (!m_scnhdrs) {
		rc = NO_MEMORY;
		goto EXIT;
	}

	fread(m_scnhdrs, sizeof(SMA_SHE), m_smahdr.scn_num, m_file);

EXIT:
	if (rc != BBA_OK)
		closeSMAFile();

	return rc;
}


SMA_HEADER *
SMAFileReader::getFileHeader()
{
	return &m_smahdr;
}

unsigned int
SMAFileReader::getSectionNum()
{
	return m_smahdr.scn_num;	
}

SMA_SHE*
SMAFileReader::getSectionHeaders()
{
	return m_scnhdrs;
}

int
SMAFileReader::getSectionHeader(unsigned int sectiontype, SMA_SHE *pshe)
{
	int rc = FAILED;

	if (!m_scnhdrs || !pshe)
		return rc;

	for (unsigned int i = 0; i < m_smahdr.scn_num; i++) {
		if (m_scnhdrs[i].scn_type == sectiontype) {
			memcpy(pshe, &m_scnhdrs[i], sizeof(SMA_SHE));
			rc = BBA_OK;
			break;
		}
	}

	return rc;
}

int
SMAFileReader::setReadSection(unsigned int sectype)
{
	int rc = FAILED;
	if (!m_scnhdrs || !m_file)
		return rc;

	for (unsigned int i = 0; i < m_smahdr.scn_num; i++) {
		if (m_scnhdrs[i].scn_type == sectype) {
			// find section, move file descriptor
			fseek(m_file, m_scnhdrs[i].scn_offset, SEEK_SET);
			m_currScn = i;
printf("Section:%d, rec_size = %d, rec_num=%d, scn_size = %d\n",
			sectype, m_scnhdrs[i].scn_rec_size, 
			m_scnhdrs[i].scn_rec_num,
			m_scnhdrs[i].scn_size);

			rc = BBA_OK;
			break;
		}	
	}
	
	return rc;
}

int
SMAFileReader::getNextSecRecord(void* pRecord, unsigned int size,
				char *recStr, unsigned int strsize)
{
	int rc = BBA_OK;

	long curpos;
	unsigned int stringoffset = 0;
	unsigned int stringlen = 0;

	if (!m_file || !m_scnhdrs )
		return FAILED;

	SMA_SHE *pshe = &m_scnhdrs[m_currScn];

	// check record type and size;
	if (size < pshe->scn_rec_size)
		return FAILED;

	curpos = ftell(m_file);
	if (curpos < pshe->scn_offset 
	|| curpos >= (long)( pshe->scn_offset + (pshe->scn_rec_size + sizeof(unsigned) * 2) * pshe->scn_rec_num) ) {
		// each record has 2 implict (unsigned int) to save string table
		//offset and string length
		return FAILED;
	}


	fread(pRecord, pshe->scn_rec_size, 1, m_file);

	fread(&stringoffset, sizeof(unsigned int), 1, m_file);
	fread(&stringlen, sizeof(unsigned int), 1, m_file);

	curpos = ftell(m_file);

	if (recStr && stringoffset != 0) { 
		if (strsize < stringlen)
			return FAILED;

		// adjust to string table and read string;
		fseek(m_file, m_smahdr.strtab_offset + pshe->scn_strtab_offset +
						stringoffset, SEEK_SET);
		fread(recStr, sizeof(char), stringlen, m_file);

		// move file descriptor back to section
		fseek(m_file, curpos, SEEK_SET);
	}	

	return rc;
}

void
SMAFileReader::closeSMAFile()
{
	if (m_file) {
		fclose(m_file);
		m_file = NULL;
	}

	if (m_scnhdrs) {
		free(m_scnhdrs);
		m_scnhdrs = NULL;
	}
}

