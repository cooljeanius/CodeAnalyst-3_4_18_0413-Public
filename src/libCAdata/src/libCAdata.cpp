// libCAdata.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "libCAdata.h"


// This is an example of an exported variable
LIBCADATA_API int nlibCAdata=0;

// This is an example of an exported function.
LIBCADATA_API int fnlibCAdata(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see libCAdata.h for the class definition
ClibCAdata::ClibCAdata()
{
	return;
}
