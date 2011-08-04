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

/** \file
 *	\brief Définitions nécessaires à la génération de fichiers projet VideoRedo
 *
 *	Ce fichier définit des classes permettant de générer des fichiers projet
 *	VideoRedo simplifiés, en y ajoutant des marqueurs temporels.
 **/

// ====================================================================================

struct TimeMark
{
	INT64	nTime;	// en unités de 100µS

	bool operator == (const TimeMark & sT2) const {
			return _abs64(sT2.nTime - nTime) < 250 * 10000; }
	bool operator < (const TimeMark & sT2) const {
			return nTime < sT2.nTime; }
};

/// Mémorisation des marqueurs de temps
class TimeMarkVect : public vector<TimeMark>
{
public:
	void Add10M(INT64 nNewTMrk);

	void Add27M(TIM_27M	tNewTMrk) {
			Add10M(tNewTMrk * 10 /27); }

	void sort() {
			::sort(begin(), end()); }

};

typedef TimeMarkVect::const_iterator	TimeMarkIter;

// ====================================================================================

class CVideoRedoPrj
{
	TimeMarkVect	vTimeMarks;
	string			strTsFileName;
	INT64			nDuration;
public:
	CVideoRedoPrj(LPCTSTR pszTsFileName, TimeMarkVect & vTimeMrks, INT64 nDurat) :
		vTimeMarks(vTimeMrks),
		nDuration(nDurat)
	{
		strcpy_T(strTsFileName, pszTsFileName);
	}

	bool Generate();
};

// ====================================================================================
