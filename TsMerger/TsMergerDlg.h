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
#include "merge_ts.h"
#include "TsMerger_resource.h"

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

		void			SetQuiet(bool bQt);

	}				cSBuf;

	virtual void	log_full(bool bOn) {
						flush(); cSBuf.bFull = bOn; }
public:
	CLogStream(HWND & hCt, tofstream & __fout);

	void			SetQuiet(bool bQt) {
						cSBuf.SetQuiet(bQt); }

	virtual void	CheckCancelState() throw(...);

	virtual void	ProgressStep(INT nValue, INT nIndex);

	void			CtlLog(LPCTSTR pszTxt) {
						cSBuf.CtlLog(pszTxt); }
};

// ====================================================================================
// Boîte de dialogue principale de l'application
// ====================================================================================

#define SC_FIL1 ID_FIL1			//!< Identifiant pour le template de surclassement du fichier 1
#define SC_FIL2 ID_FIL2			//!< Identifiant pour le template de surclassement du fichier 2
#define SC_LBX1 ID_FIL1+0x10	//!< Identifiant pour le template de surclassement de la liste 1
#define SC_LBX2 ID_FIL2+0x10	//!< Identifiant pour le template de surclassement de la liste 2

class CTsMergerDialog :
	public WinSubClass<SC_FIL1>,
	public WinSubClass<SC_FIL2>,
	public WinSubClass<SC_LBX1>,
	public WinSubClass<SC_LBX2>,
	public CModalDialogBase
{
	/// Aide en ligne
	class HelpDialog : public CHelpDialogBase, public CModelessDialogHelper<HelpDialog, LPCDLGTEMPLATE>
	{
	public:
		HelpDialog() :
			CHelpDialogBase(_TL("PTvM TS Merger - Help","PTvM TS Merger - Aide"), IDR_RTFHELPTEXT)
		{
		}
	};

	CMergeTsProcessor::Params
				sParms;
	HWND		hLeftFile;
	HWND		hRightFile;
	HWND		hLeftList;
	HWND		hRightList;
	tofstream	cLog_fil;
	CLogStream	cLog_merge;
	CLogStream	cLog_out;
	DWORD		dwThreadId;
	HANDLE		hThreadHdl;
	WORD		pcr_pid;

public:

	enum WM_msg
	{
		WM_APP_ADDTOLIST = WM_APP+1
	};

	/// Constructeur :
	CTsMergerDialog(CMergeTsProcessor::Params & sPrms);

	void			DisplayFileAnalysis();
	void			InitFile(HWND hCtl, UINT nInfoId, LPCTSTR pszFileName, LPCTSTR pszOtherFileName);
	void			InitFile1() {
						InitFile(hLeftFile, IDC_INFO1, sParms.strSrcFile1.c_str(), sParms.strSrcFile2.c_str()); }
	void			InitFile2() {
						InitFile(hRightFile, IDC_INFO2, sParms.strSrcFile2.c_str(), sParms.strSrcFile1.c_str()); }

	void			ShowFileInfo(UINT nId, LPCTSTR pszFileName);
	void			ShowPctInfo(UINT nId, UINT nPct);
	void			SetLogFile(LPCTSTR pszFileName);

	void			GetDroppedFiles(HDROP hDropFiles, tstring & strFile1, tstring & strFile2);

	void			LbCopy(HWND hCtl);

	/// Gestion des messages des list-boxes
	bool			LbSubProc(HWND hCtl, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void			StartMerging();
	void			StopAndWait();

	virtual LRESULT SubProc(WinSubClass<SC_FIL1> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT SubProc(WinSubClass<SC_FIL2> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual LRESULT SubProc(WinSubClass<SC_LBX1> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam) {
						return LbSubProc(sc.hSWnd, uMsg, wParam, lParam) ? 0 : sc.CallWindowProc(uMsg, wParam, lParam); }
	virtual LRESULT SubProc(WinSubClass<SC_LBX2> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam) {
						return LbSubProc(sc.hSWnd, uMsg, wParam, lParam) ? 0 : sc.CallWindowProc(uMsg, wParam, lParam); }

	virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	~CTsMergerDialog() {
		StopAndWait(); }
};

// ====================================================================================
