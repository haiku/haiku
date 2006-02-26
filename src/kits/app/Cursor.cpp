/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Gabe Yoder (gyoder@stny.rr.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	BCursor describes a view-wide or application-wide cursor. */

/**
	@note:	As BeOS only supports 16x16 monochrome cursors, and I would like
			to see a nice shadowes one, we will need to extend this one.
 */

#include <AppDefs.h>
#include <Cursor.h>

#include <CursorSet.h>
#include <AppServerLink.h>
#include <ServerProtocol.h>


const BCursor *B_CURSOR_SYSTEM_DEFAULT;
const BCursor *B_CURSOR_I_BEAM;
	// these are initialized in BApplication::InitData()


BCursor::BCursor(const void *cursorData)
	:
	fServerToken(-1),
	fNeedToFree(false),
	fPendingViewCursor(false)
{
	const uint8 *data = (const uint8 *)cursorData;

	if (data == B_HAND_CURSOR || data == B_I_BEAM_CURSOR) {
		// just use the default cursors from the app_server
		fServerToken = data == B_HAND_CURSOR ?
			B_CURSOR_DEFAULT : B_CURSOR_TEXT;
		return;
	}

	// Create a new cursor in the app_server

	if (data == NULL
		|| data[0] != 16	// size
		|| data[1] != 1		// depth
		|| data[2] >= 16 || data[3] >= 16)	// hot-spot
		return;

	// Send data directly to server
	BPrivate::AppServerLink link;
	link.StartMessage(AS_CREATE_CURSOR);
	link.Attach(cursorData, 68);

	status_t status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<int32>(&fServerToken);
		fNeedToFree = true;
	}
}


BCursor::BCursor(BMessage *data)
{
	// undefined on BeOS
	fServerToken = -1;
	fNeedToFree = false;
	fPendingViewCursor = false;
}


BCursor::~BCursor()
{
	// Notify server to deallocate server-side objects for this cursor
	if (fNeedToFree) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_DELETE_CURSOR);
		link.Attach<int32>(fServerToken);
		link.Attach<bool>(fPendingViewCursor);
		link.Flush();
	}
}


status_t
BCursor::Archive(BMessage *into, bool deep) const
{
	// not implemented on BeOS
	return B_OK;
}


BArchivable	*
BCursor::Instantiate(BMessage *data)
{
	// not implemented on BeOS
	return NULL;
}


status_t
BCursor::Perform(perform_code d, void *arg)
{
	return B_OK;
}


void BCursor::_ReservedCursor1() {}
void BCursor::_ReservedCursor2() {}
void BCursor::_ReservedCursor3() {}
void BCursor::_ReservedCursor4() {}
