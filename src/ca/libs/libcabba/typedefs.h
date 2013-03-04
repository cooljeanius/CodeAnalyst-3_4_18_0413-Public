
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

#ifndef _AMDTOOLS_TYPEDEFS_H_
#define _AMDTOOLS_TYPEDEFS_H_

/* This file contains only simple type defines (requiring no other include files) */

#if defined( WIN32 ) || defined( WIN64 ) || defined( _WIN32_WCE )

#define AMD_MAX_UINT64 0xffffffffffffffffui64
#define AMD_MAX_INT64  0x7fffffffffffffffi64
#define AMD_MIN_UINT64 0x0000000000000000ui64
#define AMD_MIN_INT64  0x8000000000000000i64

typedef unsigned __int64		AMD_UINT64;
typedef __int64					AMD_INT64;

#else

#define AMD_MAX_UINT64 0xffffffffffffffffULL
#define AMD_MAX_INT64  0x7fffffffffffffffLL
#define AMD_MIN_UINT64 0x0000000000000000ULL
#define AMD_MIN_INT64  0x8000000000000000LL

typedef unsigned long long int	AMD_UINT64;
typedef long long int			AMD_INT64;

#endif

#define AMD_MAX_UINT32 0xffffffffU
#define AMD_MAX_INT32  0x7fffffff
#define AMD_MAX_UINT16 0xffffU
#define AMD_MAX_INT16  0x7fff
#define AMD_MAX_UINT8  0xffU
#define AMD_MAX_INT8   0x7f
 
#define AMD_MIN_UINT32 0x00000000U
#define AMD_MIN_INT32  0x80000000
#define AMD_MIN_UINT16 0x0000U
#define AMD_MIN_INT16  0x8000
#define AMD_MIN_UINT8  0x00U
#define AMD_MIN_INT8   0x80

typedef unsigned int			AMD_UINT32;
typedef	signed int				AMD_INT32;
typedef	unsigned short int		AMD_UINT16;
typedef short int				AMD_INT16;
typedef	unsigned char			AMD_UINT8;
typedef	signed char				AMD_INT8;
typedef	float					AMD_FLOAT32;
typedef double					AMD_FLOAT64;

typedef int AMD_BOOL;
#define AMD_FALSE 0
#define AMD_TRUE 1

#endif
