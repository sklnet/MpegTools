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
#include "merge_ts.h"
#include "../common/vredo_prj.h"

// ====================================================================================

struct TS_packetRef
{
	const TS_packet_Ex *	ptr;

	bool	operator == (const TS_packetRef & sTs2) const {
				return memcmp(ptr, sTs2.ptr, sizeof(*ptr)) == 0; }
	bool	operator != (const TS_packetRef & sTs2) const {
				return memcmp(ptr, sTs2.ptr, sizeof(*ptr)) != 0; }
	bool	isTS(bool single=false) const {
				return ptr && ptr->isTS(single); }

	bool	error() const {
				return ptr->error(); }

	const TS_hdr &	operator () () const {
						return ptr->hdr; }
	operator const TS_packet_Ex & () const {
						return *ptr; }

	TS_packetRef() :
		ptr(NULL)
	{
	}

	TS_packetRef(const TS_packet_Ex & sTs) :
		ptr(&sTs)
	{
	}
};

// ====================================================================================

class TS_pack_lptr : public vector<TS_packetRef>
{
public:
	const CTsGrpFileReader *	pRefFile;	//!< Fichier source correspontant
	UINT						nErrCount;	//!< Nombre d'erreurs dans cette section

	TS_pack_lptr(const CTsGrpFileReader * pRfFile) :
		pRefFile(pRfFile),
		nErrCount(0)
	{
	}

	/// Ajout d'une référence de paquet TS, avec incrément du compteur d'erreurs si ce paquet comptient une erreur
	void Add(const TS_packetRef & sTs);

	void clear() {
		__super::clear(); nErrCount = 0; }
};

/// Ajout d'une référence de paquet TS, avec incrément du compteur d'erreurs si ce paquet comptient une erreur
void TS_pack_lptr::Add(const TS_packetRef & sTs)
{
	if (sTs.error())
		++nErrCount;
	push_back(sTs);
}

// ====================================================================================
// Utilitaires
// ====================================================================================

/// Retourne la chaîne fournie sans le nom de fichier final
tstring ExtractPath(LPCTSTR pszPathName)
{
	LPCTSTR	pszFNam = PathFindFileName(pszPathName);

	return tstring(pszPathName, pszFNam - pszPathName);
}

/// Crée un nom de fichier qui combine les deux noms de fichier fournis
tstring MakeFileNameByLCS(LPCTSTR pszStr1, LPCTSTR pszStr2)
{
	LPCTSTR	pszFNam1 = PathFindFileName(pszStr1);
	LPCTSTR	pszFNam2 = PathFindFileName(pszStr2);
	tstring strPath(pszStr1, pszFNam1 - pszStr1);

	if (strPath.empty())
		strPath.assign(pszStr2, pszFNam2 - pszStr2);

	typedef TCHAR	elem;
	typedef tstring	sequence;

	sequence A(pszFNam1);
	sequence B(pszFNam2);

	dtl::Diff< elem, sequence > d(A, B);
	//d.onOnlyEditDistance();
	d.compose();

	// editDistance
	UINT nEditDistance = (UINT)d.getEditDistance();

	// Longest Common Subsequence
	vector< elem > lcs_v = d.getLcsVec();

	return strPath + sequence(lcs_v.begin(), lcs_v.end());;
}

// ====================================================================================

CTsFileWriter::CTsFileWriter(CLogStreamBase & cLg, LPCTSTR pszFileName) :
	FileMapping(true, pszFileName, WRITE_MAP_SIZE),
	cLog(cLg),
	strFileName(pszFileName)
{
	if (!isValid()) {
		cLog << _TL("Impossible de créer le fichier « ","Cannot create file “") << pszFileName << _TL("”"," »") << endl;
		return;
	}
}

