// $Id: dwarfengine.cpp 12095 2007-05-02 18:18:09Z jyeh $

/*
// CodeAnalyst for Open Source
// Copyright 2008 Advanced Micro Devices, Inc.
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dwarfengine.h"
#define DWARF_DEBUG		0


// CONSTRUCTOR
CDwarfEngine::CDwarfEngine()
{
	m_inlineFlag = false;
	m_fd = 0;
	m_dbg = NULL;
	m_curDieLowPc = 0;
}

// DESTRUCTOR
CDwarfEngine::~CDwarfEngine()
{
	Cleanup();
}

void CDwarfEngine::Cleanup()
{
	if (m_dbg) {
		dwarf_finish(m_dbg, NULL);	
		m_dbg = NULL;
	}

	if (m_fd) {
		close(m_fd);	
		m_fd = 0;
	}

	m_routineMap.clear();
	m_inlineMap.clear();
	m_cuLineInfoMap.clear();
	m_cuFileNameVecMap.clear();
}

int CDwarfEngine::readDwarfFile( const char* fileName, bool inlineFlag )
{
	if (!fileName) 
		return -1;

	Dwarf_Die 	curDie = NULL;
	Dwarf_Unsigned 	hdr = 0;
	Dwarf_Half 	curTag = 0;


	// Open File
	if( (m_fd = open(fileName, O_RDONLY)) == -1)
	{
		if(DWARF_DEBUG) fprintf(stderr, 
			"ERROR: CDwarfEngine::readDwarfFile : Could not open file %s\n", 
			fileName);
		return -1;

	}


	//---------------------------------------------------------------------
	// Here, we initialize libdwarf
	if(dwarf_init(m_fd, DW_DLC_READ, NULL, NULL, &m_dbg, NULL) != DW_DLV_OK) {
		Cleanup();
		return -1;
	}

	m_inlineFlag = inlineFlag;

	// A loop to recursively get the CU header until finish
	while(dwarf_next_cu_header(m_dbg,NULL,NULL,NULL,NULL,&hdr,NULL) == DW_DLV_OK) 
	{
		// Obtain DIE for current CU.
		if ( dwarf_siblingof( m_dbg, NULL, &curDie, NULL ) != DW_DLV_OK ) {
			continue;	
		}

		bool bContinue= true;
		// Get the tag of current DIE 
		if( dwarf_tag( curDie, &curTag, NULL) != DW_DLV_OK )
		{
			bContinue = false;
		} else 	if(curTag != DW_TAG_compile_unit)
		{
			// this is not in compile unit one.
			bContinue = false;					
		}

		if (bContinue) {
			// Process one die
			process_one_die(curDie);
		}

		// de-allocate the DIE
		dwarf_dealloc(m_dbg, curDie, DW_DLA_DIE);
	}

	return 0;
}

int CDwarfEngine::process_one_die(Dwarf_Die oneDie)
{
	Dwarf_Half 	symTag = 0; 
	Dwarf_Die 	symDIE = 0;
	Dwarf_Die 	childDIE = 0;

	// Get start address of the current die if available.
	dwarf_lowpc( oneDie, &m_curDieLowPc , NULL );

	/* --------------------------------------------------
	 * EXTRACT SYMBOL INFO
	 * --------------------------------------------------*/
	// Note: We are looking for symbol information that belong
	// 	 to function (subprogram) which stays at LEVEL 1.
	if(dwarf_child(oneDie, &symDIE, NULL) != DW_DLV_OK)
		return -1;

	// Recursive across LEVEL 1 siblings....
	do {
		SUBROUTINE subroutine;
		Dwarf_Addr startAddr;
		Dwarf_Addr stopAddr;

		// Get the Dwarf_Tag
		if (dwarf_tag( symDIE, & symTag, NULL) != DW_DLV_OK)
			continue;

		// Make sure we've got the DW_TAG_subprogram (function symbol). 
		if(symTag != DW_TAG_subprogram)
			continue;

		if(get_DW_sym_info( symDIE, &startAddr, &stopAddr,
			&subroutine.symName, &subroutine.gOff) == 0) 
		{
			// Check for inline function	
			if (m_inlineFlag)
			{

				if (dwarf_child(symDIE, &childDIE, NULL) == DW_DLV_OK)
				{
					subroutine.numInline = get_DW_inline_sym_info( 
								childDIE, 
								startAddr);

					if (childDIE) {
						dwarf_dealloc(m_dbg,childDIE, DW_DLA_DIE);
						childDIE = 0;
					}
				}else{
					subroutine.numInline = 0;
				}

			} 
			ADDRESSKEY addrKey;
			addrKey.startAddr = startAddr;	
			addrKey.stopAddr  = stopAddr;	
			m_routineMap.insert(SUBROUTINEMAP::value_type(addrKey, subroutine));
		}
	}while(dwarf_siblingof( m_dbg,symDIE, &symDIE, NULL ) == DW_DLV_OK);

	return 0;
} // CDwarfEngine::process_one_die

int CDwarfEngine::get_DW_inline_low_high_in_child_lexical(Dwarf_Die die,
							Dwarf_Addr *low,
							Dwarf_Addr *high)
{
	bool ret 		= -1;
	Dwarf_Die childDIE 	= 0;
	Dwarf_Die sibDIE 	= 0;
	Dwarf_Half childTag 	= 0;
	Dwarf_Half sibTag 	= 0;
	
	// Looking for child
	if (dwarf_child( die, &childDIE, NULL ) == DW_DLV_OK)
	{
#if DWARF_DEBUG
		//--------------------------------------------
		// Extract the DIE CU offset

		Dwarf_Off dCuOff;
		if( dwarf_die_CU_offset( childDIE, &dCuOff, NULL ) == DW_DLV_OK )
		{
			fprintf(stderr,"DEBUG: \t\t\t\tinline function symbol CU offset = %u\n"
						, dCuOff);
		}
#endif //DWARF_DEBUG

		//--------------------------------------------
		// Get child tag
		if( dwarf_tag( childDIE, &childTag, NULL) == DW_DLV_OK )
			// Check child tag
			if(childTag == DW_TAG_lexical_block)
				// Get low/high
				if(get_DW_symbol_low_high_pc(childDIE, 
							low, 
							high) == 0)
				{
					// Done
					ret = 0;
				}

		//--------------------------------------------
		// If not found, next Looking for sibling
		if( ret != 0 
		&&  dwarf_siblingof(m_dbg, childDIE, &sibDIE, NULL ) == DW_DLV_OK)
		{
#if DWARF_DEBUG
			//--------------------------------------------
			// Extract the DIE CU offset
			Dwarf_Off dCuOff;
			if( dwarf_die_CU_offset( sibDIE, &dCuOff, NULL ) == DW_DLV_OK )
			{
				fprintf(stderr,"DEBUG: \t\t\t\tinline function symbol CU offset = %u\n"
							, dCuOff);
			}
#endif //DWARF_DEBUG

			//--------------------------------------------
			// Get sib tag
			if( dwarf_tag( sibDIE, &sibTag, NULL) == DW_DLV_OK )
				// Check sibling tag
				if(sibTag == DW_TAG_lexical_block)
					// Get low/high
					if(get_DW_symbol_low_high_pc(sibDIE, 
								low, 
								high) == 0)
					{
						// Done
						ret = 0;
					}
		}

		//--------------------------------------------
		// If not found, recurse
		if(ret != 0 && sibDIE != 0)
		{
			ret = get_DW_inline_low_high_in_child_lexical(sibDIE,
							low,
							high);
		}	
		
		//--------------------------------------------
		// Clean up
		if (sibDIE) 
		{
			dwarf_dealloc(m_dbg,sibDIE, DW_DLA_DIE);
			sibDIE = 0;
		}
		if (childDIE) 
		{
			dwarf_dealloc(m_dbg,childDIE, DW_DLA_DIE);
			childDIE = 0;
		}
	}

	return ret;
}

