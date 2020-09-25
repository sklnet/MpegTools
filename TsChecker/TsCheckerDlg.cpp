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

#include "stdafx.h"
#include "TsCheckerDlg.h"
#include "filedlg.h"
#include "version.h"
#include "Hyperlinks.h"

// ====================================================================================
// Journalisation
// ====================================================================================

CLogStream::Buf::Buf(HWND & hCt, tofstream & __fout) :
	hCtl(hCt),
	_fout(__fout),
	bQuiet(false)
{
}

void CLogStream::Buf::CtlLog(LPCTSTR pszTxt)
{
	size_t	nLen		= _tcslen(pszTxt);

	if (nLen > 0 && pszTxt[nLen-1] == TCHAR('\n'))
		--nLen;

	LPTSTR	pszHeapMsg	= new TCHAR[nLen+1];

	StringCchCopyN(pszHeapMsg, nLen+1, pszTxt, nLen);
	::PostMessage(hCtl, CTsCheckerDialog::WM_APP_ADDTOLIST, 0, (LPARAM)pszHeapMsg);
}

void CLogStream::Buf::Output(LPCTSTR pszTxt)
{
	myprint(pszTxt);
	if (_fout.is_open())
		_fout << pszTxt;
	if (bFull && hCtl && !bQuiet)
		CtlLog(pszTxt);
}

// ====================================================================================

CLogStream::CLogStream(HWND & hCt, tofstream & __fout) :
	CLogStreamBase(&cSBuf),
	cSBuf(hCt, __fout)
{
}

void CLogStream::ProgressStep(INT nValue, INT nIndex)
{
	PostMessage(WM_APP_PROGRESS, nValue, (LPARAM)nIndex);
}

void CLogStream::CheckCancelState() throw(...)
{
	MSG	msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		switch (msg.message) {

		case WM_APP_ABORT:
			throw DWORD(-1);

		case WM_APP_PAUSE:
			PostMessage(WM_APP_PAUSED, 0, NULL);
			while(GetMessage(&msg, NULL, 0, 0 ) > 0) { 
				switch (msg.message) {

				case WM_APP_ABORT:
					PostMessage(WM_APP_RESUMED, 0, NULL);
					throw DWORD(-1);

				case WM_APP_PAUSE:
					PostMessage(WM_APP_RESUMED, 0, NULL);
					return;
				}
			}
		}
	}
}

// ====================================================================================
// Boîte de dialogue "À propos"
// ====================================================================================

/// Boîte de dialogue "À propos"
class CAboutDialog : public CModalDialogBase
{
	/// Traitement des messages de la boîte de dialogue
	virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	/// Constructeur :
	CAboutDialog() :
		CModalDialogBase(IDD_ABOUTBOX)
	{
	}
};

/// Traitement des messages de la boîte de dialogue
INT_PTR CAboutDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

	case WM_INITDIALOG:
		// Initialisation de la fenêtre
		ConvertStaticToHyperlink(hDlg, IDC_LICENSE_URL);
		ConvertStaticToHyperlink(hDlg, IDC_WEBSITE_URL);
		ConvertStaticToHyperlink(hDlg, IDC_ICON_AUTHOR_URL);
		ConvertStaticToHyperlink(hDlg, IDC_ICON_LICENSE_URL);
		// Ajoute la version dans la fenêtre
		TCHAR		szVersion[128];
		TCHAR		szCompTime[32];

		ReformatCompileTime(szCompTime, _countof(szCompTime), _TL(" at "," à "), __DATE__ ", " __TIME__);
		_stprintf_s(szVersion,
			_T("PTvM TS Checker ver.") TEXT(PRODUCT_VERSION_STR) EOL
			_TL("Compiled %s","Compilé %s"),
			szCompTime);
		SetDlgItemText(hDlg, IDC_VERSION, szVersion);
		return TRUE;

	case WM_COMMAND:
		// Commande reçue (clic, etc...)
		switch(LOWORD(wParam)) {

		case IDC_LICENSE_URL:
			ShellOpen(hDlg, _T("http://www.gnu.org/licenses/gpl-3.0.txt"));
			return TRUE;

		case IDC_WEBSITE_URL:
			ShellOpen(hDlg, _T("http://www.pouchintv.fr/"));
			return TRUE;

		case IDC_ICON_AUTHOR_URL:
			ShellOpen(hDlg, _T("Voir http://code.google.com/u/newmooon/"));
			return TRUE;

		case IDC_ICON_LICENSE_URL:
			ShellOpen(hDlg, _T("http://www.gnu.org/licenses/gpl.html"));
			return TRUE;

		case IDOK:
			// Clic sur le bouton OK
			EndDialog(hDlg, IDOK);
			return TRUE;

		case IDCANCEL:
			// Clic sur le bouton Annuler
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}

	// Autre message, traité par Windows
	return FALSE;
}

