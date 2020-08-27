/*
 * Copyright 2001-2016, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */


/*!	Handles the system's cursor infrastructure */


#include "CursorManager.h"

#include "CursorData.h"
#include "ServerCursor.h"
#include "ServerConfig.h"
#include "ServerTokenSpace.h"

#include <Autolock.h>
#include <Directory.h>
#include <String.h>

#include <new>
#include <stdio.h>


CursorManager::CursorManager()
	:
	BLocker("CursorManager")
{
	// Init system cursors
	const BPoint kHandHotspot(1, 1);
	const BPoint kResizeHotspot(8, 8);
	_InitCursor(fCursorSystemDefault, kCursorSystemDefaultBits,
		B_CURSOR_ID_SYSTEM_DEFAULT, kHandHotspot);
	_InitCursor(fCursorContextMenu, kCursorContextMenuBits,
		B_CURSOR_ID_CONTEXT_MENU, kHandHotspot);
	_InitCursor(fCursorCopy, kCursorCopyBits,
		B_CURSOR_ID_COPY, kHandHotspot);
	_InitCursor(fCursorCreateLink, kCursorCreateLinkBits,
		B_CURSOR_ID_CREATE_LINK, kHandHotspot);
	_InitCursor(fCursorCrossHair, kCursorCrossHairBits,
		B_CURSOR_ID_CROSS_HAIR, BPoint(10, 10));
	_InitCursor(fCursorFollowLink, kCursorFollowLinkBits,
		B_CURSOR_ID_FOLLOW_LINK, BPoint(5, 0));
	_InitCursor(fCursorGrab, kCursorGrabBits,
		B_CURSOR_ID_GRAB, kHandHotspot);
	_InitCursor(fCursorGrabbing, kCursorGrabbingBits,
		B_CURSOR_ID_GRABBING, kHandHotspot);
	_InitCursor(fCursorHelp, kCursorHelpBits,
		B_CURSOR_ID_HELP, BPoint(0, 8));
	_InitCursor(fCursorIBeam, kCursorIBeamBits,
		B_CURSOR_ID_I_BEAM, BPoint(7, 9));
	_InitCursor(fCursorIBeamHorizontal, kCursorIBeamHorizontalBits,
		B_CURSOR_ID_I_BEAM_HORIZONTAL, BPoint(8, 8));
	_InitCursor(fCursorMove, kCursorMoveBits,
		B_CURSOR_ID_MOVE, kResizeHotspot);
	_InitCursor(fCursorNoCursor, 0, B_CURSOR_ID_NO_CURSOR, BPoint(0, 0));
	_InitCursor(fCursorNotAllowed, kCursorNotAllowedBits,
		B_CURSOR_ID_NOT_ALLOWED, BPoint(8, 8));
	_InitCursor(fCursorProgress, kCursorProgressBits,
		B_CURSOR_ID_PROGRESS, BPoint(7, 10));
	_InitCursor(fCursorResizeEast, kCursorResizeEastBits,
		B_CURSOR_ID_RESIZE_EAST, kResizeHotspot);
	_InitCursor(fCursorResizeEastWest, kCursorResizeEastWestBits,
		B_CURSOR_ID_RESIZE_EAST_WEST, kResizeHotspot);
	_InitCursor(fCursorResizeNorth, kCursorResizeNorthBits,
		B_CURSOR_ID_RESIZE_NORTH, kResizeHotspot);
	_InitCursor(fCursorResizeNorthEast, kCursorResizeNorthEastBits,
		B_CURSOR_ID_RESIZE_NORTH_EAST, kResizeHotspot);
	_InitCursor(fCursorResizeNorthEastSouthWest,
		kCursorResizeNorthEastSouthWestBits,
		B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST, kResizeHotspot);
	_InitCursor(fCursorResizeNorthSouth, kCursorResizeNorthSouthBits,
		B_CURSOR_ID_RESIZE_NORTH_SOUTH, kResizeHotspot);
	_InitCursor(fCursorResizeNorthWest, kCursorResizeNorthWestBits,
		B_CURSOR_ID_RESIZE_NORTH_WEST, kResizeHotspot);
	_InitCursor(fCursorResizeNorthWestSouthEast,
		kCursorResizeNorthWestSouthEastBits,
		B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST, kResizeHotspot);
	_InitCursor(fCursorResizeSouth, kCursorResizeSouthBits,
		B_CURSOR_ID_RESIZE_SOUTH, kResizeHotspot);
	_InitCursor(fCursorResizeSouthEast, kCursorResizeSouthEastBits,
		B_CURSOR_ID_RESIZE_SOUTH_EAST, kResizeHotspot);
	_InitCursor(fCursorResizeSouthWest, kCursorResizeSouthWestBits,
		B_CURSOR_ID_RESIZE_SOUTH_WEST, kResizeHotspot);
	_InitCursor(fCursorResizeWest, kCursorResizeWestBits,
		B_CURSOR_ID_RESIZE_WEST, kResizeHotspot);
	_InitCursor(fCursorZoomIn, kCursorZoomInBits,
		B_CURSOR_ID_ZOOM_IN, BPoint(6, 6));
	_InitCursor(fCursorZoomOut, kCursorZoomOutBits,
		B_CURSOR_ID_ZOOM_OUT, BPoint(6, 6));
}


