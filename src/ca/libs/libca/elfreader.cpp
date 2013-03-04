//$Id: elfreader.cpp,v 1.2 2006/05/15 22:09:45 jyeh Exp $

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


#include "elfreader.h"


CELFReader::CELFReader()
{
    m_shoff  = 0;
    m_dataSecOffset = 0;
    m_dataSecSize = 0;
    
   
    memset(&m_elfFileName, 0, MAXLENGTH);
    memset(&m_elfhdr, 0, sizeof(m_elfhdr));

    m_pFileStream = NULL;
    m_pshstrtab = NULL;
}


CELFReader::~CELFReader()
{
    close();

    if (NULL != m_pshstrtab) {
        delete m_pshstrtab;
        m_pshstrtab = NULL;
    }
}

void CELFReader::close()
{
    if (m_pFileStream) {
        fclose(m_pFileStream);
        m_pFileStream = NULL;
    }
}

bool CELFReader::setELFFileName(const char * pFileName)
{
    unsigned int len = strlen(pFileName);

    if (len > MAXLENGTH) {
        len = MAXLENGTH;
        strncpy(m_elfFileName, pFileName, len);
    }
    else {
        strcpy(m_elfFileName, pFileName);
    }

	m_pFileStream = fopen(m_elfFileName, "r+b");

	if (!m_pFileStream)
		return false;
	
	return true;
}


bool CELFReader::readELFHeader()
{
    bool ret = false;

    if (0 != fread(&m_elfhdr, sizeof(m_elfhdr), 1, m_pFileStream)) {
        ret = false;
        m_shoff = m_elfhdr.e_shoff; 
        m_shstrndx = m_elfhdr.e_shstrndx;
        m_eshentsize = m_elfhdr.e_shentsize;
        m_eshnum = m_elfhdr.e_shnum;
    }

    return ret;
}


void CELFReader::printELF32Header()
{
#ifdef _DEBUG_
    printf("EI_MAG: %c%c%c%c\n", 
        m_elfhdr.e_ident[EI_MAG0],
        m_elfhdr.e_ident[EI_MAG1],
        m_elfhdr.e_ident[EI_MAG2],
        m_elfhdr.e_ident[EI_MAG3]);

   printf("e_shoff: %d\n", m_shoff); 

   printf("e_shstrndx: %d\n", m_shstrndx);
#endif
}


bool CELFReader::readStrSectionHeader()
{
#ifdef __x86_64__
    return readStrSectionHeader64();
#else
    return readStrSectionHeader32();
#endif        
}


bool CELFReader::readStrSectionHeader32()
{
    Elf32_Shdr  elf_shdr;
    unsigned int strTableSize = 0;

    // Calculate the section header entry of .shstrtab
    // Read the section header entry o .shstrtab 
    m_strHeaderOffset = m_shoff + m_shstrndx * m_eshentsize;  
    fseek(m_pFileStream, m_strHeaderOffset, SEEK_SET); 
    if (0 == fread(&elf_shdr, sizeof(Elf32_Shdr), 1, m_pFileStream)) 
        return false;                 

    // Move the file position to beginning of the section header
    // of the string table.
    fseek(m_pFileStream, elf_shdr.sh_offset, SEEK_SET);
    strTableSize = elf_shdr.sh_size;
    m_pshstrtab = new char[strTableSize];
    if (NULL == m_pshstrtab)
        return false;

    if (0 == fread(m_pshstrtab, strTableSize, 1, m_pFileStream))
        return false;

	return true;
}


bool CELFReader::readStrSectionHeader64()
{
    Elf64_Shdr  elf_shdr;
    unsigned int strTableSize = 0;

    // Calculate the section header entry of .shstrtab
    // Read the section header entry o .shstrtab 
    m_strHeaderOffset = m_shoff + m_shstrndx * m_eshentsize;  
    fseek(m_pFileStream, m_strHeaderOffset, SEEK_SET); 
    if (0 == fread(&elf_shdr, sizeof(Elf32_Shdr), 1, m_pFileStream)) 
        return false;                 

    // Move the file position to beginning of the section header
    // of the string table.
    fseek(m_pFileStream, elf_shdr.sh_offset, SEEK_SET);
    strTableSize = elf_shdr.sh_size;
    m_pshstrtab = new char[strTableSize];
    if (NULL == m_pshstrtab)
        return false;

    if (0 == fread(m_pshstrtab, strTableSize, 1, m_pFileStream))
        return false;

	return true;
}

bool CELFReader::processSectionHeaders()
{
#ifdef __x86_64__
    return processSectionHeaders64();
#else
    return processSectionHeaders32();
#endif
}


bool CELFReader::processSectionHeaders32()
{
    Elf32_Shdr  elf_shdr;
    bool ret = true;

    // Move the file position of the beginning of the
    // section header offset
    fseek(m_pFileStream, m_shoff, SEEK_SET); 
    for (unsigned int i=0; i<m_eshnum; i++) {
        if (0 == fread(&elf_shdr, sizeof(elf_shdr), 1, m_pFileStream)) {
            ret = false;
            break;
        }

        if (0 == strncmp(m_pshstrtab + elf_shdr.sh_name, ".data", 4)) { 
            m_dataSecOffset = elf_shdr.sh_offset;
            m_dataSecSize = elf_shdr.sh_size;
        }
    }

    return ret;
}


bool CELFReader::processSectionHeaders64()
{
    Elf64_Shdr  elf_shdr;
    bool ret = true;

    // Move the file position of the beginning of the
    // section header offset
    fseek(m_pFileStream, m_shoff, SEEK_SET); 
    for (unsigned int i=0; i<m_eshnum; i++) {
        if (0 == fread(&elf_shdr, sizeof(elf_shdr), 1, m_pFileStream)) {
            ret = false;
            break;
        }
        if (0 == strncmp(m_pshstrtab + elf_shdr.sh_name, ".data", 4)) { 
            m_dataSecOffset = elf_shdr.sh_offset;
            m_dataSecSize = elf_shdr.sh_size;
        }
    }

    return ret;
}

unsigned int CELFReader::getDataSecOffset()
{
    return m_dataSecOffset;
}

unsigned int CELFReader::getDataSecSize()
{
    return m_dataSecSize;
}


/*
int main()
{
    CELFReader elfreader;

    elfreader.setELFFileName("jncfile.o");
    elfreader.readELFHeader();
   
    elfreader.printELF32Header();
    
    elfreader.readStrSectionHeader();
    
    elfreader.processSectionHeaders();
}
*/
