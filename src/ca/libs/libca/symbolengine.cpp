//$Id: symbolengine.cpp 12546 2007-05-07 18:04:43Z jyeh $
// implementation for the SymbolEngine class.

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

// use elf to get imagebase
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <bfd.h>
#include <elf.h>
#include <libelf.h>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <values.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <qapplication.h>
#include <qcursor.h>

#ifdef HAVE_LIBELF_GELF_H
#include <libelf/gelf.h>
#else
#include <gelf.h>
#endif

#include "config.h"
#ifdef HAVE_DEMANGLE_H
extern "C"
{
#include <demangle.h>
};
#else

#ifndef bfd_get_section_size_before_reloc
#define bfd_get_section_size_before_reloc(section) \
((section)->_raw_size)
#endif

#ifndef bfd_get_section_size_after_reloc
#define bfd_get_section_size_after_reloc(section) \
((section)->reloc_done ? (section)->_cooked_size \
: (abort (), (bfd_size_type) 1))
#endif

// from libiberty
/*@{\name demangle option parameter */
#ifndef DMGL_PARAMS
# define DMGL_PARAMS     (1 << 0)        /**< Include function args */
#endif
#ifndef DMGL_ANSI
# define DMGL_ANSI       (1 << 1)        /**< Include const, volatile, etc */
#endif
/*@}*/

#include <ansidecl.h>
extern "C"
{
	extern char *
		cplus_demangle PARAMS ((const char * mangled, int options));
};
#endif

#if OP_VERSION_BASE > 0x00903
#ifndef DISABLE_OPROFILE_LIB
extern std::string const demangle_java_symbol(std::string const & name);
#endif
#endif

#include "../../include/stdafx.h"
#include "jncreader_api.h"
#include "symbolengine.h"


#ifdef ENABLE_DWARF
#include "dwarfengine.h"
#endif //ENABLE_DWARF

using namespace std;

const char SymbolEngine::UNKNOWN_FILE_NAME[] = "Unknown file";
const char SymbolEngine::UNKNOWN_FUNCTION_NAME [] = "(no_symbols)";

//**************Structs

//this structure is used by find_addr_in_section to pass information about the
//target address and whether it has been found, by the call of
//bfd_map_over_sections

struct find_symbol_info
{
	const char *function_name;
	const char *file_name;
	unsigned int line;
	bool found;
	asymbol ** syms;
	bfd_vma addr;
};


//**************Class ObjDump

ObjDump::ObjDump()
{
	m_ready = false;
}


ObjDump::~ObjDump()
{
	QString tmp;

	if (m_process.isRunning())
		m_process.tryTerminate();
	//Clear out the buffer
	if (m_ready)
		m_lines.clear();
}


//launch objdump with
//-d = disassemble
//-l = line numbers
//-z = disassemble zeros
//-w = format output for more than 80 columns
//--show-raw-insn = show codebytes
//--demangle = auto
//--section=.text = show only executable code
//--start-address = disassemble >= this address
//--stop-address = disassemble <= this address
int ObjDump::getInfo (const char * file_name, bfd_vma start_addr,
bfd_vma stop_addr)
{
	char temp_64[LONG_STR_MAX];
	QString address;

	//Sanitary Check
	if (start_addr >= stop_addr)
	{
		return -1;
	}

	connect( &m_process, SIGNAL(readyReadStdout()),
		this, SLOT(readFromStdOut()) );

	m_process.addArgument ("objdump");
	m_process.addArgument ( "-dlwz");
	m_process.addArgument ("--demangle=auto");
	m_process.addArgument ("--show-raw-insn");

	snprintf (temp_64, (LONG_STR_MAX-1), LONG_FORMAT, 
					(long unsigned int)start_addr);

	address.sprintf ("--start-address=%s", temp_64);
	m_process.addArgument (address);

	//this should only happen for java modules...
	if (0 != stop_addr) {
		snprintf (temp_64, (LONG_STR_MAX-1), LONG_FORMAT, 
						(long unsigned int)stop_addr);

		address.sprintf ("--stop-address=%s", temp_64);
		m_process.addArgument (address);
	}

	m_process.addArgument (file_name);

	//fprintf(stderr,"DEBUG: objdump arguments = %s\n", m_process.arguments().join(" ").toAscii().data());

	m_process.closeStdin();
	m_process.setCommunication (Q3Process::Stdout | Q3Process::Stderr
		| Q3Process::DupStderr);
	m_ready = m_process.start();
	qApp->processEvents ();
	if (!m_ready) {
		//error handling
		return -1;
	}
	return 0;
}	// ObjDump::getInfo


int ObjDump::readOneLine (QString * line)
{
	*line = QString::null;

	if (! m_ready)
		return -1;

	while ((m_lines.empty()) && (m_process.isRunning())) {
		qApp->processEvents ();
	}

	if (m_lines.empty ())
		return 0;
	*line = m_lines.front().stripWhiteSpace();
	m_lines.pop_front();

	return line->length() + 1;
}	//ObjDump::readOneLine


void ObjDump::readFromStdOut()
{
	while (m_process.canReadLineStdout())
		m_lines.push_back ( m_process.readLineStdout());
}


void ObjDump::cancel()
{
	if (m_process.isRunning()) {
		m_process.tryTerminate();
		QTimer::singleShot( 2000, &m_process, SLOT( kill() ) );
	}
}


//**************Class SymbolEngine

SymbolEngine::SymbolEngine()
{
	m_last_error = "";
	m_target_bfd = NULL;
	m_symbol_table = NULL;
	m_jnc_eng  = NULL;
	m_dwf_eng  = NULL;
	m_has_jnc = 0;
	m_has_dwf = 0;
	m_symbol_count = 0;
	bfd_init();
	m_imageBase = 0;
}


SymbolEngine::~SymbolEngine()
{
	if (NULL != m_target_bfd)
		closeEngine();
}

