/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */

/**	Handles the system's cursor infrastructure */


#include <Directory.h>
#include <String.h>

#include "CursorData.h"
#include "CursorManager.h"
#include "HaikuSystemCursor.h"
#include "ServerCursor.h"
#include "ServerConfig.h"
#include "ServerTokenSpace.h"


//! Initializes the CursorManager
CursorManager::CursorManager()
 :	BLocker("CursorManager")
{
	// Set system cursors to "unassigned"
	// ToDo: decide about default cursor

#if 1
	fDefaultCursor = new ServerCursor(kHaikuCursorBits, kHaikuCursorWidth,
		kHaikuCursorHeight, kHaikuCursorFormat);
	// we just happen to know where the hotspot is
	fDefaultCursor->SetHotSpot(BPoint(1, 0));
#else
	fDefaultCursor = new ServerCursor(default_cursor_data);
	AddCursor(fDefaultCursor);
#endif

	fTextCursor = new ServerCursor(default_text_data);
	AddCursor(fTextCursor);

	fMoveCursor = new ServerCursor(default_move_data);
	AddCursor(fMoveCursor);

	fDragCursor = new ServerCursor(default_drag_data);
	AddCursor(fDragCursor);

	fResizeCursor = new ServerCursor(default_resize_data);
	AddCursor(fResizeCursor);

	fNWSECursor = new ServerCursor(default_resize_nwse_data);
	AddCursor(fNWSECursor);

	fNESWCursor = new ServerCursor(default_resize_nesw_data);
	AddCursor(fNESWCursor);

	fNSCursor = new ServerCursor(default_resize_ns_data);
	AddCursor(fNSCursor);

	fEWCursor = new ServerCursor(default_resize_ew_data);
	AddCursor(fEWCursor);
}


//! Does all the teardown
CursorManager::~CursorManager()
{
	for (int32 i = 0; i < fCursorList.CountItems(); i++) {
		ServerCursor *cursor = (ServerCursor *)fCursorList.ItemAt(i);
		if (cursor)
			delete cursor;
	}

	// Note that it is not necessary to remove and delete the system
	// cursors. These cursors are kept in the list with a empty application
	// signature so they cannot be removed very easily except via
	// SetCursor(cursor_which). At shutdown, they are removed with the 
	// above loop.
}


/*!
	\brief Registers a cursor with the manager.
	\param sc ServerCursor object to register
	\return The token assigned to the cursor or B_ERROR if sc is NULL
*/
int32
CursorManager::AddCursor(ServerCursor* cursor)
{
	if (!cursor)
		return B_ERROR;

	Lock();

	fCursorList.AddItem(cursor);
	int32 token = fTokenSpace.NewToken(B_SERVER_TOKEN, cursor);
	cursor->fToken = token;

	Unlock();
	return token;
}


/*!
	\brief Removes a cursor from the internal list and deletes it
	\param token ID value of the cursor to be deleted
	
	If the cursor is not found, this call does nothing
*/
void
CursorManager::DeleteCursor(int32 token)
{
	Lock();

	for (int32 i = 0; i < fCursorList.CountItems(); i++) {
		ServerCursor *cursor = (ServerCursor *)fCursorList.ItemAt(i);

		if (cursor && cursor->fToken == token) {
			fCursorList.RemoveItem(i);
			delete cursor;
			break;
		}
	}

	Unlock();
}


/*!
	\brief Removes and deletes all of an application's cursors
	\param signature Signature to which the cursors belong
*/
void
CursorManager::RemoveAppCursors(team_id team)
{
	Lock();

	int32 index = 0;
	while (index < fCursorList.CountItems()) {
		ServerCursor *cursor = (ServerCursor *)fCursorList.ItemAt(index);
		if (!cursor)
			break;

		if (cursor->OwningTeam() == team) {
			fCursorList.RemoveItem(index);
			delete cursor;
		} else
			index++;
	}

	Unlock();
}


/*!
	\brief Sets all the cursors from a specified CursorSet
	\param path Path to the cursor set
	
	All cursors in the set will be assigned. If the set does not specify a
	cursor for a particular cursor specifier, it will remain unchanged.
	This function will fail if passed a NULL path, an invalid path, or the
	path to a non-CursorSet file.
*/
void
CursorManager::SetCursorSet(const char *path)
{
	Lock();
	CursorSet cursorSet(NULL);

	if (!path || cursorSet.Load(path) != B_OK)
		return;

	ServerCursor *cursor = NULL;

	if (cursorSet.FindCursor(B_CURSOR_DEFAULT, &cursor) == B_OK) {
		if (fDefaultCursor)
			delete fDefaultCursor;
		fDefaultCursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_TEXT, &cursor) == B_OK) {
		if (fTextCursor)
			delete fTextCursor;
		fTextCursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_MOVE, &cursor) == B_OK) {
		if (fMoveCursor)
			delete fMoveCursor;
		fMoveCursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_DRAG, &cursor) == B_OK) {
		if (fDragCursor)
			delete fDragCursor;
		fDragCursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_RESIZE, &cursor) == B_OK) {
		if (fResizeCursor)
			delete fResizeCursor;
		fResizeCursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_RESIZE_NWSE, &cursor) == B_OK) {
		if (fNWSECursor)
			delete fNWSECursor;
		fNWSECursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_RESIZE_NESW, &cursor) == B_OK) {
		if (fNESWCursor)
			delete fNESWCursor;
		fNESWCursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_RESIZE_NS, &cursor) == B_OK) {
		if (fNSCursor)
			delete fNSCursor;
		fNSCursor = cursor;
	}

	if (cursorSet.FindCursor(B_CURSOR_RESIZE_EW, &cursor) == B_OK) {
		if (fEWCursor)
			delete fEWCursor;
		fEWCursor = cursor;
	}

	Unlock();
}