int CDwarfEngine::get_DW_inline_low_high_in_sibling_lexical(Dwarf_Die die,
							Dwarf_Addr *low,
							Dwarf_Addr *high)
{
	bool ret = -1;
	bool rmCurDie = false;
	Dwarf_Die curDIE = die;
	Dwarf_Die sibDIE = 0;
	Dwarf_Half sibTag = 0;
	
	// Looking for sibling no more
	while(dwarf_siblingof(m_dbg, curDIE, &sibDIE, NULL ) == DW_DLV_OK)
	{
		if (curDIE && rmCurDie) 
		{
			dwarf_dealloc(m_dbg,curDIE, DW_DLA_DIE);
			curDIE = 0;
			rmCurDie = false;
		}

		// Get sibling tag
		if( dwarf_tag( sibDIE, &sibTag, NULL) == DW_DLV_OK )
			// Check sibling tag
			if(sibTag == DW_TAG_lexical_block)
				// Get low/high
				if(get_DW_symbol_low_high_pc(sibDIE, 
							low, 
							high) == 0)
				{
					// Done
					ret = 0;
					break;
				}
		
		curDIE = sibDIE;
		rmCurDie = true;	
		continue;
	}

	return ret;
}

int CDwarfEngine::get_DW_inline_low_high_in_entry_pc_and_ranges(
	Dwarf_Die die,
	InlineInst * pIlInst)
{      
	int ret = -1; 
        Dwarf_Attribute dAttr;

	if (!pIlInst)
		return ret;
	
	//---------------------------------	
	// Get low pc
	Dwarf_Addr low = 0;
        if( dwarf_attr(die, DW_AT_entry_pc ,&dAttr, NULL) == DW_DLV_OK)
        {
                if( dwarf_formaddr(dAttr, &low, NULL) != DW_DLV_OK)
                {
			dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
                       	return ret;
                }
	}

	//---------------------------------	
	// Get range offset
	Dwarf_Unsigned offset; 
        if( dwarf_attr(die, DW_AT_ranges,&dAttr, NULL) == DW_DLV_OK)
        {
                if( dwarf_formudata(dAttr, &offset, NULL) != DW_DLV_OK)
                {
			dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
			return ret;
                }
        }

	//----------------------------------	
	// Get widest range
	// TODO [3.2]: This might need redesign.
	//     In case the inline instances are non-contiguous,
	//     there might not be low/high address or entry_pc.
	//     In this case, we derive the low/high from the 
	//     widest range. However, this might not be good enough.
        Dwarf_Ranges * dRangeSet = NULL;
        Dwarf_Signed dRangeCnt = 0;
        Dwarf_Unsigned dByteCnt = 0;
	Dwarf_Unsigned dBegAddr = 0;
	Dwarf_Unsigned dEndAddr = 0;
	Dwarf_Unsigned dWidestRange = 0;
        if( dwarf_get_ranges_a(m_dbg, offset, die, 
			&dRangeSet, 
			&dRangeCnt, 
			&dByteCnt, NULL) == DW_DLV_OK)
        {
		for (int i = 0; i < dRangeCnt; i++) {
			Dwarf_Unsigned delta = dRangeSet[i].dwr_addr2 - dRangeSet[i].dwr_addr1;
			if (dWidestRange < delta) {
				dWidestRange = delta;
				dBegAddr = dRangeSet[i].dwr_addr1;
				dEndAddr = dRangeSet[i].dwr_addr2;
			}

// For DEBUG code range
//			fprintf(stderr, "Range [%llx - %llx] (%u)\n",
//				m_curDieLowPc + dRangeSet[i].dwr_addr1,
//				m_curDieLowPc + dRangeSet[i].dwr_addr2,
//				delta);

		
			ADDRESSKEY key;
			key.startAddr = m_curDieLowPc + dRangeSet[i].dwr_addr1;	
			key.stopAddr  = m_curDieLowPc + dRangeSet[i].dwr_addr2;
			pIlInst->rangeVec.push_back(key);
		}

		dwarf_ranges_dealloc(m_dbg, dRangeSet, dRangeCnt);
		ret = 0;
        }

	if (low)
		pIlInst->startAddr = low;
	else 
		pIlInst->startAddr = m_curDieLowPc + dBegAddr; 
	pIlInst->stopAddr = m_curDieLowPc + dEndAddr; 
		
        return ret;                                                                          
}


/*
* FUNCTION 	: CDwarfEngine::get_DW_inline_sym_info
* DESCRIPTION	: This function extract inline function information within the 
*		: subprogram tag (function)
* INPUT	: childDIE, list
* OUTPUT	: 
*/
int CDwarfEngine::get_DW_inline_sym_info(Dwarf_Die childDIE,
					Dwarf_Addr parentAddr,
					unsigned int nestedIlNo)
{
	Dwarf_Half childTag = 0;
	Dwarf_Addr startAddr = 0;
	Dwarf_Addr stopAddr = 0;


	InlineInst inline_inst;
	int inlineCnt = 0;
	string symstring;

	/* This is the offset for the inline instance */
	Dwarf_Off inst_dOff = 0;

	do {

		if( dwarf_tag( childDIE, & childTag, NULL) != DW_DLV_OK )
			break;

		if(childTag != DW_TAG_inlined_subroutine)
			break;

		//--------------------------------------------
		// Extract the DIE offset
		if( dwarf_dieoffset( childDIE, &inst_dOff, NULL ) != DW_DLV_OK )
		{
			return 0;
		}

		// Extract the name of this symbol
		if( get_DW_symbol_name_attr( childDIE, &symstring) == -1 )
		{
			/***************************************************************
			* NOTE: In some case, the subprogram DIE does not have name, 
			* but instead the DW_AT_abstract_origin, or DW_AT_specification 
			* attribute.
			****************************************************************/
			if (get_DW_sym_from_abstract_origin( childDIE, 
					&symstring, 
					(Dwarf_Off*) &inline_inst.gOff) != 0) 
			{
				get_DW_sym_from_specification( childDIE, 
					&symstring, 
					(Dwarf_Off*) &inline_inst.gOff);
			}
		}else{
			dwarf_dieoffset( childDIE,
				(Dwarf_Off*) &inline_inst.gOff, 
				NULL );
		}
		
		/***************************************************************
		* NOTE: Typical case, the inline instance start/stop address
		* should be in the DW_AT_low_pc and DW_AT_high_pc
		****************************************************************/
		if(get_DW_symbol_low_high_pc(childDIE, 
					&(startAddr), 
					&(stopAddr)) == 0)
		{
			// Do nothing
		}
		/***************************************************************
		* NOTE: Try handling special case (GCC-4.5.x) where start address is in
		* DW_AT_entry_pc and stop address is in the ranges
		****************************************************************/
		else if(get_DW_inline_low_high_in_entry_pc_and_ranges(childDIE,
					&inline_inst) == 0)
		{
			// Do nothing
		}

		/***************************************************************
		* NOTE: Try handling special case (GCC-4.3.x) where start/stop address is in
		* a child (Leven 5) with tag "DW_TAG_lexical_block"
		****************************************************************/
		else if(get_DW_inline_low_high_in_child_lexical(childDIE,
					&(startAddr),
					&(stopAddr)) == 0)
		{
			// Do nothing
		} 
		
		/***************************************************************
		* NOTE: Try handling special case (GCC-4.1.x) where start/stop address is in
		* a sibling (Leven 2) with tag "DW_TAG_lexical_block"
		****************************************************************/
		else if(get_DW_inline_low_high_in_sibling_lexical(childDIE,
					&(startAddr),
					&(stopAddr)) == 0)
		{
			// Do nothing
		} else {
			break;
		}

		if(get_DW_inline_call_line_file(childDIE, 
					&(inline_inst.callFileNo), 
					&(inline_inst.callLineNo)) != 0)
			break;	
	
		if (symstring.empty() 
		&& (startAddr != 0 || stopAddr != 0))
		{
			char buffer[200];
			sprintf(buffer, "InlineFunc [0x%llx - 0x%llx]", 
				startAddr, stopAddr);
			symstring = buffer;
		}

		inline_inst.symName    = symstring;
		inline_inst.parentAddr = parentAddr;
		if (startAddr != 0 && stopAddr != 0) {
			inline_inst.startAddr  = startAddr;
			inline_inst.stopAddr   = stopAddr;
		}

		// Update the parent instance
		inline_inst.nestedIlParent = nestedIlNo;
		if (nestedIlNo != 0 ) {
			InlinelInstMap::iterator it = m_inlineMap.find(nestedIlNo);
			if (it != m_inlineMap.end()) {
				it->second.nestedIlList.push_back(inst_dOff);
			}
		}

		// insert item;
		m_inlineMap.insert(InlinelInstMap::value_type(inst_dOff, inline_inst));
	
		inlineCnt++;

	} while (0);
	
	//---------------------------------------------
	// NOTE: We are going depth-first-search tree traversing

	Dwarf_Die 		grandChildDIE = 0;
	Dwarf_Die 		sibDIE = 0;

	// Check if there are children left
	if (dwarf_child( childDIE, &grandChildDIE, NULL ) == DW_DLV_OK)
	{
		inlineCnt += get_DW_inline_sym_info (grandChildDIE, 
							parentAddr, 
							inst_dOff);
		if (grandChildDIE) {
			dwarf_dealloc(m_dbg,grandChildDIE, DW_DLA_DIE);
			grandChildDIE = 0;
		}
	}

	// Check if there are sibling left
	if (dwarf_siblingof(m_dbg, childDIE, &sibDIE, NULL ) == DW_DLV_OK)
	{
		inlineCnt += get_DW_inline_sym_info (sibDIE, parentAddr, nestedIlNo);
		if (sibDIE) {
			dwarf_dealloc(m_dbg,sibDIE, DW_DLA_DIE);
			sibDIE = 0;
		}
	}

	return inlineCnt;
}// CDwarfEngine::get_DW_inline_sym_info

