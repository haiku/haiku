/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef CURSOR_MANAGER_H
#define CURSOR_MANAGER_H


#include <List.h>
#include <Locker.h>

#include <TokenSpace.h>

#include "CursorSet.h"

using BPrivate::BTokenSpace;
class ServerCursor;


/*!
	\class CursorManager CursorManager.h
	\brief Handles almost all cursor management functions for the system
	
	The Cursor manager provides system cursor support, previous unseen on
	any BeOS platform. It also provides tokens for BCursors and frees all
	of an application's cursors whenever an application closes.
*/
class CursorManager : public BLocker {
	public:
						CursorManager();
		virtual			~CursorManager();

		int32			AddCursor(ServerCursor* cursor, int32 token = -1);
		void			DeleteCursors(team_id team);

		void			ReleaseCursor(ServerCursor* cursor);

		void			SetCursorSet(const char* path);
		ServerCursor*	GetCursor(cursor_which which);
		cursor_which	GetCursorWhich();
		void			ChangeCursor(cursor_which which, int32 token);
		void			SetDefaults();
		ServerCursor*	FindCursor(int32 token);

	private:
		void			_RemoveCursor(ServerCursor* cursor);
		ServerCursor*	_RemoveCursor(int32 index);

		BList			fCursorList;
		BTokenSpace		fTokenSpace;

		// System cursor members
		ServerCursor	*fDefaultCursor,
						*fTextCursor,
						*fMoveCursor,
						*fDragCursor,
						*fResizeCursor,
						*fNWSECursor,
						*fNESWCursor,
						*fNSCursor,
						*fEWCursor;
		cursor_which	fCurrentWhich;
};

#endif	/* CURSOR_MANAGER_H */
