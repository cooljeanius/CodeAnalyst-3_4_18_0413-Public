#ifndef _PERFEVENT_H_
#define _PERFEVENT_H_

#include <qstring.h>

#define		IBS_FETCH_EVENT_BEGIN	0xf000
#define		IBS_OP_EVENT_BEGIN	0xf100
#define		IBS_OP_EVENT_END	0xf24C

class PerfEvent
{
public:
	enum PerfEventFlag {
		Usr  = 0x1,
		Os   = 0x2,
		Edge = 0x4,
		Host = 0x8,
		Guest= 0x10
	};

	enum PerfEventType {
		Invalid = 0,
		Pmc,
		IbsFetch,
		IbsOp,
		Nb,
		OProfile,
	};

	PerfEvent() {
		_select = 0;
		count = 0;	
		_umask = 0;
		flags = PerfEvent::Usr | PerfEvent::Os;	// Default value
		_type  = Invalid;
		minCount = 0;
		allocMask = 0;
		counter = -1;
	};

	bool operator < (const PerfEvent & other) const {
		if (_select < other._select)
			return true;
		else if (_select == other._select 
		     &&  _umask < other._umask)
			return true;
		else if (_select == other._select 
		     &&  _umask == other._umask 
		     &&  flags < other.flags)
			return true;
		else if (_select == other._select 
		     &&  _umask == other._umask 
		     &&  flags == other.flags
		     &&  count < other.count)
			return true;
		else
			return false;
	};

	bool operator == (const PerfEvent & other) const {
		if ((_select == other._select)
		&&  (_umask == other._umask))
			return true;
		else
			return false;
	};

	void setSelect(unsigned int s) { _select = s; _type = getEventTypeFromValue(s);};
	void setUmask (unsigned int u) { _umask  = u; };
	void setOs(bool b)    { (b)? flags |= PerfEvent::Os: flags &= ~PerfEvent::Os; };
	void setUsr(bool b)   { (b)? flags |= PerfEvent::Usr: flags &= ~PerfEvent::Usr; };
	void setEdge(bool b)  { (b)? flags |= PerfEvent::Edge: flags &= ~PerfEvent::Edge; };
	void setHost(bool b)  { (b)? flags |= PerfEvent::Host: flags &= ~PerfEvent::Host; };
	void setGuest(bool b) { (b)? flags |= PerfEvent::Guest: flags &= ~PerfEvent::Guest; };

	
	unsigned int select() const { return _select; };
	unsigned int umask () const { return _umask; };
	unsigned int type () const { return _type; };
	bool os() { return ((flags & PerfEvent::Os) > 0); };
	bool usr() { return ((flags & PerfEvent::Usr) > 0); };
	bool edge() { return ((flags & PerfEvent::Edge) > 0); };
	bool host() { return ((flags & PerfEvent::Host) > 0); };
	bool guest() { return ((flags & PerfEvent::Guest) > 0); };

	void dump();
	QString getEventMaskEncodeMapKey();

	static bool isPmcEvent(unsigned int value);
	static bool isIbsFetchEvent(unsigned int value);
	static bool isIbsOpEvent(unsigned int value);
	static bool isIbsEvent(unsigned int value) 
	{ return (isIbsFetchEvent(value) || isIbsOpEvent(value)); } ;

	static unsigned int getEventTypeFromValue(unsigned int value);

	QString		opName;
	QString		name;
	unsigned long	count;
	unsigned int	flags;
	unsigned int	minCount;
	unsigned long long allocMask;
	int		counter;

private:
	void setType (unsigned int t) {_type = t; };

protected: 

	unsigned int	_type;
	unsigned int	_select;
	unsigned int	_umask;
};

///////////////////////////////////////////////////////////////////////////////
class PmcEvent : public PerfEvent
{
public:
	PmcEvent() : PerfEvent() {
		_type = PerfEvent::Pmc;
	};
};

///////////////////////////////////////////////////////////////////////////////
class IbsFetchEvent : public PerfEvent
{
public:
	IbsFetchEvent() : PerfEvent() {
		_type = PerfEvent::IbsFetch;
	};
};

///////////////////////////////////////////////////////////////////////////////
class IbsOpEvent : public PerfEvent
{
public:
	enum IbsOpUmaskFlag {
		DispatchCount = 0x1,
		DcMissAddr= 0x2,
		BranchTargetAddr= 0x4,
	};

	IbsOpEvent() : PerfEvent() {
		_type = PerfEvent::IbsOp;
		_umask= IbsOpEvent::DispatchCount;
	};
};

///////////////////////////////////////////////////////////////////////////////
class NbEvent : public PerfEvent
{
public:
	NbEvent() : PerfEvent() {
		_type = PerfEvent::Nb;
	};
};

///////////////////////////////////////////////////////////////////////////////
class OProfileEvent : public PerfEvent
{
public:
	QString	counterMask;

	OProfileEvent() : PerfEvent() {
		_type = PerfEvent::OProfile;
	};
};

#endif /*_PERFEVENT_H_ */
