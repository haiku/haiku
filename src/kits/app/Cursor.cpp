//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
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
//	File Name:		Cursor.cpp
//	Author:			Frans van Nispen (xlr8@tref.nl)
//					Gabe Yoder (gyoder@stny.rr.com)
//	Description:	BCursor describes a view-wide or application-wide cursor.
//------------------------------------------------------------------------------
/**
	@note:	As BeOS only supports 16x16 monochrome cursors, and I would like
			to see a nice shadowes one, we will need to extend this one.
 */

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <AppDefs.h>
#include <Cursor.h>
#include <PortLink.h>
#include <PortMessage.h>
#include <AppServerLink.h>

// Project Includes ------------------------------------------------------------
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
	BPrivate::BAppServerLink serverlink;
	PortMessage msg;

	serverlink.SetOpCode(AS_CREATE_BCURSOR);
	serverlink.Attach(cursorData, 68);
	serverlink.FlushWithReply(&msg);

	msg.Read<int32>(&m_serverToken);
}


// undefined on BeOS

BCursor::BCursor(BMessage *data)
{
	m_serverToken = 0;
}


BCursor::~BCursor()
{
	// Notify server to deallocate server-side objects for this cursor
	BPrivate::BAppServerLink serverlink;
	serverlink.SetOpCode(AS_DELETE_BCURSOR);
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
