/** \file CommonProjects_typedefs.h

*/

#ifndef _COMMONPROJECTS_TYPEDEFS_H_
#define _COMMONPROJECTS_TYPEDEFS_H_

#if ( defined (_WIN32) || defined (_WIN64) )

#include <CommonProjects_typedefs_win.h>

#else  // ( defined (_WIN32) || defined (_WIN64) )

#include <CommonProjects_typedefs_lin.h>

#endif // ( defined (_WIN32) || defined (_WIN64) )

#ifndef __cplusplus

#define TRUE			1
#define FALSE			0

#ifndef bool
typedef int			    bool;
#endif

#ifndef BOOL
typedef bool 			BOOL;
#endif

#define public
#define private static
#endif //__cplusplus

typedef	unsigned short int	UINT16;
typedef short int		    INT16;
typedef	unsigned char		UINT8;
typedef	signed char		    INT8;
typedef	float			    FLOAT32;
typedef double			    FLOAT64;

#define MAX_UINT32		    0xffffffffU
#define MAX_INT32		    0x7fffffff
#define MAX_UINT16		    0xffffU
#define MAX_INT16		    0x7fff
#define MAX_UINT8		    0xffU
#define MAX_INT8		    0x7f

#define MIN_UINT32		    0x00000000U
#define MIN_INT32		    0x80000000
#define MIN_UINT16		    0x0000U
#define MIN_INT16		    0x8000
#define MIN_UINT8		    0x00U
#define MIN_INT8		    0x80

#if !defined(ULONG)
typedef unsigned long		ULONG;
#endif

#if !defined(ULONG64)
typedef unsigned long long	ULONG64;
#endif

#if !defined(LONG)
typedef long 				LONG;
#endif

#if !defined(NULL) 
#define NULL 				0
#endif

#if !defined(BYTE)
typedef UINT8 				BYTE ;
#endif

#if !defined(UINT)
typedef unsigned int		UINT;
#endif

typedef UINT64				VADDR;

#endif //_COMMONPROJECTS_TYPEDEFS_H_