int CDwarfEngine::get_DW_inline_call_line_file(Dwarf_Die die, 
					unsigned int *callFileNo, 
					unsigned int *callLineNo)
{
	int ret = -1;
	Dwarf_Unsigned line = 0;
	Dwarf_Unsigned file = 0;	
	if(!callFileNo || !callLineNo)
		return ret;

	//--------------------------------------------
	// Extract call line
	if(get_DW_call_line_attr(die, &line) != 0)
	{
		return ret;
	}

	//--------------------------------------------
	// Extract call file no
	if(get_DW_call_file_attr(die, &file) != 0)
	{
		return ret;
	}
	
	
	*callFileNo = file;
	*callLineNo = line;

	ret = 0;
	return ret;		
}

int CDwarfEngine::get_DW_sym_from_specification(Dwarf_Die subDIE, 
				string *symstring,
				Dwarf_Off *gOff)
{
	Dwarf_Attribute dAttr = 0;
	Dwarf_Die spcDIE = 0;	

	int rc = -1;

	do {
		if(find_specification_die(subDIE, &spcDIE) != 0)
			break;
		
		if(DWARF_DEBUG) fprintf(stderr,"DEBUG:get_DW_sym_from_specification\n");

		//------------------------------------------------
		// Get the name of specification DIE
		rc = get_DW_symbol_name_attr( spcDIE, symstring );
		if(gOff != NULL)
		{
			dwarf_dieoffset( spcDIE, gOff, NULL ); 
		}

	} while (0);

	if (spcDIE) {
		dwarf_dealloc(m_dbg,spcDIE, DW_DLA_DIE);
		spcDIE = 0;
	}
	if (dAttr) {
		dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
		dAttr = 0;
	}
	return rc;
} //CDwarfEngine::get_DW_sym_from_specification

int CDwarfEngine::get_DW_sym_from_abstract_origin(Dwarf_Die subDIE, 
					string *symstring,
					Dwarf_Off *gOff)
{

	Dwarf_Attribute dAttr = 0;
	Dwarf_Die aboDIE = 0;	

	int rc = -1;

	do{
		if(find_abstract_origin_die(subDIE, &aboDIE) !=0)
			break;
		
		if(DWARF_DEBUG) fprintf(stderr,"DEBUG:get_DW_sym_from_abstract_origin\n");

		// Get the name of abstract origin DIE
		if( get_DW_symbol_name_attr( aboDIE, symstring) == 0) 
		{
			rc = 0;
			if(gOff != NULL)
			{
				dwarf_dieoffset( aboDIE, gOff, NULL ); 
			}
			break;
		}else{
			get_DW_sym_from_specification( aboDIE, symstring, gOff);
			rc = 0;
		}

	}while(0);

	// Cleaning up 
	if (dAttr) {
		dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
		dAttr = 0;
	}

	if (aboDIE) {
		dwarf_dealloc(m_dbg,aboDIE, DW_DLA_DIE);
		aboDIE = 0;
	}
	return rc;
} //CDwarfEngine::get_DW_sym_from_abstract_origin

int CDwarfEngine::find_abstract_origin_die(Dwarf_Die die, Dwarf_Die *aboDIE)	
{

	Dwarf_Attribute dAttr = 0;
	Dwarf_Off gOff;

	int ret = -1;

	do{
		if( dwarf_attr(die, DW_AT_abstract_origin ,&dAttr, NULL)!= DW_DLV_OK)
			break;

		// Get the global offset
		if( dwarf_global_formref(dAttr, &gOff, NULL) != DW_DLV_OK)
		{
			break;
		}

		// Here we use aboDIE to store the information of the abstract origin DIE	
		if( dwarf_offdie(m_dbg, gOff, aboDIE, NULL) != DW_DLV_OK)
		{
			break;
		}

		ret = 0;	

	}while(0);

	// Cleaning up 
	if (dAttr) {
		dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
		dAttr = 0;
	}

	return ret;
} //CDwarfEngine::find_abstract_origin_die

int CDwarfEngine::find_specification_die(Dwarf_Die subDIE, Dwarf_Die *spcDIE)
{
	Dwarf_Attribute dAttr = 0;
	Dwarf_Off gOff;

	int ret = -1;

	do {
		if( dwarf_attr(subDIE, DW_AT_specification ,&dAttr, NULL) != DW_DLV_OK)
		{
			break;
		}

		//--------------------------------------------
		// Get the global offset
		if( dwarf_global_formref(dAttr, &gOff, NULL) != DW_DLV_OK)
		{
			break;
		}

		//--------------------------------------------
		// Here we use spcDIE to store the information of the specification DIE
		if( dwarf_offdie(m_dbg, gOff, spcDIE, NULL) != DW_DLV_OK)
		{
			break;
		}
		ret = 0;

	} while (0);

	if (dAttr) {
		dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
		dAttr = 0;
	}
	return ret;

} //CDwarfEngine::find_specification_die