bool SymbolEngine::demangle(const char * mangled, QString& demangled)
{
	bool ret = false;

	if(!mangled)
		return ret;

	char* ctmp = cplus_demangle(mangled, DMGL_PARAMS|DMGL_ANSI);
	if(ctmp != NULL) {
		demangled = QString(ctmp);
		ret       = true;
	}
#if OP_VERSION_BASE > 0x00903
#ifndef DISABLE_OPROFILE_LIB
	else{
		string stmp;
		stmp = demangle_java_symbol(mangled);
		if(!stmp.empty())
		{
			demangled = QString::fromStdString(stmp);
			ret       = true;
		}
	}
#endif
#endif

	// If failed to demangle, assign mangled name
	if(ret == false)
	{
		demangled = QString(mangled);
	}
	return ret;
}

// helper function to get image base;
static bfd_vma getImageBase(bfd *m_target_bfd, const char* file_name)
//bfd_vma getImageBase(const char* file_name)
{
	bfd_vma base = -1;

	// get image base;
	int fd;
	Elf *elf;
	GElf_Phdr phdr;


	if (elf_version(EV_CURRENT) == EV_NONE)
	{
		goto FUNCEXIT;
	}

	if ((fd = open(file_name, O_RDONLY, 0)) < 0)
	{
		goto FUNCEXIT;
	}

   	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
	{
		goto FUNCEXIT;
	}

	/*
	if (gelf_getehdr(m_elf, &ehdr) != NULL) {
		m_32bit = (ehdr.e_ident[EI_CLASS] == ELFCLASS64)? false: true;
	}
	*/

	//if (gelf_getphdr(elf, 0, &phdr) == &phdr) {
	if (gelf_getphdr(elf, 0, &phdr) != NULL) {
		base = (bfd_vma) phdr.p_vaddr - phdr.p_offset;
	} else
	{
		asection *p = bfd_get_section_by_name(m_target_bfd, ".text");
		if(NULL != p)
		{
			base = p->vma;
		}
	}

	close (fd);

FUNCEXIT:

	return base;
}

int SymbolEngine::getSymbolTable()
{
	int ret_value = OKAY;
	long symtab_count = 0;

	//get the symbol table for the bfd object
	if (OKAY == ret_value) {
		symtab_count = bfd_get_symtab_upper_bound (m_target_bfd);
		if (symtab_count <= 0) {
			m_last_error = "The file has no symbols.";
			ret_value = NO_SYMBOLS;
		}
	}
	if (OKAY == ret_value) {
		m_symbol_table = (asymbol **) malloc (symtab_count);
		if (NULL == m_symbol_table) {
			m_last_error = "There is insufficient memory available.";
			closeEngine();
			ret_value = NO_MEMORY;
		}
	}
	if (OKAY == ret_value) {
		m_symbol_count = bfd_canonicalize_symtab (m_target_bfd, 
							m_symbol_table);

		if (m_symbol_count <= 0) {
			m_last_error = "Could not canonicalize the symbol table.";
			ret_value = NO_SYMBOLS;
		}
	} 

	return ret_value;
}


int SymbolEngine::getDynSymbolTable()
{
	int ret_value = OKAY;
	long symtab_count = 0;

	//get the symbol table for the bfd object
	if (OKAY == ret_value) {
		symtab_count = bfd_get_dynamic_symtab_upper_bound (m_target_bfd);
		if (symtab_count <= 0) {
			m_last_error = "The file has no dynamic symbols.";
			ret_value = NO_SYMBOLS;
		}
	}
	if (OKAY == ret_value) {
		m_symbol_table = (asymbol **) malloc (symtab_count);
		if (NULL == m_symbol_table) {
			m_last_error = "There is insufficient memory available.";
			closeEngine();
			ret_value = NO_MEMORY;
		}
	}
	if (OKAY == ret_value) {
		m_symbol_count = bfd_canonicalize_dynamic_symtab (m_target_bfd, 
							m_symbol_table);
		if (m_symbol_count <= 0) {
			m_last_error = "Could not canonicalize the dynamic symbol table.";
			ret_value = NO_SYMBOLS;
		}
	}

	return ret_value;
}


//note that if the return value is NO_SYMBOLS, it was opened.
int SymbolEngine::open (const char * file_name, bool inlineFlag)
{
	int ret_value = OKAY;
	long symtab_count = 0;

	//open the target as a default bfd object
	m_target_bfd = bfd_openr (file_name, NULL);
	if (NULL == m_target_bfd) {
		m_last_error = "Could not open file: ";
		m_last_error += bfd_errmsg (bfd_get_error());
		ret_value = NOT_OPEN;

	}
	else if (! bfd_check_format (m_target_bfd, bfd_object)) {
		//if the file isn't actually an executable module
		m_last_error = "Format didn't pass the check: ";
		m_last_error += bfd_errmsg (bfd_get_error());
		closeEngine();
		ret_value = FAILED;

	}
	else if (!(bfd_get_file_flags (m_target_bfd) & HAS_SYMS)) {
		m_last_error = "The file has no symbolic information: ";
		m_last_error += bfd_errmsg (bfd_get_error());
		//closeEngine();
		ret_value = NO_SYMBOL_TABLE;
		m_target_file_name = file_name;
	}
	if (OKAY == ret_value) {
		if (OKAY != (ret_value = getSymbolTable())) {
			// Try .dynsym
			 ret_value = getDynSymbolTable();
		} 
	}
		
	fillBst();
	m_imageBase = getImageBase(m_target_bfd, file_name);
	m_target_file_name = file_name;

#ifdef ENABLE_DWARF
	if (m_dwf_eng) {
		delete m_dwf_eng;
		m_dwf_eng = NULL;
		m_has_dwf = 0;
	}
	
	/// Without DWARF info, symbol engine can still work
	if (ret_value != FAILED
	&&  ret_value != NO_MEMORY
	&&  ret_value != NOT_OPEN) 
	{
		if(!m_target_file_name.endsWith(".jo",false))
		{	
			m_dwf_eng = new CDwarfEngine();
			if (NULL != m_dwf_eng) {
				if (0 == m_dwf_eng->readDwarfFile(file_name, inlineFlag))
					m_has_dwf = 1;
			}
			else
				ret_value = FAILED;
		}
	}
#endif // ENABLE_DWARF

	if (m_jnc_eng) {
		delete m_jnc_eng;
		m_jnc_eng = NULL;
		m_has_jnc = 0;
	}
	
	if (ret_value != FAILED
	&&  ret_value != NO_MEMORY
	&&  ret_value != NOT_OPEN) 
	{
		m_jnc_eng = new CJNCReader();
		if (NULL != m_jnc_eng) {
			m_has_jnc = m_jnc_eng->open(file_name);
		}
		else
			ret_value = FAILED;
	}

	return ret_value;
} // SymbolEngine::open


