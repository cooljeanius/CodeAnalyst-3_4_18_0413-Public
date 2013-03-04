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


#include <stdlib.h>
#include <stdio.h>

#include "jncfile.h"

CJNCFile::CJNCFile()
{
	memset(&m_elfhdr64, 0x00, sizeof(Elf64_Ehdr));
	memset(&m_elfhdr32, 0x00, sizeof(Elf32_Ehdr));
	memset(m_jncFileName, 0x00, MAXLENGTH);
	memset(m_jncClassName, 0x00, MAXLENGTH);
	memset(m_jncFuncName, 0x00, MAXLENGTH);
	
	m_pSrcInfo = NULL;
	m_SrcEntryCnt = 0;
	m_pFileStream = NULL;
	m_jitLoadAddr = 0;
}

CJNCFile::~CJNCFile()
{
	if (m_pFileStream) {
		close();
		m_pFileStream = NULL;
	}

	// don't delete it, it's allocated in caller side.
	m_pSrcInfo = NULL;
}

void CJNCFile::initElfHeader32()
{
	m_elfhdr32.e_ident[EI_NIDENT];	/* Magic number and other info */
	m_elfhdr32.e_ident[EI_MAG0]= ELFMAG0;	// magic byte 0,
	m_elfhdr32.e_ident[EI_MAG1]= ELFMAG1;	// magic byte 1, E
	m_elfhdr32.e_ident[EI_MAG2]= ELFMAG2;	// magic byte 2, L
	m_elfhdr32.e_ident[EI_MAG3]= ELFMAG3;	// magic byte 3, F
	m_elfhdr32.e_ident[EI_CLASS]= ELFCLASS32;	// 64-bit object
	m_elfhdr32.e_ident[EI_DATA]= ELFDATA2LSB;	// little endian
	m_elfhdr32.e_ident[EI_VERSION]= EV_CURRENT;
	m_elfhdr32.e_ident[EI_PAD]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 1]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 2]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 3]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 4]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 5]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 6]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 7]= 0x0;
	m_elfhdr32.e_ident[EI_PAD + 8]= 0x0;

	m_elfhdr32.e_type = ET_DYN;		/* Object file type */
	m_elfhdr32.e_machine = EM_386;		/* Architecture */
	m_elfhdr32.e_version = EV_CURRENT;	/* Object file version */
	m_elfhdr32.e_entry = 0x00;		/* Entry point virtual address */
	m_elfhdr32.e_phoff = sizeof(Elf32_Ehdr);/* Program header table file offset */
	
	// I will update section header table file offset later
	m_elfhdr32.e_shoff = 0x00;		/* Section header table file offset */

	m_elfhdr32.e_flags = 0x0;		/* Processor-specific flags */
	
	/* ELF header size in bytes */
	m_elfhdr32.e_ehsize = sizeof(Elf32_Ehdr);
	
	/* Program header table entry size */
	m_elfhdr32.e_phentsize = sizeof(Elf32_Phdr);	

	// I only use 1 program header
	m_elfhdr32.e_phnum = 1;		/* Program header table entry count */
	
	/* Section header table entry size */
	m_elfhdr32.e_shentsize = sizeof(Elf32_Shdr);	
	
	if (m_SrcEntryCnt) {
		m_elfhdr32.e_shnum = CAJNC_NUM_SECTION_WITHDATA;	
		/* Section header table entry count */
	
		m_elfhdr32.e_shstrndx = CAJNC_NUM_SECTION_WITHDATA - 1 ;	
		/* Section header string table index */
	}
	else {
		m_elfhdr32.e_shnum = CAJNC_NUM_SECTION_NODATA;	
		/* Section header table entry count */
	
		m_elfhdr32.e_shstrndx = CAJNC_NUM_SECTION_NODATA - 1 ;	
		/* Section header string table index */
	}
}


