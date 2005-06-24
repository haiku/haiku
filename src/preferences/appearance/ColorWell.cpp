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
//	File Name:		ColorWell.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Color display class which accepts drops
//  
//------------------------------------------------------------------------------
#include "ColorWell.h"

ColorWell::ColorWell(BRect frame, BMessage *msg, bool is_rectangle=false)
	: BView(frame,"ColorWell", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(0,0,0);
	invoker=new BInvoker(msg,this);
	disabledcol.red=128;
	disabledcol.green=128;
	disabledcol.blue=128;
	disabledcol.alpha=255;
	is_enabled=true;
	is_rect=is_rectangle;
}

ColorWell::~ColorWell(void)
{
	delete invoker;
}

void ColorWell::SetTarget(BHandler *tgt)
{
	invoker->SetTarget(tgt);
}

void ColorWell::SetColor(rgb_color col)
{
	SetHighColor(col);
	currentcol=col;
	Draw(Bounds());
//	Invalidate();
	invoker->Invoke();
}

void ColorWell::SetColor(uint8 r,uint8 g, uint8 b)
{
	SetHighColor(r,g,b);
	currentcol.red=r;
	currentcol.green=g;
	currentcol.blue=b;
	Draw(Bounds());
	//Invalidate();
	invoker->Invoke();
}

void ColorWell::MessageReceived(BMessage *msg)
{
	// If we received a dropped message, try to see if it has color data
	// in it
	if(msg->WasDropped())
	{
		rgb_color *col;
		uint8 *ptr;
		ssize_t size;
		if(msg->FindData("RGBColor",(type_code)'RGBC',(const void**)&ptr,&size)==B_OK)
		{
			col=(rgb_color*)ptr;
			SetHighColor(*col);
		}
	}

	// The default
	BView::MessageReceived(msg);
}

void ColorWell::SetEnabled(bool value)
{
	if(is_enabled!=value)
	{
		is_enabled=value;
		Invalidate();
	}
}

void ColorWell::Draw(BRect update)
{
	if(is_enabled)
		SetHighColor(currentcol);
	else
		SetHighColor(disabledcol);

	if(is_rect)
	{
		FillRect(Bounds());
		if(is_enabled)
		{
			BRect r(Bounds());
			SetHighColor(184,184,184);
			StrokeRect(r);
			
			SetHighColor(255,255,255);
			StrokeLine(BPoint(r.right,r.top+1), r.RightBottom());
			
			r.InsetBy(1,1);
			
			SetHighColor(216,216,216);
			StrokeLine(r.RightTop(), r.RightBottom());
			
			SetHighColor(96,96,96);
			StrokeLine(r.LeftTop(), r.RightTop());
			StrokeLine(r.LeftTop(), r.LeftBottom());

		}
	}
	else
	{
		FillEllipse(Bounds());
		if(is_enabled)
			StrokeEllipse(Bounds(),B_SOLID_LOW);
	}
}

rgb_color ColorWell::Color(void) const
{
	return currentcol;
}

void ColorWell::SetMode(bool is_rectangle)
{
	is_rect=is_rectangle;
}
