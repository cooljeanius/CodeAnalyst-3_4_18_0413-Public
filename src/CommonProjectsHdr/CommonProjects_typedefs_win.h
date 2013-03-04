/** \file CommonProjects_typedefs_win.h

*/

#ifndef _COMMONPROJECTS_TYPEDEFS_WIN_H_
#define _COMMONPROJECTS_TYPEDEFS_WIN_H_

// VS warns about use of a private non-exported STL list class by clients
// Warning is disabled. See MSDN PSS ID Number: 134980
#pragma warning(disable:4251)

// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER
// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501	
#endif

#ifndef _WIN32_WINNT
// Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501	
#endif

#ifndef WIN32_LEAN_AND_MEAN
// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>

typedef unsigned __int8	    UCHAR;
typedef unsigned __int16	USHORT;
typedef __int32			    INT32;
typedef unsigned __int32	UINT32;
typedef __int64			    INT64;
typedef unsigned __int64	UINT64;

#define MAX_UINT64		    0xffffffffffffffffui64
#define MAX_INT64		    0x7fffffffffffffffi64
#define MIN_UINT64		    0x0000000000000000ui64
#define MIN_INT64		    0x8000000000000000i64

#define sign64(num64)	    num64##i64
#define usign64(num64)	    num64##ui64

#endif //_COMMONPROJECTS_TYPEDEFS_WIN_H_