void CJNCFile::initElfHeader64()
{
	m_elfhdr64.e_ident[EI_MAG0]= ELFMAG0;	// magic byte 0,
	m_elfhdr64.e_ident[EI_MAG1]= ELFMAG1;	// magic byte 1, E
	m_elfhdr64.e_ident[EI_MAG2]= ELFMAG2;	// magic byte 2, L
	m_elfhdr64.e_ident[EI_MAG3]= ELFMAG3;	// magic byte 3, F
	m_elfhdr64.e_ident[EI_CLASS]= ELFCLASS64;	// 64-bit object
	m_elfhdr64.e_ident[EI_DATA]= ELFDATA2LSB;	// little endian
	m_elfhdr64.e_ident[EI_VERSION]= EV_CURRENT;
	m_elfhdr64.e_ident[EI_PAD]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 1]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 2]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 3]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 4]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 5]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 6]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 7]= 0x0;
	m_elfhdr64.e_ident[EI_PAD + 8]= 0x0;
	
	m_elfhdr64.e_type = ET_DYN;		/* Object file type */
	m_elfhdr64.e_machine = EM_X86_64;	/* Architecture */
	m_elfhdr64.e_version = EV_CURRENT;	/* Object file version */
	m_elfhdr64.e_entry = 0x0;		/* Entry point virtual address */
	
	/* Program header table file offset */
	m_elfhdr64.e_phoff = sizeof(Elf64_Ehdr);

	// I will reset this later.
	m_elfhdr64.e_shoff = 0x0;	/* Section header table file offset */
	
	m_elfhdr64.e_flags = 0x0;	/* Processor-specific flags */
	m_elfhdr64.e_ehsize = sizeof(Elf64_Ehdr);/* ELF header size in bytes */

	// Program header table entry size 
	m_elfhdr64.e_phentsize = sizeof(Elf64_Phdr);

	// I only use 1 program header
	m_elfhdr64.e_phnum = 1;		/* Program header table entry count */
	m_elfhdr64.e_shentsize = sizeof(Elf64_Shdr);	
	/* Section header table entry size */

	if (m_SrcEntryCnt) {
		// 4 header entries: non-defined, .text, .data, and .strtab
		m_elfhdr64.e_shnum = CAJNC_NUM_SECTION_WITHDATA;		
		/* Section header table entry count */
	
		m_elfhdr64.e_shstrndx = CAJNC_NUM_SECTION_WITHDATA  - 1;
		/* Section header string table index */
	}
	else {
		// 3 header entries: non-defined, .text, and .strtab
		m_elfhdr64.e_shnum = CAJNC_NUM_SECTION_NODATA;		
		/* Section header table entry count */
	
		m_elfhdr64.e_shstrndx = CAJNC_NUM_SECTION_NODATA  - 1;		
		/* Section header string table index */
	}
}

void CJNCFile::close()
{
	if (m_pFileStream) {
		fclose(m_pFileStream);
		m_pFileStream = NULL;
	}
}

void CJNCFile::setJNCFileName(char *pFileName )
{
	unsigned int len = strlen(pFileName);
	
	if (len > MAXLENGTH) {
		len = MAXLENGTH;
		strncpy(m_jncFileName, pFileName, len);
	} 
	else {
		strcpy(m_jncFileName, pFileName);
	}
}

void CJNCFile::setJITFuncName(char *pClassName, char *pFuncName, char *pSourceFile)
{
	unsigned int len = strlen(pClassName);
	if (len > MAXLENGTH) {
		len = MAXLENGTH;
		strncpy(m_jncClassName, pClassName, len);
	}
	else {
		strcpy(m_jncClassName, pClassName);
	}

	len = strlen(pFuncName);
	if (len > MAXLENGTH) {
		len = MAXLENGTH;
		strncpy(m_jncFuncName, pFuncName, len);
	}
	else {
		strcpy(m_jncFuncName, pFuncName);
	}

	if (pSourceFile) {
		len = strlen(pSourceFile);
		if (len > MAXLENGTH) {
			len = MAXLENGTH - 1;
			strncpy(m_javaSrcName, pSourceFile, len);
		}
		else {
			strcpy(m_javaSrcName, pSourceFile);
		}
	}
	else {
		strcpy(m_javaSrcName, "UnknownJITSource");
	}

}