/*
* FUNCTION 	: CDwarfEngine::get_DW_sym_info
* DESCRIPTION	: This function recursively finds function symbols within 
*		  the file.
* NOTE		: Currently, we are not reporting the inline function
*		  although we can figure out which one is. 
*/
int CDwarfEngine::get_DW_sym_info(Dwarf_Die subDIE, 
				Dwarf_Addr *startAddr,
				Dwarf_Addr *stopAddr,
				string *symName,
				Dwarf_Off *gOff)
{
	int ret = -1;

#if DWARF_DEBUG
	//--------------------------------------------
	// Extract the DIE CU offset
	Dwarf_Off dCuOff;
	if( dwarf_die_CU_offset( subDIE, &dCuOff, NULL ) != DW_DLV_OK )
	{
		return ret;
	}
	if(DWARF_DEBUG) fprintf(stderr,"DEBUG: \t\t\t\tsymbol CU offset = %u\n", dCuOff);
	//dwarf_dealloc(m_dbg,(void*) );

#endif /* DWARF_DEBUG */

	// Sanity check
	if(!startAddr || !stopAddr || !symName)
	{
		return ret;
	}

	//--------------------------------------------
	// Extract the name of this symbol
	if( get_DW_symbol_name_attr( subDIE, symName) == -1 )
	{
		/*************************************************************************
		* NOTE: In some case, the subprogram DIE does not have name, but instead
		*       the DW_AT_abstract_origin, or DW_AT_specification attribute.
		************************************************************************/
		if (get_DW_sym_from_abstract_origin( subDIE, symName, gOff) != 0) 
		{
			get_DW_sym_from_specification( subDIE, symName, gOff);
		}
	}else{
		dwarf_dieoffset( subDIE, gOff, NULL );
	}


	if((ret = get_DW_symbol_low_high_pc(subDIE, 
			startAddr, 
			stopAddr)) != 0)
		return ret;	

	if (symName->empty() && ret == 0) {
		char buffer[200];
		sprintf(buffer, "Func [0x%lx - 0x%lx]", 
			(unsigned long)*startAddr, (unsigned long)*stopAddr);
		*symName = string(buffer);
	}
	if(DWARF_DEBUG) fprintf(stderr,"DEBUG: name = %s\n",symName->c_str());

	return ret;
} // CDwarfEngine::get_DW_sym_info

int CDwarfEngine::get_DW_symbol_low_high_pc( Dwarf_Die die, 
						Dwarf_Addr *low,
						Dwarf_Addr *high)
{
	int ret = -1;
	Dwarf_Addr _low  = 0 ;
	Dwarf_Addr _high = 0;
	
	if(!low || !high)
		return ret;

	//--------------------------------------------
	// Extract lowpc (startAddress) 
	if( dwarf_lowpc( die, &_low , NULL ) != DW_DLV_OK )
	{
		return ret;
	}

	//--------------------------------------------
	// Extract highpc (stopAddress)
	if( dwarf_highpc( die, &_high , NULL ) != DW_DLV_OK )
	{
		return ret;
	}
	
	ret = 0;
	*low = _low;
	*high = _high;
	return ret;
}

int CDwarfEngine::get_DW_symbol_name_attr( Dwarf_Die die, string* pstr)
{
	char * symName 	= NULL;
	int rc = -1;

	if(get_DW_mangled_name( die, pstr) == 0 )
	{
		rc = 0;
	}else if( dwarf_diename(die, &symName, NULL) == DW_DLV_OK ) {
		*pstr = symName;
		dwarf_dealloc(m_dbg,(void*) symName, DW_DLA_STRING);
		rc = 0;
	}

	return rc;

}

int CDwarfEngine::dumpSymbolMap(FILE* file)
{
	unsigned int i = 0;
	SUBROUTINEMAP::iterator it;
	for (it = m_routineMap.begin(); it != m_routineMap.end(); ++it, i++)
	{
		fprintf(file, "SYMBOL");
		fprintf(file, ": [ %#8llx ", it->first.startAddr);
		fprintf(file, ", %#8llx ) ", it->first.stopAddr);
		fprintf(file, ", numInline = %3d "   , it->second.numInline);
		fprintf(file, ", gOff= %5lu "   , (unsigned long) it->second.gOff);

		fprintf(file, ", %s\n"   , it->second.symName.c_str());
	}

	fprintf(file,"\nSUMMARY: Total number of symbol = %u\n\n",i);

	InlinelInstMap::iterator itl;
	i = 0;
	for (itl = m_inlineMap.begin(); itl != m_inlineMap.end(); ++itl, i++)
	{
		fprintf(file, "INLINE");
		fprintf(file, ": [ %#8llx ", itl->second.startAddr);
		fprintf(file, ", %#8llx ) ", itl->second.stopAddr);
		fprintf(file, ", prentAddr = %#8lx ", (unsigned long) itl->second.parentAddr);
		fprintf(file, ", gOff= %5lu "   , (unsigned long) itl->second.gOff);
		fprintf(file, ", %s\n"   , itl->second.symName.c_str());
	}

	fprintf(file,"\nSUMMARY: Total number of inline functions = %u\n\n",i);

	return 0;	
}

int CDwarfEngine::get_DW_call_file_attr( Dwarf_Die die,
					Dwarf_Unsigned *fileNo)
{      
	int ret = -1; 
        Dwarf_Attribute dAttr;
	
	// Get filename information.
        if( dwarf_attr(die, DW_AT_call_file ,&dAttr, NULL) == DW_DLV_OK )
        {       
                if( dwarf_formudata(dAttr, fileNo, NULL) == DW_DLV_OK )
                {
                       ret = 0;
                }
                dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
        }
	return ret;
}

int CDwarfEngine::get_DW_call_line_attr( Dwarf_Die die,
					Dwarf_Unsigned *lineNo)
{      
	int ret = -1; 
        Dwarf_Attribute dAttr;
	
	// Get line number information.
        if( dwarf_attr(die, DW_AT_call_line ,&dAttr, NULL) == DW_DLV_OK)
        {
                if( dwarf_formudata(dAttr, lineNo, NULL) == DW_DLV_OK)
                {
                       	ret = 0; 
                }
                dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
        }
        return ret;                                                                          
} 

int CDwarfEngine::get_DW_decl_file_attr( Dwarf_Die die,
					Dwarf_Unsigned *fileNo)
{      
	int ret = -1; 
        Dwarf_Attribute dAttr;
	
	// Get filename information.
        if( dwarf_attr(die, DW_AT_decl_file ,&dAttr, NULL) == DW_DLV_OK )
        {       
                if( dwarf_formudata(dAttr, fileNo, NULL) == DW_DLV_OK )
                {
                       ret = 0;
                }
                dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
        }
	return ret;
}

int CDwarfEngine::get_DW_decl_line_attr( Dwarf_Die die,
					Dwarf_Unsigned *lineNo)
{      
	int ret = -1; 
        Dwarf_Attribute dAttr;
	
	// Get line number information.
        if( dwarf_attr(die, DW_AT_decl_line ,&dAttr, NULL) == DW_DLV_OK)
        {
                if( dwarf_formudata(dAttr, lineNo, NULL) == DW_DLV_OK)
                {
                       	ret = 0; 
                }
                dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
        }
        return ret;                                                                          
} 

int CDwarfEngine::get_DW_mangled_name( Dwarf_Die die, string* mangledName)
{      
	int ret = -1; 
        Dwarf_Attribute dAttr;
	char *name;

	// Getting DW_AT_MIPS_linkage_name attribute	
        if( dwarf_attr(die, DW_AT_MIPS_linkage_name ,&dAttr, NULL) == DW_DLV_OK)
        {
                if( dwarf_formstring(dAttr, &name, NULL) == DW_DLV_OK)
                {
			*mangledName = name;
                       	ret = 0;
                }
                dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
        }
        return ret;                                                                          
} 

