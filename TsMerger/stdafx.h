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
#ifndef WINVER
  #ifdef _DEBUG
	#define WINVER _WIN32_WINNT_VISTA	// Autorise l'utilisation des fonctionnalit�s sp�cifiques � Windows�Vista ou version ult�rieure.
  #else // _DEBUG
	#define WINVER _WIN32_WINNT_WINXP	// Autorise l'utilisation des fonctionnalit�s sp�cifiques � Windows�XP ou version ult�rieure.
  #endif // _DEBUG
#endif // WINVER						// Attribuez la valeur appropri�e � cet �l�ment pour cibler d'autres versions de Windows.

#define _WIN32_WINNT WINVER

#ifndef _WIN32_IE			
	#define _WIN32_IE _WIN32_IE_IE60	// Autorise l'utilisation des fonctionnalit�s sp�cifiques � Internet Explorer�6.0 ou version ult�rieure.
#endif									// Attribuez la valeur appropri�e � cet �l�ment pour cibler d'autres versions d'Internet Explorer.

// #define WIN32_LEAN_AND_MEAN			// Exclure les en-t�tes Windows rarement utilis�s

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

#include "../config.h"
#include "utils.h"
#include "rctutils.h"
#include "console_select.h"
#include "mpeg2defs.h"
#include "shared_ptr_ptvm.h"

#if _MSC_VER >= 1900
	// N�cessaire � partir de C++1 (VS 2015 et +)
	#pragma comment (lib, "legacy_stdio_definitions.lib")
#endif	// _MSC_VER