// ====================================================================================
// Boîte de dialogue principale de l'application
// ====================================================================================

/// Constructeur :
CTsCheckerDialog::CTsCheckerDialog(CTestTsProcessor::Params & sPrms) :
	CModalDialogBase(IDD_TSCHECKER),
	sParms(sPrms),
	hTstFile(NULL),
	hPBar(NULL),
	hTstList(NULL),
	cLog(hTstList, cLog_fil),
	dwThreadId(0),
	hThreadHdl(NULL)
{
}

void CTsCheckerDialog::ShowFileInfo(UINT nId, LPCTSTR pszFileName)
{
	WIN32_FILE_ATTRIBUTE_DATA sData;

	if (GetFileAttributesEx(pszFileName, GetFileExInfoStandard, &sData)) {
		LARGE_INTEGER	sSize = {sData.nFileSizeLow, static_cast<LONG>(sData.nFileSizeHigh)};
		TCHAR			szWrk[64];

		_stprintf_s(szWrk, _TL("Size = %s Kb","Taille = %s Ko"), thousandsSepFmt((sSize.QuadPart+0x200) / 0x400).c_str());
		SetItemText(nId, szWrk);
	}
}

/// Traitement des messages du contrôle de nom de fichier surclassé
LRESULT CTsCheckerDialog::SubProc(WinSubClass<SC_FIL> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DROPFILES) {
		vtstring vstrFiles = GetDroppedFiles(reinterpret_cast<HDROP>(wParam));

		if (!vstrFiles.empty()) {
			sParms.strTstFile = vstrFiles.front();
			InitFile();
		}
		DisplayFileAnalysis();
		return 0;
	}
	return sc.CallWindowProc(uMsg, wParam, lParam);
}

void CTsCheckerDialog::LbCopy(HWND hCtl)
{
	LRESULT		lSelCnt		= SendMessage(hCtl, LB_GETSELCOUNT, 0, NULL);
	LRESULT		lItmCnt		= SendMessage(hCtl, LB_GETCOUNT, 0, NULL);

	if (lSelCnt == LB_ERR || lItmCnt == LB_ERR)
		return;

	vector<INT>	vItms(lSelCnt ? lSelCnt : lItmCnt);

	if (vItms.empty())
		return;
	if (lSelCnt) {
		// Une sélection existe : on copie les éléments sélectionnés
		if (SendMessage(hCtl, LB_GETSELITEMS, vItms.size(), (LPARAM)&vItms[0]) == LB_ERR)
			return;
	} else {
		// Aucune sélection : on prend tout le contenu
		for (ITERATE_VECTOR(vItms, INT, it))
			*it = (INT) lSelCnt++;
	}

	UINT nTxtLen = 0;	// Longueur total calculée

	// 1ère passe : déterminer la longueur
	for (ITERATE_CONST_VECTOR(vItms, INT, it)) {
		LRESULT lLen = SendMessage(hCtl, LB_GETTEXTLEN, *it, NULL);

		if (lLen != LB_ERR)
			nTxtLen += (UINT) (lLen + 2);
	}

	// Allouer un objet global de cette longueur
	HGLOBAL hGlbCopy = GlobalAlloc(GMEM_MOVEABLE, (nTxtLen+1) * sizeof(TCHAR));

	if (!hGlbCopy)
		return;

	LPTSTR  pszCopy = (LPTSTR)GlobalLock(hGlbCopy);
	LPTSTR	pszEnd  = pszCopy + nTxtLen +1;

	// Copier toutes les lignes sélectionnées dans l'objet global
	for (ITERATE_CONST_VECTOR(vItms, INT, it)) {
		LRESULT lLen = SendMessage(hCtl, LB_GETTEXT, *it, (LPARAM)pszCopy);

		if (lLen != LB_ERR) {
			pszCopy += lLen;
			_tcscat_s(pszCopy, pszEnd-pszCopy, _T("\r\n"));
			pszCopy += 2;
		}
	}
	GlobalUnlock(hGlbCopy);

	// Transférer dans le presse-papier
	if (OpenClipboard(hCtl)) {
		EmptyClipboard();
	#ifdef _UNICODE
		SetClipboardData(CF_UNICODETEXT, hGlbCopy);
	#else
		SetClipboardData(CF_TEXT, hGlbCopy);
	#endif
		CloseClipboard();
	}
}

