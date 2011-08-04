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
 *	Fichier Include pour les fichiers Include syst�me standard,
 *	ou les fichiers Include sp�cifiques aux projets qui sont utilis�s fr�quemment,
 *	et sont rarement modifi�s
 **/

#pragma once

// Modifiez les d�finitions suivantes si vous devez cibler une plate-forme avant celles sp�cifi�es ci-dessous.
// Reportez-vous � MSDN pour obtenir les derni�res informations sur les valeurs correspondantes pour les diff�rentes plates-formes.
#ifndef WINVER				// Autorise l'utilisation des fonctionnalit�s sp�cifiques � Windows�XP ou version ult�rieure.
#define WINVER 0x0501		// Attribuez la valeur appropri�e � cet �l�ment pour cibler d'autres versions de Windows.
#endif

#ifndef _WIN32_WINNT		// Autorise l'utilisation des fonctionnalit�s sp�cifiques � Windows�XP ou version ult�rieure.
#define _WIN32_WINNT 0x0501	// Attribuez la valeur appropri�e � cet �l�ment pour cibler d'autres versions de Windows.
#endif

#ifndef _WIN32_WINDOWS		// Autorise l'utilisation des fonctionnalit�s sp�cifiques � Windows�98 ou version ult�rieure.
#define _WIN32_WINDOWS 0x0410 // Attribuez la valeur appropri�e � cet �l�ment pour cibler Windows�Me ou version ult�rieure.
#endif

#ifndef _WIN32_IE			// Autorise l'utilisation des fonctionnalit�s sp�cifiques � Internet Explorer�6.0 ou version ult�rieure.
#define _WIN32_IE 0x0600	// Attribuez la valeur appropri�e � cet �l�ment pour cibler d'autres versions d'Internet Explorer.
#endif

// #define WIN32_LEAN_AND_MEAN		// Exclure les en-t�tes Windows rarement utilis�s

#define USE_SHARED_PTR 1

// Fichiers d'en-t�te Windows�:
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <windowsx.h>

// Fichiers d'en-t�te C RunTime
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <strsafe.h>

using namespace std;

#include "utils.h"
#include "rctutils.h"
#include "console.h"
#include "mpeg2defs.h"
#include "shared_ptr_ptvm.h"
