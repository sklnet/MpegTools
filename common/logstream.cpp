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
#include "logstream.h"

// ====================================================================================

int	CLogStreamBuf::sync()
{
	DWORD		nWriteSize	= (DWORD)(pptr()-aOBuffer);

	if (nWriteSize) {
		aOBuffer[min<DWORD>(nWriteSize, _countof(aOBuffer)-1)] = 0;

		Output(aOBuffer);
	}

	// Redéfinir les pointeurs sur le tampon de sortie, celui-ci étant à nouveau libre
	pubsetbuf(aOBuffer, _countof(aOBuffer)-1);
	return 0;
}

CLogStreamBuf::int_type CLogStreamBuf::overflow(int_type nChar)
{
	if (Tr::eq_int_type(Tr::eof(), nChar))
		return Tr::not_eof(nChar);

	if (pptr() >= epptr())
		sync();

	*pptr() = Tr::to_char_type(nChar);
	pbump(1);
	return Tr::not_eof(nChar);
}

// ====================================================================================