bool CTsCheckerDialog::LbSubProc(HWND hCtl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

	case WM_APP_ADDTOLIST: {
		LPCTSTR pszTxt = (LPCTSTR)lParam;

		AddLineToList(hCtl, pszTxt);
		delete [] pszTxt;
		return true; }

	case WM_COMMAND:
		switch(LOWORD(wParam)) {

		case IDC_COPY:
			LbCopy(hCtl);
			return true;

		case IDC_SELECTALL:
			// Clic sur le bouton OK
			SendMessage(hCtl, LB_SELITEMRANGEEX, 0, (LPARAM)SendMessage(hCtl, LB_GETCOUNT, 0, NULL));
			return true;
		}
		return true;

	case WM_CONTEXTMENU: {
		HMENU hPopMenu = CreatePopupMenu();

		AppendMenu(hPopMenu, 0, IDC_COPY, _TL("Copy\tCtrl-C","Copier\tCtrl-C"));
		AppendMenu(hPopMenu, 0, IDC_SELECTALL, _TL("Select All\tCtrl-A","Tout sélectionner\tCtrl-A"));
		TrackPopupMenuEx(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hCtl, NULL);
		DestroyMenu(hPopMenu);
		return true; }

	case WM_KEYDOWN:
		if (GetKeyState(VK_CONTROL) < 1) {
			switch (wParam) {

			case 'A':
				SendMessage(hCtl, LB_SELITEMRANGEEX, 0, (LPARAM)SendMessage(hCtl, LB_GETCOUNT, 0, NULL));
				break;

			case 'C':
				LbCopy(hCtl);
				break;
			}
		}
		return true;
	}
	return false;
}

void CTsCheckerDialog::PbPaintOvl(HWND hCtl, UINT uMsg)
{
	if (uMsg != WM_PAINT)
		return;

	TCHAR			szPct[8];
	static HFONT	hFont	= static_cast<HFONT>(GetStockObject(ANSI_VAR_FONT)); 
	UINT			nPos	= static_cast<UINT>(SendMessage(hCtl, PBM_GETPOS, 0, NULL));

	_stprintf_s(szPct, _T("%u %%"), nPos);

	InvalidateRect(hCtl, NULL, FALSE);

	PAINTSTRUCT ps;
	HDC			hDC			= BeginPaint(hCtl, &ps);
	HFONT		hOldFont	= static_cast<HFONT>(SelectObject(hDC, hFont)); // Retrieve a handle to the variable stock font

	// Select the variable stock font into the specified device context. 
	if (hOldFont = static_cast<HFONT>(SelectObject(hDC, hFont))) {
		SetBkMode(hDC, TRANSPARENT);
		DrawTextEx(hDC, szPct, (int)_tcslen(szPct), &ps.rcPaint, DT_CENTER|DT_SINGLELINE|DT_VCENTER, NULL);

		// Restore the original font.        
		SelectObject(hDC, hOldFont); 
	}
	EndPaint(hCtl, &ps);
}

void CTsCheckerDialog::SetLogFile(LPCTSTR pszFileName)
{
	if (cLog_fil.is_open())
		cLog_fil.close();
	if (pszFileName)
		cLog_fil.open(pszFileName);
}