//! Does all the teardown
CursorManager::~CursorManager()
{
	for (int32 i = 0; i < fCursorList.CountItems(); i++) {
		ServerCursor* cursor = ((ServerCursor*)fCursorList.ItemAtFast(i));
		cursor->fManager = NULL;
		cursor->ReleaseReference();
	}
}


ServerCursor*
CursorManager::CreateCursor(team_id clientTeam, const uint8* cursorData)
{
	if (!Lock())
		return NULL;

	ServerCursorReference cursor(_FindCursor(clientTeam, cursorData), false);

	if (!cursor) {
		cursor.SetTo(new (std::nothrow) ServerCursor(cursorData), true);
		if (cursor) {
			cursor->SetOwningTeam(clientTeam);
			if (AddCursor(cursor) < B_OK)
				cursor = NULL;
		}
	}

	Unlock();

	return cursor.Detach();
}


ServerCursor*
CursorManager::CreateCursor(team_id clientTeam, BRect r, color_space format,
	int32 flags, BPoint hotspot, int32 bytesPerRow)
{
	if (!Lock())
		return NULL;

	ServerCursor* cursor = new (std::nothrow) ServerCursor(r, format, flags,
		hotspot, bytesPerRow);
	if (cursor != NULL) {
		cursor->SetOwningTeam(clientTeam);
		if (AddCursor(cursor) < B_OK) {
			delete cursor;
			cursor = NULL;
		}
	}

	Unlock();

	return cursor;
}


/*!	\brief Registers a cursor with the manager.
	\param cursor ServerCursor object to register
	\return The token assigned to the cursor or B_ERROR if cursor is NULL
*/
int32
CursorManager::AddCursor(ServerCursor* cursor, int32 token)
{
	if (!cursor)
		return B_BAD_VALUE;
	if (!Lock())
		return B_ERROR;

	if (!fCursorList.AddItem(cursor)) {
		Unlock();
		return B_NO_MEMORY;
	}

	if (token == -1)
		token = fTokenSpace.NewToken(kCursorToken, cursor);
	else
		fTokenSpace.SetToken(token, kCursorToken, cursor);

	cursor->fToken = token;
	cursor->AttachedToManager(this);

	Unlock();

	return token;
}


/*!	\brief Removes a cursor if it's not referenced anymore.

	If this was the last reference to this cursor, it will be deleted.
	Only if the cursor is deleted, \c true is returned.
*/
bool
CursorManager::RemoveCursor(ServerCursor* cursor)
{
	if (!Lock())
		return false;

	// TODO: this doesn't work as it looks like, and it's not safe!
	if (cursor->CountReferences() > 0) {
		// cursor has been referenced again in the mean time
		Unlock();
		return false;
	}

	_RemoveCursor(cursor);

	Unlock();
	return true;
}


/*!	\brief Removes and deletes all of an application's cursors
	\param signature Signature to which the cursors belong
*/
void
CursorManager::DeleteCursors(team_id team)
{
	if (!Lock())
		return;

	for (int32 index = fCursorList.CountItems(); index-- > 0;) {
		ServerCursor* cursor = (ServerCursor*)fCursorList.ItemAtFast(index);
		if (cursor->OwningTeam() == team)
			cursor->ReleaseReference();
	}

	Unlock();
}


