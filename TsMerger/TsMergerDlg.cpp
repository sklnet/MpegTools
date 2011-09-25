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
#include "TsMergerDlg.h"
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
	::PostMessage(hCtl, CTsMergerDialog::WM_APP_ADDTOLIST, 0, (LPARAM)pszHeapMsg);
}

void CLogStream::Buf::Output(LPCTSTR pszTxt)
{
	myprint(pszTxt);
	if (_fout.is_open())
		_fout << pszTxt;
	if (bFull && hCtl && !bQuiet)
		CtlLog(pszTxt);
}

void CLogStream::Buf::SetQuiet(bool bQt)
{
	if (bQt) {
		if (!InterlockedExchange(&bQuiet, true)) {
			Sleep(100);
			CtlLog(_TL("## Logging suspended","## Suspension de la journalisation"));
		}
	} else {
		if (bQuiet) {
			CtlLog(_TL("## Logging resumed","## Reprise de la journalisation"));
			Sleep(100);
		}
		InterlockedExchange(&bQuiet, false);
	}
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
// Bo�te de dialogue "� propos"
// ====================================================================================

/// Bo�te de dialogue "� propos"
class CAboutDialog : public CModalDialogBase
{
	/// Traitement des messages de la bo�te de dialogue
	virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	/// Constructeur :
	CAboutDialog() :
		CModalDialogBase(IDD_ABOUTBOX)
	{
	}
};

/// Traitement des messages de la bo�te de dialogue
INT_PTR CAboutDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

	case WM_INITDIALOG:
		// Initialisation de la fen�tre
		ConvertStaticToHyperlink(hDlg, IDC_LICENSE_URL);
		ConvertStaticToHyperlink(hDlg, IDC_WEBSITE_URL);
		ConvertStaticToHyperlink(hDlg, IDC_ICON_AUTHOR_URL);
		ConvertStaticToHyperlink(hDlg, IDC_ICON_LICENSE_URL);
		// Ajoute la version dans la fen�tre
		TCHAR		szVersion[128];
		TCHAR		szCompTime[32];

		ReformatCompileTime(szCompTime, _countof(szCompTime), _TL(" at "," � "), __DATE__ ", " __TIME__);
		_stprintf_s(szVersion,
			_T("PTvM TS Merger ver.") TEXT(PRODUCT_VERSION_STR) EOL
			_TL("Compiled %s","Compil� %s"),
			szCompTime);
		SetDlgItemText(hDlg, IDC_VERSION, szVersion);
		return TRUE;

	case WM_COMMAND:
		// Commande re�ue (clic, etc...)
		switch(LOWORD(wParam)) {

		case IDC_LICENSE_URL:
			ShellOpen(hDlg, _T("http://www.gnu.org/licenses/gpl-3.0.txt"));
			return TRUE;

		case IDC_WEBSITE_URL:
			ShellOpen(hDlg, _T("http://www.pouchintv.fr/"));
			return TRUE;

		case IDC_ICON_AUTHOR_URL:
			ShellOpen(hDlg, _T("http://lopagof.deviantart.com/"));
			return TRUE;

		case IDC_ICON_LICENSE_URL:
			ShellOpen(hDlg, _T("http://creativecommons.org/licenses/by-nc-nd/3.0/deed.") _TL("en","fr"));
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

	// Autre message, trait� par Windows
	return FALSE;
}

// ====================================================================================
// Bo�te de dialogue principale de l'application
// ====================================================================================

/// Constructeur :
CTsMergerDialog::CTsMergerDialog(CMergeTsProcessor::Params & sPrms) :
	CModalDialogBase(IDD_TSMERGER),
	sParms(sPrms),
	hLeftFile(NULL),
	hRightFile(NULL),
	hLeftList(NULL),
	hRightList(NULL),
	cLog_merge(hLeftList, cLog_fil),
	cLog_out(hRightList, cLog_fil),
	dwThreadId(0),
	hThreadHdl(NULL),
	pcr_pid(NO_PID)
{
}

void CTsMergerDialog::ShowPctInfo(UINT nId, UINT nPct)
{
	TCHAR	szWrk[8];

	_stprintf_s(szWrk, _T("%u %%"), nPct);
	SetItemText(nId, szWrk);
}

void CTsMergerDialog::ShowFileInfo(UINT nId, LPCTSTR pszFileName)
{
	WIN32_FILE_ATTRIBUTE_DATA sData;

	if (GetFileAttributesEx(pszFileName, GetFileExInfoStandard, &sData)) {
		LARGE_INTEGER	sSize = {sData.nFileSizeLow, sData.nFileSizeHigh};
		TCHAR			szWrk[64];

		_stprintf_s(szWrk, _TL("Size = %s Kb","Taille = %s Ko"), thousandsSepFmt((sSize.QuadPart+0x200) / 0x400).c_str());
		SetItemText(nId, szWrk);
	}
}

void CTsMergerDialog::GetDroppedFiles(HDROP hDropFiles, tstring & strFile1, tstring & strFile2)
{
	TCHAR	szFile[MAX_PATH];

	// on r�cup�re le nombre de fichiers
	UINT	nCount = DragQueryFile(hDropFiles, 0xffffffff, NULL, 0);
	UINT	nInx;

	for (nInx = 0; nInx < nCount; ++nInx) {
		DragQueryFile(hDropFiles, nInx, szFile, _countof(szFile));
		if (_tcsicmp(PathFindExtension(szFile), TEXT(".ts")) == 0) {
			strFile1 = szFile;
			break;
		}
	}

	for (++nInx; nInx < nCount; ++nInx) {
		DragQueryFile(hDropFiles, nInx, szFile, _countof(szFile));
		if (_tcsicmp(PathFindExtension(szFile), TEXT(".ts")) == 0) {
			strFile2 = szFile;
			break;
		}
	}

	// on a fini de r�cup�rer les fichiers
	// on doit le signaler au syst�me d'exploitation
	// par la fonction suivante :
	DragFinish(hDropFiles);

	// Vider le nom du fichier de sortie par d�faut
	sParms.strDstFile.clear();
}

/// Traitement des messages du contr�le de nom de fichier 1 surclass�
LRESULT CTsMergerDialog::SubProc(WinSubClass<SC_FIL1> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DROPFILES) {
		tstring strFile1, strFile2;

		GetDroppedFiles((HDROP)wParam, strFile1, strFile2);
		if (!strFile1.empty()) {
			if (!strFile2.empty()) {
				sParms.strSrcFile1 = strFile1;
				sParms.strSrcFile2 = strFile2;
				InitFile1();
				InitFile2();
			} else if (strFile1 != sParms.strSrcFile2) {
				sParms.strSrcFile1 = strFile1;
				InitFile1();
			}
		}
		DisplayFileAnalysis();
		return 0;
	}
	return sc.CallWindowProc(uMsg, wParam, lParam);
}

