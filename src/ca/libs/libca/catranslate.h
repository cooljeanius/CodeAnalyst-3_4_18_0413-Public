//$Id: catranslate.h,v 1.2 2006/05/15 22:09:45 jyeh Exp $/*{*/

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2005 Advanced Micro Devices, Inc.
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

#ifndef _CATRANSLATE_H_
#define _CATRANSLATE_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <fstream>

#include "../../include/stdafx.h"
#include "catranslate_display.h"

using namespace std;

class CATranslate {
public:
    CATranslate();

    virtual ~CATranslate ();

    virtual void init( QString & op_data_dir,
			unsigned int ncpu,
			QString & caSession,
			EventMaskEncodeMap & events_map,
			ca_translate_display *display);

    bool translate_op_to_ca (unsigned long family,
				unsigned long model,
				QStringList taskFilter,
				QString output_Name,
				bool useXML = false);

protected:
    ca_translate_display * m_pStatusDisplay;

private:
    /**  
     * op_events contains strings of event that will set as
     * profile_spec.  It must match the name oprofile uses.
     */
    EventMaskEncodeMap m_op_events_map;
    
    QString m_casession;
    QString m_op_data_dir;

    unsigned int m_ncpu;
};



#endif
