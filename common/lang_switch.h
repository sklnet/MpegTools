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
 *	\brief Définitions de macros de sélection de langue
 *
 *	Ici sont définies les fonctionnalités nécessaires à permettre de compiler
 *	l'application dans deux langues différentes (français et anglais).
 **/

// ====================================================================================

#define LANG_FR 1036
#define LANG_EN 1033

#if LANG==LANG_FR
#define _TL(en,fr)	_T(fr)
#else	// LANG==LANG_EN
#define _TL(en,fr)	_T(en)
#endif

// ====================================================================================

