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
//	File Name:		Utils.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Miscellaneous utility functions
//
//------------------------------------------------------------------------------
#ifndef UTILS_H_
#define UTILS_H_

#include <Message.h>
#include <OS.h>
#include <Accelerant.h>

void SendMessage(port_id port, BMessage *message, int32 target=-1);
const char *MsgCodeToString(int32 code);
BString MsgCodeToBString(int32 code);
status_t ConvertModeToDisplayMode(uint32 mode, display_mode *dmode);
BRect CalculatePolygonBounds(BPoint *pts, int32 pointcount);
#endif
