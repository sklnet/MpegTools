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

#include "stdafx.h"
#include "TsMergerDlg.h"

#ifdef _CONSOLE
	#include "filedlg.h"
	#include "merge_ts.h"
	#include <strstream>
	#include <sstream>
	#include <fstream>
#endif	// #ifdef _CONSOLE

// ====================================================================================
// Variables globales
// ====================================================================================

HINSTANCE	hAppInstance = NULL;		//!< Handle d'instance de l'application

// ====================================================================================
// Journalisation (Mode commande)
// ====================================================================================

#ifdef _CONSOLE

class CLogStreamCmd : public CLogStreamBase
{
	class Buf : public CLogStreamBuf
	{
		virtual void	Output(LPCTSTR pszTxt) {
							tcout << pszTxt; }
	public:
	}		cSBuf;

public:
	CLogStreamCmd() :
		CLogStreamBase(&cSBuf)
	{
	}

	//virtual void	ProgressStep(INT nValue, INT nIndex) {
	//					}

	//virtual void	CheckCancelState() throw(...) {
	//					}
};

#endif	// #ifdef _CONSOLE

// ====================================================================================
// Point d'entrée du programme (Mode commande)
// ====================================================================================

#ifdef _CONSOLE

int _tmain(int argc, _TCHAR* argv[])
{
	CLogStreamCmd		cLog_out;
	CTsMergerCmdLine	cCmdLine(argc, argv);

	cCmdLine.ProcessAll();

	//tstring		strSrcFile1, strSrcFile2, strDstFile;

	cCmdLine.strSrcFile1 =
		COpenFileNameDialog().Do(
			NULL, NULL,
			_TL("Mpeg2 TS video\0*.TS\0","Vidéo Mpeg2 TS\0*.TS\0")
			_TL("All files\0*.*\0","Tous les fichiers\0*.*\0"),
			NULL);
	if (cCmdLine.strSrcFile1.empty())
		return 1;

	for (;;) {
		cCmdLine.strSrcFile2 =
			COpenFileNameDialog().Do(
				NULL, NULL,
				_TL("Mpeg2 TS video\0*.TS\0","Vidéo Mpeg2 TS\0*.TS\0")
				_TL("All files\0*.*\0","Tous les fichiers\0*.*\0"),
				NULL);
		if (cCmdLine.strSrcFile2.empty())
			return 1;
		if (cCmdLine.strSrcFile2 != cCmdLine.strSrcFile1)
			break;
		MessageBox(NULL,
			_TL("This file is already set as first file",
				"Ce fichier est déjà spécifié comme premier fichier"),
			_TL("File Error","Erreur de fichier"), MB_ICONERROR|MB_OK);
	}

	tstring strSuggest = cCmdLine.SuggestDstName();

	cCmdLine.strDstFile = CSaveFileNameDialog().Do(NULL, NULL,
		_TL("Mpeg2 TS video\0*.TS\0","Vidéo Mpeg2 TS\0*.TS\0")
		_TL("All files\0*.*\0","Tous les fichiers\0*.*\0"),
		strSuggest.c_str());

	if (cCmdLine.strDstFile.empty())
		return 1;

	CMergeTsProcessor cThread(cLog_out, cLog_out, cCmdLine, NO_PID, false);

	bool bStarted = cThread.StartThread();

	cThread.WaitThreadExit(INFINITE);

	return 0;
}

#endif	// #ifdef _CONSOLE

// ====================================================================================
// Point d'entrée du programme (Interface graphique)
// ====================================================================================

#ifndef _CONSOLE

int APIENTRY _tWinMain(	HINSTANCE	hInstance,
						HINSTANCE	hPrevInstance,
						LPTSTR		lpCmdLine,
						int			nCmdShow)
{
	hAppInstance = hInstance;

	INITCOMMONCONTROLSEX icx = {
		sizeof(icx),
		ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS
	};

	InitCommonControlsEx(&icx);

	CTsMergerCmdLine	cCmdLine(GetCommandLine());

	cCmdLine.ProcessAll();

	CTsMergerDialog		cTsMergerDlg(cCmdLine);
	INT_PTR				nResult = cTsMergerDlg.Do(NULL);	// Ouverture du dialogue

	if (nResult < 0) {
		DWORD nDwRes = GetLastError();

		nDwRes = nDwRes;
	}

	return 0;
}
#endif	// #ifndef _CONSOLE
