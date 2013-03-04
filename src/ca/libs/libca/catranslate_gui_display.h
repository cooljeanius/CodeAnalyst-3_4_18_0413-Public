#ifndef _CATRANSLATE_GUI_DISPLAY_H_
#define _CATRANSLATE_GUI_DISPLAY_H_

#include <qwidget.h>
#include <qstring.h>

#include "catranslate_display.h"

class ca_translate_gui_display:public ca_translate_display
{
public:
    ca_translate_gui_display();
    ~ca_translate_gui_display();

    void init();
    void update_display(const char * text);
    void update_display_error(const char * text);
    void setDlgEventHandler(QWidget * evHandler);
    void display_oprofile_log();
    void translationInProgress(bool b);

protected:
    QWidget * m_evHandler;
};


#endif
