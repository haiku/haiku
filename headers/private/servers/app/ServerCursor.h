//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		ServerCursor.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Glorified ServerBitmap used for cursor work.
//  
//------------------------------------------------------------------------------
#ifndef SERVERCURSOR_H_
#define SERVERCURSOR_H_

#include <Point.h>
#include <String.h>
#include "ServerBitmap.h"

class ServerApp;
class CursorManager;

/*!
	\class ServerCursor ServerCursor.h
	\brief Class to handle all cursor capabilities for the system
	
	Although descended from ServerBitmaps, ServerCursors are not handled by
	the BitmapManager - they are allocated like any other object. Unlike BeOS 
	R5, cursors can be any size or color space, and this class accomodates and
	expands the R5 API.
*/
class ServerCursor : public ServerBitmap
{
public:
	ServerCursor(BRect r, color_space cspace, int32 flags, BPoint hotspot, int32 bytesperrow=-1, screen_id screen=B_MAIN_SCREEN_ID);
	ServerCursor(int8 *data);
	ServerCursor(const ServerCursor *cursor);
	~ServerCursor(void);
	
	//! Returns the cursor's hot spot
	BPoint GetHotSpot(void) { return fHotSpot; }
	void SetHotSpot(BPoint pt);
	const char *GetAppSignature(void) { return fAppSignature.String(); }
	void SetAppSignature(const char *signature);
	void SetOwningTeam(team_id tid) { fOwningTeam=tid; }
	team_id OwningTeam(void) const { return fOwningTeam; }
	
	//! Returns the cursor's ID
	int32 ID(void) { return fToken; }
private:
	friend class CursorManager;
	
	BPoint fHotSpot;
	team_id fOwningTeam;
	BString fAppSignature;
};

#endif
