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
 *	Fichier Include pour les fichiers Include système standard,
 *	ou les fichiers Include spécifiques aux projets qui sont utilisés fréquemment,
 *	et sont rarement modifiés
 **/

#pragma once

// Modifiez les définitions suivantes si vous devez cibler une plate-forme avant celles spécifiées ci-dessous.
// Reportez-vous à MSDN pour obtenir les dernières informations sur les valeurs correspondantes pour les différentes plates-formes.
#ifndef WINVER				// Autorise l'utilisation des fonctionnalités spécifiques à Windows XP ou version ultérieure.
#define WINVER 0x0501		// Attribuez la valeur appropriée à cet élément pour cibler d'autres versions de Windows.
#endif

#ifndef _WIN32_WINNT		// Autorise l'utilisation des fonctionnalités spécifiques à Windows XP ou version ultérieure.
#define _WIN32_WINNT 0x0501	// Attribuez la valeur appropriée à cet élément pour cibler d'autres versions de Windows.
#endif

#ifndef _WIN32_WINDOWS		// Autorise l'utilisation des fonctionnalités spécifiques à Windows 98 ou version ultérieure.
#define _WIN32_WINDOWS 0x0410 // Attribuez la valeur appropriée à cet élément pour cibler Windows Me ou version ultérieure.
#endif

#ifndef _WIN32_IE			// Autorise l'utilisation des fonctionnalités spécifiques à Internet Explorer 6.0 ou version ultérieure.
#define _WIN32_IE 0x0600	// Attribuez la valeur appropriée à cet élément pour cibler d'autres versions d'Internet Explorer.
#endif

// #define WIN32_LEAN_AND_MEAN		// Exclure les en-têtes Windows rarement utilisés

#define USE_SHARED_PTR 1

// Fichiers d'en-tête Windows :
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <windowsx.h>

// Fichiers d'en-tête C RunTime
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
#include "console_select.h"
#include "mpeg2defs.h"
#include "shared_ptr_ptvm.h"
