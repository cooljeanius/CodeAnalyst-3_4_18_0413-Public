//$Id: colorDlg.h 20128 2011-08-11 12:09:44Z sjeganat $
/*
// CodeAnalyst for Open Source
// Copyright 2006 Advanced Micro Devices, Inc.
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
#ifndef COLOR_DLG_H
#define COLOR_DLG_H

#include "IColorDlg.h"
#include "atuneoptions.h"


class ColorDlg : public QDialog, public Ui::IColorDlg
{
	Q_OBJECT

public:
	ColorDlg( QStringList labels, ColorList *pColors, QWidget* parent = 0,
		Qt::WindowFlags fl = 0);

	~ColorDlg();

private slots:
	void onOk ();
	void onApply ();
	void onColorBrowse ();
	void onLabelChanged (int index);

private:
	void updateColorButton (QColor color);

private:
	bool m_modified;
	ColorList * m_pColorList;
	ColorList m_tempColors;
};
#endif
