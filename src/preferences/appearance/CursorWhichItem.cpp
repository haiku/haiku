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
//	File Name:		CursorWhichItem.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	ListItem class for managing cursor_which specifiers
//  
//------------------------------------------------------------------------------
#include "CursorWhichItem.h"
#include <stdio.h>

CursorWhichItem::CursorWhichItem(cursor_which which)
 : BStringItem(NULL,0,false)
{
	SetAttribute(which);
	image=NULL;
}

CursorWhichItem::~CursorWhichItem(void)
{
	// Empty, but exists for just-in-case
	if(image)
		delete image;
}

void CursorWhichItem::SetAttribute(cursor_which which)
{
	switch(which)
	{
		// cases not existing in R5 which exist in Haiku
		case B_CURSOR_DEFAULT:
		{
			attribute=which;
			SetText("Default");
			break;
		}
		case B_CURSOR_TEXT:
		{
			attribute=which;
			SetText("Text");
			break;
		}
		case B_CURSOR_MOVE:
		{
			attribute=which;
			SetText("Move");
			break;
		}
		case B_CURSOR_DRAG:
		{
			attribute=which;
			SetText("Drag");
			break;
		}
		case B_CURSOR_RESIZE:
		{
			attribute=which;
			SetText("Resize");
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			attribute=which;
			SetText("Diagonal Resize NW-SE");
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			attribute=which;
			SetText("Diagonal Resize NE-SW");
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			attribute=which;
			SetText("Vertical Resize");
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			attribute=which;
			SetText("Horizontal Resize");
			break;
		}
		case B_CURSOR_OTHER:
		{
			attribute=which;
			SetText("Other");
			break;
		}
		case B_CURSOR_INVALID:
		{
			break;
		}
		default:
		{
			printf("unknown code '%c%c%c%c'\n",(char)((which & 0xFF000000) >>  24),
				(char)((which & 0x00FF0000) >>  16),
				(char)((which & 0x0000FF00) >>  8),
				(char)((which & 0x000000FF)) );
			break;
		}
	}
}

cursor_which CursorWhichItem::GetAttribute(void)
{
	return attribute;
}

