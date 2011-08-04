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

#include "stdafx.h"
#include "TsCheckerDlg.h"

// ====================================================================================
// Variables globales
// ====================================================================================

HINSTANCE	hAppInstance = NULL;		//!< Handle d'instance de l'application

// ====================================================================================
// Point d'entrée du programme
// ====================================================================================

int APIENTRY _tWinMain(	HINSTANCE	hInstance,
						HINSTANCE	hPrevInstance,
						LPTSTR		lpCmdLine,
						int			nCmdShow)
{
	hAppInstance = hInstance;

	INITCOMMONCONTROLSEX icx = {
		sizeof(icx),
		ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS
	};

	InitCommonControlsEx(&icx);

	CTsCheckerCmdLine	cCmdLine(GetCommandLine());

	cCmdLine.ProcessAll();

	INT_PTR				nResult = CTsCheckerDialog(cCmdLine).Do(NULL);	// Ouverture du dialogue

	if (nResult < 0)
		nResult = GetLastError();

	return (int)nResult;
}