void SymbolEngine::fillBst ()
{
	for (int j=0; j<m_symbol_count; j++) {
		symbol_vma_info vma_info;

		if (m_symbol_table[j]->name[0] == '.') {
			continue;
		}

		bfd_vma temp = m_symbol_table[j]->value
			+ m_symbol_table[j]->section->vma;

		demangle(m_symbol_table[j]->name,vma_info.name);

#if NEWBINUTIL == 0
		vma_info.max_size = 
			m_symbol_table[j]->section->vma + m_symbol_table[j]->section->_raw_size;
#else
		vma_info.max_size = 
			m_symbol_table[j]->section->vma + m_symbol_table[j]->section->size;
#endif
		m_bst.bst_map[temp] = vma_info;
	}
}//SymbolEngine::fillBst


void SymbolEngine::closeEngine()
{
	file_map_type::iterator it;
	if (NULL != m_symbol_table) {
		free (m_symbol_table);
		m_symbol_table = NULL;
	}

	if (NULL != m_target_bfd) {
		bfd_close (m_target_bfd);
		m_target_bfd = NULL;
	}

	for (it = m_file_map.begin(); it != m_file_map.end(); it++)
		it.data().clear();

	m_file_map.clear();
	m_target_file_name ="";

	if(NULL != m_dwf_eng)
	{
		delete m_dwf_eng;
		m_dwf_eng = NULL;
		m_has_dwf = 0;
	}

	if(NULL != m_jnc_eng)
	{
		delete m_jnc_eng;
		m_jnc_eng = NULL;
		m_has_jnc = 0;
	}
}


bool SymbolEngine::isOpen()
{
	return (NULL != m_target_bfd);
}


//check the top target in the bfd list to see what it was built for
//currently only accepts elf64 as a 64-bit object
int SymbolEngine::is64bit (bool * is_64_bit_flag)
{
	static const char str64[] = "i386:x86-64";
	static const int len = strlen (str64);
	const char * target;

	if (! isOpen()) return NOT_OPEN;
	if (NULL == is_64_bit_flag) return INVALID_ARG;

	target = bfd_printable_name (m_target_bfd);

	*is_64_bit_flag = (0 == strncmp (str64, target, len));

	return OKAY;
}


int SymbolEngine::getTextSectionStartAddr(bfd_vma * text_address)
{
	if (! isOpen()) return NOT_OPEN;
	if (NULL == text_address) return INVALID_ARG;

	*text_address = bfd_get_start_address (m_target_bfd);

	return OKAY;
}

int SymbolEngine::getImageBaseAddr(bfd_vma * base_address)
{
	if (! isOpen()) return NOT_OPEN;
	if (NULL == base_address) return INVALID_ARG;

	*base_address = m_imageBase;

	return OKAY;
}


int SymbolEngine::getImageSize (bfd_vma * size)
{
	if (! isOpen()) return NOT_OPEN;
	if (NULL == size) return INVALID_ARG;

	*size = bfd_get_size (m_target_bfd);
	return OKAY;
}


int SymbolEngine::getSecSize (bfd_size_type * size, const char * name)
{
	asection * sect;

	if (! isOpen()) return NOT_OPEN;
	if (NULL == size) return INVALID_ARG;

	*size = 0;

	for (sect = m_target_bfd->sections; NULL != sect; sect = sect->next) {
		const char *p = bfd_get_section_name (m_target_bfd, sect);

		if (0 == strcmp (p, name)) {
			*size =  bfd_section_size (m_target_bfd, sect);
		}
	}
	return OKAY;
}


int SymbolEngine::getSecBase (bfd_vma * base, const char * name)
{
	asection * sect;

	if (! isOpen()) return NOT_OPEN;
	if (NULL == base) return INVALID_ARG;

	*base = 0;

	for (sect = m_target_bfd->sections; NULL != sect; sect = sect->next) {
		const char *p = bfd_get_section_name (m_target_bfd, sect);

		if (0 ==  strcmp (p, name)) {
			*base =  bfd_section_vma (m_target_bfd, sect);
		}
	}
	return OKAY;
}


//This gets called for each section by bfd_map_over_sections
void SymbolEngine::findAddrInSection (bfd * abfd, asection * section,
		PTR data)
{

	bfd_vma vma;
	bfd_size_type size;
	find_symbol_info * current = (find_symbol_info *) data;

	if (current->found) return;

	if (0 == (bfd_get_section_flags (abfd, section) & SEC_ALLOC)) return;

	vma = bfd_get_section_vma (abfd, section);
	if (current->addr < vma) return;

#if NEWBINUTIL == 0
	size = section->_raw_size;
#else
	size = section->size;
#endif
	if (current->addr >= (vma + size)) return;

	current->found = bfd_find_nearest_line (abfd, section, current->syms,
		(current->addr - vma),
		&(current->file_name),
		&(current->function_name),
		&(current->line));

}

