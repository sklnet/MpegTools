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

typedef basic_ostream<TCHAR>	tostream;

// ====================================================================================

#define NO_PID				0xffff
#define HIGHEST_PCR_OFFSET	0x7fffffffffffffff

// ====================================================================================

/// Calcul de la distance entre deux timestamps TS en tenant compte du rebouclage possible
/// \see http://www.codeproject.com/KB/recipes/Circular-Values.aspx?display=Print (section 9)
INT64 TIM_27M_Distance(TIM_27M t1, TIM_27M t2);

// ====================================================================================

/// Construction d'une chaîne d'affichage d'une valeur de temps exprimée en unités PCR de 27 MHz
class Clk27M
{
	TIM_27M	t_Tim;
public:
	Clk27M(TIM_27M t) :
		t_Tim(t)
	{
	}

	tostream & Output(tostream & os) const;
};

inline tostream &	operator << (tostream & os, const Clk27M & sTim) {
						return sTim.Output(os); }

// ====================================================================================

class CCChecker
{
public:
	enum PcrState : BYTE
	{
		ps_NoPcr,		//!< Aucun PCR dans le paquet
		ps_OtherPcr,	//!< PCR présent, mais concerne un flux non référencé
		ps_HasPcr,		//!< PCR présent
		ps_PcrWrap,		//!< PCR présent, avec bouclage
	};

	/// Codes d'erreur liés à la continuité
	enum CcErr : BYTE
	{
		cc_NoError,		//!< Pas d'erreur
		cc_Continuity,	//!< Défaut de continuité
		cc_Duplicate	//!< Deux fois le même compteur de continuité avec le même PID
	};

	/// Codes d'erreurs intrinsèques au paquet
	enum PkErr : BYTE
	{
		pe_NoError,
		pe_ErrorFlag,
		pe_NoData,
		pe_BadData
	};

	struct Result
	{
		TIM_27M		t_RelPcr;			//!< Temps relatif (ajusté) du paquet, si \p bHasPcr est vrai
		UINT16		pid;				//!< Pid du paquet en erreur
		PcrState	ePcrState;			//!< Absence / présence du PCR
		BYTE		rsvd;				//!< Réservé (alignement)
		CcErr		eCcErr;				//!< Code d'erreur lié à la continuité
		PkErr		ePkErr;				//!< Code d'erreur intrinsèque au paquet
		BYTE		expected_cc;		//!< Valeur attendue du compteur de continuité
		BYTE		found_cc;			//!< Valeur trouvée du compteur de continuité

		tostream & OutStream(tostream & os) const;

		void		CheckCC(BYTE & ccr, BYTE cc);

		bool		error() const {
						return eCcErr != cc_NoError || ePkErr != pe_NoError; }
		bool		eof() const {
						return ePkErr >= pe_NoData; }
		bool		haspcr() const {
						return ePcrState >= ps_HasPcr && ePkErr == pe_NoError; }

		Result(UINT16 _pid = NO_PID) :
			t_RelPcr(HIGHEST_PCR_OFFSET),
			pid(_pid),
			ePcrState(ps_NoPcr),
			rsvd(0),
			eCcErr(cc_NoError),
			ePkErr(pe_NoData),
			expected_cc(0),
			found_cc(0)
		{
		}
	};

	BYTE				aCC[0x2000];	//!< Table des compteurs de continuité
										// (0x2000 = Nombre maximal de PIDs possibles, car ils sont codés sur 13 bits)
	INT64				t_PcrBase;
	TIM_27M				t_LastPcr;		//!< Dernier PCR lu (pour le PID PCR sélectionné)
	UINT16				pcr_pid;		//!< PID retenu pour le PCR
	UINT				nPacketCount;
	UINT				nCcErrCount;	//!< Comptage des erreurs de continuité
	UINT				nFlgErrCount;	//!< Comptage des indicateurs d'erreur placés
	UINT				nAllErrCount;	//!< Comptage des paquets en erreur (+1 même si 2 erreurs dans le paquet)

	CCChecker(UINT16 pcr_pref_pid = NO_PID) :
		t_PcrBase(HIGHEST_PCR_OFFSET),
		t_LastPcr(0),
		pcr_pid(pcr_pref_pid),
		nPacketCount(0),
		nCcErrCount(0),
		nFlgErrCount(0),
		nAllErrCount(0)
	{
		memset(aCC, 0xff, sizeof(aCC));
	}

	/// Transformer un marqueur de temps pour l'ajuster à la base
	TIM_27M	time_adj(TIM_27M t);

	/**
	 * \brief		Vérification d'un paquet TS pour les erreurs et la continuité
	 * \param[in]	sTs		Paquet à vérifier
	 * \return		PCR brut, ou bien #HIGHEST_PCR_OFFSET si aucun PCR dans ce paquet
	 **/
	Result	CheckTS(const TS_packet & sTs);

	/// Rapport des erreurs rencontrées
	void	OutReport(tostream & os, LPCTSTR pszIdt) const;

	/// Somme des erreurs de continuité et indicateurs d'erreurs
	UINT	errcount() const {
				return nCcErrCount + nFlgErrCount; }

	/// Obtenir le temps ajusté de la vérification
	TIM_27M	getRelativeTime() {
				return time_adj(t_LastPcr); }
};

// ====================================================================================

inline tostream & operator << (tostream & os, const CCChecker::Result & sChkRes)
{
	return sChkRes.OutStream(os);
}

// ====================================================================================