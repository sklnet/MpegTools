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
#include "ts_file_read.h"

// ====================================================================================

tstring	TS_packet_Ex::toString() const
{
	TCHAR			szWrk[128];
	TS_unwrapper	uw_ts(hdr);
	bool			bErr = hdr.transport_error_indicator();

	_stprintf_s(szWrk, _T("{%u,%u"), hdr.pid(), hdr.continuity_counter());
	if (uw_ts.p_afield) {
		_tcscat_s(szWrk, _T("--af"));
		if (!bErr) {
			TS_AF_unwrapper_full uw_af(*uw_ts.p_afield);

			if (uw_af.p_pcr) {
				_tcscat_s(szWrk, _T("--pcr="));
				_tcscat_s(szWrk, clk27M2str(*uw_af.p_pcr)());
			} else if (
				uw_af.p_opcr || uw_af.p_splicing_point || uw_af.p_priv_data ||
				uw_af.p_ext || uw_af.p_ltw || uw_af.p_piecewise_rate || uw_af.p_seamless_splice
			) {
				_tcscat_s(szWrk, _TL("--other","--autre"));
			}

			if (uw_af.p_next < uw_ts.p_next) {
				UINT nOrg = (UINT)_tcslen(szWrk);
				_stprintf_s(szWrk+nOrg, _countof(szWrk)-nOrg,
					_TL("--stuffing=%u","--remplissage=%u"), uw_ts.p_next - uw_af.p_next);
			}
		}
	}
	if (!uw_ts.p_payload)
		_tcscat_s(szWrk, _T("--nodata"));
	if (hdr.transport_priority())
		_tcscat_s(szWrk, _T("--pri"));
	if (hdr.transport_scrambling_control())
		_tcscat_s(szWrk, _T("--crypt"));
	if (bErr)
		_tcscat_s(szWrk, _T("--ERR"));
	_tcscat_s(szWrk, _T("}"));
	return szWrk;
}

tstring TS_packet_Ex::getHexDump(LPCTSTR pszSep) const
{
	tstringstream	strmTmp;
	PCUINT8			pszBytes	= bytes;

	for (UINT nIny=0; nIny<4; ++nIny) {
		strmTmp << pszSep << hex;
		for (UINT nInx=0; nInx < sizeof(TS_packet)/4; ++nInx) {
			strmTmp << _T(" ");
			strmTmp.width(2);
			strmTmp.fill(TCHAR('0'));
			strmTmp << *pszBytes++;
		}
		strmTmp << endl;
	}

	return strmTmp.str();
}

// ====================================================================================

CCChecker::Result CTsFileReader::ReadPacket(TS_packet_Ex & sTs) throw(...)
{
	int					nCount = 0;
	CCChecker::Result	sRes;

	while (true) {
		if (remap(sMm, sizeof(TS_packet)) < sizeof(TS_packet))
			return sRes;

		memcpy(&sTs, sMm.ptr, sizeof(TS_packet));
		if (sTs.isTS(true)) {
			sMm += sizeof(TS_packet);
			if (sTs.hdr.pid == p_pid_NULL)	// Sauter les paquets nuls qui peuvent parfois avoir été laissés
				continue;
			break;	// On a un paquet valide
		}

		if (++nCount > 10) {	// Au bout de 10 défauts, on laisse tomber
			sRes.ePkErr = pe_BadData;
			return sRes;
		}

		// Le pointeur courant ne pointe pas sur un paquet TS. Tenter une resynchronisation.
		LPCBYTE pSearch = (LPCBYTE)memchr(sMm.ptr+1, TS_SYNC, sMm.siz-1);
		if (!pSearch) {
			sRes.ePkErr = pe_BadData;
			return sRes;
		}
		sMm >>= pSearch;
	}

	sRes		= CheckTS(sTs);
	sTs.pid		= sRes.pid;
	sTs.eCcErr	= sRes.eCcErr;
	sTs.ePkErr	= sRes.ePkErr;

	// Messages de progression :
	INT32 progress = chkProgress(100);

	if (progress >= 0)
		ProgressStep(progress);
	return sRes;
}