bool SymbolEngine::getDwarfSymbol(bfd_vma vma_address, QString * name,
		bfd_vma * start, bfd_vma * max)
{
	bool ret = false;
#ifdef ENABLE_DWARF
	DW_sym_info dw_sym;
	InlineInst dw_inline_sym;

	if (m_dwf_eng  != NULL
	&&  0 == m_dwf_eng->getSymbolForAddr(vma_address, 
		&dw_sym, &dw_inline_sym))
	{

		*name = QString::fromStdString(dw_sym.symName);

		*start = dw_sym.startAddr;

		*max = dw_sym.stopAddr;

		ret = true;
	} 

#endif // ENABLE_DWARF
	return ret;
}


bool SymbolEngine::getBfdSymbol (bfd_vma vma_address, QString * name,
		bfd_vma * start, bfd_vma * max) 
{
	bool ret_value = false;
	bool bFound = false;
	map<bfd_vma, symbol_vma_info>::reverse_iterator rit;
	map<bfd_vma, symbol_vma_info>::reverse_iterator prev;
	map<bfd_vma, symbol_vma_info>::reverse_iterator rend;
	
	if (!isOpen() || m_bst.bst_map.empty()) {
		goto out;
	}

	rit  = m_bst.bst_map.rbegin();
	rend = prev = m_bst.bst_map.rend();
	for (; rit != rend; rit++) {
		if (vma_address >= rit->first) {
			bFound = true;
			break;
		} 

		prev = rit;
	}

	if (!bFound)
		goto unknown;

	ret_value = true;
	*name = rit->second.name;
	*start = rit->first;
	if (NULL != max) {
		if (prev != rend
		&&  prev->first != rit->first) {
			// NOTE: In this case, the possible_end return the end of the section.
			//       Here, we should use the next symbol's start address - 1 
			//       to minimize the range
			*max = prev->first - 1;	
		} else {
			*max = rit->second.max_size;
		}
	}

unknown:	
	if ((!ret_value))
		*name = UNKNOWN_FUNCTION_NAME;
out:
	return ret_value;
}



bool SymbolEngine::getOffset (bfd_vma vma_address, QString * name,
		bfd_vma * offset, bfd_vma * max)
{
	bool ret = false;

	if (NULL == name || NULL == offset || NULL == max)
	{
		fprintf(stderr,"WARNING: SymbolEngine::getOffset : Fail to get offset\n");
		goto out;
	}

	bfd_vma	sym_start;

	if (m_has_dwf)  {
		ret = getDwarfSymbol(vma_address, name, &sym_start, max);
		if (!ret) {
			ret = getBfdSymbol(vma_address, name, &sym_start, max);
		}
	}
	else{
		ret = getBfdSymbol(vma_address, name, &sym_start, max);
	}

	if (ret)
	{
		*offset = vma_address - sym_start;
		demangle((const char*)((name->data())->toAscii()),*name);

	}


out:
	return ret;
}


/**********************************
 * DWARF stuff
 **********************************/
int SymbolEngine::getSymbolForAddrDwarf(bfd_vma vma_address, 
					sym_info * sym, 
					InlineInst *inline_sym)
{
	int ret = FAILED;
#ifdef ENABLE_DWARF
	DW_sym_info dw_sym;

	if ( m_dwf_eng != NULL
	&&   0 == m_dwf_eng->getSymbolForAddr(
			vma_address, &dw_sym, inline_sym))  
	{
		demangle(dw_sym.symName.c_str(),sym->name);

		if(sym->name.isEmpty()) 
			sym->name = QString::fromStdString(dw_sym.symName);
		sym->sym_start    = dw_sym.startAddr;
		sym->possible_end = dw_sym.stopAddr;

		ret = OKAY;
	}
#endif //ENABLE_DWARF

	return ret;
}


int SymbolEngine::getLineInfoForAddr(bfd_vma vma_address,
				string *fileName,
				unsigned int *lineNum)
{
	int ret = FAILED;
#ifdef ENABLE_DWARF
	if( m_dwf_eng != NULL
	&&  0 == m_dwf_eng->getLineInfoForAddr(vma_address,fileName, lineNum, true))
	{
		ret = OKAY;
	} 
#endif //ENABLE_DWARF

	if( (OKAY != ret) && m_jnc_eng != NULL)
	{
		string file;
		int line;
		int inlineDepth;
		if( true == m_jnc_eng->getSrcInfoFromAddress(vma_address, line, file, inlineDepth))
		{
			*fileName = file;
			*lineNum = line;
			ret = OKAY;
		} 
	} 

	if (ret != OKAY) {
		// get it from bfd;
		ret = getLineInfoForAddrBFD(vma_address, fileName, lineNum);
	}
	return ret;
}


int SymbolEngine::getNumInlineDwarf(bfd_vma startAddress)
{
	int ret = 0;
#ifdef ENABLE_DWARF

	if( m_dwf_eng != NULL)
		ret = m_dwf_eng->getNumInline(startAddress);

#endif //ENABLE_DWARF
	return ret;
}

int SymbolEngine::getInlinesForFunctionJava(JavaInlineMap **mlist)
{
	int ret = 0;

	if(m_has_jnc)
	{
		*mlist = m_jnc_eng->getInlineMap();

		if(NULL != *mlist)
			ret = 1;
	}

	return ret;
}


