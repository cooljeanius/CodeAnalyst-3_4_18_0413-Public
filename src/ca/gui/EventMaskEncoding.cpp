//$Id$
// Functions for EventSelect and Unitmask encoding.

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2006 Advanced Micro Devices, Inc.
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

#include "stdafx.h"
#include "EventMaskEncoding.h"

EVMASK_T EncodeEventMask (unsigned int event, unsigned char unitMask)
{
	//Since event select is 16 bits (for IBS) , this will combine them
	EVMASK_T ret = event + (unitMask << 16);
	return ret;
}

void DecodeEventMask (EVMASK_T encoded, unsigned int *pEvent, unsigned char *pUnitMask)
{
	if (NULL != pEvent)
	{
		unsigned int mask = 0x0000ffff;
		*pEvent = encoded & mask;
	}
	if (NULL != pUnitMask)
	{
		*pUnitMask = (encoded >> 16);
	}
}
