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
//	File Name:		ColorWell.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Color display class which accepts drops
//  
//------------------------------------------------------------------------------
#ifndef COLORWELL_H_
#define COLORWELL_H_

#include <View.h>
#include <Message.h>
#include <Invoker.h>

class ColorWell : public BView
{
public:
	ColorWell(BRect frame, BMessage *msg, bool is_rectangle=false);
	~ColorWell(void);
	void SetColor(rgb_color col);
	rgb_color Color(void) const;
	void SetColor(uint8 r,uint8 g, uint8 b);
	virtual void MessageReceived(BMessage *msg);
	virtual void Draw(BRect update);
	virtual void SetTarget(BHandler *tgt);
	virtual void SetEnabled(bool value);
	void SetMode(bool is_rectangle);
protected:
	BInvoker *invoker;	
	bool is_enabled, is_rect;
	rgb_color disabledcol, currentcol;
};

#endif
