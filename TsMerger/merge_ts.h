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

#include "dtl/dtl.hpp"
#include "cmdline.h"
#include "../common/cc_checker.h"
#include "../common/vredo_prj.h"
#include "../common/ts_file_read.h"

using namespace std;
using namespace dtl;

// ====================================================================================

#define REQ_MAP_SIZE	0x80000

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

#define ID_FIL1 1	//!< Identifiant pour le fichier 1
#define ID_FIL2 2	//!< Identifiant pour le fichier 2

// ====================================================================================

class CTsFileWriter : public FileMapping, public CCChecker
{
	MemRng				sMm;
public:
	CLogStreamBase &	cLog;
	TimeMarkVect		vTimeMarks;
	tstring				strFileName;

	CTsFileWriter(CLogStreamBase & cLg, LPCTSTR pszFileName);

	void		WritePacket(const TS_packet & sTs) throw(...);
	bool		ProcessUntil(CTsGrpFileReader & cSrcF, bool bWrite, TIM_27M t_Limit=HIGHEST_PCR_OFFSET) throw(...);

	template <class T>
		void	WriteSeq(T & vPLst) throw(...)
	{
		for (ITERATE_CONST_VECTOR(vPLst, T::value_type, it))
			WritePacket(*it);
		vPLst.clear();
	}

	~CTsFileWriter();
};

// ====================================================================================

class TS_pack_lptr;	// -> D�finie dans l'impl�mentation

class CMergeTsProcessor : public WorkingThread
{
	friend class	CTsFileReader;

	CLogStreamBase &	cLog_merge;
	CLogStreamBase &	cLog_out;
	CTsGrpFileReader	cMFil1;
	CTsGrpFileReader	cMFil2;
	CTsFileWriter		cOutF;
	bool				bUnionMode;
	bool				bEof1;
	bool				bEof2;
	bool &				bMakeVrdPrj;

	virtual DWORD	ThreadProc();

	void			LogMissing(LONG nCount, LPCTSTR pszFileId);
	void			LogUnresolved(const TS_packet & sTs1, const TS_packet & sTs2);

	bool			CheckCancelState() throw(...);
	bool			ProcessBegin(CTsGrpFileReader & cMF_this, CTsGrpFileReader & cMF_other) throw(...);
	bool			ProcessEnd(CTsGrpFileReader & cMF_this, CTsGrpFileReader & cMF_other) throw(...);
	void			SyncOverflow(CTsGrpFileReader & cMF_this, CTsGrpFileReader & cMF_other) throw(...);
	void			SyncLoad() throw(...);
	void			MergeAppend(TS_pack_lptr & vSrc, TS_pack_lptr & vOther, TS_pack_lptr & vFinal) throw(...);

	/// Traitement de la fusion de deux branches qui n'ont rien en commun
	void			MergeDiff(TS_pack_lptr & vSubSq1, TS_pack_lptr & vSubSq2, TS_pack_lptr & vFinal) throw(...);

	void			Merge() throw(...);

	void			Process() throw(...);


public:

	struct Params
	{
		tstring	strSrcFile1;	//!< Nom du premier fichier source
		tstring	strSrcFile2;	//!< Nom du second fichier source
		tstring	strDstFile;		//!< Nom du fichier destination
		bool	bUnion;			//!< \p true si mode union (sinon, mode intersection)
		bool	bMakeVrdPrj;	//!< \p true si fichier projet VideoRedo � g�n�rer (peut �tre modifi� en cours de traitement)
		bool	bBatch;

		Params() :
			bUnion(true),
			bMakeVrdPrj(false),
			bBatch(false)
		{
		}

		tstring	SuggestDstName();

		/// Si le nom de fichier de destination n'a pas de chemin, lui en fournir un d'apr�s
		/// ceux des noms des fichiers source
		tstring CompleteDstName();
	};

	CMergeTsProcessor(CLogStreamBase & cLog_mrg, CLogStreamBase & cLog_o,
		Params & sPrms, UINT16 pcr_pref_pid, bool bDelOnExit);
};

// ====================================================================================
// Param�tres en ligne de commande
// ====================================================================================

class CTsMergerCmdLine : public CCmdLine, public CMergeTsProcessor::Params
{
	/// M�thode virtuelle appel�e par \p ProcessAll
	virtual bool	ProcessArg(tstring & strArg);
public:
	/// Constructeur avec les param�tres de la fonction 'tmain'
	CTsMergerCmdLine(int argc, LPTSTR argv[]) :
		CCmdLine(argc, argv)
	{
	}

	/// Constructeur avec une ligne de commande brute
	CTsMergerCmdLine(LPCTSTR pszCmdLine) :
		CCmdLine(pszCmdLine)
	{
	}
};

// ====================================================================================
