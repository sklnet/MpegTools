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
#include "cc_checker.h"
#include "lang_switch.h"

// ====================================================================================

/// Calcul de la distance entre deux timestamps TS en tenant compte du rebouclage possible
/// \see http://www.codeproject.com/KB/recipes/Circular-Values.aspx?display=Print (section 9)
INT64 TIM_27M_Distance(TIM_27M t1, TIM_27M t2)
{
	if (t2 < t1)
		t2 += (((t1-t2)/WRAP_27M)+1) * WRAP_27M;

	TIM_27M tmp = ((INT64)t2 - (INT64)t1 + WRAP_27M/2) % WRAP_27M;
	INT64	res = (INT64)tmp - WRAP_27M/2;
	return res;

	// return (((INT64)t2 - (INT64)t1 + WRAP_27M/2) % WRAP_27M) - WRAP_27M/2;
}

// ====================================================================================

tostream & Clk27M::Output(tostream & os) const
{
	if (t_Tim >= WRAP_27M)
		os << _TL("[INVALID]","[INVALIDE]");
	else {
		TCHAR	szWrk[32];
		UINT	nTmp	= UINT((t_Tim+(PCR_FREQ/20))/(PCR_FREQ/10));

		_stprintf_s(szWrk, _T("%02u:%02u:%02u.%u"), nTmp/36000, (nTmp/600)%60, (nTmp/10)%60, nTmp%10);
		os << szWrk;
	}
	return os;
}

// ====================================================================================

TIM_27M CCChecker::time_adj(TIM_27M t)
{
	if ((INT64)t < t_PcrBase) {
		if (t == 0)
			return 0;	// Aucun PCR reçu, on est au tout début
		if (t_PcrBase == HIGHEST_PCR_OFFSET)
			t_PcrBase = t;
		else {
			INT64 t_Tmp = TIM_27M_Distance(t_PcrBase, t);

			if (t_Tmp < 0 && t_Tmp >= -PCR_FREQ * 3600)
				t_PcrBase = t;
		}
	}
	return (t+WRAP_27M - t_PcrBase) % WRAP_27M;
}

/**
 * \brief		Vérification d'un paquet TS pour les erreurs et la continuité
 * \param[in]	sTs		Paquet à vérifier
 * \return		PCR brut, ou bien #HIGHEST_PCR_OFFSET si aucun PCR dans ce paquet
 **/
CCChecker::Result CCChecker::CheckTS(const TS_packet & sTs)
{
	const TS_hdr &		hdr	= sTs.hdr;
	Result				sResult(hdr.pid());
	TS_unwrapper		uw_ts(hdr);

	sResult.ePkErr = hdr.transport_error_indicator() ? pe_ErrorFlag : pe_NoError;

	// Vérifier (si pas d'erreur) si le paquet contient un PCR (marqueur de temps).
	if (sResult.ePkErr == pe_NoError && uw_ts.p_afield) {
		TS_AF_unwrapper_base uw_af(*uw_ts.p_afield);

		if (uw_af.p_pcr) {
			if (pcr_pid == NO_PID)
				pcr_pid = sResult.pid;
			if (pcr_pid == sResult.pid) {
				TIM_27M	t_OldPcr = t_LastPcr;

				t_LastPcr = *uw_af.p_pcr;

				if (t_LastPcr < t_OldPcr && TIM_27M_Distance(t_OldPcr, t_LastPcr) > 0) {
					// La valeur du PCR a excédé sa limite de 26h, 30mn, 43s et 718/1000.
					sResult.ePcrState = ps_PcrWrap;
				} else
					sResult.ePcrState = ps_HasPcr;
			} else
				sResult.ePcrState = ps_OtherPcr;
		}
	}

	sResult.t_RelPcr = getRelativeTime();

	// S'il y a une charge utile, vérifier le compteur de continuité
	if (hdr.has_payload())
		sResult.CheckCC(aCC[sResult.pid], hdr.continuity_counter());

	// Rapporter les erreurs éventuelles constatées
	if (sResult.eCcErr != cc_NoError)
		++nCcErrCount;
	if (sResult.ePkErr != pe_NoError)
		++nFlgErrCount;
	if (sResult.eCcErr != cc_NoError || sResult.ePkErr != pe_NoError)
		++nAllErrCount;
	++nPacketCount;
	return sResult;
}

/// Rapport des erreurs rencontrées
void CCChecker::OutReport(tostream & os, LPCTSTR pszIdt) const
{
	os	<< _TL( "Error report for ",
				"Récapitulatif pour ") << pszIdt << _TL(":"," :") << endl
		<< _TL(	"\tPacket count\t=\t",
				"\tNombre de paquets\t=\t") << nPacketCount << endl
		<< _TL(	"\tContinuity errors\t=\t",
				"\tErreurs de continuité\t=\t") << nCcErrCount << endl
		<< _TL(	"\tError flag set\t=\t",
				"\tIndicateurs d'erreur\t=\t") << nFlgErrCount << endl
		<< _TL(	"\tPackets with error\t=\t",
				"\tPaquets en erreur\t=\t") << nAllErrCount << endl;
}

// ====================================================================================

void CCChecker::Result::CheckCC(BYTE & ccr, BYTE cc)
{
	found_cc		= cc;
	if (cc == ccr)
		eCcErr		= cc_Duplicate;
	else {
		expected_cc	= ccr == 0xff ? cc : ccr+1 & 0x0f;
		ccr			= cc;
		eCcErr		= expected_cc != cc ? cc_Continuity : cc_NoError;
	}
}

tostream & CCChecker::Result::OutStream(tostream & os) const
{
	if (ePkErr == ps_PcrWrap) {
		os << _T("@") << Clk27M(t_RelPcr) <<
			_TL(
				":\tPCR wrap. This is not an error.",
				":\tBouclage PCR. Ce n'est pas une erreur.") << endl;
	}
	if (error()) {
		os << _T("@") << Clk27M(t_RelPcr) << _T(" (pid=") << pid << _T("):");
		if (eCcErr != cc_NoError) {
			os << _TL("\tDiscontinuity, expected=","\tDiscontinuité, attendu=")
				<< expected_cc << _TL(", found=",", trouvé=") << found_cc;
			if (ePkErr == pe_ErrorFlag)
				os << endl << _T("\t\t");
		}
		if (ePkErr == pe_ErrorFlag)
			os << _TL("\tError flag set","\tIndicateur d'erreur placé");
		os << endl;
	}

	return os;
}

// ====================================================================================