int SymbolEngine::getInlinesForFunctionDwarf(bfd_vma startAddress,
				InlinelInstMap &inlines)
{
	int ret = 0;
#ifdef ENABLE_DWARF

	if( m_dwf_eng != NULL
	&&  0 < (ret = m_dwf_eng->getInlinesForFunction(startAddress,inlines)))
	{
		InlinelInstMap::iterator it = inlines.begin(); 
		InlinelInstMap::iterator it_end = inlines.end(); 
		for(; it != it_end; it++) {
			string filename;
			unsigned int line = 0;

			// This gives the declare file name
			m_dwf_eng->getFileForId(it->second.gOff, &(it->second.declFileName));
			
			// This gives the declare line
			m_dwf_eng->getStartLineForId(it->second.gOff, &(it->second.declLineNo));

			// This gets the call filename
			m_dwf_eng->getLineInfoForInlineInstance(it->first,
				&(it->second.callFileName),
				&(it->second.callLineNo));
		}
	}

#endif //ENABLE_DWARF

	return ret;

}

bool SymbolEngine::isInlineInstance(bfd_vma vma_address)
{
	bool ret = false;
#ifdef ENABLE_DWARF
	DW_sym_info dw_sym;
	InlineInst dw_inline_sym;
	
	if ( m_dwf_eng != NULL
	&&   0 == m_dwf_eng->getSymbolForAddr(vma_address, &dw_sym, &dw_inline_sym))  
	{
		if (dw_inline_sym.startAddr != 0) 
		{
			ret = true;
		}
	}
#endif //ENABLE_DWARF
	return ret;
	
}

/**********************************
 * BFD stuff
 **********************************/
int SymbolEngine::getLineInfoForAddrBFD(bfd_vma vma_address, 
		std::string *filename, unsigned int *linenum)
{
	bfd_vma base;
	
	int ret_value = OKAY;
	find_symbol_info retrieved;

	if (! isOpen()) return NOT_OPEN;
	if (!filename || !linenum) return INVALID_ARG;

	getImageBaseAddr (&base);

	retrieved.syms = m_symbol_table;
	retrieved.addr = vma_address;
	retrieved.found = false;

	bfd_map_over_sections (m_target_bfd, findAddrInSection, (PTR) &retrieved);

	if (retrieved.found 
	&& retrieved.file_name != NULL) {
		*filename = std::string(retrieved.file_name);
		*linenum = retrieved.line;
	} else {
		m_last_error = "The address was not found: ";
		*filename = UNKNOWN_FILE_NAME;
		*linenum = 0;
		ret_value = FAILED;
	}

	return ret_value;
}


int SymbolEngine::getSymbolForAddr (
	bfd_vma vma_address, 
	sym_info * sym, 
	QString &fileName)
{
	int ret_value = FAILED;
	int line;
	string file = fileName.toStdString();
	int inlineDepth;

	if(m_jnc_eng->getSrcInfoFromAddressWithFile(vma_address, line, file))
	{
		ret_value = OKAY;
		if(!getBfdSymbol(vma_address, &(sym->name), 
					&(sym->sym_start), &(sym->possible_end))) {
			ret_value = FAILED;
		}

		sym->line = line;
		sym->file_name = fileName;
	}

	if (ret_value != OKAY) {
		if(getBfdSymbol(vma_address, &(sym->name), 
					&(sym->sym_start), &(sym->possible_end))) {
			ret_value = OKAY;
		}
	}
	
	return ret_value;
}  //SymbolEngine::getSymbolForAddr


int SymbolEngine::getSymbolForAddr (
	bfd_vma vma_address, 
	sym_info * sym, 
	InlineInst * inline_sym)
{
	int ret_value = FAILED;

	if (m_has_dwf)  {
		ret_value = getSymbolForAddrDwarf(vma_address, sym, inline_sym);
	}

	if (ret_value != OKAY) {
		if(getBfdSymbol(vma_address, &(sym->name), 
					&(sym->sym_start), &(sym->possible_end))) {
			ret_value = OKAY;
		}
	}

	if ( ret_value == OKAY && m_has_jnc) {
		int line = 0;
		string file;
		int inlineDepth = 0;
		if(!m_jnc_eng->getSrcInfoFromAddress(
			vma_address, 
			line, file, inlineDepth))
		{
			return FAILED;
		}
		sym->line = line;
		sym->file_name = QString::fromStdString(file);

		if (inlineDepth != 0 && inline_sym) {
			// NOTE: Here we handle java inline information.
			//       We only populate the information needed.
			string symName;
			bfd_vma startAddr = 0; 
			bfd_vma endAddr = 0;
			if (m_jnc_eng->getSymbolAndRangeFromAddr(
				vma_address, symName, startAddr, endAddr))
			{
				inline_sym->symName   = symName;
				inline_sym->startAddr = startAddr;
				inline_sym->stopAddr  = endAddr;
			}
		}
	}
	
	return ret_value;
}  //SymbolEngine::getSymbolForAddr


QString SymbolEngine::getLastError()
{
	return m_last_error;
}


int SymbolEngine::getSourceFileCount (long * source_file_count)
{
	if (! isOpen()) return NOT_OPEN;
	if (NULL == source_file_count) return INVALID_ARG;

	*source_file_count = (long) m_file_map.size();
	return OKAY;
}


int SymbolEngine::getSourceFileNames (QStringList * source_list)
{
	file_map_type::const_iterator it;

	if (! isOpen()) return NOT_OPEN;
	if (NULL == source_list) return INVALID_ARG;

	source_list->clear();
	if (0 == m_file_map.size()) return OKAY;

	for (it = m_file_map.begin(); it != m_file_map.end(); it++)
		source_list->append (it.key());

	return OKAY;
}


void SymbolEngine::parseInstLine (QString line, line_info * info)
{
	//save the address
	info->address = bfd_scan_vma (line.toAscii().data(), NULL, 16);

	//go past ":\t"
	int i = line.find (':');
	line = line.mid (i + 1).stripWhiteSpace();

	//save the codebytes
	info->codebytes = (line.section ('\t', 0, 0).stripWhiteSpace()).toAscii().data();

	//save the disassembly, without extra spaces
	QString temp = line.section('\t', 1, 1);

	if (temp.isEmpty())
		info->disassembly = line.toAscii().data();
	else
		info->disassembly = temp.toAscii().data();
}