void CTsCheckerDialog::DisplayFileAnalysis()
{
	if (!sParms.strTstFile.empty()) {
		SendMessage(hTstList, LB_RESETCONTENT, 0, NULL);

		CTsFileAnalyzer(sParms.strTstFile.c_str(),
			tstring(_TL("file","fichier"))).Analyze().Output(cLog, _TL("Information about this ","Informations concernant ce "));

		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);
	}
}

void CTsCheckerDialog::InitFile()
{
	if (!sParms.strTstFile.empty()) {
		size_t	nLen		= sParms.strTstFile.length();

		set_text(hTstFile, sParms.strTstFile.c_str());
		SendMessage(hTstFile, EM_SETSEL, nLen, nLen);	// Montrer la fin du nom de fichier
		EnableDlgItem(IDC_START, true);
		ShowFileInfo(IDC_INFO, sParms.strTstFile.c_str());
	}
}

void CTsCheckerDialog::StartChecking()
{
	myprint(CLS);
	cLog_fil.clear();

	SetLogFile(ChangeExtension(sParms.strTstFile.c_str(), _T(".txt")).c_str());

	cLog_fil << _TL(
		"## File = ",
		"## Fichier = ") << sParms.strTstFile << endl;

	CTsFileAnalyzer(sParms.strTstFile.c_str(), tstring(_TL("file","fichier"))).Analyze().Output(cLog_fil);

	cLog_fil << _T("---------------------------------------------------------------------------------------------------") << endl;

	SendMessage(hTstList, LB_RESETCONTENT, 0, NULL);
	WorkingThread *	pThd =
		new CTestTsProcessor(cLog, sParms, true);

	if (pThd) {
		pThd->StartThread();
		dwThreadId = pThd->getThreadId();
		hThreadHdl = pThd->getThreadHdl();
	}
}

void CTsCheckerDialog::StopAndWait()
{
	if (dwThreadId) {
		// Requérir l'arrêt si traitement en cours
		PostThreadMessage(dwThreadId, WM_APP_ABORT, 0, 0);
		// Attendre (5 secondes maximum) que le thread ait quitté
		if (hThreadHdl != NULL)
			WaitForSingleObject(hThreadHdl, 5000);
	}
}