/// Traitement des messages du contr�le de nom de fichier 2 surclass�
LRESULT CTsMergerDialog::SubProc(WinSubClass<SC_FIL2> & sc, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DROPFILES) {
		tstring strFile1, strFile2;

		GetDroppedFiles((HDROP)wParam, strFile1, strFile2);
		if (!strFile1.empty()) {
			if (!strFile2.empty()) {
				sParms.strSrcFile2 = strFile1;
				sParms.strSrcFile1 = strFile2;
				InitFile1();
				InitFile2();
			} else if (strFile1 != sParms.strSrcFile1) {
				sParms.strSrcFile2 = strFile1;
				InitFile2();
			}
		}
		DisplayFileAnalysis();
		return 0;
	}
	return sc.CallWindowProc(uMsg, wParam, lParam);
}

void CTsMergerDialog::LbCopy(HWND hCtl)
{
	LRESULT		lSelCnt		= SendMessage(hCtl, LB_GETSELCOUNT, 0, NULL);
	LRESULT		lItmCnt		= SendMessage(hCtl, LB_GETCOUNT, 0, NULL);

	if (lSelCnt == LB_ERR || lItmCnt == LB_ERR)
		return;

	vector<INT>	vItms(lSelCnt ? lSelCnt : lItmCnt);

	if (vItms.empty())
		return;
	if (lSelCnt) {
		// Une s�lection existe : on copie les �l�ments s�lectionn�s
		if (SendMessage(hCtl, LB_GETSELITEMS, vItms.size(), (LPARAM)&vItms[0]) == LB_ERR)
			return;
	} else {
		// Aucune s�lection : on prend tout le contenu
		for (ITERATE_VECTOR(vItms, INT, it))
			*it = (INT) lSelCnt++;
	}

	UINT nTxtLen = 0;	// Longueur total calcul�e

	// 1�re passe : d�terminer la longueur
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

	// Copier toutes les lignes s�lectionn�es dans l'objet global
	for (ITERATE_CONST_VECTOR(vItms, INT, it)) {
		LRESULT lLen = SendMessage(hCtl, LB_GETTEXT, *it, (LPARAM)pszCopy);

		if (lLen != LB_ERR) {
			pszCopy += lLen;
			_tcscat_s(pszCopy, pszEnd-pszCopy, _T("\r\n"));
			pszCopy += 2;
		}
	}
	GlobalUnlock(hGlbCopy);

	// Transf�rer dans le presse-papier
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

/// Gestion des messages des list-boxes
bool CTsMergerDialog::LbSubProc(HWND hCtl, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		AppendMenu(hPopMenu, 0, IDC_SELECTALL, _TL("Select All\tCtrl-A","Tout s�lectionner\tCtrl-A"));
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

void CTsMergerDialog::SetLogFile(LPCTSTR pszFileName)
{
	if (cLog_fil.is_open())
		cLog_fil.close();
	if (pszFileName)
		cLog_fil.open(pszFileName);
}

void CTsMergerDialog::DisplayFileAnalysis()
{
	SendMessage(hLeftList, LB_RESETCONTENT, 0, NULL);
	SendMessage(hRightList, LB_RESETCONTENT, 0, NULL);

	CTsFileAnalyzer::Result	sRes1;
	CTsFileAnalyzer::Result	sRes2;

	pcr_pid = NO_PID;

	// V�rification du fichier de gauche
	if (!sParms.strSrcFile1.empty()) {

		sRes1 = CTsFileAnalyzer(sParms.strSrcFile1.c_str(),
								tstring(_TL("file #1","fichier 1"))).Analyze();
		cLog_merge << _TL("Information about file #1","Informations pour le fichier 1") << endl;
		sRes1.Output(cLog_merge);
	}

	// V�rification du fichier de droite, en l'obligeant � utiliser le m�me PCR PID que le fichier
	// de gauche
	if (!sParms.strSrcFile2.empty()) {
		CTsFileAnalyzer			cAna(sParms.strSrcFile2.c_str(),
									 tstring(_TL("file #2","fichier 2")), sRes1.pcr_pid);

		sRes2 = CTsFileAnalyzer(sParms.strSrcFile2.c_str(),
								tstring(_TL("file #2","fichier 2")), sRes1.pcr_pid).Analyze();

		if (!sRes2.pcr_ok()) {
			// En cas d'�chec, les fichiers ne sont pas compatibles. On r�analyse n�anmoins, pour
			// affichage, le second fichier SANS obliger � utiliser un PCR PID quelconque.
			sRes2 = CTsFileAnalyzer(sParms.strSrcFile2.c_str(), sRes2.strFileId).Analyze();
		}
		cLog_merge << _TL("Information about file #2","Informations pour le fichier 2") << endl;
		sRes2.Output(cLog_merge);
	}

	SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETPOS, 0, 0);
	SendDlgItemMessage(hDlg, IDC_PROGRESS2, PBM_SETPOS, 0, 0);

	bool bTwoRelatedFiles = false;

	if (!sParms.strSrcFile1.empty() && !sParms.strSrcFile2.empty()) {
		if (sRes1.pcr_pid == sRes2.pcr_pid) {
			if (!sRes1.vPidList.empty() && !sRes2.vPidList.empty()) {
				pcr_pid = sRes1.pcr_pid;
				dtl::Diff<CTsFileAnalyzer::PidInfo, CTsFileAnalyzer::PidList> d(sRes1.vPidList, sRes2.vPidList);

				d.compose();

				// editDistance
				UINT	nSiz1		= (UINT)sRes1.vPidList.size();
				UINT	nSiz2		= (UINT)sRes2.vPidList.size();
				UINT	nDistance	= (UINT)d.getEditDistance();

				if (nSiz1 == nSiz2 && nDistance == 0) {
					bTwoRelatedFiles = true;
				} else {
					if (((nDistance*8) / max(nSiz1, nSiz2)) <=4) {
						cLog_merge << endl
							<<	_TL("The structures of these files are similar but not identical.",
									"Les structures de ces fichiers sont similaires mais pas identiques.") << endl
							<<	_TL("You can try to merge them at your own risks.",
									"Vous pouvez tenter de les fusionner, � vos propres risques. Vous aurez") << endl
							<<	_TL("You will probably want to use the �Shut up!� option during merging.",
									"sans doute besoin d'utiliser l'option ��Silence�!�� au cours de la fusion.") << endl;
						bTwoRelatedFiles = true;
					}
				}
			}
		}

		if (!bTwoRelatedFiles) {
			cLog_merge << endl
				<<	_TL("These files are not related and cannot be merged.",
						"Ces fichiers n'ont pas de relation et ne peuvent pas �tre fusionn�s.") << endl;
		} else {
			INT64 nDist = TIM_27M_Distance(sRes1.t_First, sRes2.t_First);

			if (nDist > 0) {
				sRes1.OutputRelation(cLog_merge, sRes2);
			} else {
				sRes2.OutputRelation(cLog_merge, sRes1);
			}
		}
	}

	EnableDlgItem(IDC_SAVE_AS, bTwoRelatedFiles);
}

void CTsMergerDialog::InitFile(HWND hCtl, UINT nInfoId, LPCTSTR pszFileName, LPCTSTR pszOtherFileName)
{
	if (*pszFileName) {
		size_t	nLen		= _tcslen(pszFileName);
		bool	bTwoFiles	= *pszOtherFileName != 0;

		set_text(hCtl, pszFileName);
		SendMessage(hCtl, EM_SETSEL, nLen, nLen);	// Montrer la fin du nom de fichier
		EnableDlgItem(IDC_SWAP, bTwoFiles);
		ShowFileInfo(nInfoId, pszFileName);
	}
}

void CTsMergerDialog::StartMerging()
{
	myprint(CLS);
	SendMessage(hLeftList, LB_RESETCONTENT, 0, NULL);
	SendMessage(hRightList, LB_RESETCONTENT, 0, NULL);
	cLog_fil.clear();

	SetLogFile(ChangeExtension(sParms.strDstFile.c_str(), _T(".txt")).c_str());

	cLog_fil << _TL(
		"## Source File #1 = ",
		"## Fichier source 1 = ") << sParms.strSrcFile1 << endl;
	CTsFileAnalyzer(sParms.strSrcFile1.c_str(), tstring(_TL("file #1","fichier 1"))).Analyze().Output(cLog_fil);

	cLog_fil << _TL(
		"## Source File #2 = ",
		"## Fichier source 2 = ") << sParms.strSrcFile2 << endl;
	CTsFileAnalyzer(sParms.strSrcFile2.c_str(), tstring(_TL("file #2","fichier 2"))).Analyze().Output(cLog_fil);

	cLog_fil <<  _TL(
		"## Output File = ",
		"## Fichier de sortie = ") << sParms.strDstFile << endl;
	cLog_fil << _T("---------------------------------------------------------------------------------------------------") << endl;

	WorkingThread *	pThd =
		new CMergeTsProcessor(cLog_merge, cLog_out, sParms, pcr_pid, true);

	if (pThd) {
		pThd->StartThread();
		dwThreadId = pThd->getThreadId();
		hThreadHdl = pThd->getThreadHdl();
	}
}

void CTsMergerDialog::StopAndWait()
{
	if (dwThreadId) {
		// Requ�rir l'arr�t si traitement en cours
		PostThreadMessage(dwThreadId, WM_APP_ABORT, 0, 0);
		// Attendre (5 secondes maximum) que le thread ait quitt�
		if (hThreadHdl != NULL)
			WaitForSingleObject(hThreadHdl, 5000);
	}
}

INT_PTR CTsMergerDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static INT a_tabs1[] = {16,56,96,106/*110,120*/};	// Positions des tabulations dans la list box 1
	static INT a_tabs2[] = {16,56,96,106};		// Positions des tabulations dans la list box 2

	switch (uMsg) {

	case WM_INITDIALOG: {
	#if USE_CONSOLE
		startConsole(_T("Fen�tre de diagnostic de TsMerger"), _T("TsMerger.log"));
	#endif
		// Initialisation de la fen�tre
		LONG	nScrW = GetSystemMetrics(SM_CXFULLSCREEN);
		LONG	nScrH = GetSystemMetrics(SM_CYFULLSCREEN);
		RECT	awRect;	// Rectangle de la fen�tre � aligner

		SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_TSMERGER)));

		hLeftFile	= GetDlgItem(hDlg, IDC_FILE1);
		hRightFile	= GetDlgItem(hDlg, IDC_FILE2);
		hLeftList	= GetDlgItem(hDlg, IDC_MSGLIST1);
		hRightList	= GetDlgItem(hDlg, IDC_MSGLIST2);

		// D�finir les tabulations dans les list boxes :
		SendMessage(hLeftList, LB_SETTABSTOPS, (WPARAM)_countof(a_tabs1), (LPARAM)a_tabs1);
		SendMessage(hRightList, LB_SETTABSTOPS, (WPARAM)_countof(a_tabs2), (LPARAM)a_tabs2);

		WinSubClass<SC_FIL1>::SubClass(hLeftFile);
		WinSubClass<SC_FIL2>::SubClass(hRightFile);
		WinSubClass<SC_LBX1>::SubClass(hLeftList);
		WinSubClass<SC_LBX2>::SubClass(hRightList);

		DragAcceptFiles(hLeftFile, TRUE);
		DragAcceptFiles(hRightFile, TRUE);

		GetWindowRect(hDlg, &awRect);
		SetWindowPos(hDlg, NULL, (nScrW-RWdt(awRect))/2, (nScrH-RHgt(awRect))/2, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

		//// Initialisation des barres de progression
		SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETPOS, 0, 0);
		SendDlgItemMessage(hDlg, IDC_PROGRESS2, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendDlgItemMessage(hDlg, IDC_PROGRESS2, PBM_SETPOS, 0, 0);

		cLog_merge.hWnd = hDlg;
		cLog_out.hWnd = hDlg;
		InitFile1();
		InitFile2();

		DisplayFileAnalysis();

		CheckRadioButton(hDlg, IDC_UNION_MODE, IDC_INTERSECTION_MODE,
			sParms.bUnion ? IDC_UNION_MODE : IDC_INTERSECTION_MODE);
		set_check(hDlg, IDC_VIDEOREDO_PRJ, sParms.bMakeVrdPrj);

		if (sParms.bBatch && !sParms.strSrcFile1.empty() && !sParms.strSrcFile2.empty() && !sParms.strDstFile.empty())
			StartMerging();

		// Renvoie True car la fen�tre a �t� initialis�e
		return TRUE; }

	case WM_COMMAND:
		// Commande re�ue (clic, etc...)
		switch(wParam) {

		case _cmd_(IDC_BROWSE1, BN_CLICKED): {
			tstring strNew =
						COpenFileNameDialog().Do(
							hDlg, NULL,
							_TL("Mpeg2 TS video\0*.TS\0","Vid�o Mpeg2 TS\0*.TS\0")
							_TL("All files\0*.*\0","Tous les fichiers\0*.*\0"),
							NULL);
			if (!strNew.empty()) {
				if (strNew == sParms.strSrcFile2) {
					MessageBox(hDlg,
						_TL("This file is already set as second file",
							"Ce fichier est d�j� sp�cifi� comme second fichier"),
						_TL("File Error","Erreur de fichier"), MB_ICONERROR|MB_OK);
					return TRUE;
				}
				sParms.strSrcFile1 = strNew;
				InitFile1();
				DisplayFileAnalysis();
				sParms.strDstFile.clear();
			}
			return TRUE; }

		case _cmd_(IDC_BROWSE2, BN_CLICKED): {
			tstring strNew =
						COpenFileNameDialog().Do(
							hDlg, NULL,
							_TL("Mpeg2 TS video\0*.TS\0","Vid�o Mpeg2 TS\0*.TS\0")
							_TL("All files\0*.*\0","Tous les fichiers\0*.*\0"),
							NULL);

			if (!strNew.empty()) {
				if (strNew == sParms.strSrcFile1) {
					MessageBox(hDlg,
						_TL("This file is already set as first file",
							"Ce fichier est d�j� sp�cifi� comme premier fichier"),
						_TL("File Error","Erreur de fichier"), MB_ICONERROR|MB_OK);
					return TRUE;
				}
				sParms.strSrcFile2 = strNew;
				InitFile2();
				DisplayFileAnalysis();
				sParms.strDstFile.clear();
			}
			return TRUE; }

		case _cmd_(IDC_SWAP, BN_CLICKED): {
			std::swap(sParms.strSrcFile1, sParms.strSrcFile2);
			InitFile1();
			InitFile2();
			DisplayFileAnalysis();
			return TRUE; }

		case _cmd_(IDC_SAVE_AS, BN_CLICKED): {
			tstring 	strNewName;

			if (!sParms.strDstFile.empty()) {
				strNewName = sParms.CompleteDstName();
			} else
				strNewName = sParms.SuggestDstName();

			strNewName = CSaveFileNameDialog().Do(hDlg, NULL,
				_TL("Mpeg2 TS video\0*.TS\0","Vid�o Mpeg2 TS\0*.TS\0")
				_TL("All files\0*.*\0","Tous les fichiers\0*.*\0"),
				strNewName.c_str());

			if (!strNewName.empty()) {
				sParms.strDstFile = strNewName;
				sParms.bMakeVrdPrj	= get_check(hDlg, IDC_VIDEOREDO_PRJ);
				sParms.bUnion		= get_check(hDlg, IDC_UNION_MODE);

				StartMerging();
			}
			return TRUE; }

		case _cmd_(IDC_QUIET1, BN_CLICKED):
			cLog_merge.SetQuiet(get_check(hDlg, IDC_QUIET1));
			return TRUE;

		case _cmd_(IDC_QUIET2, BN_CLICKED):
			cLog_out.SetQuiet(get_check(hDlg, IDC_QUIET2));
			return TRUE;

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
		EnableDlgItem(IDC_BROWSE1, false);
		EnableDlgItem(IDC_BROWSE2, false);
		EnableDlgItem(IDC_SAVE_AS, false);
		EnableDlgItem(IDC_UNION_MODE, false);
		EnableDlgItem(IDC_INTERSECTION_MODE, false);
		EnableDlgItem(IDC_SWAP, false);
		EnableDlgItem(IDC_QUIET1, true);
		EnableDlgItem(IDC_QUIET2, true);
		EnableDlgItem(IDC_PAUSE, true);
		EnableDlgItem(IDCANCEL, true);
		DragAcceptFiles(hLeftFile, FALSE);
		DragAcceptFiles(hRightFile, FALSE);
		return TRUE;

	case WM_APP_PROGRESS:
		SendDlgItemMessage(hDlg, lParam == ID_FIL1 ? IDC_PROGRESS1 : IDC_PROGRESS2, PBM_SETPOS, wParam, 0);
		ShowPctInfo(lParam == ID_FIL1 ? IDC_INFO1 : IDC_INFO2, (UINT)wParam);
		return TRUE;

	case WM_APP_PROCESS_END:
		SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETPOS, 100, 0);
		SendDlgItemMessage(hDlg, IDC_PROGRESS2, PBM_SETPOS, 100, 0);
		EnableDlgItem(IDC_BROWSE1, true);
		EnableDlgItem(IDC_BROWSE2, true);
		EnableDlgItem(IDC_SAVE_AS, true);
		EnableDlgItem(IDC_UNION_MODE, true);
		EnableDlgItem(IDC_INTERSECTION_MODE, true);
		EnableDlgItem(IDC_SWAP, true);
		EnableDlgItem(IDC_QUIET1, false);
		EnableDlgItem(IDC_QUIET2, false);
		EnableDlgItem(IDC_PAUSE, false);
		EnableDlgItem(IDCANCEL, false);
		DragAcceptFiles(hLeftFile, TRUE);
		DragAcceptFiles(hRightFile, TRUE);
		SetLogFile(NULL);
		ShowFileInfo(IDC_INFO1, sParms.strSrcFile1.c_str());
		ShowFileInfo(IDC_INFO2, sParms.strSrcFile2.c_str());
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
