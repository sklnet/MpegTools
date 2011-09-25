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

// ====================================================================================

#pragma once

#include <fstream>
#include "dlgutils.h"
#include "helpdlg.h"
#include "test_ts.h"
#include "TsChecker_resource.h"

// ====================================================================================
// Variables globales
// ====================================================================================

extern HINSTANCE	hAppInstance;	//!< Handle d'instance de l'application

// ====================================================================================
// Journalisation
// ====================================================================================

class CLogStream : public CLogStreamBase
{
	class Buf : public CLogStreamBuf
	{
		HWND &		hCtl;
		tofstream &	_fout;
		LONG		bQuiet;

	public:

		Buf(HWND & hCt, tofstream & __fout);

		virtual void	Output(LPCTSTR pszTxt);
		virtual void	CtlLog(LPCTSTR pszTxt);

	}				cSBuf;

public:
	CLogStream(HWND & hCt, tofstream & __fout);

	virtual void	CheckCancelState() throw(...);

	virtual void	ProgressStep(INT nValue, INT nIndex);
};

// ====================================================================================
// Boîte de dialogue principale de l'application
// ====================================================================================

#define SC_FIL 0	//!< Identifiant pour le template de surclassement du fichier 1
#define SC_LBX 1	//!< Identifiant pour le template de surclassement de la list box

class CTsCheckerDialog :
	public WinSubClass<SC_FIL>,
	public WinSubClass<SC_LBX>,
	public CModalDialogBase
{
	/// Aide en ligne
	class HelpDialog : public CHelpDialogBase, public CModelessDialogHelper<HelpDialog, LPCDLGTEMPLATE>
	{
	public:
		HelpDialog() :
			CHelpDialogBase(_TL("PTvM TS Checker - Help","PTvM TS Checker - Aide"), IDR_RTFHELPTEXT)
		{
		}
	};

	CTestTsProcessor::Params
				sParms;
	HWND		hTstFile;
	HWND		hTstList;
	tofstream	cLog_fil;
	CLogStream	cLog;
	DWORD		dwThreadId;
	HANDLE		hThreadHdl;

public:

	enum WM_msg
	{
		WM_APP_ADDTOLIST = WM_APP+1
	};

	/// Constructeur :
	CTsCheckerDialog(CTestTsProcessor::Params & sPrms);

	void			DisplayFileAnalysis();
	void			InitFile();
	void			ShowFileInfo(UINT nId, LPCTSTR pszFileName);
	void			ShowPctInfo(UINT nId, UINT nPct);
	void			SetLogFile(LPCTSTR pszFileName);

	void			GetDroppedFiles(HDROP hDropFiles, tstring & strFile);

	void	LbCopy(HWND hCtl);
	/// Gestion des messages de la list box
	bool			LbSubProc(HWND hCtl, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void			StartChecking();
	void			StopAndWait();

	virtual LRESULT SubProc(WinSubClass<SC_FIL> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT SubProc(WinSubClass<SC_LBX> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam) {
						return LbSubProc(sc.hSWnd, uMsg, wParam, lParam) ? 0 : sc.CallWindowProc(uMsg, wParam, lParam); }

	virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	~CTsCheckerDialog() {
		StopAndWait(); }
};

// ====================================================================================
