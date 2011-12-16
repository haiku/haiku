/*
 * Copyright (c) 1999-2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <Alert.h>
#include <Deskbar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <stdio.h>

#include "WatchApp.h"
#include "WatchView.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "WatchView"


const rgb_color COLOR_FOREGROUND = { 0, 0, 0 };

///////////////////////////////////////////////////////////////////////////////

WatchView::WatchView()
	:
	BView(BRect(0, 0, 1, 1), 0, 0, 0)
{
}

///////////////////////////////////////////////////////////////////////////////

WatchView::WatchView(BMessage* message)
	: BView(
		BRect(0, 0, 15, 15), DESKBAR_ITEM_NAME, B_FOLLOW_NONE,
		B_WILL_DRAW | B_PULSE_NEEDED) 
{
	oldTime = 0;

	//SetFont(be_bold_font);
	SetFont(be_plain_font);
	
	// Calculate the maximum width for our replicant. Note that the Deskbar
	// accepts replicants that are wider than 16 pixels, but not replicants
	// that are higher than 16 pixels.

	float width = StringWidth("@999") + 4;
	ResizeTo(width, 15);

	SetViewColor(B_TRANSPARENT_COLOR);
}

///////////////////////////////////////////////////////////////////////////////

WatchView* WatchView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, REPLICANT_CLASS)) 
	{
		return new WatchView(archive);
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

status_t WatchView::Archive(BMessage* archive, bool deep) const
{
	super::Archive(archive, deep);
  
	archive->AddString("add_on", APP_SIGNATURE);
	archive->AddString("class", REPLICANT_CLASS);

	return B_OK;
}

///////////////////////////////////////////////////////////////////////////////

void WatchView::Draw(BRect updateRect)
{
	super::Draw(updateRect);

	char string[5];
	sprintf(string, "@%03ld", GetInternetTime());
	
	font_height height;         // center the text horizontally
	GetFontHeight(&height);     // and vertically in the Deskbar
	BRect rect = Bounds();
	float width = StringWidth(string);
	float x = 1 + (rect.Width() - width)/2;
	float y = height.ascent
			    + (rect.Height() - (height.ascent + height.descent))/2;

	SetHighColor(Parent()->ViewColor());
	FillRect(updateRect);

	SetLowColor(Parent()->ViewColor());
	SetHighColor(COLOR_FOREGROUND);
	SetDrawingMode(B_OP_OVER);
	DrawString(string, BPoint(x, y));
}

///////////////////////////////////////////////////////////////////////////////

void WatchView::MouseDown(BPoint point) 
{
	BPopUpMenu* menu = new BPopUpMenu("WatchView", false, false);

	menu->AddItem(new BMenuItem(
		B_TRANSLATE("About"), 
		new BMessage(B_ABOUT_REQUESTED)));

	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED)));

	menu->SetTargetForItems(this);

	ConvertToScreen(&point); 
	menu->Go(point, true, false, true); 

	delete menu;
}

///////////////////////////////////////////////////////////////////////////////

void WatchView::MessageReceived(BMessage* msg)
{
	switch (msg->what) 
	{
		case B_ABOUT_REQUESTED: OnAboutRequested();          break;
		case B_QUIT_REQUESTED:  OnQuitRequested();           break;
		default:                super::MessageReceived(msg); break;
	}
}

///////////////////////////////////////////////////////////////////////////////

void WatchView::OnAboutRequested()
{
	BString text = B_TRANSLATE("WebWatch %1"
		"\nAn internet time clock for your Deskbar\n\n"
		"Created by Matthijs Hollemans\n"
		"mahlzeit@users.sourceforge.net\n\n"
		"Thanks to Jason Parks for his help.\n");
	text.ReplaceFirst("%1", VERSION);
	(new BAlert(NULL, text.String(), B_TRANSLATE("OK")))->Go(NULL);
}

///////////////////////////////////////////////////////////////////////////////

void WatchView::OnQuitRequested()
{
	// According to the Be Book, we are not allowed to do this
	// since we're a view that's sitting on the Deskbar's shelf.
	// But it works just fine for me, and I see no other way to
	// make a Deskbar replicant quit itself.

	BDeskbar deskbar;
	deskbar.RemoveItem(DESKBAR_ITEM_NAME);
}

///////////////////////////////////////////////////////////////////////////////

void WatchView::Pulse()
{
	int32 currentTime = GetInternetTime();
	if (oldTime != currentTime)
	{
		oldTime = currentTime;
		Invalidate();
	}
}

///////////////////////////////////////////////////////////////////////////////

int32 WatchView::GetInternetTime()
{
	// First we get the current time as the number of seconds since
	// 1 January 1970 GMT and add an hour's worth of seconds to adjust
	// for the Biel Mean Time (BMT). Then we get the number of seconds
	// that have passed today, and divide it with the length of a beat
	// to get the number of elapsed beats.

	return (int32) (((real_time_clock() + 3600) % 86400) / 86.4);
}

///////////////////////////////////////////////////////////////////////////////