void CTsFileWriter::WritePacket(const TS_packet & sTs) throw(...)
{
	CCChecker::Result sRes = CheckTS(sTs);

	cLog << sRes;
	if (sRes.error())
		vTimeMarks.Add27M(sRes.t_RelPcr);

	remap(sMm, sizeof(sTs));

	memcpy((void *)sMm.ptr, &sTs, sizeof(sTs));
	sMm += sizeof(sTs);
}

bool CTsFileWriter::ProcessUntil(CTsGrpFileReader & cSrcF, bool bWrite, TIM_27M t_Limit) throw(...)
{
	do {
		if (bWrite)
			WriteSeq(cSrcF.vData);
		else
			cSrcF.vData.clear();
		cLog.CheckCancelState();
		if (!cSrcF.LoadGroup())
			return false;
	} while (cSrcF.vData.t_End < t_Limit);
	return true;
}


CTsFileWriter::~CTsFileWriter()
{
	SetEndOfFile(sMm);
}

// ====================================================================================

tstring	CMergeTsProcessor::Params::SuggestDstName()
{
	return MakeFileNameByLCS(getSrcFile(0), getSrcFile(1));
}

/// Si le nom de fichier de destination n'a pas de chemin, lui en fournir un d'après
/// ceux des noms des fichiers source
tstring CMergeTsProcessor::Params::CompleteDstName()
{
	tstring strPath(ExtractPath(strDstFile.c_str()));

	if (strPath.empty()) {
		for (ITERATE_CONST_VECTOR(vstrSrcFiles, tstring, it)) {
			strPath = ExtractPath(it->c_str());
			if (! strPath.empty())
				break;
		}

		if (!strPath.empty())
			return strPath + strDstFile;
	}
	return strDstFile;
}

// ====================================================================================

void CMergeTsProcessor::MergeAppend(TS_pack_lptr & vSrc, TS_pack_lptr & vOther, TS_pack_lptr & vFinal) throw(...)
{
	if (vSrc.pRefFile)
		cLog_merge << _TL(", using ",", utilise ") << vSrc.pRefFile->strFileId;
	cLog_merge << endl;

	for (ITERATE_CONST_VECTOR(vSrc, TS_packetRef, ita)) {
		TS_packetRef pTs = *ita;
		vFinal.Add(pTs);
	}
	vSrc.clear();
	vOther.clear();
}

void CMergeTsProcessor::LogMissing(LONG nCount, LPCTSTR pszFileId)
{
	if (nCount < 2) {
		cLog_merge << _T("\t\t") <<
			_TL("1 missing packet in ","1 paquet manquant dans ");
	} else
		cLog_merge << _T("\t\t") << nCount <<
		_TL(" missing packets in "," paquets manquants dans ");
	cLog_merge << pszFileId;
}

void CMergeTsProcessor::LogUnresolved(const TS_packet & sTs1, const TS_packet & sTs2)
{
	TIM_27M	t_Tmp = cOutF.getRelativeTime();

	cLog_out << _T("@") << Clk27M(t_Tmp)
		<< _TL(	":\tEmpirically resolved conflict in merging",
				":\tConflit résolu empiriquement dans la fusion") << endl
		<< _TL(	"\t\tYou should check your output file",
				"\t\tVous devriez vérifier votre fichier de sortie") << endl;
	cOutF.vTimeMarks.Add27M(t_Tmp);
}

