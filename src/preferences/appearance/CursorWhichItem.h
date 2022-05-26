//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		CursorWhichItem.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	ListItem class for managing cursor_which specifiers
//  
//------------------------------------------------------------------------------
#ifndef COLORWHICH_ITEM_H
#define COLORWHICH_ITEM_H

#include <InterfaceDefs.h>
#include <ListItem.h>
#include <Bitmap.h>
#include <SysCursor.h>

class CursorWhichItem : public BStringItem
{
public:
	CursorWhichItem(cursor_which which);
	~CursorWhichItem(void);
	void SetAttribute(cursor_which which);
	cursor_which GetAttribute(void);
	void SetBitmap(BBitmap *bmp) { if(image) delete image; image=bmp; }
	BBitmap *GetBitmap(void) const { return image; }
private:
	cursor_which attribute;
	BBitmap *image;
};

#endif
