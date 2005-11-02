/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Gabe Yoder (gyoder@stny.rr.com)
 */

/**	BCursor describes a view-wide or application-wide cursor. */

/**
	@note:	As BeOS only supports 16x16 monochrome cursors, and I would like
			to see a nice shadowes one, we will need to extend this one.
 */

#include <AppDefs.h>

#include <Cursor.h>
#include <AppServerLink.h>
#include <ServerProtocol.h>


const BCursor *B_CURSOR_SYSTEM_DEFAULT;
const BCursor *B_CURSOR_I_BEAM;
	// these are initialized in BApplication::InitData()


BCursor::BCursor(const void *cursorData)
{
	int8 *data = (int8 *)cursorData;
	m_serverToken = 0;

	if (data == NULL
		|| data[0] != 16	// size
		|| data[1] != 1		// depth
		|| data[2] >= 16 || data[3] >= 16)	// hot-spot
		return;

	// Send data directly to server
	BPrivate::AppServerLink serverlink;
	int32 code = SERVER_FALSE;

	serverlink.StartMessage(AS_CREATE_BCURSOR);
	serverlink.Attach(cursorData, 68);
	serverlink.FlushWithReply(code);
	if (code == SERVER_TRUE)
		serverlink.Read<int32>(&m_serverToken);
}


// undefined on BeOS

BCursor::BCursor(BMessage *data)
{
	m_serverToken = 0;
}


BCursor::~BCursor()
{
	// Notify server to deallocate server-side objects for this cursor
	BPrivate::AppServerLink serverlink;
	serverlink.StartMessage(AS_DELETE_BCURSOR);
	serverlink.Attach<int32>(m_serverToken);
	serverlink.Flush();
}


// not implemented on BeOS

status_t BCursor::Archive(BMessage *into, bool deep) const
{
	return B_OK;
}


// not implemented on BeOS

BArchivable	*BCursor::Instantiate(BMessage *data)
{
	return NULL;
}


status_t
BCursor::Perform(perform_code d, void *arg)
{
  /*
	printf("perform %d\n", (int)d);
  */
	return B_OK;
}


void BCursor::_ReservedCursor1() {}
void BCursor::_ReservedCursor2() {}
void BCursor::_ReservedCursor3() {}
void BCursor::_ReservedCursor4() {}