/*!	\brief Sets all the cursors from a specified CursorSet
	\param path Path to the cursor set

	All cursors in the set will be assigned. If the set does not specify a
	cursor for a particular cursor specifier, it will remain unchanged.
	This function will fail if passed a NULL path, an invalid path, or the
	path to a non-CursorSet file.
*/
void
CursorManager::SetCursorSet(const char* path)
{
	BAutolock locker (this);

	CursorSet cursorSet(NULL);

	if (!path || cursorSet.Load(path) != B_OK)
		return;

	_LoadCursor(fCursorSystemDefault, cursorSet, B_CURSOR_ID_SYSTEM_DEFAULT);
	_LoadCursor(fCursorContextMenu, cursorSet, B_CURSOR_ID_CONTEXT_MENU);
	_LoadCursor(fCursorCopy, cursorSet, B_CURSOR_ID_COPY);
	_LoadCursor(fCursorCreateLink, cursorSet, B_CURSOR_ID_CREATE_LINK);
	_LoadCursor(fCursorCrossHair, cursorSet, B_CURSOR_ID_CROSS_HAIR);
	_LoadCursor(fCursorFollowLink, cursorSet, B_CURSOR_ID_FOLLOW_LINK);
	_LoadCursor(fCursorGrab, cursorSet, B_CURSOR_ID_GRAB);
	_LoadCursor(fCursorGrabbing, cursorSet, B_CURSOR_ID_GRABBING);
	_LoadCursor(fCursorHelp, cursorSet, B_CURSOR_ID_HELP);
	_LoadCursor(fCursorIBeam, cursorSet, B_CURSOR_ID_I_BEAM);
	_LoadCursor(fCursorIBeamHorizontal, cursorSet,
		B_CURSOR_ID_I_BEAM_HORIZONTAL);
	_LoadCursor(fCursorMove, cursorSet, B_CURSOR_ID_MOVE);
	_LoadCursor(fCursorNotAllowed, cursorSet, B_CURSOR_ID_NOT_ALLOWED);
	_LoadCursor(fCursorProgress, cursorSet, B_CURSOR_ID_PROGRESS);
	_LoadCursor(fCursorResizeEast, cursorSet, B_CURSOR_ID_RESIZE_EAST);
	_LoadCursor(fCursorResizeEastWest, cursorSet,
		B_CURSOR_ID_RESIZE_EAST_WEST);
	_LoadCursor(fCursorResizeNorth, cursorSet, B_CURSOR_ID_RESIZE_NORTH);
	_LoadCursor(fCursorResizeNorthEast, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_EAST);
	_LoadCursor(fCursorResizeNorthEastSouthWest, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST);
	_LoadCursor(fCursorResizeNorthSouth, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_SOUTH);
	_LoadCursor(fCursorResizeNorthWest, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_WEST);
	_LoadCursor(fCursorResizeNorthWestSouthEast, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST);
	_LoadCursor(fCursorResizeSouth, cursorSet, B_CURSOR_ID_RESIZE_SOUTH);
	_LoadCursor(fCursorResizeSouthEast, cursorSet,
		B_CURSOR_ID_RESIZE_SOUTH_EAST);
	_LoadCursor(fCursorResizeSouthWest, cursorSet,
		B_CURSOR_ID_RESIZE_SOUTH_WEST);
	_LoadCursor(fCursorResizeWest, cursorSet, B_CURSOR_ID_RESIZE_WEST);
	_LoadCursor(fCursorZoomIn, cursorSet, B_CURSOR_ID_ZOOM_IN);
	_LoadCursor(fCursorZoomOut, cursorSet, B_CURSOR_ID_ZOOM_OUT);
}


