//****************************************************************************************
//
//	File:		BottomPrefsView.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "BottomPrefsView.h"
#include "Common.h"
#include <interface/Button.h>
#include <stdlib.h>

BottomPrefsView::BottomPrefsView(BRect rect, const char *name) :
	BView(rect, name, B_FOLLOW_NONE, B_WILL_DRAW) {

	// Set the background color dynamically, don't hardcode 216, 216, 216
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect r = Bounds();
	r.right -= 13;
	float width = be_plain_font->StringWidth("OK") + 20;
	r.left = (width > 75) ? (r.right - width) : (r.right - 75);
	r.top += 10;
	r.bottom = r.top + 24;
	BButton *ok = new BButton(r, "OKButton", "OK", new BMessage(PRV_BOTTOM_OK));
	AddChild(ok);
	
	r.right = r.left - 10;
	width = be_plain_font->StringWidth("Defaults") + 20;
	r.left = (width > 75) ? (r.right - width) : (r.right - 75);
	BButton *defaults = new BButton(r, "DefaultsButton", "Defaults", new BMessage(PRV_BOTTOM_DEFAULTS));
	AddChild(defaults);
	
	ok->MakeDefault(true);
}

// Do a little drawing to fit better with BTabView
void BottomPrefsView::Draw(BRect rect) {
	PushState();
	
	BRect bounds = Bounds();
	SetHighColor(255, 255, 255, 255);
	StrokeLine(BPoint(0, 0), BPoint(bounds.right - 1, 0));
	StrokeLine(BPoint(0, 0), BPoint(0, bounds.bottom - 1));
	SetHighColor(96, 96, 96, 255);
	StrokeLine(BPoint(bounds.right, 0), BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(0, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	
	PopState();
}