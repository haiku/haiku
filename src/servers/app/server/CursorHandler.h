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
//	File Name:		CursorHandler.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	class for handling cursor display for DisplayDriver
//  
//------------------------------------------------------------------------------
#ifndef CURSORHANDLER_H
#define CURSORHANDLER_H

#include "GraphicsBuffer.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"

class DisplayDriver;

/*!
	\class CursorHandler CursorHandler.h
	\brief Object used by DisplayDriver to handle all the messiness of displaying the cursor
*/
class CursorHandler
{
public:
	CursorHandler(DisplayDriver *driver);
	~CursorHandler(void);
	
	bool IntersectsCursor(const BRect &r);
	
	void SetCursor(ServerCursor *cursor);
	ServerCursor *GetCursor(void) const { return fCursor; }
	
	void MoveTo(const BPoint &pt);
	BPoint GetPosition(void) const { return fPosition.LeftTop(); }
	
	void Hide(void);
	void Show(void);
	void Obscure(void);
	bool IsHidden(void) const { return (fHideLevel>0); }
	bool IsObscured(void) const { return fIsObscured; }
	
	void DriverHide(void);
	void DriverShow(void);
private:
	
	DisplayDriver *fDriver;

	BRect fOldPosition;
	BRect fPosition;
	BPoint fCursorPos;
	
	UtilityBitmap *fSavedData;
	ServerCursor *fCursor;
	int8 fHideLevel;
	
	bool fDriverHidden;
	bool fIsObscured;
	bool fValidSaveData;
};

#endif