bool CJNCFile::writeJITNativeCode(unsigned char *pJITNativeCode, unsigned int size,
				JavaSrcLineInfo *pSrcInfo, unsigned int srcEntryCnt)

{
	m_pFileStream = fopen(m_jncFileName, "w+b");

	if (!m_pFileStream)
		return false;
	
	m_SrcEntryCnt = srcEntryCnt;
	m_pSrcInfo = pSrcInfo;

#ifdef __x86_64__
	writeElf64(pJITNativeCode, size);
#else
	writeElf32(pJITNativeCode, size);
#endif
	close();

}

unsigned long CJNCFile::getDataSecSize()
{
	unsigned long tSize = 0;
	if (m_SrcEntryCnt)
	{
		// lengthes and strings for src file, class, function
		tSize += sizeof(unsigned int) + strlen(m_javaSrcName);
		tSize += sizeof(unsigned int) + strlen(m_jncClassName);
		tSize += sizeof(unsigned int) + strlen(m_jncFuncName);

		// load address, 64-bit regardless OS;
		tSize += sizeof(unsigned long long);

		// offset and line number pairs
		tSize += sizeof(unsigned int) + 
				sizeof(JavaSrcLineInfo) *m_SrcEntryCnt;
		// alignment
		tSize = (tSize/sizeof(unsigned long) + 1) * sizeof(unsigned long);
	}

	return tSize;
}

