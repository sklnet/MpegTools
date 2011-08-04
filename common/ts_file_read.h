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

#include "lang_switch.h"
#include "cc_checker.h"
#include "logstream.h"
#include "mapping.h"

// ====================================================================================

#define READ_MAP_SIZE	0x200000

// ====================================================================================

struct TS_packet_Ex : public TS_packet
{
	UINT16				pid;
	CCChecker::CcErr	eCcErr;		//!< Code d'erreur lié à la continuité
	CCChecker::PkErr	ePkErr;		//!< Code d'erreur intrinsèque au paquet

	tstring			toString() const;

	tstring			getHexDump(LPCTSTR pszSep) const;

	bool			operator == (const TS_packet_Ex & sTs2) const {
						return memcmp(this, &sTs2, sizeof(*this)) == 0; }
	bool			operator != (const TS_packet_Ex & sTs2) const {
						return memcmp(this, &sTs2, sizeof(*this)) != 0; }
	bool			error() const {
						return eCcErr != CCChecker::cc_NoError || ePkErr != CCChecker::pe_NoError; }

	const TS_hdr &	operator () () const {
						return hdr; }
	operator const TS_packet & () const {
						return *this; }

	TS_packet_Ex() :
		pid(NO_PID),
		eCcErr(CCChecker::cc_NoError),
		ePkErr(CCChecker::pe_NoError)
	{
	}
};

// ====================================================================================

inline tostream & operator << (tostream & os, const TS_packet_Ex & sTs)
{
	os << sTs.toString().c_str();
	return os;
}

// ====================================================================================

class TS_pack_list : public vector<TS_packet_Ex>
{
public:
	TIM_27M			t_Begin;		//!< Temps PCR du premier paquet du groupe qui en ait un
	TIM_27M			t_End;			//!< Temps PCR du dernier paquet du groupe
	UINT			nGrpErrCount;	//!< Nombre de paquets avec erreur placé dans le groupe

	TS_pack_list() :
		t_Begin(HIGHEST_PCR_OFFSET),
		t_End(HIGHEST_PCR_OFFSET)
	{
	}

	difference_type	TrimBegin(difference_type nSiz) {
						iterator it = begin(); erase(it, it + nSiz); return nSiz; }
	difference_type	TrimEnd(difference_type nSiz) {
						iterator it = end(); erase(it - nSiz, it); return nSiz; }

	/// Ajout d'une référence de paquet TS, avec incrément du compteur d'erreurs si ce paquet comptient une erreur
	void Add(const TS_packet_Ex & sTs) {
			if (sTs.error()) ++nGrpErrCount; push_back(sTs); }

	void clear() {
			__super::clear(); t_Begin = t_End; nGrpErrCount = 0; }
};

// ====================================================================================

class CTsFileReader : public FileMapping, public CCChecker
{
	virtual void	ProgressStep(INT nValue) {
						}
public:
	MemRng			sMm;			//!< Intervalle pour la "fenêtre" couramment accessible dans le fichier
	tstring			strFileName;	//!< Nom du fichier courant

	CTsFileReader(LPCTSTR pszFileName, UINT16 pcr_pref_pid = NO_PID) :
		FileMapping(false, pszFileName, READ_MAP_SIZE),
		CCChecker(pcr_pref_pid),
		strFileName(pszFileName)
	{
	}

	CCChecker::Result	ReadPacket(TS_packet_Ex & sTs) throw(...);
};


// ====================================================================================

//void LogTsTime(tostream & os, LPCTSTR pszPos, TIM_27M t_Time, LPCTSTR pszFileId);

// ====================================================================================

class CTsGrpFileReader : public CTsFileReader
{
	virtual void	ProgressStep(INT nValue) {
						cLog.ProgressStep(nValue, nIdt); }

	TS_packet_Ex		sRecycledPacket;
	CCChecker::Result	sRecycledResult;
	bool				bRecyclePacket;

public:

	CLogStreamBase &	cLog;			//!< Référence pour la journalisation
	TS_pack_list		vData;			//!< Groupe de paquets TS lus (normalement délimités par ceux ayant des PCRs)
	tstring				strFileId;		//!< Identification du fichier (texte)
	INT					nIdt;			//!< Identification du fichier (numérique)

	CTsGrpFileReader(CLogStreamBase & cLg, LPCTSTR pszFileName, tstring strFilId, INT nId = -1, UINT16 pcr_pref_pid = NO_PID) :
		CTsFileReader(pszFileName, pcr_pref_pid),
		cLog(cLg),
		strFileId(strFilId),
		nIdt(nId),
		bRecyclePacket(false)
	{
	}

	CCChecker::Result	ReadPacket(TS_packet_Ex & sTs) throw(...);
	void				RecyclePacket(const TS_packet_Ex & sTs, const CCChecker::Result & sRes);

	void				LogErrors() const {
							OutReport(cLog, strFileId.c_str()); }
	bool				LoadGroup() throw(...);

	void				clear() {
							vData.clear(); }
};

// ====================================================================================

class CTsFileAnalyzer : public CTsFileReader
{
	tstring		strFileId;		//!< Identification du fichier (texte)
public:
	struct PidInfo
	{
		WORD	pid;
		UINT	nCount;
		bool	bUsePcr;

		PidInfo(WORD pd, bool bPcr) :
			pid(pd),		//!< PID
			nCount(0),		//!< Nombre de paquets ayant ce PID
			bUsePcr(bPcr)	//!< PCR présent
		{
		}

		bool operator == (WORD pid2) const {
			return pid == pid2; }
		bool operator == (const PidInfo & p2) const {
			return pid == p2.pid; }
		bool operator < (const PidInfo & p2) const {
			return pid < p2.pid; }
	};

	class PidList : public vector<PidInfo>
	{
	public:
		PidList(const_iterator it1, const_iterator it2) :
			vector(it1, it2)
		{
		}

		PidList()
		{
		}

		void RefPid(WORD pid, bool bHasPcr);
		void Output(tostream & os, LPCTSTR pszMsg);
		void sort() {
				::sort(begin(), end()); }
	};

	struct Result
	{
		TIM_27M			t_First;	//!< Temps PCR du premier paquet du fichier qui en ait un
		TIM_27M			t_Last;		//!< Temps PCR du dernier paquet du fichier
		TIM_27M			t_Duration;	//!< Durée entre le premier et le dernier PCR
		WORD			pcr_pid;	//!< PID utilisé pour le PCR
		PidList			vPidList;	//!< Liste des PIDs de medias trouvés
		PidList			vRootPids;	//!< Liste des PIDs racine trouvés
		tstring			strFileId;	//!< Identification du fichier (texte)

		Result(tstring strFilId = tstring()) :
			t_First(HIGHEST_PCR_OFFSET),
			t_Last(HIGHEST_PCR_OFFSET),
			t_Duration(0),
			strFileId(strFilId)
		{
		}

		bool pcr_ok() const {
			return t_First != HIGHEST_PCR_OFFSET || t_Last != HIGHEST_PCR_OFFSET; }
		void Output(tostream & os);
		void OutputRelation(tostream & os, const Result & sR2);
	};

	CTsFileAnalyzer(LPCTSTR pszFileName, tstring strFilId, UINT16 pcr_pref_pid = NO_PID) :
		CTsFileReader(pszFileName, pcr_pref_pid),
		strFileId(strFilId)
	{
	}

	Result Analyze();
};

// ====================================================================================
