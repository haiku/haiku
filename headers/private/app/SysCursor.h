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
//	File Name:		SysCursor.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Private file encapsulating OBOS system cursor API
//  
//------------------------------------------------------------------------------
#ifndef SYSCURSOR_H_
#define SYSCURSOR_H_

#include <Bitmap.h>
#include <Cursor.h>
#include <Message.h>

typedef enum
{
	B_CURSOR_DEFAULT=0,
	B_CURSOR_TEXT,
	B_CURSOR_MOVE,
	B_CURSOR_DRAG,
	B_CURSOR_RESIZE,
	B_CURSOR_RESIZE_NWSE,
	B_CURSOR_RESIZE_NESW,
	B_CURSOR_RESIZE_NS,
	B_CURSOR_RESIZE_EW,
	B_CURSOR_APP,
	B_CURSOR_OTHER,
	B_CURSOR_INVALID
} cursor_which;

class ServerCursor;

void set_syscursor(cursor_which which, const BCursor *cursor);
void set_syscursor(cursor_which which, const BBitmap *bitmap);

cursor_which get_syscursor(void);

void setcursor(cursor_which which);

const char *CursorWhichToString(cursor_which which);
BBitmap *CursorDataToBitmap(int8 *data);

/*!
	\brief Class to manage system cursor sets
*/
class CursorSet : public BMessage
{
public:
	CursorSet(const char *name);
	status_t Save(const char *path,int32 saveflags=0);
	status_t Load(const char *path);
	status_t AddCursor(cursor_which which,const BBitmap *cursor, const BPoint &hotspot);
	status_t AddCursor(cursor_which which, int8 *data);
	void RemoveCursor(cursor_which which);
	status_t FindCursor(cursor_which which, BBitmap **cursor, BPoint *hotspot);
	status_t FindCursor(cursor_which which, ServerCursor **cursor);
	void Rename(const char *name);
};


#endif