//helper function for read_line_info
line_list_type * SymbolEngine::listForFile (QString file_name)
{
	file_map_type::const_iterator file_it = m_file_map.find (file_name);
	file_map_type::const_iterator file_end = m_file_map.end();
	if (file_it == file_end) {
		line_list_type line_temp;
		m_file_map[file_name] = line_temp;
	}

	return &(m_file_map[file_name]);
}


//bool SymbolEngine::readObjfile (bfd_vma hotspot)
//{
//	ObjDump dump;
//	bfd_vma base;
//	bfd_vma start = (bfd_vma) -1;
//	bfd_vma stop = 0;
//	sym_info sym, inline_sym;
//
//	getImageBaseAddr (&base);
//
//	if (OKAY != getSymbolForAddr(hotspot, &sym, &inline_sym))
//		return false;
//
//	if (inline_sym.sym_start != 0) {
//		start = inline_sym.sym_start;
//		stop  = inline_sym.possible_end;
//	} else {
//		start = sym.sym_start;
//		stop  = sym.possible_end;
//	}
//
//	qApp->setOverrideCursor( QCursor(Qt::WaitCursor) );
//	if (0 != dump.getInfo (m_target_file_name.data(), start, stop)) {
//		qApp->restoreOverrideCursor();
//		qApp->processEvents();
//		return false;
//	}
//
//	//readlineinfo restores the cursor
//	return readLineInfo (&dump, start, stop);
//} //SymbolEngine::readObjfile


int SymbolEngine::getSrcDasmMapForAddrRange(line_list_type * line_list, 
				bfd_vma startAddr, 
				bfd_vma stopAddr,
				QString filterFileName,
				unsigned int fromLine,
				unsigned int toLine)
{
	int ret = -1;
	ObjDump dump;
	bfd_vma base;
	sym_info sym, inline_sym;

	getImageBaseAddr (&base);

	if (0 != dump.getInfo (m_target_file_name.toAscii().data(), 
		startAddr, stopAddr)) 
	{
		goto out;
	}

	//readlineinfo restores the cursor
	ret = readAndFilterObjdumpOutput(&dump, startAddr, stopAddr, line_list, 
			filterFileName, fromLine, toLine);

out:
	return ret;
} 


bool SymbolEngine::containObjdumpKeywords(QString & line)
{
	return (line.contains("file format") || line.contains("Disassembly of section"));
}


bool SymbolEngine::parseObjdumpSourceInfo(QString & line, 
					unsigned int * lineNumber, 
					QString & sourceFile)
{
	unsigned int tmp = 0;
	bool isOk = false;

	// Source Info line has the following format:
	// For Open64: "cadinar01.(none):/root/classic/classic.cpp:48"
	// For GCC   : "/root/classic/classic.cpp:49 " 
	sourceFile = line.section (':', -2, -2);
	tmp = line.section (':', -1, -1).toUInt(&isOk);
	if(isOk)
		*lineNumber = tmp;

	return isOk;
}


//helper function for read_line_info
//format of the line: "<addr>:\t<codebyte+>\t<disassembly>"
bool SymbolEngine::parseObjdumpDataInfo(QString & line, line_info * info)
{
	//save the address
	info->address = bfd_scan_vma (line.toAscii().data(), NULL, 16);

	//go past ":\t"
	int i = line.find (':');
	line = line.mid (i + 1).stripWhiteSpace();

	//save the codebytes
	info->codebytes = (line.section ('\t', 0, 0).stripWhiteSpace()).toAscii().data();

	//save the disassembly, without extra spaces
	QString temp = line.section('\t', 1, 1);

	if (temp.isEmpty())
		info->disassembly = line.toAscii().data();
	else
		info->disassembly = temp.toAscii().data();


	return true;
}


//This function fills out the file map with line information
//The format of the stuff from objdump is 5 lines that can be ignored, then a
//list of functions with two lines of address and function naming, then
//repeated source file and line numbers or instructions
//Returns false if canceled
//bool SymbolEngine::readLineInfo (ObjDump * dump, 
//				bfd_vma lower, 
//				bfd_vma upper)
//{
//	QString sourceFile;
//	unsigned int objdumpLineNumber = MAXINT;
//	QString line;
//	int keep_reading;
//	line_list_type * currentLineList = NULL;
//	line_info currentDsmLineInfo;
//
//	QProgressDialog progress (NULL, NULL, true, Qt::WStyle_Customize
//		| Qt::WStyle_NormalBorder | Qt::WStyle_Title);
//	progress.setLabelText (QString ("Reading source line information"));
//
//	progress.setTotalSteps ( upper - lower);
//
//	keep_reading = dump->readOneLine (&line);
//
//	qApp->restoreOverrideCursor();
//	progress.show();
//	qApp->processEvents();
//
//	//while not eof
//	while (keep_reading > 0) {
//		sym_info sym;
//
//		if (line.isEmpty() || containObjdumpKeywords(line)) {
//			/* Do nothing */
//		} else if (isLineSourceInfo(line) 
//			&& parseObjdumpSourceInfo(line, &objdumpLineNumber, sourceFile)) {
//			/* Parse the line from objdump and get the line number,
//			 * find the correct list using the file name as key.
//			 * 
//			 * However, when a binary is compiled by PGI, objdump does not
//			 * always have the source line info even when DWARF .debug
//			 * sections are present in the ELF file. See the following comment
//			 * to ses how we handles this.
//			 */
//			currentLineList = listForFile(sourceFile);
//		} else if (isLineDataInfo(line) 
//			&& parseObjdumpDataInfo(line, &currentDsmLineInfo)) {
//			
//			/* In the case of PGI compiler, no line info is available through
//			 * objdump. We try to get the source line info using the libdwarf.
//			 * If libdwarf failed, simply skip this line and try the following
//			 * line.
//			 */
//			if (NULL == currentLineList) {
//				if (OKAY != getSymbolForAddrDwarf( currentDsmLineInfo.address, &sym))
//					continue;
//
//				currentLineList = listForFile(sym.file_name);
//			}
//			
//			/* objdumpLineNumber is the line number parsed from the output
//			 * of objdump. We try to use this if it is available. If not, we 
//			 * we use the line number from libdwarf.
//			 */
//			if (objdumpLineNumber == MAXINT) {
//				currentDsmLineInfo.line = sym.line;
//			} else {
//				currentDsmLineInfo.line = objdumpLineNumber;
//			}	
//			currentLineList->append (currentDsmLineInfo);
//		}
//
//		keep_reading = dump->readOneLine(&line);
//
//		if (progress.wasCancelled()) {
//			dump->cancel();
//			return false;
//		}
//
//		continue;
//
//	}//while there is information left
//			
//	return true;
//}//SymbolEngine::read_line_info


