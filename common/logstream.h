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

typedef std::basic_ostream<TCHAR>	tostream;
typedef std::basic_streambuf<TCHAR> tstreambuf;

// ====================================================================================

class CLogStreamBase : public tostream
{
public:
	HWND		hWnd;

	CLogStreamBase(tstreambuf * buf) :
		tostream(buf),
		hWnd(NULL)
	{
	}

	virtual	void	log_full(bool bOn) {
						}

	virtual void	ProgressStep(INT nValue, INT nIndex) {
						}

	virtual void	CheckCancelState() throw(...) {
						}

	bool			PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = NULL) const {
						return hWnd ? ::PostMessage(hWnd, uMsg, wParam, lParam) == TRUE : false; }
};

// ====================================================================================

typedef basic_ofstream<TCHAR> tofstream;

// ====================================================================================

class CLogStreamBuf : public tstreambuf
{
	friend class CLogStream;

	TCHAR				aOBuffer[0x100];
protected:
	bool				bFull;
private:

	typedef traits_type	Tr;

	virtual int			sync();
	virtual int_type	overflow(int_type = Tr::eof());

	virtual tstreambuf*	setbuf(LPTSTR pBuf, streamsize nSiz) {
							setp(pBuf, pBuf+nSiz); return this; }

	virtual void		Output(LPCTSTR pszTxt) = 0;

public:
	CLogStreamBuf() :
		bFull(true)
	{
		// Définir les pointeurs sur le tampon de sortie
		pubsetbuf(aOBuffer, _countof(aOBuffer)-1);
	}
};

// ====================================================================================