// ====================================================================================

CCChecker::Result CTsGrpFileReader::ReadPacket(TS_packet_Ex & sTs) throw(...)
{
	if (bRecyclePacket) {
		bRecyclePacket = false;
		sTs = sRecycledPacket;
		return sRecycledResult;
	}

	return __super::ReadPacket(sTs);
}

void CTsGrpFileReader::RecyclePacket(const TS_packet_Ex & sTs, const CCChecker::Result & sRes)
{
	sRecycledPacket = sTs;
	sRecycledResult = sRes;
	bRecyclePacket = true;
}

bool CTsGrpFileReader::LoadGroup() throw(...)
{
	TS_packet_Ex		sTs;
	
	CCChecker::Result	sRes = ReadPacket(sTs);

	if (sRes.eof())
		return bEof = true, false;

	if (vData.empty()) {
		// Lecture jusqu'au premier timestamp
		do {
			vData.Add(sTs);
			sRes = ReadPacket(sTs);
			if (sRes.eof())
				return bEof = true, false;
		} while (!sRes.haspcr());

		vData.t_End = vData.t_Begin = t_LastPcr;
	}

	// Poursuite de la lecture jusqu'au second timestamp
	do {
		vData.Add(sTs);
		sRes = ReadPacket(sTs);
		if (sRes.eof())
			return bEof = true, false;
	} while (!sRes.haspcr());

	vData.t_End = t_LastPcr;

	// Recycler le dernier paquet
	RecyclePacket(sTs, sRes);
	return true;
}

// ====================================================================================

#define MAX_PACKET_LOOP	150000

void CTsFileAnalyzer::PidList::RefPid(WORD pid, bool bHasPcr)
{
	iterator	it = find(begin(), end(), pid);

	if (it == end())
		push_back(PidInfo(pid, bHasPcr));
	else {
		++it->nCount;
		if (bHasPcr)
			it->bUsePcr = true;
	}
}

void CTsFileAnalyzer::PidList::Output(tostream & os, LPCTSTR pszMsg)
{
	os << pszMsg;

	tstring strWrk;

	for (ITERATE_CONST_VECTOR(*this, PidInfo, it)) {
		TCHAR	szWrk[16];

		if (_itot_s(it->pid, szWrk, 10) == 0) {
			if (it->bUsePcr)
				_tcscat_s(szWrk, _T("*"));
			if (!strWrk.empty())
				strWrk += _T(",");
			if (strWrk.length()+_tcslen(szWrk) > 36) {
				os << strWrk << endl << _T("\t\t\t\t");
				strWrk.clear();
			}
			if (!strWrk.empty())
				strWrk += _T(" ");
			strWrk += szWrk;
		}

	}
	os << strWrk << endl;
}

void CTsFileAnalyzer::Result::Output(tostream & os, LPCTSTR pszMsg)
{
	if (pszMsg) {
		os << pszMsg << strFileId << _TL(":", " :") << endl;
	}

	vPidList.Output(os, _TL("\tIncluded PIDs (*=PCR)\t=\t","\tPIDs inclus (*=PCR)\t=\t"));
	vRootPids.Output(os, _TL("\tRoot PIDs found\t=\t","\tPIDs racine trouvés\t=\t"));

	if (pcr_ok()) {
		os	<< _TL("\tFirst and last PCR\t=\t","\tPremier et dernier PCR\t=\t")
			<< Clk27M(t_First) << _T(", ") << Clk27M(t_Last) << _TL("  (in pid #","  (dans pid ") << pcr_pid << _T(")") << endl;
		os	<< _TL("\tDuration\t\t=\t","\tDurée\t\t=\t")
			<< Clk27M(t_Duration) << endl;
	}
}