////This function gives all the lines for the given file
//int SymbolEngine::getLineInfo (const char * file_name,
//				line_list_type * line_list, 
//				bfd_vma hotspot)
//{
//	fprintf (stderr,"DEBUG: getLineInfo: file %s, hotspot %lx\n", 
//				file_name, hotspot);
//
//	QString temp = file_name;
//	line_list_type::const_iterator line_it;
//	const line_list_type * temp_list = NULL;
//
//	if (! isOpen()) return NOT_OPEN;
//	if ((NULL == line_list) || (temp.isEmpty())) return INVALID_ARG;
//
//	line_list->clear();
//	temp_list = listForFile (temp);
//
//	//NOTE: If the hotspot is not found, use objdump to obtain the info...
//	bool found = false;	
//	line_list_type::const_iterator lit = temp_list->begin();
//	line_list_type::const_iterator lend = temp_list->end();
//	for (; lit != lend ; lit++) {
//		if (hotspot == (*lit).address)
//			found = true;
//	}
//
//	if (!found ) {
//		if (!readObjfile (hotspot))
//			return FAILED;
//	}
//
//	for (line_it = temp_list->begin(); line_it != temp_list->end(); line_it++) {
//		line_list->append (*line_it);
//	}
//
//	return OKAY;
//}


int SymbolEngine::readAndFilterObjdumpOutput (ObjDump * dump, 
				bfd_vma lower, 
				bfd_vma upper,
				line_list_type * currentLineList,
				QString filterFileName,
				unsigned int fromLine,
				unsigned int toLine)
{
	QString sourceFile;
	unsigned int objdumpLineNumber = MAXINT;
	QString line;
	int keep_reading;
	line_info currentDsmLineInfo;
	bool bAddToList = false;


	//while not eof
	for(keep_reading = dump->readOneLine(&line);
	    keep_reading > 0;
	    keep_reading = dump->readOneLine(&line))
	{
		sym_info sym;

		if (line.isEmpty() || containObjdumpKeywords(line)) {
			/* Do nothing */
			continue;
		}

		bool isSource = isLineSourceInfo(line);
		bool isData = isLineDataInfo(line);

		if(m_has_jnc)	// Java does not have source info in objdump
		{	// Set up as if there was a source line AND a data line
			if(!isData)
			{
				continue;	// Should never happen
			}

			isData = isData && parseObjdumpDataInfo(line, &currentDsmLineInfo);

			if(!isData)
			{
				continue;	// Should never happen
			}

			isSource = true;

			// Get the line number for this address
			if(OKAY != getSymbolForAddr(currentDsmLineInfo.address, &sym, filterFileName))
			{
				continue;
			}

			wstring nam2 = sym.file_name.toStdWString();
			sourceFile = sym.file_name;
			objdumpLineNumber = sym.line;
		} else
		{
			if(isSource)
			{
				isSource = isSource && parseObjdumpSourceInfo(line, &objdumpLineNumber, sourceFile);
			}

			if(isData)
			{
				isData = isData && parseObjdumpDataInfo(line, &currentDsmLineInfo);
			}
		}
		
		if (isSource)
		{
			// For Source file + Line number lines, check against file/line filter
			if (sourceFile == filterFileName) {
				if (fromLine == 0 && toLine == 0) {
					// If don't specify the line range, assume this is ok.
					bAddToList = true;
				} else if (fromLine <= objdumpLineNumber 
				     &&  objdumpLineNumber <= toLine) 
				{
					// Followed lines are good
					bAddToList = true;
				}
			} else {
				// Ignore lines followed
				bAddToList = false;
			}
			if(!m_has_jnc)
			{
				continue;
			}
		}
		
		if (isData && bAddToList)
		{

			/* For Dasm + codebyte, store this info */

			/* In the case of PGI compiler, no line info is available through
			 * objdump. We try to get the source line info using the libdwarf.
			 * If libdwarf failed, simply skip this line and try the following
			 * line.
			 */
			if (NULL == currentLineList) {
				if (OKAY != getSymbolForAddrDwarf( currentDsmLineInfo.address, &sym))
					continue;

				currentLineList = listForFile(sym.file_name);
			}
			
			/* objdumpLineNumber is the line number parsed from the output
			 * of objdump. We try to use this if it is available. If not, we 
			 * use the line number from libdwarf.
			 */
			if (objdumpLineNumber == MAXINT) {
				currentDsmLineInfo.line = sym.line;
			} else {
				currentDsmLineInfo.line = objdumpLineNumber;
			}	
			currentLineList->append (currentDsmLineInfo);

			continue;
		}

		continue;

	}//while there is information left

			
	return 0;
}// SymbolEngine::readAndFilterObjdumpOutput


