//$Id: symbolengine.h 12106 2007-05-02 23:16:40Z jyeh $
// interface for the SymbolEngine class.

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


#ifndef _SYMBOL_ENGINE_H
#define _SYMBOL_ENGINE_H

#include <qstring.h>
#include <Q3CString>
#include <Q3ValueList>
#include <bfd.h>
#include <q3ptrlist.h>
#include <qmap.h>
#include <qbuffer.h>
#include <q3process.h>
#include <q3textstream.h>
#include <qmutex.h>
#include <QList>
#include <qprogressdialog.h>
#include <map>
#include <vector>
#include <string>
#include "inlineinstance.h"
#include "jncreader_api.h"

class CDwarfEngine;
class CJNCReader;

enum EnumLabelType
{
    SE_INVALID_LABEL=0,
    SE_SECTION_START,
    SE_SECTION_END,
    SE_USER_DEFINED,
    SE_SAMPLE_ADDRESS,
    SE_SYMBOL
};

struct line_info
{
	unsigned long line;
	bfd_vma address;
	Q3CString disassembly;
	Q3CString file;
	Q3CString codebytes;

	bool operator < (const line_info & right) const
	{ 
		if (address < right.address)
			return true;
		else
			return false;
	};

};


struct sym_info
{
    QString name;
    bfd_vma sym_start;	
    bfd_vma possible_end;
    QString file_name;
    long line;
    unsigned int possible_end_line;
    QString inlInst_file;
    long inlInst_line;
    unsigned int inlInst_id;
    sym_info() {
		clear();
    };

    void clear() {
		name = "";
		sym_start = 0;
		possible_end = 0;
		file_name = "";
		line = 0;
		inlInst_file = "";
		inlInst_line = 0;
		possible_end_line = -1;
		inlInst_id = 0;
    };
};

struct label_info
{
    QString name;
    bfd_vma address;
    EnumLabelType type;
};

//will be used in map, where start is the key
struct section_info
{
    bfd_vma end;
    bool is_code;  
};

typedef struct _symbol_vma_info
{
	bfd_vma max_size;
	QString name;
} symbol_vma_info;

typedef std::map<bfd_vma, label_info > label_map_type;
typedef std::map<bfd_vma, section_info, std::greater <bfd_vma> > section_map_type;
typedef QList<line_info> line_list_type;
typedef QMap<QString, line_list_type> file_map_type;


//small helper class to search for symbol offsets much more quickly
class SymbolBST : public QMutex {
public:
	SymbolBST();

	~SymbolBST();

	std::map<bfd_vma, symbol_vma_info> bst_map;
};

//small helper class for symbol engine that launches objdump and reads the
//output of objdump into a string one line at a time
class ObjDump : QObject
{
Q_OBJECT

 public:
    ObjDump();
    ~ObjDump();

    //specify the file to look at with objdump
    int getInfo (const char * file_name, bfd_vma start_addr, 
		  bfd_vma stop_addr);

    //read the output, after get_info has been called.
    int readOneLine (QString * line);
    void cancel();
private slots:
    void readFromStdOut ();

private:
    bool m_ready; 
    QStringList m_lines;
    Q3Process m_process;
};


typedef std::vector<sym_info*> SYM_INFO_VECTOR; 

extern void delete_SYM_INFO_VECTOR(SYM_INFO_VECTOR &vec);


//class that will get all neccessary symbolic info for a given object file
class SymbolEngine
{
 public:
    enum {
	OKAY,
	FAILED,
	NOT_OPEN,
	NO_MEMORY,
	NO_SYMBOLS,
	INVALID_ARG,
	NO_SYMBOL_TABLE
    };

    //all executable instructions without source file and line info are in the 
    //map with this filename
    static const char UNKNOWN_FILE_NAME[];
    static const char UNKNOWN_FUNCTION_NAME [];

    SymbolEngine();
    ~SymbolEngine();

    //opens the target object file
    int open (const char * file_name, bool inlineFlag = false);
    void closeEngine();
    bool isOpen();

    //is the target 64-bits (currently only works for elf64 files)
    int is64bit (bool * is_64_bit_flag);
    int getImageBaseAddr (bfd_vma * base_address);
    int getTextSectionStartAddr(bfd_vma * start_address);
    int getImageSize (bfd_vma * size);

	//return information of a section
    int getSecSize (bfd_size_type * size, const char * name);
    int getSecBase (bfd_vma * base, const char * name);

