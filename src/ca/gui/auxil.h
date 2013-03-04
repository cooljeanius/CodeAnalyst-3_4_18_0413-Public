//$Id: auxil.h,v 1.2 2005/02/14 16:52:40 jyeh Exp $
// interface for auxillary functions

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

#if !defined (_AUXIL_H)
#define _AUXIL_H

#include "stdafx.h"

extern void ClearAllSessionGlobalMap();

extern void ClearSessionGlobalMap(QString sessionName);

extern void AddModGlobalMap(QString sessionName, 
			QString modName,
	                TaskSampleMap **pTaskMap, 
			JitBlockPtrVec **pJitBlkPtrVec);

extern void RemoveModGlobalMap(QString sessionName, QString modName);

#endif