int CDwarfEngine::get_DW_comp_dir( Dwarf_Die die, string* dir)
{      
	int ret = -1; 
        Dwarf_Attribute dAttr;
	char *name;

	// Getting DW_AT_MIPS_linkage_name attribute	
        if( dwarf_attr(die, DW_AT_comp_dir ,&dAttr, NULL) == DW_DLV_OK)
        {
                if( dwarf_formstring(dAttr, &name, NULL) == DW_DLV_OK)
                {
			*dir= name;
                       	ret = 0;
                }
                dwarf_dealloc(m_dbg,dAttr, DW_DLA_ATTR);
        }
        return ret;                                                                          
} 

int CDwarfEngine::find_level1_die( ADDRESSKEY addrKey, Dwarf_Die *die1 )
{
	int ret = -1;
	Dwarf_Half symTag = 0;
	Dwarf_Addr lowPc = 0;
	Dwarf_Addr highPc = 0; 
		
	// Recursive across LEVEL 1 siblings....
	do {

#if DWARF_DEBUG 
	//--------------------------------------------
	// Extract the DIE CU offset
	Dwarf_Off dCuOff;
	if( dwarf_die_CU_offset( *die1, &dCuOff, NULL ) != DW_DLV_OK )
	{
		return -1;
	}
	if(DWARF_DEBUG) fprintf(stderr,"DEBUG: \t\t\t\tsymbol CU offset = %u\n", dCuOff);
	//dwarf_dealloc(m_dbg,(void*) );

#endif /* DWARF_DEBUG */


		// Get the Dwarf_Tag
		if (dwarf_tag( *die1, & symTag, NULL) != DW_DLV_OK)
			continue;
	
		// Make sure we've got the DW_TAG_subprogram (function symbol). 
		if(symTag != DW_TAG_subprogram)
			continue;
		
		// Try to match lowpc and highpc
		if( get_DW_symbol_low_high_pc(*die1, &lowPc, &highPc) != 0)	
			continue;

		if( lowPc  == addrKey.startAddr
		&&  highPc == addrKey.stopAddr)
		{
			ret = 0;
			break;
		}
	}while(dwarf_siblingof( m_dbg,*die1, die1, NULL ) == DW_DLV_OK);

	return ret;	
}


int CDwarfEngine::find_level2plus_die( ADDRESSKEY inlineKey, Dwarf_Die *die2 )
{
	int ret = -1;
	Dwarf_Half die2Tag = 0;
	Dwarf_Addr lowPc = 0;
	Dwarf_Addr highPc = 0;

	do {


#if DWARF_DEBUG 
	//--------------------------------------------
	// Extract the DIE CU offset
	Dwarf_Off dCuOff;
	if( dwarf_die_CU_offset( *die2, &dCuOff, NULL ) != DW_DLV_OK )
	{
		return -1;
	}
	if(DWARF_DEBUG) fprintf(stderr,"DEBUG: \t\t\t\tdie2 offset = %u\n", dCuOff);
	//dwarf_dealloc(m_dbg,(void*) );

#endif /* DWARF_DEBUG */


		if( dwarf_tag( *die2, & die2Tag, NULL) != DW_DLV_OK )
			break;

		if(die2Tag != DW_TAG_inlined_subroutine)
			break;

		if( get_DW_symbol_low_high_pc(*die2, &lowPc, &highPc) != 0)
			break;	


		if( lowPc  == inlineKey.startAddr
		&&  highPc == inlineKey.stopAddr)
		{
			ret = 0;
			break;
		}

	} while (0);


	if(ret != 0)
	{
		//---------------------------------------------
		// NOTE: We are going depth-first-search tree traversing

		Dwarf_Die grandChildDIE = 0;
		Dwarf_Die sibDIE = 0;

		// Check if there are children left
		if (dwarf_child(*die2, &grandChildDIE, NULL ) == DW_DLV_OK)
		{
			if(find_level2plus_die( inlineKey, &grandChildDIE) == 0)
			{
				*die2 = grandChildDIE;
				ret = 0;
			}
		/*	
			if (grandChildDIE) {
				dwarf_dealloc(m_dbg,grandChildDIE, DW_DLA_DIE);
				grandChildDIE = 0;
			}
		*/
		}

		// Check if there are sibling left
		if (dwarf_siblingof(m_dbg, *die2, &sibDIE, NULL ) == DW_DLV_OK)
		{
			if(find_level2plus_die( inlineKey, &sibDIE) == 0)
			{
				*die2 = sibDIE;
				ret = 0;
			}
		/*	
			if (sibDIE) {
				dwarf_dealloc(m_dbg,sibDIE, DW_DLA_DIE);
				sibDIE = 0;
			}
		*/
		}
	}
	return ret;
}

SUBROUTINEMAP::iterator CDwarfEngine::find_func_from_id(Dwarf_Off gOff)
{
	
	SUBROUTINEMAP::iterator funcIter = m_routineMap.begin();
	for(;funcIter != m_routineMap.end(); funcIter++)
	{
		if(funcIter->second.gOff == gOff) 
		{
			break;
		}
	}
	return funcIter;
}

SUBROUTINEMAP::iterator CDwarfEngine::find_func_from_addr(Dwarf_Addr addr)
{
	SUBROUTINEMAP::iterator funcIter = m_routineMap.begin();
	for(;funcIter != m_routineMap.end(); funcIter++)
	{
		if(funcIter->first.startAddr <= addr 
		&& funcIter->first.stopAddr  > addr)
		{
			break;
		}
	}
	return funcIter;
}

/*******************************************************************************
 * PUBLIC FUNCTION MEMBER
 ******************************************************************************/
int CDwarfEngine::getSymbolForId(Dwarf_Off id, DW_sym_info *symbolInfo)
{
	int ret = -1;

	//Sanity Check
	if (!symbolInfo || !m_dbg )
		return ret;

	SUBROUTINEMAP::iterator funcIter = find_func_from_id(id);
	if(funcIter != m_routineMap.end())
	{
		ret = 0;
		symbolInfo->startAddr = funcIter->first.startAddr;
		symbolInfo->stopAddr  = funcIter->first.stopAddr;
		symbolInfo->symName   = funcIter->second.symName;
		symbolInfo->symId     = funcIter->second.gOff;
	}
	
	return ret;
}

int CDwarfEngine::getInlineForId(Dwarf_Off id, InlineInst * inlineInfo)
{
	int ret = -1;
	
	//Sanity Check
	if ( (m_inlineFlag && !inlineInfo) || !m_dbg )
		return ret;
	
	InlinelInstMap::iterator inlineIter = m_inlineMap.find(id);
	if(inlineIter != m_inlineMap.end())
	{
		ret = 0;
		*inlineInfo = inlineIter->second;
	}

	return ret;
}

int CDwarfEngine::getSymbolForAddr(Dwarf_Addr addr,
				DW_sym_info * symbolInfo,
				InlineInst * inlineInfo)
{
	int ret = -1;
	bool foundInline = false;

	//Sanity Check
	if (!symbolInfo || (m_inlineFlag && !inlineInfo) || !m_dbg )
		return ret;

	SUBROUTINEMAP::iterator funcIter = find_func_from_addr(addr);
	if(funcIter == m_routineMap.end())
		return ret;
	else
		ret = 0;

	symbolInfo->startAddr = funcIter->first.startAddr;
	symbolInfo->stopAddr  = funcIter->first.stopAddr;
	symbolInfo->symName   = funcIter->second.symName;
	symbolInfo->symId     = funcIter->second.gOff;

	// search for inline item
	if (m_inlineFlag 
	&& funcIter->second.numInline != 0) 
	{
		InlinelInstMap::iterator inlineIter = m_inlineMap.begin();
		for(;inlineIter != m_inlineMap.end(); inlineIter++)
		{
			if(inlineIter->second.startAddr <= addr 
			&& inlineIter->second.stopAddr  > addr)
			{
				foundInline = true;	
				break;
			}
		}

		if(foundInline)
		{
			*inlineInfo = inlineIter->second;
		}
	}
	
	return ret;
}


