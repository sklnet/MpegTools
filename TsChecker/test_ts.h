/*
 * $Id$
 * Copyright (C) 2011 gingko - http://gingko.homeip.net/
 *
 * This file is part of Pouchin TV Mod, a free DVB-T viewer.
 * See http://www.pouchintv.fr/ for updates.
 *
 * Pouchin TV Mod is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Pouchin TV Mod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

// ====================================================================================

#include <iostream>
#include "mapping.h"
#include "cmdline.h"
#include "../common/cc_checker.h"
#include "../common/vredo_prj.h"
#include "../common/ts_file_read.h"

using namespace std;

// ====================================================================================

#define REQ_MAP_SIZE	0x80000

#define READ_MAP_SIZE	0x200000
#define WRITE_MAP_SIZE	0x200000

// ====================================================================================

typedef basic_ostream<TCHAR>	tostream;

enum WM_APP_Messages
{
	WM_APP_FIRST = WM_APP,
	WM_APP_PROCESS_BEG,
	WM_APP_PROCESS_END,
	WM_APP_PROGRESS,
	WM_APP_ABORT,
	WM_APP_PAUSE,
	WM_APP_PAUSED,
	WM_APP_RESUMED
};

// ====================================================================================

class CTestTsProcessor : public CTsFileReader, public WorkingThread
{
	friend class CTsFileReader;

	CLogStreamBase &	cLog;
	TimeMarkVect		vTimeMarks;
	bool				bEof;
	bool &				bMakeVrdPrj;

	virtual DWORD	ThreadProc();

	void			Process() throw(...);

	virtual void	ProgressStep(INT nValue) {
						cLog.ProgressStep(nValue, -1); }

public:

	struct Params
	{
		tstring	strTstFile;		//!< Nom du fichier à tester
		bool	bBatch;
		bool	bMakeVrdPrj;	//!< \p true si fichier projet VideoRedo à générer (peut être modifié en cours de traitement)

		Params() :
			bMakeVrdPrj(false),
			bBatch(false)
		{
		}
	};

	CTestTsProcessor(CLogStreamBase & cLg,
		Params & sPrms, bool bDelOnExit);

	~CTestTsProcessor() NOEXCEPT {
		}
};

// ====================================================================================
// Paramètres en ligne de commande
// ====================================================================================

class CTsCheckerCmdLine : public CCmdLine, public CTestTsProcessor::Params
{
	/// Méthode virtuelle appelée par \p ProcessAll
	virtual bool	ProcessArg(tstring & strArg);
public:
	/// Constructeur avec les paramètres de la fonction 'tmain'
	CTsCheckerCmdLine(int argc, LPTSTR argv[]) :
		CCmdLine(argc, argv)
	{
	}

	/// Constructeur avec une ligne de commande brute
	CTsCheckerCmdLine(LPTSTR pszCmdLine) :
		CCmdLine(pszCmdLine)
	{
	}
};

// ====================================================================================