void CJNCFile::writeDataSection()
{
	unsigned int strLength = strlen(m_javaSrcName);
	fwrite(&strLength, sizeof(unsigned int), 1, m_pFileStream);
	fwrite(m_javaSrcName, sizeof(unsigned char), strLength, m_pFileStream);
	strLength = strlen(m_jncClassName);
	fwrite(&strLength, sizeof(unsigned int), 1, m_pFileStream);
	fwrite(m_jncClassName, sizeof(unsigned char), strLength, m_pFileStream);
	strLength = strlen(m_jncFuncName);
	fwrite(&strLength, sizeof(unsigned int), 1, m_pFileStream);
	fwrite(m_jncFuncName, sizeof(unsigned char), strLength, m_pFileStream);

	fwrite(&m_jitLoadAddr, sizeof(unsigned long long), 1, m_pFileStream);

	fwrite(&m_SrcEntryCnt, sizeof(unsigned int), 1, m_pFileStream);
	fwrite(m_pSrcInfo, sizeof(JavaSrcLineInfo) * m_SrcEntryCnt, 
			1, m_pFileStream);

	// write padding;
	unsigned long writtenBytes = ftell(m_pFileStream);
	unsigned int padBytes = (unsigned int) (sizeof(unsigned long) -
				(writtenBytes % sizeof(unsigned long)));
	
	unsigned char padChar = 0x00;
	for (unsigned int i = 0; i < padBytes; i++)
		fwrite(&padChar, 1, 1, m_pFileStream);
	
}
void CJNCFile::writeElf32(unsigned char *pJITNativeCode, unsigned int size)
{
	unsigned long textSecOffset = 0;
	unsigned long sectionHdrOffset = 0;
	unsigned long dataSecOffset = 0;
	unsigned long strTblOffset = 0;
	unsigned long segmentSize;

	// calculate .text section size;
	unsigned long codeSectionSize = (size / (unsigned long) + 1) * 
				sizeof(unsigned long);
	unsigned long dataSectionSize = getDataSecSize();

	unsigned long numSections = CAJNC_NUM_SECTION_NODATA;
	if (dataSectionSize)
		numSections = CAJNC_NUM_SECTION_WITHDATA;

	unsigned int textPadLength = sizeof(unsigned long) - 
			size % sizeof(unsigned long);

	initElfHeader32();

	// write a 32-bit ELF file header
	fwrite(&m_elfhdr32, sizeof(Elf32_Ehdr), 1, m_pFileStream);
	
	Elf32_Phdr elf_phdr;
	elf_phdr.p_type = PT_LOAD;	/* Segment type */

	// read and executable	
	elf_phdr.p_flags = PF_X | PF_R;	/* Segment flags */

	// offset, set to 0x0 now
	elf_phdr.p_offset = 0x0;	/* Segment file offset */
	elf_phdr.p_vaddr = 0x0;		/* Segment virtual address */
	elf_phdr.p_paddr = 0x0;		/* Segment physical address */

	segmentSize = codeSectionSize + dataSectionSize +  
		sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)
		+ numSections * sizeof(Elf32_Shdr) + 24;
	elf_phdr.p_filesz = segmentSize;/* Segment size in file */
	elf_phdr.p_memsz = segmentSize;	/* Segment size in memory */
	elf_phdr.p_align = 0x1000;	/* Segment alignment */

	fwrite(&elf_phdr, sizeof(Elf32_Phdr), 1, m_pFileStream);
		
	unsigned long padding = 0x0;

	// size of ELF file header = 0x34;
	// size of Program header = 0x20; need do 11 bytes padding.
	for (int k = 0; k < 11; k++)
		fwrite(&padding, sizeof(unsigned long), 1, m_pFileStream);
	
	textSecOffset = ftell(m_pFileStream);
	// write .text section.
	fwrite(pJITNativeCode, size, 1, m_pFileStream);
	// write a padding after .text section
	if (textPadLength) {
		// pad no op instructions
		unsigned char byte_NoOp = 0x90;
		for (unsigned int cnt = 0; cnt < textPadLength; cnt++)
			fwrite(&byte_NoOp, 1, 1, m_pFileStream);
	}
	
	dataSecOffset = ftell(m_pFileStream);
	// write data section if necessary
	if (m_SrcEntryCnt)
		writeDataSection();
		
	// this is the offset of string table section.
	strTblOffset = ftell(m_pFileStream);

	unsigned char strTableString[24];
	memset(strTableString, 0x00, 24);
	strTableString[0] = 0x0;	//terminate
	strTableString[1] = 0x2E;	// '.'
	strTableString[2] = 0x73;	// 's'
	strTableString[3] = 0x68;	// 'h'
	strTableString[4] = 0x73;	// 's'
	strTableString[5] = 0x74;	// 't'
	strTableString[6] = 0x72;	// 'r'
	strTableString[7] = 0x74;	// 't'
	strTableString[8] = 0x61;	// 'a'
	strTableString[9] = 0x62;	// 'b'
	strTableString[10] = 0x0;	//terminate
	strTableString[11] = 0x2E;	// '.'	<- index of .text section string
	strTableString[12] = 0x74;	// 't'
	strTableString[13] = 0x65;	// 'e'
	strTableString[14] = 0x78;	// 'x'
	strTableString[15] = 0x74;	// 't'
	if (dataSectionSize) {
		strTableString[16] = 0x00;// terminate
		strTableString[17] = 0x2E;// '.' <- index of .data section string
		strTableString[18] = 0x64;// 'd'
		strTableString[19] = 0x61;// 'a'
		strTableString[20] = 0x74;// 't'
		strTableString[21] = 0x61;// 'a'
	}
	fwrite(strTableString, 24, 1, m_pFileStream);

	// this is the offset of section headers	
	sectionHdrOffset = ftell(m_pFileStream);

	Elf32_Shdr elf_shdr;
	memset(&elf_shdr, 0x00, sizeof(Elf32_Shdr));
		
	// undefined section header.
	fwrite(&elf_shdr, sizeof(Elf32_Shdr), 1, m_pFileStream);

	// Write header for .text section.
	elf_shdr.sh_name = 0x0b;/* Section name (string tbl index) */
	elf_shdr.sh_type = SHT_PROGBITS;	/* Section type */
	/* Section flags */
	elf_shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;	
	/* Section virtual addr at execution */
	elf_shdr.sh_addr = textSecOffset;	
	/* Section file offset */
	elf_shdr.sh_offset = textSecOffset;
	/* Section size in bytes */
	elf_shdr.sh_size = dataSecOffset - textSecOffset;
	elf_shdr.sh_link = 0x0;	/* Link to another section */
	elf_shdr.sh_info = 0x0;	/* Additional section information */
	elf_shdr.sh_addralign = 0x10;	/* Section alignment */
	elf_shdr.sh_entsize = 0;	/* Entry size if section holds table */
	fwrite(&elf_shdr, sizeof(Elf32_Shdr), 1, m_pFileStream);

	if (dataSectionSize) {
		// Write header for .data section.
		elf_shdr.sh_name = 0x11;/* Section name (string tbl index) */
		elf_shdr.sh_type = SHT_PROGBITS;	/* Section type */
		/* Section flags */
		elf_shdr.sh_flags = SHF_ALLOC | SHF_OS_NONCONFORMING;	
		/* Section virtual addr at execution *objdump -d junk.jnc/
		elf_shdr.sh_addr = dataSecOffset;	
		/* Section file offset */
		elf_shdr.sh_offset = dataSecOffset;
		/* Section size in bytes */
		elf_shdr.sh_size = strTblOffset - dataSecOffset;
		elf_shdr.sh_link = 0x0;	/* Link to another section */
		elf_shdr.sh_info = 0x0;	/* Additional section information */
		elf_shdr.sh_addralign = 0x10;	/* Section alignment */
		elf_shdr.sh_entsize = 0;/* Entry size if section holds table */
		fwrite(&elf_shdr, sizeof(Elf32_Shdr), 1, m_pFileStream);
	}

	// Write header for sting table section.
	elf_shdr.sh_name = 0x1;	/* Section name (string tbl index) */
	elf_shdr.sh_type = SHT_STRTAB;	/* Section type */
	/* Section flags */
	elf_shdr.sh_flags = 0x0;	
	/* Section virtual addr at execution */
	elf_shdr.sh_addr = 0x0;	
	/* Section file offset */
	elf_shdr.sh_offset = strTblOffset;
	/* Section size in bytes */
	if (dataSectionSize)
		elf_shdr.sh_size = 0x16;
	else
		elf_shdr.sh_size = 0x10;
	
	elf_shdr.sh_link = 0x0;	/* Link to another section */
	elf_shdr.sh_info = 0x0;	/* Additional section information */
	elf_shdr.sh_addralign = 0x01;	/* Section alignment */
	elf_shdr.sh_entsize = 0;	/* Entry size if section holds table */
	fwrite(&elf_shdr, sizeof(Elf32_Shdr), 1, m_pFileStream);

	// OK, now I'm going to update section header offset in 
	// elf file header.
	// write a ELF file header
	m_elfhdr32.e_shoff = sectionHdrOffset;
	rewind(m_pFileStream);
	fwrite(&m_elfhdr32, sizeof(Elf32_Ehdr), 1, m_pFileStream);
}