INT_PTR CTsCheckerDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static INT a_tabs[] = {16,56,96,106}; // Positions des tabulations dans la list box

	switch (uMsg) {

	case WM_INITDIALOG: {
	#if USE_CONSOLE
		startConsole(_T("Fenêtre de diagnostic de TsChecker"), _T("TsChecker.log"));
	#endif
		// Initialisation de la fenêtre
		LONG	nScrW = GetSystemMetrics(SM_CXFULLSCREEN);
		LONG	nScrH = GetSystemMetrics(SM_CYFULLSCREEN);
		RECT	awRect;	// Rectangle de la fenêtre à aligner

		SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_TSCHECKER)));

		hTstFile	= GetDlgItem(hDlg, IDC_FILE);
		hPBar		= GetDlgItem(hDlg, IDC_PROGRESS);
		hTstList	= GetDlgItem(hDlg, IDC_MSGLIST);

		// Définir les tabulations dans la list box :
		SendMessage(hTstList, LB_SETTABSTOPS, (WPARAM)_countof(a_tabs), (LPARAM)a_tabs);

		WinSubClass<SC_FIL>::SubClass(hTstFile);
		WinSubClass<SC_PGB>::SubClass(hPBar);
		WinSubClass<SC_LBX>::SubClass(hTstList);

		DragAcceptFiles(hTstFile, TRUE);

		GetWindowRect(hDlg, &awRect);
		SetWindowPos(hDlg, NULL, (nScrW-RWdt(awRect))/2, (nScrH-RHgt(awRect))/2, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

		//// Initialisation des barres de progression
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);

		cLog.hWnd = hDlg;
		InitFile();
		DisplayFileAnalysis();

		set_check(hDlg, IDC_VIDEOREDO_PRJ, sParms.bMakeVrdPrj);

		if (sParms.bBatch && !sParms.strTstFile.empty())
			StartChecking();

		// Renvoie True car la fenêtre a été initialisée
		return TRUE; }

	case WM_COMMAND:
		// Commande reçue (clic, etc...)
		switch(wParam) {

		case _cmd_(IDC_BROWSE, BN_CLICKED): {
			tstring strNew =
						COpenFileNameDialog().Do(
							hDlg, NULL,
							_TL("Mpeg2 TS video\0*.TS\0","Vidéo Mpeg2 TS\0*.TS\0")
							_TL("All files\0*.*\0","Tous les fichiers\0*.*\0"),
							NULL);
			if (!strNew.empty()) {
				sParms.strTstFile = strNew;
				InitFile();
				DisplayFileAnalysis();
			}
			return TRUE; }

		case _cmd_(IDC_START, BN_CLICKED): {
			if (!sParms.strTstFile.empty()) {
				sParms.bMakeVrdPrj = get_check(hDlg, IDC_VIDEOREDO_PRJ);

				StartChecking();
			}
			return TRUE; }

		case _cmd_(IDC_VIDEOREDO_PRJ, BN_CLICKED):
			sParms.bMakeVrdPrj = get_check(hDlg, IDC_VIDEOREDO_PRJ);
			return TRUE;

		case _cmd_(IDC_HELPBOX, BN_CLICKED):
			HelpDialog::ShowDialog(hDlg);
			return TRUE;

		case _cmd_(IDC_ABOUTBOX, BN_CLICKED):
			CAboutDialog().Do(hDlg);
			return TRUE;

		case _cmd_(IDC_PAUSE, BN_CLICKED):
			// Clic sur le bouton Annuler
			if (dwThreadId) {
				EnableDlgItem(IDC_PAUSE, false);
				PostThreadMessage(dwThreadId, WM_APP_PAUSE, 0, 0);
			}
			return TRUE;

		case _cmd_(IDOK, BN_CLICKED):
		case _cmd_(IDCLOSE, BN_CLICKED):
			// Clic sur le bouton OK
			EndDialog(hDlg, 1);
			return TRUE;

		case _cmd_(IDCANCEL, BN_CLICKED):
			// Clic sur le bouton Annuler
			if (dwThreadId)
				PostThreadMessage(dwThreadId, WM_APP_ABORT, 0, 0);
			return TRUE;
		}
		break;

	case WM_SYSCOMMAND:
		switch(LOWORD(wParam)) {

		case SC_CLOSE:
			// Clic sur la case de fermeture
			EndDialog(hDlg, 1);
			return TRUE;
		}
		break;

	case WM_APP_PROCESS_BEG:
		EnableDlgItem(IDC_BROWSE, false);
		EnableDlgItem(IDC_START, false);
		EnableDlgItem(IDC_PAUSE, true);
		EnableDlgItem(IDCANCEL, true);
		DragAcceptFiles(hTstFile, FALSE);
		return TRUE;

	case WM_APP_PROGRESS:
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, wParam, 0);
		return TRUE;

	case WM_APP_PROCESS_END:
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, 100, 0);
		EnableDlgItem(IDC_BROWSE, true);
		EnableDlgItem(IDC_START, true);
		EnableDlgItem(IDC_PAUSE, false);
		EnableDlgItem(IDCANCEL, false);
		DragAcceptFiles(hTstFile, TRUE);
		SetLogFile(NULL);
		ShowFileInfo(IDC_INFO, sParms.strTstFile.c_str());
		if (sParms.bBatch)
			PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCLOSE, BN_CLICKED), NULL);
		hThreadHdl = NULL;
		return TRUE;

	case WM_APP_PAUSED:
		SetItemText(IDC_PAUSE, _TL("Resume","Reprendre"));
		EnableDlgItem(IDC_PAUSE, true);
		return TRUE;

	case WM_APP_RESUMED:
		SetItemText(IDC_PAUSE, _TL("Pause","Pause"));
		EnableDlgItem(IDC_PAUSE, true);
		return TRUE;

	case WM_DESTROY:
		StopAndWait();
		return TRUE;

	}
	return FALSE;
}

// ====================================================================================
