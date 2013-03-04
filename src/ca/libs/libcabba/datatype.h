
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

#ifndef _DATATYPE_H
#define _DATATYPE_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_LENGTH	2048

typedef enum _ERRORCODE
{
	BBA_OK = 0,
	FAILED,
	
	// bbanalysis
	NOT_ELF,
	CANNOT_OPEN,
	NO_MEMORY,
	DASM_ERROR,
	UNKNOWN_BLOCK,
	NOT_IN_SAME_SECTION,

	// smafile write
	EMPTY_TARGET_NAME,
	LONG_TARGETNAME,
	OVER_SECTION_LIMIT,
	INVALID_SECTION_TYPE,
	SECTION_TYPE_MISMATCH,
	DATA_SIZE_MISMATCH,

	// smafile reader
	FORMAT_ERROR
	//
} ERRORCODE;

typedef enum _INSTRTYPE{
	INSTR_UNKNOWN,
	INSTR_OTHER_TYPE,       // the type we don't care
	BR_COND_JUMP, 
	BR_RELATIVE_JUMP,
	BR_ABS_JUMP,
	BR_ABS_INDIRECT_JUMP,   // don't care jump near or far
	BR_RELATIVE_CALL,
	BR_ABS_CALL,
	BR_ABS_INDIRECT_CALL,
	BR_SYSCALL,     
	BR_SYSRET,
	// BR_ENTER, INT, IRET/IRETD
	BR_RET
} INSTRTYPE;
	
struct BR_INSTR_INFO {
	unsigned int instr_offset;
	unsigned int instr_length;
	INSTRTYPE	 instr_type;
	unsigned int destination_offset; 	
	// 0 indicate destination unknown - indirect
};

typedef enum {
	MA_NA       = 0x00,
	MA_READ     = 0x01,     // memory acces  - read;
	MA_WRITE    = 0x02,     // MEMORY ACCESS - write;
	MA_READWRITE= 0x03      // MEMORY ACCESS - read write
}MEMORYTYPE;

struct MEMACC_INST {
	unsigned int 	instr_offset;
	MEMORYTYPE		instr_memacc_type;
};

typedef struct _BASIC_BLOCK_INFO
{
	_BASIC_BLOCK_INFO() {
		bb_start_offset = 0;
		bb_size = 0;
		bb_end_instr_length = 0;	
		bb_end_instr_type = INSTR_UNKNOWN;
//		bb_load_inst_count = 0;
//		bb_store_inst_count = 0;
	};


	unsigned int bb_start_offset;	// start offset of a basic block	
	//unsigned int bb_end_offset;		// this is end instr start offset
	unsigned int bb_size;
	
	unsigned int bb_end_instr_length;	// 0 indicate unknown (no branch instr)

//	unsigned int bb_load_inst_count;
//	unsigned int bb_store_inst_count;

	INSTRTYPE	 bb_end_instr_type;
} BASIC_BLOCK_INFO; 

#endif
