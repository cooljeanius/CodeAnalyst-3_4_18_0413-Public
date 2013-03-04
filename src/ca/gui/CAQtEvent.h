#include <QEvent>

class CAQtEvent : public QEvent
{
public:

	CAQtEvent(QEvent::Type type, void *data) : QEvent(type), m_pData(data)
	{ 
    	}

	~CAQtEvent() { }

	void * data() { return m_pData; }

private:
	void *m_pData;
};