void CTsFileAnalyzer::Result::OutputRelation(tostream & os, const Result & sR2)
{
	os	<< endl
		<< _TL("The ","Le ") << strFileId
		<< _TL(" begins before the "," commence avant le ")
		<< sR2.strFileId << _T(".") << endl;

	INT64 nDist =  TIM_27M_Distance(sR2.t_First, t_Last);

	if (nDist < 0) {
		os	<<	_TL("These two files have same structure but no common part.",
					"Les deux fichiers ont la même structure mais pas de partie commune.") << endl
			<<	_TL("They can only be concatenated.",
					"Ils peuvent seulement être concaténés.") << endl;
	} else {
		os	<<	_TL("These two files have a common part with a duration of ",
					"Les deux fichiers ont une partie commune d'une durée de ")
			<<	Clk27M(min(nDist, (INT64)sR2.t_Duration))
			<<	_T(".") << endl;
	}
}

CTsFileAnalyzer::Result CTsFileAnalyzer::Analyze()
{
	TS_packet_Ex		sTs;
	CCChecker::Result	sTsRes;
	Result				sAnRes(strFileId);
	INT					nInx = 0;
	UINT64				qwEndSeek	= qwFileSize - min(qwFileSize, MAX_PACKET_LOOP*sizeof(TS_packet));

	reset();

	try {
		// Examiner les MAX_PACKET_LOOP premiers et derniers paquets
		while (!(sTsRes = ReadPacket(sTs)).eof()) {
			if (sTsRes.ePkErr != pe_NoError)
				continue;
			if (sTsRes.haspcr()) {
				if (sAnRes.t_First == HIGHEST_PCR_OFFSET) {
					sAnRes.t_First = t_LastPcr;	// Retenir le premier PCR
					sAnRes.pcr_pid = pcr_pid;	// Retenir aussi son PID
				}
				sAnRes.t_Last = t_LastPcr;	// Retenir le dernier PCR
			}

			// Retenir tous les PIDs trouvés
			if (sTsRes.pid >= 0x20)
				sAnRes.vPidList.RefPid(sTsRes.pid, sTsRes.ePcrState != CCChecker::ps_NoPcr);
			else
				sAnRes.vRootPids.RefPid(sTsRes.pid, false);

			// Une fois qu'on a passé les MAX_PACKET_LOOP premiers paquets, on se repositionne sur le début des
			// MAX_PACKET_LOOP derniers paquets … sauf si le nombre total de paquets est inférieur à 2 × MAX_PACKET_LOOP
			if (++nInx == MAX_PACKET_LOOP && qwEndSeek > fileoffset(sMm.ptr))
				seek(qwEndSeek, sMm);
		}
	} catch(DWORD dwErr) {
		tstringstream strmTmp;

		strmTmp << _TL("Error code = ","Code d'erreur = ") << dwErr;

		MessageBox(NULL, strmTmp.str().c_str(),
			_TL("Error during file analysis","Erreur durant l'analyse du fichier"), MB_ICONERROR|MB_OK);
	}

	if (sAnRes.t_First != HIGHEST_PCR_OFFSET && sAnRes.t_Last != HIGHEST_PCR_OFFSET)
		sAnRes.t_Duration = TIM_27M_Distance(sAnRes.t_First, sAnRes.t_Last);

	sAnRes.vPidList.sort();
	sAnRes.vRootPids.sort();
	return sAnRes;
}

CTsFileAnalyzer::Result CTsFileAnalyzer::Analyze(LPCTSTR pszFile, LPCTSTR pszId, UINT16 pcr_pref_pid)
{
	CTsFileAnalyzer::Result sRes = CTsFileAnalyzer(pszFile, pszId, pcr_pref_pid).Analyze();

	if (pcr_pref_pid != NO_PID && !sRes.pcr_ok()) {
		// En cas d'échec du 2nd fichier, les fichiers ne sont pas compatibles. On réanalyse néanmoins, pour
		// affichage, le second fichier SANS obliger à utiliser un PCR PID quelconque.
		sRes = CTsFileAnalyzer(pszFile, pszId).Analyze();
	}
	return sRes;
}

// ====================================================================================
