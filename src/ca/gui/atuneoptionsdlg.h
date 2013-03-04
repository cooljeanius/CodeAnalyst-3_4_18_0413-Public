//$Id: atuneoptionsdlg.h,v 1.3 2006/05/15 22:09:22 jyeh Exp $
//interface for the CATuneOptionsDlg class.

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

#ifndef _ATUNEOPTIONSDLG_H
#define _ATUNEOPTIONSDLG_H

#include <qpushbutton.h>

#include "stdafx.h"
#include "atuneoptions.h"

//generated from iatuneoptionsdlg.ui
#include "iatuneoptionsdlg.h"

// All numbers are from Oprofile 0.9.3
enum CAtuneOptionsDlgEnumType
{
	// From libop/op_config_24.h in 
	OP_MIN_PRE_WATERMARK = 8192,

	/** maximum number of entry in samples eviction buffer */
	OP_MAX_BUF_SIZE = 4194304,

	/** minimum number of entry in samples eviction buffer */
	OP_MIN_BUF_SIZE = 40960,

	// From libop/op_config.h
	OP_MIN_CPU_BUF_SIZE = 2048,

	OP_MAX_CPU_BUF_SIZE = 131072
};

enum OP_PLUG_IN
{
	OP_091,
};

class ElementPushButton : public QPushButton
{
	Q_OBJECT
public:
	ElementPushButton (const QString & text, QWidget * parent,
		const char * name = 0);
	void setCpu ( int cpu);
	void setEvent (int event);

signals:
	void pushClicked ( int cpu, int event );

private slots:
	void onClicked ();

private:
	int m_cpu;
	int m_event;
};


class CATuneOptionsDlg  : public QDialog, public Ui::IATuneOptionsDlg
{
	Q_OBJECT
public:
	CATuneOptionsDlg ( QWidget* parent, Qt::WindowFlags fl = 0);
	virtual ~CATuneOptionsDlg();
signals:
	void fontChanged();
public slots:
	// overrides
	void onApply();
	void onOk();
	void onProjectDirectoryBrowse();
	bool validateOptions();

private:
	void initializeOptions();
	void saveOptions();
	void initializePluginSelection(CATuneOptions & ao);
	void initializeSearchPaths();
	void saveSearchPaths (CATuneOptions &ao);

private slots:
	//Todo: Do this with arrays instead...

private:
	//related to dynamic creation
	QLineEdit * m_src_dirs; 
};

#endif // #ifndef _ATUNEOPTIONSDLG_H