unsigned long int CDwarfEngine::_getCuOff(Dwarf_Off id)
{
	unsigned long int ret = 0;
	Dwarf_Off lOff = 0;
	Dwarf_Off cuOff = 0;
	Dwarf_Off cuHeaderOff = 0;
	Dwarf_Die die;

	// Get die from global offset
	if( dwarf_offdie(m_dbg, id, &die, NULL) != DW_DLV_OK)
	{
		return ret;
	}

	// Get die from local offset
	if( dwarf_die_CU_offset( die, &lOff, NULL ) != DW_DLV_OK )
	{
		return ret;
	}

	// Get CU-Header offset
	cuHeaderOff = id - lOff;

	// Get CU-Die offset
	if(dwarf_get_cu_die_offset_given_cu_header_offset(
				m_dbg,
				cuHeaderOff,
				&cuOff,
				NULL) != DW_DLV_OK)
	{
		return ret;
	}
	ret = cuOff;
	return ret;
}

int CDwarfEngine::_populateFileNameVecForCu(Dwarf_Off cuOff)
{
	int ret = -1;
	Dwarf_Die cuDie;

	// Get CU-Die
	if( dwarf_offdie(m_dbg, cuOff, &cuDie, NULL) != DW_DLV_OK)
	{
		return ret;
	}

	/* --------------------------------------------------
	 * Extract srcfile / srcline for each DIE
	 * --------------------------------------------------*/
	FILENAMEVEC vec_fileName;
	vec_fileName.clear();
	Dwarf_Signed cnt;
	char **srcFiles;

	if( dwarf_srcfiles(cuDie, &srcFiles, &cnt, NULL) == DW_DLV_OK )
	{
		for( int i = 0; i< cnt ; i++)
		{
			/* NOTE:
			 * Check if this is a fullpath. Otherwise, prepend the path.
			 */
			string filename(srcFiles[i]);
			if (filename.at(0) != '/') {
				string dir;
				if (get_DW_comp_dir(cuDie, &dir) == 0) {
					filename = dir + "/" + filename;	
				}
			}

			vec_fileName.push_back(filename);

			dwarf_dealloc(m_dbg, srcFiles[i], DW_DLA_STRING);
		}
		m_cuFileNameVecMap[cuOff] = vec_fileName;
		dwarf_dealloc(m_dbg, srcFiles, DW_DLA_LIST);
		ret = 0;
	}
	else
	{
		if(DWARF_DEBUG) fprintf(stderr,"ERROR: Can't get source file .\n");
	}
	return ret;
} // CDwarfEngine::_populateFileNameVecForCu

int CDwarfEngine::_populateLineInfoMapForCu(Dwarf_Off cuOff)
{
	int ret = -1;
	Dwarf_Unsigned fileNo;	
	Dwarf_Die cuDie;
	ADDRLINEMAP addrLineMap;
	Dwarf_Signed cnt;

	// Sanity check
	if(!m_dbg)
		return ret;	

	// Get CU-Die
	if( dwarf_offdie(m_dbg, cuOff, &cuDie, NULL) != DW_DLV_OK)
	{
		return ret;
	}
	
	Dwarf_Line *linebuf;
	if ( dwarf_srclines(cuDie, &linebuf, &cnt, NULL) == DW_DLV_OK )
	{
		FILELINE fileLine;
		for(int i = 0 ; i < cnt ; i++)
		{
			Dwarf_Unsigned lineNo;
			Dwarf_Addr lineAddr;
			Dwarf_Bool lineEndSeq = 0;
			
			dwarf_line_srcfileno(linebuf[i],&fileNo,NULL);
			dwarf_lineno(linebuf[i],&lineNo,NULL);
			dwarf_lineaddr(linebuf[i],&lineAddr,NULL);
			
			//  Check if this is end of text sequence
			if (dwarf_lineendsequence(linebuf[i], &lineEndSeq, NULL) == DW_DLV_OK) {
				if (lineEndSeq)
					continue;
			}
			
			fileLine.fileName = m_cuFileNameVecMap[cuOff][fileNo-1];
			fileLine.lineNo   = lineNo;	

			// NOTE: [Suravee]
			// In some case, we have seen duplicate entry in the map.
			// We will use the latter one we find.
			ADDRLINEMAP::iterator it = addrLineMap.find(lineAddr);
			if (it != addrLineMap.end()) {
				addrLineMap.erase(it);
			}
			addrLineMap.insert(ADDRLINEMAP::value_type(lineAddr, fileLine)); 
		}

		dwarf_dealloc(m_dbg, linebuf, DW_DLA_LIST);
		m_cuLineInfoMap.insert(CULINEINFOMAP::value_type(cuOff, addrLineMap));
		ret = 0;
	}

	return ret;

} // CDwarfEngine::_populateLineInfoMapForCu

int CDwarfEngine::getLineInfoForInlineInstance(
				unsigned int ilId,
				string *fileName, 
				unsigned int *linenum)
{
	int ret = -1;
	Dwarf_Off cuOff = 0;
	DW_sym_info symInfo;
	InlineInst inlineInfo;	
	
	// Sanity check
	if(!fileName || !linenum || !m_dbg)
		return ret;

	// ----------------------------------------------------
	// Populate map of File name vector for this symbol if not yet.
	cuOff = _getCuOff(ilId);
	if( m_cuFileNameVecMap.find(cuOff) == m_cuFileNameVecMap.end())
		_populateFileNameVecForCu(cuOff);

	// ----------------------------------------------------
	// Here, we get the line/file of where the inline
	// function is called (caller side) which is 
	// stored in the InlinelInstMap

	InlinelInstMap::iterator inlineIter = m_inlineMap.find(ilId);
	if (inlineIter == m_inlineMap.end())
		return ret;

	*linenum = inlineIter->second.callLineNo;

	if(m_cuFileNameVecMap.find(cuOff) != m_cuFileNameVecMap.end()  )
	{
		*fileName = m_cuFileNameVecMap[cuOff][inlineIter->second.callFileNo-1];
		ret = 0;
	}

	return ret;
}


