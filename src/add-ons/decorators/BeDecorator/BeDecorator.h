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
//	File Name:		BeDecorator.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Fallback decorator for the app_server
//  
//------------------------------------------------------------------------------
#ifndef _DEFAULT_DECORATOR_H_
#define _DEFAULT_DECORATOR_H_

#include "Decorator.h"
#include <Region.h>

class BeDecorator: public Decorator
{
public:
	BeDecorator(BRect frame, int32 wlook, int32 wfeel, int32 wflags);
	~BeDecorator(void);
	
	void MoveBy(float x, float y);
	void MoveBy(BPoint pt);
	void Draw(BRect r);
	void Draw(void);
	void GetFootprint(BRegion *region);
	click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);
protected:
	void _DrawClose(BRect r);
	void _DrawFrame(BRect r);
	void _DrawTab(BRect r);
	void _DrawTitle(BRect r);
	void _DrawZoom(BRect r);
	void _DoLayout(void);
	void _SetFocus(void);
	void _SetColors(void);
	void DrawBlendedRect(BRect r, bool down);
	uint32 taboffset;

	RGBColor tab_highcol, tab_lowcol;
	RGBColor button_highcol, button_lowcol;
	RGBColor frame_highcol, frame_midcol, frame_lowcol, frame_highercol,
		frame_lowercol;
	RGBColor textcol;

	RGBColor *framecolors;
	
	// Individual rects for handling window frame rendering the proper way
	BRect rightborder,leftborder,topborder,bottomborder;
	uint64 solidhigh, solidlow;
	
	int32 borderwidth;

	bool slidetab;
	int textoffset;
	float titlepixelwidth;
};

#endif