/*!
	\brief Acquire the cursor which is used for a particular system cursor
	\param which Which system cursor to get
	\return Pointer to the particular cursor used or NULL if which is 
	invalid or the cursor has not been assigned
*/
ServerCursor *
CursorManager::GetCursor(cursor_which which)
{
	ServerCursor *cursor = NULL;
	Lock();

	switch (which) {
		case B_CURSOR_DEFAULT:
			cursor = fDefaultCursor;
			break;

		case B_CURSOR_TEXT:
			cursor = fTextCursor;
			break;

		case B_CURSOR_MOVE:
			cursor = fMoveCursor;
			break;

		case B_CURSOR_DRAG:
			cursor = fDragCursor;
			break;

		case B_CURSOR_RESIZE:
			cursor = fResizeCursor;
			break;

		case B_CURSOR_RESIZE_NWSE:
			cursor = fNWSECursor;
			break;

		case B_CURSOR_RESIZE_NESW:
			cursor = fNESWCursor;
			break;

		case B_CURSOR_RESIZE_NS:
			cursor = fNSCursor;
			break;

		case B_CURSOR_RESIZE_EW:
			cursor = fEWCursor;
			break;

		default:
			break;
	}

	Unlock();
	return cursor;
}


/*!
	\brief Gets the current system cursor value
	\return The current cursor value or CURSOR_OTHER if some non-system cursor
*/
cursor_which
CursorManager::GetCursorWhich()
{
	Lock();

	// ToDo: Where is fCurrentWhich set?
	cursor_which which;
	which = fCurrentWhich;

	Unlock();
	return which;
}


/*!
	\brief Sets the specified system cursor to the a particular cursor
	\param which Which system cursor to change
	\param token The ID of the cursor to become the new one
	
	A word of warning: once a cursor has been assigned to the system, the
	system will take ownership of the cursor and deleting the cursor
	will have no effect on the system.
*/
void
CursorManager::ChangeCursor(cursor_which which, int32 token)
{
	Lock();

	// Find the cursor, based on the token
	ServerCursor *cursor = FindCursor(token);

	// Did we find a cursor with this token?
	if (!cursor) {
		Unlock();
		return;
	}

	// Do the assignment
	switch (which) {
		case B_CURSOR_DEFAULT:
			delete fDefaultCursor;
			fDefaultCursor = cursor;
			break;

		case B_CURSOR_TEXT:
			delete fTextCursor;
			fTextCursor = cursor;
			break;

		case B_CURSOR_MOVE:
			delete fMoveCursor;
			fMoveCursor = cursor;
			break;

		case B_CURSOR_DRAG:
			delete fDragCursor;
			fDragCursor = cursor;
			break;

		case B_CURSOR_RESIZE:
			delete fResizeCursor;
			fResizeCursor = cursor;
			break;

		case B_CURSOR_RESIZE_NWSE:
			delete fNWSECursor;
			fNWSECursor = cursor;
			break;

		case B_CURSOR_RESIZE_NESW:
			delete fNESWCursor;
			fNESWCursor = cursor;
			break;

		case B_CURSOR_RESIZE_NS:
			delete fNSCursor;
			fNSCursor = cursor;
			break;

		case B_CURSOR_RESIZE_EW:
			delete fEWCursor;
			fEWCursor = cursor;
			break;

		default:
			Unlock();
			return;
	}

	if (cursor->GetAppSignature())
		cursor->SetAppSignature("");

	fCursorList.RemoveItem(cursor);
	Unlock();
}


/*!
	\brief Internal function which finds the cursor with a particular ID
	\param token ID of the cursor to find
	\return The cursor or NULL if not found
*/
ServerCursor *
CursorManager::FindCursor(int32 token)
{
	ServerCursor* cursor;
	if (gTokenSpace.GetToken(token, kCursorToken, (void**)&cursor) == B_OK)
		return cursor;

	return NULL;
}


//! Sets the cursors to the defaults and saves them to CURSOR_SETTINGS_DIR/"d
void
CursorManager::SetDefaults()
{
	Lock();
	CursorSet cursorSet("Default");
	cursorSet.AddCursor(B_CURSOR_DEFAULT, default_cursor_data);
	cursorSet.AddCursor(B_CURSOR_TEXT, default_text_data);
	cursorSet.AddCursor(B_CURSOR_MOVE, default_move_data);
	cursorSet.AddCursor(B_CURSOR_DRAG, default_drag_data);
	cursorSet.AddCursor(B_CURSOR_RESIZE, default_resize_data);
	cursorSet.AddCursor(B_CURSOR_RESIZE_NWSE, default_resize_nwse_data);
	cursorSet.AddCursor(B_CURSOR_RESIZE_NESW, default_resize_nesw_data);
	cursorSet.AddCursor(B_CURSOR_RESIZE_NS, default_resize_ns_data);
	cursorSet.AddCursor(B_CURSOR_RESIZE_EW, default_resize_ew_data);

	BDirectory dir;
	if (dir.SetTo(CURSOR_SET_DIR) == B_ENTRY_NOT_FOUND)
		create_directory(CURSOR_SET_DIR, 0777);

	BString string(CURSOR_SET_DIR);
	string += "Default";
	cursorSet.Save(string.String(), B_CREATE_FILE | B_FAIL_IF_EXISTS);

	SetCursorSet(string.String());
	Unlock();
}