void CJNCFile::writeElf64(unsigned char *pJITNativeCode, unsigned int size)
{
	unsigned long textSecOffset = 0;
	unsigned long dataSecOffset = 0;
	unsigned long sectionHdrOffset = 0;
	unsigned long strTblOffset = 0;
	unsigned long segmentSize;

	// calculate .text section size
	unsigned long codeSectionSize = (size / sizeof(unsigned long) + 1) *
				sizeof(unsigned long);

	// calculate .data section size
	unsigned long dataSectionSize = getDataSecSize();

	unsigned long numSections = CAJNC_NUM_SECTION_NODATA;
	if (dataSectionSize)
		numSections = CAJNC_NUM_SECTION_WITHDATA;

	unsigned int textPadLength = sizeof(unsigned long) - 
			size % sizeof(unsigned long);

	initElfHeader64();

	// 64-bit ELF format 
	
	// write a ELF file header
	fwrite(&m_elfhdr64, sizeof(Elf64_Ehdr), 1, m_pFileStream);
		
	Elf64_Phdr elf_phdr;
	elf_phdr.p_type = PT_LOAD;	/* Segment type */

	// read and executable	
	elf_phdr.p_flags = PF_X | PF_R;	/* Segment flags */

	// offset, set to 0x0 now
	elf_phdr.p_offset = 0x0;	/* Segment file offset */
	elf_phdr.p_vaddr = 0x0;		/* Segment virtual address */
	elf_phdr.p_paddr = 0x0;		/* Segment physical address */

	segmentSize = codeSectionSize + dataSectionSize +  
		sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr)
		+ numSections * sizeof(Elf64_Shdr) + 24;
	elf_phdr.p_filesz = segmentSize;/* Segment size in file */
	elf_phdr.p_memsz = segmentSize;	/* Segment size in memory */
	elf_phdr.p_align = 0x100000;	/* Segment alignment */

	fwrite(&elf_phdr, sizeof(Elf64_Phdr), 1, m_pFileStream);
		
	unsigned long padding = 0;
	// size of ELF file header = 0x40;
	// size of Program header = 0x38; need do 8 bytes padding.
	fwrite(&padding, sizeof(unsigned long), 1, m_pFileStream);
	
	// this is the offset of .text section
	textSecOffset = ftell(m_pFileStream);
	// write .text section.
	fwrite(pJITNativeCode, size, 1, m_pFileStream);
	// write a padding after .text section
	if (textPadLength) {
		// pad no op instructions
		unsigned char byte_NoOp = 0x90;
		for (unsigned int cnt = 0; cnt < textPadLength; cnt++);
			fwrite(&byte_NoOp, 1, 1, m_pFileStream);
	}
	
	dataSecOffset = ftell(m_pFileStream);
	// write data section
	if (m_SrcEntryCnt)
		writeDataSection();

	// this is the offset of string table section.
	strTblOffset = ftell(m_pFileStream);

	unsigned char strTableString[24];
	memset(strTableString, 0x00, 24);
	strTableString[0] = 0x0;	//terminate
	strTableString[1] = 0x2E;	// '.'
	strTableString[2] = 0x73;	// 's'
	strTableString[3] = 0x68;	// 'h'
	strTableString[4] = 0x73;	// 's'
	strTableString[5] = 0x74;	// 't'
	strTableString[6] = 0x72;	// 'r'
	strTableString[7] = 0x74;	// 't'
	strTableString[8] = 0x61;	// 'a'
	strTableString[9] = 0x62;	// 'b'
	strTableString[10] = 0x0;	//terminate
	strTableString[11] = 0x2E;	// '.'	<- index of .text section string
	strTableString[12] = 0x74;	// 't'
	strTableString[13] = 0x65;	// 'e'
	strTableString[14] = 0x78;	// 'x'
	strTableString[15] = 0x74;	// 't'
	if (dataSectionSize) {
		strTableString[16] = 0x00;// terminate
		strTableString[17] = 0x2E;// '.' <- index of .data section string
		strTableString[18] = 0x64;// 'd'
		strTableString[19] = 0x61;// 'a'
		strTableString[20] = 0x74;// 't'
		strTableString[21] = 0x61;// 'a'
	}
	fwrite(strTableString, 24, 1, m_pFileStream);
	
	// this is the offset of section headers	
	sectionHdrOffset = ftell(m_pFileStream);

	Elf64_Shdr elf_shdr;
	memset(&elf_shdr, 0x00, sizeof(Elf64_Shdr));
		
	// undefined section header.
	fwrite(&elf_shdr, sizeof(Elf64_Shdr), 1, m_pFileStream);

	// write a ELF file header
	// Write header for .text section.
	elf_shdr.sh_name = 0x0b;	/* Section name (string tbl index) */
	elf_shdr.sh_type = SHT_PROGBITS;	/* Section type */
	/* Section flags */
	elf_shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;	
	/* Section virtual addr at execution */
	elf_shdr.sh_addr = textSecOffset;	
	/* Section file offset */
	elf_shdr.sh_offset = textSecOffset;
	/* Section size in bytes */
	elf_shdr.sh_size = dataSecOffset - textSecOffset;
	elf_shdr.sh_link = 0x0;	/* Link to another section */
	elf_shdr.sh_info = 0x0;	/* Additional section information */
	elf_shdr.sh_addralign = 0x10;	/* Section alignment */
	elf_shdr.sh_entsize = 0;	/* Entry size if section holds table */
	fwrite(&elf_shdr, sizeof(Elf64_Shdr), 1, m_pFileStream);

	// write header for .data section if necessary 
	if (dataSectionSize) {
		// Write header for .data section.
		elf_shdr.sh_name = 0x11;/* Section name (string tbl index) */
		elf_shdr.sh_type = SHT_PROGBITS;	/* Section type */
		/* Section flags */
		elf_shdr.sh_flags = SHF_ALLOC | SHF_OS_NONCONFORMING;	
		/* Section virtual addr at execution *objdump -d junk.jnc/
		elf_shdr.sh_addr = dataSecOffset;	
		/* Section file offset */
		elf_shdr.sh_offset = dataSecOffset;
		/* Section size in bytes */
		elf_shdr.sh_size = strTblOffset - dataSecOffset;
		elf_shdr.sh_link = 0x0;	/* Link to another section */
		elf_shdr.sh_info = 0x0;	/* Additional section information */
		elf_shdr.sh_addralign = 0x10;	/* Section alignment */
		elf_shdr.sh_entsize = 0;/* Entry size if section holds table */
		fwrite(&elf_shdr, sizeof(Elf64_Shdr), 1, m_pFileStream);
	}

	// Write header for sting table section.
	elf_shdr.sh_name = 0x1;	/* Section name (string tbl index) */
	elf_shdr.sh_type = SHT_STRTAB;	/* Section type */
	/* Section flags */
	elf_shdr.sh_flags = 0x0;	
	/* Section virtual addr at execution */
	elf_shdr.sh_addr = 0x0;	
	/* Section file offset */
	elf_shdr.sh_offset = strTblOffset;
	/* Section size in bytes */
	if (dataSectionSize)
		elf_shdr.sh_size = 0x16;
	else
		elf_shdr.sh_size = 0x10;

	elf_shdr.sh_link = 0x0;	/* Link to another section */
	elf_shdr.sh_info = 0x0;	/* Additional section information */
	elf_shdr.sh_addralign = 0x01;	/* Section alignment */
	elf_shdr.sh_entsize = 0;	/* Entry size if section holds table */
	fwrite(&elf_shdr, sizeof(Elf64_Shdr), 1, m_pFileStream);

	// OK, now I'm going to update section header offset in 
	// elf file header.
	// write a ELF file header
	m_elfhdr64.e_shoff = sectionHdrOffset;
	rewind(m_pFileStream);
	fwrite(&m_elfhdr64, sizeof(Elf64_Ehdr), 1, m_pFileStream);
}