/// Traitement de la fusion de deux branches qui n'ont rien en commun
void CMergeTsProcessor::MergeDiff(TS_pack_lptr & vSubSq1, TS_pack_lptr & vSubSq2, TS_pack_lptr & vFinal) throw(...)
{
	if (vSubSq2.empty() && vSubSq1.empty())
		return;	// rien à fusionner

	// Séquence en conflit détectée
	UINT nSiz1 = (UINT)vSubSq1.size();
	UINT nSiz2 = (UINT)vSubSq2.size();

	cLog_merge
		<< _TL("\t\tConflict, len1=","\t\tConflit, lng1=") << nSiz1 << _T(", err1=") << vSubSq1.nErrCount
		<< _TL(", len2=",", lng2=") << nSiz2 << _T(", err2=") << vSubSq2.nErrCount;

	if (vSubSq1.nErrCount < vSubSq2.nErrCount) {
		// La séquence de gauche a moins d'erreurs que la séquence de droite :
		// on retient la séquence de gauche
		MergeAppend(vSubSq1, vSubSq2, vFinal);
		return;
	}
	if (vSubSq1.nErrCount > vSubSq2.nErrCount) {
		// La séquence de droite a moins d'erreurs que la séquence de gauche :
		// on retient la séquence de droite
		MergeAppend(vSubSq2, vSubSq1, vFinal);
		return;
	}

	// Nombre d'erreurs identique

	if (nSiz1 > nSiz2) {
		// La séquence de gauche est plus longue que la séquence de droite :
		// on retient la séquence de gauche
		MergeAppend(vSubSq1, vSubSq2, vFinal);
		return;
	} 
	if (nSiz2 > nSiz1) {
		// La séquence de droite est plus longue que la séquence de gauche :
		// on retient la séquence de droite
		MergeAppend(vSubSq2, vSubSq1, vFinal);
		return;
	}

	// Les deux séquences sont de longueur identique et ont le même décompte d'erreurs :
	// on ajoute les paquets, un par un, d'un côté ou de l'autre, en vérifiant les bits d'erreur

	TS_pack_lptr::const_iterator	it2 = vSubSq2.begin();
	UINT							nCnt1 = 0, nCnt2 = 0, nUnres = 0;
	tstringstream					strmDumps;	// Flot pour dump hexadécimal

	for (ITERATE_CONST_VECTOR(vSubSq1, TS_packetRef, it1)) {
		TS_packetRef	sTs1 = *it1;
		TS_packetRef	sTs2 = *it2;
		bool			bErr1 = sTs1.error();
		bool			bErr2 = sTs2.error();

		if (bErr1 != bErr2) {
			if (bErr2) {
				// Le paquet 2 a une erreur, on utilise le 1
				vFinal.Add(sTs1);
				++ nCnt1;
			} else {
				// Le paquet 1 a une erreur, on utilise le 2
				vFinal.Add(sTs2);
				++ nCnt2;
			}
		} else {
			// Les deux sections sont de même longueur, ne sont pas identiques et, soit n'ont aucune
			// erreur marquée, soit en ont exactement le même nombre  : on va (empiriquement) résoudre
			// en faveur du fichier qui a le meilleur historique d'erreurs depuis le tout début.
			if (cMFil1.vData.nGrpErrCount < cMFil2.vData.nGrpErrCount) {
				// Le groupe de paquets a moins d'erreurs dans le fichier 1
				vFinal.Add(sTs1);
				++nCnt1;
			} else if (cMFil1.vData.nGrpErrCount > cMFil2.vData.nGrpErrCount) {
				// Le groupe de paquets a moins d'erreurs dans le fichier 2
				vFinal.Add(sTs2);
				++nCnt2;
			} else if (cMFil1.errcount() < cMFil2.errcount()) {
				// Le fichier 1 a globalement moins d'erreurs
				vFinal.Add(sTs1);
				++nCnt1;
			} else if (cMFil1.errcount() > cMFil2.errcount()) {
				// Le fichier 2 a globalement moins d'erreurs
				vFinal.Add(sTs2);
				++nCnt2;
			} else {
				// Résolution impossible. Tant pis, on choisit arbitrairement le fichier 1
				vFinal.Add(sTs1);
				++nCnt1;
			}

			++nUnres;
			LogUnresolved(sTs1, sTs2);

			// Ajout d'un dump hexadécimal des deux paquets dans le fichier journal
			UINT nPkNum = (UINT)distance<TS_pack_lptr::const_iterator>(vSubSq1.begin(), it1) + 1;

			strmDumps << _TL("\tPacket #","\tContenu du paquet ") << nPkNum
				<< _TL(" contents for "," pour le ") << cMFil1.strFileId << _TL(":"," :") << endl
				<< sTs1.ptr->getHexDump(_T("\t>"));
			strmDumps << _TL("\tPacket #","\tContenu du paquet ") << nPkNum
				<< _TL(" contents for "," pour le ") << cMFil2.strFileId << _TL(":"," :") << endl
				<< sTs2.ptr->getHexDump(_T("\t>"));
		}
		++it2;
	}
	cLog_merge << _TL(", using ",", utilise ");
	if (nCnt1 == 0)
		cLog_merge << cMFil2.strFileId;
	else if (nCnt2 == 0)
		cLog_merge << cMFil1.strFileId;
	else
		cLog_merge << cMFil1.strFileId << _T(" (") << nCnt1 << _T(") & ")
			<< cMFil2.strFileId <<  _T(" (") << nCnt2 << _T(")");
	cLog_merge << endl;

	if (nUnres) {
		cLog_merge
			<< _TL(	"\t### Uncertain resolution (same length,",
					"\t### Résolution incertaine (même longueur,") << endl
			<< _TL(	"\t### same error count), packet count=",
					"\t### même nombre d'erreurs), nbr. paquets=") << nUnres << endl;
		// Lister le contenu des paquets dans le fichier journal seulement
		cLog_merge.log_full(false);
		cLog_merge << strmDumps.str();
		cLog_merge.log_full(true);
	}

	vSubSq1.clear();
	vSubSq2.clear();
}