/*!	\brief Acquire the cursor which is used for a particular system cursor
	\param which Which system cursor to get
	\return Pointer to the particular cursor used or NULL if which is
	invalid or the cursor has not been assigned
*/
ServerCursor*
CursorManager::GetCursor(BCursorID which)
{
	BAutolock locker(this);

	switch (which) {
		case B_CURSOR_ID_SYSTEM_DEFAULT:
			return fCursorSystemDefault;
		case B_CURSOR_ID_CONTEXT_MENU:
			return fCursorContextMenu;
		case B_CURSOR_ID_COPY:
			return fCursorCopy;
		case B_CURSOR_ID_CREATE_LINK:
			return fCursorCreateLink;
		case B_CURSOR_ID_CROSS_HAIR:
			return fCursorCrossHair;
		case B_CURSOR_ID_FOLLOW_LINK:
			return fCursorFollowLink;
		case B_CURSOR_ID_GRAB:
			return fCursorGrab;
		case B_CURSOR_ID_GRABBING:
			return fCursorGrabbing;
		case B_CURSOR_ID_HELP:
			return fCursorHelp;
		case B_CURSOR_ID_I_BEAM:
			return fCursorIBeam;
		case B_CURSOR_ID_I_BEAM_HORIZONTAL:
			return fCursorIBeamHorizontal;
		case B_CURSOR_ID_MOVE:
			return fCursorMove;
		case B_CURSOR_ID_NO_CURSOR:
			return fCursorNoCursor;
		case B_CURSOR_ID_NOT_ALLOWED:
			return fCursorNotAllowed;
		case B_CURSOR_ID_PROGRESS:
			return fCursorProgress;
		case B_CURSOR_ID_RESIZE_EAST:
			return fCursorResizeEast;
		case B_CURSOR_ID_RESIZE_EAST_WEST:
			return fCursorResizeEastWest;
		case B_CURSOR_ID_RESIZE_NORTH:
			return fCursorResizeNorth;
		case B_CURSOR_ID_RESIZE_NORTH_EAST:
			return fCursorResizeNorthEast;
		case B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST:
			return fCursorResizeNorthEastSouthWest;
		case B_CURSOR_ID_RESIZE_NORTH_SOUTH:
			return fCursorResizeNorthSouth;
		case B_CURSOR_ID_RESIZE_NORTH_WEST:
			return fCursorResizeNorthWest;
		case B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST:
			return fCursorResizeNorthWestSouthEast;
		case B_CURSOR_ID_RESIZE_SOUTH:
			return fCursorResizeSouth;
		case B_CURSOR_ID_RESIZE_SOUTH_EAST:
			return fCursorResizeSouthEast;
		case B_CURSOR_ID_RESIZE_SOUTH_WEST:
			return fCursorResizeSouthWest;
		case B_CURSOR_ID_RESIZE_WEST:
			return fCursorResizeWest;
		case B_CURSOR_ID_ZOOM_IN:
			return fCursorZoomIn;
		case B_CURSOR_ID_ZOOM_OUT:
			return fCursorZoomOut;

		default:
			return NULL;
	}
}


/*!	\brief Internal function which finds the cursor with a particular ID
	\param token ID of the cursor to find
	\return The cursor or NULL if not found
*/
ServerCursor*
CursorManager::FindCursor(int32 token)
{
	if (!Lock())
		return NULL;

	ServerCursor* cursor;
	if (fTokenSpace.GetToken(token, kCursorToken, (void**)&cursor) != B_OK)
		cursor = NULL;

	Unlock();

	return cursor;
}


/*!	\brief Initializes a predefined system cursor.

	This method must only be called in the CursorManager's constructor,
	as it may throw exceptions.
*/
void
CursorManager::_InitCursor(ServerCursor*& cursorMember,
	const uint8* cursorBits, BCursorID id, const BPoint& hotSpot)
{
	if (cursorBits) {
		cursorMember = new ServerCursor(cursorBits, kCursorWidth,
			kCursorHeight, kCursorFormat);
	} else
		cursorMember = new ServerCursor(kCursorNoCursor, 1, 1, kCursorFormat);

	cursorMember->SetHotSpot(hotSpot);
	AddCursor(cursorMember, id);
}


void
CursorManager::_LoadCursor(ServerCursor*& cursorMember, const CursorSet& set,
	BCursorID id)
{
	ServerCursor* cursor;
	if (set.FindCursor(id, &cursor) == B_OK) {
		int32 index = fCursorList.IndexOf(cursorMember);
		if (index >= 0) {
			ServerCursor** items = reinterpret_cast<ServerCursor**>(
				fCursorList.Items());
			items[index] = cursor;
		}
		delete cursorMember;
		cursorMember = cursor;
	}
}


ServerCursor*
CursorManager::_FindCursor(team_id clientTeam, const uint8* cursorData)
{
	int32 count = fCursorList.CountItems();
	for (int32 i = 0; i < count; i++) {
		ServerCursor* cursor = (ServerCursor*)fCursorList.ItemAtFast(i);
		if (cursor->OwningTeam() == clientTeam
			&& cursor->CursorData()
			&& memcmp(cursor->CursorData(), cursorData, 68) == 0) {
			return cursor;
		}
	}
	return NULL;
}


void
CursorManager::_RemoveCursor(ServerCursor* cursor)
{
	fCursorList.RemoveItem(cursor);
	fTokenSpace.RemoveToken(cursor->fToken);
}
