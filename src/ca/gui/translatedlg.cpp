//$Id: translatedlg.cpp,v 1.3 2005/02/14 16:52:29 jyeh Exp $

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


#include <qapplication.h>
#include <QCloseEvent>

#include "translatedlg.h"
#include "stdafx.h"

TranslateDlg::TranslateDlg (QWidget * parent, Qt::WindowFlags fl)
:QDialog (parent, fl|CA_DLG_FLAGS)
{
	setupUi(this);
	canBeClosed = true;
	setFont( qApp->font() ); 
}

TranslateDlg::~TranslateDlg ()
{
}

void TranslateDlg::closeEvent ( QCloseEvent * e )
{
	if(canBeClosed)
	{
		QDialog::close (e);
	}
}

void TranslateDlg::reject()
{
	if(canBeClosed)
	{
		QDialog::reject();
	}
}