void CMergeTsProcessor::Merge() throw(...)
{
	INT nSiz1 = (INT)cMFil1.vData.size();
	INT nSiz2 = (INT)cMFil2.vData.size();

	bool	bDiff = false;

	if (nSiz1 == nSiz2) {
		// Les deux listes ont la même taille : effectuer d'abord une comparaison rapide pour égalité.
		TS_pack_list::const_iterator it2 = cMFil2.vData.begin();

		for (ITERATE_CONST_VECTOR(cMFil1.vData, TS_packet_Ex, it)) {
			if (*it != *it2) {
				bDiff = true;
				break;
			}
			++it2;
		}

		if (!bDiff) {
			// Aucune différence : on valide directement.
			cOutF.WriteSeq(cMFil1.vData);
			cMFil1.clear();
			cMFil2.clear();
			return;
		}
		// Si au moins une différence trouvée, on recommence avec 'Diff'.
	}

	// Instancier le template de recherche de différences
	Diff<TS_packet_Ex> d(cMFil1.vData, cMFil2.vData);

	// Trouver les différences
	d.compose();

	// editDistance
	UINT nDist = (UINT)d.getEditDistance();

	if (nDist != 0) {
		cLog_merge << _T("#") << Clk27M(cOutF.time_adj(cMFil1.vData.t_Begin))
			<< _TL(":\tDiff, len1=",":\tDiff, lng1=") << nSiz1
			<< _TL(", len2=",", lng2=") << nSiz2 << _T(", diff:") << abs(nSiz1-nSiz2) << _T(", dist:") << nDist << endl;
#if 0
		// Écrire le Script d'édition le plus court dans le fichier seulement
		cLog_merge.log_full(false);
		cLog_merge << _TL("\tShortest Edit Script:","\tScript d'édition le plus court") << endl;
		d.printSES(cLog_merge);
		// d.printUnifiedFormat(cLog_merge);
		cLog_merge.log_full(true);
#endif
	}

	Ses<TS_packet_Ex>			ses = d.getSes();
	typedef pair<TS_packet_Ex, elemInfo>
								sesElem;
	Sequence<sesElem>::elemVec	seq = ses.getSequence();
	TS_pack_lptr				vSubSq1(&cMFil1);	// Séquence du fichier 1
	TS_pack_lptr				vSubSq2(&cMFil2);	// Séquence du fichier 2
	TS_pack_lptr				vFinal(NULL);		// Séquence de sortie

	for (ITERATE_CONST_VECTOR(seq, sesElem, it)) {
		TS_packetRef		sTs		= it->first;
		const eleminfo &	sInf	= it->second;

		switch(sInf.type) {

		case SES_DELETE:
			// Paquet présent uniquement dans le fichier 1
			vSubSq1.Add(sTs);
			break;

		case SES_ADD:
			// Paquet présent uniquement dans le fichier 2
			vSubSq2.Add(sTs);
			break;

		case SES_COMMON:
			// Paquet présent dans les deux fichiers
			// - traiter d'abord la fusion des paquets non communs qui ont précédé
			MergeDiff(vSubSq1, vSubSq2, vFinal);
			// Puis ajouter directement le paquet dans la séquence de sortie
			vFinal.Add(sTs);
		}
	}

	// Traiter la fusion des paquets non communs qui terminent la séquence
	MergeDiff(vSubSq1, vSubSq2, vFinal);
	cMFil1.clear();
	cMFil2.clear();
	// Envoi de la séquence de sortie
	cOutF.WriteSeq(vFinal);
}