int CDwarfEngine::getLineInfoForAddr(Dwarf_Addr addr, 
				string *fileName, 
				unsigned int *linenum,
				bool isCaller)
{
	int ret = -1;
	Dwarf_Off cuOff = 0;
	DW_sym_info symInfo; 
	InlineInst inlineInfo;	
	
	// Sanity check
	if(!fileName || !linenum || !m_dbg)
		return ret;	
	// ----------------------------------------------------
	// First, figure out what is the symbol this address belong to
	if(getSymbolForAddr(addr,&symInfo,&inlineInfo) != 0)
		return ret;
	// ----------------------------------------------------
	// Populate map of File name vector for this symbol if not yet.
	cuOff = _getCuOff(symInfo.symId);
	if( m_cuFileNameVecMap.find(cuOff) == m_cuFileNameVecMap.end())
		_populateFileNameVecForCu(cuOff);

	// ----------------------------------------------------
	// If address belongs to inline instance, look in InlinelInstMap 
	if(inlineInfo.startAddr!= 0 && isCaller )
	{
		// Here, we get the line/file of where the inline
		// function is called (caller side) which is 
		// stored in the InlinelInstMap

// FIXME	
		// For find to work, the "<" function for ADDRESSKEY needed modification
/*
		ADDRESSKEY key ;
		key.startAddr = inlineInfo.startAddr;
		key.stopAddr  = inlineInfo.stopAddr;
		InlinelInstMap::iterator inlineIter = m_inlineMap.find(key);
		if(inlineIter!= m_inlineMap.end())
		{
			*linenum = inlineIter->second.callLineNo;

			Dwarf_Off cuOff = _getCuOff(inlineInfo.symId); 	
			if(m_cuFileNameVecMap.find(cuOff) != m_cuFileNameVecMap.end()  )
			{
				*fileName = m_cuFileNameVecMap[cuOff][inlineIter->second.callFileNo-1];
				ret = 0;
			}
		}
*/
/*
		// This is a hack,
		InlinelInstMap::iterator inlineIter = m_inlineMap.begin();
		for(;inlineIter != m_inlineMap.end(); inlineIter++)
		{
			if(inlineIter->first.startAddr == inlineInfo.startAddr 
			&& inlineIter->first.stopAddr  == inlineInfo.stopAddr)
			{
				*linenum = inlineIter->second.callLineNo;

				Dwarf_Off cuOff = _getCuOff(inlineInfo.symId); 	
				if(m_cuFileNameVecMap.find(cuOff) != m_cuFileNameVecMap.end()  )
				{
					*fileName = m_cuFileNameVecMap[cuOff][inlineIter->second.callFileNo-1];
					ret = 0;
				}
				break;
			}
		}
*/
	} else {
		// ----------------------------------------------------
		// If address does not belongs to inline instance.

		// Populate map of addr to File/line info for this cu if not yet.
		CULINEINFOMAP::iterator it = m_cuLineInfoMap.find(cuOff);
		if( it == m_cuLineInfoMap.end())
		{
			_populateLineInfoMapForCu(cuOff);
		}

		// Here we try to find the address from the CU .debug_line
		ADDRLINEMAP::reverse_iterator rit = m_cuLineInfoMap.find(cuOff)->second.rbegin();
		ADDRLINEMAP::reverse_iterator rit_end = m_cuLineInfoMap.find(cuOff)->second.rend();
		for(; rit != rit_end ; rit++)
		{
#if 0 
			fprintf(stderr,"DEBUG: lineNo = %u, lineAddr = %x, file = %s\n", 
					rit->second.lineNo, rit->first, rit->second.fileName.c_str());
#endif 
			if(addr >= rit->first)
			{
				*fileName = rit->second.fileName;
				*linenum  = rit->second.lineNo;
				ret = 0;
				break;
			}
		}
	}

	// NOTE: Open64 file name has the following formats
	// "hostname.:junk.:fullpath_to_source"	
	// We need to check and strip it. We here look for ':' since
	// this should not be part of the full path anyways (on Linux).
	if (fileName->length() != 0) {
		size_t st;
		while ( (st = fileName->find_first_of(':')) != string::npos) {
			*fileName  = fileName->substr(st + 1);
		}
	}

	if (fileName->length() == 0) {	
		ret = -1;
	}
	
	return ret;	
}


int CDwarfEngine::getNumInline(Dwarf_Addr startAddr)
{
	int ret = -1;

	//Sanity Check
	if (!m_inlineFlag  || !m_dbg )
		return ret;
	
	// First find out the symbol with this start Address 
	SUBROUTINEMAP::iterator funcIter = find_func_from_addr(startAddr);
	if(funcIter != m_routineMap.end())
		ret = funcIter->second.numInline;

	return ret;
}


int CDwarfEngine::getInlinesForFunction(Dwarf_Addr startAddr, InlinelInstMap &inlines )
{
	int ret = -1;
	int cnt = 0;

	//Sanity Check
	if (!m_inlineFlag  || !m_dbg )
		return ret;
	
	// First find out the symbol with this start Address 
	SUBROUTINEMAP::iterator funcIter = find_func_from_addr(startAddr);
	if (funcIter == m_routineMap.end())
		return ret;

	if ( funcIter->second.numInline == 0)
		return 0;

	// search for inline item
	if (m_inlineFlag 
	&& funcIter->second.numInline != 0) 
	{
		InlinelInstMap::iterator inlineIter = m_inlineMap.begin();
		for (;inlineIter != m_inlineMap.end(); inlineIter++)
		{
			if (inlineIter->second.parentAddr == funcIter->first.startAddr)
			{
				inlines.insert(InlinelInstMap::value_type(
					inlineIter->first, inlineIter->second));
				cnt++;
			}
		}
	}
	ret = cnt;

	return ret;
}


int CDwarfEngine::getFileForId(Dwarf_Off id, string *fileName)
{
	int ret = -1;
	DW_sym_info symInfo, inlineInfo;	
	Dwarf_Unsigned fileNo;	
	Dwarf_Off cuOff = 0;
	Dwarf_Die die;
	
	// Sanity check
	if(!fileName || !m_dbg)
		return ret;	

	// ----------------------------------------------------
	// Get file name vector
	cuOff = _getCuOff(id);
	if( m_cuFileNameVecMap.find(cuOff) == m_cuFileNameVecMap.end())
	{
		_populateFileNameVecForCu(cuOff);
	}

	// ----------------------------------------------------
	if( dwarf_offdie(m_dbg, id, &die, NULL) != DW_DLV_OK)
	{
		return ret;
	}
	if(get_DW_decl_file_attr(die, &fileNo) == 0)
	{
		*fileName = m_cuFileNameVecMap[cuOff][fileNo-1];
		ret = 0;
	}

	return ret;
} //CDwarfEngine::getFileForId

int CDwarfEngine::getStartLineForId(Dwarf_Off id, unsigned int *lineNum)
{
	int ret = -1;
	DW_sym_info symInfo, inlineInfo;	
	Dwarf_Unsigned lineNo;	
	Dwarf_Die die;
	
	// Sanity check
	if(!lineNum || !m_dbg)
		return ret;	

	// ----------------------------------------------------
	if( dwarf_offdie(m_dbg, id, &die, NULL) != DW_DLV_OK)
	{
		return ret;
	}

	if(get_DW_decl_line_attr(die, &lineNo) == 0)
	{
		*lineNum          = lineNo;
		ret = 0;
	}

	return ret;
} //CDwarfEngine::getLineForId

