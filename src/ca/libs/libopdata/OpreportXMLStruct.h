#ifndef _OPREPORTXMLSTRUCT_H_
#define _OPREPORTXMLSTRUCT_H_

#include <list>
#include <map>

using namespace std;

class op_event
{
public:
	std::string	eventname;
	unsigned long	setupcount;
	
	op_event()
	{
		setupcount = 0;
	};	
} ;

class op_class
{
public:
	std::string	name;
	unsigned int	cpu;
	op_event *	event;		
	unsigned int	mask;
	
	op_class()
	{
		cpu   = 0;
		event = NULL;
		mask  = 0;
	};
} ;

typedef map <std::string, op_class> CLASS_MAP;

class op_symbol
{
public:
	unsigned int	idref;
	unsigned int	lo;
	unsigned int	hi;
	
	op_symbol()
	{
		idref = 0;
		lo    = 0;
		hi    = 0;
	};
};

typedef list <op_symbol> SYMBOL_LIST;

class op_module
{
public:
	std::string	name;
	unsigned int	symbolCount;
	SYMBOL_LIST	symbolList;

	op_module() 
	{
		symbolCount = 0;
	};

	~op_module() 
	{ 
		symbolList.clear();
	};
	
};

typedef list<op_module> MODULE_LIST;

class op_binary
{
public:
	std::string	name;
	unsigned int	symbolCount;
	SYMBOL_LIST	symbolList;
	MODULE_LIST	modList;	

	op_binary()
	{
		symbolCount = 0;
	};

	~op_binary() 
	{ 
		symbolList.clear();
		modList.clear();
	};
};

typedef list <op_binary> BINARY_LIST;


class op_thread
{
public:
	unsigned int	tid;
	MODULE_LIST	modList;	
	
	op_thread()
	{
		tid = 0;
	};

	~op_thread() 
	{
		modList.clear(); 
	};
};

typedef list <op_thread> THREAD_LIST;


class op_process
{
public:
	unsigned int	pid;
	std::string	name;
	THREAD_LIST	threadList;

	op_process()
	{
		pid = 0;
	};

	~op_process() 
	{ 
		threadList.clear();
	};
};

typedef list <op_process> PROCESS_LIST;

///////////////////////////////////////

class op_symboldata
{
public:
	std::string	name;
	unsigned int    id;
	unsigned long	startingaddr;
	
	op_symboldata()
	{
		id = 0;
		startingaddr = 0;
	};
} ;

///////////////////////////////////////

class op_detaildata
{
public:
	unsigned int	id;
	unsigned long	vmaoffset;
	unsigned int	evcount;
	std::string	evclass;

	op_detaildata()
	{
		id        = 0;
		vmaoffset = 0;
		evcount   = 0;
	};
};

typedef vector <op_detaildata> DETAILDATA_VEC;

class op_symboldetails
{
public:
	unsigned int	id;
	DETAILDATA_VEC	detaildataVec;

	op_symboldetails()
	{
		id = 0;
	};
	
	~op_symboldetails()
	{
		detaildataVec.clear();
	};
};

#endif /* _OPREPORTXMLHANDLER_H_ */
