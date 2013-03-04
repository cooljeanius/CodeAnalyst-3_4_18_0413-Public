#include "stdafx.h"
#include "libCAdata.h"
#include "Function.h"

using namespace std;

Function::Function()
{
	m_total			= 0;
	m_baseAddr		= 0;	
}


Function::~Function()
{
		m_aptMap.clear();
		m_jilMap.clear();
}

/*
bool Function::operator==(const Function& f) const
{ 
	TSC	this_total;
	TSC	that_total;

	this_total.qwTimeStamp = m_total;
	that_total.qwTimeStamp = f.m_total;

	return (this_total.dwHigh == that_total.dwHigh) 
		&& (this_total.dwLow == that_total.dwLow);
}
*/

