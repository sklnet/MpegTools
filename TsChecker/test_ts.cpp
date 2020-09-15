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

// #define WIN32_LEAN_AND_MEAN

// ====================================================================================

#include "stdafx.h"
#include "test_ts.h"
#include "../common/vredo_prj.h"

// ====================================================================================

CTestTsProcessor::CTestTsProcessor(CLogStreamBase & cLg,
								   Params & sPrms, bool bDelOnExit) :
	WorkingThread(bDelOnExit),
	CTsFileReader(sPrms.strTstFile.c_str()),
	cLog(cLg),
	bEof(false),
	bMakeVrdPrj(sPrms.bMakeVrdPrj)
{
}

void CTestTsProcessor::Process() throw(...)
{
	reset();

	TS_packet_Ex		sTs;
	CCChecker::Result	sRes;
	TIM_27M				t_Begin	= HIGHEST_PCR_OFFSET;
	TIM_27M				t_End	= HIGHEST_PCR_OFFSET;

	while (!(sRes = ReadPacket(sTs)).eof()) {

		cLog << sRes;

		if (sRes.error())
			vTimeMarks.Add27M(sRes.t_RelPcr);

		if (sRes.haspcr()) {
			if (t_Begin == HIGHEST_PCR_OFFSET) {
				t_Begin = t_LastPcr;
				//LogTsTime(cLog, _TL("start ","début"), t_Begin, _TL("file","fichier"));
			} else
				t_End = t_LastPcr;
		}

		cLog.CheckCancelState();
	}

	//LogTsTime(cLog, _TL("end ","fin"), t_End, _TL("file","fichier"));
	cLog <<	_TL("Done.", "Effectué.") << endl;

	OutReport(cLog, _TL("this file","ce fichier"));

	TIM_27M	t_Duration = getRelativeTime();

	cLog <<	_TL("Total duration = ","Durée totale = ") << Clk27M(t_Duration) << endl;

	cLog
		<<	_TL("\tNote that this duration is solely based on the timestamps found inside the",
				"\tNotez que cette durée est uniquement basée sur les marqueur de temps") << endl
		<<	_TL("\tTS stream. If there are missing parts in the output file, the displayed value",
				"\ttrouvés dans le flux TS. Si des parties sont manquantes dans le fichier de") << endl
		<<	_TL("\twill probably be inaccurate.",
				"\tsortie, la valeur affichée sera probablement inexacte.") << endl;

	if (bMakeVrdPrj) {
		CVideoRedoPrj	cVrdPrj(strFileName.c_str(), vTimeMarks, t_Duration * 10 / 27);

		cLog << _TL("Generating VideoRedo project file","Génération du fichier projet VideoRedo") << endl;
		if (!cVrdPrj.Generate())
			cLog << _TL("Cannot create VideoRedo project file","Impossible de créer le fichier projet VideoRedo") << endl;
		else
			cLog << _TL("\tMark count\t=\t","\tNombre de marqueurs\t=\t") << vTimeMarks.size() << endl;
	}
}

DWORD CTestTsProcessor::ThreadProc()
{
	DWORD dwRes = 0;

	cLog.PostMessage(WM_APP_PROCESS_BEG);

	cLog << _TL("Checking in progress","Vérification en cours") << endl;

	try {
		Process();
	} catch(DWORD dwErr) {
		dwRes = dwErr;
	}

	switch (dwRes) {

	case DWORD(-1):
		cLog << _TL("Checking stopped","Vérification interrompue") << endl;
		break;

	case 0:
		cLog << _TL("Checking terminated","Vérification terminée") << endl;
		break;

	default:
		cLog << _TL("Error 0x","Erreur 0x") << hex << dwRes << endl;
	}
	cLog.PostMessage(WM_APP_PROCESS_END, dwRes);
	return dwRes;
}

// ====================================================================================
// Paramètres en ligne de commande
// ====================================================================================

bool CTsCheckerCmdLine::ProcessArg(tstring & strArg)
{
	LPCTSTR pszParam = strArg.c_str();

	if (*pszParam == TCHAR('-')) {
		TCHAR ch;

		while ((ch = * ++pszParam)) {
			switch (ch) {

			case 'b':
				bBatch = true;
				break;

			case 'v':
				bMakeVrdPrj = true;
				break;

			default:
				return false;
			}
		}
	} else if (strTstFile.empty())
		strTstFile = strArg;
	else
		return false;
	return true;
}

// ====================================================================================