// ====================================================================================

CMergeTsProcessor::CMergeTsProcessor(CLogStreamBase & cLog_mrg, CLogStreamBase & cLog_o,
									 Params & sPrms, UINT16 pcr_pref_pid, bool bDelOnExit) :
	WorkingThread(bDelOnExit),
	cLog_merge(cLog_mrg),
	cLog_out(cLog_o),
	cMFil1(cLog_mrg, sPrms.getSrcFile(0), _TL("file #1","fichier 1"), ID_FIL1, pcr_pref_pid),
	cMFil2(cLog_mrg, sPrms.getSrcFile(1), _TL("file #2","fichier 2"), ID_FIL2, pcr_pref_pid),
	cOutF(cLog_o, sPrms.strDstFile.c_str()),
	bUnionMode(sPrms.bUnion),
	bMakeVrdPrj(sPrms.bMakeVrdPrj)
{
}

void CMergeTsProcessor::SyncOverflow(CTsGrpFileReader & cMF_this, CTsGrpFileReader & cMF_other) throw(...)
{
	if (!cMF_other.vData.empty()) {
		cLog_merge
			<< _TL(	"## Missing part too large at ","## Partie manquante trop large à ")
			<< Clk27M(min<TIM_27M>(cOutF.time_adj(cMFil1.vData.t_Begin), cOutF.time_adj(cMFil2.vData.t_Begin))) << endl;
		cLog_merge
			<< _TL(	"## Using simple merging.","## Passage en fusion simplifiée.") << endl;
		cLog_merge << _T("## ") << cMF_other.vData.size()
			<< _TL(" discarded packets from ", " paquets éliminés depuis ") << cMF_other.strFileId << endl;
		cMF_other.vData.clear();
	}

	LogMissing((LONG)cMF_this.vData.size(), cMF_other.strFileId.c_str());
	cLog_merge << endl;
	cOutF.WriteSeq(cMF_this.vData);
}

void CMergeTsProcessor::SyncLoad() throw(...)
{
	while (cMFil1.vData.t_End != cMFil2.vData.t_End) {
		cMFil1.cLog.CheckCancelState();

		if (cMFil1.vData.size() >= 50000)
			SyncOverflow(cMFil1, cMFil2);
		else if (cMFil2.vData.size() >= 50000)
			SyncOverflow(cMFil2, cMFil1);

		// Time stamp manquant d'un côté ou de l'autre : on ajoute des données
		// des deux côtés jusqu'à rencontrer une nouvelle coincidence
		if (TIM_27M_Distance(cMFil1.vData.t_End, cMFil2.vData.t_End) > 0)
			cMFil1.LoadGroup();
		else
			cMFil2.LoadGroup();

		if (cMFil1.bEof || cMFil2.bEof)
			return;
	}

	if (cMFil1.vData.empty() && cMFil2.vData.empty())
		throw (DWORD)ERROR_BAD_FORMAT;

	if (cMFil1.vData.t_End != cMFil2.vData.t_End)
		// Internal error - checkpoint fault
		throw DWORD(-2);

	cMFil1.cLog.CheckCancelState();
}

