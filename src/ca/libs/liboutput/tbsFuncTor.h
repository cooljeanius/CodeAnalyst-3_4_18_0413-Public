#include <Q3ValueList>
//$Id: tbsFuncTor.h 20128 2011-08-11 12:09:44Z sjeganat $
// Function pointer classes

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

#ifndef _TBSFUNCTOR_H_
#define _TBSFUNCTOR_H_
/*
 * The callser of TbsFile::readFuncInfo and TbsFile::readModInfo
 * must supply the callback if it wishes to display any kind of
 * progress.  This is done by the caller supplying a TBSDisFunctor
 * which contains valid code for displaying progress back to the 
 * user.
 */

// abstract base class
class TFunctor
{
public:
	virtual bool operator()(const char * msg)=0;  // call using operator
	virtual ~TFunctor(){};  
};


template <class TClass> class TBSDisFunctor : public TFunctor
{
private:
	/// pointer to member function
	bool (TClass::*fpt)(const char *);   

	/// pointer to object
	TClass* pt2Object;                  

public:
	/// constructor - takes pointer to an object and pointer to a member and stores
	/// them in two private variables
	TBSDisFunctor(TClass* _pt2Object, bool(TClass::*_fpt)(const char *))
	{ pt2Object = _pt2Object;  fpt=_fpt; };

	// override operator "()"
	bool operator()(const char * msg)
	{ return (*pt2Object.*fpt)(msg);};              // execute member function
};

///////////////////////////////////////////////////////////////////////////
/*
 * derived template class
 * Function pointers are used to implement callbacks in case both
 * version 1 and version 3 support are needed in the binary. The set of
 * function pointers are decided in function TBSFile::processEnvLine() after
 * reading the line TBPFILEVERSION=<version number.
 */
template <class TClass> class TEnvsectionFunctor
{
private:
	/// pointer to member function
	bool (TClass::*fpt)( EnvValues & env_vals);
	/// pointer to object
	TClass* pt2Object;                  

public:
	/// constructor - takes pointer to an object and pointer to a member and stores
	/// them in two private variables
	TEnvsectionFunctor(TClass* _pt2Object, bool(TClass::*_fpt)( EnvValues & env_vals))
	{ pt2Object = _pt2Object;  fpt=_fpt; };

	// override operator "()"
	bool operator()( EnvValues & env_vals)
	{ return (*pt2Object.*fpt)(env_vals);};
};

///////////////////////////////////////////////////////////////////////////
template <class TClass> class TModFunctor
{
private:
	/// pointer to member function
	bool (TClass::*fpt)(Q3ValueList<SYS_LV_ITEM> & list, 
				EnvValues & env_vals);
	/// pointer to object
	TClass* pt2Object;                  

public:
	/// constructor - takes pointer to an object and pointer to a member and stores
	/// them in two private variables
	TModFunctor(TClass* _pt2Object, bool(TClass::*_fpt)(Q3ValueList<SYS_LV_ITEM> & list, 
				EnvValues & env_vals))
	{ pt2Object = _pt2Object;  fpt=_fpt; };

	// override operator "()"
	bool operator()(Q3ValueList<SYS_LV_ITEM> & list, 
				EnvValues & env_vals)
	{ return (*pt2Object.*fpt)(list , env_vals);};
};


///////////////////////////////////////////////////////////////////////////
template <class TClass> class TFuncFunctor
{
private:
	/// pointer to member function
	bool (TClass::*fpt)(Q3ValueList<MOD_LV_ITEM> & list, 
		QString const & module_name, 
		TFunctor *,
		EnvValues & env_vals);
	/// pointer to object
	TClass* pt2Object;                  

public:
	/// constructor - takes pointer to an object and pointer to a member and stores
	/// them in two private variables
	TFuncFunctor(TClass* _pt2Object, bool(TClass::*_fpt)(
		Q3ValueList<MOD_LV_ITEM> & list,
		QString const &,
		TFunctor *,
		EnvValues & env_vals))
	{ pt2Object = _pt2Object;  fpt=_fpt; };

	// override operator "()"
	bool operator()(Q3ValueList<MOD_LV_ITEM> & list, 
		QString const & mod_name, 
		TFunctor * dis_callback,
		EnvValues & env_vals)
	{ return (*pt2Object.*fpt)(list, mod_name, dis_callback, env_vals);};
};

///////////////////////////////////////////////////////////////////////////

template <class TClass> class TEnvFunctor
{
private:
	/// pointer to member function
	void (TClass::*fpt)(QString const & line, EnvValues & env_vals);   

	/// pointer to object
	TClass* pt2Object;                  

public:
	/// constructor - takes pointer to an object and pointer to a member and stores
	/// them in two private variables
	TEnvFunctor(TClass* _pt2Object, void(TClass::*_fpt)(QString const & line, EnvValues & env_vals))
	{ pt2Object = _pt2Object;  fpt=_fpt; };

	// override operator "()"
	void operator()(QString const & line, EnvValues & env_vals)
	{ (*pt2Object.*fpt)(line,env_vals);};              // execute member function
};



#endif //#ifndef_TBSFUNCTOR_H_