/*
// Unit test
int main ()
{
	CJNCFile tjnc;

	tjnc.setJNCFileName("junk.jnc");
	tjnc.setJITFuncName("testClass", "testFunc", "javasource file");
	tjnc.setJITStartAddr(0x12345678);
	// write a ELF file header
	
	unsigned char testText[100];
	testText[0]=0x48;
	testText[1]=0x83;
	testText[2]=0xec;
	testText[3]=0x08;
	testText[4]=0x48;
	testText[5]=0x8b;
	testText[6]=0x05;
	testText[7]=0x85;
	testText[8]=0x03;
	testText[9]=0x10;
	testText[10]=0x00;
	testText[11]=0x48;
	testText[12]=0x85;
	testText[13]=0xc0;
	testText[14]=0x74;
	testText[15]=0x02;
	testText[16]=0xff;
	testText[17]=0xd0;
	testText[18]=0x48;
	testText[19]=0x83;
	testText[20]=0xc4;
	testText[21]=0x08;
	testText[22]=0xc3;
	
	JavaSrcLineInfo tLineInfo[20];
	for (int k = 0; k < 20; k++)
	{
		tLineInfo[k].addrOffset = k;
		tLineInfo[k].lineNum = k+100;
	}

	tjnc.writeJITNativeCode(testText, 23, tLineInfo, 20);
	tjnc.close();
	return 0;
}
*/