bool CMergeTsProcessor::ProcessBegin(CTsGrpFileReader & cMF_this, CTsGrpFileReader & cMF_other) throw(...)
{
	if (bUnionMode) {
		cLog_merge
			<< _TL("## Using only ","## Utilise ")
			<< cMF_this.strFileId
			<< _TL(" until beginning of "," seul jusqu'à début ")
			<< cMF_other.strFileId << endl;
	} else {
		cLog_merge
			<< _TL("## Skipping ","## Ignore ")
			<< cMF_this.strFileId
			<< _TL(" until beginning of "," jusqu'à début ")
			<< cMF_other.strFileId << endl;
	}

	bool bEof = !cOutF.ProcessUntil(cMF_this, bUnionMode, cMF_other.vData.t_Begin);

	if (!bEof)
		bEof = !cMF_this.LoadGroup();
	return bEof;
}

bool CMergeTsProcessor::ProcessEnd(CTsGrpFileReader & cMF_this, CTsGrpFileReader & cMF_other) throw(...)
{
	cLog_merge
		<< _TL("## ","## ")
		<< cMF_other.strFileId
		<<  ( bUnionMode ?
			_TL(" ended, using remaining of "," terminé, utilise reste du ") :
			_TL(" ended, discarding remaining of "," terminé, ignore reste du ") )
		<< cMF_this.strFileId << endl;

	bool bEof = !cOutF.ProcessUntil(cMF_this, bUnionMode);

	if (!cMF_this.vData.empty())
		cOutF.WriteSeq(cMF_this.vData);
	return bEof;
}

