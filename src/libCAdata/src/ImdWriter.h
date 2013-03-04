//$Id: ImdWriter.h 17060 2009-08-25 15:41:38Z ssuthiku $

//  Interface for the ImdWriter class.

/*
// CodeAnalyst for Open Source
// Copyright 2002 - 2010 Advanced Micro Devices, Inc.
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

#ifndef _IMDWRITER_H_
#define _IMDWRITER_H_

#include "libCAdata.h"
#include "CaDataWriter.h"

using namespace std;

class ImdWriter : public CaDataWriter
{
public:
	enum imd_output_stage 
	{
		evOut_ModDetail = 1,
		evOut_JitDetail,
	};

	ImdWriter();

	~ImdWriter();

	bool writeToFile(CA_Module & mod, map<EVMASK_T, int> * pEvToIndexMap);

private:	
	bool writeModDetailProlog(CA_Module & mod);

	bool writeModDetailLine(TGID_T tgid, TID_T tid, VADDR sampAddr, 
		const AggregatedSample & agSamp);
				
	bool writeModDetailEpilog();

	bool writeSubProlog( const CA_Function & func);
	
	bool writeSubEpilog();

	map<EVMASK_T, int>		*m_pEvToIndexMap;
};

#endif //ifndef _IMDWRITER_H_