#if 0 // This is an old implementation.
int CDwarfEngine::getLineInfoForAddr(Dwarf_Addr addr, 
					string *fileName, 
					unsigned int *lineNum)
{
	DW_sym_info symInfo, inlineInfo;	
	ADDRESSKEY symAddr,inlineAddr;
	FILELINE fileLine;
	int ret = -1;
	
	// Sanity check
	if(!fileName || !lineNum || !m_dbg)
		return ret;	

	// ----------------------------------------------------
	// First, figure out what is the symbol this address belong to
	if(getSymbolForAddr(addr,&symInfo,&inlineInfo) != 0)
	{
		return ret;
	}
	symAddr.startAddr = symInfo.startAddr;
	symAddr.stopAddr  = symInfo.stopAddr;
	
	inlineAddr.startAddr = inlineInfo.startAddr;
	inlineAddr.stopAddr  = inlineInfo.stopAddr;

	// ----------------------------------------------------
	// Second
	Dwarf_Die 	curDie = NULL;
	Dwarf_Die 	die1 = NULL;
	Dwarf_Die 	die2 = NULL;
	Dwarf_Unsigned 	hdr = 0;
	Dwarf_Half 	curTag = 0;
	Dwarf_Unsigned  fileNo = 0;
	Dwarf_Unsigned  lineNo = 0;
	std::vector<string> vec_fileName;
	vec_fileName.clear();
	bool foundL1 = false;

	// A loop to recursively get the CU header until finish
	while(dwarf_next_cu_header(m_dbg,NULL,NULL,NULL,NULL,&hdr,NULL) == DW_DLV_OK) 
	{
		// Obtain DIE for current CU.
		if ( dwarf_siblingof( m_dbg, NULL, &curDie, NULL ) != DW_DLV_OK ) {
			continue;	
		}

		// Get the tag of current DIE 
		if( dwarf_tag( curDie, &curTag, NULL) != DW_DLV_OK )
			break;	

		if(curTag != DW_TAG_compile_unit)
			break;

		/* --------------------------------------------------
		 * Extract srcfile info for each DIE
		 * --------------------------------------------------*/
		Dwarf_Signed cnt;
		char **srcFiles;

		if( dwarf_srcfiles(curDie, &srcFiles, &cnt, NULL) == DW_DLV_OK )
		{
			vec_fileName.clear();
			for( int i = 0; i< cnt ; i++)
			{
fprintf(stderr,"Found srcFiles[%d] = %s\n",i, srcFiles[i]);
				vec_fileName.push_back(string(srcFiles[i]));

				dwarf_dealloc(m_dbg, srcFiles[i], DW_DLA_STRING);
			}

			dwarf_dealloc(m_dbg, srcFiles, DW_DLA_LIST);
		}
		else
		{
			if(DWARF_DEBUG) fprintf(stderr,
				"ERROR: get_DW_sym_file_line:  Can't get source file .\n");
		}


		// ----------------------------------
		// Looking for level 1 die
fprintf(stderr,"Level 1\n");
		if(dwarf_child(curDie, &die1, NULL) != DW_DLV_OK)
			continue;
fprintf(stderr,"Level 1.1\n");

		if(find_level1_die(symAddr, &die1 ) == 0)
		{
			foundL1 = true;
		} else if(find_level1_die(inlineAddr, &die1 ) == 0){
			foundL1 = true;
		}else{
			continue;  // go to next CU
		}
fprintf(stderr,"Level 1.2\n");

		if(foundL1
		&& inlineAddr.startAddr == 0
		&& inlineAddr.stopAddr == 0)
		{
fprintf(stderr,"Level 1.3\n");
			// Get file name and line no from level1
			if( get_DW_decl_file_attr(die1,&fileNo) == 0
			&&  get_DW_decl_line_attr(die1,&lineNo) == 0)
			{
				fileLine.fileName = vec_fileName[fileNo-1];
				*fileName         = vec_fileName[fileNo-1];
				fileLine.lineNo   = lineNo;
				*lineNum          = lineNo;

				// Store in the FILELINEMAP
				m_fileLineMap.insert(FILELINEMAP::value_type(symAddr, fileLine));
				
				ret = 0;
			}else{
				Dwarf_Die aboDIE,spcDIE;
				if( find_abstract_origin_die(die1,&aboDIE) == 0)
				{
					// Get file name and line no from level2+
					if( get_DW_decl_file_attr(aboDIE,&fileNo) == 0
					&&  get_DW_decl_line_attr(aboDIE,&lineNo) == 0)
					{
						fileLine.fileName = vec_fileName[fileNo-1];
						*fileName         = vec_fileName[fileNo-1];
						fileLine.lineNo   = lineNo;
						*lineNum          = lineNo;

						// Store in the FILELINEMAP
						m_fileLineMap.insert(FILELINEMAP::value_type(symAddr, fileLine));

						ret = 0;
						// Clean up aboDIE, spcDIE
						if (aboDIE) {
							dwarf_dealloc(m_dbg,aboDIE, DW_DLA_DIE);
							aboDIE = 0;
						}
					}
				}
			}
			break;
		}

fprintf(stderr,"Level 2+\n");
		// ----------------------------------
		// Looking for level 2+ die
		if(dwarf_child(die1, &die2, NULL) != DW_DLV_OK)
			break;
fprintf(stderr,"Level 2+.1\n");

		if(find_level2plus_die( inlineAddr, &die2 ) != 0)
		{
fprintf(stderr,"Level 2+.2\n");
			break;
		}else {
fprintf(stderr,"Level 2+.3\n");
			// Need to go to DW_AT_abstract_origin --> DW_AT_specification
			// to get file/line info.
			Dwarf_Die aboDIE,spcDIE;
			
			if( find_abstract_origin_die(die2,&aboDIE) == 0)
			{
				if( find_specification_die(aboDIE,&spcDIE) == 0)
				{
fprintf(stderr,"Level 2+.4\n");
					// Get file name and line no from level2+
					if( get_DW_decl_file_attr(spcDIE,&fileNo) == 0
					&&  get_DW_decl_line_attr(spcDIE,&lineNo) == 0)
					{
						fileLine.fileName = vec_fileName[fileNo-1];
						*fileName         = vec_fileName[fileNo-1];
						fileLine.lineNo   = lineNo;
						*lineNum          = lineNo;

						// Store in the FILELINEMAP
						m_fileLineMap.insert(FILELINEMAP::value_type(inlineAddr, fileLine));

						ret = 0;
					}
		
					// Clean up aboDIE, spcDIE
					if (aboDIE) {
						dwarf_dealloc(m_dbg,aboDIE, DW_DLA_DIE);
						aboDIE = 0;
					}
					if (spcDIE) {
						dwarf_dealloc(m_dbg,spcDIE, DW_DLA_DIE);
						aboDIE = 0;
					}
				}else{
fprintf(stderr,"Level 2+.4\n");
					// Get file name and line no from level2+
					if( get_DW_decl_file_attr(aboDIE,&fileNo) == 0
					&&  get_DW_decl_line_attr(aboDIE,&lineNo) == 0)
					{
						fileLine.fileName = vec_fileName[fileNo-1];
						*fileName         = vec_fileName[fileNo-1];
						fileLine.lineNo   = lineNo;
						*lineNum          = lineNo;

						// Store in the FILELINEMAP
						m_fileLineMap.insert(FILELINEMAP::value_type(inlineAddr, fileLine));

						ret = 0;
					}
		
					// Clean up aboDIE
					if (aboDIE) {
						dwarf_dealloc(m_dbg,aboDIE, DW_DLA_DIE);
						aboDIE = 0;
					}
				}
			}
		}	

		// ----------------------------------
		// Clean up
		vec_fileName.clear();

		// de-allocate the DIE
		dwarf_dealloc(m_dbg, curDie, DW_DLA_DIE);

	}
	return ret;
	
}
#endif


#if 0

int CDwarfEngine::getFileForAddr(Dwarf_Addr addr, string *fileName)
{
	int ret = -1;
	DW_sym_info symInfo, inlineInfo;	
	
	// Sanity check
	if(!fileName || !m_dbg)
		return ret;	

	// ----------------------------------------------------
	// First, figure out what is the symbol this address belong to
	if(getSymbolForAddr(addr,&symInfo,&inlineInfo) != 0)
	{
		return ret;
	}

	if(inlineInfo.symId != 0)
		ret = getFileForId(inlineInfo.symId, fileName);
	else if (symInfo.symId != 0)
		ret = getFileForId(symInfo.symId, fileName);
	return ret;	
}

int CDwarfEngine::getLineForAddr(Dwarf_Addr addr, unsigned int *linenum)
{
	int ret = -1;
	DW_sym_info symInfo, inlineInfo;	
	
	// Sanity check
	if(!linenum || !m_dbg)
		return ret;	

	// ----------------------------------------------------
	// First, figure out what is the symbol this address belong to
	if(getSymbolForAddr(addr,&symInfo,&inlineInfo) != 0)
	{
		return ret;
	}

	if(inlineInfo.symId != 0)
		ret = getLineForId(inlineInfo.symId, linenum);
	else if (symInfo.symId != 0)
		ret = getLineForId(symInfo.symId, linenum);
	return ret;	
}
#endif