void CMergeTsProcessor::Process() throw(...)
{
	if (!cOutF.isValid())
		throw DWORD(-1);

	cMFil1.reset();
	cMFil2.reset();

	cMFil1.LoadGroup();
	cMFil2.LoadGroup();

	// Ajustement de temps : prendre le marqueur de temps le plus petit comme base temporelle
	// pour le début du fichier.
	cOutF.time_adj(cMFil1.vData.t_Begin);
	cOutF.time_adj(cMFil2.vData.t_Begin);

	if (!cMFil1.bEof && !cMFil2.bEof) {
		if (TIM_27M_Distance(cMFil1.vData.t_End, cMFil2.vData.t_Begin) > 0) {
			if (!cMFil1.bEof)
				ProcessBegin(cMFil1, cMFil2);
		} else if (TIM_27M_Distance(cMFil2.vData.t_End, cMFil1.vData.t_Begin) > 0) {
			if (!cMFil2.bEof)
				ProcessBegin(cMFil2, cMFil1);
		}
		if (!cMFil1.bEof && !cMFil2.bEof) {
			cLog_merge
				<< _TL("## Common part found at ","## Partie commune trouvée à ")
				<< Clk27M(min<TIM_27M>(cOutF.time_adj(cMFil1.vData.t_Begin), cOutF.time_adj(cMFil2.vData.t_Begin)))
				<< endl;
			SyncLoad();
			Merge();
			if (!cMFil1.bEof && !cMFil2.bEof)
				cLog_merge
					<< _TL("## Now actively merging.","## Fusion active en cours.") << endl;
		} else {
			cLog_merge
				<< _TL("## No common part fond. Wild concatenation in progress.",
					"## Aucune partie commune trouvée. Concaténation sauvage en cours.") << endl;
		}
	}

	// Répéter tant qu'aucun des deux fichiers n'est à sa fin
	for (;;) {
		cMFil1.LoadGroup();
		cMFil2.LoadGroup();

		if (cMFil1.bEof || cMFil2.bEof)
			break;

		SyncLoad();
		Merge();
	}

	cLog_merge
		<< _TL("## End of common point at ","## Fin de partie commune à ")
		<< Clk27M(min<TIM_27M>(cOutF.time_adj(cMFil1.vData.t_Begin), cOutF.time_adj(cMFil2.vData.t_Begin)))
		<< endl;

	Merge();

	if (cMFil1.bEof)
		ProcessEnd(cMFil2, cMFil1);
	else if (cMFil2.bEof)
		ProcessEnd(cMFil1, cMFil2);

	//LogTsTime(cLog_merge, _TL("end ","fin"), cMFil1.vData.t_End, cMFil1.strFileId.c_str());
	//LogTsTime(cLog_merge, _TL("end ","fin"), cMFil2.vData.t_End, cMFil2.strFileId.c_str());
	cLog_merge <<	_TL("Done.", "Effectué.") << endl;

	cMFil1.LogErrors();
	cMFil2.LogErrors();
	cOutF.OutReport(cLog_out, _TL("output file","le fichier de sortie"));

	TIM_27M	t_Duration = cOutF.getRelativeTime();

	cLog_out
		<<	_TL("Total duration = ","Durée totale = ") << Clk27M(t_Duration) << endl;
	cLog_out
		<<	_TL("\tNote that this duration is solely based on the timestamps found inside the",
				"\tNotez que cette durée est uniquement basée sur les marqueur de temps") << endl
		<<	_TL("\tTS stream. If there are missing parts in the output file, the displayed value",
				"\ttrouvés dans le flux TS. Si des parties sont manquantes dans le fichier de") << endl
		<<	_TL("\twill probably be inaccurate.",
				"\tsortie, la valeur affichée sera probablement inexacte.") << endl;

	if (bMakeVrdPrj) {
		CVideoRedoPrj	cVrdPrj(cOutF.strFileName.c_str(), cOutF.vTimeMarks, t_Duration * 10 / 27);

		cLog_out << _TL("Generating VideoRedo project file","Génération du fichier projet VideoRedo") << endl;
		if (!cVrdPrj.Generate())
			cLog_out << _TL("Cannot create VideoRedo project file","Impossible de créer le fichier projet VideoRedo") << endl;
		else
			cLog_out << _TL("\tMark count\t=\t","\tNombre de marqueurs\t=\t") << cOutF.vTimeMarks.size() << endl;
	}
}

DWORD CMergeTsProcessor::ThreadProc()
{
	DWORD dwRes = 0;

	cLog_merge.PostMessage(WM_APP_PROCESS_BEG);

	cLog_out << _TL("Saving in progress.","Enregistrement en cours.") << endl;

	try {
		Process();
	} catch(DWORD dwErr) {
		dwRes = dwErr;
	}

	switch (dwRes) {

	case DWORD(-1):
		cLog_out << _TL("Saving stopped.","Enregistrement interrompu.") << endl;
		break;

	case 0:
		cLog_out << _TL("Saving terminated.","Enregistrement terminé.") << endl;
		break;

	default:
		cLog_out << _TL("Error 0x","Erreur 0x") << hex << dwRes << endl;
	}
	cLog_merge.PostMessage(WM_APP_PROCESS_END, dwRes);
	return dwRes;
}

// ====================================================================================
// Paramètres en ligne de commande
// ====================================================================================

bool CTsMergerCmdLine::ProcessArg(tstring & strArg)
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

			case 'u':
				bUnion = true;
				break;

			case 'i':
				bUnion = false;
				break;

			default:
				return false;
			}
		}
	} else if (vstrSrcFiles.size() < 2)
		vstrSrcFiles.push_back(strArg);
	else if (strDstFile.empty())
		strDstFile = strArg;
	else
		return false;
	return true;
}

// ====================================================================================