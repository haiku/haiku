/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#include "ColorWell.h"

ColorWell::ColorWell(BRect frame, BMessage *msg, uint32 resizingMode, uint32 flags)
 :	BView(frame,"ColorWell", resizingMode, flags | B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(0,0,0);
	invoker=new BInvoker(msg,this);
	disabledcol.red=128;
	disabledcol.green=128;
	disabledcol.blue=128;
	disabledcol.alpha=255;
	is_enabled=true;
	is_rect = true;
}

ColorWell::~ColorWell(void)
{
	delete invoker;
}

void
ColorWell::SetTarget(BHandler *tgt)
{
	invoker->SetTarget(tgt);
}

void
ColorWell::SetColor(rgb_color col)
{
	SetHighColor(col);
	currentcol=col;
	Draw(Bounds());
	invoker->Invoke();
}

void
ColorWell::SetColor(uint8 r,uint8 g, uint8 b)
{
	SetHighColor(r,g,b);
	currentcol.red=r;
	currentcol.green=g;
	currentcol.blue=b;
	Draw(Bounds());
	invoker->Invoke();
}

void
ColorWell::MessageReceived(BMessage *msg)
{
	// If we received a dropped message, try to see if it has color data
	// in it
	if(msg->WasDropped()) {
		rgb_color *col;
		uint8 *ptr;
		ssize_t size;
		if(msg->FindData("RGBColor",(type_code)'RGBC',
			(const void**)&ptr,&size)==B_OK) {
			col=(rgb_color*)ptr;
			SetHighColor(*col);
		}
	}

	// The default
	BView::MessageReceived(msg);
}

void
ColorWell::SetEnabled(bool value)
{
	if(is_enabled!=value) {
		is_enabled=value;
		Invalidate();
	}
}

void
ColorWell::Draw(BRect update)
{
	rgb_color color;
	if (is_enabled)
		color = currentcol;
	else
		color = disabledcol;
	
	if(is_rect) {
		if(is_enabled) {
			BRect r(Bounds());
			SetHighColor(184,184,184);
			StrokeRect(r);
			
			SetHighColor(255,255,255);
			StrokeLine(BPoint(r.right, r.top+1), r.RightBottom());
			
			r.InsetBy(1,1);
			
			SetHighColor(216,216,216);
			StrokeLine(r.RightTop(), r.RightBottom());
			
			SetHighColor(96,96,96);
			StrokeLine(r.LeftTop(), r.RightTop());
			StrokeLine(r.LeftTop(), r.LeftBottom());
			
			r.InsetBy(1, 1);
			SetHighColor(color);
			FillRect(r);
		} else {
			SetHighColor(color);
			FillRect(Bounds());
		}
	}
	else {
		// fill background
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(update);
		
		SetHighColor(color);		
		FillEllipse(Bounds());
		if(is_enabled)
			StrokeEllipse(Bounds(),B_SOLID_LOW);
	}
}

rgb_color
ColorWell::Color(void) const
{
	return currentcol;
}

void
ColorWell::SetMode(bool is_rectangle)
{
	is_rect=is_rectangle;
}
