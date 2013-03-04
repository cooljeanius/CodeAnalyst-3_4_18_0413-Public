
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

#ifndef _BBAFILE_H
#define _BBAFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "datatype.h"

#define SMA_FILE_SIGNATURE	"_CA.SMA_"

#define SMA_STRING_SIZE	8

#define CURRENT_MAJOR	1
#define CURRENT_MINOR	0

#define ALIGNMENT		0x10

typedef enum {
	SMA_ELF=0,
	SMA_PE_COFF
} SMA_FORMAT;

typedef enum {
	SMA_x86 = 0,
	SMA_x86_64
} SMA_BITNESS;

typedef enum {
	SMA_SCN_INVALID = 0,
	SMA_SCN_STRTAB,
	SMA_SCN_DMOD,			// Dependent Module
	SMA_SCN_IF,				// Import Function
	SMA_SCN_EF,
	SMA_SCN_WRAPPER,
	SMA_SCN_REP,
	SMA_SCN_APIMON,			// monitored APIs
	SMA_SCN_BASICBLOCK,		// basic block section
	SMA_SCN_JMP,			// jump instruction section
	SMA_SCN_CALL,			// call instruction section
	SMA_SCN_RET,			// return instruciton
	SMA_SCN_MEMORYACCESS,

	SMA_SCN_USERDEFINED,

	SMA_SCN_BOUNDARY		// use this as boundary check. ALWAYS last one.
} SMA_SECTION_TYPE;

// total 64 bytes
typedef struct { 
	char 			signature[SMA_STRING_SIZE];
	unsigned int	ver_major;
	unsigned int	ver_minor;
	long long		mod_imagebase;
	unsigned int	mod_format;
	unsigned int	mod_bitness;
	unsigned int	mod_name;		// offset to strtab base;
	long long 		mod_time;		// time_t is (long int) type;
	unsigned int	mod_size;
	unsigned int	sht_offset;
	unsigned int	scn_num;
	long long 		file_time;		// time_t
	unsigned int	file_size;
	unsigned int	strtab_offset;
} SMA_HEADER;	



// Total 32 bytes
typedef struct _SMA_SHE {
	_SMA_SHE() {
		memset(scn_name, 0x00, SMA_STRING_SIZE);
		scn_type = SMA_SCN_INVALID;
		scn_offset = 0;
		scn_size = 0;
		scn_rec_num = 0;
		scn_rec_size = 0;
		scn_strtab_offset = 0;
	};	
	char			scn_name[SMA_STRING_SIZE];
	unsigned int	scn_type;			// SMA_SECTION_TYPE;
	unsigned int 	scn_offset;			// to file base
	unsigned int	scn_size;			// total section size (include padding)
	unsigned int	scn_rec_num;		// section record number
	unsigned int	scn_rec_size;		// size of each record in the section
	unsigned int	scn_strtab_offset ;	// offset to STRTAB base
} SMA_SHE;	

class SMAFileWriter {
public:
	SMAFileWriter();
	~SMAFileWriter();

	int createSMAFile(const char *fullpath, unsigned int section_num = 10);
	int createSMAFile(const char *fullpath, const char *target,
					long long imagebase, unsigned int bitness, 
					long long modtime, unsigned int modsize,
					unsigned int section_num = 10);

	int addSection(unsigned int sectype, const char *sec_string = NULL);
	int addSecRecord(void *pRecord, unsigned int size, const char *recStr=NULL);
	int completeSection(unsigned int sectype);

	int completeSMAFile();
	void cleanup();


private:
	FILE 			*m_file;
	SMA_HEADER 		m_smahdr;	
	SMA_SHE			*m_scnhdrs;

	FILE			*m_strtab_file;
	unsigned int	m_currScn;
};


class SMAFileReader {
public:
	SMAFileReader();
	~SMAFileReader();

	int openSMAFile(const char *fullpath);

	SMA_HEADER* getFileHeader();

	unsigned int getSectionNum();
	SMA_SHE* getSectionHeaders();	
	int getSectionHeader(unsigned int sectiontype, SMA_SHE *pshe);
	
	int setReadSection(unsigned int sectype);
	int getNextSecRecord(void *pRecord, unsigned int size, 
					char *recStr, unsigned int strsize);

	void closeSMAFile();

private:
	FILE 			*m_file;
	SMA_HEADER 		m_smahdr;	
	SMA_SHE			*m_scnhdrs;
	unsigned int	m_currScn;

};
#endif