    //given an address, you get symbol(function) name, offset into the function,
    // and the stop address
    bool getOffset (bfd_vma vma_address, QString * name, bfd_vma * offset, 
		    bfd_vma *max = NULL);

    int getSymbolForAddr (bfd_vma vma_address, 
				sym_info * sym, 
				QString &str);

    int getSymbolForAddr (bfd_vma vma_address, 
				sym_info * sym, 
				InlineInst * inline_sym=NULL);

    int getSourceFileCount (long * source_file_count);
    int getSourceFileNames (QStringList * source_list);
   
// TODO: This is no longer used and should be removed 
//    //for the given filename, gives the line info
//    int getLineInfo (const char * filename, line_list_type * line_list, 
//			bfd_vma hotspot);
   
    // for a given address, find the source file name and the line number;		
    int getLineInfoForAddr(bfd_vma vma_address,
			std::string *fileName,
			unsigned int *lineNum);
    
    //requires that the start address is a valid instruction, gives codebytes 
    //and disassembly for all instructions in the range
    int disassembleRange (bfd_vma start, bfd_vma end, 
			line_list_type * disassembly_list,
			QWidget * pParent = NULL,
			QProgressDialog * pProgressDlg = NULL);

    int getAllSymbols (label_map_type * label_map);
    int getAllSections (section_map_type * section_map);
    QString getLastError();
    bool demangle(const char * mangled, QString& demangled);

    int getSrcDasmMapForAddrRange(line_list_type * line_list, 
			bfd_vma startAddr, 
			bfd_vma stopAddr,
			QString filterFileName,
			unsigned int fromLine,
			unsigned int toLine);

/***************
 * DWARF stuff
 ***************/
public:
    int getNumInlineDwarf(bfd_vma startAddress); 

    int getInlinesForFunctionDwarf(bfd_vma startAddress,
			InlinelInstMap &inlines);

    int getInlinesForFunctionJava(JavaInlineMap **inlines);
	
    bool isInlineInstance(bfd_vma vma_address);	

private:
    // Use DWARF engine to get symbol information
    int getSymbolForAddrDwarf(bfd_vma vma_address, 
			sym_info * sym, 
			InlineInst *inline_sym = NULL);

    bool getDwarfSymbol(bfd_vma vma_address, QString * name, 
				bfd_vma * start, bfd_vma *max = NULL);

/***************
 * BFD stuff
 ***************/
    // Use BFD engine to get line info for a given address 
    int getLineInfoForAddrBFD(bfd_vma vma_address, 
					std::string *filename, unsigned int *linenum);

    bool getBfdSymbol(bfd_vma vma_address, QString * name, 
				bfd_vma * start, bfd_vma *max = NULL);

    //used by bfd_map_over_sections in get_symbol_for_addr
    static void findAddrInSection (bfd * abfd, asection * section, PTR data);

// TODO: This is no longer used and should be removed 
//    bool readObjfile (bfd_vma hotspot);

private:

// TODO: This is no longer used and should be removed 
//    //analyzes the output of the objdump and builds an address / file & line 
//    //number map
//    bool readLineInfo (ObjDump * dump, 
//			bfd_vma lower, 
//			bfd_vma upper);

    int readAndFilterObjdumpOutput (ObjDump * dump, 
				bfd_vma lower, 
				bfd_vma upper,
				line_list_type * currentLineList,
				QString filterFileName,
				unsigned int fromLine,
				unsigned int toLine);

    //helper function for read_line_info
    void parseInstLine (QString line, line_info * info);
    void fillBst ();

    inline bool isLineSourceInfo (QString line);
    inline bool isLineDataInfo (QString line);
    bool parseObjdumpSourceInfo(QString & line, unsigned int * lineNumber, 
					QString & sourceFile);

    bool parseObjdumpDataInfo(QString & line, line_info * info);
    bool containObjdumpKeywords(QString & line);

    int getSymbolTable();
    int getDynSymbolTable();

    //helper function for read_line_info
    line_list_type * listForFile (QString file_name);

    QString m_last_error;
    bfd * m_target_bfd;
    QString m_target_file_name;

    asymbol ** m_symbol_table;
    long m_symbol_count;
    file_map_type m_file_map;

    //bst for quickly finding symbol offsets
    SymbolBST m_bst;

	CJNCReader *m_jnc_eng;
	int m_has_jnc;

	CDwarfEngine * m_dwf_eng;
	int m_has_dwf;
	bfd_vma m_imageBase;
};

#endif //_SYMBOL_ENGINE_H
