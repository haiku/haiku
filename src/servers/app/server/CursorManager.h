//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		CursorManager.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handles the system's cursor infrastructure
//  
//------------------------------------------------------------------------------
#ifndef CURSORMANAGER_H_
#define CURSORMANAGER_H_

#include <List.h>
#include <OS.h>
#include <SysCursor.h>
#include "TokenHandler.h"

class ServerCursor;

/*!
	\class CursorManager CursorManager.h
	\brief Handles almost all cursor management functions for the system
	
	The Cursor manager provides system cursor support, previous unseen on
	any BeOS platform. It also provides tokens for BCursors and frees all
	of an application's cursors whenever an application closes.
*/
class CursorManager
{
public:
	CursorManager(void);
	~CursorManager(void);
	int32 AddCursor(ServerCursor *sc);
	void DeleteCursor(int32 token);
	void RemoveAppCursors(const char *signature);
	void ShowCursor(void);
	void HideCursor(void);
	void ObscureCursor(void);
	void SetCursor(int32 token);
	void SetCursor(cursor_which which);
	ServerCursor *GetCursor(cursor_which which);
	cursor_which GetCursorWhich(void);
	void ChangeCursor(cursor_which which, int32 token);

private:
	ServerCursor *_FindCursor(int32 token);

	BList *_cursorlist;
	TokenHandler _tokenizer;
	sem_id _lock;

	// System cursor members
	ServerCursor 	*_defaultcsr,
					*_textcsr,
					*_movecsr,
					*_dragcsr,
					*_resizecsr,
					*_resize_nwse_csr,
					*_resize_nesw_csr,
					*_resize_ns_csr,
					*_resize_ew_csr;
	cursor_which _current_which;
};

extern CursorManager *cursormanager;

#endif
