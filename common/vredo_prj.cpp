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

/** \file
 *	\brief Implémentation des méthodes nécessaires à la génération de fichiers projet VideoRedo
 *
 *	Ce fichier implémente les fonctionnalités permettant de générer des fichiers projet
 *	VideoRedo simplifiés, en y ajoutant des marqueurs temporels.
 **/

// ====================================================================================

#include "stdafx.h"
#include "vredo_prj.h"
#include "xmlutils.h"

// ====================================================================================

void TimeMarkVect::Add10M(INT64 nNewTMrk)
{
	TimeMark	sTm = {nNewTMrk};

	TimeMarkIter	itb = begin();
	TimeMarkIter	ite = end();
	TimeMarkIter	itf = find(itb, ite, sTm);

	if (itf == ite)
		push_back(sTm);
}

// ====================================================================================

TiXmlElement * FindOrAddElement(TiXmlNode * pXmlParent, LPCSTR pszName)
{
	TiXmlElement * pXmlElm = pXmlParent->FirstChildElement(pszName);

	if (!pXmlElm)
		pXmlParent->LinkEndChild(pXmlElm = new TiXmlElement(pszName));
	return pXmlElm;
}

template <typename T>
	TiXmlElement *  SetAttributeIfNotSet(TiXmlElement * pXmlElm, LPCSTR pszName, T value)
{
	LPCSTR pszOldVal = pXmlElm->Attribute(pszName);

	if (!pszOldVal)
		pXmlElm->SetAttribute(pszName, value);
	return pXmlElm;
}

TiXmlElement * SetTextIfNotSet(TiXmlElement * pXmlElm, LPCSTR pszText)
{
	const TiXmlNode	* pXmlChild = NULL;

	while ((pXmlChild = pXmlElm->IterateChildren(pXmlChild)) && pXmlChild->Type() != TiXmlNode::TEXT);

	if (!pXmlChild)
		pXmlElm->LinkEndChild(new TiXmlText(pszText));
	return pXmlElm;
}

// ====================================================================================

/// Création ou mise à jour du fichier projet VideoRedo
bool CVideoRedoPrj::Generate()
{
	CHAR	szWrk[32];

	// Construction du nom de fichier projet
	class string_local : public string
	{
	public:
		string_local(LPCSTR pszTsFn) {
			assign(pszTsFn, PathFindExtensionA(pszTsFn) - pszTsFn);
			append(".Vprj");
		}
	}				strVpFileName(strTsFileName.c_str());

	// Document XML (Vprj)
	TiXmlDocument	sXmlDoc(strVpFileName.c_str());

	// Chargement du document antérieur, s'il existe
	sXmlDoc.LoadFile();

	// Si erreur (le document n'existe pas ou n'est pas valide), réinitialiser pour en préparer un nouveau
	if (sXmlDoc.Error())
		sXmlDoc.ClearError();

	// Noeud racine
	TiXmlElement *	pXmlElmPrj = FindOrAddElement(&sXmlDoc, "VideoReDoProject");

	SetAttributeIfNotSet(pXmlElmPrj, "Version", 3);

	//SetTextIfNotSet(
	//	FindOrAddElement(
	//		SetTextIfNotSet(
	//			FindOrAddElement(pXmlElmPrj, "VideoReDoVersion"),
	//			"3.20.2.609 - Nov 16 2010"),
	//		"BuildNumber"), "609");

	// Ajout du nom du fichier s'il n'existe pas
	SetTextIfNotSet(FindOrAddElement(pXmlElmPrj, "Filename"), strTsFileName.c_str());

	// Conversion en chaîne de la durée
	_i64toa_s(nDuration, szWrk, _countof(szWrk), 10);

	// Autres éléments à ajouter
	SetTextIfNotSet(FindOrAddElement(pXmlElmPrj, "Duration"), szWrk);
	SetTextIfNotSet(
		FindOrAddElement(pXmlElmPrj, "ProjectTime"), szWrk);
	SetTextIfNotSet(
		FindOrAddElement(
			FindOrAddElement(pXmlElmPrj, "OpenStreamParams"), "InputFilename"),
			strTsFileName.c_str() );

	// Trouver ou créer le noeud "SceneList"
	TiXmlElement * pXmlScnLst = FindOrAddElement(pXmlElmPrj, "SceneList");
	TiXmlElement * pXmlScnMrk;

	// Récupérer les "SceneMarker" antérieurs, et les détruire au passage
	while (pXmlScnMrk = pXmlScnLst->FirstChildElement("SceneMarker")) {
		LPCSTR pszTxt = pXmlScnMrk->GetText();

		if (pszTxt) {
			INT64 nOldMrk = _strtoi64(pszTxt, NULL, 10);

			vTimeMarks.Add10M(nOldMrk);
			myprintf(_T("Text = %") A2t EOL, pszTxt);
		}
		pXmlScnLst->RemoveChild(pXmlScnMrk);
	}

	// Trier les "SceneMarker", puisque les anciens peuvent avoir été mélangés
	// avec les nôtres
	vTimeMarks.sort();

	// Recréer une nouvelle liste de "SceneMarker"
	for (ITERATE_CONST_VECTOR(vTimeMarks, TimeMark, it)) {

		pXmlScnMrk = new TiXmlElement("SceneMarker");

		pXmlScnMrk->SetAttribute("Sequence", (int)distance<TimeMarkIter>(vTimeMarks.begin(), it)+1);
		sprintf_s(szWrk, "%02u:%02u:%02u;%02u",
			UINT(it->nTime / 36000000000),
			UINT(it->nTime /   600000000) % 60,
			UINT(it->nTime /    10000000) % 60,
			UINT(it->nTime /      100000) % 100);
		pXmlScnMrk->SetAttribute("Timecode", szWrk);
		_i64toa_s(it->nTime, szWrk, _countof(szWrk), 10);
		pXmlScnMrk->LinkEndChild(new TiXmlText(szWrk));
		pXmlScnLst->LinkEndChild(pXmlScnMrk);
	}

	// Enregistrer le fichier créé ou modifié
	return sXmlDoc.SaveFile();
}
