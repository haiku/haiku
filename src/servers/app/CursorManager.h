/*
 * Copyright 2001-2010, Haiku.
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
								~CursorManager();

			ServerCursor*		CreateCursor(team_id clientTeam,
									 const uint8* cursorData);
			ServerCursor*		CreateCursor(team_id clientTeam,
									BRect r, color_space format, int32 flags,
									BPoint hotspot, int32 bytesPerRow = -1);

			int32				AddCursor(ServerCursor* cursor,
									int32 token = -1);
			void				DeleteCursors(team_id team);

			bool				RemoveCursor(ServerCursor* cursor);

			void				SetCursorSet(const char* path);
			ServerCursor*		GetCursor(BCursorID which);

			ServerCursor*		FindCursor(int32 token);

private:
			void				_InitCursor(ServerCursor*& cursorMember,
									const uint8* cursorBits, BCursorID id,
									const BPoint& hotSpot = B_ORIGIN);
			void				_LoadCursor(ServerCursor*& cursorMember,
									const CursorSet& set, BCursorID id);
			ServerCursor*		_FindCursor(team_id cientTeam,
									const uint8* cursorData);
			void				_RemoveCursor(ServerCursor* cursor);

private:
			BList				fCursorList;
			BTokenSpace			fTokenSpace;

			// System cursor members
			ServerCursor*		fCursorSystemDefault;

			ServerCursor*		fCursorContextMenu;
			ServerCursor*		fCursorCopy;
			ServerCursor*		fCursorCreateLink;
			ServerCursor*		fCursorCrossHair;
			ServerCursor*		fCursorFollowLink;
			ServerCursor*		fCursorGrab;
			ServerCursor*		fCursorGrabbing;
			ServerCursor*		fCursorHelp;
			ServerCursor*		fCursorIBeam;
			ServerCursor*		fCursorIBeamHorizontal;
			ServerCursor*		fCursorMove;
			ServerCursor*		fCursorNoCursor;
			ServerCursor*		fCursorNotAllowed;
			ServerCursor*		fCursorProgress;
			ServerCursor*		fCursorResizeEast;
			ServerCursor*		fCursorResizeEastWest;
			ServerCursor*		fCursorResizeNorth;
			ServerCursor*		fCursorResizeNorthEast;
			ServerCursor*		fCursorResizeNorthEastSouthWest;
			ServerCursor*		fCursorResizeNorthSouth;
			ServerCursor*		fCursorResizeNorthWest;
			ServerCursor*		fCursorResizeNorthWestSouthEast;
			ServerCursor*		fCursorResizeSouth;
			ServerCursor*		fCursorResizeSouthEast;
			ServerCursor*		fCursorResizeSouthWest;
			ServerCursor*		fCursorResizeWest;
			ServerCursor*		fCursorZoomIn;
			ServerCursor*		fCursorZoomOut;
};

#endif	// CURSOR_MANAGER_H