int SymbolEngine::getAllSymbols (label_map_type * label_map)
{
	if (! isOpen()) return NOT_OPEN;
	if (NULL == label_map) return INVALID_ARG;

	label_map->clear();

	label_info temp_label;

	for (long i = 0; i < m_symbol_count; i++) {
		if (m_symbol_table[i] != NULL
		&&  0 != strlen(m_symbol_table[i]->name)) {
			//&& 
			//m_symbol_table[i]->name[0] != '.') {
			//all symbols except the sections
			if (NULL != m_symbol_table[i]->name) {
				demangle(m_symbol_table[i]->name,temp_label.name);
			} else
				temp_label.name = UNKNOWN_FUNCTION_NAME;

			temp_label.address = m_symbol_table[i]->value +
				m_symbol_table[i]->section->vma;

			temp_label.type = SE_SYMBOL;
			label_map->insert (label_map_type::value_type (temp_label.address,
				temp_label));
		}
	}

	return OKAY;
}//SymbolEngine::getAllSymbols


int SymbolEngine::getAllSections (section_map_type * section_map)
{
	asection * sect;

	if (! isOpen()) return NOT_OPEN;
	if (NULL == section_map) return INVALID_ARG;

	section_map->clear();

	for (sect = m_target_bfd->sections; NULL != sect; sect = sect->next) {
		bfd_vma section_start = sect->vma;
		section_info temp_section;

		temp_section.end = section_start + bfd_section_size (m_target_bfd, sect);
		temp_section.is_code = sect->flags & SEC_CODE;
//		fprintf(stderr,"getAllSections: section %s = [%lx, %lx], size = %x\n",
//						sect->name, section_start, temp_section.end,
//						bfd_section_size(m_target_bfd, sect));
		section_map->insert (section_map_type::value_type (section_start, temp_section));
	}

	return OKAY;
}


int SymbolEngine::disassembleRange (
		bfd_vma start, 
		bfd_vma end, 
		line_list_type * disassembly_list, 
		QWidget * pParent,
		QProgressDialog * pProgressDlg)
{
	ObjDump dump;
	QString line;
	int keep_reading;

	//Sanitary Check
	if(start >= end)
	{
		return FAILED;
	}

	if (! isOpen()) return NOT_OPEN;
	if (NULL == disassembly_list) return INVALID_ARG;

	if (0 != dump.getInfo (m_target_file_name.toAscii().data(), start, end)) {
		return NOT_OPEN;
	}

	keep_reading = dump.readOneLine (&line);
	
	if (pProgressDlg) 
	{
		pProgressDlg->setLabelText (
			QString ("Reading disassembly information"));
			pProgressDlg->setMinimum(0);

		if (0!=end)
			pProgressDlg->setMaximum( end - start );
		else
			pProgressDlg->setMaximum(20000);
	}
	qApp->processEvents();
	

	//while not eof
	while ( keep_reading > 0) {
		long cur_line_num = 0;

		//skip empty sections.
		while ((keep_reading > 0) && (line.isEmpty())) {
			keep_reading = dump.readOneLine (&line);
			if (pProgressDlg && pProgressDlg->wasCanceled()) {
				dump.cancel();
				return FAILED;
			}
		}
		while ((keep_reading > 0) && (!isLineDataInfo (line))) {
			keep_reading = dump.readOneLine (&line);
			if (pProgressDlg && pProgressDlg->wasCanceled()) {
				dump.cancel();
				return FAILED;
			}
		}
		
		//for all instructions and file/line lines in this function
		while (isLineDataInfo (line)) {
			//if the line has file & line number info
			if (isLineSourceInfo (line)) {
				cur_line_num = line.section (':', -1, -1).toUInt();
			}
			else if (!line.section (':', 0, 0).endsWith("()")) {
				//ignore function names

				//if the line is an instruction line
				line_info one_line;

				one_line.line = cur_line_num;
				//parseInstLine (line, &one_line);
				parseObjdumpDataInfo(line, &one_line);
				if (pProgressDlg) {
					pProgressDlg->setValue( one_line.address - start);
					qApp->processEvents();
				}
				
				// Sanity check to make sure the line has address	
				if(one_line.address != 0)
					disassembly_list->append (one_line);
			}
			keep_reading = dump.readOneLine (&line);
			if (pProgressDlg && pProgressDlg->wasCanceled()) {
				dump.cancel();
				return FAILED;
			}
		} //while there are lines for the function
		if (pProgressDlg && pProgressDlg->wasCanceled()) {
			dump.cancel();
			return FAILED;
		}
	} //while there is information left
	return OKAY;
} //SymbolEngine::disassembleRange


inline bool SymbolEngine::isLineSourceInfo (QString line)
{
	// Source Info line has the following format:
	// For Open64: "cadinar01.(none):/root/classic/classic.cpp:48"
	// For GCC   : "/root/classic/classic.cpp:49 " 
	return (line.startsWith (QString ("/"))
		|| line.startsWith (QString ("."))
		|| line.contains(QString (":/")));
}


inline bool SymbolEngine::isLineDataInfo (QString line)
{
	// Assuming that data Info line has one or many colons
	//FIXME [Suravee] Future design should not rely on this.
	return (!line.section (':', 1, 1).isEmpty())
		&& (!line.section (':', 0, 0).isEmpty());
}


// Implementation for the SymbolBST class, a binary search tree which will find
//  the closest symbol to the address that has an address lower or equal to
//  the given address, and the offset from the symbol.
SymbolBST::SymbolBST () : QMutex ()
{
}


SymbolBST::~SymbolBST()
{
	bst_map.empty();
}



void delete_SYM_INFO_VECTOR(SYM_INFO_VECTOR &vec)
{
	SYM_INFO_VECTOR::iterator it = vec.begin();
	SYM_INFO_VECTOR::iterator it_end = vec.end();
	
	for (; it != it_end; it++) 
	{
		delete(*it);
		*it = NULL;
	}
	vec.clear();
};	
